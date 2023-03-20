/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FileProcessorTests.h"

namespace UnitTests
{
    constexpr int ConnectionBusId = 0;

    void FileProcessorTests::SetUp()
    {
        AssetProcessorTest::SetUp();
        ConnectionBus::Handler::BusConnect(ConnectionBusId);

        m_databaseLocationListener.BusConnect();

        m_assetRootSourceDir = QDir(m_databaseLocationListener.GetAssetRootDir().c_str());

        // Initialize the database:
        m_connection.ClearData(); // this is expected to reset/clear/reopen

        m_config = AZStd::make_unique<AssetProcessor::PlatformConfiguration>();
        m_config->EnablePlatform({ "pc", { "host", "renderer", "desktop" } }, true);

        m_fileProcessor = AZStd::make_unique<FileProcessor>(m_config.get());

        m_scanFolder = { m_assetRootSourceDir.absoluteFilePath("dev").toUtf8().constData(), "dev", "rootportkey" };
        m_scanFolder2 = { m_assetRootSourceDir.absoluteFilePath("dev2").toUtf8().constData(), "dev2", "dev2" };
        ASSERT_TRUE(m_connection.SetScanFolder(m_scanFolder));
        ASSERT_TRUE(m_connection.SetScanFolder(m_scanFolder2));

        m_config->AddScanFolder(ScanFolderInfo(
            m_scanFolder.m_scanFolder.c_str(), m_scanFolder.m_displayName.c_str(), m_scanFolder.m_portableKey.c_str(),
            m_scanFolder.m_isRoot, true, m_config->GetEnabledPlatforms(), 0, m_scanFolder.m_scanFolderID));
        m_config->AddScanFolder(ScanFolderInfo(
            m_scanFolder2.m_scanFolder.c_str(), m_scanFolder2.m_displayName.c_str(), m_scanFolder2.m_portableKey.c_str(),
            m_scanFolder2.m_isRoot, true, m_config->GetEnabledPlatforms(), 0, m_scanFolder2.m_scanFolderID));

        for (int index = 0; index < 10; ++index)
        {
            FileDatabaseEntry entry;
            entry.m_fileName = AZStd::string::format("somefile_%d.tif", index);
            entry.m_isFolder = false;
            entry.m_modTime = 0;
            entry.m_scanFolderPK = m_scanFolder.m_scanFolderID;
            m_fileEntries.push_back(entry);
        }
    }

    void FileProcessorTests::TearDown()
    {
        m_databaseLocationListener.BusDisconnect();
        ConnectionBus::Handler::BusDisconnect(ConnectionBusId);
        AssetProcessorTest::TearDown();
    }

    size_t FileProcessorTests::Send([[maybe_unused]] unsigned int serial, [[maybe_unused]] const AzFramework::AssetSystem::BaseAssetProcessorMessage& message)
    {
        ++m_messagesSent;

        return 0;
    }

    TEST_F(FileProcessorTests, FilesAdded_WhenSentMultipleAdds_ShouldEmitOnlyOneAdd)
    {
        m_fileProcessor->AssessAddedFile((AZ::IO::Path(m_scanFolder.m_scanFolder) / m_fileEntries[0].m_fileName).c_str());
        m_fileProcessor->AssessAddedFile((AZ::IO::Path(m_scanFolder.m_scanFolder) / m_fileEntries[0].m_fileName).c_str());

        ASSERT_EQ(m_messagesSent, 1);
    }

    TEST_F(FileProcessorTests, FilesFromScanner_ShouldSaveToDatabaseWithoutCreatingDuplicates)
    {
        QSet<AssetFileInfo> scannerFiles;
        auto* scanFolder = m_config->GetScanFolderByPath(m_scanFolder.m_scanFolder.c_str());

        ASSERT_NE(scanFolder, nullptr);

        for (const auto& file : m_fileEntries)
        {
            scannerFiles.insert(AssetFileInfo((AZ::IO::Path(m_scanFolder.m_scanFolder) / file.m_fileName.c_str()).c_str(), QDateTime::fromMSecsSinceEpoch(file.m_modTime), 1234, scanFolder, file.m_isFolder));
        }

        m_fileProcessor->AssessFilesFromScanner(scannerFiles);
        m_fileProcessor->Sync();

        // Run again to make sure we don't get duplicate entries
        m_fileProcessor->AssessFilesFromScanner(scannerFiles);
        m_fileProcessor->Sync();

        FileDatabaseEntryContainer actualEntries;

        auto filesFunction = [&actualEntries](AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry)
        {
            actualEntries.push_back(entry);
            return true;
        };

        ASSERT_TRUE(m_connection.QueryFilesTable(filesFunction));
        ASSERT_THAT(m_fileEntries, testing::UnorderedElementsAreArray(actualEntries));
    }

