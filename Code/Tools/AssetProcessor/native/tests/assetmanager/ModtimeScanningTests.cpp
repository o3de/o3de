/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/assetmanager/ModtimeScanningTests.h>
#include <native/tests/assetmanager/AssetProcessorManagerTest.h>
#include <QObject>
#include <ToolsFileUtils/ToolsFileUtils.h>

namespace UnitTests
{
    using AssetFileInfo = AssetProcessor::AssetFileInfo;

    void ModtimeScanningTest::SetUpAssetProcessorManager()
    {
        using namespace AssetProcessor;

        m_assetProcessorManager->SetEnableModtimeSkippingFeature(true);
        m_assetProcessorManager->RecomputeDirtyBuilders();

        QObject::connect(
            m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess,
            [this](JobDetails details)
            {
                m_data->m_processResults.push_back(AZStd::move(details));
            });

        QObject::connect(
            m_assetProcessorManager.get(), &AssetProcessorManager::SourceDeleted,
            [this](const SourceAssetReference& file)
            {
                m_data->m_deletedSources.push_back(file);
            });

        m_idleConnection = QObject::connect(
            m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState,
            [this](bool newState)
            {
                m_isIdling = newState;
            });
    }

    void ModtimeScanningTest::PopulateDatabase()
    {
    }

    void ModtimeScanningTest::SetUp()
    {
        using namespace AssetProcessor;

        AssetProcessorManagerTest::SetUp();

        m_data = AZStd::make_unique<StaticData>();

        // Create the test file
        const auto& scanFolder = m_config->GetScanFolderAt(1);
        m_data->m_sourcePaths.push_back(AssetProcessor::SourceAssetReference(scanFolder.ScanPath(), "modtimeTestFile.txt"));
        m_data->m_sourcePaths.push_back(AssetProcessor::SourceAssetReference(scanFolder.ScanPath(), "modtimeTestDependency.txt"));
        m_data->m_sourcePaths.push_back(AssetProcessor::SourceAssetReference(scanFolder.ScanPath(), "modtimeTestDependency.txt.assetinfo"));

        for (const auto& path : m_data->m_sourcePaths)
        {
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(path.AbsolutePath().c_str(), ""));
        }

        // We don't want the mock application manager to provide builder descriptors, mockBuilderInfoHandler will provide our own
        m_mockApplicationManager->BusDisconnect();

