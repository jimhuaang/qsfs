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

#ifndef _QSFS_FUSE_INCLUDED_QINGSTOR_MOUNTER_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_QINGSTOR_MOUNTER_H_  // NOLINT

#include <string>

namespace QS {

namespace QingStor {

class Options;

// TODO(jim): design it as singleton
class Mounter {
  using Outcome = std::pair<bool, std::string>;
 public:
  Mounter(Mounter &&) = delete;
  Mounter(const Mounter &) = delete;
  Mounter &operator=(Mounter &&) = delete;
  Mounter &operator=(const Mounter &) = delete;
  ~Mounter() = default;

 public:
  static Mounter &Instance();
  Outcome IsMountable(const std::string &mountPoint) const;
  Outcome CheckEmpty(const std::string &mountPoint) const;
  bool IsMounted(const std::string &mountPoint) const;
  bool Mount(const Options &options) const ;
  void UnMount(const std::string &mountPoint) const;

 private:
  Mounter() = default;
};

}  // namespace QingStor
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_QINGSTOR_MOUNTER_H_
