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

#include "filesystem/Operations.h"

#include <assert.h>
#include <string.h>  // for memset, strlen

#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>  // for uid_t
#include <unistd.h>     // for R_OK

#include <memory>
#include <tuple>
#include <utility>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "data/Directory.h"
#include "filesystem/Configure.h"
#include "filesystem/Drive.h"

namespace QS {

namespace FileSystem {

using QS::Data::Node;
using QS::Exception::QSException;
using QS::FileSystem::Configure::GetNameMaxLen;
using QS::FileSystem::Configure::GetPathMaxLen;
using QS::FileSystem::Configure::GetDefineFileMode;
using QS::FileSystem::Drive;
using QS::StringUtils::AccessMaskToString;
using QS::StringUtils::FormatPath;
using QS::StringUtils::ModeToString;
using QS::StringUtils::Trim;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetBaseName;
using QS::Utils::GetDirName;
using QS::Utils::IsRootDirectory;
using std::pair;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::tuple;
using std::weak_ptr;

namespace {

// --------------------------------------------------------------------------
bool IsValidPath(const char* path) {
  return path != nullptr && path[0] != '\0';
}

// --------------------------------------------------------------------------
uid_t GetFuseContextUID() {
  static struct fuse_context* fuseCtx = fuse_get_context();
  return fuseCtx->uid;
}

// --------------------------------------------------------------------------
gid_t GetFuseContextGID() {
  static struct fuse_context* fuseCtx = fuse_get_context();
  return fuseCtx->gid;
}

// --------------------------------------------------------------------------
shared_ptr<Node> CheckParentDir(const string& path, int amode, int* ret,
                                bool updateIfisDir = false,
                                bool updateDirAsync = false) {
  // Normally, put CheckParentDir before check the file itself.
  string dirName = GetDirName(path);
  auto& drive = Drive::Instance();
  auto parent = drive.GetNodeSimple(dirName).lock();
  if (!parent) {
    auto res = drive.GetNode(dirName, updateIfisDir, updateDirAsync);
    parent = res.first.lock();
  }

  if (!(parent && *parent)) {
    *ret = -EINVAL;  // invalid argument
    throw QSException("No parent directory " + FormatPath(path));
  }

  // Check whether parent is directory
  if (!parent->IsDirectory()) {
    *ret = -EINVAL;  // invalid argument
    throw QSException("Parent is not a directory " + FormatPath(dirName));
  }

  // Check access permission
  if (!parent->FileAccess(GetFuseContextUID(), GetFuseContextGID(), amode)) {
    *ret = -EACCES;  // Permission denied
    throw QSException("No access permission (" + AccessMaskToString(amode) +
                      ") for directory" + FormatPath(dirName));
  }

  return parent;
}

// --------------------------------------------------------------------------
bool CheckOwner(uid_t uid) {
  return GetFuseContextUID() == 0 || GetFuseContextUID() == uid;
}

// --------------------------------------------------------------------------
void CheckStickyBit(const shared_ptr<Node>& dir, const shared_ptr<Node>& file,
                    int* ret) {
  // When a directory's sticky bit is set, the filesystem treats the files in
  // such directories in a special way so only the file's owner, the
  // directory's owner, or root user can rename or delete the file.
  uid_t uid = GetFuseContextUID();
  if ((S_ISVTX & dir->GetFileMode()) &&
      !(uid == 0 || uid == file->GetUID() || uid == dir->GetUID())) {
    *ret = -EPERM;  // operation not permitted
    throw QSException(
        "sticky bit set: only the owner/root user can delete the file [user=" +
        to_string(uid) + ", file owner=" + to_string(file->GetUID()) +
        ", dir owner=" + to_string(dir->GetUID()) + "] " +
        FormatPath(file->GetFilePath()));
  }
}

// --------------------------------------------------------------------------
void FillStat(const struct stat& source, struct stat* target) {
  assert(target != nullptr);
  target->st_size = source.st_size;
  target->st_blocks = source.st_blocks;
  target->st_blksize = source.st_blksize;
  target->st_atim = source.st_atim;
  target->st_mtim = source.st_mtim;
  target->st_ctim = source.st_ctim;
  target->st_uid = source.st_uid;
  target->st_gid = source.st_gid;
  target->st_mode = source.st_mode;
  target->st_dev = source.st_dev;
  target->st_nlink = source.st_nlink;
}

// --------------------------------------------------------------------------
void FillStatvfs(const struct statvfs& source, struct statvfs* target) {
  assert(target != nullptr);
  target->f_bsize = source.f_bsize;
  target->f_frsize = source.f_frsize;
  target->f_blocks = source.f_blocks;
  target->f_bfree = source.f_bfree;
  target->f_bavail = source.f_bavail;
  target->f_files = source.f_files;
  target->f_namemax = source.f_namemax;
}

// --------------------------------------------------------------------------
// Get the file from local dir tree
//
// @param  : path
// @return : {node, path_}
//          - 1st member is the node;
//          - 2nd member is the path maybe appended with "/"
//
pair<weak_ptr<Node>, string> GetFileSimple(const char *path) {
  string appendPath = path;
  auto& drive = Drive::Instance();
  auto node = drive.GetNodeSimple(path).lock();
  if (!node && string(path).back() != '/') {
    appendPath = AppendPathDelim(path);
    node = drive.GetNodeSimple(appendPath).lock();
  }
  return {node, appendPath};
}

// --------------------------------------------------------------------------
// Get the file
//
// @param  : path, flag update dir, flag update dir asynchronizely
// @return : {node, bool, path_}
//          - 1st member is the node;
//          - 2nd member denote if the node is modified comparing with the
//          moment before this operation.
//          - 3rd member is the path maybe appended with "/"
//
// Note: GetFile will connect to object storage to retrive the object and 
// update the local dir tree if the object is modified.
tuple<weak_ptr<Node>, bool, string> GetFile(const char* path,
                                            bool updateIfIsDir = false,
                                            bool updateDirAsync = false) {
  auto& drive = Drive::Instance();
  auto out = GetFileSimple(path);
  if (out.first.lock()) {  // found node in local dir tree
    // connect to object storage to update file
    auto path_ = out.second;
    auto res = drive.GetNode(path_, updateIfIsDir, updateDirAsync);
    return std::make_tuple(res.first, res.second, path_);
  } else {  // not found in local dir tree
    // connect to object storage to retrive file
    string appendPath = path;
    auto res = drive.GetNode(path, updateIfIsDir, updateDirAsync);
    auto node = res.first.lock();
    if (!node && string(path).back() != '/') {
      appendPath = AppendPathDelim(path);
      res = drive.GetNode(appendPath, updateIfIsDir, updateDirAsync);
    }
    return std::make_tuple(res.first, res.second, appendPath);
  }
}

}  // namespace

// --------------------------------------------------------------------------
void InitializeFUSECallbacks(struct fuse_operations* fuseOps) {
  memset(fuseOps, 0, sizeof(*fuseOps));  // clear input

  fuseOps->getattr = qsfs_getattr;
  fuseOps->readlink = qsfs_readlink;
  fuseOps->mknod = qsfs_mknod;
  fuseOps->mkdir = qsfs_mkdir;
  fuseOps->unlink = qsfs_unlink;
  fuseOps->rmdir = qsfs_rmdir;
  fuseOps->symlink = qsfs_symlink;
  fuseOps->rename = qsfs_rename;
  fuseOps->link = qsfs_link;
  fuseOps->chmod = qsfs_chmod;
  fuseOps->chown = qsfs_chown;
  fuseOps->truncate = qsfs_truncate;
  fuseOps->open = qsfs_open;
  fuseOps->read = qsfs_read;
  fuseOps->write = qsfs_write;
  fuseOps->statfs = qsfs_statfs;
  // fuseOps->flush = NULL;
  fuseOps->release = qsfs_release;
  // fuseOps->fsync = NULL;
  // fuseOps->setxattr = NULL;
  // fuseOps->getxattr = NULL;
  // fuseOps->listxattr = NULL;
  // fuseOps->removexattr = NULL;
  fuseOps->opendir = qsfs_opendir;
  fuseOps->readdir = qsfs_readdir;
  // fuseOps->releasedir = NULL;
  // fuseOps->fsyncdir = NULL;
  fuseOps->init = qsfs_init;
  fuseOps->destroy = qsfs_destroy;
  fuseOps->access = qsfs_access;
  fuseOps->create = qsfs_create;
  // fuseOps->ftruncate = NULL;
  // fuseOps->fgetattr = NULL;
  // fuseOps->lock = NULL;
  fuseOps->utimens = qsfs_utimens;
  // fuseOps->write_buf = NULL;
  // fuseOps->read_buf = NULL;
  // fuseOps->fallocate = NULL;
}

// --------------------------------------------------------------------------
// Get file attributes
//
// Similar to stat(). The 'st_dev' and 'st_blksize' fields are ignored. The
// 'st_ino' filed is ignored except if the 'use_ino' mount option is given.
int qsfs_getattr(const char* path, struct stat* statbuf) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (statbuf == nullptr) {
    Error("Null statbuf parameter from fuse");
    return -EINVAL;
  }

