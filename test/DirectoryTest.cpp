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

#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "data/Directory.h"
#include "data/FileMetaData.h"

namespace {

using QS::Data::Entry;
using QS::Data::FileMetaData;
using QS::Data::FileType;
using QS::Data::Node;
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
  string fileName;
  uint64_t fileSize;
  FileType fileType;
  int numLink;
  bool isDir;
  bool isOperable;

  friend ostream &operator<<(ostream &os, const MetaData &meta) {
    return os << "FileName: " << meta.fileName << " FileSize: " << meta.fileSize
              << " FileType: " << QS::Data::GetFileTypeName(meta.fileType)
              << " NumLink: " << meta.numLink << " IsDir: " << meta.isDir
              << " IsOperable: " << meta.isOperable;
  }
};

class EntryTest : public Test, public WithParamInterface<MetaData> {
 public:
  EntryTest() {
    auto meta = GetParam();
    m_pFileMetaData.reset(new FileMetaData(meta.fileName, meta.fileSize, mtime_,
                                           mtime_, uid_, gid_, fileMode_,
                                           meta.fileType));
    m_pEntry.reset(new Entry(meta.fileName, meta.fileSize, mtime_, mtime_, uid_,
                             gid_, fileMode_, meta.fileType));
  }

 protected:
  shared_ptr<FileMetaData> m_pFileMetaData;
  unique_ptr<Entry> m_pEntry;
};

class NodeTest : public Test {
 protected:
  static void SetUpTestCase() {
    pRootEntry = unique_ptr<Entry>(new Entry(
        "/", 0, mtime_, mtime_, uid_, gid_, fileMode_, FileType::Directory));
    pRootNode = make_shared<Node>( unique_ptr<Entry>(new Entry(*pRootEntry)), nullptr);
    pFileNode1 = make_shared<Node>(
        unique_ptr<Entry>(new Entry("file1", 1024, mtime_, mtime_, uid_, gid_,
                                    fileMode_, FileType::File)),
        pRootNode);
    pLinkNode = make_shared<Node>(
        unique_ptr<Entry>(new Entry("linkToFile1", strlen(path), mtime_, mtime_,
                                    uid_, gid_, fileMode_, FileType::SymLink)),
        pRootNode, path);
    pEmptyNode = unique_ptr<Node>(new Node);
  }

  static void TearDownTestCase() {
    pRootEntry = nullptr;
    pRootNode = nullptr;
    pFileNode1 = nullptr;
    pLinkNode = nullptr;
    pEmptyNode = nullptr;
  }

 protected:
  static const char path[];
  static unique_ptr<Entry> pRootEntry;
  static shared_ptr<Node> pRootNode;
  static shared_ptr<Node> pFileNode1;
  static shared_ptr<Node> pLinkNode;
  static unique_ptr<Node> pEmptyNode;
};

const char NodeTest::path[] = "pathLinkToFile1";
unique_ptr<Entry> NodeTest::pRootEntry(nullptr);
shared_ptr<Node> NodeTest::pRootNode(nullptr);
shared_ptr<Node> NodeTest::pFileNode1(nullptr);
shared_ptr<Node> NodeTest::pLinkNode(nullptr);
unique_ptr<Node> NodeTest::pEmptyNode(nullptr);
}  // namespace

TEST_P(EntryTest, CopyControl) {
  auto meta = GetParam();
  Entry entry = Entry(m_pFileMetaData);
  EXPECT_EQ(*m_pEntry, entry);
}

TEST_P(EntryTest, PublicFunctions) {
  auto meta = GetParam();
  EXPECT_EQ(m_pEntry->GetFileName(), meta.fileName);
  EXPECT_EQ(m_pEntry->GetFileSize(), meta.fileSize);
  EXPECT_EQ(m_pEntry->GetFileType(), meta.fileType);
  EXPECT_EQ(m_pEntry->GetNumLink(), meta.numLink);
  EXPECT_EQ(m_pEntry->IsDirectory(), meta.isDir);
  EXPECT_EQ(m_pEntry->operator bool(), meta.isOperable);
}

INSTANTIATE_TEST_CASE_P(
    FSEntryTest, EntryTest,
    // fileName, fileSize, fileType, numLink, isDir, isOperable
    Values(MetaData{"", 0, FileType::Directory, 2, true, false},
           MetaData{"/", 0, FileType::Directory, 2, true, true},
           MetaData{"/file1", 0, FileType::File, 1, false, true},
           MetaData{"/file2", 1024, FileType::File, 1, false, true}));

TEST_F(NodeTest, DefaultCtor) {
  EXPECT_FALSE(pEmptyNode->operator bool());
  EXPECT_TRUE(pEmptyNode->IsEmpty());
  EXPECT_FALSE(const_cast<const Node*> (pEmptyNode.get())->GetEntry());
  EXPECT_TRUE(pEmptyNode->GetFileName().empty());
}

TEST_F(NodeTest, CustomCtors) {
  EXPECT_TRUE(pRootNode->operator bool());
  EXPECT_TRUE(pRootNode->IsEmpty());
  EXPECT_EQ(pRootNode->GetFileName(), "/");
  EXPECT_EQ(*(const_cast<const Node*>(pRootNode.get())->GetEntry()),
            *pRootEntry);
  EXPECT_EQ(pRootNode->GetFileName(), pRootEntry->GetFileName());

  EXPECT_EQ(*(pFileNode1->GetParent().lock()), *pRootNode);

  EXPECT_EQ(pLinkNode->GetSymbolicLink(), string(path));
}

TEST_F(NodeTest, PublicFunctions) {
  // When sharing resources between tests in test case of NodeTest,
  // as the test oreder is undefined, so we must restore the state
  // to its original value before passing control to the next test.
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
  pRootNode->RenameChild(newFileName, oldFileName);

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
