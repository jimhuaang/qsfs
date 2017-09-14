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

#ifndef _QSFS_FUSE_INCLUDED_FILESYSTEM_DRIVE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_FILESYSTEM_DRIVE_H_  // NOLINT

#include <atomic>  // NOLINT
#include <memory>
#include <utility>
#include <unordered_map>
#include <vector>

#include <sys/stat.h>
#include <sys/statvfs.h>

#include "base/HashUtils.h"
#include "data/Cache.h"
#include "data/Directory.h"

namespace QS {

namespace Client {
class Client;
class File;
class QSClient;
class QSTransferManager;
class TransferHandle;
class TransferManager;
}

namespace Data {
class Cache;
class DirectoryTree;
class FileMetaData;
class Node;
}

namespace FileSystem {

class Drive {
 public:
  Drive(Drive &&) = delete;
  Drive(const Drive &) = delete;
  Drive &operator=(Drive &&) = delete;
  Drive &operator=(const Drive &) = delete;
  ~Drive();

 public:
  static Drive &Instance();
  bool IsMountable() const;
  
  // accessor
  const std::shared_ptr<QS::Client::Client> &GetClient() const {
    return m_client;
  }
  const std::unique_ptr<QS::Client::TransferManager> &GetTransferManager()
      const {
    return m_transferManager;
  }
  const std::unique_ptr<QS::Data::Cache> &GetCache() const { return m_cache; }
  const std::unique_ptr<QS::Data::DirectoryTree> &GetDirectoryTree() const {
    return m_directoryTree;
  }

 public:
  // Connect to object storage
  //
  // @param  : void
  // @return : flag of success
  //
  // Connect will build up the root level of directory tree asynchornizely.
  bool Connect() const;

  // Return the drive root node.
  std::shared_ptr<QS::Data::Node> GetRoot();

  // Return information about the mounted bucket.
  struct statvfs GetFilesystemStatistics();

  // Get the node
  //
  // @param  : file path, bool denotes if update dir tree if node is a directory
  // @return : a pair of { node, bool denotes if the node is modified comparing
  //           with the moment before this operation }
  //
  // Dir path should ending with '/'.
  // Using updateIfDirectory to invoke updating the directory tree
  // asynchronizely if node is directory, which means the children of the
  // directory will be add to the tree.
  //
  // Notes: GetNode will connect to object storage to retrive the object and 
  // update the local dir tree
  std::pair<std::weak_ptr<QS::Data::Node>, bool> GetNode(
      const std::string &path, bool updateIfDirectory = true);

  // Get the node from local dir tree
  // @param  : file path
  // @return : node
  //
  // GetNodeSimple just find the node in local dir tree
  std::weak_ptr<QS::Data::Node> GetNodeSimple(const std::string &path);

  // Get the children
  //
  // @param  : dir path
  // @return : a pair of iterators point to the range of the children in dir tree.
  //
  // This will update the directory tree synchronizely if directory is modified.
  std::pair<QS::Data::ChildrenMultiMapConstIterator,
            QS::Data::ChildrenMultiMapConstIterator>
  GetChildren(const std::string &dirPath);

  // Change the permission bits of a file
  //
  // @param  : file path, mode
  // @return : void
  void Chmod(const std::string &filePath, mode_t mode);

  // Change the owner and group of a file
  //
  // @param  : file path, uid, gid
  // @return : void
  void Chown(const std::string &filePath, uid_t uid, gid_t gid);

  // Delete a file
  //
  // @param  : file path, flag to do check
  // @return : void
  //
  // flag doCheck control whether to check the node existence
  // and its file type.
  void DeleteFile(const std::string &filePath, bool doCheck = true);

  // Delete a directory
  //
  // @param  : dir path, flag recursive, flag to do check
  // @return : void
  //
  // flag recursive control whether to delete the directory recursively.
  // If set recursive = false, DeleteDir can only delete an empty dir.
  // flag doCheck control whether to check the node existence, file type,
  // and if it's empty
  void DeleteDir(const std::string &dirPath, bool recursive,
                 bool doCheck = true);

  // Create a hard link to a file
  //
  // @param  : file path to link to, hard link path
  // @return : void
  //
  // Notes: hard link is only cached in local not in object storage,
  // So it could be removed, e.g. when updating its parent dir.
  void HardLink(const std::string &filePath, const std::string &hardlinkPath);

