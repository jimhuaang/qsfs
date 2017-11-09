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

#include <deque>
#include <future>  // NOLINT
#include <memory>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/TimeUtils.h"
#include "base/Utils.h"
#include "client/Client.h"
#include "client/ClientError.h"
#include "client/ClientFactory.h"
#include "client/QSError.h"
#include "client/TransferHandle.h"
#include "client/TransferManager.h"
#include "client/TransferManagerFactory.h"
#include "configure/Default.h"
#include "configure/Options.h"
#include "data/Cache.h"
#include "data/Directory.h"
#include "data/FileMetaData.h"
#include "data/IOStream.h"
#include "data/Size.h"

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
using QS::Data::ContentRangeDeque;
using QS::Data::ChildrenMultiMapConstIterator;
using QS::Data::DirectoryTree;
using QS::Data::Entry;
using QS::Data::FileMetaData;
using QS::Data::FileType;
using QS::Data::FilePathToNodeUnorderedMap;
using QS::Data::IOStream;
using QS::Data::Node;
using QS::Exception::QSException;
using QS::Configure::Default::GetCacheTemporaryDirectory;
using QS::Configure::Default::GetDefaultMaxParallelTransfers;
using QS::Configure::Default::GetDefaultTransferMaxBufSize;
using QS::StringUtils::FormatPath;
using QS::Utils::AppendPathDelim;
using QS::Utils::DeleteFilesInDirectory;
using QS::Utils::FileExists;
using QS::Utils::IsDirectory;
using QS::Utils::GetDirName;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using QS::Utils::IsRootDirectory;
using std::deque;
using std::make_shared;
using std::pair;
using std::shared_ptr;
using std::string;
using std::stringstream;
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
      m_cleanup(false),
      m_client(ClientFactory::Instance().MakeClient()),
      m_transferManager(std::move(
          TransferManagerFactory::Create(TransferManagerConfigure()))) {
  uint64_t cacheSize = static_cast<uint64_t>(
      QS::Configure::Options::Instance().GetMaxCacheSizeInMB() *
      QS::Data::Size::MB1);
  m_cache = std::move(unique_ptr<Cache>(new Cache(cacheSize)));

  uid_t uid = GetProcessEffectiveUserID();
  gid_t gid = GetProcessEffectiveGroupID();

  m_directoryTree = unique_ptr<DirectoryTree>(new DirectoryTree(
      time(NULL), uid, gid, QS::Configure::Default::GetRootMode()));

  m_transferManager->SetClient(m_client);
}

// --------------------------------------------------------------------------
Drive::~Drive() { CleanUp(); }

// --------------------------------------------------------------------------
void Drive::CleanUp() {
  if (!m_cleanup) {
    // abort unfinished multipart uploads
    if (!m_unfinishedMultipartUploadHandles.empty()) {
      for (auto &fileToHandle : m_unfinishedMultipartUploadHandles) {
        m_transferManager->AbortMultipartUpload(fileToHandle.second);
      }
    }
    // remove temp folder if existing
    auto tmpfolder = GetCacheTemporaryDirectory();
    if (FileExists(tmpfolder, false) &&
        IsDirectory(tmpfolder, true)) {         // log on
      DeleteFilesInDirectory(tmpfolder, true);  // delete folder itself
    }

    m_client.reset();
    m_transferManager.reset();
    m_cache.reset();
    m_directoryTree.reset();
    m_unfinishedMultipartUploadHandles.clear();

    m_cleanup.store(true);
  }
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
  m_mountable.store(Connect());
  return m_mountable.load();
}

// --------------------------------------------------------------------------
bool Drive::Connect() const {
  // Connect is call before fuse_main and thread pools are initialized in
  // fuse init, so at the time the thread pool is still not been initialized.
  bool notUseThreadPool = false;
  auto err = GetClient()->HeadBucket(notUseThreadPool);
  if (!IsGoodQSError(err)) {
    DebugError(GetMessageForQSError(err));
    return false;
  }

  // Update root node of the tree
  if (!m_directoryTree->GetRoot()) {
    m_directoryTree->Grow(QS::Data::BuildDefaultDirectoryMeta("/", time(NULL)));
  }

  // Build up the root level of directory tree asynchornizely.
  auto DoListDirectory = [this, notUseThreadPool] {
    auto err = GetClient()->ListDirectory("/", notUseThreadPool);
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  };
  std::async(std::launch::async, DoListDirectory);

  return true;
}

