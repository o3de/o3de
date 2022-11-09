/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzTest/AzTest.h>

#include <native/tests/AssetProcessorTest.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <native/unittests/AssetProcessorManagerUnitTests.h>
#include <native/unittests/MockApplicationManager.h>
#include <native/unittests/MockConnectionHandler.h>

#include <QCoreApplication>

namespace AssetProcessor
{
    using namespace UnitTestUtils;
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzToolsFramework::AssetDatabase;

    class AssetProcessorManager_Test
        : public AssetProcessorManager
    {
    public:
        explicit AssetProcessorManager_Test(PlatformConfiguration* config, QObject* parent = 0)
            : AssetProcessorManager(config, parent)
        {}

        friend class AssetProcessorManagerUnitTests;

    public:
        using GetRelativeProductPathFromFullSourceOrProductPathRequest = AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathRequest;
        using GetRelativeProductPathFromFullSourceOrProductPathResponse = AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathResponse;
        using GenerateRelativeSourcePathRequest = AzFramework::AssetSystem::GenerateRelativeSourcePathRequest;
        using GenerateRelativeSourcePathResponse = AzFramework::AssetSystem::GenerateRelativeSourcePathResponse;
        using GetFullSourcePathFromRelativeProductPathRequest = AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathRequest;
        using GetFullSourcePathFromRelativeProductPathResponse = AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathResponse;
    };

    AssetProcessorManagerUnitTests::~AssetProcessorManagerUnitTests()
    {
    }

