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

#ifndef _QSFS_FUSE_INCLUDED_QINGSTOR_OPERATIONS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_QINGSTOR_OPERATIONS_H_  // NOLINT

#define FUSE_USE_VERSION 26

#include <sys/stat.h>
#include <fuse.h>

namespace QS {

namespace QingStor {

void InitializeFUSECallbacks(struct fuse_operations & fuseOps);

int qsfs_getattr(const char * path, struct stat * statbuf);
int qsfs_readlink(const char * path, char * link, size_t size);
int qsfs_mknod(const char* path, mode_t mode, dev_t dev);
int qsfs_mkdir(const char* path, mode_t mode);
int qsfs_unlink(const char* path);
int qsfs_rmdir(const char* path);
int qsfs_symlink(const char* path, const char* link);
int qsfs_rename(const char* path, const char* newpath);
int qsfs_link(const char* path, const char* newpath);
int qsfs_chmod(const char* path, mode_t mode);
int qsfs_chown(const char* path, uid_t uid, gid_t gid);
int qsfs_truncate(const char* path, off_t newsize);
int qsfs_open(const char* path, struct fuse_file_info* fi);
int qsfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
int qsfs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
int qsfs_statfs(const char* path, struct statvfs* statv);
int qsfs_flush(const char* path, struct fuse_file_info* fi);
int qsfs_release(const char* path, struct fuse_file_info* fi);
int qsfs_fsync(const char* path, int datasync, struct fuse_file_info* fi);
int qsfs_setxattr(const char* path, const char* name, const char* value, size_t size, int flags);
int qsfs_getxattr(const char* path, const char* name, char* value, size_t size);
int qsfs_listxattr(const char* path, char* list, size_t size);
int qsfs_removexattr(const char* path, const char* name);
int qsfs_opendir(const char* path, struct fuse_file_info* fi);
int qsfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
int qsfs_releasedir(const char* path, struct fuse_file_info* fi);
int qsfs_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi);
void* qsfs_init(struct fuse_conn_info* conn);
void qsfs_destroy(void* userdata);
int qsfs_access(const char* path, int mask);
int qsfs_create(const char* path, mode_t mode, struct fuse_file_info* fi);
int qsfs_ftruncate(const char* path, off_t offset, struct fuse_file_info* fi);
int qsfs_fgetattr(const char* path, struct stat* statbuf, struct fuse_file_info* fi);
int qsfs_lock(const char* path, struct fuse_file_info* fi, int cmd, struct flock* lock);
int qsfs_utimens(const char * path, const struct timespec tv[2]);
int qsfs_write_buf(const char * path, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *);
int qsfs_read_buf(const char* path, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info* fi);
int qsfs_fallocate(const char* path, int, off_t offseta, off_t offsetb, struct fuse_file_info* fi);

}  // namespace QingStor
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_QINGSTOR_OPERATIONS_H_