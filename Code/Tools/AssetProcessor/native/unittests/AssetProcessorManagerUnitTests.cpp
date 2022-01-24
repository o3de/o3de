/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/base.h>

#include "AssetProcessorManagerUnitTests.h"

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include "MockApplicationManager.h"
#include "native/FileWatcher/FileWatcher.h"
#include "native/unittests/MockConnectionHandler.h"

#include <AzTest/AzTest.h>

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

        friend class AssetProcessorManagerUnitTests_ScanFolders;

#if !AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS
        friend class AssetProcessorManagerUnitTests;
        friend class AssetProcessorManagerUnitTests_JobKeys;
        friend class AssetProcessorManagerUnitTests_JobDependencies_Fingerprint;
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS

    public:
        using GetRelativeProductPathFromFullSourceOrProductPathRequest = AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathRequest;
        using GetRelativeProductPathFromFullSourceOrProductPathResponse = AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathResponse;
        using GenerateRelativeSourcePathRequest = AzFramework::AssetSystem::GenerateRelativeSourcePathRequest;
        using GenerateRelativeSourcePathResponse = AzFramework::AssetSystem::GenerateRelativeSourcePathResponse;
        using GetFullSourcePathFromRelativeProductPathRequest = AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathRequest;
        using GetFullSourcePathFromRelativeProductPathResponse = AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathResponse;
    };



    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests_ScanFolders)

#if !AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS
    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests)
    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests_JobKeys)
    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests_JobDependencies_Fingerprint)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS


    namespace
    {
        constexpr const char* AssetProcessorManagerTestGameProject = "AutomatedTesting";
        /// This functions sorts the processed result list by platform name
        /// if platform is same than it sorts by job description
        void sortAssetToProcessResultList(QList<JobDetails>& processResults)
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

            //AZ_TracePrintf("test", "-------------------------\n");
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
                extraInfoForPC.append(assetRecogniser->m_platformSpecs["pc"].m_extraRCParams);
                extraInfoForANDROID.append(assetRecogniser->m_platformSpecs["android"].m_extraRCParams);
                extraInfoForPC.append(assetRecogniser->m_version);
                extraInfoForANDROID.append(assetRecogniser->m_version);
            }

            //Calculating fingerprints for the file for pc and android platforms
            AZ::Uuid sourceId = AZ::Uuid("{2206A6E0-FDBC-45DE-B6FE-C2FC63020BD5}");
            JobEntry jobEntryPC(scanFolderPath, relPath, relPath, 0, { "pc", {"desktop", "renderer"} }, "", 0, 1, sourceId);
            JobEntry jobEntryANDROID(scanFolderPath, relPath, relPath, 0, { "android", {"mobile", "renderer"} }, "", 0, 2, sourceId);

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

    void AssetProcessorManagerUnitTests::StartTest()
    {
        // the asset processor manager is generally sitting on top of many other systems.
        // we have tested those systems individually in other unit tests, but we need to create
        // a simulated environment to test the manager itself.
        // for the manager, the only things we care about is that it emits the correct signals
        // when the appropriate stimulus is given and that state is appropriately updated.

        // attach a file monitor to ensure this occurs.

        MockApplicationManager  mockAppManager;
        mockAppManager.BusConnect();

        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();
        FileWatcher fileWatcher;

        UNIT_TEST_EXPECT_TRUE(QDir::temp().exists());
        QString tmpDirPath = AssetUtilities::NormalizeDirectoryPath(QDir::tempPath());

        QTemporaryDir dir;
        // We use 'canonical' paths here so that platforms that have junctions/symlinks return the same path
        // regardless.  Otherwise things like QDir::current() might return a different path than what happens if you
        // call TempPath::absolutePath() despite actually being the same path.  This problem is unique to these unit tests
        // since AP does not usually run normally inside a temp folder.

        QDir tempPath(dir.path());
        QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
        UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
        NetworkRequestID requestId(1, 1);

        QString gameName = AssetUtilities::ComputeProjectName(AssetProcessorManagerTestGameProject);

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        // create a dummy file in the cache folder, so the folder structure gets created
        // Override the cache folder to be the within the temporary directory
        auto projectCacheRootKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/project_cache_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Set(projectCacheRootKey, tempPath.absoluteFilePath("Cache").toUtf8().data());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*settingsRegistry);
        }
        QDir projectCacheRoot;
        AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);
        CreateDummyFile(projectCacheRoot.absoluteFilePath("placeholder.txt"));

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());
        // should create cache folder in the root, and read everything from there.

        // There is a sub-case of handling mixed cases, but is only supported on case-insensitive filesystems. 
#if defined(AZ_PLATFORM_LINUX)
        // Linux is case-sensitive, so 'basefile.txt' will stay the same case as the other subfolder versions
        constexpr const char* subfolder3BaseFilePath = "subfolder3/basefile.txt";
        constexpr int expectedLegacyAssetIdCount = 1;
#else
        constexpr const char* subfolder3BaseFilePath = "subfolder3/BaseFile.txt";
        constexpr int expectedLegacyAssetIdCount = 2;
#endif

        QSet<QString> expectedFiles;
        // set up some interesting files:
        expectedFiles << tempPath.absoluteFilePath("rootfile2.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rootfile1.txt"); // note:  Must override the actual root file
        expectedFiles << tempPath.absoluteFilePath("subfolder1/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");

        expectedFiles << tempPath.absoluteFilePath(subfolder3BaseFilePath); 

        expectedFiles << tempPath.absoluteFilePath("subfolder8/a/b/c/test.txt");

        // subfolder3 is not recursive so none of these should show up in any scan or override check
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");

        expectedFiles << tempPath.absoluteFilePath("subfolder3/uniquefile.txt"); // only exists in subfolder3
        expectedFiles << tempPath.absoluteFilePath("subfolder3/uniquefile.ignore"); // only exists in subfolder3

        expectedFiles << tempPath.absoluteFilePath("subfolder3/rootfile3.txt"); // must override rootfile3 in root
        expectedFiles << tempPath.absoluteFilePath("rootfile1.txt");
        expectedFiles << tempPath.absoluteFilePath("rootfile3.txt");
        expectedFiles << tempPath.absoluteFilePath("unrecognised.file"); // a file that should not be recognised
        expectedFiles << tempPath.absoluteFilePath("unrecognised2.file"); // a file that should not be recognised
        expectedFiles << tempPath.absoluteFilePath("subfolder1/test/test.format"); // a file that should be recognised
        expectedFiles << tempPath.absoluteFilePath("test.format"); // a file that should NOT be recognised
        expectedFiles << tempPath.absoluteFilePath("subfolder3/somefile.xxx");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/savebackup/test.txt");//file that should be excluded
        expectedFiles << tempPath.absoluteFilePath("subfolder3/somerandomfile.random");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/folder/ship.tiff");

        // these will be used for the "check lock" tests
        expectedFiles << tempPath.absoluteFilePath("subfolder4/needsLock.tiff");
        expectedFiles << tempPath.absoluteFilePath("subfolder4/noLockNeeded.txt");

        // this will be used for the "rename a folder" test.
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rename_this/somefile1.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rename_this/somefolder/somefile2.txt");

        // this will be used for the "rename a folder" test with deep folders that don't contain files:
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rename_this_secondly/somefolder/somefile2.txt");

        // this will be used for the "delete a SOURCE file" test.
        expectedFiles << tempPath.absoluteFilePath("subfolder1/to_be_deleted/some_deleted_file.txt");

        // this will be used for the "fewer products than last time" test.
        expectedFiles << tempPath.absoluteFilePath("subfolder1/fewer_products/test.txt");

        for (const QString& expect : expectedFiles)
        {
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(expect));
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Created file %s with msecs %llu\n", expect.toUtf8().constData(),
                QFileInfo(expect).lastModified().toMSecsSinceEpoch());

#if defined(AZ_PLATFORM_WINDOWS)
            QThread::msleep(35); // give at least some milliseconds so that the files never share the same timestamp exactly
#else
            // on platforms such as mac, the file time resolution is only a second :(
            QThread::msleep(1001);
