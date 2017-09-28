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
#include <utility>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "base/StringUtils.h"
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
using QS::FileSystem::Configure::GetCacheTemporaryDirectory;
using QS::FileSystem::Configure::GetDefaultMaxParallelTransfers;
using QS::FileSystem::Configure::GetDefaultTransferMaxBufSize;
using QS::FileSystem::Configure::GetMaxFileCacheSize;
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
Drive::~Drive() {
  // abort unfinished multipart uploads
  if (!m_unfinishedMultipartUploadHandles.empty()) {
    for (auto &fileToHandle : m_unfinishedMultipartUploadHandles) {
      m_transferManager->AbortMultipartUpload(fileToHandle.second);
    }
  }
  // remove temp folder if existing
  auto tmpfolder = GetCacheTemporaryDirectory();
  if(FileExists(tmpfolder, true) && IsDirectory(tmpfolder, true)){  // log on
    DeleteFilesInDirectory(tmpfolder, true);  // delete folder itself
  }

  m_client.reset();
  m_transferManager.reset();
  m_cache.reset();
  m_directoryTree.reset();
  m_unfinishedMultipartUploadHandles.clear();
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
  m_mountable.store(Connect(false));  //synchronizely
  return m_mountable.load();
}

// --------------------------------------------------------------------------
bool Drive::Connect(bool buildupDirTreeAsync) const {
  auto err = GetClient()->HeadBucket();
  if (!IsGoodQSError(err)) {
    DebugError(GetMessageForQSError(err));
    return false;
  }

  // Update root node of the tree
  if (!m_directoryTree->GetRoot()) {
    m_directoryTree->Grow(QS::Data::BuildDefaultDirectoryMeta("/", time(NULL)));
  }

  // Build up the root level of directory tree asynchornizely.
  auto ReceivedHandler = [](const ClientError<QSError> &err) {
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  };

  if (buildupDirTreeAsync) {  // asynchronizely
    GetClient()->GetExecutor()->SubmitAsyncPrioritized(
        ReceivedHandler, [this] { return GetClient()->ListDirectory("/"); });
  } else { // synchronizely
    ReceivedHandler(GetClient()->ListDirectory("/"));
  }

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
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  };

  if (node && *node) {
    UpdateNode(path, node);
  } else {
    auto err = GetClient()->Stat(path);  // head it
    if (IsGoodQSError(err)) {
      node = m_directoryTree->Find(path).lock();
    } else {
      DebugError(GetMessageForQSError(err));
    }
  }

  // Update directory tree asynchornizely
  // Should check node existence as given file could be not existing which is
  // not be considered as an error.
  // The modified time is only the meta of an object, we should not take
  // modified time as an precondition to decide if we need to update dir or not.
  if (node && *node && node->IsDirectory() && updateIfDirectory) {
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
  auto ReceivedHandler = [](const ClientError<QSError> &err) {
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
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
    if(!IsGoodQSError(err)){
      DebugError(GetMessageForQSError(err));
      return;
    }

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
  if(!IsGoodQSError(err)){
    DebugError(GetMessageForQSError(err));
    return;
  }

  // QSClient::MakeDirectory doesn't grow directory tree with the created dir
  // node, So we call Stat synchronizely.
  err = GetClient()->Stat(dirPath);
  DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
}

// --------------------------------------------------------------------------
void Drive::OpenFile(const string &filePath) {
  auto res = GetNode(filePath, false);
  auto node = res.first.lock();
  bool modified = res.second;

  auto ranges = m_cache->GetUnloadedRanges(filePath, node->GetFileSize());
  time_t mtime = node->GetMTime();
  bool fileContentExist =
      m_cache->HasFileData(filePath, 0, node->GetFileSize());
  if (!fileContentExist || modified) {
    // TODO(jim): should we do this async?
    DownloadFileContentRanges(filePath, ranges, mtime, false);
  }

  node->SetFileOpen(true);
}

// --------------------------------------------------------------------------
size_t Drive::ReadFile(const string &filePath, off_t offset, size_t size,
                       char *buf) {
  // if (size > GetMaxFileCacheSize()) {
  //   DebugError("Input size surpass max file cache size");
  //   return 0;
  // }

  auto res = GetNode(filePath, false);
  auto node = res.first.lock();
  bool modified = res.second;

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

  // Download file if not found in cache or if cache need update
  bool fileContentExist = m_cache->HasFileData(filePath, offset, downloadSize);
  time_t mtime = node->GetMTime();
  if (!fileContentExist || modified) {
    // download synchronizely for request file part
    auto stream = make_shared<IOStream>(downloadSize);
    auto handle =
        m_transferManager->DownloadFile(filePath, offset, downloadSize, stream);

    // waiting for download to finish for request file part
    if (handle) {
      handle->WaitUntilFinished();
      if (handle->DoneTransfer() && !handle->HasFailedParts()) {
        bool success = m_cache->Write(filePath, offset, downloadSize,
                                      std::move(stream), mtime);
        DebugErrorIf(!success,
                     "Fail to write cache [offset:len=" + to_string(offset) +
                         ":" + to_string(downloadSize) + "] " +
                         FormatPath(filePath));
      }
    }
  }

  // download asynchronizely for unloaded part
  if (remainingSize > 0) {
    auto ranges = m_cache->GetUnloadedRanges(filePath, fileSize);
    if(!ranges.empty()){
      DownloadFileContentRanges(filePath, ranges, mtime, true);
    }
  }

  // Read from cache
  return m_cache->Read(filePath, offset, downloadSize, buf, node);
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
    DebugWarningIf(!node, "Fail to rename file " + FormatPath(filePath));
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
      DebugErrorIf(!node, "Fail to rename dir " + FormatPath(dirPath));
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

  // QSClient::Symlink doesn't update directory tree (refer it for details)
  // with the created symlink node, So we call Stat synchronizely.
  err = GetClient()->Stat(linkPath);
  if(!IsGoodQSError(err)){
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
  // any modification on diretory tree should be synchronized by its
  // api
}

// --------------------------------------------------------------------------
void Drive::UploadFile(const string &filePath, bool async) {
  auto res = GetNode(filePath, false);
  auto node = res.first.lock();
  auto Callback = [this, node](const shared_ptr<TransferHandle> &handle) {
    if (handle) {
      node->SetNeedUpload(false);
      node->SetFileOpen(false);
      if (handle->IsMultipart()) {
        m_unfinishedMultipartUploadHandles.emplace(handle->GetObjectKey(),
                                                   handle);
      }
      handle->WaitUntilFinished();
      m_unfinishedMultipartUploadHandles.erase(handle->GetObjectKey());

      if (handle->DoneTransfer() && !handle->HasFailedParts()) {
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
  auto ranges = m_cache->GetUnloadedRanges(filePath, fileSize);
  time_t mtime = node->GetMTime();
  if(async){
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
  // if (size > GetMaxFileCacheSize()) {
  //   DebugError("Input size surpass max file cache size");
  //   return 0;
  // }

  auto res = GetNode(filePath, false);
  auto node = res.first.lock();

  bool success = m_cache->Write(filePath, offset, size, buf, time(NULL));
  if (success) {
    node->SetNeedUpload(true);
    if (offset + size > node->GetFileSize()) {
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
