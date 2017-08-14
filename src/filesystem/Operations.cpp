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
#include <string.h>  // for memset

#include <errno.h>
#include <sys/types.h>  // for uid_t

#include <memory>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "data/Directory.h"
#include "filesystem/Drive.h"

namespace {

bool IsValidPath(const char *path) {
  return path != nullptr && *path != 0;
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

}  // namespace


namespace QS {

namespace FileSystem {

using QS::Data::Node;
using QS::Exception::QSException;
using QS::FileSystem::Drive;
using std::string;
using std::weak_ptr;

void InitializeFUSECallbacks(struct fuse_operations *fuseOps) {
  memset(fuseOps, 0, sizeof(*fuseOps));  // clear input

  fuseOps->getattr     = qsfs_getattr;
  fuseOps->readlink    = qsfs_readlink;
  fuseOps->mknod       = qsfs_mknod;
  fuseOps->mkdir       = qsfs_mkdir;
  fuseOps->unlink      = qsfs_unlink;
  fuseOps->rmdir       = qsfs_rmdir;
  fuseOps->symlink     = qsfs_symlink;
  fuseOps->rename      = qsfs_rename;
  fuseOps->link        = qsfs_link;
  fuseOps->chmod       = qsfs_chmod;
  fuseOps->chown       = qsfs_chown;
  fuseOps->truncate    = qsfs_truncate;
  fuseOps->open        = qsfs_open;
  fuseOps->read        = qsfs_read;
  fuseOps->write       = qsfs_write;
  fuseOps->statfs      = qsfs_statfs;
  fuseOps->flush       = NULL;  // s3fs
  fuseOps->release     = qsfs_release;
  fuseOps->fsync       = NULL;  // s3fs
  fuseOps->setxattr    = NULL;  // s3fs
  fuseOps->getxattr    = NULL;  // s3fs
  fuseOps->listxattr   = NULL;  // s3fs
  fuseOps->removexattr = NULL;  // s3fs
  fuseOps->opendir     = NULL;  // s3fs
  fuseOps->readdir     = qsfs_readdir;
  fuseOps->releasedir  = NULL;
  fuseOps->fsyncdir    = NULL;
  fuseOps->init        = qsfs_init;
  fuseOps->destroy     = qsfs_destroy;
  fuseOps->access      = qsfs_access;
  fuseOps->create      = qsfs_create;
  fuseOps->ftruncate   = NULL;
  fuseOps->fgetattr    = NULL;
  fuseOps->lock        = NULL;
  fuseOps->utimens     = qsfs_utimens;
  fuseOps->write_buf   = NULL;
  fuseOps->read_buf    = NULL;
  fuseOps->fallocate   = NULL;
}

// --------------------------------------------------------------------------
int qsfs_getattr(const char* path, struct stat* statbuf) {
  if(!IsValidPath(path)){
    Error("Null path parameter from fuse");
    return -EINVAL;  // invalid argument
  }

  memset(statbuf, 0, sizeof(*statbuf));
  int ret = 0;
  auto &drive = Drive::Instance();
  try {
    auto node = drive.FindNode(path).lock();
    if(node){
      auto st = const_cast<const Node&>(*node).GetEntry().ToStat();
      FillStat(st, statbuf);
    } else {
      throw QSException("Fail to find path " + string(path));
    }
  } catch (const QSException& err) {
    ret = -errno;
    Error(err.get());
    return ret;
  }
  return ret;
}

// --------------------------------------------------------------------------
int qsfs_readlink(const char* path, char* link, size_t size) { 
  //
  return 0; 
}

// --------------------------------------------------------------------------
int qsfs_mknod(const char* path, mode_t mode, dev_t dev) {
  // put object
  return 0;
}

// --------------------------------------------------------------------------
int qsfs_mkdir(const char* path, mode_t mode) {
  // put object
  return 0;
}

// --------------------------------------------------------------------------
int qsfs_unlink(const char* path) {
  // delete object
  return 0;
}

// --------------------------------------------------------------------------
int qsfs_rmdir(const char* path) {
  // delete whole tree recursively
  return 0;
}

// --------------------------------------------------------------------------
int qsfs_symlink(const char* path, const char* link) { return 0; }

// --------------------------------------------------------------------------
int qsfs_rename(const char* path, const char* newpath) {
  // copy object
  return 0;
}

int qsfs_link(const char* path, const char* newpath) { return 0; }
int qsfs_chmod(const char* path, mode_t mode) { return 0; }
int qsfs_chown(const char* path, uid_t uid, gid_t gid) { return 0; }
int qsfs_truncate(const char* path, off_t newsize) { return 0; }

// --------------------------------------------------------------------------
int qsfs_open(const char* path, struct fuse_file_info* fi) { 
  // head oject, store file size for reading
  // if file not exist, create empty one
  // put object
  return 0; 
}

// --------------------------------------------------------------------------
int qsfs_read(const char* path, char* buf, size_t size, off_t offset,
              struct fuse_file_info* fi) {
  // get object
  return 0;
}

// --------------------------------------------------------------------------
int qsfs_write(const char* path, const char* buf, size_t size, off_t offset,
               struct fuse_file_info* fi) {
  // create a write buffer if necceaary
  // handle hole if file
  return 0;
}


int qsfs_statfs(const char* path, struct statvfs* statv) { return 0; }
int qsfs_flush(const char* path, struct fuse_file_info* fi) { return 0; }

// --------------------------------------------------------------------------
int qsfs_release(const char* path, struct fuse_file_info* fi) {
  // put object
  return 0;
}


int qsfs_fsync(const char* path, int datasync, struct fuse_file_info* fi) {
  return 0;
}
int qsfs_setxattr(const char* path, const char* name, const char* value,
                  size_t size, int flags) {
  return 0;
}
int qsfs_getxattr(const char* path, const char* name, char* value,
                  size_t size) {
  return 0;
}
int qsfs_listxattr(const char* path, char* list, size_t size) { return 0; }
int qsfs_removexattr(const char* path, const char* name) { return 0; }
int qsfs_opendir(const char* path, struct fuse_file_info* fi) { return 0; }
int qsfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info* fi) {
  return 0;
}
int qsfs_releasedir(const char* path, struct fuse_file_info* fi) { return 0; }
int qsfs_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi) {
  return 0;
}

// --------------------------------------------------------------------------
void* qsfs_init(struct fuse_conn_info* conn) {
  // Initialization and checking are done when mounting
  // just print info here
  Info("Connecting qsfs...")
  return NULL; 
}

// --------------------------------------------------------------------------
void qsfs_destroy(void* userdata) {
  Info("Disconnecting qsfs...");
}

int qsfs_access(const char* path, int mask) { return 0; }

// --------------------------------------------------------------------------
int qsfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
  // refer qsfs_open
  return 0;
}

// --------------------------------------------------------------------------
int qsfs_ftruncate(const char* path, off_t offset, struct fuse_file_info* fi) {
  // 
  return 0;
}


int qsfs_fgetattr(const char* path, struct stat* statbuf,
                  struct fuse_file_info* fi) {
  return 0;
}
int qsfs_lock(const char* path, struct fuse_file_info* fi, int cmd,
              struct flock* lock) {
  return 0;
}
int qsfs_utimens(const char* path, const struct timespec tv[2]) { return 0; }
int qsfs_write_buf(const char* path, struct fuse_bufvec* buf, off_t off,
                   struct fuse_file_info*) {
  return 0;
}
int qsfs_read_buf(const char* path, struct fuse_bufvec** bufp, size_t size,
                  off_t off, struct fuse_file_info* fi) {
  return 0;
}
int qsfs_fallocate(const char* path, int, off_t offseta, off_t offsetb,
                   struct fuse_file_info* fi) {
  return 0;
}

}  // namespace FileSystem
}  // namespace QS
