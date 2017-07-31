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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENT_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENT_H_  // NOLINT

#include <memory>
#include <string>

#include "client/Client.h"

namespace QS {

namespace Client {

class QSClient : public Client {
 public:
  QSClient() = default;
  QSClient(QSClient &&) = default;
  QSClient(const QSClient &) = default;
  QSClient &operator=(QSClient &&) = default;
  QSClient &operator=(const QSClient &) = default;
  ~QSClient() = default;

 public:
  // TODO(jim):
  void Initialize() override;
  bool Connect() override;
  bool DisConnect() override;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENT_H_