// --------------------------------------------------------------------------
shared_ptr<Node> Drive::GetRoot() {
  if (!Connect()) {
    throw QSException("Unable to connect to object storage bucket");
  }
  return m_directoryTree->GetRoot();
}

// --------------------------------------------------------------------------
pair<weak_ptr<Node>, bool> Drive::GetNode(const string &path,
                                          bool updateIfDirectory,
                                          bool updateDirAsync) {
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
    if (!IsGoodQSError(err)) {
      // As user can remove file through other ways such as web console, etc.
      // So we need to remove file from local dir tree and cache.
      if (err.GetError() == QSError::KEY_NOT_EXIST) {
        // remove node
        DebugInfo("File not exist " + FormatPath(path));
        m_directoryTree->Remove(path);
        if (m_cache->HasFile(path)) {
          m_cache->Erase(path);
        }
      } else {
        DebugError(GetMessageForQSError(err));
      }
    }
  };

  auto expireDurationInMin =
      QS::Configure::Options::Instance().GetStatExpireInMin();
  if (node && *node) {
    if (QS::TimeUtils::IsExpire(node->GetCachedTime(),
                                expireDurationInMin)) {
      UpdateNode(path, node);
    }
  } else {
    auto err = GetClient()->Stat(path);  // head it
    if (IsGoodQSError(err)) {
      node = m_directoryTree->Find(path).lock();
    } else {
      if (err.GetError() == QSError::KEY_NOT_EXIST) {
        DebugInfo("File not exist " + FormatPath(path));
      } else {
        DebugError(GetMessageForQSError(err));
      }
    }
  }

  // Update directory tree asynchornizely
  // Should check node existence as given file could be not existing which is
  // not be considered as an error.
  // The modified time is only the meta of an object, we should not take
  // modified time as an precondition to decide if we need to update dir or not.
  if (node && *node && node->IsDirectory() && updateIfDirectory &&
      (QS::TimeUtils::IsExpire(node->GetCachedTime(),
                               expireDurationInMin) ||
       node->IsEmpty())) {
    auto ReceivedHandler = [](const ClientError<QSError> &err) {
      DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
    };

    if (updateDirAsync) {
      GetClient()->GetExecutor()->SubmitAsync(ReceivedHandler, [this, path] {
        return GetClient()->ListDirectory(AppendPathDelim(path));
      });
    } else {
      ReceivedHandler(GetClient()->ListDirectory(AppendPathDelim(path)));
    }
  }

  return {node, modified};
}

// --------------------------------------------------------------------------
weak_ptr<Node> Drive::GetNodeSimple(const string &path) {
  return m_directoryTree->Find(path);
}

// --------------------------------------------------------------------------
struct statvfs Drive::GetFilesystemStatistics() {
  struct statvfs statv;
  auto err = GetClient()->Statvfs(&statv);
  DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  return statv;
}

