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

#include <exception>
#include <string>

#include "base/Exception.h"
#include "filesystem/Configure.h"
#include "filesystem/HelpText.h"
#include "filesystem/Initializer.h"
#include "filesystem/Mounter.h"
#include "filesystem/Options.h"
#include "filesystem/Parser.h"

using QS::Exception::QSException;
using QS::FileSystem::Configure::GetProgramName;
using QS::FileSystem::HelpText::ShowQSFSHelp;
using QS::FileSystem::HelpText::ShowQSFSUsage;
using QS::FileSystem::HelpText::ShowQSFSVersion;
using QS::FileSystem::Initializer;
using std::string;

namespace {
void CheckBucketName();
void CheckMountPoint();

}  // namespace

int main(int argc, char **argv) {
  int ret = 0;
  auto errorHandle = [&ret](const char *err) {
    std::cerr << "[" << GetProgramName() << " ERROR] " << err << "\n";
    ret = 1;
  };

  // Parse command line arguments.
  try {
    QS::FileSystem::Parser::Parse(argc, argv);
  } catch (const QSException &err) {
    errorHandle(err.what());
    return ret;
  }

  const auto &options = QS::FileSystem::Options::Instance();
  auto &mounter = QS::FileSystem::Mounter::Instance();
  try {
    if (options.IsNoMount()) {
      if (options.IsShowVersion()) {
        ShowQSFSVersion();
      }
      if (options.IsShowHelp()) {
        ShowQSFSHelp();
      }
    } else {  // Mount qsfs
      CheckBucketName();
      CheckMountPoint();

      auto mountPoint = options.GetMountPoint();
      auto outcome = mounter.IsMountable(mountPoint, false);  // log off
      if (!outcome.first) {
        throw outcome.second;
      }

      // Notice: DO NOT use logging before initialization done.
      // Do initializations.
      Initializer::RunInitializers();

      // Mount the file system.
      try {
        ret = mounter.Mount(options, true);  // log on
      } catch (const QSException &err) {
        if (mounter.IsMounted(mountPoint, true)) {
          mounter.UnMount(mountPoint, true);
        }
        throw err.what();
      }
    }
  } catch (const QSException &err) {
    errorHandle(err.what());
  } catch (const char *err) {
    errorHandle(err);
  } catch (const string &err) {
    errorHandle(err.c_str());
  } catch (const std::exception &err) {
    errorHandle(err.what());
  }
  return ret;
}

namespace {

static const char *illegalChars = "/:\\;!@#$%^&*?|+=";

void CheckBucketName() {
  const auto &options = QS::FileSystem::Options::Instance();
  if (options.GetBucket().empty()) {
    ShowQSFSUsage();
    throw "Missing BUCKET parameter";
  } else {
    if (options.GetBucket().find_first_of(illegalChars) != string::npos) {
      throw "BUCKET " + options.GetBucket() +
          " -- bucket name contains an illegal character of " + illegalChars;
    }
  }
}

void CheckMountPoint() {
  const auto &options = QS::FileSystem::Options::Instance();
  if (options.GetMountPoint().empty()) {
    ShowQSFSUsage();
    throw "Missing MOUNTPOINT parameter. Please provide mount directory";
  }
}

}  // namespace
