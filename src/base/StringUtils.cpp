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

#include "base/StringUtils.h"

#include <algorithm>
#include <cctype>

#include <sys/stat.h>  // for S_ISUID
#include <unistd.h>    // for R_OK

namespace QS {

namespace StringUtils {

using std::string;

// --------------------------------------------------------------------------
string ToLower(const string &str) {
  string copy(str);
  for (auto &c : copy) {
    c = std::tolower(c);
  }
  return copy;
}

// --------------------------------------------------------------------------
string ToUpper(const string &str) {
  string copy(str);
  for (auto &c : copy) {
    c = std::toupper(c);
  }
  return copy;
}

// --------------------------------------------------------------------------
string LTrim(const string &str , unsigned char ch) {
  string copy(str);
  auto pos = std::find_if_not(copy.begin(), copy.end(),
                              [&ch](unsigned char c) { return ch == c; });
  copy.erase(copy.begin(), pos);
  return copy;
}

// --------------------------------------------------------------------------
string RTrim(const string &str, unsigned char ch) {
  string copy(str);
  auto rpos = std::find_if_not(copy.rbegin(), copy.rend(),
                               [&ch](unsigned char c) { return ch == c; });
  copy.erase(rpos.base(), copy.end());
  return copy;
}

// --------------------------------------------------------------------------
string Trim(const string &str, unsigned char ch) { return LTrim(RTrim(str, ch), ch); }

// --------------------------------------------------------------------------
string AccessMaskToString(int amode) {
  string ret;
  if(amode & R_OK){
    ret.append("R_OK ");
  }
  if(amode & W_OK){
    ret.append("W_OK ");
  }
  if(amode & X_OK){
    ret.append("X_OK");
  }
  ret = QS::StringUtils::RTrim(ret,' ');
  string::size_type pos = 0;
  while((pos = ret.find(' ')) != string::npos){
    ret.replace(pos, 1, "&");
  }
  return ret;
}

// --------------------------------------------------------------------------
std::string ModeToString(mode_t mode) {
  // access MODE bits          000    001    010    011
  //                           100    101    110    111
  static const char *rwx[] = {"---", "--x", "-w-", "-wx",
                              "r--", "r-x", "rw-", "rwx"};
  string modeStr;
  modeStr.append(rwx[(mode >> 6) & 7]);  // user
  modeStr.append(rwx[(mode >> 3) & 7]);  // group
  modeStr.append(rwx[(mode & 7)]);

  if (mode & S_ISUID) {
    modeStr[2] = (mode & S_IXUSR) ? 's' : 'S';
  }
  if (mode & S_ISGID) {
    modeStr[5] = (mode & S_IXGRP) ? 's' : 'l';
  }
  if (mode & S_ISVTX) {
    modeStr[8] = (mode & S_IXUSR) ? 't' : 'T';
  }
  return modeStr;
}

}  // namespace StringUtils
}  // namespace QS