#endif
        }


        PlatformConfiguration config;
        config.EnablePlatform({ "pc",{ "desktop", "renderer" } }, true);
        config.EnablePlatform({ "android",{ "mobile", "renderer" } }, true);
        config.EnablePlatform({ "fandago",{ "console", "renderer" } }, false);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse  platforms   order
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", false, false, platforms, -6)); // subfolder 4 overrides subfolder3
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3", false, false, platforms,-5)); // subfolder 3 overrides subfolder2
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2", false, true,  platforms, -2)); // subfolder 2 overrides subfolder1
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", false, true,  platforms, -1)); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(),         "temp",       "tempfolder", true, false,  platforms, 0)); // add the root


        config.AddMetaDataType("exportsettings", QString());

        AZ::Uuid buildIDRcLegacy;
        BUILDER_ID_RC.GetUuid(buildIDRcLegacy);

        AssetRecognizer rec;
        AssetPlatformSpec specpc;
        AssetPlatformSpec specandroid;

        specandroid.m_extraRCParams = "somerandomparam";
        rec.m_name = "random files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.random", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert("pc", specpc);
        config.AddRecognizer(rec);
        UNIT_TEST_EXPECT_TRUE(mockAppManager.RegisterAssetRecognizerAsBuilder(rec));

        specpc.m_extraRCParams = ""; // blank must work
        specandroid.m_extraRCParams = "testextraparams";

        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_platformSpecs.insert("android", specandroid);

        config.AddRecognizer(rec);

        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // test dual-recognisers - two recognisers for the same pattern.
        const char* builderTxt2Name = "txt files 2 (builder2)";
        rec.m_name = builderTxt2Name;
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // tiff file recognizer
        rec.m_name = "tiff files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.tiff", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.clear();
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_testLockSource = true;
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_platformSpecs.clear();
        rec.m_testLockSource = false;

        specpc.m_extraRCParams = "pcparams";
        specandroid.m_extraRCParams = "androidparams";

        rec.m_name = "xxx files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.xxx", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_platformSpecs.insert("android", specandroid);
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // two recognizers for the same pattern.
        rec.m_name = "xxx files 2 (builder2)";
        specpc.m_extraRCParams = "pcparams2";
        specandroid.m_extraRCParams = "androidparams2";
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_platformSpecs.insert("android", specandroid);
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // Ignore recognizer
        AssetPlatformSpec ignore_spec;
        ignore_spec.m_extraRCParams = "skip";
        AssetRecognizer ignore_rec;
        ignore_rec.m_name = "ignore files";
        ignore_rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ignore", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        ignore_rec.m_platformSpecs.insert("pc", specpc);
        ignore_rec.m_platformSpecs.insert("android", ignore_spec);
        config.AddRecognizer(ignore_rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(ignore_rec);


        ExcludeAssetRecognizer excludeRecogniser;
        excludeRecogniser.m_name = "backup";
        excludeRecogniser.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("(^|.+/)savebackup/.*", AssetBuilderSDK::AssetBuilderPattern::Regex);
        config.AddExcludeRecognizer(excludeRecogniser);

        AssetProcessorManager_Test apm(&config);  // note, this will 'push' the scan folders in to the db.
        QDir cacheRoot;
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(cacheRoot));
        QString normalizedCacheRoot = AssetUtilities::NormalizeDirectoryPath(cacheRoot.canonicalPath());

        // make sure it picked up the one in the cache and not for example the real working folder

        QString normalizedDirPathCheck = AssetUtilities::NormalizeDirectoryPath(QDir(canonicalTempDirPath).absoluteFilePath("Cache"));
        UNIT_TEST_EXPECT_TRUE(normalizedCacheRoot == normalizedDirPathCheck);
        QDir normalizedCacheRootDir(normalizedCacheRoot);

        QList<JobDetails> processResults;
        QList<QPair<QString, QString> > changedInputResults;
        QList<AzFramework::AssetSystem::AssetNotificationMessage>  assetMessages;

        bool idling = false;

        auto assetToProcessConnection = connect(&apm, &AssetProcessorManager::AssetToProcess,
            this, [&processResults](JobDetails details)
            {
                //AZ_TracePrintf("Results", "ProcessResult: %s - %s - %s - %s - %u\n", file.toUtf8().constData(), platform.toUtf8().constData(), jobDescription.toUtf8().constData(), destination.toUtf8().constData(), originalFingerprint);
                processResults.push_back(AZStd::move(details));
            });

        auto assetMessageConnection = connect(&apm, &AssetProcessorManager::AssetMessage,
            this, [&assetMessages](AzFramework::AssetSystem::AssetNotificationMessage message)
            {
                assetMessages.push_back( message);
            });

        auto inputAssetProcessedConnection = connect(&apm, &AssetProcessorManager::InputAssetProcessed,
            this, [&changedInputResults](QString relativePath, QString platform)
            {
                changedInputResults.push_back(QPair<QString, QString>(relativePath, platform));
            });

        auto assetProcessorManagerIdleStateConnection = connect(&apm, &AssetProcessorManager::AssetProcessorManagerIdleState,
            this, [&idling](bool state)
            {
                idling = state;
            }
            );

        AssetProcessor::MockConnectionHandler connection;
        connection.BusConnect(1);
        QList<QPair<unsigned int, QByteArray> > payloadList;
        connection.m_callback = [&](unsigned int type, [[maybe_unused]] unsigned int serial, const QByteArray payload)
            {
                payloadList.append(qMakePair(type, payload));
            };

        // run the tests.
        // first, feed it things which it SHOULD ignore and should NOT generate any tasks:

        // the following is a file which does exist but should not be processed as it is in a non-watched folder (not recursive)
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString,  tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        // an imaginary non-existent file should also fail even if it matches filters:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString,  tempPath.absoluteFilePath("subfolder3/basefileaaaaa.txt")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString,  tempPath.absoluteFilePath("basefileaaaaa.txt")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        processResults.clear();

        QString inputIgnoreFilePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/uniquefile.ignore"));

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputIgnoreFilePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // 1, since we have one recognizer for .ignore, but the 'android' platform is marked as skip
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc"));


        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        processResults.clear();

        // give it a file that should actually cause the generation of a task:

        QString relativePathFromWatchFolder = "uniquefile.txt";
        QString watchFolderPath = tempPath.absoluteFilePath("subfolder3");
        QString absolutePath = AssetUtilities::NormalizeFilePath(watchFolderPath + "/" + relativePathFromWatchFolder);

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and android,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));


        QList<int> androidJobsIndex;
        QList<int> pcJobsIndex;
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_jobRunKey != 0);
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_watchFolderPath == AssetUtilities::NormalizeFilePath(watchFolderPath));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_pathRelativeToWatchFolder == "uniquefile.txt");
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_databaseSourceName == "uniquefile.txt");
            QString platformFolder = cacheRoot.filePath(QString::fromUtf8(processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()));
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_destinationPath.startsWith(platformFolder));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);

            QMetaObject::invokeMethod(&apm, "OnJobStatusChanged", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[checkIdx].m_jobEntry), Q_ARG(JobStatus, JobStatus::Queued));

            QCoreApplication::processEvents(QEventLoop::AllEvents);

            // create log files, so that we can test the correct retrieval

            // we create all of them except for #1
            if (checkIdx != 1)
            {
                JobInfo info;
                info.m_jobRunKey = processResults[checkIdx].m_jobEntry.m_jobRunKey;
                info.m_builderGuid = processResults[checkIdx].m_jobEntry.m_builderGuid;
                info.m_jobKey = processResults[checkIdx].m_jobEntry.m_jobKey.toUtf8().data();
                info.m_platform = processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str();
                info.m_sourceFile = processResults[checkIdx].m_jobEntry.m_pathRelativeToWatchFolder.toUtf8().data();
                info.m_watchFolder = processResults[checkIdx].m_jobEntry.m_watchFolderPath.toUtf8().data();

                AZStd::string logFolder = AZStd::string::format("%s/%s", AssetUtilities::ComputeJobLogFolder().c_str(), AssetUtilities::ComputeJobLogFileName(info).c_str());
                AZ::IO::HandleType logHandle;
                AZ::IO::LocalFileIO::GetInstance()->CreatePath(AssetUtilities::ComputeJobLogFolder().c_str());
                UNIT_TEST_EXPECT_TRUE(AZ::IO::LocalFileIO::GetInstance()->Open(logFolder.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, logHandle));
                AZStd::string logLine = AZStd::string::format("Log stored for job run key %lli\n", processResults[checkIdx].m_jobEntry.m_jobRunKey);
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

            apm.ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Queued);
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());
                UNIT_TEST_EXPECT_TRUE(jobInfo.m_jobRunKey != 0);

                for (const JobDetails& details : processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_pathRelativeToWatchFolder, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_watchFolderPath, Qt::CaseSensitive) == 0) &&
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
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        // ------------- JOB LOG TEST -------------------
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            const JobDetails& details = processResults[checkIdx];
            // create log files, so that we can test the correct retrieval

            // we create all of them except for #1
            if (checkIdx != 1)
            {
                AZStd::string logFolder = AZStd::string::format("%s/%s", AssetUtilities::ComputeJobLogFolder().c_str(), AssetUtilities::ComputeJobLogFileName(details.m_jobEntry).c_str());
                AZ::IO::HandleType logHandle;
                AZ::IO::LocalFileIO::GetInstance()->CreatePath(AssetUtilities::ComputeJobLogFolder().c_str());
                UNIT_TEST_EXPECT_TRUE(AZ::IO::LocalFileIO::GetInstance()->Open(logFolder.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, logHandle));
                AZStd::string logLine = AZStd::string::format("Log stored for job %u\n", processResults[checkIdx].m_jobEntry.GetHash());
                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
                AZ::IO::LocalFileIO::GetInstance()->Close(logHandle);
            }
        }

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            const JobDetails& details = processResults[checkIdx];

            // request job logs.
            AssetJobLogRequest requestLog;
            AssetJobLogResponse requestResponse;
            requestLog.m_jobRunKey = details.m_jobEntry.m_jobRunKey;
            {
                // send our request:
                apm.ProcessGetAssetJobLogRequest(requestLog, requestResponse);

                if (checkIdx != 1)
                {
                    UNIT_TEST_EXPECT_TRUE(requestResponse.m_isSuccess);
                    UNIT_TEST_EXPECT_FALSE(requestResponse.m_jobLog.empty());
                    AZStd::string checkString = AZStd::string::format("Log stored for job %u\n", processResults[checkIdx].m_jobEntry.GetHash());
                    UNIT_TEST_EXPECT_TRUE(requestResponse.m_jobLog.find(checkString.c_str()) != AZStd::string::npos);
                }
                else
                {
                    // the [1] index was not written so it should be failed and empty
                    UNIT_TEST_EXPECT_FALSE(requestResponse.m_isSuccess);
                }
            }
        }

        // now indicate the job has started.
        for (const JobDetails& details : processResults)
        {
            apm.OnJobStatusChanged(details.m_jobEntry, JobStatus::InProgress);
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
                apm.ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);
            }

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::InProgress);
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (const JobDetails& details : processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_pathRelativeToWatchFolder, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_watchFolderPath, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;
                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        // ---------- test successes ----------


        QStringList androidouts;
        androidouts.push_back(cacheRoot.filePath(QString("android/basefile.arc1")));
        androidouts.push_back(cacheRoot.filePath(QString("android/basefile.arc2")));

        // feed it the messages its waiting for (create the files)
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts[0], "products."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts[1], "products."))

        //Invoke Asset Processed for android platform , txt files job description
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts[0].toUtf8().constData(), AZ::Uuid::CreateNull(), 1));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts[1].toUtf8().constData(), AZ::Uuid::CreateNull(), 2));

        // make sure legacy SubIds get stored in the DB and in asset response messages.
        // also make sure they don't get filed for the wrong asset.
        response.m_outputProducts[0].m_legacySubIDs.push_back(1234);
        response.m_outputProducts[0].m_legacySubIDs.push_back(5678);
        response.m_outputProducts[1].m_legacySubIDs.push_back(2222);

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 2);

        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "android");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_platform == "android");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_data == "basefile.arc2");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_sizeBytes != 0);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_sizeBytes != 0);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_assetId.IsValid());
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_assetId.IsValid());
        UNIT_TEST_EXPECT_TRUE(!assetMessages[0].m_legacyAssetIds.empty());
        UNIT_TEST_EXPECT_TRUE(!assetMessages[1].m_legacyAssetIds.empty());
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_legacyAssetIds[0].IsValid());
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_legacyAssetIds[0].IsValid());
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_legacyAssetIds[0] != assetMessages[0].m_assetId);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_legacyAssetIds[0] != assetMessages[1].m_assetId);

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_legacyAssetIds.size() == 3);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_legacyAssetIds.size() == 2);

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_legacyAssetIds[1].m_subId == 1234);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_legacyAssetIds[2].m_subId == 5678);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_legacyAssetIds[1].m_subId == 2222);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(absolutePath));

        // ----------------------- test job info requests, when some assets are done.
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;
            bool escalated = false;
            int numEscalated = 0;

            requestInfo.m_escalateJobs = true;
            requestInfo.m_searchTerm = absolutePath.toUtf8().constData();
            auto connectionMade = QObject::connect(&apm, &AssetProcessorManager::EscalateJobs, this, [&escalated, &numEscalated](AssetProcessor::JobIdEscalationList jobList)

                    {
                        escalated = true;
                        numEscalated = jobList.size();
                    });

            AssetJobsInfoResponse jobResponse;
            // send our request:
            apm.ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);

            // wait for it to process:
            QCoreApplication::processEvents(QEventLoop::AllEvents);

            QObject::disconnect(connectionMade);

            UNIT_TEST_EXPECT_TRUE(escalated);
            UNIT_TEST_EXPECT_TRUE(numEscalated > 0);

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (int detailsIdx = 0; detailsIdx < processResults.size(); ++detailsIdx)
                {
                    const JobDetails& details = processResults[detailsIdx];

                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_pathRelativeToWatchFolder, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_watchFolderPath, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;

                        if (detailsIdx == 0) // we only said that the first job was done
                        {
                            UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Completed);
                        }
                        else
                        {
                            UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::InProgress);
                        }

                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        changedInputResults.clear();
        assetMessages.clear();

        androidouts.clear();
        androidouts.push_back(cacheRoot.filePath(QString("android/basefile.azm")));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts[0], "products."));

        //Invoke Asset Processed for android platform , txt files2 job description
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts[0].toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "android");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefile.azm");

        changedInputResults.clear();
        assetMessages.clear();

        QStringList pcouts;
        pcouts.push_back(cacheRoot.filePath(QString("pc/basefile.arc1")));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        //Invoke Asset Processed for pc platform , txt files job description
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefile.arc1");


        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(absolutePath));

        changedInputResults.clear();
        assetMessages.clear();

        pcouts.clear();
        pcouts.push_back(cacheRoot.filePath(QString("pc/basefile.azm")));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        //Invoke Asset Processed for pc platform , txt files 2 job description
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefile.azm");

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(absolutePath));

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
            apm.ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Completed);
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (const JobDetails& details : processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_pathRelativeToWatchFolder, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_watchFolderPath, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;
                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        // feed it the exact same file again.
        // this should result in NO ADDITIONAL processes since nothing has changed.
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        // delete one of the products and tell it that it changed
        // it should reprocess that file, for that platform only:

        payloadList.clear();
        connection.m_sent = false;

        AssetNotificationMessage assetNotifMessage;
        SourceFileNotificationMessage sourceFileChangedMessage;

        // this should result in NO ADDITIONAL processes since nothing has changed.
        UNIT_TEST_EXPECT_TRUE(QFile::remove(pcouts[0]));
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, pcouts[0]));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        // We should not be receiving any sourcefile notification message here since the source file hasn't changed
        UNIT_TEST_EXPECT_TRUE(payloadList.size() == 0);

        // should have asked to launch only the PC process because the other assets are already done for the other plat
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(processResults[0].m_jobEntry.GetAbsoluteSourcePath()) == AssetUtilities::NormalizeFilePath(absolutePath));

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "products2"));
        // tell it were done again!

        changedInputResults.clear();
        assetMessages.clear();

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefile.azm");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "pc");
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(absolutePath));

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        connection.m_sent = false;
        payloadList.clear();

        // modify the input file, then
        // feed it the exact same file again.
        // it should spawn BOTH compilers:
        UNIT_TEST_EXPECT_TRUE(QFile::remove(absolutePath));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(absolutePath, "new!"));
        AZ_TracePrintf(AssetProcessor::DebugChannel, "-------------------------------------------\n");

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(connection.m_sent);
        UNIT_TEST_EXPECT_TRUE(payloadList.size() == 1);// We should always receive only one of these messages
        UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(0).second.data(), payloadList.at(0).second.size(), sourceFileChangedMessage));
        QDir scanFolder(sourceFileChangedMessage.m_scanFolder.c_str());
        QString pathToCheck = scanFolder.filePath(sourceFileChangedMessage.m_relativeSourcePath.c_str());
        UNIT_TEST_EXPECT_TRUE(QString::compare(absolutePath, pathToCheck, Qt::CaseSensitive) == 0);

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and android,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);
        UNIT_TEST_EXPECT_TRUE(processResults[1].m_jobEntry.m_computedFingerprint != 0);

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = processResults[checkIdx].m_jobEntry.GetAbsoluteSourcePath();
            UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(processFile1) == AssetUtilities::NormalizeFilePath(absolutePath));
            QString platformFolder = cacheRoot.filePath(QString::fromUtf8(processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()));
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            processFile1 = processResults[checkIdx].m_destinationPath;
            UNIT_TEST_EXPECT_TRUE(processFile1.startsWith(platformFolder));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);
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
        androidouts.push_back(cacheRoot.filePath(QString("android/basefilea.arc1")));
        androidouts2.push_back(cacheRoot.filePath(QString("android/basefilea.azm")));
        // note that the android outs have changed
        // but the pc outs are still the same.
        pcouts.push_back(cacheRoot.filePath(QString("pc/basefile.arc1")));
        pcouts2.push_back(cacheRoot.filePath(QString("pc/basefile.azm")));

        // feed it the messages its waiting for (create the files)
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts2[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));

        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 50);

        changedInputResults.clear();
        assetMessages.clear();

        // send all the done messages simultaneously:

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 50);

        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 4);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 7);

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

        for (auto element : assetMessages)
        {
            if (element.m_data == "basefile.arc1")
            {
                if (element.m_platform == "pc")
                {
                    UNIT_TEST_EXPECT_TRUE(element.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
                }
                else
                {
                    UNIT_TEST_EXPECT_TRUE(element.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
                }
            }

            if (element.m_data == "basefilea.arc1")
            {
                UNIT_TEST_EXPECT_TRUE(element.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
                UNIT_TEST_EXPECT_TRUE(element.m_platform == "android");
            }

            if (element.m_data == "basefile.arc2")
            {
                UNIT_TEST_EXPECT_TRUE(element.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
                UNIT_TEST_EXPECT_TRUE(element.m_platform == "android");
            }
        }

        // original products must no longer exist since it should have found and deleted them!
        for (QString outFile: oldandroidouts)
        {
            UNIT_TEST_EXPECT_FALSE(QFile::exists(outFile));
        }

        // the old pc products should still exist because they were emitted this time around.
        for (QString outFile: oldpcouts)
        {
            UNIT_TEST_EXPECT_TRUE(QFile::exists(outFile));
        }

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        // add a fingerprint file thats next to the original file
        // feed it the exportsettings file again.
        // it should spawn BOTH compilers again.
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(absolutePath + ".exportsettings", "new!"));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath + ".exportsettings"));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // pc and android
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);

        // send all the done messages simultaneously:
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = processResults[checkIdx].m_jobEntry.GetAbsoluteSourcePath();
            UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(processFile1) == AssetUtilities::NormalizeFilePath(absolutePath));
            QString platformFolder = cacheRoot.filePath(QString::fromUtf8(processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()));
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            processFile1 = processResults[checkIdx].m_destinationPath;
            UNIT_TEST_EXPECT_TRUE(processFile1.startsWith(platformFolder));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);
        }


        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // --- delete the input asset and make sure it cleans up all products.

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();


        // first, delete the fingerprint file, this should result in normal reprocess:
        QFile::remove(absolutePath + ".exportsettings");
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath + ".exportsettings"));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and android,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);
        UNIT_TEST_EXPECT_TRUE(processResults[1].m_jobEntry.m_computedFingerprint != 0);


        // send all the done messages simultaneously:

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // deleting the fingerprint file should not have erased the products
        UNIT_TEST_EXPECT_TRUE(QFile::exists(pcouts[0]));
        UNIT_TEST_EXPECT_TRUE(QFile::exists(androidouts[0]));
        UNIT_TEST_EXPECT_TRUE(QFile::exists(pcouts2[0]));
        UNIT_TEST_EXPECT_TRUE(QFile::exists(androidouts2[0]));

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        connection.m_sent = false;
        payloadList.clear();

        // delete the original input.
        QFile::remove(absolutePath);

        SourceFileNotificationMessage sourceFileRemovedMessage;
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        // 9 messages because there's one source file with 4 products so:
        //      1 * file remove for the source file.
        //      4 * file claimed for the produce file to be able to update it safely.
        //      4 * file released for the produce file so it's free for other tools to use it again.
        UNIT_TEST_EXPECT_TRUE(payloadList.size() == 9);
        unsigned int messageLoadCount = 0;
        for (auto payload : payloadList)
        {
            if (payload.first == SourceFileNotificationMessage::MessageType)
            {
                UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), sourceFileRemovedMessage));
                UNIT_TEST_EXPECT_TRUE(sourceFileRemovedMessage.m_type == SourceFileNotificationMessage::FileRemoved);
                ++messageLoadCount;
            }
            else if (payload.first == AssetNotificationMessage::MessageType)
            {
                AssetNotificationMessage message;
                UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), message));
                UNIT_TEST_EXPECT_TRUE(
                    message.m_type == AssetNotificationMessage::NotificationType::JobFileClaimed ||
                    message.m_type == AssetNotificationMessage::NotificationType::JobFileReleased);
                ++messageLoadCount;
            }
        }

        UNIT_TEST_EXPECT_TRUE(connection.m_sent);
        UNIT_TEST_EXPECT_TRUE(messageLoadCount == azlossy_cast<unsigned>(payloadList.size())); // make sure all messages are accounted for
        scanFolder = QDir(sourceFileRemovedMessage.m_scanFolder.c_str());
        pathToCheck = scanFolder.filePath(sourceFileRemovedMessage.m_relativeSourcePath.c_str());
        UNIT_TEST_EXPECT_TRUE(QString::compare(absolutePath, pathToCheck, Qt::CaseSensitive) == 0);

        // nothing to process, but products should be gone!
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());

        // should have gotten four "removed" messages for its products:
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 4);

        for (auto element : assetMessages)
        {
            UNIT_TEST_EXPECT_TRUE(element.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        }

        UNIT_TEST_EXPECT_FALSE(QFile::exists(pcouts[0]));
        UNIT_TEST_EXPECT_FALSE(QFile::exists(androidouts[0]));
        UNIT_TEST_EXPECT_FALSE(QFile::exists(pcouts2[0]));
        UNIT_TEST_EXPECT_FALSE(QFile::exists(androidouts2[0]));

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        // test: if an asset fails, it should recompile it next time, and not report success

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(absolutePath, "new2"));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and android,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts2[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));

        // send both done messages simultaneously!
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // send one failure only for PC :
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetFailed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

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
            apm.ProcessGetAssetJobsInfoRequest(requestInfo, jobResponse);

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (int detailsIdx = 0; detailsIdx < processResults.size(); ++detailsIdx)
                {
                    const JobDetails& details = processResults[detailsIdx];

                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_pathRelativeToWatchFolder, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_watchFolder.c_str(), details.m_jobEntry.m_watchFolderPath, Qt::CaseSensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;

                        if (detailsIdx == 2) // we only said that the index [2] job was dead
                        {
                            UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Failed);
                        }
                        else
                        {
                            UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Completed);
                        }

                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        // we should have get three success:
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 3);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 3);

        // which should be for the ANDROID:
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == absolutePath);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefilea.arc1" || assetMessages[0].m_data == "basefilea.azm");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "android");

        for (auto& payload : payloadList)
        {
            if (payload.first == SourceFileNotificationMessage::MessageType)
            {
                UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), sourceFileRemovedMessage));
                UNIT_TEST_EXPECT_TRUE(sourceFileRemovedMessage.m_type == SourceFileNotificationMessage::FileRemoved);
            }
        }

        scanFolder = QDir(sourceFileRemovedMessage.m_scanFolder.c_str());
        pathToCheck = scanFolder.filePath(sourceFileRemovedMessage.m_relativeSourcePath.c_str());
        UNIT_TEST_EXPECT_TRUE(QString::compare(absolutePath, pathToCheck, Qt::CaseSensitive) == 0);

        // now if we notify again, only the pc should process:
        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();
        payloadList.clear();

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // pc only
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "new1"));

        // send one failure only for PC :

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // we should have got only one success:
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "pc");

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();


        //----------This file is used for testing ProcessGetFullAssetPath function //
        absolutePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/somerandomfile.random"));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // 1 for pc
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");

        pcouts.clear();

        pcouts.push_back(cacheRoot.filePath(QString("pc/subfolder3/randomfileoutput.random")));
        pcouts.push_back(cacheRoot.filePath(QString("pc/subfolder3/randomfileoutput.random1")));
        pcouts.push_back(cacheRoot.filePath(QString("pc/subfolder3/randomfileoutput.random2")));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[1], "products."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[2], "products."));

        //Invoke Asset Processed for pc platform , txt files job description
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData(), AZ::Uuid::CreateNull(), 1));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[1].toUtf8().constData(), AZ::Uuid::CreateNull(), 2));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[2].toUtf8().constData(), AZ::Uuid::CreateNull(), 3));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 3);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();


        // -------------- override test -----------------
        // set up by letting it compile basefile.txt from 3:

        absolutePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath(subfolder3BaseFilePath));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and android,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);

        androidouts.clear();
        androidouts2.clear();
        pcouts.clear();
        pcouts2.clear();
        androidouts.push_back(cacheRoot.filePath(QString("android/basefilez.arc2")));
        androidouts2.push_back(cacheRoot.filePath(QString("android/basefileaz.azm2")));
        // note that the android outs have changed
        // but the pc outs are still the same.
        pcouts.push_back(cacheRoot.filePath(QString("pc/basefile.arc2")));
        pcouts2.push_back(cacheRoot.filePath(QString("pc/basefile.azm2")));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(androidouts2[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));
        changedInputResults.clear();
        assetMessages.clear();

        // send all the done messages simultaneously:
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts[0].toUtf8().constData(), AZ::Uuid::CreateNull(), 1));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(androidouts2[0].toUtf8().constData(), AZ::Uuid::CreateNull(), 2));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData(), AZ::Uuid::CreateNull(), 3));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData(), AZ::Uuid::CreateNull(), 4));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

        // we should have got only one success:
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 4);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 4);
        for (auto element : assetMessages)
        {
            UNIT_TEST_EXPECT_TRUE(element.m_legacyAssetIds.size() == expectedLegacyAssetIdCount);
        }

        // ------------- setup complete, now do the test...
        // now feed it a file that has been overridden by a more important later file
        absolutePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder1/basefile.txt"));

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        // since it was overridden, nothing should occur.
        //AZ_TracePrintf("Asset Processor", "Preparing the assessDeletedFiles invocation...\n");

        // delete the highest priority override file and ensure that it generates tasks
        // for the next highest priority!  Basically, deleting this file should "reveal" the file underneath it in the other subfolder
        QString deletedFile = tempPath.absoluteFilePath(subfolder3BaseFilePath);
        QString expectedReplacementInputFile = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder2/basefile.txt"));

        UNIT_TEST_EXPECT_TRUE(QFile::remove(deletedFile));
        // sometimes the above deletion actually takes a moment to trickle, for some reason, and it doesn't actually get that the file was erased.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        UNIT_TEST_EXPECT_FALSE(QFile::exists(deletedFile));

        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString,  deletedFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