  memset(statbuf, 0, sizeof(*statbuf));
  int ret = 0;
  try {
    // Getattr is invoked before most callbacks to decide if path is existing,
    // We do update dir in opendir instead here.

    // Check parent access permission
    CheckParentDir(path, X_OK, &ret, false);  // should always put at beginning

    // Check file
    auto res = GetFile(path, false);  // not update dir
    auto node = std::get<0>(res).lock();
    if (node && *node) {
      auto st = const_cast<const Node&>(*node).GetEntry().ToStat();
      FillStat(st, statbuf);
    } else {          // node not existing
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatPath(path));
    }
  } catch (const QSException& err) {
    Warning(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Read the target of a symbolic link
//
// The buffer should be filled with a null terminated string. The buffer size
// argument includes the space for the terminating null character. If the link
// name is too long to fit in the buffer, it should be truncated.
//
// FUSE Invariants (https://github.com/libfuse/libfuse/wiki/Invariants)
// The arguments is already verified
// Readlink is only called with an existing symlink.
int qsfs_readlink(const char* path, char* link, size_t size) {
  // if (!IsValidPath(path)) {
  //   Error("Null path parameter from fuse");
  //   return -EINVAL;  // invalid argument
  // }
  // if (link == nullptr) {
  //   Error("Null buffer parameter from fuse");
  //   return -EINVAL;
  // }
  // if (IsRootDirectory(path)) {
  //   Error("Unable to link on root directory");
  //   return -EPERM;  // operation not permitted
  // }

  memset(link, 0, size);
  int ret = 0;
  try {
    auto res = GetFileSimple(path);
    auto node = res.first.lock();
    string path_ = res.second;
    
    if (!(node && *node)) {
      ret = -ENOLINK;  // Link has been severed
      throw QSException("No such file " + FormatPath(path_));
    }

    // Check whether it is a symlink
    if (!node->IsSymLink()) {
      assert(false);  // should not happen
      ret = -EINVAL;  // invalid argument
      throw QSException("Not a symlink " + FormatPath(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read permission " + FormatPath(path_));
    }

    // Read the link
    Drive::Instance().ReadSymlink(path);
    auto symlink = Trim(node->GetSymbolicLink(), ' ');
    size_t size_ = symlink.size();
    if (size <= size_) {
      size_ = size - 1;
    }
    memcpy(link, symlink.c_str(), size_);
    link[size_] = '\0';

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Create a file node
//
// This is called for creation of all non-directory, non-symlink nodes.
// If the filesystem defines a create() method, then for regular files that
// will be called instead.
int qsfs_mknod(const char* path, mode_t mode, dev_t dev) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to create root directory");
    return -EPERM;  // operation not permitted
  }
  // Check whether filename is too long
  string filename = GetBaseName(path);
  if (filename.size() > GetNameMaxLen()) {
    Error("File name too long [name=" + filename + "]");
    return -ENAMETOOLONG;  // file name too long
  }
  // Check whether the pathname is too long
  if (strlen(path) > GetPathMaxLen()) {
    Error("Path name too long " + FormatPath(path));
    return -ENAMETOOLONG;  // name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory permission
    CheckParentDir(path, W_OK | X_OK, &ret, false);
    
    auto node = drive.GetNodeSimple(path).lock();
    if (node && *node) {
      ret = -EEXIST;  // File exist
      throw QSException("File already exists " + FormatPath(path));
    }

    // Create the new node
    drive.MakeFile(path, mode | GetDefineFileMode(), dev);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Create a directory
//
// Note that the mode argument may not have the type specific bits set, i.e.
// S_ISDIR(mode) can be false. To obtain the correct directory type bits use
// mode|S_IFDIR.
int qsfs_mkdir(const char* path, mode_t mode) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to create root directory");
    return -EPERM;  // operation not permitted
  }
  // Check whether filename is too long
  string filename = GetBaseName(path);
  if (filename.size() > GetNameMaxLen()) {
    Error("File name too long [name=" + filename + "]");
    return -ENAMETOOLONG;  // file name too long
  }
  // Check whether the pathname is too long
  if (strlen(path) > GetPathMaxLen()) {
    Error("Path name too long " + FormatPath(path));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    CheckParentDir(path, W_OK | X_OK, &ret, false);

    auto res = GetFileSimple(path);
    auto node = res.first.lock();
    string path_ = res.second;
    if (node && *node) {
      ret = -EEXIST;  // File exist
      throw QSException("File already exists " + FormatPath(path_));
    }

    // Create the directory
    drive.MakeDir(AppendPathDelim(path), mode | S_IFDIR);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Remove a file
int qsfs_unlink(const char* path) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to remove root directory");
    return -EPERM;  // operation not permitted
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    auto dir = CheckParentDir(path, W_OK | X_OK, &ret, false);

    auto node = drive.GetNodeSimple(path).lock();  // getattr synchornized node
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file " + FormatPath(path));
    }

    // Check stick bits
    CheckStickyBit(dir, node, &ret);

    // Remove the file
    if (node->IsDirectory()) {
      ret = -EINVAL;  // invalid argument
      throw QSException("Not a file, but a directory " + FormatPath(path));
    } else {
      drive.RemoveFile(path);
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Remove a directory
int qsfs_rmdir(const char* path) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to remove root directory");
    return -EPERM;  // operation not permitted
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    auto dir = CheckParentDir(path, W_OK | X_OK, &ret, false);

    string path_ = AppendPathDelim(path);
    auto res = drive.GetNode(path_, true);  // update dir synchronizely
    auto node = res.first.lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such directory " + FormatPath(path_));
    }

    if (!node->IsDirectory()) {
      ret = -EINVAL;  // invalid argument
      throw QSException("Not a directory " + FormatPath(path_));
    }

    // Check whether the directory empty
    if (!node->IsEmpty()) {
      ret = -ENOTEMPTY;  // directory not empty
      throw QSException("Unable to remove, directory is not empty " +
                        FormatPath(path_));
    }

    // Check sticky bit
    CheckStickyBit(dir, node, &ret);

    // Do delete empty directory
    drive.RemoveFile(path_);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Create a symbolic link
//
// FUSE Invariants (https://github.com/libfuse/libfuse/wiki/Invariants)
// The arguments is already verified
// Symlink is only called if there isn't already another object with the
// requested linkname.
int qsfs_symlink(const char* path, const char* link) {
  //
    // if (!IsValidPath(path)) {
    //   Error("Null path parameter from fuse");
    //   return -EINVAL;  // invalid argument
    // }
    // if (!IsValidPath(link)) {
    //   Error("Null link parameter from fuse");
    //   return -EINVAL;  // invalid argument
    // }
    // if (IsRootDirectory(path)) {
    //   Error("Unable to symlink root directory");
    //   return -EPERM;  // operation not permitted
    // }

  string filename = GetBaseName(link);
  if (filename.empty()) {
    Error("Invalid link parameter " + FormatPath(link));
    return -EINVAL;
  } else if (filename.size() > GetNameMaxLen()) {
    Error("filename too long [name=" + filename +"]");
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check link parent directory
    CheckParentDir(link, W_OK | X_OK, &ret, false);

    auto res = GetFileSimple(link);
    auto node = res.first.lock();
    string link_ = res.second;
    if (node && *node) {
      ret = -EEXIST;  // File exist
      throw QSException("File already exists " + FormatPath(link_));
    }

    // Create a symbolic link
    drive.SymLink(path, link);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Rename a file
//
// If new file name exists and is a non empty directory, the filesystem will
// not overwrite new file name and return an error (ENOTEMPTY) instead.
// Otherwise, the filesystem will replace the new file name.
int qsfs_rename(const char* path, const char* newpath) {
  if (!IsValidPath(path) || !IsValidPath(newpath)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path) || IsRootDirectory(newpath)) {
    Error("Unable to rename on root directory");
    return -EPERM;  // operation not permitted
  }
  string newPathBaseName = GetBaseName(newpath);
  if (newPathBaseName.empty()) {
    Error("Invalid new file path " + FormatPath(newpath));
    return -EINVAL;
  } else if (newPathBaseName.size() > GetNameMaxLen()) {
    Error("File name too long [name=" + newPathBaseName +"]");
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  try {
    // Check parent permission
    auto dir = CheckParentDir(path, W_OK | X_OK, &ret, false);

    auto res = GetFile(path, true);  // update dir synchronizely
    auto node = std::get<0>(res).lock();
    string path_ = std::get<2>(res);
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatPath(path_));
    }

    // Check sticky bits
    CheckStickyBit(dir, node, &ret);

    // Delete newpath if it exists and it's an empty directory
    auto nRes = GetFile(newpath, true);  // update dir synchronizely
    auto nNode = std::get<0>(nRes).lock();
    string newpath_ = std::get<2>(nRes);
    if (nNode) {
      if (nNode->IsDirectory() && !nNode->IsEmpty()) {
        ret = -ENOTEMPTY;  // directory not empty
        throw QSException("Unable to rename, directory not empty " +
                          FormatPath(path_, newpath_));
      } else {
        // Check new path parent permission
        CheckParentDir(newpath_, W_OK | X_OK, &ret, false);

        // Delete the file or empty directory with new file name
        Warning("File exists, replace it " + FormatPath(newpath_));
        Drive::Instance().RemoveFile(newpath_);
      }
    }

    // Do Renaming
    if (node->IsDirectory()) {
      // rename dir asynchronizely
      Drive::Instance().RenameDir(path_, AppendPathDelim(newpath), true);
    } else {
      Drive::Instance().RenameFile(path_, newpath);
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Create a hard link to a file
int qsfs_link(const char* path, const char* linkpath) {
  DebugError("Hard link not permitted [from=" + string(path) +
             " to=" + string(linkpath));
  return -EPERM;  // operation not permitted

//  Not support hard link currently.
  // if (!IsValidPath(path) || !IsValidPath(linkpath)) {
  //   Error("Null path parameter from fuse");
  //   return -EINVAL;  // invalid argument
  // }
  // if (IsRootDirectory(path) || IsRootDirectory(linkpath)) {
  //   Error("Unable to link on root directory");
  //   return -EPERM;  // operation not permitted
  // }
  // string linkPathBaseName = GetBaseName(linkpath);
  // if (linkPathBaseName.empty()) {
  //   Error("Invalid link file path " + FormatPath(linkpath));
  //   return -EINVAL;
  // } else if (linkPathBaseName.size() > GetNameMaxLen()) {
  //   Error("file name too long [name=" + linkPathBaseName + "]");
  //   return -ENAMETOOLONG;  // file name too long
  // }
  // 
  // int ret = 0;
  // auto& drive = Drive::Instance();
  // try {
  //   auto node = drive.GetNodeSimple(path).lock();
  //   if (!(node && *node)) {
  //     ret = -ENOENT;  // No such file or directory
  //     throw QSException("No such file or directory " + FormatPath(path));
  //   }
  // 
  //   // Check if it is a directory
  //   if (node->IsDirectory()) {
  //     ret = -EPERM;  // operation not permitted
  //     throw QSException("Unable to hard link to a directory " +
  //                       FormatPath(path));
  //   }
  // 
  //   // Check access permission
  //   if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(),
  //                         (W_OK | R_OK))) {
  //     ret = -EACCES;  // Permission denied
  //     throw QSException("No read and write permission for path " +
  //                       FormatPath(path));
  //   }
  // 
  //   // Check the directory where link is to be created
  //   CheckParentDir(linkpath, W_OK | X_OK, &ret, false);
  // 
  //   // Check if linkpath existing
  //   auto lnkRes = GetFileSimple(linkpath);
  //   auto lnkNode = lnkRes.first.lock();
  //   string linkpath_ = lnkRes.second;  
  //   if (lnkNode && *lnkNode) {
  //     ret = -EEXIST;
  //     throw QSException("File already exists for link path " +
  //                       FormatPath(linkpath_));
  //   }
  // 
  //   // Create hard link
  //   drive.HardLink(path, linkpath_);
  // 
  // } catch (const QSException& err) {
  //   Error(err.get());
  //   if (ret == 0) {  // catch exception from lower level
  //     ret = -errno;
  //   }
  //   return ret;
  // }
  // 
  // return ret;

}

// --------------------------------------------------------------------------
// Change the permission bits of a file
int qsfs_chmod(const char* path, mode_t mode) {
  DebugInfo("Trying to change permisions to " + ModeToString(mode) +
            " for path" + FormatPath(path));
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to chmod on root directory");
    return -EPERM;  // operation not permitted
  }
  // Check whether the pathname is too long
  if (strlen(path) > GetPathMaxLen()) {
    Error("Path name too long " + FormatPath(path));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // "Getattr()" is called before this callback, which already checked X_OK
    // Check parent access permission
    // CheckParentDir(path, X_OK, &ret, false);

    auto res = GetFileSimple(path);
    auto node = res.first.lock();
    string path_ = res.second;
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatPath(path));
    }

    // Check owner
    if (!CheckOwner(node->GetUID())) {
      ret = -EPERM;  // operation not permitted
      throw QSException("Only owner/root can change file permissions [user=" +
                        to_string(GetFuseContextUID()) + ",owner=" +
                        to_string(node->GetUID()) + "] " + FormatPath(path_));
    }

    // Change the file permission
    drive.Chmod(path_, mode);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Change the owner and group of a file
int qsfs_chown(const char* path, uid_t uid, gid_t gid) {
  DebugInfo("Trying to change owner and group to [uid=" + to_string(uid) +
            ", gid=" + to_string(gid) + "]" + FormatPath(path));
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to chown on root directory");
    return -EPERM;  // operation not permitted
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // "Getattr()" is called before this callback, which already checked X_OK
    // Check parent access permission
    // CheckParentDir(path, X_OK, &ret, false);

    // Check if file exists
    auto res = GetFileSimple(path);
    auto node = res.first.lock();
    string path_ = res.second;
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatPath(path));
    }

    // Check owner
    if (!CheckOwner(node->GetUID())) {
      ret = -EPERM;  // operation not permitted
      throw QSException(
          "Only owner/root can change file owner and group [user=" +
          to_string(GetFuseContextUID()) +
          ",owner=" + to_string(node->GetUID()) + "] " + FormatPath(path_));
    }

    // Change owner and group
    drive.Chown(path_, uid, gid);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Change the size of a file
int qsfs_truncate(const char* path, off_t newsize) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (newsize < 0) {
    Error("Invalid new size parameter [size=" + to_string(newsize) +"]");
    return -EINVAL;
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // "Getattr()" is called before this callback, which already checked X_OK
    // Check parent permission
    // CheckParentDir(path, X_OK, &ret, false);

    auto node = drive.GetNodeSimple(path).lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatPath(path));
    }

    // Check if it is a directory
    if (node->IsDirectory()) {
      ret = -EPERM;  // operation not permitted
      throw QSException("Unable to truncate a directory " + FormatPath(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), W_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No write permission for path " + FormatPath(path));
    }

    // Do truncating
    drive.TruncateFile(path, newsize);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// File open operation
//
// No creation (O_CREAT, O_EXCL) and by default also no
// truncation (O_TRUNC) flags will be passed to open(). If an
// application specifies O_TRUNC, fuse first calls truncate()
// and then open(). Only if 'atomic_o_trunc' has been
// specified and kernel version is 2.6.24 or later, O_TRUNC is
// passed on to open.
//
// Unless the 'default_permissions' mount option is given,
// open should check if the operation is permitted for the
// given flags. Optionally open may also return an arbitrary
// filehandle in the fuse_file_info structure, which will be
// passed to all file operations.
int qsfs_open(const char* path, struct fuse_file_info* fi) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    if(static_cast<unsigned int>(fi->flags) & O_TRUNC){
      drive.TruncateFile(path, 0);
    } else {
      // Check parent directory
      string dirName = GetDirName(path);
      auto parent = drive.GetNodeSimple(dirName).lock();
      if (!(parent && *parent)) {
        ret = -EINVAL;  // invalid argument
        throw QSException("No parent directory " + FormatPath(path));
      }
  
      // "Getattr()" is called before this callback, which already checked X_OK
      // Check parent permission
      // CheckParentDir(path, X_OK, &ret, false);

      auto node = drive.GetNodeSimple(path).lock();
      if (node && *node) {
        // Check if it is a directory
        if (node->IsDirectory()) {
          ret = -EPERM;  // operation not permitted
          throw QSException("Not a file, but a directory " + FormatPath(path));
        }
  
        // Check access permission
        if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
          ret = -EACCES;  // Permission denied
          throw QSException("No read permission for path " + FormatPath(path));
        }
      } else {
        // Check access permission
        if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), W_OK)) {
          ret = -EACCES;  // Permission denied
          throw QSException("No write permission for path " + FormatPath(path));
        }
        // Create a empty file
        drive.MakeFile(path, GetDefineFileMode());
      }
  
      // Do Open
      drive.OpenFile(path, false);  // load file synchronizely if not exist
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Read data from an open file
//
// Read should return exactly the number of bytes requested except on EOF or
// error, otherwise the rest of the data will be substituted with zeroes. An
// exception to this is when the 'direct_io' mount option is specified, in which
// case the return value of the read system call will reflect the return value
// of this operation.
//
// FUSE Invariants (https://github.com/libfuse/libfuse/wiki/Invariants)
// Read are only called if the file has been opend with the correct flags.
int qsfs_read(const char* path, char* buf, size_t size, off_t offset,
              struct fuse_file_info* fi) {
  // if (!IsValidPath(path)) {
  //   Error("Null path parameter from fuse");
  //   errno = EINVAL;  // invalid argument
  //   return 0;
  // }
  // if (buf == nullptr) {
  //   Error("Null buf parameter from fuse");
  //   errno = EINVAL;
  //   return 0;
  // }

  if (size == 0) {
    // Test shows fuse may call read with size = 0, offset = file size
    // For such case, just return
    return 0;
  }

  int readSize = 0;
  auto& drive = Drive::Instance();
  try {
    // Check if file exists
    auto node = drive.GetNodeSimple(path).lock();
    if (!(node && *node)) {
      errno = ENOENT;  // No such file or directory
      throw QSException("No such file " + FormatPath(path));
    }

    // Check if it is a directory
    if (node->IsDirectory()) {
      errno = EPERM;  // operation not permitted
      throw QSException("Not a file, but a directory " + FormatPath(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
      errno = EACCES;  // Permission denied
      throw QSException("No read permission for path " + FormatPath(path));
    }

    // Do Read
    try {
      readSize = drive.ReadFile(path, offset, size, buf);
    } catch (const QSException& err) {
      errno = EAGAIN;  // try again
      throw;           // rethrow
    }

  } catch (const QSException& err) {
    Error(err.get());
    return readSize;
  }

  return readSize;
}

// --------------------------------------------------------------------------
// Write data to an open file
//
// Write should return exactly the number of bytes requested except on error. An
// exception to this is when the 'direct_io' mount option is specified (see read
// operation).
//
// FUSE Invariants (https://github.com/libfuse/libfuse/wiki/Invariants)
// Write is only called if the file has been opened with the correct flags.
int qsfs_write(const char* path, const char* buf, size_t size, off_t offset,
               struct fuse_file_info* fi) {
  // if (!IsValidPath(path)) {
  //   Error("Null path parameter from fuse");
  //   errno = EINVAL;
  //   return 0;  // invalid argument
  // }
  // if (buf == nullptr) {
  //   Error("Null buf parameter from fuse");
  //   errno = EINVAL;
  //   return 0;
  // }

  int writeSize = 0;
  auto& drive = Drive::Instance();
  try {
    // Check if file exists
    auto node = drive.GetNodeSimple(path).lock();
    if (!(node && *node)) {
      errno = ENOENT;  // No such file or directory
      throw QSException("No such file " + FormatPath(path));
    }

    // Check if it is a directory
    if (node->IsDirectory()) {
      errno = EPERM;  // operation not permitted
      throw QSException("Not a file, but a directory " + FormatPath(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), W_OK)) {
      errno = EACCES;  // Permission denied
      throw QSException("No write permission for path " + FormatPath(path));
    }

    // Do Write
    try {
      writeSize = drive.WriteFile(path, offset, size, buf);
    } catch (const QSException& err) {
      errno = EAGAIN;  // try again
      throw;           // rethrow
    }

  } catch (const QSException& err) {
    Error(err.get());
    return writeSize;
  }

  return writeSize;
}

// --------------------------------------------------------------------------
// Get filesystem statistics
//
// The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored.
int qsfs_statfs(const char* path, struct statvfs* statv) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (statv == nullptr) {
    Error("Null statvfs parameter from fuse");
    return -EINVAL;
  }

  memset(statv, 0, sizeof(*statv));
  int ret = 0;
  try {
    // Check whether path existing within the mounted filesystem
    auto res = GetFileSimple(path);
    auto node = res.first.lock();
    if (node && *node) {
      // Set qsfs parameters
      struct statvfs stfs = Drive::Instance().GetFilesystemStatistics();
      FillStatvfs(stfs, statv);
    } else {          // node not existing
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatPath(path));
    }
  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Possibly flush cahced data
//
// NOT equivalent to fsync().
//
// Flush is called on each close() of a file descriptor. So if a filesystem
// wants to return write errors in close() and the file has cached dirty data,
// this is a good place to write back data and return any errors. Since many
// applications ignore close() errors this is not always useful.
//
// NOTE: the flush() method may be called more thance once for each open().
// Filesystes shouldn't assume that flus will always be called after some
// writes, or that if will be called at all.
int qsfs_flush(const char* path, struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Release an open file
//
// This will put the file to the object storage like qingstor.
//
// Release is called when there are no more references to an open file,
// all file descriptors are closed and all memory mapping are unmapped.
// For every open() call there will be exactly one release() with the
// same flags and file descriptor. It is possible to have a file opened more
// than once, in which case only the last release will mean, that no more
// reads/writes will happen on the file.
int qsfs_release(const char* path, struct fuse_file_info* fi) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }

  int ret = 0;
  try {
    // "Getattr()" is called before this callback, which already checked X_OK
    // Check parent permission
    // CheckParentDir(path, X_OK, &ret, false);

    // Check whether path existing
    auto res = GetFileSimple(path);
    auto node = res.first.lock();
    string path_ = res.second;
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatPath(path_));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read permission " + FormatPath(path_));
    }

    // Write the file to object storage
    if (node->IsNeedUpload()) {
      try {
        Drive::Instance().UploadFile(path_);
      } catch (const QSException& err) {
        Error(err.get());
        return -EAGAIN;  // Try again
      }
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Synchronize file contents
//
// If the datasync parameter is non-zero, then only the user data should be
// flushed, not the meta data.
int qsfs_fsync(const char* path, int datasync, struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Set extended attributes
int qsfs_setxattr(const char* path, const char* name, const char* value,
                  size_t size, int flags) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Get extended attributes
int qsfs_getxattr(const char* path, const char* name, char* value,
                  size_t size) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// List extended attributes
int qsfs_listxattr(const char* path, char* list, size_t size) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Remove extended attributes
int qsfs_removexattr(const char* path, const char* name) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Open directory
//
// Unless the 'default_permissions' mount option is given, this method should
// check if opendir is permitted for this directory. Optionally opendir may also
// return an arbitrary file handle in the fuse_file_info structure, which will
// be passed to readdir, closedir and fsyncdir.
int qsfs_opendir(const char* path, struct fuse_file_info* fi) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }

  int mask = (O_RDONLY != (fi->flags & O_ACCMODE) ? W_OK : R_OK) | X_OK;

  int ret = 0;
  auto& drive = Drive::Instance();
  auto dirPath = AppendPathDelim(path);
  try {
    // Check parent permission
    CheckParentDir(path, mask, &ret, false);

    // Check if dir exists
    auto node = drive.GetNodeSimple(dirPath).lock();

    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such directory " + FormatPath(path));
    }

    // Check if file is dir
    if (!node->IsDirectory()) {
      ret = -ENOTDIR;  // Not a directory
      throw QSException("Not a directory " + FormatPath(dirPath));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), mask)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read permission " + FormatPath(dirPath));
    }

    if (node->IsEmpty()) {
      drive.GetNode(dirPath, true);  // update dir synchronizely
    }
  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Read directory.
//
// Ignores the offset parameter, and passes zero to the filler function's
// offset. The filler function will not return '1' (unless an error happens),
// so the whole directory is read in a single readdir operation.
//
// FUSE Invariants (https://github.com/libfuse/libfuse/wiki/Invariants)
// Readdir is only called with an existing directory name
int qsfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info* fi) {
  // if (!IsValidPath(path)) {
  //   Error("Null path parameter from fuse");
  //   return -EINVAL;  // invalid argument
  // }
  // if (buf == nullptr) {
  //   Error("Null buffer parameter from fuse");
  //   return -EINVAL;
  // }

  int ret = 0;
  auto& drive = Drive::Instance();
  auto dirPath = AppendPathDelim(path);
  try {
    // Check if dir exists
    // As opendir get called before readdir, no need to update dir again.
    auto node = drive.GetNodeSimple(dirPath).lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such directory " + FormatPath(path));
    }

    // Check if file is dir
    if (!node->IsDirectory()) {
      ret = -ENOTDIR;  // Not a directory
      throw QSException("Not a directory " + FormatPath(dirPath));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read permission " + FormatPath(dirPath));
    }

    // Put the . and .. entries in the filler
    if (filler(buf, ".", NULL, 0) == 1 || filler(buf, "..", NULL, 0) == 1) {
      ret = -ENOMEM;  // out of memeory
      throw QSException("Fuse filler is full! dir: " + dirPath);
    }

    // Put the children into filler
    auto childs = drive.FindChildren(dirPath, false);
    for (auto &child : childs){
      if (auto childNode = child.lock()) {
        auto filename = childNode->MyBaseName();
        assert(!filename.empty());
        if (filename.empty()) continue;
        if (filler(buf, filename.c_str(), NULL, 0) == 1) {
          ret = -ENOMEM;  // out of memory
          throw QSException("Fuse filler is full! dir: " + dirPath +
                            "child: " + filename);
        }
      }
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Release a directory.
int qsfs_releasedir(const char* path, struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Synchronize directory contents
//
// If the datasync parameter is non-zero, then only the user data should be
// flushed, not the meta data.
int qsfs_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Initialize filesystem.
//
// The return value will passed in the private_data field of fuse_context
// to all file operations, and as a parameter to the destroy() method.
//
// It overrides the initial value provided to fuse_main() / fuse_new().
void* qsfs_init(struct fuse_conn_info* conn) {
  // Initialization and checking are done when mounting, and we design Drive
  // as a singleton. So just print info here.
  Info("Connecting qsfs...");
  return NULL;
}

// --------------------------------------------------------------------------
// Clean up filesystem.
//
// Called on filesystem exit.
void qsfs_destroy(void* userdata) {
  // Drive get clean by itself. Just print an info here.
  Info("Disconnecting qsfs...");
}

// --------------------------------------------------------------------------
// Check file access permissions
//
// This will be called for the access() system call. If the
// 'default_permissions' mount option is given, this method is not called.
//
// This method is not called under Linux kernel versions 2.4.x
int qsfs_access(const char* path, int mask) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }

  int ret = 0;
  try {
    // Check whether file exists
    auto res = GetFile(path, true, true);  // update dir asynchronizely
    auto node = std::get<0>(res).lock();
    string path_ = std::get<2>(res);
    if (!(node && *node)) {
      ret = -ENOENT;
      throw QSException("No such file or directory " + FormatPath(path_));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), mask)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No access permission(" + AccessMaskToString(mask) +
                        ") for path " + FormatPath(path_));
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Create and open a file.
//
// If the file does not exist, first create it with the specified mode, and
// then open it.
//
// If this method is not implemented or under Linux Kernal verions earlier
// than 2.6.15, the mknod() and open() methods will be called instead.
int qsfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to create root directory");
    return -EPERM;  // operation not permitted
  }
  // Check whether filename is too long
  string filename = GetBaseName(path);
  if (filename.size() > GetNameMaxLen()) {
    Error("File name too long [name=" + filename +"]");
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    CheckParentDir(path, W_OK | X_OK, &ret, false);

    // Check whether path exists
    auto node = drive.GetNodeSimple(path).lock();
    if (node && *node) {
      ret = -EEXIST;  // File exist
      throw QSException("File already exists " + FormatPath(path));
    }

    // Create the new node
    drive.MakeFile(path, mode);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Change the size of an open file
//
// This method is called instead of the truncate() method if the
// truncation was invoked from an ftruncate() system call.
//
// If this method is not implemented or under Linux kernel
// versions earlier than 2.6.15, the truncate() method will be
// called instead.
int qsfs_ftruncate(const char* path, off_t offset, struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Get attributes from an open file
//
// This method is called instead of the getattr() method if the
// file information is available.
//
// Currently this is only called after the create() method if that
// is implemented (see above).  Later it may be called for
// invocations of fstat() too.
int qsfs_fgetattr(const char* path, struct stat* statbuf,
                  struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Perform POSIX file locking operation
//
// Note: if this method is not implemented, the kernel will still allow file
// locking to work locally. Hence it is only interesting for network filesystems
// and similar.
int qsfs_lock(const char* path, struct fuse_file_info* fi, int cmd,
              struct flock* lock) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Change the access and modification times of a file with
// nanosecond resolution
//
// See the utimensat(2) man page for details.
int qsfs_utimens(const char* path, const struct timespec tv[2]) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to change mtime for root directory");
    return -EPERM;  // operation not permitted
  }

  int ret = 0;
  try {
    // "Getattr()" is called before this callback, which already checked X_OK
    // Check parent directory access permission
    // CheckParentDir(path, X_OK, &ret, false);

    // Check whether file exists
    auto res = GetFileSimple(path);
    auto node = res.first.lock();
    string path_ = res.second;
    if (!(node && *node)) {
      ret = -ENOENT;
      throw QSException("No such file or directory " + FormatPath(path_));
    }

    // Check file access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), W_OK) &&
        !CheckOwner(node->GetUID())) {
      ret = -EPERM;  // Operation not permitted
      throw QSException("No write permission and No owner/root user [user=" +
                        to_string(GetFuseContextUID()) + ",owner=" +
                        to_string(node->GetUID()) + "] " + FormatPath(path_));
    }

    time_t mtime = tv[1].tv_sec;
    Drive::Instance().Utimens(path_, mtime);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
    return ret;
  }

  return ret;
}

// --------------------------------------------------------------------------
// Write contents of buffer to an open file
//
// Similar to the write() method, but data is supplied in a generic buffer.
int qsfs_write_buf(const char* path, struct fuse_bufvec* buf, off_t off,
                   struct fuse_file_info*) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Store data from an open file in a buffer
//
// Similar to the read() method, but data is stored and returned in a generic
// buffer.
//
// No actual copying of data has to take place, the source file descriptor may
// simply be stored in the buffer for later data transfer.
//
// The buffer must be allocated dynamically and stored at the location pointed
// to by bufp. If the buffer contains memory regions, they too must be allocated
// using malloc(). The allocated memory will be freed by the caller.
int qsfs_read_buf(const char* path, struct fuse_bufvec** bufp, size_t size,
                  off_t off, struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Allocates space for an open file
//
// This function ensures that required space is allocated for specified file. If
// this function returns success then any subsequent write request to specified
// range is guaranteed not to fail because of lack of space on the file system
// media
int qsfs_fallocate(const char* path, int, off_t offseta, off_t offsetb,
                   struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

}  // namespace FileSystem
}  // namespace QS
