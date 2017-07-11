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

#include <stddef.h>  // for offsetof
#include <string.h>  // for strdup

#include <fuse.h>

#include "base/Exception.h"
#include "client/Protocol.h"
#include "client/RetryStrategy.h"
#include "client/Zone.h"
#include "qingstor/Configure.h"

namespace QS {

namespace QingStor {

using QS::Exception::QSException;

namespace Parser {

namespace {
static struct options {
  // We can't set default values for the char* fields here
  // because fuse_opt_parse would attempt to free() them
  // when user specifies different values on command line.
  const char *bucket;
  const char *mountPoint;
  const char *zone;
  const char *host;
  const char *protocol;
  unsigned port = QS::Client::DefaultPort::HTTPS;
  unsigned retries = QS::Client::Retry::DefaultMaxRetries;
  const char *addtionalAgent;
  const char *logDirectory;
  int foreground = 0;
  int debug = 0;
  int showHelp = 0;
  int showVersion = 0;
} options;

#define OPTION(t, p) \
  { t, offsetof(struct options, p), 1 }

static const struct fuse_opt optionSpec[] = {
    OPTION("--b=%s", bucket),         OPTION("bucket=%s", bucket),
    OPTION("--m=%s", mountPoint),     OPTION("mount=%s", mountPoint),
    OPTION("--z=%s", zone),           OPTION("zone=%s", zone),
    OPTION("--h=%s", host),           OPTION("host=%s", host),
    OPTION("--p=%s", protocol),       OPTION("protocol=%s", protocol),
    OPTION("--t=%u", port),           OPTION("port=%u", port),
    OPTION("--r=%u", retries),        OPTION("retries=%u", retries),
    OPTION("--a=%s", addtionalAgent), OPTION("agent=%s", addtionalAgent),
    OPTION("--l=%s", logDirectory),   OPTION("logdir=%s", logDirectory),
    OPTION("-f", foreground),         OPTION("--foreground", foreground),
    OPTION("-d", debug),              OPTION("--debug", debug),
    OPTION("-h", showHelp),           OPTION("--help", showHelp),
    OPTION("-v", showVersion),        OPTION("--version", showVersion),
    FUSE_OPT_END
};

}  // namespace

void Parse(int argc, char **argv) {
  auto &qsOptions =QS::QingStor::Options::Instance();
  qsOptions.SetFuseArgs(argc, argv);

  // Set defaults. we have to use strdup so that fuse_opt_parse
  // can free the defaults if other values are specified.
  options.bucket         = strdup("");
  options.mountPoint     = strdup("");
  options.zone           = strdup(QS::Client::Zone::PEK_3A);
  options.host           = strdup(QS::Client::Host::BASE);
  options.protocol       = strdup(QS::Client::Protocol::HTTPS);
  options.addtionalAgent = strdup("");
  options.logDirectory   = strdup(
      QS::QingStor::Configure::GetDefaultLogDirectory().c_str());

  if (0 != fuse_opt_parse(&(qsOptions.GetFuseArgs()), 
                          &options, optionSpec, NULL)) {
    throw QSException("Error while parsing command line optons.");
  }

  qsOptions.SetBucket(options.bucket);
  qsOptions.SetMountPoint(options.mountPoint);
  qsOptions.SetZone(options.zone);
  qsOptions.SetHost(options.host);
  qsOptions.SetProtocol(options.protocol);
  qsOptions.SetPort(options.port);
  qsOptions.SetRetries(options.retries);
  qsOptions.SetAdditionalAgent(options.addtionalAgent);
  qsOptions.SetLogDirectory(options.logDirectory);
  qsOptions.SetForeground(options.foreground != 0);
  qsOptions.SetDebug(options.debug != 0);
  qsOptions.SetShowHelp(options.showHelp != 0);
  qsOptions.setShowVerion(options.showVersion !=0);
}

}  // namespace Parser
}  // namespace QingStor
}  // namespace QS
