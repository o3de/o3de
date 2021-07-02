/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        m_data.reset(new StaticData());
        m_data->m_databaseLocationListener.BusConnect();

        m_data->m_temporarySourceDir = QDir(m_data->m_temporaryDir.path());

        // in other unit tests we may open the database called ":memory:" to use an in-memory database instead of one on disk.
        // in this test, however, we use a real database, because the file processor shares it and opens its own connection to it.
        // ":memory:" databases are one-instance-only, and even if another connection is opened to ":memory:" it would
        // not share with others created using ":memory:" and get a unique database instead.
        m_data->m_databaseLocation = m_data->m_temporarySourceDir.absoluteFilePath("test_database.sqlite").toUtf8().constData();

        ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
            .WillByDefault(
                DoAll( // set the 0th argument ref (string) to the database location and return true.
                    SetArgReferee<0>(m_data->m_databaseLocation),
                    Return(true)));

        // Initialize the database:
        m_data->m_connection.ClearData(); // this is expected to reset/clear/reopen

        m_data->m_config = AZStd::make_unique<AssetProcessor::PlatformConfiguration>();
        m_data->m_config->EnablePlatform({ "pc", { "host", "renderer", "desktop" } }, true);
        
        m_data->m_fileProcessor = AZStd::make_unique<FileProcessor>(m_data->m_config.get());

        m_data->m_scanFolder = { m_data->m_temporarySourceDir.absolutePath().toUtf8().constData(), "dev", "rootportkey" };
        ASSERT_TRUE(m_data->m_connection.SetScanFolder(m_data->m_scanFolder));

        m_data->m_config->AddScanFolder(ScanFolderInfo(m_data->m_temporarySourceDir.absolutePath(), "dev", "rootportkey", false, true, m_data->m_config->GetEnabledPlatforms(), 0, m_data->m_scanFolder.m_scanFolderID));

        for (int index = 0; index < 10; ++index)
        {
            FileDatabaseEntry entry;
            entry.m_fileName = AZStd::string::format("somefile_%d.tif", index);
            entry.m_isFolder = false;
            entry.m_modTime = 0;
            entry.m_scanFolderPK = m_data->m_scanFolder.m_scanFolderID;
            m_data->m_fileEntries.push_back(entry);
        }
    }

    void FileProcessorTests::TearDown()
    {
        m_data->m_databaseLocationListener.BusDisconnect();
        m_data.reset();
        ConnectionBus::Handler::BusDisconnect(ConnectionBusId);
        AssetProcessorTest::TearDown();
    }

    size_t FileProcessorTests::Send([[maybe_unused]] unsigned int serial, [[maybe_unused]] const AzFramework::AssetSystem::BaseAssetProcessorMessage& message)
    {
        m_data->m_messagesSent++;

        return 0;
    }

    TEST_F(FileProcessorTests, FilesAdded_WhenSentMultipleAdds_ShouldEmitOnlyOneAdd)
    {
        QSet<AssetFileInfo> scannerFiles;

        m_data->m_fileProcessor->AssessAddedFile(m_data->m_temporarySourceDir.absoluteFilePath(m_data->m_fileEntries[0].m_fileName.c_str()));
        m_data->m_fileProcessor->AssessAddedFile(m_data->m_temporarySourceDir.absoluteFilePath(m_data->m_fileEntries[0].m_fileName.c_str()));

        ASSERT_EQ(m_data->m_messagesSent, 1);
    }

    TEST_F(FileProcessorTests, FilesFromScanner_ShouldSaveToDatabaseWithoutCreatingDuplicates)
    {
        QSet<AssetFileInfo> scannerFiles;
        auto* scanFolder = m_data->m_config->GetScanFolderByPath(m_data->m_scanFolder.m_scanFolder.c_str());

        ASSERT_NE(scanFolder, nullptr);

        for (const auto& file : m_data->m_fileEntries)
        {
            scannerFiles.insert(AssetFileInfo(m_data->m_temporarySourceDir.absoluteFilePath(file.m_fileName.c_str()), QDateTime::fromMSecsSinceEpoch(file.m_modTime), 1234, scanFolder, file.m_isFolder));
        }

        m_data->m_fileProcessor->AssessFilesFromScanner(scannerFiles);
        m_data->m_fileProcessor->Sync();

        // Run again to make sure we don't get duplicate entries
        m_data->m_fileProcessor->AssessFilesFromScanner(scannerFiles);
        m_data->m_fileProcessor->Sync();

        FileDatabaseEntryContainer actualEntries;

        auto filesFunction = [&actualEntries](AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry)
        {
            actualEntries.push_back(entry);
            return true;
        };
        
        ASSERT_TRUE(m_data->m_connection.QueryFilesTable(filesFunction));
        ASSERT_THAT(m_data->m_fileEntries, testing::UnorderedElementsAreArray(actualEntries));
    }

    TEST_F(FileProcessorTests, FilesFromScanner_ShouldHandleChangesBetweenSyncs)
    {
        QSet<AssetFileInfo> scannerFiles;
        auto* scanFolder = m_data->m_config->GetScanFolderByPath(m_data->m_scanFolder.m_scanFolder.c_str());

        ASSERT_NE(scanFolder, nullptr);

        for (const auto& file : m_data->m_fileEntries)
        {
            scannerFiles.insert(AssetFileInfo(m_data->m_temporarySourceDir.absoluteFilePath(file.m_fileName.c_str()), QDateTime::fromMSecsSinceEpoch(file.m_modTime), 1234, scanFolder, file.m_isFolder));
        }

        m_data->m_fileProcessor->AssessFilesFromScanner(scannerFiles);
        m_data->m_fileProcessor->Sync();

        FileDatabaseEntryContainer actualEntries;

        auto filesFunction = [&actualEntries](AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry)
        {
            actualEntries.push_back(entry);
            return true;
        };

        ASSERT_TRUE(m_data->m_connection.QueryFilesTable(filesFunction));
        ASSERT_THAT(m_data->m_fileEntries, testing::UnorderedElementsAreArray(actualEntries));

        // Clear the db (we don't have the file IDs in m_fileEntries to remove 1 by 1 so its easier to just remove them all)
        for (const auto& file : actualEntries)
        {
            m_data->m_connection.RemoveFile(file.m_fileID);
        }

        // Remove two files
        m_data->m_fileEntries.erase(m_data->m_fileEntries.begin());
        m_data->m_fileEntries.erase(m_data->m_fileEntries.begin());

        // Add a file
        FileDatabaseEntry entry;
        entry.m_fileName = AZStd::string::format("somefile_%d.tif", 11);
        entry.m_isFolder = false;
        entry.m_modTime = 0;
        entry.m_scanFolderPK = m_data->m_scanFolder.m_scanFolderID;
        m_data->m_fileEntries.push_back(entry);

        scannerFiles.clear();

        for (const auto& file : m_data->m_fileEntries)
        {
            scannerFiles.insert(AssetFileInfo(m_data->m_temporarySourceDir.absoluteFilePath(file.m_fileName.c_str()), QDateTime::fromMSecsSinceEpoch(file.m_modTime), 1234, scanFolder, file.m_isFolder));
        }
        
        // Sync again
        m_data->m_fileProcessor->AssessFilesFromScanner(scannerFiles);
        m_data->m_fileProcessor->Sync();

        actualEntries.clear();

        ASSERT_TRUE(m_data->m_connection.QueryFilesTable(filesFunction));
        ASSERT_THAT(m_data->m_fileEntries, testing::UnorderedElementsAreArray(actualEntries));
    }
}
