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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_RETRYSTRATEGY_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_RETRYSTRATEGY_H_  // NOLINT

#include "client/ClientError.h"
#include "client/QSError.h"

namespace QS {

namespace Client {

namespace Retry {
static const unsigned DefaultMaxRetries = 1;
static const unsigned DefaultScaleFactor = 25;
}  // namespace Retry

class RetryStrategy {
 public:
  RetryStrategy(unsigned maxRetryTimes = Retry::DefaultMaxRetries,
                unsigned scaleFactor = Retry::DefaultScaleFactor)
      : m_maxRetryTimes(maxRetryTimes), m_scaleFactor(scaleFactor) {}

  bool ShouldRetry(const ClientError<QSError> &error,
                   unsigned attemptedRetryTimes) const;

  int32_t CalculateDelayBeforeNextRetry(const ClientError<QSError> &error,
                                        unsigned attemptedRetryTimes) const;

 private:
  unsigned m_maxRetryTimes;
  unsigned m_scaleFactor;
};

RetryStrategy GetDefaultRetryStrategy();
RetryStrategy GetCustomRetryStrategy();

}  // namespace Client
}  // namespace QS

#endif  // _QSFS_FUSE_INCLUDED_CLIENT_RETRYSTRATEGY_H_
