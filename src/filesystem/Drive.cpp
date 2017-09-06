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

#include <assert.h>
#include <stdint.h>
#include <time.h>

#include <sys/stat.h>
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
#include "client/TransferHandle.h"
#include "client/TransferManager.h"
#include "client/TransferManagerFactory.h"
#include "data/Cache.h"
#include "data/Directory.h"
#include "data/FileMetaData.h"
#include "data/IOStream.h"
#include "filesystem/Configure.h"

namespace QS {

namespace FileSystem {

using QS::Client::Client;
using QS::Client::ClientError;
using QS::Client::ClientFactory;
using QS::Client::GetMessageForQSError;
using QS::Client::IsGoodQSError;
using QS::Client::QSError;
using QS::Client::TransferHandle;
using QS::Client::TransferManager;
using QS::Client::TransferManagerConfigure;
using QS::Client::TransferManagerFactory;
using QS::Data::Cache;
using QS::Data::ChildrenMultiMapConstIterator;
using QS::Data::DirectoryTree;
using QS::Data::Entry;
using QS::Data::FileMetaData;
using QS::Data::FileType;
using QS::Data::FilePathToNodeUnorderedMap;
using QS::Data::IOStream;
using QS::Data::Node;
using QS::Exception::QSException;
using QS::FileSystem::Configure::GetMaxFileCacheSize;
using QS::FileSystem::Configure::GetDefaultTransferMaxBufSize;
using QS::FileSystem::Configure::GetDefaultMaxParallelTransfers;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetDirName;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using QS::Utils::IsRootDirectory;
using std::make_shared;
using std::pair;
using std::shared_ptr;
using std::string;
using std::to_string;
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

