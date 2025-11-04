/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FrameworkApplicationFixture.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzTest/Utils.h>

class ApplicationTest
    : public UnitTest::FrameworkApplicationFixture
{
protected:

    void SetUp() override
    {
        FrameworkApplicationFixture::SetUp();
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder, m_tempDirectory.GetDirectory());
        }
        if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
        {
            fileIoBase->SetAlias("@products@", m_tempDirectory.GetDirectory());
        }
    }

    void TearDown() override
    {
        FrameworkApplicationFixture::TearDown();
    }

    AZ::Test::ScopedAutoTempDirectory m_tempDirectory;
};

TEST_F(ApplicationTest, MakePathAssetRootRelative_AbsPath_Valid)
{
    AZ::IO::Path inputPath = m_tempDirectory.GetDirectoryAsPath() / "TestA.txt";
    m_application->MakePathAssetRootRelative(inputPath.Native());
    EXPECT_EQ(inputPath, "testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_AbsPath_Valid)
{
    AZ::IO::Path inputPath = m_tempDirectory.GetDirectoryAsPath() / "TestA.txt";
    m_application->MakePathRelative(inputPath.Native(), m_tempDirectory.GetDirectory());
    EXPECT_EQ(inputPath, "TestA.txt");
}

TEST_F(ApplicationTest, MakePathRelative_AbsPath_RootLowerCase_Valid)
{
    AZ::IO::Path root = m_tempDirectory.GetDirectory();
    AZStd::to_lower(root.Native());
    AZ::IO::Path inputPath = root / "TestA.txt";
    m_application->MakePathRelative(inputPath.Native(), root.c_str());
    EXPECT_EQ(inputPath, "TestA.txt");
}


TEST_F(ApplicationTest, MakePathAssetRootRelative_AbsPathWithSubFolders_Valid)
{
    AZ::IO::Path inputPath = m_tempDirectory.GetDirectoryAsPath() / "Foo/TestA.txt";
    m_application->MakePathAssetRootRelative(inputPath.Native());
    EXPECT_EQ(inputPath, "foo/testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_AbsPathWithSubFolders_Valid)
{
    AZ::IO::Path inputPath = m_tempDirectory.GetDirectoryAsPath() / "Foo/TestA.txt";
    m_application->MakePathRelative(inputPath.Native(), m_tempDirectory.GetDirectory());
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

TEST_F(ApplicationTest, MakePathAssetRootRelative_RelPathStartingWithSeparator_NotRelative)
{
    // A path starting with a Posix path separator is an absolute path not relative
    // Therefore it can't be relative to the AssetPath
    AZStd::string inputPath("//TestA.txt");
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_NE(inputPath, "testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_RelPathStartingWithSeparator_NotRelative)
{
    // A path starting with a Posix path separator is an absolute path not relative
    // Therefore it can't be relative to the temporary directory
    AZStd::string inputPath("//TestA.txt");
    m_application->MakePathRelative(inputPath, m_tempDirectory.GetDirectory());
    EXPECT_NE(inputPath, "TestA.txt");
}

TEST_F(ApplicationTest, MakePathAssetRootRelative_RelPathWithSubFolderStartingWithSeparator_NotRelative)
{
    // A path starting with a Posix path separator is an absolute path not relative
    // Therefore it can't be relative to the AssetPath
    AZStd::string inputPath("//Foo/TestA.txt");
    m_application->MakePathAssetRootRelative(inputPath);
    EXPECT_NE(inputPath, "foo/testa.txt");
}

TEST_F(ApplicationTest, MakePathRelative_RelPathWithSubFolderStartingWithSeparator_NotRelative)
{
    // A path starting with a Posix path separator is an absolute path not relative
    // Therefore it can't be relative to the temporary directory
    AZStd::string inputPath("//Foo/TestA.txt");
    m_application->MakePathRelative(inputPath, m_tempDirectory.GetDirectory());
    EXPECT_NE(inputPath, "Foo/TestA.txt");
}
