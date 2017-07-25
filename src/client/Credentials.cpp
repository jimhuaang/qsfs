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

#include "client/Credentials.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>

#include <fstream>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <utility>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "base/Utils.h"
#include "filesystem/Configure.h"

namespace QS {

namespace Client {

using QS::Exception::QSException;
using std::call_once;
using std::ifstream;
using std::make_pair;
using std::once_flag;
using std::pair;
using std::string;
using std::unique_ptr;

namespace {

pair<bool, string> ErrorOut(string&& str) { return {false, str}; }
pair<bool, string> CheckCredentialsFilePermission(const string& file);

}  // namespace

static unique_ptr<CredentialsProvider> credentialsProvider(nullptr);
static once_flag credentialsProviderOnceFlag;

void InitializeCredentialsProvider(unique_ptr<CredentialsProvider> provider) {
  assert(provider);
  call_once(credentialsProviderOnceFlag,
            [&provider] { credentialsProvider = std::move(provider); });
}

CredentialsProvider& GetCredentialsProviderInstance() {
  call_once(credentialsProviderOnceFlag, [] {
    credentialsProvider =
        unique_ptr<CredentialsProvider>(new DefaultCredentialsProvider(
            QS::FileSystem::Configure::GetCredentialsFile()));
  });
  return *credentialsProvider.get();
}

DefaultCredentialsProvider::DefaultCredentialsProvider(
    const string& credentialFile)
    : m_credentialsFile(credentialFile) {
  auto outcome = ReadCredentialsFile(credentialFile);
  if (!outcome.first) {
    throw QSException(outcome.second);
  }
}

pair<bool, string> DefaultCredentialsProvider::ReadCredentialsFile(
    const std::string& file) {
  bool success = true;
  string errMsg;
  auto Postfix = [&file]() -> string {
    return "credentials file " + file;
  };

  if (QS::Utils::FileExists(file)) {
    // Check credentials file permission
    auto outcome = CheckCredentialsFilePermission(file);
    if (!outcome.first) return outcome;

    // Check if have permission to read credetials file
    if (!QS::Utils::HavePermission(file, true)) {
      return ErrorOut("Credentials file " + file + " : Permisson denied");
    }

    // Read credentials
    static const char* invalidChars = " \t";  // Not allow space and tab
    static const char DELIM = ':';

    ifstream credentials(file);
    if (credentials) {
      string line;
      string::size_type firstPos = string::npos;
      string::size_type lastPos = string::npos;
      while (std::getline(credentials, line)) {
        if (line.front() == '#' || line.empty()) continue;
        if (line.back() == '\r') {
          line.pop_back();
          if (line.empty()) continue;
        }
        if (line.front() == '[') {
          return ErrorOut(
              "Invalid line starting with a bracket \"[\" is found in " +
              Postfix());
        }
        if (line.find_first_of(invalidChars) != string::npos) {
          return ErrorOut("Invalid line with whitespace or tab is found in " +
                          Postfix());
        }

        firstPos = line.find_first_of(DELIM);
        if (firstPos == string::npos) {
          return ErrorOut("Invalid line with no \":\" seperator is found in " +
                          Postfix());
        }
        lastPos = line.find_last_of(DELIM);

        if (firstPos == lastPos) {  // Found default key
          if (HasDefaultKey()) {
            DebugWarning("More than one default key pairs are provided in " +
                         Postfix() + ". Only set with the first one");
            continue;
          } else {
            SetDefaultKey(line.substr(0, firstPos), line.substr(firstPos + 1));
          }
        } else {  // Found bucket specified key
          m_bucketMap.emplace(line.substr(0, firstPos),
                              make_pair(line.substr(firstPos + 1, lastPos),
                                        line.substr(lastPos + 1)));
        }
      }
    } else {
      return ErrorOut("Fail to read credentilas file " + file + " : " +
                      strerror(errno));
    }
  } else {
    return ErrorOut("Credentials file " + file + " is not existing");
  }

  return {success, errMsg};
}

namespace {

pair<bool, string> CheckCredentialsFilePermission(const string& file) {
  struct stat st;
  if (stat(file.c_str(), &st) != 0) {
    return ErrorOut("Unable to read credentials file " + file + " : " +
                    strerror(errno));
  }
  if ((st.st_mode & S_IROTH) || (st.st_mode & S_IWOTH) ||
      (st.st_mode & S_IXOTH)) {
    return ErrorOut("Credentials file " + file +
                    " should not have others permissions");
  }
  if ((st.st_mode & S_IRGRP) || (st.st_mode & S_IWGRP) ||
      (st.st_mode & S_IXGRP)) {
    return ErrorOut("Credentials file " + file +
                    " should not have group permissions");
  }
  if ((st.st_mode & S_IXUSR)) {
    return ErrorOut("Credentials file " + file +
                    " should not have executable permissions");
  }

  return {true, ""};
}

}  // namespace

}  // namespace Client
}  // namespace QS