  m_transferManager->SetClient(m_client);
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
  // TODO(jim): wait for sdk api of meta data
  // change meta mode: x-qs-meta-mode
  // call Stat to update meta locally
}

// --------------------------------------------------------------------------
void Drive::Chown(const std::string &filePath, uid_t uid, gid_t gid) {
  // TODO(jim): wait for sdk api of meta
  // change meta uid gid; x-qs-meta-uid, x-qs-meta-gid
  // call Stat to update meta locally
}

// --------------------------------------------------------------------------
void Drive::DeleteFile(const string &filePath, bool doCheck) {
  if (filePath.empty()) {
    DebugWarning("Null file path");
    return;
  }

  auto res = GetNode(filePath, false);
  auto node = res.first.lock();
  if (doCheck) {
    if (!(node && *node)) {
      DebugWarning("No such file " + filePath);
      return;
    }
    if (node->IsDirectory()) {
      DebugWarning("Target file is a directory " + filePath);
      return;
    }
  }

  // delete file asynchronizely
  auto receivedHandler = [](const ClientError<QSError> &err) {
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  };
  GetClient()->GetExecutor()->SubmitAsyncPrioritized(
      receivedHandler,
      [this, filePath] { return GetClient()->DeleteFile(filePath); });
}

// --------------------------------------------------------------------------
void Drive::DeleteDir(const string &dirPath, bool recursive, bool doCheck) {
  if (dirPath.empty()) {
    DebugWarning("Null dir path");
    return;
  }

  string path = AppendPathDelim(dirPath);
  if (doCheck) {
    auto res = GetNode(path, true);  // invoking update directory
    auto node = res.first.lock();
    if (!(node && *node)) {
      DebugWarning("No such file or directory" + path);
      return;
    }
    if (!node->IsDirectory()) {
      DebugWarning("Not a directory " + path);
      return;
    }
    if (!node->IsEmpty()) {
      DebugWarning("Unable to remove, directory is not empty " + path);
      return;
    }
  }

  // delete empty dir asynchronizely
  auto receivedHandler = [](const ClientError<QSError> &err) {
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  };
  GetClient()->GetExecutor()->SubmitAsyncPrioritized(
      receivedHandler, [this, path, recursive] {
        return GetClient()->DeleteDirectory(path, recursive);
      });
}

// --------------------------------------------------------------------------
void Drive::HardLink(const string &filePath, const string &hardlinkPath) {
  assert(!filePath.empty() && !hardlinkPath.empty());
  if (filePath.empty() || hardlinkPath.empty()) {
    DebugWarning("Invalid empty parameter");
    return;
  }

  m_directoryTree->HardLink(filePath, hardlinkPath);
}

// --------------------------------------------------------------------------
void Drive::MakeFile(const string &filePath, mode_t mode, dev_t dev) {
  assert(!filePath.empty());
  if (filePath.empty()) {
    DebugWarning("Invalid empty file path");
    return;
  }

  FileType type = FileType::File;
  if (mode & S_IFREG) {
    type = FileType::File;
  } else if (mode & S_IFBLK) {
    type = FileType::Block;
  } else if (mode & S_IFCHR) {
    type = FileType::Character;
  } else if (mode & S_IFIFO) {
    type = FileType::FIFO;
  } else if (mode & S_IFSOCK) {
    type = FileType::Socket;
  } else {
    DebugWarning(
        "Try to make a directory or symbolic link, but MakeFile is only for "
        "creation of non-directory and non-symlink nodes. ");
    return;
  }

  if (type == FileType::File) {
    GetClient()->MakeFile(filePath);
    // QSClient::MakeFile doesn't update directory tree, (refer it for details)
    // So we call Stat asynchronizely which will update dir tree.
    auto receivedHandler = [](const ClientError<QSError> &err) {
      DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
    };
    GetClient()->GetExecutor()->SubmitAsyncPrioritized(
        receivedHandler,
        [this, filePath] { return GetClient()->Stat(filePath); });
  } else {
    time_t mtime = time(NULL);
    m_directoryTree->Grow(make_shared<FileMetaData>(
        filePath, 0, mtime, mtime, GetProcessEffectiveUserID(),
        GetProcessEffectiveGroupID(), mode, type, "", "", false, dev));
  }
}

// --------------------------------------------------------------------------
void Drive::MakeDir(const string &dirPath, mode_t mode) {
  assert(!dirPath.empty());
  if (dirPath.empty()) {
    DebugWarning("Invalid empty dir path");
    return;
  }
  if (!(mode & S_IFDIR)) {
    DebugWarning("Try to make a non-directory file. ");
    return;
  }

  string path = AppendPathDelim(dirPath);
  GetClient()->MakeDirectory(path);
  // QSClient::MakeDirectory doesn't update directory tree,
  // So we call Stat asynchronizely which will update dir tree.
  auto receivedHandler = [](const ClientError<QSError> &err) {
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  };
  GetClient()->GetExecutor()->SubmitAsyncPrioritized(
      receivedHandler, [this, path] { return GetClient()->Stat(path); });
}

// --------------------------------------------------------------------------
void Drive::OpenFile(const string &filePath, bool doCheck) {
  if (doCheck) {
    if (filePath.empty()) {
      DebugWarning("Invalid input");
      return;
    }
  }

  auto res = GetNode(filePath, true);
  auto node = res.first.lock();
  bool modified = res.second;
  if (doCheck) {
    if (!(node && *node)) {
      DebugError("No such file or directory " + filePath);
      return;
    }
    if (node->IsDirectory()) {
      DebugError("Not a file but a directory " + filePath);
      return;
    }
  }

  // Download (partial) file if not found in cache or if cache need update
  auto it = m_cache->Find(filePath);
  time_t mtime = node->GetMTime();
  if (it == m_cache->End() || modified) {
    // download aysnchornizely (partial) file  (not overflow)
    auto remainingSize = node->GetFileSize();
    size_t downloadedSize = 0;
    size_t size = GetDefaultTransferMaxBufSize();
    off_t offset = 0;
    size_t count = GetDefaultMaxParallelTransfers();
    while (remainingSize > 0 && count > 0) {
      off_t offset_ = offset + downloadedSize;
      size_t downloadSize_ = downloadedSize + size < GetMaxFileCacheSize()
                                 ? size
                                 : GetMaxFileCacheSize() - downloadedSize;
      if (downloadSize_ == 0) {
        break;
      }

      auto stream_ = make_shared<IOStream>(downloadSize_);

      auto callback = [this, filePath, offset_, downloadSize_, stream_,
                       mtime](const shared_ptr<TransferHandle> &handle) {
        if (handle) {
          handle->WaitUntilFinished();
          bool success = m_cache->Write(filePath, offset_, downloadSize_,
                                        std::move(stream_), mtime);
          DebugErrorIf(!success,
                       "Fail to write cache [file:offset:len=" + filePath +
                           ":" + to_string(offset_) + ":" +
                           to_string(downloadSize_) + "]");
        }
      };

      GetTransferManager()->GetExecutor()->SubmitAsync(
          callback, [this, filePath, offset_, downloadSize_, stream_]() {
            return m_transferManager->DownloadFile(filePath, offset_,
                                                   downloadSize_, stream_);
          });

      downloadedSize += downloadSize_;
      remainingSize -= downloadSize_;
      --count;
    }
  }

  node->SetFileOpen(true);
}

// --------------------------------------------------------------------------
size_t Drive::ReadFile(const string &filePath, off_t offset, size_t size,
                       char *buf, bool doCheck) {
  if (doCheck && (filePath.empty() || buf == nullptr)) {
    DebugWarning("Invalid input");
    return 0;
  }

  if (size > GetMaxFileCacheSize()) {
    DebugError("Input size surpass max file cache size");
    return 0;
  }

  auto res = GetNode(filePath, false);
  auto node = res.first.lock();
  bool modified = res.second;

  // Check file
  if (doCheck) {
    if (!(node && *node)) {
      DebugError("No such file " + filePath);
      return 0;
    }
    if (node->IsDirectory()) {
      DebugError("Not a file but a directory " + filePath);
      return 0;
    }
  }

  // Ajust size or calculate remaining size
  size_t downloadSize = size;
  int64_t remainingSize = 0;
  auto fileSize = node->GetFileSize();
  if (offset + size > fileSize) {
    DebugWarning("Input overflow [file:offset:size:totalsize = " + filePath +
                 ":" + to_string(offset) + ":" + to_string(size) + ":" +
                 to_string(fileSize) + "]. Ajust it");
    downloadSize = fileSize - offset;
  } else {
    remainingSize = fileSize - (offset + size);
  }

  // Download file if not found in cache or if cache need update
  auto it = m_cache->Find(filePath);
  time_t mtime = node->GetMTime();
  if (it == m_cache->End() || modified) {
    // download synchronizely for request file part
    auto stream = make_shared<IOStream>(downloadSize);
    auto handle =
        m_transferManager->DownloadFile(filePath, offset, downloadSize, stream);

    // download aysnchornizely for (partial) remaining file part (not overflow)
    size_t downloadedSize = size;
    size_t count = GetDefaultMaxParallelTransfers();
    while (remainingSize > 0 && count > 0) {
      off_t offset_ = offset + downloadedSize;
      size_t downloadSize_ = downloadedSize + size < GetMaxFileCacheSize()
                                 ? size
                                 : GetMaxFileCacheSize() - downloadedSize;
      if (downloadSize_ == 0) {
        break;
      }
      auto stream_ = make_shared<IOStream>(downloadSize_);

      auto callback = [this, filePath, offset_, downloadSize_, stream_,
                       mtime](const shared_ptr<TransferHandle> &handle) {
        if (handle) {
          handle->WaitUntilFinished();
          bool success = m_cache->Write(filePath, offset_, downloadSize_,
                                        std::move(stream_), mtime);
          DebugErrorIf(!success,
                       "Fail to write cache [file:offset:len=" + filePath +
                           ":" + to_string(offset_) + ":" +
                           to_string(downloadSize_) + "]");
        }
      };

      GetTransferManager()->GetExecutor()->SubmitAsync(
          callback, [this, filePath, offset_, downloadSize_, stream_]() {
            return m_transferManager->DownloadFile(filePath, offset_,
                                                   downloadSize_, stream_);
          });

      downloadedSize += downloadSize_;
      remainingSize -= downloadSize_;
      --count;
    }

    // waiting for download to finish for request file part
    if (handle) {
      handle->WaitUntilFinished();
      bool success = m_cache->Write(filePath, offset, downloadSize,
                                    std::move(stream), mtime);
      DebugErrorIf(!success,
                   "Fail to write cache [file:offset:len=" + filePath + ":" +
                       to_string(offset) + ":" + to_string(downloadSize) + "]");
    }
  }

  // Read from cache
  return m_cache->Read(filePath, offset, downloadSize, buf, node);
}

// --------------------------------------------------------------------------
void Drive::RenameFile(const string &filePath, const string &newFilePath,
                       bool doCheck) {
  if (doCheck) {
    if (filePath.empty() || newFilePath.empty()) {
      DebugWarning("Invalid empty parameter");
      return;
    }
    if (IsRootDirectory(filePath)) {
      DebugError("Unable to rename root");
      return;
    }

    auto res = GetNode(filePath, false);  // Do not invoke updating dirctory
                                          // as we are changing it
    auto node = res.first.lock();
    if (!(node && *node)) {
      DebugInfo("No such file " + filePath);
      return;
    }
    if (node->IsDirectory()) {
      DebugError("Not a file but a directory " + filePath);
      return;
    }
  }

  // Do Renaming
  auto err = GetClient()->MoveFile(filePath, newFilePath);

  // Call GetNode to update meta(such as mtime, .etc)
  if (IsGoodQSError(err)) {
    auto res = GetNode(newFilePath, true);
    auto node = res.first.lock();
    DebugErrorIf(!node, "Fail to rename file for " + filePath);
    return;
  } else {
    DebugError(GetMessageForQSError(err));
    return;
  }
}

// --------------------------------------------------------------------------
void Drive::RenameDir(const string &dirPath, const string &newDirPath,
                      bool doCheck) {
  if (doCheck) {
    if (dirPath.empty() || newDirPath.empty()) {
      DebugWarning("Invalid empty parameter");
      return;
    }
    if (IsRootDirectory(dirPath)) {
      DebugError("Unable to rename root");
      return;
    }

    auto res = GetNode(dirPath, false);  // Do not invoke updating dirctory
                                         // as we are changing it
    auto node = res.first.lock();
    if (!(node && *node)) {
      DebugInfo("No such file or directory " + dirPath);
      return;
    }
    if (!node->IsDirectory()) {
      DebugError("Not a directory but a file " + dirPath);
      return;
    }
  }

  auto newPath = newDirPath;
  if (newDirPath.back() != '/') {
    DebugWarning(
        "New file path is not ending with '/' for a directory, appending "
        "it");
    newPath = AppendPathDelim(newDirPath);
  }

  // Do Renaming
  auto err = GetClient()->MoveDirectory(dirPath, newPath);

  // Call GetNode to update meta(such as mtime, .etc)
  if (IsGoodQSError(err)) {
    auto res = GetNode(newPath, true);
    auto node = res.first.lock();
    DebugErrorIf(!node, "Fail to rename dir for " + dirPath);
    return;
  } else {
    DebugError(GetMessageForQSError(err));
    return;
  }
}

// --------------------------------------------------------------------------
// Symbolic link is a file that contains a reference to another file or dir
// in the form of an absolute path (in qsfs) or relative path and that affects
// pathname resolution.
void Drive::SymLink(const string &filePath, const string &linkPath) {
  assert(!filePath.empty() && !linkPath.empty());
  if (filePath.empty() || linkPath.empty()) {
    DebugWarning("Invalid empty parameter");
    return;
  }

  time_t mtime = time(NULL);
  auto lnkNode = m_directoryTree->Grow(make_shared<FileMetaData>(
      linkPath, filePath.size(), mtime, mtime, GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), Configure::GetDefineFileMode(),
      FileType::SymLink));
  if (lnkNode && (lnkNode)) {
    lnkNode->SetSymbolicLink(filePath);
  } else {
    DebugError("Fail to create a symbolic link [path=" + filePath +
               ", link=" + linkPath);
  }
}

