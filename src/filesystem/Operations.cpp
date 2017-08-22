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
using QS::StringUtils::Trim;
using QS::Utils::AppendPathDelim;
using QS::Utils::AccessMaskToString;
using QS::Utils::GetBaseName;
using QS::Utils::GetDirName;
using QS::Utils::IsRootDirectory;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::tuple;
using std::weak_ptr;

namespace {

bool IsValidPath(const char* path) {
  return path != nullptr && path[0] != '\0';
}

string FormatArg(const string& para) { return "(arg = " + para + ")"; }

uid_t GetFuseContextUID() {
  static struct fuse_context* fuseCtx = fuse_get_context();
  return fuseCtx->uid;
}

gid_t GetFuseContextGID() {
  static struct fuse_context* fuseCtx = fuse_get_context();
  return fuseCtx->gid;
}

shared_ptr<Node> CheckParentDir(const string& path, int amode, int* ret) {
  // Check parent with invoking asynchronizely update
  string dirName = GetDirName(path);
  auto& drive = Drive::Instance();
  auto res = drive.GetNode(dirName, true);
  auto parent = res.first.lock();

  if (!(parent && *parent)) {
    *ret = -EINVAL;  // invalid argument
    throw QSException("No parent directory " + FormatArg(path));
  }

  // Check whether parent is directory
  if (!parent->IsDirectory()) {
    *ret = -EINVAL;  // invalid argument
    throw QSException("Parent is not a directory " + FormatArg(dirName));
  }

  // Check access permission
  if (!parent->FileAccess(GetFuseContextUID(), GetFuseContextGID(), amode)) {
    *ret = -EACCES;  // Permission denied
    throw QSException("No permission (" + AccessMaskToString(amode) +
                      ") for directory" + FormatArg(dirName));
  }

  return parent;
}

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

// Return a tuple:
// - the 1st member is the node;
// - the 2nd member denote if the node is modified comparing with the moment
// before this operation.
// - the 3rd member is the path maybe appended with "/"
tuple<weak_ptr<Node>, bool, string> GetFile(const char* path,
                                            bool updateIfIsDir) {
  string appendPath = path;
  auto& drive = Drive::Instance();
  auto res = drive.GetNode(path, updateIfIsDir);
  auto node = res.first.lock();
  if (!node && string(path).back() != '/') {
    appendPath = AppendPathDelim(path);
    res = drive.GetNode(appendPath, updateIfIsDir);
  }
  return std::make_tuple(res.first, res.second, appendPath);
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
  fuseOps->flush = NULL;  // TODO
  fuseOps->release = qsfs_release;
  fuseOps->fsync = NULL;        // TODO
  fuseOps->setxattr = NULL;     // TODO
  fuseOps->getxattr = NULL;     // TODO
  fuseOps->listxattr = NULL;    // TODO
  fuseOps->removexattr = NULL;  // TODO
  fuseOps->opendir = NULL;      // TODO
  fuseOps->readdir = qsfs_readdir;
  fuseOps->releasedir = NULL;
  fuseOps->fsyncdir = NULL;
  fuseOps->init = qsfs_init;
  fuseOps->destroy = qsfs_destroy;
  fuseOps->access = qsfs_access;
  fuseOps->create = qsfs_create;
  fuseOps->ftruncate = NULL;
  fuseOps->fgetattr = NULL;
  fuseOps->lock = NULL;
  fuseOps->utimens = qsfs_utimens;
  fuseOps->write_buf = NULL;
  fuseOps->read_buf = NULL;
  fuseOps->fallocate = NULL;
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
    auto res = GetFile(path, true);  // asynchornize update if is dir
    auto node = std::get<0>(res).lock();
    if (node && *node) {
      // TODO(jim): should associate file type to node symbolic link as gdfs?
      // currently gdfs do this in getattr, mknod, link
      auto st = const_cast<const Node&>(*node).GetEntry().ToStat();
      FillStat(st, statbuf);
    } else {          // node not existing
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatArg(path));
    }
  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
  }
  return ret;
}