        m_data->m_mockBuilderInfoHandler.CreateBuilderDesc(
            "test builder", "{DF09DDC0-FD22-43B6-9E22-22C8574A6E1E}",
            { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) },
            MockMultiBuilderInfoHandler::AssetBuilderExtraInfo{ "", m_data->m_sourcePaths[1].AbsolutePath().c_str(), "", "", {} });
        m_data->m_mockBuilderInfoHandler.BusConnect();

        ASSERT_TRUE(m_mockApplicationManager->GetBuilderByID("txt files", m_data->m_builderTxtBuilder));

        SetUpAssetProcessorManager();

        // Add file to database with no modtime
        {
            AssetDatabaseConnection connection;
            ASSERT_TRUE(connection.OpenDatabase());
            AzToolsFramework::AssetDatabase::FileDatabaseEntry fileEntry;
            fileEntry.m_fileName = m_data->m_sourcePaths[0].RelativePath().Native();
            fileEntry.m_modTime = 0;
            fileEntry.m_isFolder = false;
            fileEntry.m_scanFolderPK = scanFolder.ScanFolderID();

            bool entryAlreadyExists;
            ASSERT_TRUE(connection.InsertFile(fileEntry, entryAlreadyExists));
            ASSERT_FALSE(entryAlreadyExists);

            fileEntry.m_fileID = AzToolsFramework::AssetDatabase::InvalidEntryId; // Reset the id so we make a new entry
            fileEntry.m_fileName = m_data->m_sourcePaths[1].RelativePath().Native();
            ASSERT_TRUE(connection.InsertFile(fileEntry, entryAlreadyExists));
            ASSERT_FALSE(entryAlreadyExists);

            fileEntry.m_fileID = AzToolsFramework::AssetDatabase::InvalidEntryId; // Reset the id so we make a new entry
            fileEntry.m_fileName = m_data->m_sourcePaths[2].RelativePath().Native();
            ASSERT_TRUE(connection.InsertFile(fileEntry, entryAlreadyExists));
            ASSERT_FALSE(entryAlreadyExists);
        }

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        ASSERT_TRUE(BlockUntilIdle(5000));
        ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 2);
        ASSERT_EQ(m_data->m_processResults.size(), 2);
        ASSERT_EQ(m_data->m_deletedSources.size(), 0);

        ProcessAssetJobs();

        m_data->m_processResults.clear();
        m_data->m_mockBuilderInfoHandler.m_createJobsCount = 0;

        m_isIdling = false;
    }

    void ModtimeScanningTest::TearDown()
    {
        m_data = nullptr;

        AssetProcessorManagerTest::TearDown();
    }

    void ModtimeScanningTest::ProcessAssetJobs()
    {
        m_data->m_productPaths.clear();

        for (const auto& processResult : m_data->m_processResults)
        {
            AZStd::string file = QString((processResult.m_jobEntry.m_sourceAssetReference.RelativePath().Native() + ".arc1").c_str()).toLower().toUtf8().constData();

            m_data->m_productPaths.emplace(processResult.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData(), (processResult.m_cachePath / file).c_str());

            // Create the file on disk
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile((processResult.m_cachePath / file).AsPosix().c_str(), "products."));

            AssetBuilderSDK::ProcessJobResponse response;
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct((processResult.m_relativePath / file).StringAsPosix(), AZ::Uuid::CreateNull(), 1));

            using JobEntry = AssetProcessor::JobEntry;

            QMetaObject::invokeMethod(
                m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResult.m_jobEntry),
                Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        ASSERT_TRUE(BlockUntilIdle(5000));

        m_isIdling = false;
    }

    void ModtimeScanningTest::SimulateAssetScanner(QSet<AssetProcessor::AssetFileInfo> filePaths)
    {
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "OnAssetScannerStatusChange", Qt::QueuedConnection,
            Q_ARG(AssetProcessor::AssetScanningStatus, AssetProcessor::AssetScanningStatus::Started));
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessFilesFromScanner", Qt::QueuedConnection, Q_ARG(QSet<AssetFileInfo>, filePaths));
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "OnAssetScannerStatusChange", Qt::QueuedConnection,
            Q_ARG(AssetProcessor::AssetScanningStatus, AssetProcessor::AssetScanningStatus::Completed));
    }

    QSet<AssetProcessor::AssetFileInfo> ModtimeScanningTest::BuildFileSet()
    {
        QSet<AssetFileInfo> filePaths;

        for (const auto& path : m_data->m_sourcePaths)
        {
            QFileInfo fileInfo(path.AbsolutePath().c_str());
            auto modtime = fileInfo.lastModified();
            AZ::u64 fileSize = fileInfo.size();
            filePaths.insert(AssetFileInfo(path.AbsolutePath().c_str(), modtime, fileSize, m_config->GetScanFolderForFile(path.AbsolutePath().c_str()), false));
        }

        return filePaths;
    }

    void ModtimeScanningTest::ExpectWork(int createJobs, int processJobs)
    {
        ASSERT_TRUE(BlockUntilIdle(5000));

        EXPECT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, createJobs);
        EXPECT_EQ(m_data->m_processResults.size(), processJobs);
        for (int i = 0; i < processJobs; ++i)
        {
            EXPECT_FALSE(m_data->m_processResults[i].m_autoFail);
        }
        EXPECT_EQ(m_data->m_deletedSources.size(), 0);

        m_isIdling = false;
    }

    void ModtimeScanningTest::ExpectNoWork()
    {
        // Since there's no work to do, the idle event isn't going to trigger, just process events a couple times
        for (int i = 0; i < 10; ++i)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }

        ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 0);
        ASSERT_EQ(m_data->m_processResults.size(), 0);
        ASSERT_EQ(m_data->m_deletedSources.size(), 0);

        m_isIdling = false;
    }

    void ModtimeScanningTest::SetFileContents(QString filePath, QString contents)
    {
        QFile file(filePath);
        file.open(QIODevice::WriteOnly | QIODevice::Truncate);
        file.write(contents.toUtf8().constData());
        file.close();
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_FileUnchanged_WithoutModtimeSkipping)
    {
        using namespace AzToolsFramework::AssetSystem;

        // Make sure modtime skipping is disabled
        // We're just going to do 1 quick sanity test to make sure the files are still processed when modtime skipping is turned off
        m_assetProcessorManager->SetEnableModtimeSkippingFeature(false);

        QSet<AssetProcessor::AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        // 2 create jobs but 0 process jobs because the file has already been processed before in SetUp
        ExpectWork(2, 0);
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_FileUnchanged)
    {
        using namespace AzToolsFramework::AssetSystem;

        AssetUtilities::SetUseFileHashOverride(true, true);

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        ExpectNoWork();
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_EnablePlatform_ShouldProcessFilesForPlatform)
    {
        using namespace AzToolsFramework::AssetSystem;

        AssetUtilities::SetUseFileHashOverride(true, true);

        // Enable android platform after the initial SetUp has already processed the files for pc
        AssetBuilderSDK::PlatformInfo androidPlatform("android", { "host", "renderer" });
        m_config->EnablePlatform(androidPlatform, true);

        // There's no way to remove scanfolders and adding a new one after enabling the platform will cause the pc assets to build as well,
        // which we don't want Instead we'll just const cast the vector and modify the enabled platforms for the scanfolder
        auto& platforms = const_cast<AZStd::vector<AssetBuilderSDK::PlatformInfo>&>(m_config->GetScanFolderAt(1).GetPlatforms());
        platforms.push_back(androidPlatform);

        // We need the builder fingerprints to be updated to reflect the newly enabled platform
        m_assetProcessorManager->ComputeBuilderDirty();

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        ExpectWork(
            4, 2); // CreateJobs = 4, 2 files * 2 platforms.  ProcessJobs = 2, just the android platform jobs (pc is already processed)

        ASSERT_TRUE(m_data->m_processResults[0].m_cachePath.Filename() == "android");
        ASSERT_TRUE(m_data->m_processResults[1].m_cachePath.Filename() == "android");
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyTimestamp)
    {
        // Update the timestamp on a file without changing its contents
        // This should not cause any job to run since the hash of the file is the same before/after
        // Additionally, the timestamp stored in the database should be updated
        using namespace AzToolsFramework::AssetSystem;

        uint64_t timestamp = 1594923423;

        const auto& sourcePath = m_data->m_sourcePaths[1];

        AzToolsFramework::AssetDatabase::FileDatabaseEntry fileEntry;

        m_assetProcessorManager->m_stateData->GetFileByFileNameAndScanFolderId(sourcePath.RelativePath().c_str(), sourcePath.ScanFolderId(), fileEntry);

        ASSERT_NE(fileEntry.m_modTime, timestamp);
        uint64_t existingTimestamp = fileEntry.m_modTime;

        // Modify the timestamp on just one file
        AzToolsFramework::ToolsFileUtils::SetModificationTime(sourcePath.AbsolutePath().c_str(), timestamp);

        AssetUtilities::SetUseFileHashOverride(true, true);

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        ExpectNoWork();

        m_assetProcessorManager->m_stateData->GetFileByFileNameAndScanFolderId(sourcePath.RelativePath().c_str(), sourcePath.ScanFolderId(), fileEntry);

        // The timestamp should be updated even though nothing processed
        ASSERT_NE(fileEntry.m_modTime, existingTimestamp);
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyTimestampNoHashing_ProcessesFile)
    {
        // Update the timestamp on a file without changing its contents
        // This should not cause any job to run since the hash of the file is the same before/after
        // Additionally, the timestamp stored in the database should be updated
        using namespace AzToolsFramework::AssetSystem;

        uint64_t timestamp = 1594923423;

        // Modify the timestamp on just one file
        AzToolsFramework::ToolsFileUtils::SetModificationTime(m_data->m_sourcePaths[1].AbsolutePath().c_str(), timestamp);

        AssetUtilities::SetUseFileHashOverride(true, false);

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        ExpectWork(2, 2);
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyFile)
    {
        using namespace AzToolsFramework::AssetSystem;

        SetFileContents(m_data->m_sourcePaths[1].AbsolutePath().c_str(), "hello world");

        AssetUtilities::SetUseFileHashOverride(true, true);

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        // Even though we're only updating one file, we're expecting 2 createJob calls because our test file is a dependency that triggers
        // the other test file to process as well
        ExpectWork(2, 2);
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyFile_AndThenRevert_ProcessesAgain)
    {
        using namespace AzToolsFramework::AssetSystem;
        auto theFile = m_data->m_sourcePaths[1].AbsolutePath().Native();
        const char* theFileString = theFile.c_str();

        SetFileContents(theFileString, "hello world");

        AssetUtilities::SetUseFileHashOverride(true, true);

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        // Even though we're only updating one file, we're expecting 2 createJob calls because our test file is a dependency that triggers
        // the other test file to process as well
        ExpectWork(2, 2);
        ProcessAssetJobs();

        m_data->m_mockBuilderInfoHandler.m_createJobsCount = 0;
        m_data->m_processResults.clear();
        m_data->m_deletedSources.clear();

        SetFileContents(theFileString, "");

        filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        // Expect processing to happen again
        ExpectWork(2, 2);
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyFilesSameHash_BothProcess)
    {
        using namespace AzToolsFramework::AssetSystem;

        SetFileContents(m_data->m_sourcePaths[1].AbsolutePath().c_str(), "hello world");

        AssetUtilities::SetUseFileHashOverride(true, true);

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        // Even though we're only updating one file, we're expecting 2 createJob calls because our test file is a dependency that triggers
        // the other test file to process as well
        ExpectWork(2, 2);
        ProcessAssetJobs();

        m_data->m_mockBuilderInfoHandler.m_createJobsCount = 0;
        m_data->m_processResults.clear();
        m_data->m_deletedSources.clear();

        // Make file 0 have the same contents as file 1
        SetFileContents(m_data->m_sourcePaths[0].AbsolutePath().c_str(), "hello world");

        filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        ExpectWork(1, 1);
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyMetadataFile)
    {
        using namespace AzToolsFramework::AssetSystem;

        SetFileContents(m_data->m_sourcePaths[2].AbsolutePath().c_str(), "hello world");

        AssetUtilities::SetUseFileHashOverride(true, true);

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        // Even though we're only updating one file, we're expecting 2 createJob calls because our test file is a metadata file
        // that triggers the source file which is a dependency that triggers the other test file to process as well
        ExpectWork(2, 2);
    }

    TEST_F(ModtimeScanningTest, ModtimeSkipping_DeleteFile)
    {
        using namespace AzToolsFramework::AssetSystem;

        AssetUtilities::SetUseFileHashOverride(true, true);

        ASSERT_TRUE(QFile::remove(m_data->m_sourcePaths[0].AbsolutePath().c_str()));

        // Feed in ONLY one file (the one we didn't delete)
        QSet<AssetFileInfo> filePaths;
        QFileInfo fileInfo(m_data->m_sourcePaths[1].AbsolutePath().c_str());
        auto modtime = fileInfo.lastModified();
        AZ::u64 fileSize = fileInfo.size();
        filePaths.insert(AssetFileInfo(m_data->m_sourcePaths[1].AbsolutePath().c_str(), modtime, fileSize, &m_config->GetScanFolderAt(0), false));

        SimulateAssetScanner(filePaths);

        QElapsedTimer timer;
        timer.start();

        do
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        } while (m_data->m_deletedSources.empty() && timer.elapsed() < 5000);

        ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 0);
        ASSERT_EQ(m_data->m_processResults.size(), 0);
        ASSERT_THAT(m_data->m_deletedSources, testing::ElementsAre(m_data->m_sourcePaths[0]));
    }

    TEST_F(ModtimeScanningTest, ReprocessRequest_FileNotModified_FileProcessed)
    {
        using namespace AzToolsFramework::AssetSystem;

        m_assetProcessorManager->RequestReprocess(m_data->m_sourcePaths[0].AbsolutePath().c_str());

        ASSERT_TRUE(BlockUntilIdle(5000));

        ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 1);
        ASSERT_EQ(m_data->m_processResults.size(), 1);
    }

    TEST_F(ModtimeScanningTest, ReprocessRequest_SourceWithDependency_BothWillProcess)
    {
        using namespace AzToolsFramework::AssetSystem;
        using namespace AzToolsFramework::AssetDatabase;

        SourceFileDependencyEntry newEntry1;
        newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
        newEntry1.m_sourceGuid = AZ::Uuid{ "{C0BD819A-F84E-4A56-A6A5-917AE3ECDE53}" };
        newEntry1.m_dependsOnSource = PathOrUuid(m_data->m_sourcePaths[1].AbsolutePath().c_str());
        newEntry1.m_typeOfDependency = SourceFileDependencyEntry::DEP_SourceToSource;

        m_assetProcessorManager->RequestReprocess(m_data->m_sourcePaths[0].AbsolutePath().c_str());
        ASSERT_TRUE(BlockUntilIdle(5000));

        ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 1);
        ASSERT_EQ(m_data->m_processResults.size(), 1);

        m_assetProcessorManager->RequestReprocess(m_data->m_sourcePaths[1].AbsolutePath().c_str());
        ASSERT_TRUE(BlockUntilIdle(5000));

        ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 3);
        ASSERT_EQ(m_data->m_processResults.size(), 3);
    }

    TEST_F(ModtimeScanningTest, ReprocessRequest_RequestFolder_SourceAssetsWillProcess)
    {
        using namespace AzToolsFramework::AssetSystem;

        const auto& scanFolder = m_config->GetScanFolderAt(1);

        QString scanPath = scanFolder.ScanPath();
        m_assetProcessorManager->RequestReprocess(scanPath);
        ASSERT_TRUE(BlockUntilIdle(5000));

        // two text files are source assets, assetinfo is not
        ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 2);
        ASSERT_EQ(m_data->m_processResults.size(), 2);
    }

     TEST_F(ModtimeScanningTest, AssetProcessorIsRestartedBeforeDependencyIsProcessed_DependencyIsProcessedOnStart)
    {
        using namespace AzToolsFramework::AssetSystem;
        auto theFile = m_data->m_sourcePaths[1].AbsolutePath();
        const char* theFileString = theFile.c_str();

        SetFileContents(theFileString, "hello world");

        // Enable the features we're testing
        m_assetProcessorManager->SetEnableModtimeSkippingFeature(true);
        AssetUtilities::SetUseFileHashOverride(true, true);

        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        // Even though we're only updating one file, we're expecting 2 createJob calls because our test file is a dependency that triggers
        // the other test file to process as well
        ExpectWork(2, 2);

        // Sort the results and process the first one, which should always be the modtimeTestDependency.txt file
        // which is the same file we modified above.  modtimeTestFile.txt depends on this file but we're not going to process it yet.
        {
            std::sort(
                m_data->m_processResults.begin(), m_data->m_processResults.end(),
                [](decltype(m_data->m_processResults[0])& left, decltype(left)& right)
                {
                    return left.m_jobEntry.m_sourceAssetReference < right.m_jobEntry.m_sourceAssetReference;
                });

            const auto& processResult = m_data->m_processResults[0];
            AZStd::string file = QString((processResult.m_jobEntry.m_sourceAssetReference.RelativePath().Native() + ".arc1").c_str()).toLower().toUtf8().constData();

            m_data->m_productPaths.emplace(processResult.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData(), (processResult.m_cachePath / file).c_str());

            // Create the file on disk
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile((processResult.m_cachePath / file).AsPosix().c_str(), "products."));

            AssetBuilderSDK::ProcessJobResponse response;
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(file, AZ::Uuid::CreateNull(), 1));

            using JobEntry = AssetProcessor::JobEntry;

            QMetaObject::invokeMethod(
                m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResult.m_jobEntry),
                Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        ASSERT_TRUE(BlockUntilIdle(5000));

        // Shutdown and restart the APM
        m_assetProcessorManager.reset();
        m_assetProcessorManager = AZStd::make_unique<AssetProcessorManager_Test>(m_config.get());

        SetUpAssetProcessorManager();

        m_data->m_mockBuilderInfoHandler.m_createJobsCount = 0;
        m_data->m_processResults.clear();
        m_data->m_deletedSources.clear();

        // Re-run the scanner on our files
        filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        // Expect processing to resume on the job we didn't process before
        ExpectWork(1, 1);
    }

    void DeleteTest::SetUp()
    {
        AssetProcessorManagerTest::SetUp();

        m_data = AZStd::make_unique<StaticData>();

        // We don't want the mock application manager to provide builder descriptors, mockBuilderInfoHandler will provide our own
        m_mockApplicationManager->BusDisconnect();

        m_data->m_mockBuilderInfoHandler.CreateBuilderDesc(
            "test builder", "{DF09DDC0-FD22-43B6-9E22-22C8574A6E1E}",
            { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) }, {});
        m_data->m_mockBuilderInfoHandler.BusConnect();

        ASSERT_TRUE(m_mockApplicationManager->GetBuilderByID("txt files", m_data->m_builderTxtBuilder));

        SetUpAssetProcessorManager();

        m_data->m_sourcePaths.clear();

        auto createFileAndAddToDatabaseFunc = [this](const AssetProcessor::ScanFolderInfo* scanFolder, QString file)
        {
            using namespace AzToolsFramework::AssetDatabase;

            QString watchFolderPath = scanFolder->ScanPath();
            QString absPath(QDir(watchFolderPath).absoluteFilePath(file));
            UnitTestUtils::CreateDummyFile(absPath);

            m_data->m_sourcePaths.push_back(AssetProcessor::SourceAssetReference(absPath));

            AzToolsFramework::AssetDatabase::FileDatabaseEntry fileEntry;
            fileEntry.m_fileName = file.toUtf8().constData();
            fileEntry.m_modTime = 0;
            fileEntry.m_isFolder = false;
            fileEntry.m_scanFolderPK = scanFolder->ScanFolderID();

            bool entryAlreadyExists;
            ASSERT_TRUE(m_assetProcessorManager->m_stateData->InsertFile(fileEntry, entryAlreadyExists));
            ASSERT_FALSE(entryAlreadyExists);
        };

        // Create test files
        QDir assetRootPath(m_assetRootDir);
        const auto* scanFolder1 = m_config->GetScanFolderByPath(assetRootPath.absoluteFilePath("subfolder1"));
        const auto* scanFolder4 = m_config->GetScanFolderByPath(assetRootPath.absoluteFilePath("subfolder4"));

        createFileAndAddToDatabaseFunc(scanFolder1, QString("textures/a.txt"));
        createFileAndAddToDatabaseFunc(scanFolder4, QString("textures/b.txt"));

        // Run the test files through AP all the way to processing stage
        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        ASSERT_TRUE(BlockUntilIdle(5000));
        ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 2);
        ASSERT_EQ(m_data->m_processResults.size(), 2);
        ASSERT_EQ(m_data->m_deletedSources.size(), 0);

        ProcessAssetJobs();

        m_data->m_processResults.clear();
        m_data->m_mockBuilderInfoHandler.m_createJobsCount = 0;

        // Reboot the APM since we added stuff to the database that needs to be loaded on-startup of the APM
        m_assetProcessorManager = nullptr; // Destroy the old instance first so everything can destruct before we construct a new instance
        m_assetProcessorManager.reset(new AssetProcessorManager_Test(m_config.get()));

        SetUpAssetProcessorManager();
    }

    TEST_F(DeleteTest, DeleteFolderSharedAcrossTwoScanFolders_CorrectFileAndFolderAreDeletedFromCache)
    {
        // There was a bug where AP wasn't repopulating the "known folders" list when modtime skipping was enabled and no work was needed
        // As a result, deleting a folder didn't count as a "folder", so the wrong code path was taken.  This test makes sure the correct
        // deletion events fire

        using namespace AzToolsFramework::AssetSystem;

        // Feed in the files from the asset scanner, no jobs should run since they're already up-to-date
        QSet<AssetFileInfo> filePaths = BuildFileSet();
        SimulateAssetScanner(filePaths);

        ExpectNoWork();

        // Delete one of the folders
        QDir assetRootPath(m_assetRootDir);
        QString absPath(assetRootPath.absoluteFilePath("subfolder1/textures"));
        QDir(absPath).removeRecursively();

        AZStd::vector<AZStd::string> deletedFolders;
        QObject::connect(
            m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::SourceFolderDeleted,
            [&deletedFolders](QString file)
            {
                deletedFolders.push_back(file.toUtf8().constData());
            });

        m_assetProcessorManager->AssessDeletedFile(absPath);
        ASSERT_TRUE(BlockUntilIdle(5000));

        ASSERT_THAT(m_data->m_deletedSources, testing::UnorderedElementsAre(AssetProcessor::SourceAssetReference(m_assetRootDir.absoluteFilePath("subfolder1"), "textures/a.txt")));
        ASSERT_THAT(deletedFolders, testing::UnorderedElementsAre(absPath.toUtf8().constData()));
    }

    TEST_F(LockedFileTest, DeleteFile_LockedProduct_DeleteFails)
    {
        auto theFile = m_data->m_sourcePaths[1].AbsolutePath();
        const char* theFileString = theFile.c_str();
        auto [sourcePath, productPath] = *m_data->m_productPaths.find(theFileString);

        {
            QFile file(theFileString);
            file.remove();
        }

        ASSERT_GT(m_data->m_productPaths.size(), 0);
        QFile product(productPath);

        ASSERT_TRUE(product.open(QIODevice::ReadOnly));

        // Check if we can delete the file now, if we can't, proceed with the test
        // If we can, it means the OS running this test doesn't lock open files so there's nothing to test
        if (!AZ::IO::SystemFile::Delete(productPath.toUtf8().constData()))
        {
            QMetaObject::invokeMethod(
                m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, QString(theFileString)));

            EXPECT_TRUE(BlockUntilIdle(5000));

            EXPECT_TRUE(QFile::exists(productPath));
            EXPECT_EQ(m_data->m_deletedSources.size(), 0);
        }
        else
        {
            SUCCEED() << "Skipping test.  OS does not lock open files.";
        }
    }

    TEST_F(LockedFileTest, DeleteFile_LockedProduct_DeletesWhenReleased)
    {
        // This test is intended to verify the AP will successfully retry deleting a source asset
        // when one of its product assets is locked temporarily
        // We'll lock the file by holding it open

        auto theFile = m_data->m_sourcePaths[1].AbsolutePath();
        const char* theFileString = theFile.c_str();
        auto [sourcePath, productPath] = *m_data->m_productPaths.find(theFileString);

        {
            QFile file(theFileString);
            file.remove();
        }

        ASSERT_GT(m_data->m_productPaths.size(), 0);
        QFile product(productPath);

        // Open the file and keep it open to lock it
        // We'll start a thread later to unlock the file
        // This will allow us to test how AP handles trying to delete a locked file
        ASSERT_TRUE(product.open(QIODevice::ReadOnly));

        // Check if we can delete the file now, if we can't, proceed with the test
        // If we can, it means the OS running this test doesn't lock open files so there's nothing to test
        if (!AZ::IO::SystemFile::Delete(productPath.toUtf8().constData()))
        {
            m_deleteCounter = 0;

            // Set up a callback which will fire after at least 1 retry
            // Unlock the file at that point so AP can successfully delete it
            m_callback = [&product]()
            {
                product.close();
            };

            QMetaObject::invokeMethod(
                m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, QString(theFileString)));

            EXPECT_TRUE(BlockUntilIdle(5000));

            EXPECT_FALSE(QFile::exists(productPath));
            EXPECT_EQ(m_data->m_deletedSources.size(), 1);

            EXPECT_GT(m_deleteCounter, 1); // Make sure the AP tried more than once to delete the file
            m_errorAbsorber->ExpectAsserts(0);
        }
        else
        {
            SUCCEED() << "Skipping test.  OS does not lock open files.";
        }
    }
}
