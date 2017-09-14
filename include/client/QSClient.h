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

namespace QingStor {
class QingStorService;
}  // namespace QingStor

namespace QS {

namespace Client {

class QSClientImpl;

class QSClient : public Client {
 public:
  QSClient();
  QSClient(QSClient &&) = default;
  QSClient(const QSClient &) = delete;
  QSClient &operator=(QSClient &&) = default;
  QSClient &operator=(const QSClient &) = delete;
  ~QSClient();

 public:

  // Head bucket
  //
  // @param  : void
  // @return : ClientError
  ClientError<QSError> HeadBucket() override;

  // Delete a file
  //
  // @param  : file path
  // @return : ClientError
  //
  // DeleteFile is used to delete a file or an empty directory.
  // As object storage has no concept of file type (such as directory),
  // you can call DeleteFile to delete any object. But if the object is
  // a nonempty directory, DeleteFile will not delete its contents (including
  // files or subdirectories belongs to it).
  ClientError<QSError> DeleteFile(const std::string &filePath) override;

  // Delete a directory
  //
  // @param  : dir path, flag recursively to remove its contents
  // @return : ClientError
  //
  // DeleteDirectory is used to delete a directory and its contents recursively.
  // Current no implementation.
  ClientError<QSError> DeleteDirectory(const std::string &dirPath,
                                       bool recursive) override;

  // Create an empty file
  //
  // @param  : file path
  // @return : ClientError
  // As qs sdk doesn't return the created file meta data in PutObjectOutput,
  // So we cannot grow the directory tree here, instead we need to call
  // Stat to head the object again in Drive::MakeFile;
  ClientError<QSError> MakeFile(const std::string &filePath) override;

  // Create a directory
  //
  // @param  : dir path
  // @return : ClientError
  // As qs sdk doesn't return the created dir meta data in PutObjectOutput,
  // So we cannot grow the directory tree here, instead we need to call
  // Stat to head the object again in Drive::MakeDirectory;
  ClientError<QSError> MakeDirectory(const std::string &dirPath) override;

  // Move file
  //
  // @param  : file path, new file path
  // @return : ClientError
  //
  // MoveFile will invoke dirTree and Cache renaming.
  ClientError<QSError> MoveFile(const std::string &sourcefilePath,
                                const std::string &destFilePath) override;

  // Move directory
  //
  // @param  : source path, target path
  // @return : ClientError
  //
  // MoveDirectory will invoke dirTree and Cache renaming
  ClientError<QSError> MoveDirectory(const std::string &sourceDirPath,
                                     const std::string &targetDirPath) override;

  // Download file
  //
  // @param  : file path, contenct range, buffer(input), eTag (output)
  // @return : ClinetError
  //
  // If range is empty, then the whole file will be downloaded.
  // The file data will be written to buffer.
  ClientError<QSError> DownloadFile(
      const std::string &filePath, const std::string &range,
      const std::shared_ptr<std::iostream> &buffer, std::string *eTag) override;

  // Initiate multipart upload id
  //
  // @param  : file path, upload id (output)
  // @return : ClientError
  ClientError<QSError> InitiateMultipartUpload(const std::string &filePath,
                                               std::string *uploadId) override;

  // Upload multipart
  //
  // @param  : file path, upload id, part number, content len, buffer
  // @return : ClientError
  ClientError<QSError> UploadMultipart(
      const std::string &filePath, const std::string &uploadId, int partNumber,
      uint64_t contentLength,
      const std::shared_ptr<std::iostream> &buffer) override;

  // Complete multipart upload
  //
  // @param  : file path, upload id, sorted part ids
  // @return : ClientError
  ClientError<QSError> CompleteMultipartUpload(
      const std::string &filePath, const std::string &uploadId,
      const std::vector<int> &sortedPartIds) override;

  // Abort mulitpart upload
  //
  // @param  : file path, upload id
  // @return : ClientError
  ClientError<QSError> AbortMultipartUpload(
      const std::string &filePath, const std::string &uploadId) override;

  // Upload file using PutObject
  //
  // @param  : file path, file size, buffer
  // @return : ClientError
  ClientError<QSError> UploadFile(
      const std::string &filePath, uint64_t fileSize,
      const std::shared_ptr<std::iostream> &buffer) override;

  // List directory
  //
  // @param  : dir path
  // @return : ClientError
  //
  // ListDirectory will update directory in tree if dir exists and is modified
  // or grow the tree if the directory is not existing in tree.
  //
  // Notice the dirPath should end with delimiter.
  ClientError<QSError> ListDirectory(const std::string &dirPath) override;

  // Get object meta data
  //
  // @param  : file path, modifiedSince, *modified(output)
  // @return : ClientError
  //
  // Using modifiedSince to match if the object modified since then.
  // Using modifiedSince = 0 to always get object meta data, this is default.
  // Using modified to gain output of object modified status since given time.
  //
  // Stat will update the node meta in dir tree if the node is modified.
  //
  // Notes: the meta will be return if object is modified, otherwise
  // the response code will be 304 (NOT MODIFIED) and no meta returned.
  ClientError<QSError> Stat(const std::string &path, time_t modifiedSince = 0,
                            bool *modified = nullptr ) override;

  // Get information about mounted bucket
  //
  // @param  : *stvfs(output)
  // @return : ClientError
  ClientError<QSError> Statvfs(struct statvfs *stvfs) override;



 public:
  static const std::unique_ptr<QingStor::QingStorService> &GetQingStorService();
  const std::shared_ptr<QSClientImpl> &GetQSClientImpl() const;

 private:
  std::shared_ptr<QSClientImpl> &GetQSClientImpl();
  static void StartQSService();
  void CloseQSService();
  void InitializeClientImpl();

 private:
  static std::unique_ptr<QingStor::QingStorService> m_qingStorService;
  std::shared_ptr<QSClientImpl> m_qsClientImpl;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENT_H_
