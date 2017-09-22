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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENTOUTCOME_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENTOUTCOME_H_  // NOLINT

#include "qingstor-sdk-cpp/Bucket.h"

#include "client/ClientError.h"
#include "client/Outcome.h"
#include "client/QSError.h"

namespace QS {

namespace Client {

using GetBucketStatisticsOutcome =
    Outcome<QingStor::GetBucketStatisticsOutput, ClientError<QSError>>;
using HeadBucketOutcome =
    Outcome<QingStor::HeadBucketOutput, ClientError<QSError>>;
using ListObjectsOutcome =
    Outcome<std::vector<QingStor::ListObjectsOutput>, ClientError<QSError>>;
using DeleteMultipleObjectsOutcome =
    Outcome<QingStor::DeleteMultipleObjectsOutput, ClientError<QSError>>;
using ListMultipartUploadsOutcome =
    Outcome<std::vector<QingStor::ListMultipartUploadsOutput>,
            ClientError<QSError>>;

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
    Outcome<std::vector<QingStor::ListMultipartOutput>, ClientError<QSError>>;

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENTOUTCOME_H_
