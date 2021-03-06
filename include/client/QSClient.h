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

#ifndef INCLUDE_CLIENT_QSCLIENT_H_
#define INCLUDE_CLIENT_QSCLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "client/Client.h"
#include "client/QSClientOutcome.h"

namespace QingStor {
class QsConfig;
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
  // @param  : flag to use thread pool worker thread or not
  // @return : ClientError
  ClientError<QSError> HeadBucket(bool useThreadPool = true) override;

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
  // MoveDirectory move dir, subdirs and subfiles recursively.
  // Notes: MoveDirectory will do nothing on dir tree and cache.
  ClientError<QSError> MoveDirectory(const std::string &sourceDirPath,
                                     const std::string &targetDirPath,
                                     bool async = false) override;

  // Download file
  //
  // @param  : file path, buffer(input), contenct range, eTag (output)
  // @return : ClinetError
  //
  // If range is empty, then the whole file will be downloaded.
  // The file data will be written to buffer.
  ClientError<QSError> DownloadFile(
      const std::string &filePath, const std::shared_ptr<std::iostream> &buffer,
      const std::string &range = std::string(),
      std::string *eTag = nullptr) override;

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
  // @param  : dir path, falg to use thread pool or not
  // @return : ClientError
  //
  // ListDirectory will update directory in tree if dir exists and is modified
  // or grow the tree if the directory is not existing in tree.
  //
  // Notice the dirPath should end with delimiter.
  ClientError<QSError> ListDirectory(const std::string &dirPath,
                                     bool useThreadPool = true) override;

  // Create a symbolic link to a file
  //
  // @param  : file path to link to, link path
  // @return : void
  //
  // symbolic link is a file that contains a reference to the file or dir,
  // the reference is the realitive path (from fuse) to the file,
  // fuse will parse . and .., so we just put the path as link file content.
  ClientError<QSError> SymLink(const std::string &filePath,
                               const std::string &linkPath) override;

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
                            bool *modified = nullptr) override;

  // Get information about mounted bucket
  //
  // @param  : *stvfs(output)
  // @return : ClientError
  ClientError<QSError> Statvfs(struct statvfs *stvfs) override;

 public:
  //
  // Following api only submit sdk request, no ops on local dirtree and cache.
  //

  // Delete object
  //
  // @param  : object path
  // @return : ClientError
  //
  // This only submit skd delete object request, no ops on dir tree and cache.
  ClientError<QSError> DeleteObject(const std::string &path);

  // Move object
  //
  // @param  : source file path, target file path
  // @return : ClientError
  //
  // This only submit sdk put(move) object request, no ops on dir tree and cache
  ClientError<QSError> MoveObject(const std::string &sourcePath,
                                  const std::string &targetPath);

  // List objects
  //
  // @param  : input, resultTruncated(output), resCount(outputu) maxCount,
  //           flag to use thread pool or not
  // @return : ListObjectsOutcome
  //
  // Use maxCount to specify the count limit of objects you want to list.
  // Use maxCount = 0 to list all the objects, this is default option.
  // Use resCount to obtain the actual listed objects number
  // Use resultTruncated to obtain the status of whether the operation has
  // list all of the objects of the bucket;
  //
  // This only submit skd listobjects request, no ops on dir tree and cache.
  ListObjectsOutcome ListObjects(const std::string &dirPath,
                                 bool *resultTruncated = nullptr,
                                 uint64_t *resCount = nullptr,
                                 uint64_t maxCount = 0,
                                 bool useThreadPool = true);

 public:
  static const std::unique_ptr<QingStor::QsConfig> &GetQingStorConfig();
  const std::shared_ptr<QSClientImpl> &GetQSClientImpl() const;

 private:
  std::shared_ptr<QSClientImpl> &GetQSClientImpl();
  static void StartQSService();
  void CloseQSService();
  void InitializeClientImpl();

 private:
  static std::unique_ptr<QingStor::QsConfig> m_qingStorConfig;
  std::shared_ptr<QSClientImpl> m_qsClientImpl;
};

}  // namespace Client
}  // namespace QS


#endif  // INCLUDE_CLIENT_QSCLIENT_H_