#if defined(AZ_PLATFORM_LINUX)
        // On Linux, because of we cannot change the case of the source file, the job fingerprint is not updated due the case-switch so
        // there will be actually nothing to process
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 0);
#else
        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and android,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = processResults[checkIdx].m_jobEntry.GetAbsoluteSourcePath();
            UNIT_TEST_EXPECT_TRUE(processFile1 == expectedReplacementInputFile);
            QString platformFolder = cacheRoot.filePath(QString::fromUtf8(processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()));
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            processFile1 = processResults[checkIdx].m_destinationPath;
            UNIT_TEST_EXPECT_TRUE(processFile1.startsWith(platformFolder));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);
        }
#endif // defined(AZ_PLATFORM_LINUX)
        relativePathFromWatchFolder = "somefile.xxx";
        watchFolderPath = tempPath.absoluteFilePath("subfolder3");
        absolutePath = watchFolderPath + "/" + relativePathFromWatchFolder;

        unsigned int fingerprintForPC = 0;
        unsigned int fingerprintForANDROID = 0;

        ComputeFingerprints(fingerprintForPC, fingerprintForANDROID, config, watchFolderPath, relativePathFromWatchFolder);

        processResults.clear();
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // // 2 each for pc and android,since we have two recognizer for .xxx file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));

        config.RemoveRecognizer("xxx files 2 (builder2)");
        UNIT_TEST_EXPECT_TRUE(mockAppManager.UnRegisterAssetRecognizerAsBuilder("xxx files 2 (builder2)"));

        //Changing specs for pc
        specpc.m_extraRCParams = "new pcparams";
        rec.m_platformSpecs.insert("pc", specpc);

        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        processResults.clear();
        absolutePath = AssetUtilities::NormalizeFilePath(absolutePath);
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // we never actually submitted any fingerprints or indicated success, so the same number of jobs should occur as before
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // // 2 each for pc and android,since we have two recognizer for .xxx file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));

        // tell it that all those assets are now successfully done:
        AZ::u32 resultIdx = 0;
        for (const auto& processResult : processResults)
        {
            ++resultIdx;
            QString outputFile = normalizedCacheRootDir.absoluteFilePath(processResult.m_destinationPath + "/doesn'tmatter.dds" + processResult.m_jobEntry.m_jobKey);
            CreateDummyFile(outputFile);
            response = {};
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(outputFile.toUtf8().constData(), AZ::Uuid::CreateNull(), resultIdx));
            apm.AssetProcessed(processResult.m_jobEntry, response);
        }

        // now re-perform the same test, this time only the pc ones should re-appear.
        // this should happen because we're changing the extra params, which should be part of the fingerprint
        // if this unit test fails, check to make sure that the extra params are being ingested into the fingerprint computation functions
        // and also make sure that the jobs that are for the remaining android platform don't change.

        // store the UUID so that we can insert the new one with the same UUID
        AZStd::shared_ptr<InternalMockBuilder> builderTxt2Builder;
        UNIT_TEST_EXPECT_TRUE(mockAppManager.GetBuilderByID("xxx files 2 (builder2)", builderTxt2Builder));

        AZ::Uuid builderUuid;
        UNIT_TEST_EXPECT_TRUE(mockAppManager.GetBuildUUIDFromName("xxx files 2 (builder2)", builderUuid));

        builderTxt2Builder.reset();

        config.RemoveRecognizer("xxx files 2 (builder2)");
        mockAppManager.UnRegisterAssetRecognizerAsBuilder("xxx files 2 (builder2)");
        //Changing specs for pc
        specpc.m_extraRCParams = "new pcparams---"; // make sure the xtra params are different.
        rec.m_platformSpecs.remove("pc");
        rec.m_platformSpecs.insert("pc", specpc);

        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        processResults.clear();

        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // only 1 for pc
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc"));

        // ---------------------

        unsigned int newfingerprintForPC = 0;
        unsigned int newfingerprintForANDROID = 0;

        ComputeFingerprints(newfingerprintForPC, newfingerprintForANDROID, config, watchFolderPath, relativePathFromWatchFolder);

        UNIT_TEST_EXPECT_TRUE(newfingerprintForPC != fingerprintForPC);//Fingerprints should be different
        UNIT_TEST_EXPECT_TRUE(newfingerprintForANDROID == fingerprintForANDROID);//Fingerprints are same

        config.RemoveRecognizer("xxx files 2 (builder2)");
        mockAppManager.UnRegisterAssetRecognizerAsBuilder("xxx files 2 (builder2)");

        //Changing version
        rec.m_version = "1.0";
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        processResults.clear();

        absolutePath = AssetUtilities::NormalizeFilePath(absolutePath);
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // pc and android
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier != processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc") || (processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "pc") || (processResults[1].m_jobEntry.m_platformInfo.m_identifier == "android"));

        unsigned int newfingerprintForPCAfterVersionChange = 0;
        unsigned int newfingerprintForANDROIDAfterVersionChange = 0;

        ComputeFingerprints(newfingerprintForPCAfterVersionChange, newfingerprintForANDROIDAfterVersionChange, config, watchFolderPath, relativePathFromWatchFolder);

        UNIT_TEST_EXPECT_TRUE((newfingerprintForPCAfterVersionChange != fingerprintForPC) || (newfingerprintForPCAfterVersionChange != newfingerprintForPC));//Fingerprints should be different
        UNIT_TEST_EXPECT_TRUE((newfingerprintForANDROIDAfterVersionChange != fingerprintForANDROID) || (newfingerprintForANDROIDAfterVersionChange != newfingerprintForANDROID));//Fingerprints should be different

        processResults.clear();
        
        // ------------- Test querying asset status -------------------
        {
            absolutePath = tempPath.absoluteFilePath("subfolder2/folder/ship.tiff");
            absolutePath = AssetUtilities::NormalizeFilePath(absolutePath);

            QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
            UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
            resultIdx = 0;
            for (const JobDetails& processResult : processResults)
            {
                ++resultIdx;
                QString outputFile = normalizedCacheRootDir.absoluteFilePath(processResult.m_destinationPath + "/ship_nrm.dds");

                CreateDummyFile(outputFile);

                AssetBuilderSDK::ProcessJobResponse jobResponse;
                jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(outputFile.toUtf8().constData(), AZ::Uuid::CreateNull(), resultIdx));

                apm.AssetProcessed(processResult.m_jobEntry, jobResponse);
            }

            // let events bubble through:
            QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

            bool foundIt = false;

            auto connectionMade = connect(&apm, &AssetProcessorManager::SendAssetExistsResponse,
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

            // Test source without path, should all fail
            for (const auto& testCase : successCases)
            {
                foundIt = false;

                apm.OnRequestAssetExists(requestId, "pc", testCase, AZ::Data::AssetId());
                UNIT_TEST_EXPECT_FALSE(foundIt);
            }

            // Test source with the path included
            for (const auto& testCase : successCases)
            {
                foundIt = false;
                AZStd::string withPath = AZStd::string("folder/") + testCase;

                apm.OnRequestAssetExists(requestId, "pc", withPath.c_str(), AZ::Data::AssetId());
                UNIT_TEST_EXPECT_TRUE(foundIt);
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

                apm.OnRequestAssetExists(requestId, "pc", testCase, AZ::Data::AssetId());
                UNIT_TEST_EXPECT_FALSE(foundIt);
            }
        }

        // -----------------------------------------------------------------------------------------------
        // -------------------------------------- FOLDER RENAMING TEST -----------------------------------
        // -----------------------------------------------------------------------------------------------
        // Test: Rename a source folder

        // test renaming an entire folder!

        QString fileToMove1 = tempPath.absoluteFilePath("subfolder1/rename_this/somefile1.txt");
        QString fileToMove2 = tempPath.absoluteFilePath("subfolder1/rename_this/somefolder/somefile2.txt");

        config.RemoveRecognizer(builderTxt2Name); // don't need this anymore.
        mockAppManager.UnRegisterAssetRecognizerAsBuilder(builderTxt2Name);

        processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove2));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 fils on 2 platforms

        for (int index = 0; index < processResults.size(); ++index)
        {
            QFileInfo fi(processResults[index].m_jobEntry.GetAbsoluteSourcePath());
            QString pcout = QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName());
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcout.toUtf8().constData(), AZ::Uuid::CreateNull(), index));
            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }


        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // setup complete.  now RENAME that folder.

        QDir renamer;
        UNIT_TEST_EXPECT_TRUE(renamer.rename(tempPath.absoluteFilePath("subfolder1/rename_this"), tempPath.absoluteFilePath("subfolder1/done_renaming")));

        // renames appear as a delete then add of that folder:
        processResults.clear();
        assetMessages.clear();
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, tempPath.absoluteFilePath("subfolder1/rename_this")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 0); // nothing to process

        // we are aware that 4 products went missing (android and pc versions of the 2 files since we renamed the SOURCE folder)
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 4);
        for (auto element : assetMessages)
        {
            UNIT_TEST_EXPECT_TRUE(element.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        }

        processResults.clear();
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, tempPath.absoluteFilePath("subfolder1/done_renaming")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 files on 2 platforms

        // -----------------------------------------------------------------------------------------------
        // -------------------------------------- FOLDER RENAMING TEST -----------------------------------
        // -----------------------------------------------------------------------------------------------
        // Test: Rename a cache folder

        QStringList outputsCreated;

        for (int index = 0; index < processResults.size(); ++index)
        {
            QFileInfo fi(processResults[index].m_jobEntry.GetAbsoluteSourcePath());
            QString pcout = QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName());
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcout.toUtf8().constData(), AZ::Uuid::CreateNull(), index));
            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // it now believes that there are a whole bunch of assets in subfolder1/done_renaming and they resulted in
        // a whole bunch of files to have been created in the asset cache, listed in processresults, and they exist in outputscreated...
        // rename the output folder:

        QString originalCacheFolderName = normalizedCacheRootDir.absoluteFilePath("pc") + "/done_renaming";
        QString newCacheFolderName = normalizedCacheRootDir.absoluteFilePath("pc") + "/renamed_again";

        UNIT_TEST_EXPECT_TRUE(renamer.rename(originalCacheFolderName, newCacheFolderName));

        // tell it that the products moved:
        processResults.clear();
        assetMessages.clear();
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, originalCacheFolderName));
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, newCacheFolderName));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // at this point, we should NOT get 2 removed products - we should only get those messages later
        // once the processing queue actually processes these assets - not prematurely as it discovers them missing.
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 0);

        // We've already (above) verified that the product list should be ok, this is just to avoid a crash instead of a failure.
        UNIT_TEST_EXPECT_TRUE(processResults.size() > 1);


        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");
        UNIT_TEST_EXPECT_TRUE(processResults[1].m_jobEntry.m_platformInfo.m_identifier == "pc");

        // -----------------------------------------------------------------------------------------------
        // -------------------------------------- FOLDER RENAMING TEST -----------------------------------
        // -----------------------------------------------------------------------------------------------
        // Test: Rename folders that did not have files in them (but had child files, this was a bug at a point)

        fileToMove1 = tempPath.absoluteFilePath("subfolder1/rename_this_secondly/somefolder/somefile2.txt");

        processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));


        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 file on 2 platforms

        for (int index = 0; index < processResults.size(); ++index)
        {
            QFileInfo fi(processResults[index].m_jobEntry.GetAbsoluteSourcePath());
            QString pcout = QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName());
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcout.toUtf8().constData(), AZ::Uuid::CreateNull(), index));
            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // setup complete.  now RENAME that folder.

        originalCacheFolderName = normalizedCacheRootDir.absoluteFilePath("pc") + "/rename_this_secondly";
        newCacheFolderName = normalizedCacheRootDir.absoluteFilePath("pc") + "/done_renaming_again";

        UNIT_TEST_EXPECT_TRUE(renamer.rename(originalCacheFolderName, newCacheFolderName));

        // tell it that the products moved:
        processResults.clear();
        assetMessages.clear();
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, originalCacheFolderName));
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, newCacheFolderName));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 0); // we don't prematurely emit "AssetRemoved" until we actually finish process.
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // ONLY the PC files need to be re-processed because only those were renamed.
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");

        // --------------------------------------------------------------------------------------------------
        // ------------------------------ TEST DELETED SOURCE RESULTING IN DELETED PRODUCTS -----------------
        // --------------------------------------------------------------------------------------------------

        // first, set up a whole pipeline to create, notify, and consume the file:
        fileToMove1 = tempPath.absoluteFilePath("subfolder1/to_be_deleted/some_deleted_file.txt");

        processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 file on 2 platforms

        QStringList createdDummyFiles;

        for (int index = 0; index < processResults.size(); ++index)
        {
            QFileInfo fi(processResults[index].m_jobEntry.GetAbsoluteSourcePath());
            QString pcout = QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName());
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcout.toUtf8().constData(), AZ::Uuid::CreateNull(), index));
            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        processResults.clear();
        assetMessages.clear();

        // setup complete.  now delete the source file:
        UNIT_TEST_EXPECT_TRUE(renamer.remove(fileToMove1));
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 2); // all products must be removed
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 0); // nothing should process

        for (int index = 0; index < createdDummyFiles.size(); ++index)
        {
            QFileInfo fi(createdDummyFiles[index]);
            UNIT_TEST_EXPECT_FALSE(fi.exists());
            // in fact, the directory must also no longer exist in the cache:
            UNIT_TEST_EXPECT_FALSE(fi.dir().exists());
        }

        // --------------------------------------------------------------------------------------------------
        // - TEST SOURCE FILE REPROCESSING RESULTING IN FEWER PRODUCTS NEXT TIME ----------------------------
        // (it needs to delete the products and it needs to notify listeners about it)
        // --------------------------------------------------------------------------------------------------

        // first, set up a whole pipeline to create, notify, and consume the file:
        fileToMove1 = tempPath.absoluteFilePath("subfolder1/fewer_products/test.txt");

        processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 file on 2 platforms

        createdDummyFiles.clear(); // keep track of the files which we expect to be gone next time

        for (int index = 0; index < processResults.size(); ++index)
        {
            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            // this time, ouput 2 files for each job instead of just one:
            QFileInfo fi(processResults[index].m_jobEntry.GetAbsoluteSourcePath());

            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName() + ".0.txt").toUtf8().constData(), AZ::Uuid::CreateNull(), index));
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName() + ".1.txt").toUtf8().constData(), AZ::Uuid::CreateNull(), index + 100));

            createdDummyFiles.push_back(response.m_outputProducts[0].m_productFileName.c_str()); // we're only gong to delete this one out of the two, which is why we don't push the other one.

            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(response.m_outputProducts[0].m_productFileName.c_str(), "product 0"));
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(response.m_outputProducts[1].m_productFileName.c_str(), "product 1"));

            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // at this point, we have a cache with the four files (2 for each platform)
        // we're going to resubmit the job with different data
        UNIT_TEST_EXPECT_TRUE(renamer.remove(fileToMove1));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(fileToMove1, "fresh data!"));

        processResults.clear();

        // tell file changed:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 file on 2 platforms

        assetMessages.clear();

        for (int index = 0; index < processResults.size(); ++index)
        {
            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            // this time, ouput only one file for each job instead of just one:
            QFileInfo fi(processResults[index].m_jobEntry.GetAbsoluteSourcePath());

            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName() + ".1.txt").toUtf8().constData(), AZ::Uuid::CreateNull(), index));
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(response.m_outputProducts[0].m_productFileName.c_str(), "product 1 changed"));

            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        sortAssetToProcessResultList(processResults);

        // we should have gotten 2 product removed, 2 product changed, total of 4 asset messages

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 4);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_assetId != AZ::Data::AssetId());
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_assetId != AZ::Data::AssetId());
        UNIT_TEST_EXPECT_TRUE(assetMessages[2].m_assetId != AZ::Data::AssetId());
        UNIT_TEST_EXPECT_TRUE(assetMessages[3].m_assetId != AZ::Data::AssetId());

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "android");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_platform == "android");
        UNIT_TEST_EXPECT_TRUE(assetMessages[2].m_platform == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[3].m_platform == "pc");

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "fewer_products/test.txt.0.txt");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_data == "fewer_products/test.txt.1.txt");
        UNIT_TEST_EXPECT_TRUE(assetMessages[2].m_data == "fewer_products/test.txt.0.txt");
        UNIT_TEST_EXPECT_TRUE(assetMessages[3].m_data == "fewer_products/test.txt.1.txt");

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        UNIT_TEST_EXPECT_TRUE(assetMessages[2].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        UNIT_TEST_EXPECT_TRUE(assetMessages[3].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);


        // and finally, the actual removed products should be gone from the HDD:
        for (int index = 0; index < createdDummyFiles.size(); ++index)
        {
            QFileInfo fi(createdDummyFiles[index]);
            UNIT_TEST_EXPECT_FALSE(fi.exists());
            // the directory must still exist because there were other files in there (no accidental deletions!)
            UNIT_TEST_EXPECT_TRUE(fi.dir().exists());
        }

        // -----------------------------------------------------------------------------------------------
        // ------------------- ASSETBUILDER TEST---------------------------------------------------
        //------------------------------------------------------------------------------------------------

        mockAppManager.ResetMatchingBuildersInfoFunctionCalls();
        mockAppManager.ResetMockBuilderCreateJobCalls();
        mockAppManager.UnRegisterAllBuilders();

        AssetRecognizer abt_rec1;
        AssetPlatformSpec abt_specandroid;
        abt_rec1.m_name = "UnitTestTextBuilder1";
        abt_rec1.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        //abt_rec1.m_regexp.setPatternSyntax(QRegExp::Wildcard);
        //abt_rec1.m_regexp.setPattern("*.txt");
        abt_rec1.m_platformSpecs.insert("android", specandroid);
        mockAppManager.RegisterAssetRecognizerAsBuilder(abt_rec1);

        AssetRecognizer abt_rec2;
        AssetPlatformSpec abt_specpc;
        abt_rec2.m_name = "UnitTestTextBuilder2";
        abt_rec2.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        //abt_rec2.m_regexp.setPatternSyntax(QRegExp::Wildcard);
        //abt_rec2.m_regexp.setPattern("*.txt");
        abt_rec2.m_platformSpecs.insert("pc", specpc);
        mockAppManager.RegisterAssetRecognizerAsBuilder(abt_rec2);

        processResults.clear();

        absolutePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/uniquefile.txt"));

        // Pass the txt file through the asset pipeline
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absolutePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(mockAppManager.GetMatchingBuildersInfoFunctionCalls() == 1);
        UNIT_TEST_EXPECT_TRUE(mockAppManager.GetMockBuilderCreateJobCalls() == 2);  // Since we have two text builder registered

        AssetProcessor::BuilderInfoList builderInfoList;
        mockAppManager.GetMatchingBuildersInfo(AZStd::string(absolutePath.toUtf8().constData()), builderInfoList);
        auto builderInfoListCount = builderInfoList.size();
        UNIT_TEST_EXPECT_TRUE(builderInfoListCount == 2);

        for (auto& buildInfo : builderInfoList)
        {
            AZStd::shared_ptr<InternalMockBuilder> builder;
            UNIT_TEST_EXPECT_TRUE(mockAppManager.GetBuilderByID(buildInfo.m_name, builder));

            UNIT_TEST_EXPECT_TRUE(builder->GetCreateJobCalls() == 1);

            // note, uuid does not include watch folder name.  This is a quick test to make sure that the source file UUID actually makes it into the CreateJobRequest.
            // the ProcessJobRequest is populated frmo the CreateJobRequest.
            UNIT_TEST_EXPECT_TRUE(builder->GetLastCreateJobRequest().m_sourceFileUUID == AssetUtilities::CreateSafeSourceUUIDFromName("uniquefile.txt"));
            QString watchedFolder(AssetUtilities::NormalizeFilePath(builder->GetLastCreateJobRequest().m_watchFolder.c_str()));
            QString expectedWatchedFolder(tempPath.absoluteFilePath("subfolder3"));
            UNIT_TEST_EXPECT_TRUE(QString::compare(watchedFolder, expectedWatchedFolder, Qt::CaseInsensitive) == 0); // verify watchfolder

            QString filename(AssetUtilities::NormalizeFilePath(builder->GetLastCreateJobRequest().m_sourceFile.c_str()));
            QString expectFileName("uniquefile.txt");
            UNIT_TEST_EXPECT_TRUE(QString::compare(filename, expectFileName, Qt::CaseInsensitive) == 0); // verify filename
            builder->ResetCounters();
        }

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 for pc and android
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "android");
        UNIT_TEST_EXPECT_TRUE(processResults[1].m_jobEntry.m_platformInfo.m_identifier == "pc");
        UNIT_TEST_EXPECT_TRUE(QString::compare(processResults[0].m_jobEntry.GetAbsoluteSourcePath(), absolutePath, Qt::CaseInsensitive) == 0);
        UNIT_TEST_EXPECT_TRUE(QString::compare(processResults[1].m_jobEntry.GetAbsoluteSourcePath(), absolutePath, Qt::CaseInsensitive) == 0);
        UNIT_TEST_EXPECT_TRUE(QString::compare(QString(processResults[0].m_jobEntry.m_jobKey), QString(abt_rec1.m_name)) == 0);
        UNIT_TEST_EXPECT_TRUE(QString::compare(QString(processResults[1].m_jobEntry.m_jobKey), QString(abt_rec2.m_name)) == 0);
        Q_EMIT UnitTestPassed();

        // Stop file watching, disconnect everything and process all events so nothing gets called after the method finishes
        fileWatcher.StopWatching();

        QObject::disconnect(assetProcessorManagerIdleStateConnection);
        QObject::disconnect(inputAssetProcessedConnection);
        QObject::disconnect(assetMessageConnection);
        QObject::disconnect(assetToProcessConnection);

        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    void AssetProcessorManagerUnitTests_JobKeys::StartTest()
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

        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        QTemporaryDir dir;
        UnitTestUtils::ScopedDir changeDir(dir.path());
        QDir tempPath(dir.path());

        QString gameName = AssetUtilities::ComputeProjectName(AssetProcessorManagerTestGameProject);

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());

        PlatformConfiguration config;
        config.EnablePlatform({ "pc" ,{ "desktop", "renderer" } }, true);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse order
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", false, true, platforms,-1)); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "temp", "tempfolder", true, false, platforms, 0)); // add the root

        AssetProcessorManager_Test apm(&config);

        QDir cacheRoot;
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(cacheRoot));

        QList<JobDetails> processResults;
        QList<QPair<QString, QString> > changedInputResults;
        QList<AzFramework::AssetSystem::AssetNotificationMessage > assetMessages;

        bool idling = false;

        connect(&apm, &AssetProcessorManager::AssetToProcess,
            this, [&processResults](JobDetails details)
        {
            processResults.push_back(AZStd::move(details));
        });

        connect(&apm, &AssetProcessorManager::AssetMessage,
            this, [&assetMessages](AzFramework::AssetSystem::AssetNotificationMessage message)
        {
            assetMessages.push_back( message);
        });

        connect(&apm, &AssetProcessorManager::InputAssetProcessed,
            this, [&changedInputResults](QString relativePath, QString platform)
        {
            changedInputResults.push_back(QPair<QString, QString>(relativePath, platform));
        });

        connect(&apm, &AssetProcessorManager::AssetProcessorManagerIdleState,
            this, [&idling](bool state)
        {
            idling = state;
        }
        );

        QString sourceFile = tempPath.absoluteFilePath("subfolder1/basefile.foo");
        CreateDummyFile(sourceFile);

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 2; //Create two jobs for this file

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2);
        for (int idx = 0; idx < processResults.size(); idx++)
        {
            UNIT_TEST_EXPECT_TRUE((processResults[idx].m_jobEntry.m_platformInfo.m_identifier == "pc"));
            UNIT_TEST_EXPECT_TRUE(processResults[idx].m_jobEntry.m_pathRelativeToWatchFolder.startsWith("basefile.foo"));
        }
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_jobKey.compare(processResults[1].m_jobEntry.m_jobKey) != 0);

        QStringList pcouts;
        pcouts.push_back(cacheRoot.filePath(QString("pc/basefile.arc1")));
        pcouts.push_back(cacheRoot.filePath(QString("pc/basefile.arc2")));

        // Create the product files for the first job
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[1], "product2"));


        // Invoke Asset Processed for pc platform for the first job
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData(), AZ::Uuid::CreateNull(), 1));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[1].toUtf8().constData(), AZ::Uuid::CreateNull(), 2));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 2);

        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_platform == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_data == "basefile.arc2");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(sourceFile));

        pcouts.clear();
        pcouts.push_back(cacheRoot.filePath(QString("pc/basefile.arc3")));
        // Create the product files for the second job
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));

        // Invoke Asset Processed for pc platform for the second job
        response.m_outputProducts.clear();
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        assetMessages.clear();
        changedInputResults.clear();
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);

        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_platform == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_data == "basefile.arc3");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(sourceFile));

        //Delete the product of the second job
        UNIT_TEST_EXPECT_TRUE(QFile::remove(pcouts[0]));

        processResults.clear();

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // We should only have one job to process here
        for (int idx = 0; idx < processResults.size(); idx++)
        {
            UNIT_TEST_EXPECT_TRUE((processResults[idx].m_jobEntry.m_platformInfo.m_identifier == "pc"));
            UNIT_TEST_EXPECT_TRUE(processResults[idx].m_jobEntry.m_pathRelativeToWatchFolder.startsWith("basefile.foo"));
        }

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 1; //Create one job for this file this time

        processResults.clear();

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 0); // We should not have any job to process here

        // products of the second job should not exists any longer
        for (QString outFile : pcouts)
        {
            UNIT_TEST_EXPECT_FALSE(QFile::exists(pcouts[0]));
        }

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 2; //Again create two jobs for this file, this should result in one additional job

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // We should see a job to process here
        for (int idx = 0; idx < processResults.size(); idx++)
        {
            UNIT_TEST_EXPECT_TRUE((processResults[idx].m_jobEntry.m_platformInfo.m_identifier == "pc"));
            UNIT_TEST_EXPECT_TRUE(processResults[idx].m_jobEntry.m_pathRelativeToWatchFolder.startsWith("basefile.foo"));
        }

        Q_EMIT UnitTestPassed();
    }

    void AssetProcessorManagerUnitTests_ScanFolders::StartTest()
    {
        using namespace AzToolsFramework::AssetDatabase;

        MockApplicationManager  mockAppManager;
        mockAppManager.BusConnect();

        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        QTemporaryDir dir;
        UNIT_TEST_EXPECT_TRUE(dir.isValid());
        UnitTestUtils::ScopedDir changeDir(dir.path());
        QDir tempPath(dir.path());

        QString gameName = AssetUtilities::ComputeProjectName(AssetProcessorManagerTestGameProject);

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());
        // should create cache folder in the root, and read everything from there.

        PlatformConfiguration config;
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse platforms order
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", false, false, platforms, -6)); // subfolder 4 overrides subfolder3
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3", false, false, platforms, -5)); // subfolder 3 overrides subfolder2
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2", false, true, platforms, -2)); // subfolder 2 overrides subfolder1
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", false, true, platforms, -1)); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "temp", "tempfolder", true, false, platforms, 0)); // add the root

        {
            // create this, which will write those scan folders into the db as-is
            AssetProcessorManager_Test apm(&config);
        }

        ScanFolderDatabaseEntryContainer entryContainer;
        auto puller = [&entryContainer](ScanFolderDatabaseEntry& entry)
            {
                entryContainer.push_back(entry);
                return true;
            };

        {
            AssetDatabaseConnection connection;
            UNIT_TEST_EXPECT_TRUE(connection.OpenDatabase());
            // make sure we find the scan folders.
            entryContainer.clear();
            connection.QueryScanFoldersTable(puller);
            UNIT_TEST_EXPECT_TRUE(config.GetScanFolderCount() == entryContainer.size());
            // make sure they are all present and have port key:
            for (int idx = 0; idx < config.GetScanFolderCount(); ++idx)
            {
                AssetProcessor::ScanFolderInfo& scanFolderInConfig = config.GetScanFolderAt(idx);
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

                UNIT_TEST_EXPECT_TRUE(found != entryContainer.end());
            }
        }

        // now make a different config with different scan folders but with some of the same portable keys but new paths.
        PlatformConfiguration config2;
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms2;
        config2.PopulatePlatformsForScanFolder(platforms2);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse platforms order
        // case 1:  same absolute path, but the same portable key - should use same ID as before.
        config2.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", false, false, platforms2, -6)); // subfolder 4 overrides subfolder3

        // case 2:  A new absolute path, but same portable key - should use same id as before
        config2.AddScanFolder(ScanFolderInfo(tempPath.filePath("newfolder3"), "subfolder3", "subfolder3", false, false, platforms2, -5)); // subfolder 3 overrides subfolder2

        // case 3:  same absolute path, new portable key - should use a new ID
        config2.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder3", "newfolder3", false, false, platforms2, -5)); // subfolder 3 overrides subfolder2

        // case 4:  subfolder2 is missing - it should be gone.

        {
            // create this, which will write those scan folders into the db as-is
            AssetProcessorManager_Test apm(&config2);
            apm.CheckMissingFiles();
        }

        {
            AssetDatabaseConnection connection;
            UNIT_TEST_EXPECT_TRUE(connection.OpenDatabase());
            // make sure we find the scan folders.
            entryContainer.clear();
            connection.QueryScanFoldersTable(puller);
            UNIT_TEST_EXPECT_TRUE(config2.GetScanFolderCount() == entryContainer.size());

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

                UNIT_TEST_EXPECT_TRUE(found != entryContainer.end());
            }
        }

        const AssetProcessor::ScanFolderInfo* subfolder4InConfig1 = nullptr;
        const AssetProcessor::ScanFolderInfo* subfolder4InConfig2 = nullptr;

        const AssetProcessor::ScanFolderInfo* subfolder3InConfig1 = nullptr;
        const AssetProcessor::ScanFolderInfo* subfolder3InConfig2 = nullptr;

        AZStd::unordered_set<AZ::s64> idsInConfig1;

        for (int idx = 0; idx < config.GetScanFolderCount(); ++idx)
        {
            AssetProcessor::ScanFolderInfo& scanFolderInConfig = config.GetScanFolderAt(idx);
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
            AssetProcessor::ScanFolderInfo& scanFolderInConfig = config.GetScanFolderAt(idx);

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
                UNIT_TEST_EXPECT_TRUE(idsInConfig1.find(scanFolderInConfig.ScanFolderID()) == idsInConfig1.end()); // must not be found
            }
        }

        UNIT_TEST_EXPECT_TRUE(subfolder3InConfig2);
        UNIT_TEST_EXPECT_TRUE(subfolder3InConfig1);

        UNIT_TEST_EXPECT_TRUE(subfolder4InConfig2);
        UNIT_TEST_EXPECT_TRUE(subfolder4InConfig1);

        // the above scan folders should not have changed id
        UNIT_TEST_EXPECT_TRUE(subfolder3InConfig1->ScanFolderID() == subfolder3InConfig2->ScanFolderID());
        UNIT_TEST_EXPECT_TRUE(subfolder4InConfig1->ScanFolderID() == subfolder4InConfig2->ScanFolderID());

        Q_EMIT UnitTestPassed();
    }

    void AssetProcessorManagerUnitTests_JobDependencies_Fingerprint::StartTest()
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

        m_assetBuilderDesc.m_name = "Job Dependency UnitTest";
        m_assetBuilderDesc.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        m_assetBuilderDesc.m_busId = builderUuid;
        m_assetBuilderDesc.m_analysisFingerprint = "xyz"; // Normally this would include the same fingerprint info from the job but for the purposes of testing, we just need something here
        m_assetBuilderDesc.m_createJobFunction = [&fileBJobDependentOnFileAJob, &changeJobAFingerprint, &fileCJobDependentOnFileBJob, &sourceFileBUuid]
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

        AssetProcessor::AssetBuilderInfoBus::Handler::BusConnect();
        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        QTemporaryDir dir;
        UNIT_TEST_EXPECT_TRUE(dir.isValid());
        QString test = dir.path();
        UnitTestUtils::ScopedDir changeDir(dir.path());
        QDir tempPath(dir.path());

        QString gameName = AssetUtilities::ComputeProjectName(AssetProcessorManagerTestGameProject);

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());
        // should create cache folder in the root, and read everything from there.

        PlatformConfiguration config;
        config.EnablePlatform({ "pc",{ "desktop", "renderer" } }, true);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse order
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", false, true, platforms, -1)); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "temp", "tempfolder", true, false, platforms, 0)); // add the root

        AssetProcessorManager_Test apm(&config);

        QList<JobDetails> processResults;

        bool idling = false;

        connect(&apm, &AssetProcessorManager::AssetToProcess,
            this, [&processResults](JobDetails details)
        {
            processResults.push_back(AZStd::move(details));
        });

        connect(&apm, &AssetProcessorManager::AssetProcessorManagerIdleState,
            this, [&idling](bool state)
        {
            idling = state;
        }
        );

        QString sourceFileAPath = tempPath.absoluteFilePath("subfolder1/some/random/folders/FileA.txt");
        QString sourceFileBPath = tempPath.absoluteFilePath("subfolder1/FileB.txt");
        QString sourceFileCPath = tempPath.absoluteFilePath("FileC.txt");
        sourceFileBUuid = AssetUtilities::CreateSafeSourceUUIDFromName("FileB.txt");

        QDir cacheRoot;
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(cacheRoot));

        QString productFileAPath = cacheRoot.filePath(QString("pc/fileaproduct.txt"));
        QString productFileBPath = cacheRoot.filePath(QString("pc/filebproduct1.txt"));
        QString product2FileBPath = cacheRoot.filePath(QString("pc/filebproduct2.txt"));
        QString productFileCPath = cacheRoot.filePath(QString("pc/filecproduct.txt"));
        QString product2FileCPath = cacheRoot.filePath(QString("pc/filecproduct2.txt"));

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(sourceFileAPath, ""));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(sourceFileBPath, ""));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(sourceFileCPath, ""));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(productFileAPath, "product"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(productFileBPath, "product"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(product2FileBPath, "product"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(productFileCPath, "product"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(product2FileCPath, "product"));

        // Analyze FileA
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 500000));

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1);
        UNIT_TEST_EXPECT_FALSE(processResults[0].m_jobDependencyList.size());

        // Invoke Asset Processed for pc platform for the FileA job
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileAPath.toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        processResults.clear();
        response.m_outputProducts.clear();

        // Analyze FileB, one of the jobs should declare a job dependency on the FileA job
        fileBJobDependentOnFileAJob = true;
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileBPath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2);
        bool onlyOneJobHaveJobDependency = false;
        for (JobDetails& jobDetail : processResults)
        {
            if (jobDetail.m_jobDependencyList.size())
            {
                UNIT_TEST_EXPECT_FALSE(onlyOneJobHaveJobDependency);
                onlyOneJobHaveJobDependency = true;
                UNIT_TEST_EXPECT_TRUE(jobDetail.m_jobDependencyList.size() == 1);
                JobDependencyInternal& jobDependencyInternal = jobDetail.m_jobDependencyList[0];
                UNIT_TEST_EXPECT_TRUE(jobDependencyInternal.m_builderUuidList.find(builderUuid) != jobDependencyInternal.m_builderUuidList.end());
                UNIT_TEST_EXPECT_TRUE(QString(jobDependencyInternal.m_jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str()).endsWith("FileA.txt", Qt::CaseSensitivity::CaseInsensitive));
            }
        }

        UNIT_TEST_EXPECT_TRUE(onlyOneJobHaveJobDependency);

        // Invoke Asset Processed for pc platform for the first FileB job
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileBPath.toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        response.m_outputProducts.clear();

        // Invoke Asset Processed for pc platform for the second FileB job
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2FileBPath.toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000000));

        processResults.clear();
        response.m_outputProducts.clear();

        // Change the fingerprint of the FileA job and analyze the file again
        // This time it should not only process its job again but should also process the dependent FileB job
        changeJobAFingerprint = true;
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2);

        for (JobDetails& jobDetail : processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileB.txt"))
            {
                // Ensure that we are processing the right FileB job
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy") == 0);

                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2FileBPath.toUtf8().constData()));
                QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
            }
            else
            {
                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileAPath.toUtf8().constData()));
                QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
            }
        }

        processResults.clear();
        response.m_outputProducts.clear();

        // Modify FileA and analyze the file again.
        // This time also it should not only process its job again but should also process the dependent FileB job
        UNIT_TEST_EXPECT_TRUE(QFile::remove(sourceFileAPath));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(sourceFileAPath, "changed"));

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2);

        for (JobDetails& jobDetail : processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileB.txt"))
            {
                // Ensure that we are processing the right FileB job
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy") == 0);

                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2FileBPath.toUtf8().constData()));
                QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
            }
            else
            {
                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileAPath.toUtf8().constData()));
                QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
            }
        }

        // First we will analyze File C
        // This should make Job("FileC","zzz", "pc") depends on Job("FileB", "yyy", "pc") which already depends on Job("FileA", "xxx", "pc")
        // After that we will change the fingerprint of Job("FileA", "xxx", "pc") and analyze FileA again,
        // which should process all the three jobs once again.
        processResults.clear();

        fileCJobDependentOnFileBJob = true;
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileCPath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2);

        for (JobDetails& jobDetail : processResults)
        {
            UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileC.txt"));
            if (jobDetail.m_jobDependencyList.size())
            {
                // Verify FileC jobinfo
                AssetBuilderSDK::SourceFileDependency& source = jobDetail.m_jobDependencyList[0].m_jobDependency.m_sourceFile;
                UNIT_TEST_EXPECT_TRUE(QString(source.m_sourceFileDependencyPath.c_str()).compare("FileB.txt") == 0);
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobDependencyList[0].m_jobDependency.m_jobKey.c_str()).compare("yyy") == 0);

                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2FileCPath.toUtf8().constData()));
                QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
            }
            else
            {
                response.m_outputProducts.clear();
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFileCPath.toUtf8().constData()));
                QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetail.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
                UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
            }
        }

        processResults.clear();
        // Modify fingerprint of Job("FileA", "xxx", "pc") and analyze FileA again,
        changeJobAFingerprint = false; // This will revert back the changes in the extra info used for fingerprinting of this job

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        //One of the FileC job("FileC.txt","zzz") depends on the FileB job("FileB.txt", "yyy") which depends on FileA job("FileA.txt", "xxx")
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 3);

        for (JobDetails& jobDetail : processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileA.txt"))
            {
                // Verify FileA jobinfo
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("xxx") == 0);
            }
            else if(QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileB.txt"))
            {
                // Verify FileB jobinfo
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy") == 0);

            }
            else if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileC.txt"))
            {
                // Verify FileC jobinfo
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("zzz") == 0);
            }
        }


        // Since one of the FileC job("FileC.txt","zzz") have emitted a job dependency on a FileB job("FileB.txt", "yyy")
        // which also have a job dependency on a FileA job("FileA.txt", "xxx") therefore deleting File A source file should
        // cause both jobs (File B and File C) to be processed again.

        processResults.clear();

        QFile::remove(sourceFileAPath);

        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileAPath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2);

        for (JobDetails& jobDetail : processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileB.txt"))
            {
                // Verify FileB jobinfo
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy") == 0);
            }
            else if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileC.txt"))
            {
                // Verify FileC jobinfo
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("zzz") == 0);
            }
            else
            {
                // invalid job info
                UNIT_TEST_EXPECT_TRUE(false);
            }
        }
        processResults.clear();

        // Adding FileA back should cause all the three jobs to be processed again.

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(sourceFileAPath, "reappear"));

        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString , sourceFileAPath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));


        UNIT_TEST_EXPECT_TRUE(processResults.size() == 3);

        for (JobDetails& jobDetail : processResults)
        {
            if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileA.txt"))
            {
                // Verify FileA jobinfo
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("xxx") == 0);
            }
            else if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileB.txt"))
            {
                // Verify FileB jobinfo
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("yyy") == 0);

            }
            else if (QString(jobDetail.m_jobEntry.m_pathRelativeToWatchFolder).endsWith("FileC.txt"))
            {
                // Verify FileC jobinfo
                UNIT_TEST_EXPECT_TRUE(QString(jobDetail.m_jobEntry.m_jobKey).compare("zzz") == 0);
            }
        }

        AssetProcessor::AssetBuilderInfoBus::Handler::BusDisconnect();

        Q_EMIT UnitTestPassed();
    }

    void AssetProcessorManagerUnitTests_JobDependencies_Fingerprint::GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
    {
        AZ_UNUSED(assetPath);
        builderInfoList.push_back(m_assetBuilderDesc);
    }

    void AssetProcessorManagerUnitTests_JobDependencies_Fingerprint::GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList)
    {
        builderInfoList.push_back(m_assetBuilderDesc);
    }

} // namespace AssetProcessor
