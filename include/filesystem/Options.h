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

#ifndef _QSFS_FUSE_INCLUDED_FILESYSTEM_OPTIONS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_FILESYSTEM_OPTIONS_H_  // NOLINT

#include <stdint.h>  // for uint16_t

#include <iostream>
#include <string>

#include "base/LogLevel.h"
#include "filesystem/IncludeFuse.h"  // for fuse.h
#include "filesystem/Mounter.h"      // for friend function Mounter::Mount
#include "filesystem/Parser.h"       // for friend function Parser::Parse

namespace QS {

namespace FileSystem {

using QS::Logging::LogLevel;

class Options {
 public:
  Options(Options &&) = delete;
  Options(const Options &) = delete;
  Options &operator=(Options &&) = delete;
  Options &operator=(const Options &) = delete;
  ~Options() { fuse_opt_free_args(&m_fuseArgs); }

 public:
  static Options &Instance();
  bool IsNoMount() const { return m_showHelp || m_showVersion; }

  // accessor
  const std::string &GetBucket() const { return m_bucket; }
  const std::string &GetMountPoint() const { return m_mountPoint; }
  const std::string &GetZone() const { return m_zone; }
  const std::string &GetCredentialsFile() const { return m_credentialsFile; }
  const std::string &GetLogDirectory() const { return m_logDirectory; }
  LogLevel GetLogLevel() const { return m_logLevel; }
  uint16_t GetRetries() const { return m_retries; }
  const std::string &GetHost() const { return m_host; }
  const std::string &GetProtocol() const { return m_protocol; }
  uint16_t GetPort() const { return m_port; }
  const std::string GetAdditionalAgent() const { return m_additionalAgent; }
  bool IsClearLogDir() const { return m_clearLogDir; }
  bool IsForeground() const { return m_foreground; }
  bool IsSingleThread() const { return m_singleThread; }
  bool IsDebug() const { return m_debug; }
  bool IsShowHelp() const { return m_showHelp; }
  bool IsShowVersion() const { return m_showVersion; }

 private:
  Options() = default;

  struct fuse_args &GetFuseArgs() {
    return m_fuseArgs;
  }

  // mutator
  void SetBucket(const char *bucket) { m_bucket = bucket; }
  void SetMountPoint(const char *path) { m_mountPoint = path; }
  void SetZone(const char *zone) { m_zone = zone; }
  void SetCredentialsFile(const char *file) { m_credentialsFile = file; }
  void SetLogDirectory(const std::string &path) { m_logDirectory = path; }
  void SetLogLevel(LogLevel level) { m_logLevel = level; }
  void SetRetries(unsigned retries) { m_retries = retries; }
  void SetHost(const char *host) { m_host = host; }
  void SetProtocol(const char *protocol) { m_protocol = protocol; }
  void SetPort(unsigned port) { m_port = port; }
  void SetAdditionalAgent(const char *agent) { m_additionalAgent = agent; }
  void SetClearLogDir(bool clearLogDir) { m_clearLogDir = clearLogDir; }
  void SetForeground(bool foreground) { m_foreground = foreground; }
  void SetSingleThread(bool singleThread) { m_singleThread = singleThread; }
  void SetDebug(bool debug) { m_debug = debug; }
  void SetShowHelp(bool showHelp) { m_showHelp = showHelp; }
  void setShowVerion(bool showVersion) { m_showVersion = showVersion; }
  void SetFuseArgs(int argc, char **argv) {
    m_fuseArgs = FUSE_ARGS_INIT(argc, argv);
  }

  std::string m_bucket;
  std::string m_mountPoint;
  std::string m_zone;
  std::string m_credentialsFile;
  std::string m_logDirectory;
  LogLevel m_logLevel;
  uint16_t m_retries;
  std::string m_host;
  std::string m_protocol;
  uint16_t m_port;
  std::string m_additionalAgent;
  bool m_clearLogDir;
  bool m_foreground;    // FUSE foreground option
  bool m_singleThread;  // FUSE single threaded opton
  bool m_debug;
  bool m_showHelp;
  bool m_showVersion;
  struct fuse_args m_fuseArgs;

  friend void QS::FileSystem::Parser::Parse(int argc, char **argv);
  friend bool Mounter::DoMount(const Options &options, bool logOn,
                               void *userData) const;
  friend std::ostream &operator<<(std::ostream &os, const Options &opts);
};

std::ostream &operator<<(std::ostream &os, const Options &opts);
}  // namespace FileSystem
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_FILESYSTEM_OPTIONS_H_