// --------------------------------------------------------------------------
// Read the target of a symbolic link
//
// The buffer should be filled with a null terminated string. The buffer size
// argument includes the space for the terminating null character. If the link
// name is too long to fit in the buffer, it should be truncated.
int qsfs_readlink(const char* path, char* link, size_t size) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (link == nullptr) {
    Error("Null buffer parameter from fuse");
    return -EINVAL;
  }
  if (IsRootDirectory(path)) {
    Error("Unable to link on root directory ");
    return -EPERM;  // operation not permitted
  }

  memset(link, 0, size);
  int ret = 0;
  try {
    auto res = GetFile(path, false);
    auto node = std::get<0>(res).lock();
    string path_ = std::get<2>(res);

    // Check whether path exists
    if (!(node && *node)) {
      ret = -ENOLINK;  // Link has been severed
      throw QSException("No such file " + FormatArg(path_));
    }

    // Check whether it is a symlink
    if (!node->IsSymLink()) {
      ret = -EINVAL;  // invalid argument
      throw QSException("Not a symlink " + FormatArg(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read permission " + FormatArg(path_));
    }

    // Read the link
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
  }
  return ret;
}

// --------------------------------------------------------------------------
// Create a file node
//
// This is called for creation of all non-directory, non-symlink nodes.
// If the filesystem defines a crete() method, then for regular files that
// will be called instead.
int qsfs_mknod(const char* path, mode_t mode, dev_t dev) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to create root directory ");
    return -EPERM;  // operation not permitted
  }

  // Check whether filename is too long
  string filename = GetBaseName(path);
  if (filename.size() > GetNameMaxLen()) {
    Error("File name too long " + FormatArg(filename));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    CheckParentDir(path, W_OK | X_OK, &ret);

    // Check whether path exists
    auto res = drive.GetNode(path, false);
    auto node = res.first.lock();
    if (node && *node) {
      ret = -EEXIST;  // File exist
      throw QSException("File already exists " + FormatArg(path));
    }

    // Create the new node
    drive.MakeFile(path, mode | GetDefineFileMode(), dev);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
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
    Error("Unable to create root directory ");
    return -EPERM;  // operation not permitted
  }
  // Check whether filename is too long
  string filename = GetBaseName(path);
  if (filename.size() > GetNameMaxLen()) {
    Error("File name too long" + FormatArg(filename));
    return -ENAMETOOLONG;  // file name too long
  }
  // Check whether the pathname is too long
  if (strlen(path) > GetPathMaxLen()) {
    Error("Path name too long " + FormatArg(path));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    CheckParentDir(path, W_OK | X_OK, &ret);

    // Check whether a file or a directory with same path exists
    auto res = GetFile(path, true);
    auto node = std::get<0>(res).lock();
    string path_ = std::get<2>(res);
    if (node) {
      ret = -EEXIST;  // File exist
      throw QSException("File already exists " + FormatArg(path_));
    }

    // Create the directory
    drive.MakeDir(path_, mode | S_IFDIR);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
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
    Error("Unable to remove root directory ");
    return -EPERM;  // operation not permitted
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    CheckParentDir(path, W_OK, &ret);

    // Check whether the file exists
    auto node = drive.GetNode(path, false).first.lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file " + FormatArg(path));
    }

    // Remove the file
    if (node->IsDirectory()) {
      ret = -EINVAL;  // invalid argument
      throw QSException("Not a file, but a directory " + FormatArg(path));
    } else {
      drive.DeleteFile(path);
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
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
    Error("Unable to remove root directory ");
    return -EPERM;  // operation not permitted
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    CheckParentDir(path, W_OK, &ret);

    // Check whether the directory exists
    string path_ = AppendPathDelim(path);
    auto node = drive.GetNode(path_, true).first.lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such directory " + FormatArg(path_));
    }
    if (!node->IsDirectory()) {
      ret = -EINVAL;  // invalid argument
      throw QSException("Not a directory " + FormatArg(path_));
    }

    // Check whether the directory empty
    if (!node->IsEmpty()) {
      ret = -ENOTEMPTY;  // directory not empty
      throw QSException("Unable to remove, directory is not empty " +
                        FormatArg(path_));
    }

    // Check access permission
    if ((S_ISVTX & node->GetFileMode()) &&
        (GetFuseContextUID() != 0 && GetFuseContextUID() != node->GetUID())) {
      ret = -EACCES;  // permission denied
      throw QSException(
          "sticky bit set: only the owner/root user can delete the "
          "directory [user=" +
          to_string(GetFuseContextUID()) +
          ",owner=" + to_string(node->GetUID()) + "] " + FormatArg(path_));
    }

    // Do delete directory
    drive.DeleteDir(path_);  // delete directory

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
  }
  return ret;
}