    TEST_F(FileProcessorTests, IdenticalFilesInDifferentScanFolders_DeleteFolder_CorrectFilesRemoved)
    {
        auto* scanFolder = m_config->GetScanFolderByPath(m_scanFolder.m_scanFolder.c_str());
        auto* scanFolder2 = m_config->GetScanFolderByPath(m_scanFolder2.m_scanFolder.c_str());

        m_fileProcessor->AssessFoldersFromScanner({
            AssetFileInfo{ (AZ::IO::Path(m_scanFolder.m_scanFolder) / "folder").c_str(), QDateTime::currentDateTime(), 0, scanFolder, true },
            AssetFileInfo{ (AZ::IO::Path(m_scanFolder2.m_scanFolder) / "folder").c_str(), QDateTime::currentDateTime(), 0, scanFolder2, true }
        });

        m_fileProcessor->AssessFilesFromScanner({
            AssetFileInfo{ (AZ::IO::Path(m_scanFolder.m_scanFolder) / "folder" / "file.txt").c_str(), QDateTime::currentDateTime(), 0, scanFolder, true },
            AssetFileInfo{ (AZ::IO::Path(m_scanFolder2.m_scanFolder) / "folder" / "file.txt").c_str(), QDateTime::currentDateTime(), 0, scanFolder2, true }
        });

        m_fileProcessor->Sync();

        m_fileProcessor->AssessDeletedFile((AZ::IO::Path(m_scanFolder.m_scanFolder) / "folder").c_str());

        int fileCount = 0;

        m_connection.QueryFilesTable(
            [&fileCount](FileDatabaseEntry&)
            {
                ++fileCount;
                return true;
            });

        ASSERT_EQ(fileCount, 2); // 1 file, 1 folder
    }

    TEST_F(FileProcessorTests, FilesFromScanner_ShouldHandleChangesBetweenSyncs)
    {
        QSet<AssetFileInfo> scannerFiles;
        auto* scanFolder = m_config->GetScanFolderByPath(m_scanFolder.m_scanFolder.c_str());

        ASSERT_NE(scanFolder, nullptr);

        for (const auto& file : m_fileEntries)
        {
            scannerFiles.insert(AssetFileInfo((AZ::IO::Path(m_scanFolder.m_scanFolder) / file.m_fileName).c_str(), QDateTime::fromMSecsSinceEpoch(file.m_modTime), 1234, scanFolder, file.m_isFolder));
        }

        m_fileProcessor->AssessFilesFromScanner(scannerFiles);
        m_fileProcessor->Sync();

        FileDatabaseEntryContainer actualEntries;

        auto filesFunction = [&actualEntries](AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry)
        {
            actualEntries.push_back(entry);
            return true;
        };

        ASSERT_TRUE(m_connection.QueryFilesTable(filesFunction));
        ASSERT_THAT(m_fileEntries, testing::UnorderedElementsAreArray(actualEntries));

        // Clear the db (we don't have the file IDs in m_fileEntries to remove 1 by 1 so its easier to just remove them all)
        for (const auto& file : actualEntries)
        {
            m_connection.RemoveFile(file.m_fileID);
        }

        // Remove two files
        m_fileEntries.erase(m_fileEntries.begin());
        m_fileEntries.erase(m_fileEntries.begin());

        // Add a file
        FileDatabaseEntry entry;
        entry.m_fileName = AZStd::string::format("somefile_%d.tif", 11);
        entry.m_isFolder = false;
        entry.m_modTime = 0;
        entry.m_scanFolderPK = m_scanFolder.m_scanFolderID;
        m_fileEntries.push_back(entry);

        scannerFiles.clear();

        for (const auto& file : m_fileEntries)
        {
            scannerFiles.insert(AssetFileInfo((AZ::IO::Path(m_scanFolder.m_scanFolder) / file.m_fileName).c_str(), QDateTime::fromMSecsSinceEpoch(file.m_modTime), 1234, scanFolder, file.m_isFolder));
        }

        // Sync again
        m_fileProcessor->AssessFilesFromScanner(scannerFiles);
        m_fileProcessor->Sync();

        actualEntries.clear();

        ASSERT_TRUE(m_connection.QueryFilesTable(filesFunction));
        ASSERT_THAT(m_fileEntries, testing::UnorderedElementsAreArray(actualEntries));
    }
}
