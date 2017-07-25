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

#include <assert.h>

#include <unistd.h>

#include <exception>
#include <iostream>

#include "base/Exception.h"
#include "client/Protocol.h"
#include "client/URI.h"
#include "client/Zone.h"
#include "filesystem/Configure.h"
#include "filesystem/Mounter.h"
#include "filesystem/Initializer.h"
#include "filesystem/Options.h"
#include "filesystem/Parser.h"

#include "qingstor-sdk-cpp/Bucket.h"
#include "qingstor-sdk-cpp/QingStor.h"
#include "qingstor-sdk-cpp/QsConfig.h"
#include "qingstor-sdk-cpp/QsErrors.h"

using QS::Exception::QSException;
using QS::FileSystem::Configure::GetProgramName;
using QS::FileSystem::Initializer;
using std::cout;
using std::endl;
using std::string;
using std::to_string;

namespace {

static const char *illegalChars = "/:\\;!@#$%^&*?|+=";

void ShowQSFSVersion();
void ShowQSFSHelp();
void ShowQSFSUsage();

}  // namespace


int main(int argc, char **argv) {
  int ret = 0;
  auto errorHandle = [&ret](const char *err) {
    std::cerr << "[" << GetProgramName() << " ERROR] " << err << "\n";
    ret = 1;
  };

  // Notice: Should NOT use log macros before initializing log.
  //
  // Parse command line arguments.
  try {
    QS::FileSystem::Parser::Parse(argc, argv);
  } catch (const QSException &err) {
    errorHandle(err.what());
    return ret;
  }

  const auto &options = QS::FileSystem::Options::Instance();
  // TODO(jim) : remove this line which is for test
  cout << options << endl << endl;

  // test sdk-cpp
  QingStor::QingStorService::initService("/home/jim/qsfs.test/qsfs.log/sdk.log");

  QingStor::QsConfig sdkQsConfig;
  sdkQsConfig.m_LogLevel = "info";
  sdkQsConfig.m_AccessKeyId = "DIRMZUZDFDCUEGWBMEUX";
  sdkQsConfig.m_SecretAccessKey = "wpYpihVOy5AWo9SjPZNMvFAuVPhycoRKu252rGjt";
  QingStor::QingStorService qsService(sdkQsConfig);

  QingStor::ListBucketsInput input;
  input.SetLocation("pek3a");
  QingStor::ListBucketsOutput output; 

  auto err = qsService.listBuckets(input, output);

  QingStor::Bucket bucket = qsService.GetBucket("jimbucket1", "pek3a");

  auto &mounter = QS::FileSystem::Mounter::Instance();
  try {
    if (options.IsNoMount()) {
      if (options.IsShowVersion()) {
        ShowQSFSVersion();
        // Signal FUSE to show additional version info.
        mounter.MountLite(options, false);  // log off
      }
      if (options.IsShowHelp()) {
        ShowQSFSHelp();
        // Do not signal FUSE to show additional help
        // as it does not work well with pipe such as less, more ,etc.
      }
    } else {  // Mount qsfs

      if (options.GetBucket().empty()) {
        ShowQSFSUsage();
        throw "Missing BUCKET parameter";
      } else {
        if (options.GetBucket().find_first_of(illegalChars) != string::npos) {
          throw "BUCKET " + options.GetBucket() +
              " -- bucket name cotains an illegal character of " + illegalChars;
        }
      }

      auto &mountPoint = options.GetMountPoint();
      if (mountPoint.empty()) {
        ShowQSFSUsage();
        throw "Missing MOUNTPOINT parameter. Please provide mount directory";
      } else {
        auto outcome = mounter.IsMountable(mountPoint, false);  // log off
        if (!outcome.first) {
          throw outcome.second;
        }

        // Do initialization.
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
      }  // end of if (mountPoint.empty())
    }    // end of if (options.IsNoMount())
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
static const char *mountingStr = "qsfs -b=<BUCKET> -m=<MOUNTPOINT> [options]";

void ShowQSFSVersion() {
  cout << "qsfs version: " << QS::FileSystem::Configure::GetQSFSVersion() << endl;
}

void ShowQSFSHelp() {
  using namespace QS::Client;    // NOLINT
  using namespace QS::Client::Http;  // NOLINT
  using namespace QS::FileSystem;  // NOLINT
  cout <<
  "\n"
  "Mount a QingStor bucket as a file system.\n"
  "Usage: qsfs -b|--bucket=<name> -m|--mount=<mount point>\n"
  "       [-z|--zone=[value]] [-c|--credentials=[file path]] [-l|--logdir=[dir]]\n"
  "       [-L|--loglevel=[INFO|WARN|ERROR|FATAL]] [-r|--retries=[value]]\n"
  "       [-H|--host=[value]] [-p|--protocol=[value]]\n"
  "       [-P|--port=[value]] [-a|--agent=[value]]\n"
  "       [-C|--clearlogdir] [-f|--foreground] [-s|--single] [-d|--debug]\n"
  "       [-h|--help] [-V|--version]\n"
  "       [FUSE options]\n"
  "\n"
  "  mounting\n"
  "    " << mountingStr << "\n" <<
  "  unmounting\n"
  "    umount <MOUNTPOINT>  or  fusermount -u <MOUNTPOINT>\n"
  "\n"
  "qsfs Options:\n"
  "Mandatory argements to long options are mandatory for short options too.\n"
  "  -b, --bucket       Specify bucket name\n"
  "  -m, --mount        Specify mount point (path)\n"
  "  -z, --zone         Zone or region, default is " << QS::Client::GetDefaultZone() << "\n"
  "  -c, --credentials  Specify credentials file, default is " << 
                                  Configure::GetDefaultCredentialsFile() << "\n" <<
  "  -l, --logdir       Specify log directory, default is " <<
                                  Configure::GetDefaultLogDirectory() << "\n" <<
  "  -L, --loglevel     Min log level, message lower than this level don't logged;\n"
  "                     Specify one of following log level: INFO,WARN,ERROR,FATAL;\n"
  "                     INFO is set by default\n"
  "  -r, --retries      Number of times to retry a failed transaction\n"
  "  -H, --host         Host name, default is " << Http::GetDefaultHostName() << "\n" <<
  "  -p, --protocol     Protocol could be https or http, default is " <<
                                              GetDefaultProtocolName() << "\n" <<
  "  -P, --port         Specify port, default is 443 for https, 80 for http\n"
  "  -a, --agent        Additional user agent\n"
  "\n"
  "Miscellaneous Options:\n"
  "  -C, --clearlogdir  Clear log directory at beginning\n"
  "  -f, --forground    Turns on log messages to STDERR and enable FUSE\n"
  "                     foreground option\n"
  "  -s, --single       FUSE single threaded option - disable multi-threaded\n"
  "  -d, --debug        Turn on debug messages to log and enable FUSE debug option\n"
  "  -h, --help         Print qsfs help and FUSE help\n"
  "  -V, --version      Print qsfs version and FUSE version\n"
  "\n"
  "FUSE Options:\n"
  "  -o opt[,opt...]\n"
  "  There are many FUSE specific mount options that can be specified,\n"
  "  e.g. nonempty, allow_other, etc. See the FUSE's README for the full set.\n";
}

void ShowQSFSUsage() { cout << "Usage: " << mountingStr << endl; }

}  // namespace