// --------------------------------------------------------------------------
// Create a symbolic link
int qsfs_symlink(const char* path, const char* link) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (!IsValidPath(link)) {
    Error("Null link parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to symlink root directory ");
    return -EPERM;  // operation not permitted
  }
  string filename = GetBaseName(link);
  if (filename.empty()) {
    Error("Invalid link parameter " + FormatArg(link));
    return -EINVAL;
  } else if (filename.size() > GetNameMaxLen()) {
    Error("filename too long " + FormatArg(filename));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check link parent directory
    CheckParentDir(link, W_OK | X_OK, &ret);

    // Check whether file with the link name already exists
    auto res = GetFile(link, true);
    auto node = std::get<0>(res).lock();
    string link_ = std::get<2>(res);
    if (node) {
      ret = -EEXIST;  // File exist
      throw QSException("File already exists " + FormatArg(link_));
    }

    // Create a symbolic link
    drive.SymLink(path, link_);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
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
    Error("Unable to rename on root directory ");
    return -EPERM;  // operation not permitted
  }
  string newPathBaseName = GetBaseName(newpath);
  if (newPathBaseName.empty()) {
    Error("Invalid new file path " + FormatArg(newpath));
    return -EINVAL;
  } else if (newPathBaseName.size() > GetNameMaxLen()) {
    Error("New file name too long " + FormatArg(newPathBaseName));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  try {
    // Check whether the file exists
    auto res = GetFile(path, false);
    auto node = std::get<0>(res).lock();
    string path_ = std::get<2>(res);
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatArg(path_));
    }

    // Check access permission
    if ((S_ISVTX & node->GetFileMode()) &&
        (GetFuseContextUID() != 0 && GetFuseContextUID() != node->GetUID())) {
      ret = -EACCES;  // permission denied
      throw QSException(
          "sticky bit set: only the owner/root user can rename the file "
          "[user=" +
          to_string(GetFuseContextUID()) +
          ",owner=" + to_string(node->GetUID()) + "] " + FormatArg(path_));
    }

    // Delete newpath if it exists and it's not a nonempty directory
    auto nRes = GetFile(newpath, true);
    auto nNode = std::get<0>(nRes).lock();
    string newpath_ = std::get<2>(nRes);
    if (nNode) {
      if (nNode->IsDirectory() && !nNode->IsEmpty()) {
        ret = -ENOTEMPTY;  // directory not empty
        throw QSException(
            "Unable to rename. New file name is an existing directory and is "
            "not empty " +
            FormatArg(newpath_));
      } else {
        // Delete the file or empty directory with new file name
        Warning("New file name is existing. Replacing it " +
                FormatArg(newpath_));
        Drive::Instance().DeleteFile(newpath_);
      }
    }

    // Do Renaming
    Drive::Instance().RenameFile(path_, newpath_);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
  }
  return ret;
}

// --------------------------------------------------------------------------
// Create a hard link to a file
int qsfs_link(const char* path, const char* linkpath) {
  if (!IsValidPath(path) || !IsValidPath(linkpath)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path) || IsRootDirectory(linkpath)) {
    Error("Unable to link on root directory ");
    return -EPERM;  // operation not permitted
  }
  string linkPathBaseName = GetBaseName(linkpath);
  if (linkPathBaseName.empty()) {
    Error("Invalid link file path " + FormatArg(linkpath));
    return -EINVAL;
  } else if (linkPathBaseName.size() > GetNameMaxLen()) {
    Error("file name too long " + FormatArg(linkPathBaseName));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check if file exists
    auto node = drive.GetNode(path, true).first.lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatArg(path));
    }

    // Check if it is a directory
    if (node->IsDirectory()) {
      ret = -EPERM;  // operation not permitted
      throw QSException("Unable to hard link to a directory " +
                        FormatArg(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(),
                          (W_OK | R_OK))) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read and write permission for path " +
                        FormatArg(path));
    }

    // Check the directory where link is to be created
    CheckParentDir(linkpath, W_OK | X_OK, &ret);

    // Check if linkpath existing
    auto lnkRes = GetFile(linkpath, true);
    auto lnkNode = std::get<0>(lnkRes).lock();
    string linkpath_ = std::get<2>(lnkRes);
    if (lnkNode && *lnkNode) {
      ret = -EEXIST;
      throw QSException("File already exists for link path " +
                        FormatArg(linkpath_));
    }

    // Create hard link
    drive.Link(path, linkpath_);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
  }
  return ret;
}

