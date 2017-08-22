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

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "base/Logging.h"
#include "base/Utils.h"
#include "client/ClientConfiguration.h"
#include "client/Credentials.h"
#include "filesystem/Configure.h"
#include "filesystem/Initializer.h"
#include "filesystem/MimeTypes.h"
#include "filesystem/Options.h"

using QS::Client::ClientConfiguration;
using QS::Client::CredentialsProvider;
using QS::Client::DefaultCredentialsProvider;
using QS::Client::GetCredentialsProviderInstance;
using QS::Client::InitializeClientConfiguration;
using QS::Client::InitializeCredentialsProvider;
using QS::Exception::QSException;
using QS::FileSystem::Configure::GetCredentialsFile;
using QS::FileSystem::Configure::GetLogDirectory;
using QS::FileSystem::Configure::GetMimeFile;
using QS::FileSystem::Initializer;
using QS::FileSystem::Priority;
using QS::FileSystem::PriorityInitFuncPair;
using QS::Logging::ConsoleLog;
using QS::Logging::DefaultLog;
using QS::Logging::InitializeLogging;
using QS::Logging::Log;
using QS::Utils::FileExists;
using std::string;
using std::unique_ptr;

void LoggingInitializer() {
  const auto &options = QS::FileSystem::Options::Instance();
  if (options.IsForeground()) {
    InitializeLogging(unique_ptr<Log>(new ConsoleLog));
  } else {
    InitializeLogging(unique_ptr<Log>(new DefaultLog(GetLogDirectory())));
  }
  auto log = QS::Logging::GetLogInstance();
  if (log == nullptr) throw QSException("Fail to initialize logging");
  if (options.IsDebug()) {
    log->SetDebug(true);
  }
  log->SetLogLevel(options.GetLogLevel());
  if (options.IsClearLogDir()) {
    log->ClearLogDirectory();
  }
}

void CredentialsInitializer() {
  if (!FileExists(GetCredentialsFile())) {
    throw QSException("qsfs credentials file " + GetCredentialsFile() +
                      " does not exist");
  } else {
    InitializeCredentialsProvider(unique_ptr<CredentialsProvider>(
        new DefaultCredentialsProvider(GetCredentialsFile())));
  }
}

void ClientConfigurationInitializer() {
  InitializeClientConfiguration(unique_ptr<ClientConfiguration>(
      new ClientConfiguration(GetCredentialsProviderInstance())));
  ClientConfiguration::Instance().InitializeByOptions();
}

void MimeTypesInitializer() {
  string mimeFile = GetMimeFile();
  if (!FileExists(mimeFile)) {
    throw QSException(mimeFile + " does not exist");
  } else {
    QS::FileSystem::InitializeMimeTypes(mimeFile);
  }
}

void PrintCommandLineOptions() {
  // Notice: this should only be invoked after logging initialization
  const auto &options = QS::FileSystem::Options::Instance();
  std::stringstream ss;
  ss << "<<Command Line Options>> ";
  ss << options << std::endl;
  DebugInfo(ss.str());
}

namespace {

// Register the initializers
static Initializer logInitializer(PriorityInitFuncPair(Priority::First,
                                                       LoggingInitializer));
static Initializer credentialsInitializer(
    PriorityInitFuncPair(Priority::Second, CredentialsInitializer));
static Initializer clientConfigInitializer(
    PriorityInitFuncPair(Priority::Third, ClientConfigurationInitializer));

static Initializer mimeTypesInitializer(
    PriorityInitFuncPair(Priority::Fourth, MimeTypesInitializer));

// Priority must be lower than log initializer
static Initializer printCommandLineOpts(
    PriorityInitFuncPair(Priority::Fifth, PrintCommandLineOptions));

}  // namespace
