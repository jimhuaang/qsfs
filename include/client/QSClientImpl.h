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

#ifndef INCLUDE_CLIENT_QSCLIENTIMPL_H_
#define INCLUDE_CLIENT_QSCLIENTIMPL_H_

#include <stdint.h>  // for uint64_t

#include <memory>
#include <string>
#include <vector>

#include "qingstor/Bucket.h"  // for instantiation of QSClientImpl

#include "client/ClientConfiguration.h"
#include "client/ClientImpl.h"
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
  GetBucketStatisticsOutcome GetBucketStatistics(
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration()) const;

  // Head bucket
  //
  // @param  : time duration in ms, flag to use thread pool worker thread or not
  // @return : HeadBucketOutcome
  HeadBucketOutcome HeadBucket(
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration(),
      bool useThreadPool = true) const;

  // List bucket objects
  //
  // @param  : input, resultTruncated(output), resCount(outputu) maxCount,
  //           time duration in ms, flag to use thread pool or not
  // @return : ListObjectsOutcome
  //
  // Use maxCount to specify the count limit of objects you want to list.
  // Use maxCount = 0 to list all the objects, this is default option.
  // Use resCount to obtain the actual listed objects number
  // Use resultTruncated to obtain the status of whether the operation has
  // list all of the objects of the bucket;
  //
  // If resultTruncated is true the input will be set with the next marker which
  // will help to continue the following list operation.
  ListObjectsOutcome ListObjects(
      QingStor::ListObjectsInput *input, bool *resultTruncated = nullptr,
      uint64_t *resCount = nullptr, uint64_t maxCount = 0,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration() * 10,
      bool useThreadPool = true) const;

  // Delete multiple objects
  DeleteMultipleObjectsOutcome DeleteMultipleObjects(
      QingStor::DeleteMultipleObjectsInput *input,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration() *
          10) const;

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
      bool *resultTruncated = nullptr, uint64_t maxCount = 0,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration() *
          10) const;

  //
  // Object Level Operations
  //

  // Delete object
  //
  // @param  : object key
  // @return : DeleteObjectOutcome
  DeleteObjectOutcome DeleteObject(
      const std::string &objKey,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration()) const;

  // Get object
  //
  // @param  : object key, GetObjectInput
  // @return : GetObjectOutcome
  GetObjectOutcome GetObject(
      const std::string &objKey, QingStor::GetObjectInput *input,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration()) const;

  // Head object
  //
  // @param  : object key, HeadObjectInput, time duration in milliseconds
  // @return : HeadObjectOutcome
  HeadObjectOutcome HeadObject(
      const std::string &objKey, QingStor::HeadObjectInput *input,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration()) const;

  // Put object
  //
  // @param  : object key, PutObjectInput, time duration in milliseconds
  // @return : PutObjectOutcome
  PutObjectOutcome PutObject(
      const std::string &objKey, QingStor::PutObjectInput *input,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration()) const;

  //
  // Multipart Operations
  //

  // Initiate multipart upload
  //
  // @param  : object key, InitiateMultipartUploadInput
  // @return : InitiateMultipartUploadOutcome
  InitiateMultipartUploadOutcome InitiateMultipartUpload(
      const std::string &objKey, QingStor::InitiateMultipartUploadInput *input,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration()) const;

  // Upload multipart
  //
  // @param  : object key, UploadMultipartInput
  // @return : UploadMultipartOutcome
  UploadMultipartOutcome UploadMultipart(
      const std::string &objKey, QingStor::UploadMultipartInput *input,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration() *
          10) const;

  // Complete multipart upload
  //
  // @param  : object key, CompleteMultipartUploadInput
  // @return : CompleteMultipartUploadOutcome
  CompleteMultipartUploadOutcome CompleteMultipartUpload(
      const std::string &objKey, QingStor::CompleteMultipartUploadInput *input,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration() *
          10) const;

  // Abort multipart upload
  //
  // @param  : object key, AbortMultipartUploadInput
  // @return : AbortMultipartUploadOutcome
  AbortMultipartUploadOutcome AbortMultipartUpload(
      const std::string &objKey, QingStor::AbortMultipartUploadInput *input,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration()) const;

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
  ListMultipartOutcome ListMultipart(
      const std::string &objKey, QingStor::ListMultipartInput *input,
      bool *resultTruncated = nullptr, uint64_t maxCount = 0,
      uint32_t msTimeDuration =
          ClientConfiguration::Instance().GetTransactionTimeDuration() *
          10) const;

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


#endif  // INCLUDE_CLIENT_QSCLIENTIMPL_H_