// --------------------------------------------------------------------------
void Drive::TruncateFile(const string &filePath, size_t newSize) {
  // download file, truncate it, delete old file, and write it
  // TODO(jim): maybe we do not need this method, just call delete and write
  // node->SetNeedUpload(true);  // Mark upload

  // if newSize = 0, empty file

  // if newSize > size, fill the hole, Write file hole
  /*   start = newsize > entry->file_size ? entry->file_size : newsize - 1;
    size = newsize > entry->file_size ? (newsize - entry->file_size) :
    (entry->file_size - newsize);
    if (start > entry->file_size) {
      buf = new char[size];
      assert(buf != NULL);
      memset(buf, 0, size);
    } */

  // Fill hole when Resize File(Cache, Page)

  // if newSize < size, resize it

  // update cached file
  // set entry->write = true; should do this in Drive
  // TODO(jim): any modification on diretory tree should be synchronized by its
  // api
}

// --------------------------------------------------------------------------
void Drive::UploadFile(const string &filePath, bool doCheck) {
  //
  // if file size > 20M call tranfser to upload multipart
  // need a mechnasim to handle the memory insufficient situation
  // // set entry->write = fasle; // should do this in drive::
  // invoke putobject
  // or invoke multipart upload (first two step) transfer manager
  // and at end invoke complete mulitpart upload

  if (doCheck && filePath.empty()) {
    DebugWarning("Invalid input");
    return;
  }

  auto res = GetNode(filePath, false);
  auto node = res.first.lock();

  // Check file
  if (doCheck) {
    if (!(node && *node)) {
      DebugError("No such file " + filePath);
      return;
    }
    if (node->IsDirectory()) {
      DebugError("Not a file but a directory " + filePath);
      return;
    }
  }

    // TODO(jim):
  // check if [0, offset] is already existing in cache
  // if not, l

  auto callback = [this, filePath](const shared_ptr<TransferHandle> &handle){
    if (handle) {
      handle->WaitUntilFinished();
      //m_cache->Erase(filePath);  // TODO erase if cache is full
      // maybe wait for a scond;
      m_multipartUploads.erase(filePath);
    }
  };

  // always upload from cache, no retry

  GetTransferManager()->GetExecutor()->SubmitAsync(callback, [this, filePath](){
    auto it = m_multipartUploads.find(filePath);
    if(it != m_multipartUploads.end()){
      auto handle = it->second;
      // TODO(jim)
      // do sth to update handle
      // handle->AddPendingPart();
      return m_transferManager->RetryUpload();
    } else {
      return m_transferManager->UploadFile(filePath);
    }
  });

  node->SetNeedUpload(false);
  node->SetFileOpen(false);
}