  // Create a file
  //
  // @param  : file path, file mode, dev
  // @return : void
  //
  // MakeFile is called for creation of non-directory, non-symlink nodes.
  void MakeFile(const std::string &filePath, mode_t mode, dev_t dev = 0);

  // Create a Directory
  //
  // @param  : dir path, file mode
  // @return : void
  void MakeDir(const std::string &dirPath, mode_t mode);

  // Open a file
  //
  // @param  : file path, flag to do check
  // @return : void
  void OpenFile(const std::string &filePath, bool doCheck = true);

  // Read data from a file
  //
  // @param  : file path to read data from, offset, size, buf, flag doCheck
  // @return : number of bytes has been read
  //
  // If cannot find or file need update, download it, otherwise read from cache.
  // For download, if besides the size need to be download for this time, the file
  // has more data need to be download, in this case, an asynchronize task will
  // be submit to download extra partial data of the file.
  //
  // Flag doCheck control whether to check the file existence and file type.
  size_t ReadFile(const std::string &filePath, off_t offset, size_t size,
                  char *buf, bool doCheck = true);

  // Rename a file
  //
  // @param  : file path, new file path, flag to do check
  // @return : void
  void RenameFile(const std::string &filePath, const std::string &newFilePath,
                  bool doCheck = true);

  // Rename a directory
  //
  // @param  : dir path, new dir path, flag to do check
  // @return : void
  void RenameDir(const std::string &dirPath, const std::string &newDirPath,
                 bool doCheck = true);

  // Create a symbolic link to a file
  //
  // @param  : file path to link to, link path
  // @return : void
  //
  // symbolic link is a file that contains a reference to another file or dir
  // in the form of an absolute path (in qsfs).
  // Notes: symbolic link is only cached in local not in object storage,
  // So it could be removed, e.g. when updating its parent dir.
  void SymLink(const std::string &filePath, const std::string &linkPath);

  // Truncate a file
  //
  // @param  : file path, new file size
  // @return : void
  void TruncateFile(const std::string &filePath, size_t newSize);

  // Upload a file
  //
  // @param  : file path to upload to object storage, flag doCheck
  // @return : void
  void UploadFile(const std::string &filePath, bool doCheck = true);

  // Change access and modification times of a file
  //
  // @param  : file path, mtime
  // @return : void
  void Utimens(const std::string &path, time_t mtime);

  // Write a file
  //
  // @param  : file path to write data to, buf containing data, size, offset
  // @return : number of bytes has been wrote
  int WriteFile(const std::string &filePath, off_t offset, size_t size,
                const char *buf, bool doCheck = true);

 private:
  // Download file contents
  //
  // @param  : file path, file content ranges, asynchronizely or synchronizely
  // @return : void
  void DownloadFileContentRanges(const std::string &filePath,
                                 const QS::Data::ContentRangeDeque &ranges,
                                 time_t mtime,
                                 bool async);

 private:
  std::shared_ptr<QS::Client::Client> &GetClient() { return m_client; }
  std::unique_ptr<QS::Client::TransferManager> &GetTransferManager() {
    return m_transferManager;
  }
  std::unique_ptr<QS::Data::Cache> &GetCache() { return m_cache; }
  std::unique_ptr<QS::Data::DirectoryTree> &GetDirectoryTree() {
    return m_directoryTree;
  }
  // mutator
  void SetClient(std::shared_ptr<QS::Client::Client> client);
  void SetTransferManager(
      std::unique_ptr<QS::Client::TransferManager> transferManager);
  void SetCache(std::unique_ptr<QS::Data::Cache> cache);
  void SetDirectoryTree(std::unique_ptr<QS::Data::DirectoryTree> dirTree);

 private:
  Drive();
  mutable std::atomic<bool> m_mountable;
  std::shared_ptr<QS::Client::Client> m_client;
  std::unique_ptr<QS::Client::TransferManager> m_transferManager;
  std::unique_ptr<QS::Data::Cache> m_cache;
  std::unique_ptr<QS::Data::DirectoryTree> m_directoryTree;
  std::unordered_map<std::string, std::shared_ptr<QS::Client::TransferHandle>,
                     HashUtils::StringHash>
      m_unfinishedMultipartUploadHandles;

  friend class QS::Client::QSClient;
  friend class QS::Client::QSTransferManager;  // for cache
  friend class QS::Data::Cache;  // for directory
  friend class QS::Data::File;   // for transfer manager
};

}  // namespace FileSystem
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_FILESYSTEM_DRIVE_H_
