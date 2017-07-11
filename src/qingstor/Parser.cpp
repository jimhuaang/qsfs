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

#include "qingstor/Options.h"

#include <getopt.h>

#include <iostream>
#include <string>
#include <utility>

#include <fuse_opt.h>

#include "base/Exception.h"

namespace QS {

namespace QingStor {

using QS::Exception::QSException;
using std::string;

namespace Parser {

namespace {

// Parse optarg string with form of "opt=arg".
std::pair<string, string> GetOptionValuePair(const char *str) {
  string option;
  string value;
  // TODO
  return {option, value};
}

static const struct option longOpts[] = {
    {"mount_point", required_argument, NULL, 'm'},
    {"option", required_argument, NULL, 'o'},
    {"log_dir", required_argument, NULL, 'l'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {"debug", no_argument, NULL, 'd'},
    {"foregroud", no_argument, NULL, 'f'},
    {0, 0, 0, 0}

};

static const char *optStr = ":mol:hv:df";

int MyFuseOptionProcess(void *data, const char *arg, int key,
                        struct fuse_args *outargs) {
  int ret = 0;

  // TODO: consider adding reference to fuse_args in Options
  return ret;
}

}  // namespace

void Parse(int argc, char **argv) {
  // TODO(jim)
  auto options = QS::QingStor::Options::Instance();
  auto &map = options.GetMap();

  while (int arg = getopt_long(argc, argv, optStr, longOpts, NULL) != -1) {
    switch (arg) {
      case 'm':
        options.SetMountPoint(optarg);
        break;
      case 'o':
        map.emplace(GetOptionValuePair(optarg));
        break;
      case 'l':
        options.SetLogDirectory(optarg);
        break;
      case 'h':
        // print help message
        break;
      case 'v':
        // print version message
        break;
      case 'd':
        // enable debug macro on
        break;
      case 'f':
        // pass to fuse ???
        // set options.foreground
        break;
      case '?':
        // As getopt_long already printed an error message, do nothing.
        break;
      case ':':
        // An option with a missing argument, do nothing.
        break;
      default:
        std::cerr << "Invalid command line argument" << std::endl;
        break;
    }
  }  // end of while

  struct fuse_args customArgs = FUSE_ARGS_INIT(argc, argv);
  if (0 != fuse_opt_parse(&customArgs, NULL, NULL, MyFuseOptionProcess)) {
    throw QSException("Error while parsing command line optons.");
  }
}

}  // namespace Parser
}  // namespace QingStor
}  // namespace QS