// --------------------------------------------------------------------------
vector<weak_ptr<Node>> Drive::FindChildren(const string &dirPath,
                                           bool updateIfDir) {
  auto node = GetNodeSimple(dirPath).lock();
  if (node && *node) {
    if (node->IsDirectory() && updateIfDir) {
      // Update directory tree synchornizely
      auto err = GetClient()->ListDirectory(dirPath);
      DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
    }
    return m_directoryTree->FindChildren(dirPath);
  } else {
    DebugError("Directory not exist " + FormatPath(dirPath));
    return vector<weak_ptr<Node>>();
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
// Remove a file or an empty directory
void Drive::RemoveFile(const string &filePath, bool async) {
  auto ReceivedHandler = [filePath](const ClientError<QSError> &err) {
    if (IsGoodQSError(err)) {
      DebugInfo("Delete file " + FormatPath(filePath));
    } else {
      DebugError(GetMessageForQSError(err));
    }
  };

  if (async) {  // delete file asynchronizely
    GetClient()->GetExecutor()->SubmitAsyncPrioritized(
        ReceivedHandler,
        [this, filePath] { return GetClient()->DeleteFile(filePath); });
  } else {
    ReceivedHandler(GetClient()->DeleteFile(filePath));
  }
}

// --------------------------------------------------------------------------
void Drive::HardLink(const string &filePath, const string &hardlinkPath) {
  // DO NOT use it for now.
  // As DirectoryTree::HardLink is not ready, and Drive still need to add
  // another function ReadHardLink to support hardlink.
  assert(!filePath.empty() && !hardlinkPath.empty());
  if (filePath.empty() || hardlinkPath.empty()) {
    DebugWarning("Invalid empty parameter");
    return;
  }

  m_directoryTree->HardLink(filePath, hardlinkPath);
}

// --------------------------------------------------------------------------
void Drive::MakeFile(const string &filePath, mode_t mode, dev_t dev) {
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
        "Try to create a directory or symbolic link, but MakeFile is only for "
        "creation of non-directory and non-symlink nodes. ");
    return;
  }

  if (type == FileType::File) {
    auto err = GetClient()->MakeFile(filePath);
    if (!IsGoodQSError(err)) {
      DebugError(GetMessageForQSError(err));
      return;
    }

    DebugInfo("Create file " + FormatPath(filePath));

    // QSClient::MakeFile doesn't update directory tree (refer it for details)
    // with the created file node, So we call Stat synchronizely.
    err = GetClient()->Stat(filePath);
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  } else {
    DebugError(
        "Not support to create a special file (block, char, FIFO, etc.)");
    // This only make file of other types in local dir tree, nothing happens
    // in server. And it will be removed when synchronize with server.
    // TODO(jim): may consider to support them in server if sdk support this
    // time_t mtime = time(NULL);
    // m_directoryTree->Grow(make_shared<FileMetaData>(
    //     filePath, 0, mtime, mtime, GetProcessEffectiveUserID(),
    //     GetProcessEffectiveGroupID(), mode, type, "", "", false, dev));
  }
}

// --------------------------------------------------------------------------
void Drive::MakeDir(const string &dirPath, mode_t mode) {
  auto err = GetClient()->MakeDirectory(dirPath);
  if (!IsGoodQSError(err)) {
    DebugError(GetMessageForQSError(err));
    return;
  }

  DebugInfo("Create dir " + FormatPath(dirPath));

  // QSClient::MakeDirectory doesn't grow directory tree with the created dir
  // node, So we call Stat synchronizely.
  err = GetClient()->Stat(dirPath);
  DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
}

// --------------------------------------------------------------------------
void Drive::OpenFile(const string &filePath, bool async) {
  auto res = GetNode(filePath, false);
  auto node = res.first.lock();
  bool modified = res.second;

  if (!(node && *node)) {
    DebugWarning("File not exist " + FormatPath(filePath));
    return;
  }

  auto fileSize = node->GetFileSize();
  assert(fileSize >= 0);
  if (fileSize == 0) {
    m_cache->Write(filePath, 0, 0, NULL, time(NULL));
  } else if (fileSize > 0) {
    bool fileContentExist = m_cache->HasFileData(filePath, 0, fileSize);
    if (!fileContentExist || modified) {
      auto ranges = m_cache->GetUnloadedRanges(filePath, 0, fileSize);
      if (!ranges.empty()) {
        time_t mtime = node->GetMTime();
        DownloadFileContentRanges(filePath, ranges, mtime, async);
      }
    }
  }

  node->SetFileOpen(true);
  m_cache->SetFileOpen(filePath, true);
}

