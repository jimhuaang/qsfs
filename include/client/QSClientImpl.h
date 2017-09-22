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

#include "client/QSClientOutcome.h"

namespace QS {

namespace Client {
class QSClient;

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

  // Get bucket statistics
  GetBucketStatisticsOutcome GetBucketStatistics() const;

  // Head bucket
  HeadBucketOutcome HeadBucket() const;

  // List bucket objects
  //
  // @param  : input, resultTruncated(output), maxCount
  // @return : ListObjectsOutcome
  //
  // Use maxCount to specify the count limit of objects you want to list.
  // Use maxCount = 0 to list all the objects, this is default option.
  // Use resultTruncated to obtain the status of whether the operation has
  // list all of the objects of the bucket;
  //
  // If resultTruncated is true the input will be set with the next marker which
  // will help to continue the following list operation.
  ListObjectsOutcome ListObjects(QingStor::ListObjectsInput *input,
                                 bool *resultTruncated = nullptr,
                                 uint64_t maxCount = 0) const;

  // Delete multiple objects
  DeleteMultipleObjectsOutcome DeleteMultipleObjects(
      QingStor::DeleteMultipleObjectsInput *input) const;

  // List multipart uploads
  //
  // @param  : object key, input, resultTruncated(output), maxCount
  // @return : ListMultipartUploadsOutcome
  //
  // Use maxCount to specify the count limit of uploading part you want to list.
  // Use maxCount = 0 to list all the uploading part, this is default option.
  // Use resultTruncated to obtain the status of whether the operation has
  // list all of the uploading parts of the bucket;
  //
  // If resultTruncated is true the input will be set with the next marker which
  // will help to continue the following list operation.
  ListMultipartUploadsOutcome ListMultipartUploads(
      QingStor::ListMultipartUploadsInput *input,
      bool *resultTruncated = nullptr, uint64_t maxCount = 0) const;

  //
  // Object Level Operations
  //

  // Delete object
  //
  // @param  : object key
  // @return : DeleteObjectOutcome
  DeleteObjectOutcome DeleteObject(const std::string &objKey) const;

  // Get object
  //
  // @param  : object key, GetObjectInput
  // @return : GetObjectOutcome
  GetObjectOutcome GetObject(const std::string &objKey,
                             QingStor::GetObjectInput *input) const;

  // Head object
  //
  // @param  : object key, HeadObjectInput
  // @return : HeadObjectOutcome
  HeadObjectOutcome HeadObject(const std::string &objKey,
                               QingStor::HeadObjectInput *input) const;

  // Put object
  //
  // @param  : object key, PutObjectInput
  // @return : PutObjectOutcome
  PutObjectOutcome PutObject(const std::string &objKey,
                             QingStor::PutObjectInput *input) const;

  //
  // Multipart Operations
  //

  // Initiate multipart upload
  //
  // @param  : object key, InitiateMultipartUploadInput
  // @return : InitiateMultipartUploadOutcome
  InitiateMultipartUploadOutcome InitiateMultipartUpload(
      const std::string &objKey,
      QingStor::InitiateMultipartUploadInput *input) const;

  // Upload multipart
  //
  // @param  : object key, UploadMultipartInput
  // @return : UploadMultipartOutcome
  UploadMultipartOutcome UploadMultipart(
      const std::string &objKey, QingStor::UploadMultipartInput *input) const;

  // Complete multipart upload
  //
  // @param  : object key, CompleteMultipartUploadInput
  // @return : CompleteMultipartUploadOutcome
  CompleteMultipartUploadOutcome CompleteMultipartUpload(
      const std::string &objKey,
      QingStor::CompleteMultipartUploadInput *input) const;

  // Abort multipart upload
  //
  // @param  : object key, AbortMultipartUploadInput
  // @return : AbortMultipartUploadOutcome
  AbortMultipartUploadOutcome AbortMultipartUpload(
      const std::string &objKey,
      QingStor::AbortMultipartUploadInput *input) const;

  // List multipart
  //
  // @param  : object key, input, resultTruncated(output), maxCount
  // @return : ListMultipartOutcome
  //
  // Use maxCount to specify the count limit of parts you want to list.
  // Use maxCount = 0 to list all the parts, this is default option.
  // Use resultTruncated to obtain the status of whether the operation has
  // list all of the parts of the object;
  //
  // If resultTruncated is true the input will be set with the last part id of
  // this operation which will help to continue the following list operation.
  ListMultipartOutcome ListMultipart(const std::string &objKey,
                                     QingStor::ListMultipartInput *input,
                                     bool *resultTruncated = nullptr,
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
