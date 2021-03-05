/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "FrameworkApplicationFixture.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzTest/Utils.h>

class ApplicationTest
    : public UnitTest::FrameworkApplicationFixture
{
protected:

    void SetUp() override
    {
        FrameworkApplicationFixture::SetUp();
        m_application->SetAssetRoot(m_tempDirectory.GetDirectory());
    }

    void TearDown() override
    {
        FrameworkApplicationFixture::TearDown();
    }

    AZStd::string m_root;
    AZ::Test::ScopedAutoTempDirectory   m_tempDirectory;
};

TEST_F(ApplicationTest, MakePathAssetRootRelative_AbsPath_Valid)
{
    AZStd::string inputPath;
    AzFramework::StringFunc::Path::ConstructFull(m_tempDirectory.GetDirectory(), "TestA.txt", inputPath, true);
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_EQ(inputPath, "testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_AbsPath_Valid)
{
    AZStd::string inputPath;
    AzFramework::StringFunc::Path::ConstructFull(m_tempDirectory.GetDirectory(), "TestA.txt", inputPath, true);
    m_application->MakePathRelative(inputPath, m_tempDirectory.GetDirectory());
    EXPECT_EQ(inputPath, "TestA.txt");
}

TEST_F(ApplicationTest, MakePathAssetRootRelative_AbsPath_RootLowerCase_Valid)
{
    AZStd::string inputPath;
    AZStd::string root = m_tempDirectory.GetDirectory();
    AZStd::to_lower(root.begin(), root.end());
    AzFramework::StringFunc::Path::ConstructFull(root.c_str(), "TestA.txt", inputPath, true);
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_EQ(inputPath, "testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_AbsPath_RootLowerCase_Valid)
{
    AZStd::string inputPath;
    AZStd::string root = m_tempDirectory.GetDirectory();
    AZStd::to_lower(root.begin(), root.end());
    AzFramework::StringFunc::Path::ConstructFull(root.c_str(), "TestA.txt", inputPath, true);
    m_application->MakePathRelative(inputPath, root.c_str());
    EXPECT_EQ(inputPath, "TestA.txt");
}


TEST_F(ApplicationTest, MakePathAssetRootRelative_AbsPathWithSubFolders_Valid)
{
    AZStd::string inputPath;
    AzFramework::StringFunc::Path::ConstructFull(m_tempDirectory.GetDirectory(), "Foo/TestA.txt", inputPath, true);
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_EQ(inputPath, "foo/testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_AbsPathWithSubFolders_Valid)
{
    AZStd::string inputPath;
    AzFramework::StringFunc::Path::ConstructFull(m_tempDirectory.GetDirectory(), "Foo/TestA.txt", inputPath, true);
    m_application->MakePathRelative(inputPath, m_tempDirectory.GetDirectory());
    EXPECT_EQ(inputPath, "Foo/TestA.txt");
}

TEST_F(ApplicationTest, MakePathAssetRootRelative_RelPath_Valid)
{
    AZStd::string inputPath("TestA.txt");
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_EQ(inputPath, "testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_RelPath_Valid)
{
    AZStd::string inputPath("TestA.txt");
    m_application->MakePathRelative(inputPath, m_tempDirectory.GetDirectory());
    EXPECT_EQ(inputPath, "TestA.txt");
}

TEST_F(ApplicationTest, MakePathAssetRootRelative_RelPathWithSubFolder_Valid)
{
    AZStd::string inputPath("Foo/TestA.txt");
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_EQ(inputPath, "foo/testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_RelPathWithSubFolder_Valid)
{
    AZStd::string inputPath("Foo/TestA.txt");
    m_application->MakePathRelative(inputPath, m_tempDirectory.GetDirectory());
    EXPECT_EQ(inputPath, "Foo/TestA.txt");
}

TEST_F(ApplicationTest, MakePathAssetRootRelative_RelPathStartingWithSeparator_Valid)
{
    AZStd::string inputPath("//TestA.txt");
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_EQ(inputPath, "testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_RelPathStartingWithSeparator_Valid)
{
    AZStd::string inputPath("//TestA.txt");
    m_application->MakePathRelative(inputPath, m_tempDirectory.GetDirectory());
    EXPECT_EQ(inputPath, "TestA.txt");
}

TEST_F(ApplicationTest, MakePathAssetRootRelative_RelPathWithSubFolderStartingWithSeparator_Valid)
{
    AZStd::string inputPath("//Foo/TestA.txt");
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_EQ(inputPath, "foo/testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_RelPathWithSubFolderStartingWithSeparator_Valid)
{
    AZStd::string inputPath("//Foo/TestA.txt");
    m_application->MakePathRelative(inputPath, m_tempDirectory.GetDirectory());
    EXPECT_EQ(inputPath, "Foo/TestA.txt");
}