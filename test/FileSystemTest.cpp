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

#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "base/FileSystem.h"

namespace {

using namespace QS::FileSystem;

using std::make_shared;
using std::ostream;
using std::string;
using std::shared_ptr;
using std::unique_ptr;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

// default values for non interested attributes
time_t mtime_ = time(NULL);
uid_t uid_ = 1000U;
gid_t gid_ = 1000U;
mode_t fileMode_ = S_IRWXU | S_IRWXG | S_IROTH;

struct MetaData {
  string fileId;
  uint64_t fileSize;
  FileType fileType;
  int numLink;
  bool isDir;
  bool isOperable;

  friend ostream &operator<<(ostream &os, const MetaData &meta) {
    return os << "FileId: " << meta.fileId << " FileSize: " << meta.fileSize
              << " FileType: " << GetFileTypeName(meta.fileType)
              << " NumLink: " << meta.numLink << " IsDir: " << meta.isDir
              << " IsOperable: " << meta.isOperable;
  }
};

class EntryTest : public Test, public WithParamInterface<MetaData> {
 public:
  EntryTest() {
    auto meta = GetParam();
    m_pFileMetaData.reset(new Entry::FileMetaData(mtime_, mtime_, uid_, gid_,
                                                  fileMode_, meta.fileType));
    m_pEntry.reset(new Entry(meta.fileId, meta.fileSize, mtime_, mtime_, uid_,
                             gid_, fileMode_, meta.fileType));
  }

 protected:
  unique_ptr<Entry::FileMetaData> m_pFileMetaData;
  unique_ptr<Entry> m_pEntry;
};

class NodeTest : public Test {
 public:
  NodeTest()
      : pEmptyNode(unique_ptr<Node>(new Node)),
        rootEntry("root", 0, mtime_, mtime_, uid_, gid_, fileMode_,
                  FileType::Directory),
        pRootNode(make_shared<Node>(
            "/", unique_ptr<Entry>(new Entry(rootEntry)), nullptr)),
        pFileNode1(make_shared<Node>(
            "/myfile1",
            unique_ptr<Entry>(new Entry("file1", 1024, mtime_, mtime_, uid_,
                                        gid_, fileMode_, FileType::File)),
            pRootNode)),
        path("pathLinkToFile1"),
        pLinkNode(make_shared<Node>(
            "/mylink1", unique_ptr<Entry>(new Entry(
                            "linkToFile1", path.size(), mtime_, mtime_, uid_,
                            gid_, fileMode_, FileType::SymLink)),
            pRootNode, path)) {}

 public:
  unique_ptr<Node> pEmptyNode;
  Entry rootEntry;
  shared_ptr<Node> pRootNode;
  shared_ptr<Node> pFileNode1;
  string path;
  shared_ptr<Node> pLinkNode;
};

}  // namespace

TEST_P(EntryTest, CopyControl) {
  auto meta = GetParam();
  Entry entry = Entry(meta.fileId, meta.fileSize, *m_pFileMetaData);
  EXPECT_EQ(*m_pEntry, entry);
}

TEST_P(EntryTest, PublicFunctions) {
  auto meta = GetParam();
  EXPECT_EQ(m_pEntry->GetFileId(), meta.fileId);
  EXPECT_EQ(m_pEntry->GetFileSize(), meta.fileSize);
  EXPECT_EQ(m_pEntry->GetFileType(), meta.fileType);
  EXPECT_EQ(m_pEntry->GetNumLink(), meta.numLink);
  EXPECT_EQ(m_pEntry->IsDirectory(), meta.isDir);
  EXPECT_EQ(m_pEntry->operator bool(), meta.isOperable);
}

INSTANTIATE_TEST_CASE_P(
    FSEntryTest, EntryTest,
    // fileId, fileSize, fileType, numLink, isDir, isOperable
    Values(MetaData{"null", 0, FileType::None, 0, false, false},
           MetaData{"", 0, FileType::Directory, 2, true, false},
           MetaData{"root", 0, FileType::Directory, 2, true, true},
           MetaData{"file1", 0, FileType::File, 1, false, true},
           MetaData{"file2", 1024, FileType::File, 1, false, true}));

TEST_F(NodeTest, DefaultCtor) {
  EXPECT_FALSE(pEmptyNode->operator bool());
  EXPECT_TRUE(pEmptyNode->IsEmpty());
  EXPECT_FALSE(pEmptyNode->GetEntry());
  EXPECT_TRUE(pEmptyNode->GetFileId().empty());
}

TEST_F(NodeTest, CustomCtors) {
  EXPECT_TRUE(pRootNode->operator bool());
  EXPECT_TRUE(pRootNode->IsEmpty());
  EXPECT_EQ(pRootNode->GetFileName(), "/");
  EXPECT_EQ(*(pRootNode->GetEntry()), rootEntry);
  EXPECT_EQ(pRootNode->GetFileId(), rootEntry.GetFileId());

  EXPECT_EQ(*(pFileNode1->GetParent().lock()), *pRootNode);

  EXPECT_EQ(pLinkNode->GetSymbolicLink(), path);
}

TEST_F(NodeTest, PublicFunctions){
  EXPECT_FALSE(pRootNode->Find(pFileNode1->GetFileName()));
  pRootNode->Insert(pFileNode1);
  EXPECT_EQ(pRootNode->Find(pFileNode1->GetFileName()), pFileNode1);
  EXPECT_EQ(pRootNode->GetChildren().size(), 1U);

  EXPECT_FALSE(pRootNode->Find(pLinkNode->GetFileName()));
  pRootNode->Insert(pLinkNode);
  EXPECT_EQ(pRootNode->Find(pLinkNode->GetFileName()), pLinkNode);
  EXPECT_EQ(pRootNode->GetChildren().size(), 2U);

  string oldFileName = pFileNode1->GetFileName();
  string newFileName("myNewFile1");
  pRootNode->RenameChild(oldFileName, newFileName);
  EXPECT_FALSE(pRootNode->Find(oldFileName));
  EXPECT_TRUE(pRootNode->Find(newFileName));
  EXPECT_EQ(pFileNode1->GetFileName(), newFileName);

  pRootNode->Remove(pFileNode1);
  EXPECT_FALSE(pRootNode->Find(pFileNode1->GetFileName()));
  pRootNode->Remove(pLinkNode);
  EXPECT_FALSE(pRootNode->Find(pLinkNode->GetFileName()));
  EXPECT_TRUE(pRootNode->IsEmpty());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
