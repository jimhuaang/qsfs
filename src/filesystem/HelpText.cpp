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

#include "filesystem/HelpText.h"

#include <iostream>
#include <string>

#include "configure/Default.h"
#include "configure/Version.h"
#include "data/Size.h"

namespace QS {

namespace FileSystem {

namespace HelpText {

using QS::Configure::Default::GetDefaultCredentialsFile;
using QS::Configure::Default::GetDefaultDiskCacheDirectory;
using QS::Configure::Default::GetDefaultLogDirectory;
using QS::Configure::Default::GetDefaultHostName;
using QS::Configure::Default::GetDefaultProtocolName;
using QS::Configure::Default::GetDefaultParallelTransfers;
using QS::Configure::Default::GetDefaultTransferBufSize;
using QS::Configure::Default::GetDefaultZone;
using QS::Configure::Default::GetMaxCacheSize;
using QS::Configure::Default::GetMaxListObjectsCount;
using QS::Configure::Default::GetMaxStatCount;
using QS::Configure::Default::GetTransactionDefaultTimeDuration;
using std::cout;
using std::endl;
using std::to_string;

void ShowQSFSVersion() {
  cout << "qsfs version: " << QS::Configure::Version::GetVersionString()
       << endl;
}

void ShowQSFSHelp() {
  cout <<
  "Mount a QingStor bucket as a file system.\n";
  ShowQSFSUsage();
  cout <<
  "\n"
  "  mounting\n"
  "    qsfs -b=<BUCKET> -m=<MOUNTPOINT> -c=<CREDENTIALS> [options]\n"
  "  unmounting\n"
  "    umount <MOUNTPOINT>  or  fusermount -u <MOUNTPOINT>\n"
  "\n"
  "qsfs Options:\n"
  "Mandatory argements to long options are mandatory for short options too.\n"
  "  -b, --bucket       Specify bucket name\n"
  "  -m, --mount        Specify mount point (path)\n"
  "  -c, --credentials  Specify credentials file, default is " << 
                          GetDefaultCredentialsFile() << "\n" <<
  "  -z, --zone         Zone or region, default is " << GetDefaultZone() << "\n"
  "  -l, --logdir       Specify log directory, default is " <<
                          GetDefaultLogDirectory() << "\n" <<
  "  -L, --loglevel     Min log level, message lower than this level don't logged;\n"
  "                     Specify one of following log level: INFO,WARN,ERROR,FATAL;\n"
  "                     INFO is set by default\n"
  "  -r, --retries      Number of times to retry a failed transaction\n"
  "  -R, --reqtimeout   Time(milliseconds) to wait before timing out a request which\n"
  "                     is not time-consuming such as head a file, make a file, etc.\n"
  "                     For these time-consuming requests, e.g. upload/download a file,\n"
  "                     it will be evaluated depended on this time value and file size;\n"
  "                     Default value is " << to_string(GetTransactionDefaultTimeDuration())
                                          << " milliseconds\n"
  "  -Z, --maxcache     Max in-memory cache size(MB) for files, default is "
                        << to_string(GetMaxCacheSize() / QS::Data::Size::MB1) << "MB\n"
  "  -D, --diskdir      Specify the directory to store file data when in-memory cache\n"
  "                     is not availabe, default is " << GetDefaultDiskCacheDirectory() << "\n"
  "  -t, --maxstat      Max count(K) of cached stat entrys, default is "
                        << to_string(GetMaxStatCount() / QS::Data::Size::K1) << "K\n"
  "  -e, --statexpire   Expire time(minutes) for stat entries, negative value will\n"
  "                     disable stat expire, default is no expire\n"
  "  -i, --maxlist      Max count of files of ls operation, negative value will list\n"
  "                     all files, default is " << to_string(GetMaxListObjectsCount()) <<"\n"
  "  -n, --numtransfer  Max number file tranfers to run in parallel, you can increase\n"
  "                     the value when transfer large files, default is "
                        << to_string(GetDefaultParallelTransfers()) << "\n"
  "  -u, --bufsize      File transfer buffer size(MB), this should be larger than 8MB,\n"
  "                     default is " 
                        << to_string(GetDefaultTransferBufSize() / QS::Data::Size::MB1) << "MB\n"
  "  -H, --host         Host name, default is " << GetDefaultHostName() << "\n" <<
  "  -p, --protocol     Protocol could be https or http, default is " <<
                                              GetDefaultProtocolName() << "\n" <<
  "  -P, --port         Specify port, default is 443 for https and 80 for http\n"
  "  -a, --agent        Additional user agent\n"
  "\n"
  "Miscellaneous Options:\n"
  "  -C, --clearlogdir  Clear log directory at beginning\n"
  "  -f, --forground    Turn on log to STDERR and enable FUSE foreground mode\n"
  "  -s, --single       Turn on FUSE single threaded option - disable multi-threaded\n"
  "  -S, --Single       Turn on qsfs single threaded option - disable multi-threaded\n"
  "  -d, --debug        Turn on debug messages to log and enable FUSE debug option\n"
  "  -U, --curldbg      Turn on debug message from libcurl\n"
  "  -h, --help         Print qsfs help\n"
  "  -V, --version      Print qsfs version\n"
  "\n"
  "FUSE Options:\n"
  "  -o opt[,opt...]\n"
  "  There are many FUSE specific mount options that can be specified,\n"
  "  e.g. nonempty, allow_other, etc. See the FUSE's README for the full set.\n";
  cout.flush();
}

void ShowQSFSUsage() { 
  cout << 
  "Usage: qsfs -b|--bucket=<name> -m|--mount=<mount point>\n"
  "       [-c|--credentials=[file path]] [-z|--zone=[value]]\n"
  "       [-l|--logdir=[dir]] [-L|--loglevel=[INFO|WARN|ERROR|FATAL]] \n"
  "       [-r|--retries=[value]] [-R|reqtimeout=[value]]\n"
  "       [-Z|--maxcache=[value]] [-D|--diskdir=[value]]\n"
  "       [-t|--maxstat=[value]] [-e|--statexpire=[value]]\n"
  "       [-i|--maxlist=[value]]\n"
  "       [-n|--numtransfer=[value]] [-u|--bufsize=value]]\n"
  "       [-H|--host=[value]] [-p|--protocol=[value]]\n"
  "       [-P|--port=[value]] [-a|--agent=[value]]\n"
  "       [-C|--clearlogdir] [-f|--foreground] \n"
  "       [-s|--single] [-S|--Single]\n"
  "       [-d|--debug] [-U|--curldbg]\n"
  "       [-h|--help] [-V|--version]\n"
  "       [FUSE options]\n";
  cout.flush();
}

}  // namespace HelpText
}  // namespace FileSystem
}  // namespace QS
