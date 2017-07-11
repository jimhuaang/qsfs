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

#ifndef _QSFS_FUSE_INCLUDED_QINGSTOR_OPTIONS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_QINGSTOR_OPTIONS_H_  // NOLINT

#include <memory>
#include <unordered_map>

#include "qingstor/Parser.h"

namespace QS {

namespace QingStor {

using OptionToValueMap = std::unordered_map<std::string, std::string>;

class Options {
 public:
  /*  Options(int count, char **values, const std::string &mountPoint,
            const std::string &confDir)
        : m_count(count),
          m_values(values),
          m_mountPoint(mountPoint),
          m_logDirectory(confDir) {}
  */
  Options(Options &&) = delete;
  Options(const Options &) = delete;
  Options &operator=(Options &&) = delete;
  Options &operator=(const Options &) = delete;
  ~Options() = default;

 public:
  static Options &Instance();

  // accessor
  int GetCount() const { return m_count; }
  char **GetValues() const { return m_values; }
  const std::string &GetMountPoint() const { return m_mountPoint; }
  const std::string &GetLogDirectory() const { return m_logDirectory; }
  const OptionToValueMap &GetMap() const { return m_map; }

 private:
  Options() = default;

  OptionToValueMap &GetMap() { return m_map; }

  // mutator
  void SetCount(int count) { m_count = count; }
  void SetValues(char **values) { m_values = values; }
  void SetMountPoint(const std::string &path) { m_mountPoint = path; }
  void SetLogDirectory(const std::string &path) { m_logDirectory = path; }

  int m_count;
  char **m_values;
  std::string m_bucket;
  std::string m_mountPoint;
  std::string m_host;
  std::string m_zone;
  std::string m_logDirectory;
  OptionToValueMap m_map;

  friend void QS::QingStor::Parser::Parse(int argc, char **argv);
};

}  // namespace QingStor
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_QINGSTOR_OPTIONS_H_
