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

#include "configure/Options.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>  // NOLINT
#include <string>

#include "base/LogLevel.h"

namespace QS {

namespace Configure {

using QS::Logging::GetLogLevelName;
using std::ostream;
using std::string;
using std::to_string;

static std::unique_ptr<Options> instance(nullptr);
static std::once_flag flag;

Options &Options::Instance() {
  std::call_once(flag, [] { instance.reset(new Options); });
  return *instance.get();
}

ostream &operator<<(ostream &os, const Options &opts) {
  auto CatArgv = [](int argc, char **argv) -> string {
    string str;
    int i = 0;
    while (i != argc) {
      if (i != 0) { str.append(" "); }
      str.append(argv[i]);
      ++i;
    }
    return str;
  };
  auto &fuseArg = opts.m_fuseArgs;

  return os
         << "[bucket: " << opts.m_bucket << "] "
         << "[mount point: " << opts.m_mountPoint << "] "
         << "[zone: " << opts.m_zone << "] "
         << "[credentials: " << opts.m_credentialsFile << "] "
         << "[log directory: " << opts.m_logDirectory << "] "
         << "[log level: " << GetLogLevelName(opts.m_logLevel) << "] "
         << "[retries: " << to_string(opts.m_retries) << "] "
         << "[req timeout(ms): " << to_string(opts.m_requestTimeOut) << "] "
         << "[max cache(MB): " << to_string(opts.m_maxCacheSizeInMB) << "] "
         << "[max stat(K): " << to_string(opts.m_maxStatCountInK) << "] "
         << "[stat expire(min): " << to_string(opts.m_statExpireInMin) << "] "
         << "[num transfers: " << to_string(opts.m_parallelTransfers) << "] "
         << "[transfer buf(MB): " << to_string(opts.m_transferBufferSizeInMB) <<"] "
         << "[pool size: " << to_string(opts.m_clientPoolSize) << "] "
         << "[host: " << opts.m_host << "] "
         << "[protocol: " << opts.m_protocol << "] "
         << "[port: " << to_string(opts.m_port) << "] "
         << "[additional agent: " << opts.m_additionalAgent << "] "
         << "[clear logdir: " << std::boolalpha << opts.m_clearLogDir << "] "
         << "[foreground: " << opts.m_foreground << "] "
         << "[FUSE single thread: " << opts.m_singleThread << "] "
         << "[qsfs single thread: " << opts.m_qsfsSingleThread << "] "
         << "[debug: " << opts.m_debug << "] "
         << "[show help: " << opts.m_showHelp << "] "
         << "[show version: " << opts.m_showVersion << "] "
         << "[fuse_args.argc: " << to_string(fuseArg.argc) << "] "
         << "[fuse_args.argv: " << CatArgv(fuseArg.argc, fuseArg.argv) << "] "
         << "[fuse_args.allocated: " << static_cast<bool>(fuseArg.allocated)
         << "] " << std::noboolalpha;
}

}  // namespace Configure
}  // namespace QS
