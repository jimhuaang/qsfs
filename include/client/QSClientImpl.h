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
#include <string>
#include <vector>

#include "client/ClientError.h"
#include "client/Outcome.h"
#include "client/QSError.h"

namespace QS {

namespace Client {
class QSClient;

using GetBucketStatisticsOutcome =
    Outcome<QingStor::GetBucketStatisticsOutput, ClientError<QSError>>;
using HeadBucketOutcome =
    Outcome<QingStor::HeadBucketOutput, ClientError<QSError>>;
using ListObjectsOutcome =
    Outcome<std::vector<QingStor::ListObjectsOutput>, ClientError<QSError>>;
using DeleteMultipleObjectsOutcome =
    Outcome<QingStor::DeleteMultipleObjectsOutput, ClientError<QSError>>;
using ListMultipartUploadsOutcome =
    Outcome<QingStor::ListMultipartUploadsOutput, ClientError<QSError>>;

using DeleteObjectOutcome =
    Outcome<QingStor::DeleteObjectOutput, ClientError<QSError>>;
using GetObjectOutcome =
    Outcome<QingStor::GetObjectOutput, ClientError<QSError>>;
using HeadObjectOutcome =
    Outcome<QingStor::HeadObjectOutput, ClientError<QSError>>;
using PutObjectOutcome =
    Outcome<QingStor::PutObjectOutput, ClientError<QSError>>;

using InitiateMultipartUploadOutcome =
    Outcome<QingStor::InitiateMultipartUploadOutput, ClientError<QSError>>;
using UploadMultipartOutcome =
    Outcome<QingStor::UploadMultipartOutput, ClientError<QSError>>;
using CompleteMultipartUploadOutcome =
    Outcome<QingStor::CompleteMultipartUploadOutput, ClientError<QSError>>;
using AbortMultipartUploadOutcome =
    Outcome<QingStor::AbortMultipartUploadOutput, ClientError<QSError>>;
using ListMultipartOutcome =
    Outcome<QingStor::ListMultipartOutput, ClientError<QSError>>;

class QSClientImpl : public ClientImpl {
 public:
  QSClientImpl();
  QSClientImpl(QSClientImpl &&) = default;
  QSClientImpl(const QSClientImpl &) = default;
  QSClientImpl &operator=(QSClientImpl &&) = default;
  QSClientImpl &operator=(const QSClientImpl &) = default;
  ~QSClientImpl() = default;

 public:
  //
  // Bucket Level Operations
  //
  GetBucketStatisticsOutcome GetBucketStatistics() const;
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

  DeleteMultipleObjectsOutcome DeleteMultipleObjects(
      QingStor::DeleteMultipleObjectsInput *input) const;
  ListMultipartUploadsOutcome ListMultipartUploads(
      QingStor::ListMultipartUploadsInput *input) const;

  //
  // Object Level Operations
  //
  DeleteObjectOutcome DeleteObject(const std::string &objKey) const;
  GetObjectOutcome GetObject(const std::string &objKey,
                             QingStor::GetObjectInput *input) const;
  HeadObjectOutcome HeadObject(const std::string &objKey,
                               QingStor::HeadObjectInput *input) const;
  PutObjectOutcome PutObject(const std::string &objKey,
                             QingStor::PutObjectInput *input) const;
  // TODO(jim): PostObject
  // Multipart Operations
  InitiateMultipartUploadOutcome InitiateMultipartUpload(
      const std::string &objKey,
      QingStor::InitiateMultipartUploadInput *input) const;
  UploadMultipartOutcome UploadMultipart(
      const std::string &objKey, QingStor::UploadMultipartInput *input) const;
  CompleteMultipartUploadOutcome CompleteMultipartUpload(
      const std::string &objKey,
      QingStor::CompleteMultipartUploadInput *input) const;
  AbortMultipartUploadOutcome AbortMultipartUpload(
      const std::string &objKey,
      QingStor::AbortMultipartUploadInput *input) const;
  ListMultipartOutcome ListMultipart(const std::string &objKey,
                                     QingStor::ListMultipartInput *input) const;

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