// --------------------------------------------------------------------------
size_t Drive::ReadFile(const string &filePath, off_t offset, size_t size,
                       char *buf) {
  auto res = GetNode(filePath, false);
  auto node = res.first.lock();
  bool modified = res.second;

  if (!(node && *node)) {
    DebugWarning("File not exist " + FormatPath(filePath));
    return 0;
  }

  // Ajust size or calculate remaining size
  uint64_t downloadSize = size;
  int64_t remainingSize = 0;
  auto fileSize = node->GetFileSize();
  if (offset + size > fileSize) {
    DebugInfo("Read file [offset:size=" + to_string(offset) + ":" +
              to_string(size) + " file size=" + to_string(fileSize) + "] " +
              FormatPath(filePath) + " Overflow, ajust it");
    downloadSize = fileSize - offset;
  } else {
    remainingSize = fileSize - (offset + size);
  }

  if (downloadSize == 0) {
    return 0;
  }

  time_t mtime = node->GetMTime();
  if (mtime > m_cache->GetTime(filePath)) {
    m_cache->Erase(filePath);
  }
  // Download file if not found in cache or if cache need update
  bool fileContentExist = m_cache->HasFileData(filePath, offset, downloadSize);
  if (!fileContentExist || modified) {
    // download synchronizely for request file part
    auto stream = make_shared<IOStream>(downloadSize);
    auto handle =
        m_transferManager->DownloadFile(filePath, offset, downloadSize, stream);

    // waiting for download to finish for request file part
    if (handle) {
      handle->WaitUntilFinished();
      if (handle->DoneTransfer() && !handle->HasFailedParts()) {
        DebugInfo("Download file [offset:len=" + to_string(offset) + ":" +
                  to_string(downloadSize) + "] " + FormatPath(filePath));

        bool success = m_cache->Write(filePath, offset, downloadSize,
                                      std::move(stream), mtime);
        DebugErrorIf(!success,
                     "Fail to write cache [offset:len=" + to_string(offset) +
                         ":" + to_string(downloadSize) + "] " +
                         FormatPath(filePath));
      }
    }
  }

  // download asynchronizely for unloaded part // TODO(jim): consider not
  // download all remaining unloaded large range
  if (remainingSize > 0) {
    auto ranges = m_cache->GetUnloadedRanges(filePath, 0, fileSize);
    if (!ranges.empty()) {
      DownloadFileContentRanges(filePath, ranges, mtime, true);
    }
  }

  // Read from cache
  auto outcome = m_cache->Read(filePath, offset, downloadSize, buf, mtime);
  return std::get<0>(outcome);
}

// --------------------------------------------------------------------------
void Drive::ReadSymlink(const std::string &linkPath) {
  auto node = GetNodeSimple(linkPath).lock();
  auto buffer = std::make_shared<stringstream>();
  auto err = GetClient()->DownloadFile(linkPath, buffer);
  if (IsGoodQSError(err)) {
    node->SetSymbolicLink(buffer->str());
  } else {
    DebugError(GetMessageForQSError(err));
  }
}

// --------------------------------------------------------------------------
void Drive::RenameFile(const string &filePath, const string &newFilePath) {
  // Do Renaming
  auto err = GetClient()->MoveFile(filePath, newFilePath);

  // Update meta(such as mtime, .etc)
  if (IsGoodQSError(err)) {
    auto res = GetNode(newFilePath, false);
    auto node = res.first.lock();
    if (node) {
      DebugInfo("Rename file " + FormatPath(filePath, newFilePath));
    } else {
      DebugWarning("Fail to rename file " + FormatPath(filePath, newFilePath));
    }
    return;
  } else {
    DebugError(GetMessageForQSError(err));
    return;
  }
}

