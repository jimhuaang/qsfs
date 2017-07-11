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

#include "qingstor/Operations.h"

#include <string.h>

namespace QS {

namespace QingStor {

void InitializeFUSECallbacks(struct fuse_operations & fuseOps){
  memset(&fuseOps, 0, sizeof(fuseOps));  // clear input

  fuseOps.getattr     = qsfs_getattr;
  fuseOps.readlink    = qsfs_readlink;
  fuseOps.mknod       = qsfs_mknod;
  fuseOps.mkdir       = qsfs_mkdir;
  fuseOps.unlink      = qsfs_unlink;
  fuseOps.rmdir       = qsfs_rmdir;
  fuseOps.symlink     = qsfs_symlink;
  fuseOps.rename      = qsfs_rename;
  fuseOps.link        = qsfs_link;
  fuseOps.chmod       = qsfs_chmod;
  fuseOps.chown       = qsfs_chown;
  fuseOps.truncate    = qsfs_truncate;
  fuseOps.open        = qsfs_open;
  fuseOps.read        = qsfs_read;
  fuseOps.write       = qsfs_write;
  fuseOps.statfs      = qsfs_statfs;
  fuseOps.flush       = NULL;  //s3fs
  fuseOps.release     = qsfs_release;
  fuseOps.fsync       = NULL;  //s3fs
  fuseOps.setxattr    = NULL;  //s3fs
  fuseOps.getxattr    = NULL;  //s3fs
  fuseOps.listxattr   = NULL;  //s3fs
  fuseOps.removexattr = NULL;  //s3fs
  fuseOps.opendir     = NULL;  //s3fs
  fuseOps.readdir     = qsfs_readdir;
  fuseOps.releasedir  = NULL;
  fuseOps.fsyncdir    = NULL;
  fuseOps.init        = qsfs_init;
  fuseOps.destroy     = qsfs_destroy;
  fuseOps.access      = qsfs_access;
  fuseOps.create      = qsfs_create;
  fuseOps.ftruncate   = NULL;
  fuseOps.fgetattr    = NULL;
  fuseOps.lock        = NULL;
  fuseOps.utimens     = qsfs_utimens;
  fuseOps.write_buf   = NULL;
  fuseOps.read_buf    = NULL;
  fuseOps.fallocate   = NULL;
}


}  // namespace QingStor
}  // namespace QS
