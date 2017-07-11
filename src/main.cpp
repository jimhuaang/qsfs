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

#include <unistd.h>

#include <iostream>
#include <memory>

#include "base/Exception.h"
#include "base/Logging.h"
#include "base/Utils.h"
#include "client/Protocol.h"
#include "client/Zone.h"
#include "qingstor/Configure.h"
#include "qingstor/Mounter.h"
#include "qingstor/Options.h"
#include "qingstor/Parser.h"

using QS::Exception::QSException;
using QS::Logging::DefaultLog;
using QS::Logging::Log;
using QS::Logging::InitializeLogging;
using QS::QingStor::Configure::GetConfigureDirectory;
using QS::QingStor::Configure::GetCredentialsFile;
using QS::QingStor::Configure::GetLogDirectory;
using QS::Utils::CreateDirectoryIfNotExistsNoLog;
using QS::Utils::FileExists;
using std::cout;
using std::endl;
using std::string;
using std::to_string;

namespace {
void ShowQSFSVersion();
void ShowQSFSHelp();
void ShowQSFSUsage();
}  // namespace

int main(int argc, char **argv) {
  int ret = 0;
  auto errorHandle = [&ret](const char *err) {
    std::cerr << "[qsfs ERROR] " << err << "\n";
    ret = 1;
  };

  // Parse command line arguments.
  QS::QingStor::Parser::Parse(argc, argv);

  const auto &options = QS::QingStor::Options::Instance();
  try {
    if (options.IsNoMount()) {
      if (options.IsShowVersion()) {
        ShowQSFSVersion();
      }
      if (options.IsShowHelp()) {
        ShowQSFSHelp();
      }
    } else {
      auto &mounter = QS::QingStor::Mounter::Instance();
      if (options.GetBucket().empty()) {
        ShowQSFSUsage();
        throw "Missing BUCKET parameter.";
      }
      if (options.GetMountPoint().empty()) {
        ShowQSFSUsage();
        throw "Missing MOUNTPOINT parameter. Please provide mount directory.";
      }
      if (!mounter.IsMountable(options.GetMountPoint())) {
        throw "Unable to mount the directory " + options.GetMountPoint();
      }

      // Check and create configure directory.
      if (!CreateDirectoryIfNotExistsNoLog(GetConfigureDirectory())) {
        throw "qsfs congfigure directory " + GetConfigureDirectory() +
            " does not exist.";
      }

      // Check if credentials file exists.
      if (!FileExists(GetCredentialsFile())) {
        throw "qsfs credentials file " + GetCredentialsFile() +
            " does not exist.";
      }

      // Initialize logging.
      // TODO(jim): if foreground initialize with ConsoleLog
      InitializeLogging(
          std::unique_ptr<Log>(new DefaultLog(GetLogDirectory())));

      // Mount the file system.
      mounter.Mount(options);
    }
  } catch (const QSException &err) {
    errorHandle(err.what());
  } catch (const char *err) {
    errorHandle(err);
  } catch (const string &err) {
    errorHandle(err.c_str());
  }

  return ret;
}

namespace {
void ShowQSFSVersion() {
  cout << "qsfs version: " << QS::QingStor::Configure::GetQSVersion() << endl;
}

void ShowQSFSHelp() {
  using namespace QS::Client;  // NOLINT
  using namespace QS::QingStor;  // NOLINT
  cout << 
  "Mount a QingStor bucket as a file system.\n"
  "Usage: qsfs -b|--bucket=<name> -m|--mount=<mount point>\n"
  "       [-z|--zone=<value>] [-h|--host=<value>] [-p|--protocol=<value>]\n"
  "       [-t|--port=<value>] [-r|--retries=<value>] [-a|--agent=<value>]\n"
  "       [-l|--logdir=<dir>] [-f|--foreground] [-s|--single] [-d|--debug]\n"
  "       [--help] [--version]\n"
  "\n"
  "  mounting\n"
  "    qsfs --b=<BUCKET> --m=<MOUNTPOINT> [options]\n"
  "  unmounting\n"
  "    umount <MOUNTPOINT>\n"
  "\n"
  "qsfs Options:\n"
  "Mandatory argements to long options are mandatory for short options too.\n"
  "  -b, --bucket     Specify bucket name\n"
  "  -m, --mount      Specify mount point (path)\n"
  "  -z, --zone       Zone or region, default is " << Zone::PEK_3A << "\n" <<
  "  -h, --host       Host name, default is " << Host::BASE << "\n" <<
  "  -p, --protocol   Protocol could be https or http, default is " <<
                                            Protocol::HTTPS << "\n" <<
  "  -t, --port       Specify port, default is 443 for https, 80 for http\n"
  "  -r, --retries    Number of times to retry a failed QingStor transation\n"
  "  -a, --agent      Additional user agent\n"
  "  -l, --logdir     Spcecify log directory, default is " <<
                                Configure::GetDefaultLogDirectory() << "\n" <<
  "\n"
  "Miscellaneous Options:\n"
  "  -f, --forground    FUSE foreground option - do not run as daemon\n"
  "  -s, --single       FUSE single threaded option - disable multi-threaded\n"
  "  -d, --debug        Turn on debug messages to log. Specifying -f turns on\n"
  "                     debug messages to STDOUT\n"
  "      --help         Display this help and exit\n"
  "      --version      Output version information and exit\n";
}

void ShowQSFSUsage() {
  cout << "Usage: qsfs --b=<BUCKET> --m=<MOUNTPOINT> [options]" << endl;
}
}  // namespace