// --------------------------------------------------------------------------
void Drive::Utimens(const string &path, time_t mtime) {
  // TODO(jim): wait for sdk meta data api
  // x-qs-meta-mtime
  // x-qs-copy-source
  // x-qs-metadata-directive = REPLACE
  // call Stat to update meta locally
  // NOTE just do this with put object copy (this will delete orginal file
  // then create a copy of it)
}

// --------------------------------------------------------------------------
int Drive::WriteFile(const string &filePath, off_t offset, size_t size,
                     const char *buf, bool doCheck) {
  if (doCheck && (filePath.empty() || buf == nullptr)) {
    DebugWarning("Invalid input");
    return 0;
  }

  if (size > GetMaxFileCacheSize()) {
    DebugError("Input size surpass max file cache size");
    return 0;
  }

  auto res = GetNode(filePath, false);
  auto node = res.first.lock();

  // Check file
  if (doCheck) {
    if (!(node && *node)) {
      DebugError("No such file " + filePath);
      return 0;
    }
    if (node->IsDirectory()) {
      DebugError("Not a file but a directory " + filePath);
      return 0;
    }
  }

  if (!node->IsFileOpen()) {
    DebugError("File is not open " + filePath);
    return 0;
  }

  bool success = m_cache->Write(filePath, offset, size, buf, time(NULL));
  if(success){
    node->SetNeedUpload(true);
    if(offset + size > node->GetFileSize()){
      node->SetFileSize(offset + size);
    }  
  }

  return success ? size : 0;

  // if entry->file size < size + offset, update the entry size with max one
  // Call Cache->Put to store fiel into cache and when overpass max size,
  // invoke multipart upload (first two step)
  // Finshed will be done in Release/ (here in Drive::UploadFile)

  // node->SetNeedUpload(true);  // Mark upload

  // create a write buffer if necceaary
  // handle hole if file

  // stat(filePath) to get file size to determine if trigger multiple upload
  //
  // if cache enough, load (0 to offset + size) and write to cache, mark need
  // upload
  // else NoCacheLoadAndPost
  // every time when the cache file is a candidate to invoke multiupload, do it
  // async

  // For a random write case, when there is no enough cache, need to

  // need to write to a temp cache file in local
  // when inconsecutive large file
}

}  // namespace FileSystem
}  // namespace QS
