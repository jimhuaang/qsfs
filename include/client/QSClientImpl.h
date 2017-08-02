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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENTIMPL_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENTIMPL_H_  // NOLINT

#include "client/ClientImpl.h"

#include <stdint.h>  // for uint64_t

#include "qingstor-sdk-cpp/Bucket.h"  // for instantiation of QSClientImpl

#include <memory>
#include <vector>

#include "client/ClientError.h"
#include "client/Outcome.h"
#include "client/QSError.h"

namespace QS {

namespace Client {
class QSClient;

using HeadBucketOutcome =
    Outcome<QingStor::HeadBucketOutput, ClientError<QSError>>;
using ListObjectsOutcome =
    Outcome<std::vector<QingStor::ListObjectsOutput>, ClientError<QSError>>;

class QSClientImpl : public ClientImpl {
 public:
  QSClientImpl();
  QSClientImpl(QSClientImpl &&) = default;
  QSClientImpl(const QSClientImpl &) = default;
  QSClientImpl &operator=(QSClientImpl &&) = default;
  QSClientImpl &operator=(const QSClientImpl &) = default;
  ~QSClientImpl() = default;

 public:
  void HeadObject() override;

 public:
  HeadBucketOutcome HeadBucket() const;
  // Use maxCount to specify the maximum count of objects you want to list.
  // Use maxCount = 0 to list all the objects, this is default option.
  // Use resultTruncated to obtain the status of whether the operation has
  // list all of the objects of the bucket; If resultTruncated is true the
  // input will be set with the next marker which will help to continue
  // the following list operation.
  ListObjectsOutcome ListObjects(QingStor::ListObjectsInput *input,
                                 bool *resultTruncated,
                                 uint64_t maxCount = 0) const;

 public:
  const std::unique_ptr<QingStor::Bucket> &GetBucket() const {
    return m_bucket;
  }

 private:
  void SetBucket(std::unique_ptr<QingStor::Bucket> bucket);

 private:
  std::unique_ptr<QingStor::Bucket> m_bucket;
  friend class QSClient;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENTIMPL_H_
