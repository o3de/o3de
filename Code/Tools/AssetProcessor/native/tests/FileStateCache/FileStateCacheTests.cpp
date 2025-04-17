/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/FileStateCache/FileStateCacheTests.h>
#include <native/utilities/assetUtils.h>
#include <native/unittests/UnitTestUtils.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace UnitTests
{
    using AssetFileInfo = AssetProcessor::AssetFileInfo;

    void FileStateCacheTests::SetUp()
    {
        m_temporarySourceDir = QDir(m_temporaryDir.path());
        AZ::IO::FileIOBase::SetInstance(aznew AZ::IO::LocalFileIO());
        m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStateCache>();
    }

    void FileStateCacheTests::TearDown()
    {
        m_fileStateCache = nullptr;
        delete AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(nullptr);
    }

    void FileStateCacheTests::CheckForFile(QString path, bool shouldExist)
    {
        bool exists = false;
        AssetProcessor::FileStateInfo fileInfo;

        auto* fileStateInterface = AZ::Interface<AssetProcessor::IFileStateRequests>::Get();

        ASSERT_NE(fileStateInterface, nullptr);
        exists = fileStateInterface->Exists(path);
        
        ASSERT_EQ(exists, shouldExist);

        exists = fileStateInterface->GetFileInfo(path, &fileInfo);

        ASSERT_EQ(exists, shouldExist);

        if (exists)
        {
            ASSERT_EQ(AssetUtilities::NormalizeFilePath(fileInfo.m_absolutePath), AssetUtilities::NormalizeFilePath(path));
            ASSERT_FALSE(fileInfo.m_isDirectory);
            ASSERT_EQ(fileInfo.m_fileSize, 0);
        }
    }

    TEST_F(FileStateCacheTests, QueryFile_ShouldNotExist)
    {
        QString testPath = m_temporarySourceDir.absoluteFilePath("test.txt");

        // Make the file but don't tell the cache about it
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testPath));

        CheckForFile(testPath, false);
    }

    TEST_F(FileStateCacheTests, QueryAddedFile_ShouldExist)
    {
        QString testPath = m_temporarySourceDir.absoluteFilePath("test.txt");

        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testPath));

        m_fileStateCache->AddFile(testPath);

        CheckForFile(testPath, true);
    }

    TEST_F(FileStateCacheTests, QueryBulkAddedFile_ShouldExist)
    {
        QString testPath = m_temporarySourceDir.absoluteFilePath("test.txt");

        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testPath));

        QSet<AssetFileInfo> infoSet;
        AssetFileInfo fileInfo;
        fileInfo.m_filePath = testPath;
        fileInfo.m_isDirectory = false;
        fileInfo.m_fileSize = 0;
        fileInfo.m_modTime = QFileInfo(testPath).lastModified();
        infoSet.insert(fileInfo);

        m_fileStateCache->AddInfoSet(infoSet);

        CheckForFile(testPath, true);
    }

    TEST_F(FileStateCacheTests, QueryRemovedFile_ShouldNotExist)
    {
        QString testPath = m_temporarySourceDir.absoluteFilePath("test.txt");

        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testPath));

        m_fileStateCache->AddFile(testPath);
        m_fileStateCache->RemoveFile(testPath);

        CheckForFile(testPath, false);
    }

    TEST_F(FileStateCacheTests, AddAndRemoveFolder_ShouldAddAndRemoveSubFiles)
    {
        QDir testFolder = m_temporarySourceDir.absoluteFilePath("subfolder");
        QString testPath1 = testFolder.absoluteFilePath("test1.txt");
        QString testPath2 = testFolder.absoluteFilePath("test2.txt");

        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testPath1));
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testPath2));

        m_fileStateCache->AddFile(testFolder.absolutePath());

        CheckForFile(testPath1, true);
        CheckForFile(testPath2, true);

        m_fileStateCache->RemoveFile(testFolder.absolutePath());

        CheckForFile(testPath1, false);
        CheckForFile(testPath2, false);
    }

    TEST_F(FileStateCacheTests, UpdateFileAndQuery_ShouldExist)
    {
        QString testPath = m_temporarySourceDir.absoluteFilePath("test.txt");

        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testPath));

        QSet<AssetFileInfo> infoSet;
        AssetFileInfo fileInfo;
        fileInfo.m_filePath = testPath;
        fileInfo.m_isDirectory = false;
        fileInfo.m_fileSize = 1234; // Setting the file size to non-zero (even though the actual file is 0), UpdateFile should update this to 0 and allow CheckForFile to pass as a result
        fileInfo.m_modTime = QFileInfo(testPath).lastModified();
        infoSet.insert(fileInfo);

        m_fileStateCache->AddInfoSet(infoSet);
        m_fileStateCache->UpdateFile(testPath);

        CheckForFile(testPath, true);
    }

    TEST_F(FileStateCacheTests, PassthroughTest)
    {
        m_fileStateCache = nullptr; // Need to release the existing one first since only one handler can exist for the ebus
        m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStatePassthrough>();
        QString testPath = m_temporarySourceDir.absoluteFilePath("test.txt");

        CheckForFile(testPath, false);

        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testPath));

        CheckForFile(testPath, true);
    }

    TEST_F(FileStateCacheTests, HandlesMixedSeperators)
    {
        QSet<AssetFileInfo> infoSet;
        AssetFileInfo fileInfo;
        fileInfo.m_filePath = R"(c:\some/test\file.txt)";
        infoSet.insert(fileInfo);

        m_fileStateCache->AddInfoSet(infoSet);

        CheckForFile(R"(c:\some\test\file.txt)", true);
        CheckForFile(R"(c:/some/test/file.txt)", true);
    }
}