// --------------------------------------------------------------------------
void Drive::RenameDir(const string &dirPath, const string &newDirPath,
                      bool async) {
  // Do Renaming
  auto ReceivedHandler = [this, dirPath,
                          newDirPath](const ClientError<QSError> &err) {
    if (IsGoodQSError(err)) {
      // Rename local cache
      auto node = GetNodeSimple(dirPath).lock();
      auto childPaths = node->GetChildrenIdsRecursively();
      size_t len = dirPath.size();
      deque<string> childTargetPaths;
      for (auto &path : childPaths) {
        if (path.substr(0, len) != dirPath) {
          DebugError("Directory has an invalid child file [dir=" + dirPath +
                     " child=" + path + "]");
          childTargetPaths.emplace_back(path);  // put old path
        }
        childTargetPaths.emplace_back(newDirPath + path.substr(len));
      }
      while (!childPaths.empty() && !childTargetPaths.empty()) {
        auto source = childPaths.back();
        childPaths.pop_back();
        auto target = childTargetPaths.back();
        childTargetPaths.pop_back();
        if (m_cache && m_cache->HasFile(source)) {
          m_cache->Rename(source, target);
        }
      }

      // Remove old dir node from dir tree
      if (m_directoryTree) {
        m_directoryTree->Remove(dirPath);
      }

      // Add new dir node to dir tree
      auto res = GetNode(newDirPath, true, false);  // update dir sync
      node = res.first.lock();
      if (node) {
        DebugInfo("Rename dir " + FormatPath(dirPath, newDirPath));
      } else {
        DebugWarning("Fail to rename dir " + FormatPath(dirPath));
      }
    } else {
      DebugError(GetMessageForQSError(err));
    }
  };

  if (async) {
    GetClient()->GetExecutor()->SubmitAsyncPrioritized(
        ReceivedHandler, [this, dirPath, newDirPath]() {
          return GetClient()->MoveDirectory(dirPath, newDirPath, true);
        });
  } else {
    ReceivedHandler(GetClient()->MoveDirectory(dirPath, newDirPath, false));
  }
}

// --------------------------------------------------------------------------
// Symbolic link is a file that contains a reference to another file or dir
// in the form of an absolute path (in qsfs) or relative path and that affects
// pathname resolution.
void Drive::SymLink(const string &filePath, const string &linkPath) {
  assert(!filePath.empty() && !linkPath.empty());
  auto err = GetClient()->SymLink(filePath, linkPath);
  if (!IsGoodQSError(err)) {
    DebugError("Fail to create a symbolic link [path=" + filePath +
               ", link=" + linkPath);
    DebugError(GetMessageForQSError(err));
    return;
  }

  DebugInfo("Create symlink " + FormatPath(filePath, linkPath));

  // QSClient::Symlink doesn't update directory tree (refer it for details)
  // with the created symlink node, So we call Stat synchronizely.
  err = GetClient()->Stat(linkPath);
  if (!IsGoodQSError(err)) {
    DebugError(GetMessageForQSError(err));
    return;
  }

  auto lnkNode = GetNodeSimple(linkPath).lock();
  if (lnkNode && *lnkNode) {
    lnkNode->SetSymbolicLink(filePath);
  }
}

// --------------------------------------------------------------------------
void Drive::TruncateFile(const string &filePath, size_t newSize) {
  auto node = GetNodeSimple(filePath).lock();
  if (!(node && *node)) {
    DebugWarning("File not exist " + FormatPath(filePath));
    return;
  }

  if (newSize != node->GetFileSize()) {
    DebugInfo(
        "Truncate file [oldsize:newsize=" + to_string(node->GetFileSize()) +
        ":" + to_string(newSize) + "]" + FormatPath(filePath));
    m_cache->Resize(filePath, newSize, time(NULL));
    node->SetFileSize(newSize);
    node->SetNeedUpload(true);
  }
}