// --------------------------------------------------------------------------
// Change the permission bits of a file
int qsfs_chmod(const char* path, mode_t mode) {
  DebugInfo("Trying to change permisions to " + QS::Utils::ModeToString(mode) +
            " for path" + FormatArg(path));
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to chmod on root directory ");
    return -EPERM;  // operation not permitted
  }
  // Check whether the pathname is too long
  if (strlen(path) > GetPathMaxLen()) {
    Error("Path name too long " + FormatArg(path));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check if file exists
    auto res = GetFile(path, true);
    auto node = std::get<0>(res).lock();
    string path_ = std::get<2>(res);
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatArg(path));
    }

    // Check access permission
    if (GetFuseContextUID() != 0 && GetFuseContextUID() != node->GetUID()) {
      ret = -EPERM;  // operation not permitted
      throw QSException("Only owner/root can change file permissions [user=" +
                        to_string(GetFuseContextUID()) + ",owner=" +
                        to_string(node->GetUID()) + "] " + FormatArg(path_));
    }

    // Do changing the file permission
    drive.Chmod(path_, mode);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
  }
  return ret;
}

// --------------------------------------------------------------------------
// Change the owner and group of a file
int qsfs_chown(const char* path, uid_t uid, gid_t gid) {
  DebugInfo("Trying to change owner and group to [uid=" + to_string(uid) +
            ", gid=" + to_string(gid) + "]" + FormatArg(path));
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (IsRootDirectory(path)) {
    Error("Unable to chown on root directory ");
    return -EPERM;  // operation not permitted
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check if file exists
    auto res = GetFile(path, true);
    auto node = std::get<0>(res).lock();
    string path_ = std::get<2>(res);
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatArg(path));
    }

    // Check access permission
    if (GetFuseContextUID() != 0 && GetFuseContextUID() != node->GetUID()) {
      ret = -EPERM;  // operation not permitted
      throw QSException(
          "Only owner/root can change file owner and group [user=" +
          to_string(GetFuseContextUID()) +
          ",owner=" + to_string(node->GetUID()) + "] " + FormatArg(path_));
    }

    // Do changing owner and group
    drive.Chown(path_, uid, gid);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
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
    Error("Invalid new size parameter " + FormatArg(to_string(newsize)));
    return -EINVAL;
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check if file exists
    auto node = drive.GetNode(path, true).first.lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatArg(path));
    }

    // Check if it is a directory
    if (node->IsDirectory()) {
      ret = -EPERM;  // operation not permitted
      throw QSException("Unable to truncate a directory " + FormatArg(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), W_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No write permission for path " + FormatArg(path));
    }

    // Do truncating
    drive.TruncateFile(path, newsize);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
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
    // Check parent directory
    string dirName = GetDirName(path);
    auto res = drive.GetNode(dirName, true);
    auto parent = res.first.lock();
    if (!(parent && *parent)) {
      ret = -EINVAL;  // invalid argument
      throw QSException("No parent directory " + FormatArg(path));
    }

    // Check if file exists
    auto node = drive.GetNode(path, false).first.lock();
    if (node && *node) {
      // Check if it is a directory
      if (node->IsDirectory()) {
        ret = -EPERM;  // operation not permitted
        throw QSException("Not a file, but a directory " + FormatArg(path));
      }

      // Check access permission
      if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
        ret = -EACCES;  // Permission denied
        throw QSException("No read permission for path " + FormatArg(path));
      }
    } else {
      // Create a empty file
      drive.MakeFile(path, GetDefineFileMode());
    }

    // Do Open
    drive.OpenFile(path);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
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
int qsfs_read(const char* path, char* buf, size_t size, off_t offset,
              struct fuse_file_info* fi) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (buf == nullptr) {
    Error("Null buf parameter from fuse");
    return -EINVAL;
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check if file exists
    auto node = drive.GetNode(path, false).first.lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file " + FormatArg(path));
    }

    // Check if it is a directory
    if (node->IsDirectory()) {
      ret = -EPERM;  // operation not permitted
      throw QSException("Not a file, but a directory " + FormatArg(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read permission for path " + FormatArg(path));
    }

    // Do Read
    try {
      drive.ReadFile(path, buf, size, offset);
    } catch (const QSException& err) {
      ret = -EAGAIN;  // try again
      throw;          // rethrow
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
  }
  return ret;
}

// --------------------------------------------------------------------------
// Write data to an open file
//
// Write should return exactly the number of bytes requested except on error. An
// exception to this is when the 'direct_io' mount option is specified (see read
// operation).
int qsfs_write(const char* path, const char* buf, size_t size, off_t offset,
               struct fuse_file_info* fi) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (buf == nullptr) {
    Error("Null buf parameter from fuse");
    return -EINVAL;
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check if file exists
    auto node = drive.GetNode(path, false).first.lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file " + FormatArg(path));
    }

    // Check if it is a directory
    if (node->IsDirectory()) {
      ret = -EPERM;  // operation not permitted
      throw QSException("Not a file, but a directory " + FormatArg(path));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), W_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No write permission for path " + FormatArg(path));
    }

    // Do Write
    try {
      drive.WriteFile(path, buf, size, offset);
    } catch (const QSException& err) {
      ret = -EAGAIN;  // try again
      throw;          // rethrow
    }

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
  }
  return ret;
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
    auto res = GetFile(path, true);
    auto node = std::get<0>(res).lock();
    if (node && *node) {
      // Set qsfs parameters
      struct statvfs stfs = Drive::Instance().GetFilesystemStatistics();
      FillStatvfs(stfs, statv);
    } else {          // node not existing
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatArg(path));
    }
  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
  }
  return ret;
}

