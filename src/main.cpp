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
using std::string;

int main(int argc, char **argv) {
  int ret = 0;

  // Parse command line arguments.
  QS::QingStor::Parser::Parse(argc, argv);


  // read configure file
  // and read it from options
  // Set configures. maybe do not need.

  bool noMount = false;
  // get these options

  auto errorHandle = [&ret](const char *err) {
    std::cerr << "\n[ERROR] " << err << "\n\n";
    ret = 1;
  };

  try {
    if (noMount) {
      QS::QingStor::Mounter::Instance().MountLite(
          QS::QingStor::Options::Instance());
      // show help
      // show version
    } else {
      const auto &options = QS::QingStor::Options::Instance();
      auto &mounter = QS::QingStor::Mounter::Instance();
      if (options.GetMountPoint().empty()) {
        throw "Missing mount point parameter. Please provide mount directory.";
      } else if (!mounter.IsMountable(options.GetMountPoint())) {
        throw "Unable to mount the directory " + options.GetMountPoint();
      }

      // Check and create configure directory.
      if (!CreateDirectoryIfNotExistsNoLog(GetConfigureDirectory())) {
        throw "QingStor File System congfigure directory " +
            GetConfigureDirectory() + " does not exist.";
      }

      // Check if credentials file exists.
      if (!FileExists(GetCredentialsFile())) {
        throw "QingStor File System credentials file " + GetCredentialsFile() +
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