    void AssetProcessorManagerUnitTests::SetUp()
    {
        UnitTest::AssetProcessorUnitTestBase::SetUp();

        m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStatePassthrough>();

        // update the engine root
        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        m_sourceRoot = QDir(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
        QString canonicalAssetRootDirPath = AssetUtilities::NormalizeDirectoryPath(m_sourceRoot.canonicalPath());
        m_changeDir = AZStd::make_unique<UnitTestUtils::ScopedDir>(canonicalAssetRootDirPath);

        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &m_sourceRoot);

        // create a dummy file in the cache folder, so the folder structure gets created
        // Override the cache folder to be the within the asset root directory
        auto projectCacheRootKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/project_cache_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Set(projectCacheRootKey, m_sourceRoot.absoluteFilePath("Cache").toUtf8().data());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*settingsRegistry);
        }
        EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(m_cacheRoot));
        CreateDummyFile(m_cacheRoot.absoluteFilePath("placeholder.txt"));

        // make sure it picked up the one in the cache and not for example the real working folder
        QString normalizedCacheRootPath = AssetUtilities::NormalizeDirectoryPath(m_cacheRoot.canonicalPath());
        QString normalizedDirPathCheck = AssetUtilities::NormalizeDirectoryPath(QDir(canonicalAssetRootDirPath).absoluteFilePath("Cache"));
        EXPECT_EQ(normalizedCacheRootPath, normalizedDirPathCheck);
        m_cacheRoot = QDir(normalizedCacheRootPath);

        constexpr const char* AssetProcessorManagerTestGameProject = "AutomatedTesting";
        QString gameName = AssetUtilities::ComputeProjectName(AssetProcessorManagerTestGameProject);
        EXPECT_FALSE(gameName.isEmpty());

        m_config.EnablePlatform({ "pc",{ "desktop", "renderer" } }, true);
        m_config.EnablePlatform({ "android",{ "mobile", "renderer" } }, true);
        m_config.EnablePlatform({ "fandago",{ "console", "renderer" } }, false);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse  platforms   order
        m_config.AddScanFolder(ScanFolderInfo(m_sourceRoot.filePath("subfolder4"), "subfolder4", "subfolder4", false, false, platforms, -6)); // subfolder 4 overrides subfolder3
        m_config.AddScanFolder(ScanFolderInfo(m_sourceRoot.filePath("subfolder3"), "subfolder3", "subfolder3", false, false, platforms,-5)); // subfolder 3 overrides subfolder2
        m_config.AddScanFolder(ScanFolderInfo(m_sourceRoot.filePath("subfolder2"), "subfolder2", "subfolder2", false, true,  platforms, -2)); // subfolder 2 overrides subfolder1
        m_config.AddScanFolder(ScanFolderInfo(m_sourceRoot.filePath("subfolder1"), "subfolder1", "subfolder1", false, true,  platforms, -1)); // subfolder1 overrides root
        m_config.AddScanFolder(ScanFolderInfo(m_sourceRoot.absolutePath(),         "root",       "rootfolder", true, false,  platforms, 0)); // add the root

        m_config.AddIntermediateScanFolder();

        m_config.AddMetaDataType("exportsettings", QString());

        // Configure asset processor manager
        m_assetProcessorManager = AZStd::make_unique<AssetProcessorManager_Test>(&m_config);  // note, this will 'push' the scan folders in to the db.

        m_assetProcessorConnections.append(connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess,
            this, [&](JobDetails details)
            {
                m_processResults.push_back(AZStd::move(details));
            }));

        m_assetProcessorConnections.append(connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetMessage,
            this, [&](AzFramework::AssetSystem::AssetNotificationMessage message)
            {
                m_assetMessages.push_back( message);
            }));

        m_assetProcessorConnections.append(connect(m_assetProcessorManager.get(), &AssetProcessorManager::InputAssetProcessed,
            this, [&](QString relativePath, QString platform)
            {
                m_changedInputResults.push_back(QPair<QString, QString>(relativePath, platform));
            }));

        m_assetProcessorConnections.append(connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetProcessorManagerIdleState,
            this, [&](bool state)
            {
                m_idling = state;
            }));
    }

    void AssetProcessorManagerUnitTests::TearDown()
    {
        // Stop file watching, disconnect everything and process all events so nothing gets called after the method finishes
        m_fileWatcher.StopWatching();

        for (const QMetaObject::Connection& connection : m_assetProcessorConnections)
        {
            QObject::disconnect(connection);
        }        

        QCoreApplication::processEvents(QEventLoop::AllEvents);

        m_assetProcessorManager.reset();
        m_changeDir.reset();
        m_fileStateCache.reset();

        UnitTest::AssetProcessorUnitTestBase::TearDown();
    }

    // Takes an absolute cache path and returns the portion after cache/platform/
    AZStd::string AssetProcessorManagerUnitTests::AbsProductPathToRelative(const QString& absolutePath)
    {
        AZ::IO::Path platformRelativePath = absolutePath.toUtf8().constData();
        platformRelativePath = platformRelativePath.LexicallyRelative(m_cacheRoot.absolutePath().toUtf8().constData());

        return (*++platformRelativePath.begin()).StringAsPosix();
    };

    void AssetProcessorManagerUnitTests::VerifyProductPaths(const JobDetails& jobDetails)
    {
        QString platformFolder = m_cacheRoot.filePath(QString::fromUtf8(jobDetails.m_jobEntry.m_platformInfo.m_identifier.c_str()));
        platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
        AZ::IO::Path expectedCachePath = m_cacheRoot.absoluteFilePath(platformFolder).toUtf8().constData();
        AZ::IO::FixedMaxPath intermediateAssetsFolder = AssetUtilities::GetIntermediateAssetsFolder(m_cacheRoot.absolutePath().toUtf8().constData());

        EXPECT_EQ(jobDetails.m_cachePath, expectedCachePath);
        EXPECT_EQ(jobDetails.m_intermediatePath, intermediateAssetsFolder);
    };

    namespace AssetProcessorManagerUnitTestUtils
    {
        class MockAssetBuilderInfoHandler
            : public AssetProcessor::AssetBuilderInfoBus::Handler
        {
        public:
            // AssetProcessor::AssetBuilderInfoBus::Handler
            void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
            {
                AZ_UNUSED(assetPath);
                builderInfoList.push_back(m_assetBuilderDesc);
            }

            void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList)
            {
                builderInfoList.push_back(m_assetBuilderDesc);
            }

            AssetBuilderSDK::AssetBuilderDesc m_assetBuilderDesc;
        };

        void CreateExpectedFiles(const QSet<QString>& expectedFiles)
        {
            QDateTime fileTime = QDateTime::currentDateTime();
            for (const QString& expect : expectedFiles)
            {
                EXPECT_TRUE(CreateDummyFile(expect));

                // Set a different timestamp for each file.
                QFile file(expect);
                ASSERT_TRUE(file.open(QIODevice::Append | QIODevice::Text))
                    << AZStd::string::format("Failed to open %s", expect.toUtf8().data()).c_str();
                EXPECT_TRUE(file.setFileTime(fileTime, QFileDevice::FileModificationTime))
                    << AZStd::string::format("Failed to modify the creation time of %s", expect.toUtf8().data()).c_str();
                file.close();

                //Add 2 seconds to the next file timestamp since the file time resolution is one second on platforms other than Windows. 
                fileTime = fileTime.addSecs(2);
            }
        }

        /// This functions sorts the processed result list by platform name
        /// if platform is same than it sorts by job description
        void SortAssetToProcessResultList(QList<JobDetails>& processResults)
        {
            //Sort the processResults based on platforms
            std::sort(processResults.begin(), processResults.end(),
                [](const JobDetails& first, const JobDetails& second)
                {
                    if (first.m_jobEntry.m_platformInfo.m_identifier == second.m_jobEntry.m_platformInfo.m_identifier)
                    {
                        return first.m_jobEntry.m_jobKey.toLower() < second.m_jobEntry.m_jobKey.toLower();
                    }

                    return first.m_jobEntry.m_platformInfo.m_identifier < second.m_jobEntry.m_platformInfo.m_identifier;
                });
        }

        void ComputeFingerprints(unsigned int& fingerprintForPC, unsigned int& fingerprintForANDROID, PlatformConfiguration& config, QString scanFolderPath, QString relPath)
        {
            QString extraInfoForPC;
            QString extraInfoForANDROID;
            RecognizerPointerContainer output;
            QString filePath = scanFolderPath + "/" + relPath;
            config.GetMatchingRecognizers(filePath, output);
            for (const AssetRecognizer* assetRecogniser : output)
            {
                extraInfoForPC.append(assetRecogniser->m_platformSpecs.at("pc") == AssetInternalSpec::Copy ? "copy" : "skip");
                extraInfoForANDROID.append(assetRecogniser->m_platformSpecs.at("android") == AssetInternalSpec::Copy ? "copy" : "skip");
                extraInfoForPC.append(assetRecogniser->m_version.c_str());
                extraInfoForANDROID.append(assetRecogniser->m_version.c_str());
            }

            //Calculating fingerprints for the file for pc and android platforms
            AZ::Uuid sourceId = AZ::Uuid("{2206A6E0-FDBC-45DE-B6FE-C2FC63020BD5}");
            JobEntry jobEntryPC(SourceAssetReference(scanFolderPath, relPath), {}, { "pc", {"desktop", "renderer"} }, "", 0, 1, sourceId);
            JobEntry jobEntryANDROID(SourceAssetReference(scanFolderPath, relPath), {}, { "android", {"mobile", "renderer"} }, "", 0, 2, sourceId);

            JobDetails jobDetailsPC;
            jobDetailsPC.m_extraInformationForFingerprinting = extraInfoForPC.toUtf8().constData();
            jobDetailsPC.m_jobEntry = jobEntryPC;
            JobDetails jobDetailsANDROID;
            jobDetailsANDROID.m_extraInformationForFingerprinting = extraInfoForANDROID.toUtf8().constData();
            jobDetailsANDROID.m_jobEntry = jobEntryANDROID;
            fingerprintForPC = AssetUtilities::GenerateFingerprint(jobDetailsPC);
            fingerprintForANDROID = AssetUtilities::GenerateFingerprint(jobDetailsANDROID);
        }
    }

    // the asset processor manager is generally sitting on top of many other systems.
    // we have tested those systems individually in other unit tests, but we need to create
    // a simulated environment to test the manager itself.
    // for the manager, the only things we care about is that it emits the correct signals
    // when the appropriate stimulus is given and that state is appropriately updated.

    TEST_F(AssetProcessorManagerUnitTests, SkipProcessing_FeedFilesToIgnore_NoTasksGenerated)
    {
        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        // txt recognizer
        AssetRecognizer rec;
        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // Ignore recognizer
        AssetRecognizer ignore_rec;
        ignore_rec.m_name = "ignore files";
        ignore_rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ignore", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        ignore_rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        ignore_rec.m_platformSpecs.insert({"android", AssetInternalSpec::Skip});
        m_config.AddRecognizer(ignore_rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(ignore_rec);
     
        QSet<QString> expectedFiles;
        // subfolder3 is not recursive so none of these should show up in any scan or override check
        expectedFiles << m_sourceRoot.absoluteFilePath("subfolder3/aaa/basefile.txt");
        expectedFiles << m_sourceRoot.absoluteFilePath("subfolder3/uniquefile.ignore"); // only exists in subfolder3
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles(expectedFiles);

        // the following is a file which does exist but should not be processed as it is in a non-watched folder (not recursive)
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, m_sourceRoot.absoluteFilePath("subfolder3/aaa/basefile.txt")));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        EXPECT_TRUE(m_processResults.isEmpty());
        EXPECT_TRUE(m_changedInputResults.isEmpty());
        EXPECT_TRUE(m_assetMessages.isEmpty());

        // an imaginary non-existent file should also fail even if it matches filters:
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, m_sourceRoot.absoluteFilePath("subfolder3/basefileaaaaa.txt")));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        EXPECT_TRUE(m_processResults.isEmpty());
        EXPECT_TRUE(m_changedInputResults.isEmpty());
        EXPECT_TRUE(m_assetMessages.isEmpty());

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, m_sourceRoot.absoluteFilePath("basefileaaaaa.txt")));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        EXPECT_TRUE(m_processResults.isEmpty());
        EXPECT_TRUE(m_changedInputResults.isEmpty());
        EXPECT_TRUE(m_assetMessages.isEmpty());

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        m_processResults.clear();

        QString inputIgnoreFilePath = AssetUtilities::NormalizeFilePath(m_sourceRoot.absoluteFilePath("subfolder3/uniquefile.ignore"));

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputIgnoreFilePath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 1); // 1, since we have one recognizer for .ignore, but the 'android' platform is marked as skip
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "pc");

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, ProcessFile_FeedFileToProcess_TasksGenerated)
    {
        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        QList<QPair<unsigned int, QByteArray>> payloadList;
        AssetProcessor::MockConnectionHandler connection;
        connection.BusConnect(1);
        connection.m_callback = [&payloadList](unsigned int type, [[maybe_unused]] unsigned int serial, const QByteArray payload)
        {
            payloadList.append(qMakePair(type, payload));
        };

        AssetRecognizer rec;
        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // test dual-recognisers - two recognisers for the same pattern.
        rec.m_name = "txt files 2 (builder2)";
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        QString relativePathFromWatchFolder = "uniquefile.txt";
        QString watchFolderPath = m_sourceRoot.absoluteFilePath("subfolder3");
        QString absolutePath = AssetUtilities::NormalizeFilePath(watchFolderPath + "/" + relativePathFromWatchFolder);
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({absolutePath});

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 4); // 2 each for pc and android,since we have two recognizer for .txt file
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");

        QList<int> androidJobsIndex;
        QList<int> pcJobsIndex;
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            EXPECT_NE(m_processResults[checkIdx].m_jobEntry.m_computedFingerprint, 0);
            EXPECT_NE(m_processResults[checkIdx].m_jobEntry.m_jobRunKey, 0);
            EXPECT_EQ(QString(m_processResults[checkIdx].m_jobEntry.m_sourceAssetReference.ScanFolderPath().c_str()), AssetUtilities::NormalizeFilePath(watchFolderPath));
            EXPECT_EQ(m_processResults[checkIdx].m_jobEntry.m_sourceAssetReference.RelativePath().Native(), "uniquefile.txt");

            QString platformFolder = m_cacheRoot.filePath(QString::fromUtf8(m_processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()));
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            AZ::IO::Path expectedCachePath = m_cacheRoot.absoluteFilePath(platformFolder).toUtf8().constData();
            AZ::IO::FixedMaxPath intermediateAssetsFolder = AssetUtilities::GetIntermediateAssetsFolder(m_cacheRoot.path().toUtf8().constData());

            EXPECT_EQ(m_processResults[checkIdx].m_cachePath, expectedCachePath);
            EXPECT_EQ(m_processResults[checkIdx].m_intermediatePath, intermediateAssetsFolder);
            EXPECT_NE(m_processResults[checkIdx].m_jobEntry.m_computedFingerprint, 0);

            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "OnJobStatusChanged", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[checkIdx].m_jobEntry), Q_ARG(JobStatus, JobStatus::Queued));

            QCoreApplication::processEvents(QEventLoop::AllEvents);

            // create log files, so that we can test the correct retrieval

            // we create all of them except for #1
            if (checkIdx != 1)
            {
                JobInfo info;
                info.m_jobRunKey = m_processResults[checkIdx].m_jobEntry.m_jobRunKey;
                info.m_builderGuid = m_processResults[checkIdx].m_jobEntry.m_builderGuid;
                info.m_jobKey = m_processResults[checkIdx].m_jobEntry.m_jobKey.toUtf8().data();
                info.m_platform = m_processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str();
                info.m_sourceFile = m_processResults[checkIdx].m_jobEntry.m_sourceAssetReference.RelativePath().c_str();
                info.m_watchFolder = m_processResults[checkIdx].m_jobEntry.m_sourceAssetReference.ScanFolderPath().c_str();

                AZStd::string logFolder = AZStd::string::format("%s/%s", AssetUtilities::ComputeJobLogFolder().c_str(), AssetUtilities::ComputeJobLogFileName(info).c_str());
                AZ::IO::HandleType logHandle;
                AZ::IO::LocalFileIO::GetInstance()->CreatePath(AssetUtilities::ComputeJobLogFolder().c_str());
                EXPECT_TRUE(AZ::IO::LocalFileIO::GetInstance()->Open(logFolder.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, logHandle));
                AZStd::string logLine = AZStd::string::format("Log stored for job run key %lli\n", m_processResults[checkIdx].m_jobEntry.m_jobRunKey);
                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
                AZ::IO::LocalFileIO::GetInstance()->Close(logHandle);
            }
        }


        // ----------------------- test job info requests, while we have some assets in flight ---------------------------

        // by this time, querying for the status of those jobs should be possible since the "OnJobStatusChanged" event should have bubbled through
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;
            AssetJobsInfoResponse jobResponse;

            requestInfo.m_searchTerm = absolutePath.toUtf8().constData();

            m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);

            EXPECT_TRUE(jobResponse.m_isSuccess);
            EXPECT_EQ(jobResponse.m_jobList.size(), m_processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                EXPECT_EQ(jobInfo.m_status, JobStatus::Queued);
                EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                EXPECT_FALSE(jobInfo.m_platform.empty());
                EXPECT_FALSE(jobInfo.m_jobKey.empty());
                EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());
                EXPECT_NE(jobInfo.m_jobRunKey, 0);

                for (const JobDetails& details : m_processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_sourceAssetReference.RelativePath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_sourceAssetReference.ScanFolderPath().c_str(), Qt::CaseSensitive) ==
                         0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.m_jobRunKey == details.m_jobEntry.m_jobRunKey) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;
                        break;
                    }
                }
                EXPECT_TRUE(foundIt);
            }
        }

        // ------------- JOB LOG TEST -------------------
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            const JobDetails& details = m_processResults[checkIdx];
            // create log files, so that we can test the correct retrieval

            // we create all of them except for #1
            if (checkIdx != 1)
            {
                AZStd::string logFolder = AZStd::string::format("%s/%s", AssetUtilities::ComputeJobLogFolder().c_str(), AssetUtilities::ComputeJobLogFileName(details.m_jobEntry).c_str());
                AZ::IO::HandleType logHandle;
                AZ::IO::LocalFileIO::GetInstance()->CreatePath(AssetUtilities::ComputeJobLogFolder().c_str());
                EXPECT_TRUE(AZ::IO::LocalFileIO::GetInstance()->Open(logFolder.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, logHandle));
                AZStd::string logLine = AZStd::string::format("Log stored for job %u\n", m_processResults[checkIdx].m_jobEntry.GetHash());
                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
                AZ::IO::LocalFileIO::GetInstance()->Close(logHandle);
            }
        }

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            const JobDetails& details = m_processResults[checkIdx];

            // request job logs.
            AssetJobLogRequest requestLog;
            AssetJobLogResponse requestResponse;
            requestLog.m_jobRunKey = details.m_jobEntry.m_jobRunKey;
            {
                // send our request:
                m_assetProcessorManager->ProcessGetAssetJobLogRequest(requestLog, requestResponse);

                if (checkIdx != 1)
                {
                    EXPECT_TRUE(requestResponse.m_isSuccess);
                    EXPECT_FALSE(requestResponse.m_jobLog.empty());
                    AZStd::string checkString = AZStd::string::format("Log stored for job %u\n", m_processResults[checkIdx].m_jobEntry.GetHash());
                    EXPECT_NE(requestResponse.m_jobLog.find(checkString.c_str()), AZStd::string::npos);
                }
                else
                {
                    // the [1] index was not written so it should be failed and empty
                    EXPECT_FALSE(requestResponse.m_isSuccess);
                }
            }
        }

        // now indicate the job has started.
        for (const JobDetails& details : m_processResults)
        {
            m_assetProcessorManager->OnJobStatusChanged(details.m_jobEntry, JobStatus::InProgress);
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents);


        // ----------------------- test job info requests, while we have some assets in flight ---------------------------

        // by this time, querying for the status of those jobs should be possible since the "OnJobStatusChanged" event should have bubbled through
        // and this time, it should be "in progress"
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;
            AssetJobsInfoResponse jobResponse;

            requestInfo.m_searchTerm = absolutePath.toUtf8().constData();

            {
                // send our request:
                payloadList.clear();
                connection.m_sent = false;
                m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);
            }

            EXPECT_TRUE(jobResponse.m_isSuccess);
            EXPECT_EQ(jobResponse.m_jobList.size(), m_processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                EXPECT_EQ(jobInfo.m_status, JobStatus::InProgress);
                EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                EXPECT_FALSE(jobInfo.m_platform.empty());
                EXPECT_FALSE(jobInfo.m_jobKey.empty());
                EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (const JobDetails& details : m_processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_sourceAssetReference.RelativePath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_sourceAssetReference.ScanFolderPath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;
                        break;
                    }
                }
                EXPECT_TRUE(foundIt);
            }
        }


        QStringList androidouts;
        androidouts.push_back(m_cacheRoot.filePath(QString("android/basefile.arc1")));
        androidouts.push_back(m_cacheRoot.filePath(QString("android/basefile.arc2")));

        // feed it the messages its waiting for (create the files)
        EXPECT_TRUE(CreateDummyFile(androidouts[0], "products."));
        EXPECT_TRUE(CreateDummyFile(androidouts[1], "products."));

        //Invoke Asset Processed for android platform , txt files job description
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts[0]), AZ::Uuid::CreateNull(), 1));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts[1]), AZ::Uuid::CreateNull(), 2));

        // make sure legacy SubIds get stored in the DB and in asset response messages.
        // also make sure they don't get filed for the wrong asset.
        response.m_outputProducts[0].m_legacySubIDs.push_back(1234);
        response.m_outputProducts[0].m_legacySubIDs.push_back(5678);
        response.m_outputProducts[1].m_legacySubIDs.push_back(2222);

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        EXPECT_EQ(m_assetMessages.size(), 2);

        EXPECT_EQ(m_changedInputResults.size(), 1);

        // always RELATIVE, always with the product name.
        EXPECT_EQ(m_assetMessages[0].m_platform, "android");
        EXPECT_EQ(m_assetMessages[1].m_platform, "android");
        EXPECT_EQ(m_assetMessages[0].m_data, "basefile.arc1");
        EXPECT_EQ(m_assetMessages[1].m_data, "basefile.arc2");
        EXPECT_EQ(m_assetMessages[0].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        EXPECT_EQ(m_assetMessages[1].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        EXPECT_NE(m_assetMessages[0].m_sizeBytes, 0);
        EXPECT_NE(m_assetMessages[1].m_sizeBytes, 0);
        EXPECT_TRUE(m_assetMessages[0].m_assetId.IsValid());
        EXPECT_TRUE(m_assetMessages[1].m_assetId.IsValid());
        EXPECT_TRUE(!m_assetMessages[0].m_legacyAssetIds.empty());
        EXPECT_TRUE(!m_assetMessages[1].m_legacyAssetIds.empty());
        EXPECT_TRUE(m_assetMessages[0].m_legacyAssetIds[0].IsValid());
        EXPECT_TRUE(m_assetMessages[1].m_legacyAssetIds[0].IsValid());
        EXPECT_NE(m_assetMessages[0].m_legacyAssetIds[0], m_assetMessages[0].m_assetId);
        EXPECT_NE(m_assetMessages[1].m_legacyAssetIds[0], m_assetMessages[1].m_assetId);

        EXPECT_EQ(m_assetMessages[0].m_legacyAssetIds.size(), 3);
        EXPECT_EQ(m_assetMessages[1].m_legacyAssetIds.size(), 2);

        EXPECT_EQ(m_assetMessages[0].m_legacyAssetIds[1].m_subId, 1234);
        EXPECT_EQ(m_assetMessages[0].m_legacyAssetIds[2].m_subId, 5678);
        EXPECT_EQ(m_assetMessages[1].m_legacyAssetIds[1].m_subId, 2222);

        EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_changedInputResults[0].first), AssetUtilities::NormalizeFilePath(absolutePath));

        // ----------------------- test job info requests, when some assets are done.
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;
            bool escalated = false;
            int numEscalated = 0;

            requestInfo.m_escalateJobs = true;
            requestInfo.m_searchTerm = absolutePath.toUtf8().constData();
            auto connectionMade = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::EscalateJobs, this, [&escalated, &numEscalated](AssetProcessor::JobIdEscalationList jobList)

                    {
                        escalated = true;
                        numEscalated = jobList.size();
                    });

            AssetJobsInfoResponse jobResponse;
            // send our request:
            m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);

            // wait for it to process:
            QCoreApplication::processEvents(QEventLoop::AllEvents);

            QObject::disconnect(connectionMade);

            EXPECT_TRUE(escalated);
            EXPECT_GT(numEscalated, 0);

            EXPECT_TRUE(jobResponse.m_isSuccess);
            EXPECT_EQ(jobResponse.m_jobList.size(), m_processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                EXPECT_FALSE(jobInfo.m_platform.empty());
                EXPECT_FALSE(jobInfo.m_jobKey.empty());
                EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (int detailsIdx = 0; detailsIdx < m_processResults.size(); ++detailsIdx)
                {
                    const JobDetails& details = m_processResults[detailsIdx];

                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_sourceAssetReference.RelativePath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_sourceAssetReference.ScanFolderPath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;

                        if (detailsIdx == 0) // we only said that the first job was done
                        {
                            EXPECT_TRUE(jobInfo.m_status == JobStatus::Completed);
                        }
                        else
                        {
                            EXPECT_EQ(jobInfo.m_status, JobStatus::InProgress);
                        }

                        break;
                    }
                }
                EXPECT_TRUE(foundIt);
            }
        }

        m_changedInputResults.clear();
        m_assetMessages.clear();

        androidouts.clear();
        androidouts.push_back(m_cacheRoot.filePath(QString("android/basefile.azm")));
        EXPECT_TRUE(CreateDummyFile(androidouts[0], "products."));

        //Invoke Asset Processed for android platform , txt files2 job description
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts[0])));

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        EXPECT_EQ(m_assetMessages.size(), 1);
        EXPECT_EQ(m_changedInputResults.size(), 1);

        // always RELATIVE, always with the product name.
        EXPECT_EQ(m_assetMessages[0].m_platform, "android");
        EXPECT_EQ(m_assetMessages[0].m_data, "basefile.azm");

        m_changedInputResults.clear();
        m_assetMessages.clear();

        QStringList pcouts;
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/basefile.arc1")));
        EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0])));

        //Invoke Asset Processed for pc platform , txt files job description
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        EXPECT_EQ(m_assetMessages.size(), 1);
        EXPECT_EQ(m_changedInputResults.size(), 1);

        // always RELATIVE, always with the product name.
        EXPECT_EQ(m_assetMessages[0].m_platform, "pc");
        EXPECT_EQ(m_assetMessages[0].m_data, "basefile.arc1");


        EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_changedInputResults[0].first), AssetUtilities::NormalizeFilePath(absolutePath));

        m_changedInputResults.clear();
        m_assetMessages.clear();

        pcouts.clear();
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/basefile.azm")));
        EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0])));

        //Invoke Asset Processed for pc platform , txt files 2 job description
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        EXPECT_EQ(m_assetMessages.size(), 1);
        EXPECT_EQ(m_changedInputResults.size(), 1);

        // always RELATIVE, always with the product name.
        EXPECT_EQ(m_assetMessages[0].m_platform, "pc");
        EXPECT_EQ(m_assetMessages[0].m_data, "basefile.azm");

        EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_changedInputResults[0].first), AssetUtilities::NormalizeFilePath(absolutePath));

        // all four should now be complete:
        // ----------------------- test job info requests, now that all are done ---------------------------

        // by this time, querying for the status of those jobs should be possible since the "OnJobStatusChanged" event should have bubbled through
        // and this time, it should be "in progress"
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;
            AssetJobsInfoResponse jobResponse;

            requestInfo.m_searchTerm = absolutePath.toUtf8().constData();

            // send our request:
            m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);

            EXPECT_TRUE(jobResponse.m_isSuccess);
            EXPECT_EQ(jobResponse.m_jobList.size(), m_processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                EXPECT_EQ(jobInfo.m_status, JobStatus::Completed);
                EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                EXPECT_FALSE(jobInfo.m_platform.empty());
                EXPECT_FALSE(jobInfo.m_jobKey.empty());
                EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (const JobDetails& details : m_processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_sourceAssetReference.RelativePath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_sourceAssetReference.ScanFolderPath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;
                        break;
                    }
                }
                EXPECT_TRUE(foundIt);
            }
        }

        m_changedInputResults.clear();
        m_assetMessages.clear();
        m_processResults.clear();

        // feed it the exact same file again.
        // this should result in NO ADDITIONAL processes since nothing has changed.
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_TRUE(m_processResults.isEmpty());
        EXPECT_TRUE(m_changedInputResults.isEmpty());
        EXPECT_TRUE(m_assetMessages.isEmpty());

        // delete one of the products and tell it that it changed
        // it should reprocess that file, for that platform only:

        payloadList.clear();
        connection.m_sent = false;

        AssetNotificationMessage assetNotifMessage;
        SourceFileNotificationMessage sourceFileChangedMessage;

        // this should result in NO ADDITIONAL processes since nothing has changed.
        EXPECT_TRUE(QFile::remove(pcouts[0]));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, pcouts[0]));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        // We should not be receiving any sourcefile notification message here since the source file hasn't changed
        EXPECT_EQ(payloadList.size(), 0);

        // should have asked to launch only the PC process because the other assets are already done for the other plat
        EXPECT_EQ(m_processResults.size(), 1);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_processResults[0].m_jobEntry.GetAbsoluteSourcePath()), AssetUtilities::NormalizeFilePath(absolutePath));

        EXPECT_TRUE(CreateDummyFile(pcouts[0], "products2"));
        // tell it were done again!

        m_changedInputResults.clear();
        m_assetMessages.clear();

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0])));

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_EQ(m_assetMessages.size(), 1);
        EXPECT_EQ(m_changedInputResults.size(), 1);

        // always RELATIVE, always with the product name.
        EXPECT_EQ(m_assetMessages[0].m_data, "basefile.azm");
        EXPECT_EQ(m_assetMessages[0].m_platform, "pc");
        EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_changedInputResults[0].first), AssetUtilities::NormalizeFilePath(absolutePath));

        m_changedInputResults.clear();
        m_assetMessages.clear();
        m_processResults.clear();

        connection.m_sent = false;
        payloadList.clear();

        // modify the input file, then
        // feed it the exact same file again.
        // it should spawn BOTH compilers:
        EXPECT_TRUE(QFile::remove(absolutePath));
        EXPECT_TRUE(CreateDummyFile(absolutePath, "new!"));
        AZ_TracePrintf(AssetProcessor::DebugChannel, "-------------------------------------------\n");

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_TRUE(connection.m_sent);
        EXPECT_EQ(payloadList.size(), 1);// We should always receive only one of these messages
        EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(0).second.data(), payloadList.at(0).second.size(), sourceFileChangedMessage));
        QDir scanFolder(sourceFileChangedMessage.m_scanFolder.c_str());
        QString pathToCheck = scanFolder.filePath(sourceFileChangedMessage.m_relativeSourcePath.c_str());
        EXPECT_EQ(QString::compare(absolutePath, pathToCheck, Qt::CaseSensitive), 0);

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        // --------- same result as above ----------
        EXPECT_EQ(m_processResults.size(), 4); // 2 each for pc and android,since we have two recognizer for .txt file
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_NE(m_processResults[0].m_jobEntry.m_computedFingerprint, 0);
        EXPECT_NE(m_processResults[1].m_jobEntry.m_computedFingerprint, 0);

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = m_processResults[checkIdx].m_jobEntry.GetAbsoluteSourcePath();
            EXPECT_EQ(AssetUtilities::NormalizeFilePath(processFile1), AssetUtilities::NormalizeFilePath(absolutePath));
            QString platformFolder = m_cacheRoot.filePath(QString::fromUtf8(m_processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()));
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            AZ::IO::Path expectedCachePath = m_cacheRoot.absoluteFilePath(platformFolder).toUtf8().constData();
            AZ::IO::FixedMaxPath intermediateAssetsFolder = AssetUtilities::GetIntermediateAssetsFolder(m_cacheRoot.path().toUtf8().constData());

            EXPECT_EQ(m_processResults[checkIdx].m_cachePath, expectedCachePath);
            EXPECT_EQ(m_processResults[checkIdx].m_intermediatePath, intermediateAssetsFolder);
            EXPECT_NE(m_processResults[checkIdx].m_jobEntry.m_computedFingerprint, 0);
        }

        // this time make different products:

        QStringList oldandroidouts;
        QStringList oldpcouts;
        oldandroidouts = androidouts;
        oldpcouts.append(pcouts);
        QStringList androidouts2;
        QStringList pcouts2;
        androidouts.clear();
        pcouts.clear();
        androidouts.push_back(m_cacheRoot.filePath(QString("android/basefilea.arc1")));
        androidouts2.push_back(m_cacheRoot.filePath(QString("android/basefilea.azm")));
        // note that the android outs have changed
        // but the pc outs are still the same.
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/basefile.arc1")));
        pcouts2.push_back(m_cacheRoot.filePath(QString("pc/basefile.azm")));

        // feed it the messages its waiting for (create the files)
        EXPECT_TRUE(CreateDummyFile(androidouts[0], "newfile."));
        EXPECT_TRUE(CreateDummyFile(pcouts[0], "newfile."));
        EXPECT_TRUE(CreateDummyFile(androidouts2[0], "newfile."));
        EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));

        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 50);

        m_changedInputResults.clear();
        m_assetMessages.clear();

        // send all the done messages simultaneously:

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts2[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts2[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 50);

        EXPECT_EQ(m_changedInputResults.size(), 4);
        EXPECT_EQ(m_assetMessages.size(), 7);

        // what we expect to happen here is that it tells us that 3 files were removed, and 4 files were changed.
        // The files removed should be the ones we did not emit this time
        // note that order isn't guarantee but an example output it this

        // [0] Removed: ANDROID, basefile.arc1
        // [1] Removed: ANDROID, basefile.arc2
        // [2] Changed: ANDROID, basefilea.arc1 (added)

        // [3] Removed: ANDROID, basefile.azm
        // [4] Changed: ANDROID, basefilea.azm (added)

        // [5] changed: PC, basefile.arc1 (changed)
        // [6] changed: PC, basefile.azm (changed)

        for (auto element : m_assetMessages)
        {
            if (element.m_data == "basefile.arc1")
            {
                if (element.m_platform == "pc")
                {
                    EXPECT_EQ(element.m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
                }
                else
                {
                    EXPECT_EQ(element.m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
                }
            }

            if (element.m_data == "basefilea.arc1")
            {
                EXPECT_EQ(element.m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
                EXPECT_EQ(element.m_platform, "android");
            }

            if (element.m_data == "basefile.arc2")
            {
                EXPECT_EQ(element.m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
                EXPECT_EQ(element.m_platform, "android");
            }
        }

        // original products must no longer exist since it should have found and deleted them!
        for (QString outFile: oldandroidouts)
        {
            EXPECT_FALSE(QFile::exists(outFile));
        }

        // the old pc products should still exist because they were emitted this time around.
        for (QString outFile: oldpcouts)
        {
            EXPECT_TRUE(QFile::exists(outFile));
        }

        m_changedInputResults.clear();
        m_assetMessages.clear();
        m_processResults.clear();

        // add a fingerprint file thats next to the original file
        // feed it the exportsettings file again.
        // it should spawn BOTH compilers again.
        EXPECT_TRUE(CreateDummyFile(absolutePath + ".exportsettings", "new!"));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath + ".exportsettings"));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        // --------- same result as above ----------
        EXPECT_EQ(m_processResults.size(), 4); // pc and android
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_NE(m_processResults[0].m_jobEntry.m_computedFingerprint, 0);

        // send all the done messages simultaneously:
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = m_processResults[checkIdx].m_jobEntry.GetAbsoluteSourcePath();
            EXPECT_EQ(AssetUtilities::NormalizeFilePath(processFile1), AssetUtilities::NormalizeFilePath(absolutePath));
            VerifyProductPaths(m_processResults[checkIdx]);
            EXPECT_NE(m_processResults[checkIdx].m_jobEntry.m_computedFingerprint, 0);
        }

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts2[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts2[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // --- delete the input asset and make sure it cleans up all products.

        m_changedInputResults.clear();
        m_assetMessages.clear();
        m_processResults.clear();


        // first, delete the fingerprint file, this should result in normal reprocess:
        QFile::remove(absolutePath + ".exportsettings");
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath + ".exportsettings"));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        // --------- same result as above ----------
        EXPECT_EQ(m_processResults.size(), 4); // 2 each for pc and android,since we have two recognizer for .txt file
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_NE(m_processResults[0].m_jobEntry.m_computedFingerprint, 0);
        EXPECT_NE(m_processResults[1].m_jobEntry.m_computedFingerprint, 0);


        // send all the done messages simultaneously:

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts2[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts2[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // deleting the fingerprint file should not have erased the products
        EXPECT_TRUE(QFile::exists(pcouts[0]));
        EXPECT_TRUE(QFile::exists(androidouts[0]));
        EXPECT_TRUE(QFile::exists(pcouts2[0]));
        EXPECT_TRUE(QFile::exists(androidouts2[0]));

        m_changedInputResults.clear();
        m_assetMessages.clear();
        m_processResults.clear();

        connection.m_sent = false;
        payloadList.clear();

        // delete the original input.
        QFile::remove(absolutePath);

        SourceFileNotificationMessage sourceFileRemovedMessage;
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        // 9 messages because there's one source file with 4 products so:
        //      1 * file remove for the source file.
        //      4 * file claimed for the produce file to be able to update it safely.
        //      4 * file released for the produce file so it's free for other tools to use it again.
        EXPECT_EQ(payloadList.size(), 9);
        unsigned int messageLoadCount = 0;
        for (auto payload : payloadList)
        {
            if (payload.first == SourceFileNotificationMessage::MessageType)
            {
                EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), sourceFileRemovedMessage));
                EXPECT_EQ(sourceFileRemovedMessage.m_type, SourceFileNotificationMessage::FileRemoved);
                ++messageLoadCount;
            }
            else if (payload.first == AssetNotificationMessage::MessageType)
            {
                AssetNotificationMessage message;
                EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), message));
                EXPECT_TRUE(
                    message.m_type == AssetNotificationMessage::NotificationType::JobFileClaimed ||
                    message.m_type == AssetNotificationMessage::NotificationType::JobFileReleased);
                ++messageLoadCount;
            }
        }

        EXPECT_TRUE(connection.m_sent);
        EXPECT_EQ(messageLoadCount, azlossy_cast<unsigned>(payloadList.size())); // make sure all messages are accounted for
        scanFolder = QDir(sourceFileRemovedMessage.m_scanFolder.c_str());
        pathToCheck = scanFolder.filePath(sourceFileRemovedMessage.m_relativeSourcePath.c_str());
        EXPECT_EQ(QString::compare(absolutePath, pathToCheck, Qt::CaseSensitive), 0);

        // nothing to process, but products should be gone!
        EXPECT_TRUE(m_processResults.isEmpty());
        EXPECT_TRUE(m_changedInputResults.isEmpty());

        // should have gotten four "removed" messages for its products:
        EXPECT_EQ(m_assetMessages.size(), 4);

        for (auto element : m_assetMessages)
        {
            EXPECT_EQ(element.m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        }

        EXPECT_FALSE(QFile::exists(pcouts[0]));
        EXPECT_FALSE(QFile::exists(androidouts[0]));
        EXPECT_FALSE(QFile::exists(pcouts2[0]));
        EXPECT_FALSE(QFile::exists(androidouts2[0]));

        m_changedInputResults.clear();
        m_assetMessages.clear();
        m_processResults.clear();

        // test: if an asset fails, it should recompile it next time, and not report success

        EXPECT_TRUE(CreateDummyFile(absolutePath, "new2"));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        // --------- same result as above ----------
        EXPECT_EQ(m_processResults.size(), 4); // 2 each for pc and android,since we have two recognizer for .txt file
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_NE(m_processResults[0].m_jobEntry.m_computedFingerprint, 0);

        EXPECT_TRUE(CreateDummyFile(androidouts[0], "newfile."));
        EXPECT_TRUE(CreateDummyFile(androidouts2[0], "newfile."));
        EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));

        // send both done messages simultaneously!
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts2[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // send one failure only for PC :
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetFailed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[2].m_jobEntry));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts2[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        // ----------------------- test job info requests, some assets have failed (specifically, the [2] index job entry
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;

            requestInfo.m_searchTerm = absolutePath.toUtf8().constData();

            payloadList.clear();

            AssetJobsInfoResponse jobResponse;
            m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);

            EXPECT_TRUE(jobResponse.m_isSuccess);
            EXPECT_EQ(jobResponse.m_jobList.size(), m_processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                EXPECT_FALSE(jobInfo.m_platform.empty());
                EXPECT_FALSE(jobInfo.m_jobKey.empty());
                EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (int detailsIdx = 0; detailsIdx < m_processResults.size(); ++detailsIdx)
                {
                    const JobDetails& details = m_processResults[detailsIdx];

                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_sourceAssetReference.RelativePath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_sourceAssetReference.ScanFolderPath().c_str(), Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;

                        if (detailsIdx == 2) // we only said that the index [2] job was dead
                        {
                            EXPECT_EQ(jobInfo.m_status, JobStatus::Failed);
                        }
                        else
                        {
                            EXPECT_EQ(jobInfo.m_status, JobStatus::Completed);
                        }

                        break;
                    }
                }
                EXPECT_TRUE(foundIt);
            }
        }

        // we should have get three success:
        EXPECT_EQ(m_changedInputResults.size(), 3);
        EXPECT_EQ(m_assetMessages.size(), 3);

        // which should be for the ANDROID:
        EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_changedInputResults[0].first), absolutePath);

        // always RELATIVE, always with the product name.
        EXPECT_TRUE(m_assetMessages[0].m_data == "basefilea.arc1" || m_assetMessages[0].m_data == "basefilea.azm");
        EXPECT_EQ(m_assetMessages[0].m_platform, "android");

        for (auto& payload : payloadList)
        {
            if (payload.first == SourceFileNotificationMessage::MessageType)
            {
                EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), sourceFileRemovedMessage));
                EXPECT_EQ(sourceFileRemovedMessage.m_type, SourceFileNotificationMessage::FileRemoved);
            }
        }

        scanFolder = QDir(sourceFileRemovedMessage.m_scanFolder.c_str());
        pathToCheck = scanFolder.filePath(sourceFileRemovedMessage.m_relativeSourcePath.c_str());
        EXPECT_EQ(QString::compare(absolutePath, pathToCheck, Qt::CaseSensitive), 0);

        // now if we notify again, only the pc should process:
        m_changedInputResults.clear();
        m_assetMessages.clear();
        m_processResults.clear();
        payloadList.clear();

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // --------- same result as above ----------
        EXPECT_EQ(m_processResults.size(), 1); // pc only
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "pc");

        EXPECT_TRUE(CreateDummyFile(pcouts[0], "new1"));

        // send one failure only for PC :

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0])));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // we should have got only one success:
        EXPECT_EQ(m_changedInputResults.size(), 1);
        EXPECT_EQ(m_assetMessages.size(), 1);

        // always RELATIVE, always with the product name.
        EXPECT_EQ(m_assetMessages[0].m_data, "basefile.arc1");
        EXPECT_EQ(m_assetMessages[0].m_platform, "pc");

        connection.BusDisconnect(1);
        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, ValidatePlatformSpecificAssetRecognizer_FeedFileToProcess_PlatformSpecificTaskGenerated)
    {
        //Test the ProcessGetFullAssetPath function

        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        AssetRecognizer rec;
        rec.m_name = "random files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.random", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        EXPECT_TRUE(mockAppManager.RegisterAssetRecognizerAsBuilder(rec));
        
        QString absolutePath = AssetUtilities::NormalizeFilePath(m_sourceRoot.absoluteFilePath("subfolder3/somerandomfile.random"));
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({absolutePath});
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 1); // 1 for pc
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "pc");

        QStringList pcouts;
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/subfolder3/randomfileoutput.random")));
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/subfolder3/randomfileoutput.random1")));
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/subfolder3/randomfileoutput.random2")));
        EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));
        EXPECT_TRUE(CreateDummyFile(pcouts[1], "products."));
        EXPECT_TRUE(CreateDummyFile(pcouts[2], "products."));

        //Invoke Asset Processed for pc platform , txt files job description
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0]), AZ::Uuid::CreateNull(), 1));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[1]), AZ::Uuid::CreateNull(), 2));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[2]), AZ::Uuid::CreateNull(), 3));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

        EXPECT_EQ(m_assetMessages.size(), 3);
        EXPECT_EQ(m_changedInputResults.size(), 1);

        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, ValidateOverrideSystem_FeedFilesWithSameNameButUnderDifferentScanFolders_TasksGeneratedBasedOnOverrideRules)
    {
        // There is a sub-case of handling mixed cases, but is only supported on case-insensitive filesystems.
#if defined(AZ_PLATFORM_LINUX)
        // Linux is case-sensitive, so 'basefile.txt' will stay the same case as the other subfolder versions
        constexpr const char* subfolder3BaseFilePath = "subfolder3/basefile.txt";
        constexpr int expectedLegacyAssetIdCount = 1;
#else
        constexpr const char* subfolder3BaseFilePath = "subfolder3/BaseFile.txt";
        constexpr int expectedLegacyAssetIdCount = 2;
#endif

        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        AssetRecognizer rec;
        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // test dual-recognisers - two recognisers for the same pattern.
        rec.m_name =  "txt files 2 (builder2)";
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_platformSpecs.clear();
        rec.m_testLockSource = false;

        rec.m_name = "xxx files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.xxx", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // two recognizers for the same pattern.
        rec.m_name = "xxx files 2 (builder2)";
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        QSet<QString> expectedFiles;
        expectedFiles << m_sourceRoot.absoluteFilePath("subfolder1/basefile.txt");
        expectedFiles << m_sourceRoot.absoluteFilePath("subfolder2/basefile.txt");
        expectedFiles << m_sourceRoot.absoluteFilePath(subfolder3BaseFilePath);
        expectedFiles << m_sourceRoot.absoluteFilePath("subfolder3/somefile.xxx");
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles(expectedFiles);

        // set up by letting it compile basefile.txt from subfolder3:
        QString absolutePath = AssetUtilities::NormalizeFilePath(m_sourceRoot.absoluteFilePath(subfolder3BaseFilePath));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        // --------- same result as above ----------
        EXPECT_EQ(m_processResults.size(), 4); // 2 each for pc and android,since we have two recognizer for .txt file
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_NE(m_processResults[0].m_jobEntry.m_computedFingerprint, 0);

        QStringList pcouts;
        QStringList androidouts;
        QStringList androidouts2;
        QStringList pcouts2;
        androidouts.push_back(m_cacheRoot.filePath(QString("android/basefilez.arc2")));
        androidouts2.push_back(m_cacheRoot.filePath(QString("android/basefileaz.azm2")));
        // note that the android outs have changed
        // but the pc outs are still the same.
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/basefile.arc2")));
        pcouts2.push_back(m_cacheRoot.filePath(QString("pc/basefile.azm2")));
        EXPECT_TRUE(CreateDummyFile(androidouts[0], "newfile."));
        EXPECT_TRUE(CreateDummyFile(pcouts[0], "newfile."));
        EXPECT_TRUE(CreateDummyFile(androidouts2[0], "newfile."));
        EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));
        m_changedInputResults.clear();
        m_assetMessages.clear();

        // send all the done messages simultaneously:
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts[0]), AZ::Uuid::CreateNull(), 1));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(androidouts2[0]), AZ::Uuid::CreateNull(), 2));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts[0]), AZ::Uuid::CreateNull(), 3));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(AbsProductPathToRelative(pcouts2[0]), AZ::Uuid::CreateNull(), 4));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

        // we should have got only one success:
        EXPECT_EQ(m_changedInputResults.size(), 4);
        EXPECT_EQ(m_assetMessages.size(), 4);
        for (auto element : m_assetMessages)
        {
            EXPECT_EQ(element.m_legacyAssetIds.size(), expectedLegacyAssetIdCount);
        }

        // ------------- setup complete, now do the test...
        // now feed it a file that has been overridden by a more important later file
        absolutePath = AssetUtilities::NormalizeFilePath(m_sourceRoot.absoluteFilePath("subfolder1/basefile.txt"));
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({absolutePath});
        m_changedInputResults.clear();
        m_assetMessages.clear();
        m_processResults.clear();

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        EXPECT_TRUE(m_processResults.isEmpty());
        EXPECT_TRUE(m_changedInputResults.isEmpty());
        EXPECT_TRUE(m_assetMessages.isEmpty());

        // since it was overridden, nothing should occur.
        //AZ_TracePrintf("Asset Processor", "Preparing the assessDeletedFiles invocation...\n");

        // delete the highest priority override file and ensure that it generates tasks
        // for the next highest priority!  Basically, deleting this file should "reveal" the file underneath it in the other subfolder
        QString deletedFile = m_sourceRoot.absoluteFilePath(subfolder3BaseFilePath);
        QString expectedReplacementInputFile = AssetUtilities::NormalizeFilePath(m_sourceRoot.absoluteFilePath("subfolder2/basefile.txt"));

        EXPECT_TRUE(QFile::remove(deletedFile));
        // sometimes the above deletion actually takes a moment to trickle, for some reason, and it doesn't actually get that the file was erased.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        EXPECT_FALSE(QFile::exists(deletedFile));

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, deletedFile));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        // On Linux, because we cannot change the case of the source file, the job fingerprint is not updated due the case-switch.
        // The reason the fingerprint for subfolder3/basefile.txt and subfolder2/basefile.txt are the same ON LINUX is because the
        // fingerprint of the file includes the filename (also both files have the same contents).  Additionally, when this test is set up,
        // subfolder3BaseFilePath ON LINUX is set to basefile.txt whereas it is set to BaseFile.txt on windows.  That is why the hash is the
        // same only for linux but different for other platforms. Note that if this test breaks on linux, it can be debugged on windows by
        // setting subfolder3BaseFilePath = basefile.txt on windows.
        // We still expect linux to produce the same result as other platforms however because we no longer query sources using just the relative path.
        // This means the override file which has not been processed yet MUST be processed, regardless of whether it just happens to have the same fingerprint
        // on linux.

        // --------- same result as above ----------
        EXPECT_EQ(m_processResults.size(), 4); // 2 each for pc and android,since we have two recognizer for .txt file
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_NE(m_processResults[0].m_jobEntry.m_computedFingerprint, 0);

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = m_processResults[checkIdx].m_jobEntry.GetAbsoluteSourcePath();
            EXPECT_EQ(processFile1, expectedReplacementInputFile);
            VerifyProductPaths(m_processResults[checkIdx]);
            EXPECT_NE(m_processResults[checkIdx].m_jobEntry.m_computedFingerprint, 0);
        }

        QString relativePathFromWatchFolder = "somefile.xxx";
        QString watchFolderPath = m_sourceRoot.absoluteFilePath("subfolder3");
        absolutePath = watchFolderPath + "/" + relativePathFromWatchFolder;

        unsigned int fingerprintForPC = 0;
        unsigned int fingerprintForANDROID = 0;

        AssetProcessorManagerUnitTestUtils::ComputeFingerprints(fingerprintForPC, fingerprintForANDROID, m_config, watchFolderPath, relativePathFromWatchFolder);

        m_processResults.clear();
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 4); // // 2 each for pc and android,since we have two recognizer for .xxx file
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");

        m_config.RemoveRecognizer("xxx files 2 (builder2)");
        EXPECT_TRUE(mockAppManager.UnRegisterAssetRecognizerAsBuilder("xxx files 2 (builder2)"));

        //Changing specs for pc
        rec.m_platformSpecs.insert({ "pc", AssetInternalSpec::Copy });

        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        m_processResults.clear();
        absolutePath = AssetUtilities::NormalizeFilePath(absolutePath);
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // we never actually submitted any fingerprints or indicated success, so the same number of jobs should occur as before
        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 4); // // 2 each for pc and android,since we have two recognizer for .xxx file
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, m_processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[2].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[3].m_jobEntry.m_platformInfo.m_identifier, "pc");

        // tell it that all those assets are now successfully done:
        AZ::u32 resultIdx = 0;
        for (const auto& processResult : m_processResults)
        {
            ++resultIdx;
            AZStd::string filename = ("doesn'tmatter.dds" + processResult.m_jobEntry.m_jobKey).toUtf8().constData();
            QString outputFile = (processResult.m_cachePath / filename).AsPosix().c_str();
            CreateDummyFile(outputFile);
            response = {};
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(
                AssetBuilderSDK::JobProduct((processResult.m_relativePath / filename).StringAsPosix(), AZ::Uuid::CreateNull(), resultIdx));
            m_assetProcessorManager->AssetProcessed(processResult.m_jobEntry, response);
        }

        m_config.RemoveRecognizer("xxx files 2 (builder2)");
        mockAppManager.UnRegisterAssetRecognizerAsBuilder("xxx files 2 (builder2)");

        //Changing version
        rec.m_version = "1.0";
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        m_processResults.clear();

        absolutePath = AssetUtilities::NormalizeFilePath(absolutePath);
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        EXPECT_EQ(m_processResults.size(), 2); // pc and android
        EXPECT_NE(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, m_processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        EXPECT_TRUE((m_processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc") || (m_processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        EXPECT_TRUE((m_processResults[1].m_jobEntry.m_platformInfo.m_identifier == "pc") || (m_processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));

        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, QueryAssetStatus_FeedFileToProcess_AssetStatusRetrieved)
    {
        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        AssetRecognizer rec;
        // tiff file recognizer
        rec.m_name = "tiff files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.tiff", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.clear();
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_testLockSource = true;
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        QString absolutePath = m_sourceRoot.absoluteFilePath("subfolder2/folder/ship.tiff");
        absolutePath = AssetUtilities::NormalizeFilePath(absolutePath);
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({absolutePath});

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        AZ::u32 resultIdx = 0;
        for (const JobDetails& processResult : m_processResults)
        {
            ++resultIdx;
            AZStd::string filename = "ship_nrm.dds";
            QString outputFile = (processResult.m_cachePath / filename).AsPosix().c_str();

            CreateDummyFile(outputFile);

            AssetBuilderSDK::ProcessJobResponse jobResponse;
            jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(
                (processResult.m_relativePath / filename).StringAsPosix(), AZ::Uuid::CreateNull(), resultIdx));

            m_assetProcessorManager->AssetProcessed(processResult.m_jobEntry, jobResponse);
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

        bool foundIt = false;

        auto connectionMade = connect(m_assetProcessorManager.get(), &AssetProcessorManager::SendAssetExistsResponse,
            this, [&foundIt]([[maybe_unused]] NetworkRequestID requestId, bool result)
        {
            foundIt = result;
        });

        const char* successCases[] =
        {
            "ship.tiff", // source
            "ship", // source no extension
            "ship_nrm.dds", // product
            "ship_nrm", // product no extension
        };

        NetworkRequestID requestId(1, 1);
        // Test source without path, should all fail
        for (const auto& testCase : successCases)
        {
            foundIt = false;

            m_assetProcessorManager->OnRequestAssetExists(requestId, "pc", testCase, AZ::Data::AssetId());
            EXPECT_FALSE(foundIt);
        }

        // Test source with the path included
        for (const auto& testCase : successCases)
        {
            foundIt = false;
            AZStd::string withPath = AZStd::string("folder/") + testCase;

            m_assetProcessorManager->OnRequestAssetExists(requestId, "pc", withPath.c_str(), AZ::Data::AssetId());
            EXPECT_TRUE(foundIt);
        }

        const char* failCases[] =
        {
            "folder/ships.tiff",
            "otherfolder/ship.tiff",
            "otherfolder/ship_nrm.dds",
            "folder/ship_random.other/random",
            "folder/ship.dds", // source wrong extension
            "folder/ship_nrm.tiff", // product wrong extension
            "folder/ship_color.dds", // product that doesn't exist
        };

        for (const auto& testCase : failCases)
        {
            foundIt = false;

            m_assetProcessorManager->OnRequestAssetExists(requestId, "pc", testCase, AZ::Data::AssetId());
            EXPECT_FALSE(foundIt);
        }

        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, RenameFolders_RenameSourceOrCacheFolders_AssetsReprocessedAccordingly)
    {
        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        AssetRecognizer rec;
        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // Test: Rename a source folder
        QString fileToMove1 = m_sourceRoot.absoluteFilePath("subfolder1/rename_this/somefile1.txt");
        QString fileToMove2 = m_sourceRoot.absoluteFilePath("subfolder1/rename_this/somefolder/somefile2.txt");
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({fileToMove1, fileToMove2});

        m_processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove2));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 4); // 2 fils on 2 platforms

        AssetBuilderSDK::ProcessJobResponse response;
        for (int index = 0; index < m_processResults.size(); ++index)
        {
            QFileInfo fi(m_processResults[index].m_jobEntry.GetAbsoluteSourcePath());
            AZStd::string filename = fi.fileName().toUtf8().constData();
            QString pcout = (m_processResults[index].m_cachePath / filename).c_str();
            EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct((m_processResults[index].m_relativePath / filename).StringAsPosix(), AZ::Uuid::CreateNull(), index));
            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }


        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // setup complete.  now RENAME that folder.
        QDir renamer;
        EXPECT_TRUE(renamer.rename(m_sourceRoot.absoluteFilePath("subfolder1/rename_this"), m_sourceRoot.absoluteFilePath("subfolder1/done_renaming")));

        // renames appear as a delete then add of that folder:
        m_processResults.clear();
        m_assetMessages.clear();
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, m_sourceRoot.absoluteFilePath("subfolder1/rename_this")));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 0); // nothing to process

        // we are aware that 4 products went missing (android and pc versions of the 2 files since we renamed the SOURCE folder)
        EXPECT_EQ(m_assetMessages.size(), 4);
        for (auto element : m_assetMessages)
        {
            EXPECT_EQ(element.m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        }

        m_processResults.clear();
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_sourceRoot.absoluteFilePath("subfolder1/done_renaming")));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 4); // 2 files on 2 platforms

        // Test: Rename a cache folder
        for (int index = 0; index < m_processResults.size(); ++index)
        {
            QFileInfo fi(m_processResults[index].m_jobEntry.GetAbsoluteSourcePath());
            AZStd::string filename = fi.fileName().toUtf8().constData();
            QString pcout = (m_processResults[index].m_cachePath / filename).c_str();
            EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(
                (m_processResults[index].m_relativePath / filename).StringAsPosix(), AZ::Uuid::CreateNull(), index));
            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // it now believes that there are a whole bunch of assets in subfolder1/done_renaming and they resulted in
        // a whole bunch of files to have been created in the asset cache, listed in m_processResults, and they exist in outputscreated...
        // rename the output folder:
        QString originalCacheFolderName = m_cacheRoot.absoluteFilePath("pc") + "/done_renaming";
        QString newCacheFolderName = m_cacheRoot.absoluteFilePath("pc") + "/renamed_again";

        EXPECT_TRUE(renamer.rename(originalCacheFolderName, newCacheFolderName));

        // tell it that the products moved:
        m_processResults.clear();
        m_assetMessages.clear();
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, originalCacheFolderName));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, newCacheFolderName));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        // at this point, we should NOT get 2 removed products - we should only get those messages later
        // once the processing queue actually processes these assets - not prematurely as it discovers them missing.
        EXPECT_EQ(m_assetMessages.size(), 0);

        // We've already (above) verified that the product list should be ok, this is just to avoid a crash instead of a failure.
        EXPECT_GT(m_processResults.size(), 1);

        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "pc");

        // Test: Rename folders that did not have files in them (but had child files, this was a bug at a point)
        fileToMove1 = m_sourceRoot.absoluteFilePath("subfolder1/rename_this_secondly/somefolder/somefile2.txt");
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({fileToMove1});

        m_processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));


        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 2); // 1 file on 2 platforms

        for (int index = 0; index < m_processResults.size(); ++index)
        {
            QFileInfo fi(m_processResults[index].m_jobEntry.GetAbsoluteSourcePath());
            AZStd::string filename = fi.fileName().toUtf8().constData();
            QString pcout = (m_processResults[index].m_cachePath / filename).c_str();
            EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(
                (m_processResults[index].m_relativePath / filename).StringAsPosix(), AZ::Uuid::CreateNull(), index));
            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // setup complete.  now RENAME that folder.

        originalCacheFolderName = m_cacheRoot.absoluteFilePath("pc") + "/rename_this_secondly";
        newCacheFolderName = m_cacheRoot.absoluteFilePath("pc") + "/done_renaming_again";

        EXPECT_TRUE(renamer.rename(originalCacheFolderName, newCacheFolderName));

        // tell it that the products moved:
        m_processResults.clear();
        m_assetMessages.clear();
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, originalCacheFolderName));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, newCacheFolderName));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_assetMessages.size(), 0); // we don't prematurely emit "AssetRemoved" until we actually finish process.
        EXPECT_EQ(m_processResults.size(), 1); // ONLY the PC files need to be re-processed because only those were renamed.
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "pc");

        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, DeleteSource_RemoveFileAfterProcessing_ProductDeleted)
    {
        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        AssetRecognizer rec;
        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // first, set up a whole pipeline to create, notify, and consume the file:
        QString fileToMove1 = m_sourceRoot.absoluteFilePath("subfolder1/to_be_deleted/some_deleted_file.txt");
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({fileToMove1});

        // put the two files on the map:
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 2); // 1 file on 2 platforms

        QStringList createdDummyFiles;
        AssetBuilderSDK::ProcessJobResponse response;
        for (int index = 0; index < m_processResults.size(); ++index)
        {
            QFileInfo fi(m_processResults[index].m_jobEntry.GetAbsoluteSourcePath());
            AZStd::string filename = fi.fileName().toUtf8().constData();
            QString pcout = (m_processResults[index].m_cachePath / filename).c_str();
            EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(
                (m_processResults[index].m_relativePath / filename).StringAsPosix(), AZ::Uuid::CreateNull(), index));
            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        m_processResults.clear();
        m_assetMessages.clear();

        // setup complete.  now delete the source file:
        QDir renamer;
        EXPECT_TRUE(renamer.remove(fileToMove1));
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);
        EXPECT_EQ(m_assetMessages.size(), 2); // all products must be removed
        EXPECT_EQ(m_processResults.size(), 0); // nothing should process

        for (int index = 0; index < createdDummyFiles.size(); ++index)
        {
            QFileInfo fi(createdDummyFiles[index]);
            EXPECT_FALSE(fi.exists());
            // in fact, the directory must also no longer exist in the cache:
            EXPECT_FALSE(fi.dir().exists());
        }

        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, ReprocessSource_ModifyFileAfterProcessing_ProductsRegenerated)
    {
        // --------------------------------------------------------------------------------------------------
        // - TEST SOURCE FILE REPROCESSING RESULTING IN FEWER PRODUCTS NEXT TIME ----------------------------
        // (it needs to delete the products and it needs to notify listeners about it)
        // --------------------------------------------------------------------------------------------------

        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        AssetRecognizer rec;
        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        m_config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // first, set up a whole pipeline to create, notify, and consume the file:
        QString fileToMove1 = m_sourceRoot.absoluteFilePath("subfolder1/fewer_products/test.txt");
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({fileToMove1});

        m_processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 2); // 1 file on 2 platforms

        QStringList createdDummyFiles; // keep track of the files which we expect to be gone next time
        AssetBuilderSDK::ProcessJobResponse response;
        for (int index = 0; index < m_processResults.size(); ++index)
        {
            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            // this time, ouput 2 files for each job instead of just one:
            QFileInfo fi(m_processResults[index].m_jobEntry.GetAbsoluteSourcePath());

            AZStd::string filename0 = (fi.fileName() + ".0.txt").toUtf8().constData();
            AZStd::string filename1 = (fi.fileName() + ".1.txt").toUtf8().constData();

            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(
                (m_processResults[index].m_relativePath / filename0).StringAsPosix(), AZ::Uuid::CreateNull(), index));
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(
                (m_processResults[index].m_relativePath / filename1).StringAsPosix(), AZ::Uuid::CreateNull(), index + 100));

            createdDummyFiles.push_back((m_processResults[index].m_cachePath / filename0).c_str()); // we're only gong to delete this one out of the two, which is why we don't push the other one.

            EXPECT_TRUE(CreateDummyFile((m_processResults[index].m_cachePath / filename0).c_str(), "product 0"));
            EXPECT_TRUE(CreateDummyFile((m_processResults[index].m_cachePath / filename1).c_str(), "product 1"));

            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // at this point, we have a cache with the four files (2 for each platform)
        // we're going to resubmit the job with different data
        QDir renamer;
        EXPECT_TRUE(renamer.remove(fileToMove1));
        EXPECT_TRUE(CreateDummyFile(fileToMove1, "fresh data!"));

        m_processResults.clear();

        // tell file changed:
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 2); // 1 file on 2 platforms

        m_assetMessages.clear();

        for (int index = 0; index < m_processResults.size(); ++index)
        {
            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            // this time, ouput only one file for each job instead of just one:
            QFileInfo fi(m_processResults[index].m_jobEntry.GetAbsoluteSourcePath());

            AZStd::string filename = (fi.fileName() + ".1.txt").toUtf8().constData();

            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(
                (m_processResults[index].m_relativePath / filename).StringAsPosix(), AZ::Uuid::CreateNull(), index));
            EXPECT_TRUE(CreateDummyFile((m_processResults[index].m_cachePath / filename).c_str(), "product 1 changed"));

            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        // we should have gotten 2 product removed, 2 product changed, total of 4 asset messages

        EXPECT_EQ(m_assetMessages.size(), 4);
        EXPECT_NE(m_assetMessages[0].m_assetId, AZ::Data::AssetId());
        EXPECT_NE(m_assetMessages[1].m_assetId, AZ::Data::AssetId());
        EXPECT_NE(m_assetMessages[2].m_assetId, AZ::Data::AssetId());
        EXPECT_NE(m_assetMessages[3].m_assetId, AZ::Data::AssetId());

        EXPECT_EQ(m_assetMessages[0].m_platform, "android");
        EXPECT_EQ(m_assetMessages[1].m_platform, "android");
        EXPECT_EQ(m_assetMessages[2].m_platform, "pc");
        EXPECT_EQ(m_assetMessages[3].m_platform, "pc");

        EXPECT_EQ(m_assetMessages[0].m_data, "fewer_products/test.txt.0.txt");
        EXPECT_EQ(m_assetMessages[1].m_data, "fewer_products/test.txt.1.txt");
        EXPECT_EQ(m_assetMessages[2].m_data, "fewer_products/test.txt.0.txt");
        EXPECT_EQ(m_assetMessages[3].m_data, "fewer_products/test.txt.1.txt");

        EXPECT_EQ(m_assetMessages[0].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        EXPECT_EQ(m_assetMessages[1].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        EXPECT_EQ(m_assetMessages[2].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        EXPECT_EQ(m_assetMessages[3].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);


        // and finally, the actual removed products should be gone from the HDD:
        for (int index = 0; index < createdDummyFiles.size(); ++index)
        {
            QFileInfo fi(createdDummyFiles[index]);
            EXPECT_FALSE(fi.exists());
            // the directory must still exist because there were other files in there (no accidental deletions!)
            EXPECT_TRUE(fi.dir().exists());
        }

        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, ValidateAssetBuilder_FeedFileToProcess_ProductsGenerated)
    {
        MockApplicationManager mockAppManager;
        mockAppManager.BusConnect();

        AssetRecognizer abt_rec1;
        abt_rec1.m_name = "UnitTestTextBuilder1";
        abt_rec1.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        abt_rec1.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});
        mockAppManager.RegisterAssetRecognizerAsBuilder(abt_rec1);

        AssetRecognizer abt_rec2;
        abt_rec2.m_name = "UnitTestTextBuilder2";
        abt_rec2.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        abt_rec2.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
        mockAppManager.RegisterAssetRecognizerAsBuilder(abt_rec2);

        m_processResults.clear();

        QString absolutePath = AssetUtilities::NormalizeFilePath(m_sourceRoot.absoluteFilePath("subfolder3/uniquefile.txt"));
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({absolutePath});

        // Pass the txt file through the asset pipeline
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));
        EXPECT_EQ(mockAppManager.GetMatchingBuildersInfoFunctionCalls(), 1);
        EXPECT_EQ(mockAppManager.GetMockBuilderCreateJobCalls(), 2);  // Since we have two text builder registered

        AssetProcessor::BuilderInfoList builderInfoList;
        mockAppManager.GetMatchingBuildersInfo(AZStd::string(absolutePath.toUtf8().constData()), builderInfoList);
        auto builderInfoListCount = builderInfoList.size();
        EXPECT_EQ(builderInfoListCount, 2);

        for (auto& buildInfo : builderInfoList)
        {
            AZStd::shared_ptr<InternalMockBuilder> builder;
            EXPECT_TRUE(mockAppManager.GetBuilderByID(buildInfo.m_name, builder));

            EXPECT_EQ(builder->GetCreateJobCalls(), 1);

            // note, uuid does not include watch folder name.  This is a quick test to make sure that the source file UUID actually makes it into the CreateJobRequest.
            // the ProcessJobRequest is populated frmo the CreateJobRequest.
            EXPECT_EQ(builder->GetLastCreateJobRequest().m_sourceFileUUID, AssetUtilities::CreateSafeSourceUUIDFromName("uniquefile.txt"));
            QString watchedFolder(AssetUtilities::NormalizeFilePath(builder->GetLastCreateJobRequest().m_watchFolder.c_str()));
            QString expectedWatchedFolder(m_sourceRoot.absoluteFilePath("subfolder3"));
            EXPECT_EQ(QString::compare(watchedFolder, expectedWatchedFolder, Qt::CaseInsensitive), 0); // verify watchfolder

            QString filename(AssetUtilities::NormalizeFilePath(builder->GetLastCreateJobRequest().m_sourceFile.c_str()));
            QString expectFileName("uniquefile.txt");
            EXPECT_EQ(QString::compare(filename, expectFileName, Qt::CaseInsensitive), 0); // verify filename
            builder->ResetCounters();
        }

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 2); // 1 for pc and android
        EXPECT_EQ(m_processResults[0].m_jobEntry.m_platformInfo.m_identifier, "android");
        EXPECT_EQ(m_processResults[1].m_jobEntry.m_platformInfo.m_identifier, "pc");
        EXPECT_EQ(QString::compare(m_processResults[0].m_jobEntry.GetAbsoluteSourcePath(), absolutePath, Qt::CaseInsensitive), 0);
        EXPECT_EQ(QString::compare(m_processResults[1].m_jobEntry.GetAbsoluteSourcePath(), absolutePath, Qt::CaseInsensitive), 0);
        EXPECT_EQ(QString::compare(QString(m_processResults[0].m_jobEntry.m_jobKey), QString(abt_rec1.m_name.c_str())), 0);
        EXPECT_EQ(QString::compare(QString(m_processResults[1].m_jobEntry.m_jobKey), QString(abt_rec2.m_name.c_str())), 0);

        mockAppManager.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, ValidateJobsWithDifferentKeys_FeedFileToProcess_GetJobsToProcess)
    {
        // Test Strategy
        // Tell the mock builder to create two jobs for the same source file and platform but having different job keys.
        // Feed the source file to the asset pipeline and ensure we get two jobs to be processed.
        // Register products for those jobs in the asset database.
        // Delete all products for one of those jobs and feed the source file to the asset pipeline, ensure that we get only one job to be processed.
        // Tell the mock builder to create one job now for the same source file and platform.
        // Feed the source file to the asset pipeline and ensure that we do not get any new jobs to be processed and also ensure that all the products of the missing jobs are deleted from disk.
        // Tell the mock builder to create two jobs again for the same source file and platform but having different job keys.
        // Feed the source file to the asset pipeline and ensure that we do get a new job to be process this time.

        // attach a file monitor to ensure this occurs.
        MockAssetBuilderInfoHandler mockAssetBuilderInfoHandler;

        QString sourceFile = m_sourceRoot.absoluteFilePath("subfolder1/basefile.foo");
        AssetProcessorManagerUnitTestUtils::CreateExpectedFiles({ sourceFile });

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 2; //Create two jobs for this file

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 2);
        for (int idx = 0; idx < m_processResults.size(); idx++)
        {
            EXPECT_EQ(m_processResults[idx].m_jobEntry.m_platformInfo.m_identifier, "pc");
            EXPECT_TRUE(m_processResults[idx].m_jobEntry.m_sourceAssetReference.RelativePath().Native().starts_with("basefile.foo"));
        }
        EXPECT_NE(m_processResults[0].m_jobEntry.m_jobKey.compare(m_processResults[1].m_jobEntry.m_jobKey), 0);

        QStringList pcouts;
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/basefile.arc1")));
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/basefile.arc2")));

        // Create the product files for the first job
        EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));
        EXPECT_TRUE(CreateDummyFile(pcouts[1], "product2"));


        // Invoke Asset Processed for pc platform for the first job
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("basefile.arc1", AZ::Uuid::CreateNull(), 1));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("basefile.arc2", AZ::Uuid::CreateNull(), 2));

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        EXPECT_EQ(m_assetMessages.size(), 2);

        EXPECT_EQ(m_changedInputResults.size(), 1);

        EXPECT_EQ(m_assetMessages[0].m_platform, "pc");
        EXPECT_EQ(m_assetMessages[1].m_platform, "pc");
        EXPECT_EQ(m_assetMessages[0].m_data, "basefile.arc1");
        EXPECT_EQ(m_assetMessages[1].m_data, "basefile.arc2");
        EXPECT_EQ(m_assetMessages[0].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        EXPECT_EQ(m_assetMessages[1].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_changedInputResults[0].first), AssetUtilities::NormalizeFilePath(sourceFile));

        pcouts.clear();
        pcouts.push_back(m_cacheRoot.filePath(QString("pc/basefile.arc3")));
        // Create the product files for the second job
        EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));

        // Invoke Asset Processed for pc platform for the second job
        response.m_outputProducts.clear();
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("basefile.arc3"));
        m_assetMessages.clear();
        m_changedInputResults.clear();
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_EQ(m_assetMessages.size(), 1);

        EXPECT_EQ(m_changedInputResults.size(), 1);

        EXPECT_EQ(m_assetMessages[0].m_platform, "pc");
        EXPECT_EQ(m_assetMessages[0].m_data, "basefile.arc3");
        EXPECT_EQ(m_assetMessages[0].m_type, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        EXPECT_EQ(AssetUtilities::NormalizeFilePath(m_changedInputResults[0].first), AssetUtilities::NormalizeFilePath(sourceFile));

        //Delete the product of the second job
        EXPECT_TRUE(QFile::remove(pcouts[0]));

        m_processResults.clear();

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        AssetProcessorManagerUnitTestUtils::SortAssetToProcessResultList(m_processResults);

        EXPECT_EQ(m_processResults.size(), 1); // We should only have one job to process here
        for (int idx = 0; idx < m_processResults.size(); idx++)
        {
            EXPECT_EQ(m_processResults[idx].m_jobEntry.m_platformInfo.m_identifier, "pc");
            EXPECT_TRUE(m_processResults[idx].m_jobEntry.m_sourceAssetReference.RelativePath().Native().starts_with("basefile.foo"));
        }

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 1; //Create one job for this file this time

        m_processResults.clear();

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        EXPECT_EQ(m_processResults.size(), 0); // We should not have any job to process here

        // products of the second job should not exists any longer
        for (QString outFile : pcouts)
        {
            EXPECT_FALSE(QFile::exists(pcouts[0]));
        }

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 2; //Again create two jobs for this file, this should result in one additional job

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        EXPECT_EQ(m_processResults.size(), 1); // We should see a job to process here
        for (int idx = 0; idx < m_processResults.size(); idx++)
        {
            EXPECT_EQ(m_processResults[idx].m_jobEntry.m_platformInfo.m_identifier, "pc");
            EXPECT_TRUE(m_processResults[idx].m_jobEntry.m_sourceAssetReference.RelativePath().Native().starts_with("basefile.foo"));
        }

        mockAssetBuilderInfoHandler.BusDisconnect();
    }

    TEST_F(AssetProcessorManagerUnitTests, ValidateScanFolders_ModifyPortableKeys_GetCorrectScanFolderIds)
    {
        using namespace AzToolsFramework::AssetDatabase;

        ScanFolderDatabaseEntryContainer entryContainer;
        auto puller = [&entryContainer](ScanFolderDatabaseEntry& entry)
            {
                entryContainer.push_back(entry);
                return true;
            };

        {
            AssetDatabaseConnection connection;
            EXPECT_TRUE(connection.OpenDatabase());
            // make sure we find the scan folders.
            entryContainer.clear();
            connection.QueryScanFoldersTable(puller);
            EXPECT_EQ(m_config.GetScanFolderCount(), entryContainer.size());
            // make sure they are all present and have port key:
            for (int idx = 0; idx < m_config.GetScanFolderCount(); ++idx)
            {
                AssetProcessor::ScanFolderInfo& scanFolderInConfig = m_config.GetScanFolderAt(idx);
                auto found = AZStd::find_if(entryContainer.begin(), entryContainer.end(), [&scanFolderInConfig](const ScanFolderDatabaseEntry& target)
                        {
                            return (
                                (target.m_scanFolderID == scanFolderInConfig.ScanFolderID()) &&
                                (scanFolderInConfig.GetPortableKey() == target.m_portableKey.c_str()) &&
                                (scanFolderInConfig.ScanPath() == target.m_scanFolder.c_str()) &&
                                (scanFolderInConfig.GetDisplayName() == target.m_displayName.c_str())
                                );
                        }
                        );

                EXPECT_NE(found, entryContainer.end());
            }
        }

        // now make a different config with different scan folders but with some of the same portable keys but new paths.
        PlatformConfiguration config2;
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms2;
        config2.PopulatePlatformsForScanFolder(platforms2);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse platforms order
        // case 1:  same absolute path, but the same portable key - should use same ID as before.
        config2.AddScanFolder(ScanFolderInfo(m_sourceRoot.filePath("subfolder4"), "subfolder4", "subfolder4", false, false, platforms2, -6)); // subfolder 4 overrides subfolder3

        // case 2:  A new absolute path, but same portable key - should use same id as before
        config2.AddScanFolder(ScanFolderInfo(m_sourceRoot.filePath("newfolder3"), "subfolder3", "subfolder3", false, false, platforms2, -5)); // subfolder 3 overrides subfolder2

        // case 3:  same absolute path, new portable key - should use a new ID
        config2.AddScanFolder(ScanFolderInfo(m_sourceRoot.filePath("subfolder1"), "subfolder3", "newfolder3", false, false, platforms2, -5)); // subfolder 3 overrides subfolder2

        // case 4:  subfolder2 is missing - it should be gone.

        {
            // create this, which will write those scan folders into the db as-is
            AssetProcessorManager_Test apm(&config2);
            apm.CheckMissingFiles();
        }

        {
            AssetDatabaseConnection connection;
            EXPECT_TRUE(connection.OpenDatabase());
            // make sure we find the scan folders.
            entryContainer.clear();
            connection.QueryScanFoldersTable(puller);
            EXPECT_EQ(config2.GetScanFolderCount(), entryContainer.size());

            // make sure they are all present and have port key:
            for (int idx = 0; idx < config2.GetScanFolderCount(); ++idx)
            {
                AssetProcessor::ScanFolderInfo& scanFolderInConfig = config2.GetScanFolderAt(idx);
                auto found = AZStd::find_if(entryContainer.begin(), entryContainer.end(), [&scanFolderInConfig](const ScanFolderDatabaseEntry& target)
                        {
                            return (
                                (target.m_scanFolderID == scanFolderInConfig.ScanFolderID()) &&
                                (scanFolderInConfig.GetPortableKey() == target.m_portableKey.c_str()) &&
                                (scanFolderInConfig.ScanPath() == target.m_scanFolder.c_str()) &&
                                (scanFolderInConfig.GetDisplayName() == target.m_displayName.c_str())
                                );
                        }
                        );

                EXPECT_NE(found, entryContainer.end());
            }
        }

        const AssetProcessor::ScanFolderInfo* subfolder4InConfig1 = nullptr;
        const AssetProcessor::ScanFolderInfo* subfolder4InConfig2 = nullptr;

        const AssetProcessor::ScanFolderInfo* subfolder3InConfig1 = nullptr;
        const AssetProcessor::ScanFolderInfo* subfolder3InConfig2 = nullptr;

        AZStd::unordered_set<AZ::s64> idsInConfig1;

        for (int idx = 0; idx < m_config.GetScanFolderCount(); ++idx)
        {
            AssetProcessor::ScanFolderInfo& scanFolderInConfig = m_config.GetScanFolderAt(idx);
            idsInConfig1.insert(scanFolderInConfig.ScanFolderID());

            if (scanFolderInConfig.GetPortableKey() == "subfolder4")
            {
                subfolder4InConfig1 = &scanFolderInConfig;
            }

            if (scanFolderInConfig.GetPortableKey() == "subfolder3")
            {
                subfolder3InConfig1 = &scanFolderInConfig;
            }
        }

        for (int idx = 0; idx < config2.GetScanFolderCount(); ++idx)
        {
            AssetProcessor::ScanFolderInfo& scanFolderInConfig = m_config.GetScanFolderAt(idx);

            if (scanFolderInConfig.GetPortableKey() == "subfolder4")
            {
                subfolder4InConfig2 = &scanFolderInConfig;
            }

            if (scanFolderInConfig.GetPortableKey() == "subfolder3")
            {
                subfolder3InConfig2 = &scanFolderInConfig;
            }

            if (scanFolderInConfig.GetPortableKey() == "newfolder3")
            {
                // it must be a new ID, so it can't reuse any ids.
                EXPECT_EQ(idsInConfig1.find(scanFolderInConfig.ScanFolderID()), idsInConfig1.end()); // must not be found
            }
        }

        EXPECT_TRUE(subfolder3InConfig2);
        EXPECT_TRUE(subfolder3InConfig1);

        EXPECT_TRUE(subfolder4InConfig2);
        EXPECT_TRUE(subfolder4InConfig1);

        // the above scan folders should not have changed id
        EXPECT_EQ(subfolder3InConfig1->ScanFolderID(), subfolder3InConfig2->ScanFolderID());
        EXPECT_EQ(subfolder4InConfig1->ScanFolderID(), subfolder4InConfig2->ScanFolderID());
    }

    TEST_F(AssetProcessorManagerUnitTests, ValidateJobDependencies_FeedHierarchyOfFiles_JobsProcessedInOrder)
    {
        // in this test, we create a hierarchy of files
        // where Job C depends on job B, which depends on job A.
        // if all three are in the queue, then Job A should be thus the first to be allowed to proceed.

        using namespace AzToolsFramework::AssetDatabase;

        AZ::Uuid builderUuid  = AZ::Uuid::CreateString("{3A1E7DE0-3E89-4F52-8B2D-B822D137D4F0}");

        AZ::Uuid sourceFileBUuid;
        bool fileBJobDependentOnFileAJob = false;
        bool changeJobAFingerprint = false;
        bool fileCJobDependentOnFileBJob = false;

        AssetProcessorManagerUnitTestUtils::MockAssetBuilderInfoHandler assetBuilderInfoHandler;
        assetBuilderInfoHandler.m_assetBuilderDesc.m_name = "Job Dependency UnitTest";
        assetBuilderInfoHandler.m_assetBuilderDesc.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        assetBuilderInfoHandler.m_assetBuilderDesc.m_busId = builderUuid;
        assetBuilderInfoHandler.m_assetBuilderDesc.m_analysisFingerprint = "xyz"; // Normally this would include the same fingerprint info from the job but for the purposes of testing, we just need something here
        assetBuilderInfoHandler.m_assetBuilderDesc.m_createJobFunction = [&fileBJobDependentOnFileAJob, &changeJobAFingerprint, &fileCJobDependentOnFileBJob, &sourceFileBUuid]
        (const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "xxx";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                AssetBuilderSDK::SourceFileDependency sourceFileDependency;
                QString sourceFile(request.m_sourceFile.c_str());

                // if we are analyzing job B...
                if (fileBJobDependentOnFileAJob && sourceFile.endsWith("FileB.txt"))
                {
                    AssetBuilderSDK::JobDescriptor secondDescriptor = descriptor;
                    secondDescriptor.m_jobKey = "yyy";
                    #if defined(AZ_PLATFORM_WINDOWS)
                    sourceFileDependency.m_sourceFileDependencyPath = "some\\random/Folders/FILEa.TxT";
                    #else
                    sourceFileDependency.m_sourceFileDependencyPath = "some/random/folders/FileA.txt";
                    #endif // defined(AZ_PLATFORM_WINDOWS)

                    // ... declare a job dependency on job A ('FileA.txt', 'xxx', platform)
                    AssetBuilderSDK::JobDependency jobDependency("xxx", platformInfo.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Fingerprint, sourceFileDependency);
                    secondDescriptor.m_jobDependencyList.push_back(jobDependency);
                    response.m_createJobOutputs.push_back(secondDescriptor);
                }
                else if (changeJobAFingerprint && sourceFile.endsWith("FileA.txt"))
                {
                    // if we are analyzing job A...
                    descriptor.m_additionalFingerprintInfo = "data";
                }
                else if (fileCJobDependentOnFileBJob && sourceFile.endsWith("FileC.txt"))
                {
                    // if we are analyzing job C...
                    AssetBuilderSDK::JobDescriptor secondDescriptor = descriptor;
                    secondDescriptor.m_jobKey = "zzz";
                    sourceFileDependency.m_sourceFileDependencyUUID = sourceFileBUuid;
                    // ... declare a job dependency on job B ('FileB.txt', 'yyy', platform)
                    AssetBuilderSDK::JobDependency jobDependency("yyy", platformInfo.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Fingerprint, sourceFileDependency);
                    secondDescriptor.m_jobDependencyList.push_back(jobDependency);
                    response.m_createJobOutputs.push_back(secondDescriptor);
                }

                response.m_createJobOutputs.push_back(descriptor);
            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        };

        assetBuilderInfoHandler.BusConnect();

        QString sourceFileAPath = m_sourceRoot.absoluteFilePath("subfolder1/some/random/folders/FileA.txt");
        QString sourceFileBPath = m_sourceRoot.absoluteFilePath("subfolder1/FileB.txt");
        QString sourceFileCPath = m_sourceRoot.absoluteFilePath("FileC.txt");
        sourceFileBUuid = AssetUtilities::CreateSafeSourceUUIDFromName("FileB.txt");

        constexpr const char* productFileAFilename = "fileaproduct.txt";
        constexpr const char* productFileBFilename = "filebproduct1.txt";
        constexpr const char* product2FileBFilename = "filebproduct2.txt";
        constexpr const char* productFileCFilename = "filecproduct.txt";
        constexpr const char* product2FileCFilename = "filecproduct2.txt";

        QString productFileAPath = m_cacheRoot.filePath(QString("pc/") + productFileAFilename);
        QString productFileBPath = m_cacheRoot.filePath(QString("pc/") + productFileBFilename);
        QString product2FileBPath = m_cacheRoot.filePath(QString("pc/") + product2FileBFilename);
        QString productFileCPath = m_cacheRoot.filePath(QString("pc/") + productFileCFilename);
        QString product2FileCPath = m_cacheRoot.filePath(QString("pc/") + product2FileCFilename);

        EXPECT_TRUE(CreateDummyFile(sourceFileAPath, ""));
        EXPECT_TRUE(CreateDummyFile(sourceFileBPath, ""));
        EXPECT_TRUE(CreateDummyFile(sourceFileCPath, ""));
        EXPECT_TRUE(CreateDummyFile(productFileAPath, "product"));
        EXPECT_TRUE(CreateDummyFile(productFileBPath, "product"));
        EXPECT_TRUE(CreateDummyFile(product2FileBPath, "product"));
        EXPECT_TRUE(CreateDummyFile(productFileCPath, "product"));
        EXPECT_TRUE(CreateDummyFile(product2FileCPath, "product"));

        AZStd::string cacheWithPlatform = m_cacheRoot.absoluteFilePath("pc").toUtf8().constData();

        m_config.EnablePlatform({ "android",{ "mobile", "renderer" } }, false);

        // Analyze FileA
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        EXPECT_TRUE(BlockUntil(m_idling, 500000));

        EXPECT_EQ(m_processResults.size(), 1);
        EXPECT_FALSE(m_processResults[0].m_jobDependencyList.size());

        // Invoke Asset Processed for pc platform for the FileA job
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileAFilename));
        response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        m_processResults.clear();
        response.m_outputProducts.clear();

        // Analyze FileB, one of the jobs should declare a job dependency on the FileA job
        fileBJobDependentOnFileAJob = true;
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileBPath));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_EQ(m_processResults.size(), 2);
        bool onlyOneJobHaveJobDependency = false;
        for (JobDetails& jobDetail : m_processResults)
        {
            if (jobDetail.m_jobDependencyList.size())
            {
                EXPECT_FALSE(onlyOneJobHaveJobDependency);
                onlyOneJobHaveJobDependency = true;
                EXPECT_EQ(jobDetail.m_jobDependencyList.size(), 1);
                JobDependencyInternal& jobDependencyInternal = jobDetail.m_jobDependencyList[0];
                EXPECT_NE(jobDependencyInternal.m_builderUuidList.find(builderUuid), jobDependencyInternal.m_builderUuidList.end());
                EXPECT_TRUE(QString(jobDependencyInternal.m_jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str()).endsWith("FileA.txt", Qt::CaseSensitivity::CaseInsensitive));
            }
        }

        EXPECT_TRUE(onlyOneJobHaveJobDependency);

        // Invoke Asset Processed for pc platform for the first FileB job
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileBFilename));
        response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        response.m_outputProducts.clear();

        // Invoke Asset Processed for pc platform for the second FileB job
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2FileBFilename));
        response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, m_processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        EXPECT_TRUE(BlockUntil(m_idling, 5000000));

        m_processResults.clear();
        response.m_outputProducts.clear();

        // Change the fingerprint of the FileA job and analyze the file again
        // This time it should not only process its job again but should also process the dependent FileB job
        changeJobAFingerprint = true;
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_EQ(m_processResults.size(), 2);

        for (JobDetails& jobDetail : m_processResults)
        {
            EXPECT_EQ(m_processResults.size(), 2); // Repeat to ensure count doesn't change while looping

            if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileB.txt"))
            {
                // Ensure that we are processing the right FileB job
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy"), 0);

                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2FileBFilename));
                response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;
                QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                EXPECT_TRUE(BlockUntil(m_idling, 5000));
            }
            else
            {
                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileAFilename));
                response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;
                QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                EXPECT_TRUE(BlockUntil(m_idling, 5000));
            }
        }

        m_processResults.clear();
        response.m_outputProducts.clear();

        // Modify FileA and analyze the file again.
        // This time also it should not only process its job again but should also process the dependent FileB job
        EXPECT_TRUE(QFile::remove(sourceFileAPath));
        EXPECT_TRUE(CreateDummyFile(sourceFileAPath, "changed"));

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_EQ(m_processResults.size(), 2);

        for (JobDetails& jobDetail : m_processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileB.txt"))
            {
                // Ensure that we are processing the right FileB job
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy"), 0);

                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2FileBFilename));
                response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;
                QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                EXPECT_TRUE(BlockUntil(m_idling, 5000));
            }
            else
            {
                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileAFilename));
                response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;
                QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                EXPECT_TRUE(BlockUntil(m_idling, 5000));
            }
        }

        // First we will analyze File C
        // This should make Job("FileC","zzz", "pc") depends on Job("FileB", "yyy", "pc") which already depends on Job("FileA", "xxx", "pc")
        // After that we will change the fingerprint of Job("FileA", "xxx", "pc") and analyze FileA again,
        // which should process all the three jobs once again.
        m_processResults.clear();

        fileCJobDependentOnFileBJob = true;
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileCPath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_EQ(m_processResults.size(), 2);

        for (JobDetails& jobDetail : m_processResults)
        {
            EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileC.txt"));
            if (jobDetail.m_jobDependencyList.size())
            {
                // Verify FileC jobinfo
                AssetBuilderSDK::SourceFileDependency& source = jobDetail.m_jobDependencyList[0].m_jobDependency.m_sourceFile;
                EXPECT_EQ(source.m_sourceFileDependencyUUID, sourceFileBUuid);
                EXPECT_EQ(QString(jobDetail.m_jobDependencyList[0].m_jobDependency.m_jobKey.c_str()).compare("yyy"), 0);

                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2FileCFilename));
                response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;
                QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                EXPECT_TRUE(BlockUntil(m_idling, 5000));
            }
            else
            {
                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileCFilename));
                response.m_outputProducts.back().m_outputPathOverride = cacheWithPlatform;
                QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                EXPECT_TRUE(BlockUntil(m_idling, 5000));
            }
        }

        m_processResults.clear();
        // Modify fingerprint of Job("FileA", "xxx", "pc") and analyze FileA again,
        changeJobAFingerprint = false; // This will revert back the changes in the extra info used for fingerprinting of this job

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        //One of the FileC job("FileC.txt","zzz") depends on the FileB job("FileB.txt", "yyy") which depends on FileA job("FileA.txt", "xxx")
        EXPECT_EQ(m_processResults.size(), 3);

        for (JobDetails& jobDetail : m_processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileA.txt"))
            {
                // Verify FileA jobinfo
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("xxx"), 0);
            }
            else if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileB.txt"))
            {
                // Verify FileB jobinfo
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy"), 0);
            }
            else if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileC.txt"))
            {
                // Verify FileC jobinfo
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("zzz"), 0);
            }
        }


        // Since one of the FileC job("FileC.txt","zzz") have emitted a job dependency on a FileB job("FileB.txt", "yyy")
        // which also have a job dependency on a FileA job("FileA.txt", "xxx") therefore deleting File A source file should
        // cause both jobs (File B and File C) to be processed again.

        m_processResults.clear();

        QFile::remove(sourceFileAPath);

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));

        EXPECT_EQ(m_processResults.size(), 2);

        for (JobDetails& jobDetail : m_processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileB.txt"))
            {
                // Verify FileB jobinfo
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy"), 0);
            }
            else if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileC.txt"))
            {
                // Verify FileC jobinfo
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("zzz"), 0);
            }
            else
            {
                // invalid job info
                EXPECT_TRUE(false);
            }
        }
        m_processResults.clear();

        // Adding FileA back should cause all the three jobs to be processed again.

        EXPECT_TRUE(CreateDummyFile(sourceFileAPath, "reappear"));

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString , sourceFileAPath));

        EXPECT_TRUE(BlockUntil(m_idling, 5000));


        EXPECT_EQ(m_processResults.size(), 3);

        for (JobDetails& jobDetail : m_processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileA.txt"))
            {
                // Verify FileA jobinfo
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("xxx"), 0);
            }
            else if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileB.txt"))
            {
                // Verify FileB jobinfo
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy"), 0);
            }
            else if (QString(jobDetail.m_jobEntry.m_sourceAssetReference.RelativePath().c_str()).endsWith("FileC.txt"))
            {
                // Verify FileC jobinfo
                EXPECT_EQ(QString(jobDetail.m_jobEntry.m_jobKey).compare("zzz"), 0);
            }
        }

        assetBuilderInfoHandler.BusDisconnect();
    }
} // namespace AssetProcessor