// --------------------------------------------------------------------------
// Possibly flush cahced data
//
// NOT equivalent to fsync().
//
// Flush is called on each close() of a file descriptor. So if a filesystem
// wants to return write errors in close() and the file ahs cached dirty data,
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
    // Check whether path existing
    auto res = GetFile(path, false);
    auto node = std::get<0>(res).lock();
    string path_ = std::get<2>(res);
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such file or directory " + FormatArg(path_));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read permission " + FormatArg(path_));
    }

    // Write the file to object storage
    if (node->IsNeedUpload()) {
      try {
        Drive::Instance().UploadFile(path_);
        // TODO(jim):
        // set needUpload = false;
        // fileOpen = false;
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
// be
// passed to readdir, closedir and fsyncdir.
int qsfs_opendir(const char* path, struct fuse_file_info* fi) {
  // Currently no implementation.
  return 0;
}

// --------------------------------------------------------------------------
// Read directory.
//
// Ignores the offset parameter, and passes zero to the filler function's
// offset. The filler function will not return '1' (unless an error happens),
// so the whole directory is read in a single readdir operation.
int qsfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info* fi) {
  if (!IsValidPath(path)) {
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }
  if (buf == nullptr) {
    Error("Null buffer parameter from fuse");
    return -EINVAL;
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  auto dirPath = AppendPathDelim(path);
  try {
    // Check if file exists
    auto res = drive.GetNode(dirPath);
    auto node = res.first.lock();
    if (!(node && *node)) {
      ret = -ENOENT;  // No such file or directory
      throw QSException("No such directory " + FormatArg(path));
    }

    // Check if file is dir
    if (!node->IsDirectory()) {
      ret = -ENOTDIR;  // Not a directory
      throw QSException("Not a directory " + FormatArg(dirPath));
    }

    // Check access permission
    if (!node->FileAccess(GetFuseContextUID(), GetFuseContextGID(), R_OK)) {
      ret = -EACCES;  // Permission denied
      throw QSException("No read permission " + FormatArg(dirPath));
    }

    // Put the . and .. entries in the filler
    if (filler(buf, ".", NULL, 0) == 1 || filler(buf, "..", NULL, 0) == 1) {
      ret = -ENOMEM;  // out of memeory
      throw QSException("Fuse filler is full! dir: " + dirPath);
    }

    // Put the children into filler
    auto range = drive.GetChildren(dirPath);
    for (auto it = range.first; it != range.second; ++it) {
      if (auto child = it->second.lock()) {
        auto filename = child->MyBaseName();
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
  Info("Connecting qsfs...") return NULL;
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
  // TODO(jim):
  return 0;
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
    Error("Unable to create root directory ");
    return -EPERM;  // operation not permitted
  }
  // Check whether filename is too long
  string filename = GetBaseName(path);
  if (filename.size() > GetNameMaxLen()) {
    Error("File name too long " + FormatArg(filename));
    return -ENAMETOOLONG;  // file name too long
  }

  int ret = 0;
  auto& drive = Drive::Instance();
  try {
    // Check parent directory
    CheckParentDir(path, W_OK | X_OK, &ret);

    // Check whether path exists
    auto res = drive.GetNode(path, false);
    auto node = res.first.lock();
    if (node) {
      ret = -EEXIST;  // File exist
      throw QSException("File already exists " + FormatArg(path));
    }

    // Create the new node
    drive.MakeFile(path, mode);

  } catch (const QSException& err) {
    Error(err.get());
    if (ret == 0) {  // catch exception from lower level
      ret = -errno;
    }
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
  // TODO(jim):
  return 0;
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