// --------------------------------------------------------------------------
void Drive::UploadFile(const string &filePath, bool async) {
  auto res = GetNode(filePath, false);
  auto node = res.first.lock();

  if (!(node && *node)) {
    DebugWarning("File not exist " + FormatPath(filePath));
    return;
  }

  auto Callback = [this, node,
                   filePath](const shared_ptr<TransferHandle> &handle) {
    if (handle) {
      node->SetNeedUpload(false);
      node->SetFileOpen(false);
      m_cache->SetFileOpen(filePath, false);
      if (handle->IsMultipart()) {
        m_unfinishedMultipartUploadHandles.emplace(handle->GetObjectKey(),
                                                   handle);
      }
      handle->WaitUntilFinished();
      m_unfinishedMultipartUploadHandles.erase(handle->GetObjectKey());

      if (handle->DoneTransfer() && !handle->HasFailedParts()) {
        DebugInfo("Upload file " + FormatPath(filePath));
        // update meta mtime
        auto err = GetClient()->Stat(handle->GetObjectKey());
        if (IsGoodQSError(err)) {
          // update cache mtime
          auto node = GetNodeSimple(handle->GetObjectKey()).lock();
          if (node && *node) {
            m_cache->SetTime(handle->GetObjectKey(), node->GetMTime());
          }
        } else {
          DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
        }
      }
    }
  };

  auto fileSize = node->GetFileSize();
  time_t mtime = node->GetMTime();
  auto ranges = m_cache->GetUnloadedRanges(filePath, 0, fileSize);
  if (async) {
    GetTransferManager()->GetExecutor()->SubmitAsync(
        Callback, [this, filePath, fileSize, ranges, mtime]() {
          // download unloaded pages for file
          // this is need as user could open a file and edit a part of it,
          // but you need the completed file in order to upload it.
          if (!ranges.empty()) {
            DownloadFileContentRanges(filePath, ranges, mtime, false);
          }
          // upload the completed file
          return m_transferManager->UploadFile(filePath, fileSize);
        });
  } else {
    if (!ranges.empty()) {
      DownloadFileContentRanges(filePath, ranges, mtime, false);
    }
    Callback(m_transferManager->UploadFile(filePath, fileSize));
  }
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
                     const char *buf) {
  auto node = GetNodeSimple(filePath).lock();
  if (!(node && *node)) {
    DebugWarning("File not exist " + FormatPath(filePath));
    return 0;
  }

  bool success = m_cache->Write(filePath, offset, size, buf, time(NULL));
  if (success) {
    node->SetNeedUpload(true);
    if (offset + size > node->GetFileSize()) {
      node->SetFileSize(offset + size);
    }
  }

  return success ? size : 0;
}

// --------------------------------------------------------------------------
void Drive::DownloadFileContentRanges(const string &filePath,
                                      const ContentRangeDeque &ranges,
                                      time_t mtime, bool async) {
  auto DownloadRange = [this, filePath, async,
                        mtime](const pair<off_t, size_t> &range) {
    off_t offset = range.first;
    size_t size = range.second;
    // Download file if not found in cache or if cache need update
    bool fileContentExist = m_cache->HasFileData(filePath, offset, size);
    if (!fileContentExist) {
      auto bufSize = GetDefaultTransferMaxBufSize();
      auto remainingSize = size;
      uint64_t downloadedSize = 0;

      while (remainingSize > 0) {
        off_t offset_ = offset + downloadedSize;
        int64_t downloadSize_ =
            remainingSize > bufSize ? bufSize : remainingSize;
        if (downloadSize_ <= 0) {
          break;
        }

        auto stream_ = make_shared<IOStream>(downloadSize_);
        auto Callback = [this, filePath, offset_, downloadSize_, stream_,
                         mtime](const shared_ptr<TransferHandle> &handle) {
          if (handle) {
            handle->WaitUntilFinished();
            if (handle->DoneTransfer() && !handle->HasFailedParts()) {
              bool success = m_cache->Write(filePath, offset_, downloadSize_,
                                            std::move(stream_), mtime);
              DebugErrorIf(!success,
                           "Fail to write cache [file:offset:len=" + filePath +
                               ":" + to_string(offset_) + ":" +
                               to_string(downloadSize_) + "]");
            }
          }
        };

        if (async) {
          GetTransferManager()->GetExecutor()->SubmitAsync(
              Callback, [this, filePath, offset_, downloadSize_, stream_]() {
                return m_transferManager->DownloadFile(filePath, offset_,
                                                       downloadSize_, stream_);
              });
        } else {
          auto handle = m_transferManager->DownloadFile(filePath, offset_,
                                                        downloadSize_, stream_);
          Callback(handle);
        }

        downloadedSize += downloadSize_;
        remainingSize -= downloadSize_;
      }
    }
  };

  for (auto &range : ranges) {
    DownloadRange(range);
  }
}

}  // namespace FileSystem
}  // namespace QS
