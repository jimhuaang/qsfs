// +-------------------------------------------------------------------------
// | Copyright (C) 2017 Yunify, Inc.
// +-------------------------------------------------------------------------
// | Licensed under the Apache License, Version 2.0 (the "License");
// | You may not use this work except in compliance with the License.
// | You may obtain a copy of the License in the LICENSE file, or at:
// |
// | http://www.apache.org/licenses/LICENSE-2.0
// |
// | Unless required by applicable law or agreed to in writing, software
// | distributed under the License is distributed on an "AS IS" BASIS,
// | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// | See the License for the specific language governing permissions and
// | limitations under the License.
// +-------------------------------------------------------------------------

#include "filesystem/Drive.h"

#include <time.h>

#include <sys/types.h>

#include <future>  // NOLINT
#include <memory>
#include <mutex>  // NOLINT
#include <utility>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "base/Utils.h"
#include "client/Client.h"
#include "client/ClientError.h"
#include "client/ClientFactory.h"
#include "client/QSError.h"
#include "client/TransferManager.h"
#include "client/TransferManagerFactory.h"
#include "data/Cache.h"
#include "data/Directory.h"
#include "filesystem/Configure.h"

namespace QS {

namespace FileSystem {

using QS::Client::Client;
using QS::Client::ClientError;
using QS::Client::ClientFactory;
using QS::Client::GetMessageForQSError;
using QS::Client::IsGoodQSError;
using QS::Client::QSError;
using QS::Client::TransferManager;
using QS::Client::TransferManagerConfigure;
using QS::Client::TransferManagerFactory;
using QS::Data::Cache;
using QS::Data::ChildrenMultiMapConstIterator;
using QS::Data::DirectoryTree;
using QS::Data::FileMetaData;
using QS::Data::FilePathToNodeUnorderedMap;
using QS::Data::Node;
using QS::Exception::QSException;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetDirName;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using QS::Utils::IsRootDirectory;
using std::pair;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
using std::weak_ptr;

static std::unique_ptr<Drive> instance(nullptr);
static std::once_flag flag;

// --------------------------------------------------------------------------
Drive &Drive::Instance() {
  std::call_once(flag, [] { instance.reset(new Drive); });
  return *instance.get();
}

// --------------------------------------------------------------------------
Drive::Drive()
    : m_mountable(true),
      m_client(ClientFactory::Instance().MakeClient()),
      m_transferManager(std::move(
          TransferManagerFactory::Create(TransferManagerConfigure()))),
      m_cache(std::move(unique_ptr<Cache>(new Cache))) {
  uid_t uid = GetProcessEffectiveUserID();
  gid_t gid = GetProcessEffectiveGroupID();

  m_directoryTree = unique_ptr<DirectoryTree>(
      new DirectoryTree(time(NULL), uid, gid, Configure::GetRootMode()));
}

// --------------------------------------------------------------------------
void Drive::SetClient(shared_ptr<Client> client) { m_client = client; }

// --------------------------------------------------------------------------
void Drive::SetTransferManager(unique_ptr<TransferManager> transferManager) {
  m_transferManager = std::move(transferManager);
}

// --------------------------------------------------------------------------
void Drive::SetCache(unique_ptr<Cache> cache) { m_cache = std::move(cache); }

// --------------------------------------------------------------------------
void Drive::SetDirectoryTree(unique_ptr<DirectoryTree> dirTree) {
  m_directoryTree = std::move(dirTree);
}

// --------------------------------------------------------------------------
bool Drive::IsMountable() const {
  m_mountable.store(GetClient()->Connect());
  return m_mountable.load();
}

// --------------------------------------------------------------------------
shared_ptr<Node> Drive::GetRoot() {
  bool connectSuccess = GetClient()->Connect();
  if (!connectSuccess) {
    throw QSException("Unable to connect to object storage bucket");
  }
  return m_directoryTree->GetRoot();
}

// --------------------------------------------------------------------------
struct statvfs Drive::GetFilesystemStatistics() {
  struct statvfs statv;
  auto err = GetClient()->Statvfs(&statv);
  DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  return statv;
}

// --------------------------------------------------------------------------
pair<weak_ptr<Node>, bool> Drive::GetNode(const string &path,
                                          bool updateIfDirectory) {
  if (path.empty()) {
    Error("Null file path");
    return {weak_ptr<Node>(), false};
  }
  auto node = m_directoryTree->Find(path).lock();
  bool modified = false;

  auto UpdateNode = [this, &modified](const string &path,
                                      const shared_ptr<Node> &node) {
    time_t modifiedSince = 0;
    modifiedSince = const_cast<const Node &>(*node).GetEntry().GetMTime();
    auto err = GetClient()->Stat(path, modifiedSince, &modified);
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  };

  if (node) {
    UpdateNode(path, node);
  } else {
    auto err = GetClient()->Stat(path, 0, &modified);  // head it
    if (IsGoodQSError(err)) {
      node = m_directoryTree->Find(path).lock();
    } else {
      DebugError(GetMessageForQSError(err));
    }
  }

  // Update directory tree asynchornizely.
  if (node->IsDirectory() && updateIfDirectory && modified) {
    auto receivedHandler = [](const ClientError<QSError> &err) {
      DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
    };
    GetClient()->GetExecutor()->SubmitAsync(receivedHandler, [this, path] {
      return GetClient()->ListDirectory(AppendPathDelim(path));
    });
  }

  return {node, modified};
}

// --------------------------------------------------------------------------
pair<ChildrenMultiMapConstIterator, ChildrenMultiMapConstIterator>
Drive::GetChildren(const string &dirPath) {
  auto emptyRes = std::make_pair(m_directoryTree->CEndParentToChildrenMap(),
                                 m_directoryTree->CEndParentToChildrenMap());
  if (dirPath.empty()) {
    Error("Null dir path");
    return emptyRes;
  }

  auto path = dirPath;
  if (dirPath.back() != '/') {
    path = AppendPathDelim(dirPath);
    DebugInfo("Input dir path not ending with '/', append it");
  }

  auto res = GetNode(path, false);  // Do not invoke updating dirctory
                                    // as we will do it synchronizely
  auto node = res.first.lock();
  bool modified = res.second;
  if (node) {
    if (modified || node->IsEmpty()) {
      // Update directory tree synchornizely
      auto f = GetClient()->GetExecutor()->SubmitCallablePrioritized(
          [this, path] { return GetClient()->ListDirectory(path); });
      auto err = f.get();
      DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
    }
    return m_directoryTree->FindChildren(path);
  } else {
    DebugInfo("Directory is not existing for " + dirPath);
    return emptyRes;
  }
}

// --------------------------------------------------------------------------
void Drive::Chmod(const std::string &filePath, mode_t mode) {
  // TODO(jim): this need call sdk api of meta data
  // change meta mode
  // change ctime
}

// --------------------------------------------------------------------------
void Drive::Chown(const std::string &filePath, uid_t uid, gid_t gid) {
  // TODO(jim): this nedd call sdk api of meta
  // change meta uid gid
  // chang ctime
}

// --------------------------------------------------------------------------
void Drive::DeleteFile(const string &filePath) {
  // Delete a file.
  // first check if file exist in dir
  // call QSClient to delete which will update cache and dir
}

// --------------------------------------------------------------------------
void Drive::DeleteDir(const string &dirPath) {
  // Delete a emptyr diectory
}

// --------------------------------------------------------------------------
void Drive::Link(const string &filePath, const string &hardlinkPath) {
  // gdfs_link: ++entry->ref_count; node->insert(newNode(..,'h')); // locally
  //
}

// --------------------------------------------------------------------------
void Drive::MakeFile(const string &filePath, mode_t mode, dev_t dev) {
  /*   if(mode & S_IRFEG){
      // make regular file
      // new a fileMeta, ignore dev
    } else {
      // make non-directory, non-symlink file
      // new a fileMeta considering dev
    }
    else make symlink file
    */
  // call GetClient()->MakeFile(, mode_t mode, dev_t dev);
  // which will call m_directoryTree->Grow(fileMeta);
  // put object
  // TODO(jim): store symbolic link to Node, refer gdfs_mkmod, gdfs_getattr
}

// --------------------------------------------------------------------------
void Drive::MakeDir(const string &dirPath, mode_t mode) {
  // Call GetClient()->MakeDirectory();
  // whicl will call putObject to create a dir
  // and call m_directorytree->Grow(fileMata)
}

// --------------------------------------------------------------------------
void Drive::OpenFile(const string &filePath) {
  // call GetClient->HeadFile
  // call call GetClient->GetObject asynchornizeing download object
  // call m_directoryTree->Open a file to set entry with fileOpen = true
}

// --------------------------------------------------------------------------
void Drive::OpenDir(const string &dirPath) {
  //
}

// --------------------------------------------------------------------------
void Drive::ReadFile(const string &filePath, char *buf, size_t size,
                     off_t offset) {
  // memset(buf, 0, size);
  // call GetNode see if it changed
  // store file to cache synchornizely
  // call cache-Get()
  // Should change access time and call GetClient->PutObject
  // cal the size, use minmum

  // if node->IsEmpty(), return and debugInfo
}

// --------------------------------------------------------------------------
void Drive::RenameFile(const string &filePath, const string &newFilePath) {
  if (filePath.empty() || newFilePath.empty()) {
    DebugWarning("Invalid empty parameter");
    return;
  }
  if (IsRootDirectory(filePath)) {
    DebugError("Unable to rename root");
    return;
  }
  auto oldDir = GetDirName(filePath);
  auto newDir = GetDirName(newFilePath);
  if (oldDir.empty() || newDir.empty()) {
    DebugError("Input file paths have invalid dir path");
    return;
  }
  if (oldDir != newDir) {
    DebugError("Input file paths have different dir path [old file: " +
               filePath + ", new file: " + newFilePath + "]");
    return;
  }

  auto res = GetNode(filePath, false);  // Do not invoke updating dirctory
                                        // as we are changing it
  auto node = res.first.lock();
  if (node) {
    // Check parameter
    auto newPath = newFilePath;
    bool isDir = node->IsDirectory();
    if (isDir) {
      if (newFilePath.back() != '/') {
        DebugWarning(
            "New file path is not ending with '/' for a directory, appending "
            "it");
        newPath = AppendPathDelim(newFilePath);
      }
    } else {
      if (newFilePath.back() == '/') {
        DebugWarning(
            "New file path ending with '/' for a non directory, cut it");
        newPath.pop_back();
      }
    }

    // Do Renaming
    auto err = GetClient()->RenameFile(filePath, newPath);
    // Update meta and invoking updating directory tree asynchronizely
    if (IsGoodQSError(err)) {
      res = GetNode(newPath, true);
      node = res.first.lock();
      DebugErrorIf(!node, "Fail to rename file for " + filePath);
      return;
    } else {
      DebugError(GetMessageForQSError(err));
      return;
    }
  } else {
    DebugInfo("File is not existing for " + filePath);
    return;
  }
}

// --------------------------------------------------------------------------
void Drive::SymLink(const string &filePath, const string &linkPath) {
  // create a new Node in local directory tree
  // no need to put object
  // refer gdfs_symlik gdfs_mknod
}

// --------------------------------------------------------------------------
void Drive::TruncateFile(const string &filePath, size_t newSize) {
  // download file, truncate it, delete old file, and write it
  // TODO(jim): maybe we do not need this method, just call delete and write
  // node->SetNeedUpload(true);  // Mark upload

  // if newSize = 0,
  // if newSize > size, fill the hole
  // if newSize < size, resize it

  // update cached file
  // set entry->write = true; should do this in Drive
  // TODO(jim): any modification on diretory tree should be synchronized by its
  // api
}

// --------------------------------------------------------------------------
void Drive::UploadFile(const string &filePath) {
  //
  // if file size > 20M call tranfser to upload multipart
  // need a mechnasim to handle the memory insufficient situation
  // // set entry->write = fasle; // should do this in drive::
  // invoke putobject
  // or invoke multipart upload (first two step) transfer manager
  // and at end invoke complete mulitpart upload

  // at last entry->write = fasle; // should do this in drive::
  // node->SetNeedUpload(false);  // Done upload
}

// --------------------------------------------------------------------------
void Drive::WriteFile(const string &filePath, const char *buf, size_t size,
                      off_t offset) {
  // if entry->file size < size + offset, update the entry size with max one
  // Call Cache->Put to store fiel into cache and when overpass max size,
  // invoke multipart upload (first two step)
  // Finshed will be done in Release/ (here in Drive::UploadFile)

  // node->SetNeedUpload(true);  // Mark upload

  // create a write buffer if necceaary
  // handle hole if file
}

}  // namespace FileSystem
}  // namespace QS
