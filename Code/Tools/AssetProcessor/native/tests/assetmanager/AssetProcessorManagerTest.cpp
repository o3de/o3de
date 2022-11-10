/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetProcessorManagerTest.h"
#include "native/AssetManager/PathDependencyManager.h"
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/ToolsFileUtils/ToolsFileUtils.h>

#include <AzTest/AzTest.h>

#include <limits>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Jobs/JobManagerDesc.h>
#include <tests/assetmanager/AssetManagerTestingBase.h>

using namespace AssetProcessor;

AssetProcessorManager_Test::AssetProcessorManager_Test(AssetProcessor::PlatformConfiguration* config, QObject* parent /*= 0*/)
    :AssetProcessorManager(config, parent)
{
}

AssetProcessorManager_Test::~AssetProcessorManager_Test()
{
}

bool AssetProcessorManager_Test::CheckJobKeyToJobRunKeyMap(AZStd::string jobKey)
{
    return (m_jobKeyToJobRunKeyMap.find(jobKey) != m_jobKeyToJobRunKeyMap.end());
}

AssetProcessorManagerTest::AssetProcessorManagerTest()
    : m_argc(0)
    , m_argv(0)
{

    m_qApp.reset(new QCoreApplication(m_argc, m_argv));
    qRegisterMetaType<AssetProcessor::JobEntry>("JobEntry");
    qRegisterMetaType<AssetBuilderSDK::ProcessJobResponse>("ProcessJobResponse");
    qRegisterMetaType<AZStd::string>("AZStd::string");
    qRegisterMetaType<AssetProcessor::AssetScanningStatus>("AssetProcessor::AssetScanningStatus");
    qRegisterMetaType<QSet<AssetFileInfo>>("QSet<AssetFileInfo>");
}

bool AssetProcessorManagerTest::BlockUntilIdle(int millisecondsMax)
{
    QElapsedTimer limit;
    limit.start();

    if(AZ::Debug::Trace::Instance().IsDebuggerPresent())
    {
        millisecondsMax = std::numeric_limits<int>::max();
    }

    // Always run at least once so that if we're in an idle state to start, we don't end up skipping the loop before finishing all the queued work
    do
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    } while ((!m_isIdling) && (limit.elapsed() < millisecondsMax));

    // and then once more, so that any queued events as a result of the above finish.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

    return m_isIdling;
}

void AssetProcessorManagerTest::SetUp()
{
    using namespace testing;
    using ::testing::NiceMock;
    using namespace AssetProcessor;
    using namespace AzToolsFramework::AssetDatabase;

    AssetProcessorTest::SetUp();

    AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
    AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

    m_data = AZStd::make_unique<StaticData>();

    m_data->m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

    m_data->m_descriptor = AZ::JobManagerComponent::CreateDescriptor();
    m_data->m_descriptor->Reflect(m_data->m_serializeContext.get());

    m_data->m_jobManagerEntity = aznew AZ::Entity{};
    m_data->m_jobManagerEntity->CreateComponent<AZ::JobManagerComponent>();
    m_data->m_jobManagerEntity->Init();
    m_data->m_jobManagerEntity->Activate();

    m_config.reset(new AssetProcessor::PlatformConfiguration());
    m_mockApplicationManager.reset(new AssetProcessor::MockApplicationManager());

    AssetUtilities::ResetAssetRoot();

    m_assetRootDir = QDir(m_databaseLocationListener.GetAssetRootDir().c_str());
    m_scopeDir = AZStd::make_unique<UnitTestUtils::ScopedDir>();
    m_scopeDir->Setup(m_assetRootDir.path());

    auto registry = AZ::SettingsRegistry::Get();
    auto cacheRootKey =
        AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_cache_path";
    registry->Set(cacheRootKey, m_assetRootDir.absoluteFilePath("Cache").toUtf8().constData());
    auto projectPathKey =
        AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
    AZ::IO::FixedMaxPath enginePath;
    registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

    m_gameName = AssetUtilities::ComputeProjectName("AutomatedTesting", true);

    AssetUtilities::ResetAssetRoot();
    QDir newRoot;
    AssetUtilities::ComputeEngineRoot(newRoot, &m_assetRootDir);

    QDir cacheRoot;
    AssetUtilities::ComputeProjectCacheRoot(cacheRoot);
    QString normalizedCacheRoot = AssetUtilities::NormalizeDirectoryPath(cacheRoot.absolutePath());

    m_normalizedCacheRootDir.setPath(normalizedCacheRoot);

    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));

    m_config->EnablePlatform({ "pc", { "host", "renderer", "desktop" } }, true);

    m_config->AddScanFolder(ScanFolderInfo(m_assetRootDir.filePath("subfolder1"), "subfolder1", "subfolder1", false, true, m_config->GetEnabledPlatforms(), 1));
    m_config->AddScanFolder(ScanFolderInfo(m_assetRootDir.filePath("subfolder2"), "subfolder2", "subfolder2", false, true, m_config->GetEnabledPlatforms()));
    m_config->AddScanFolder(ScanFolderInfo(m_assetRootDir.filePath("subfolder3"), "subfolder3", "subfolder3", false, true, m_config->GetEnabledPlatforms(), 1));
    m_config->AddScanFolder(ScanFolderInfo(m_assetRootDir.filePath("subfolder4"), "subfolder4", "subfolder4", false, true, m_config->GetEnabledPlatforms(), 1));
    m_config->AddMetaDataType("assetinfo", "");
    m_config->AddIntermediateScanFolder();

    AssetRecognizer rec;
    rec.m_name = "txt files";
    rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
    rec.m_supportsCreateJobs = false;
    rec.m_supportsCreateJobs = false;
    ASSERT_TRUE(m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec));
    m_mockApplicationManager->BusConnect();

    m_assetProcessorManager.reset(new AssetProcessorManager_Test(m_config.get()));
    m_errorAbsorber->Clear();

    m_isIdling = false;

    m_idleConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState, [this](bool newState)
    {
        m_isIdling = newState;
    });

    PopulateDatabase();
}

void AssetProcessorManagerTest::TearDown()
{
    m_data->m_jobManagerEntity->Deactivate();
    delete m_data->m_jobManagerEntity;
    delete m_data->m_descriptor;

    m_data = nullptr;

    QObject::disconnect(m_idleConnection);
    m_mockApplicationManager->BusDisconnect();
    m_mockApplicationManager->UnRegisterAllBuilders();

    AssetUtilities::ResetAssetRoot();
    AssetUtilities::ResetGameName();

    m_assetProcessorManager.reset();
    m_mockApplicationManager.reset();
    m_config.reset();
    m_qApp.reset();
    m_scopeDir.reset();

    AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

    AssetProcessor::AssetProcessorTest::TearDown();
}

void AssetProcessorManagerTest::CreateSourceAndFile(const char* tempFolderRelativePath)
{
    auto absolutePath = m_assetRootDir.absoluteFilePath(tempFolderRelativePath);

    auto scanFolder = m_config->GetScanFolderForFile(absolutePath);

    QString relPath;
    m_config->ConvertToRelativePath(absolutePath, scanFolder, relPath);

    auto uuid = AssetUtilities::CreateSafeSourceUUIDFromName(relPath.toUtf8().constData());

    AzToolsFramework::AssetDatabase::SourceDatabaseEntry source(scanFolder->ScanFolderID(), relPath.toUtf8().constData(), uuid, "fingerprint");
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSource(source));

    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(absolutePath));
}

void AssetProcessorManagerTest::PopulateDatabase()
{
    using namespace AzToolsFramework::AssetDatabase;

    AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder(
        m_assetRootDir.absoluteFilePath("subfolder1").toUtf8().constData(), "temp path", "temp path");
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetScanFolder(scanFolder));

    CreateSourceAndFile("subfolder1/a.txt");
    CreateSourceAndFile("subfolder1/b.txt");
    CreateSourceAndFile("subfolder1/c.txt");
    CreateSourceAndFile("subfolder1/d.txt");
}

TEST_F(AssetProcessorManagerTest, UnitTestForGettingJobInfoBySourceUUIDSuccess)
{
    // Here we first mark a job for an asset complete and than fetch jobs info using the job log api to verify
    // Next we mark another job for that same asset as queued, and we again fetch jobs info from the api to verify,

    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolder = m_assetRootDir.absoluteFilePath("subfolder1");

    JobEntry entry;
    entry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(watchFolder, relFileName);
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;
    UnitTestUtils::CreateDummyFile(m_normalizedCacheRootDir.absoluteFilePath("pc/outputfile.txt"));

    AssetBuilderSDK::ProcessJobResponse jobResponse;
    jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("outputfile.txt"));

    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, entry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, jobResponse));

    // let events bubble through:
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    AZ::Uuid uuid = AssetUtilities::CreateSafeSourceUUIDFromName(relFileName.toUtf8().data());
    AssetJobsInfoRequest request;
    request.m_assetId = AZ::Data::AssetId(uuid, 0);
    request.m_escalateJobs = false;
    AssetJobsInfoResponse response;
    m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(request, response);

    EXPECT_TRUE(response.m_isSuccess);
    EXPECT_EQ(1, response.m_jobList.size());
    ASSERT_GT(response.m_jobList.size(), 0); // Assert on this to exit early if needed, otherwise indexing m_jobList later will crash.
    EXPECT_EQ(JobStatus::Completed, response.m_jobList[0].m_status);
    EXPECT_STRCASEEQ(relFileName.toUtf8().data(), response.m_jobList[0].m_sourceFile.c_str());


    m_assetProcessorManager->OnJobStatusChanged(entry, JobStatus::Queued);

    response.m_isSuccess = false;
    response.m_jobList.clear();

    m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(request, response);
    EXPECT_TRUE(response.m_isSuccess);
    EXPECT_EQ(1, response.m_jobList.size());
    ASSERT_GT(response.m_jobList.size(), 0); // Assert on this to exit early if needed, otherwise indexing m_jobList later will crash.

    EXPECT_EQ(JobStatus::Queued, response.m_jobList[0].m_status);
    EXPECT_STRCASEEQ(relFileName.toUtf8().data(), response.m_jobList[0].m_sourceFile.c_str());
    EXPECT_STRCASEEQ(m_assetRootDir.filePath("subfolder1").toUtf8().data(), response.m_jobList[0].m_watchFolder.c_str());

    ASSERT_EQ(m_errorAbsorber->m_numWarningsAbsorbed, 0);
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
    ASSERT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
}

using AssetProcessorManagerFinishTests = UnitTests::AssetManagerTestingBase;

TEST_F(AssetProcessorManagerFinishTests, IntermediateAsset_AnalysisCountHitsZero)
{
    // Test that FinishedAnalysis occurs and that we can reliably determine both APM and RC have finished working when intermediate assets
    // are involved
    using namespace AssetBuilderSDK;

    CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
    CreateBuilder("stage2", "*.stage2", "stage3", false, ProductOutputFlags::ProductAsset);

    int remainingFiles = 0;
    int maxWaitingFiles = 0;
    bool finishedAnalysisOccurred = false;
    bool finishedAnalysisAndIdle = false;
    bool idle = false;

    QObject::connect(
        m_assetProcessorManager.get(),
        &AssetProcessor::AssetProcessorManager::FinishedAnalysis,
        [&remainingFiles, &maxWaitingFiles, &finishedAnalysisOccurred, &idle, &finishedAnalysisAndIdle, this](int count)
        {
            finishedAnalysisOccurred = true;

            if (count > maxWaitingFiles)
            {
                maxWaitingFiles = count;
            }

            remainingFiles = count;

            if (idle && remainingFiles == 0 && finishedAnalysisOccurred && m_rc->IsIdle())
            {
                EXPECT_FALSE(finishedAnalysisAndIdle);
                finishedAnalysisAndIdle = true;
            }
        });

    QObject::connect(
        m_assetProcessorManager.get(),
        &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState,
        [&idle, &remainingFiles, &finishedAnalysisAndIdle, &finishedAnalysisOccurred, this](bool state)
        {
            idle = state;

            if (idle && remainingFiles == 0 && finishedAnalysisOccurred && m_rc->IsIdle())
            {
                EXPECT_FALSE(finishedAnalysisAndIdle);
                finishedAnalysisAndIdle = true;
            }
        });

    ProcessFileMultiStage(2, true);

    QCoreApplication::processEvents(); // Execute FinishAnalysis

    EXPECT_TRUE(finishedAnalysisOccurred);
    EXPECT_TRUE(finishedAnalysisAndIdle);
}

TEST_F(AssetProcessorManagerFinishTests, MultipleFiles_WithDuplicateJobs_AnalysisCountHitsZero)
{
    // Test that FinishedAnalysis emits a non-zero value when multiple files are queued up and that having the same file submitted twice
    // does not result in the counter being stuck at a non-zero value.
    using namespace AssetBuilderSDK;

    CreateBuilder("stage1", "*.stage1", "stage2", false, ProductOutputFlags::ProductAsset);

    // Connect RC to APM
    QObject::connect(
        m_rc.get(),
        &AssetProcessor::RCController::FileCompiled,
        m_assetProcessorManager.get(),
        &AssetProcessor::AssetProcessorManager::AssetProcessed,
        Qt::UniqueConnection);
    QObject::connect(
        m_rc.get(),
        &AssetProcessor::RCController::FileFailed,
        m_assetProcessorManager.get(),
        &AssetProcessor::AssetProcessorManager::AssetFailed);
    QObject::connect(
        m_rc.get(),
        &AssetProcessor::RCController::FileCancelled,
        m_assetProcessorManager.get(),
        &AssetProcessor::AssetProcessorManager::AssetCancelled);

    int remainingFiles = 0;
    int maxWaitingFiles = 0;
    bool finishedAnalysisOccurred = false;

    QObject::connect(
        m_assetProcessorManager.get(),
        &AssetProcessor::AssetProcessorManager::FinishedAnalysis,
        [&remainingFiles, &maxWaitingFiles, &finishedAnalysisOccurred](int count)
        {
            finishedAnalysisOccurred = true;

            if (count > maxWaitingFiles)
            {
                maxWaitingFiles = count;
            }

            remainingFiles = count;
        });

    // Set up a second file to process
    AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
    AZStd::string testFilename = "second.stage1";
    QString testFilePath = (scanFolderDir / testFilename).AsPosix().c_str();

    UnitTestUtils::CreateDummyFile(testFilePath.toUtf8().constData(), "unit test file");

    const char* file = m_testFilePath.c_str();
    int endStage = 1;

    int expectedJobCount = 1;
    int expectedFileCount = 1;

    // Process the first file
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, file));
    QCoreApplication::processEvents();

    RunFile(expectedJobCount, expectedFileCount);

    // Copy out the job since it will get cleared next time we call RunFile
    auto jobListCopy = m_jobDetailsList;

    // Process the first file again, this will record 2 jobs for analysis in APM
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, file));
    QCoreApplication::processEvents();

    RunFile(expectedJobCount, expectedFileCount);

    // Add the first job back in
    jobListCopy.push_back(m_jobDetailsList[0]);

    // Process the 2nd file so there are 2 different files waiting for analysis
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, testFilePath));
    QCoreApplication::processEvents();

    RunFile(expectedJobCount, expectedFileCount);

    jobListCopy.push_back(m_jobDetailsList[0]);
    m_jobDetailsList = jobListCopy;

    std::stable_sort(
        m_jobDetailsList.begin(),
        m_jobDetailsList.end(),
        [](const AssetProcessor::JobDetails& a, const AssetProcessor::JobDetails& b) -> bool
        {
            return a.m_jobEntry.m_sourceAssetReference < b.m_jobEntry.m_sourceAssetReference;
        });

    ASSERT_EQ(m_jobDetailsList.size(), 3);

    // Run all 3 jobs through RC.  The duplicate should get discarded and marked as cancelled which allows APM to clear it from the analysis
    // list
    ProcessJob(*m_rc, m_jobDetailsList[0]);
    ProcessJob(*m_rc, m_jobDetailsList[1]);
    ProcessJob(*m_rc, m_jobDetailsList[2]);

    ASSERT_TRUE(m_fileCompiled);

    m_assetProcessorManager->CheckFilesToExamine(0);
    m_assetProcessorManager->CheckActiveFiles(0);
    m_assetProcessorManager->CheckJobEntries(0);

    QCoreApplication::processEvents(); // Execute FinishAnalysis

    CheckProduct(AZStd::string::format("test.stage%d", endStage + 1).c_str());

    // FinishAnalysis should have run and reported no files left waiting for analysis
    EXPECT_TRUE(finishedAnalysisOccurred);
    EXPECT_EQ(remainingFiles, 0);
    EXPECT_EQ(maxWaitingFiles, 1);
}

TEST_F(AssetProcessorManagerTest, WarningsAndErrorsReported_SuccessfullySavedToDatabase)
{
    // This tests the JobDiagnosticTracker:  Warnings/errors reported to it should be recorded in the database when AssetProcessed is fired and able to be retrieved when querying job status

    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolder = m_assetRootDir.absoluteFilePath("subfolder1");

    JobEntry entry;
    entry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(watchFolder, relFileName);
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    UnitTestUtils::CreateDummyFile(m_normalizedCacheRootDir.absoluteFilePath("pc/outputfile.txt"));

    AssetBuilderSDK::ProcessJobResponse jobResponse;
    jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("outputfile.txt"));

    JobDiagnosticRequestBus::Broadcast(&JobDiagnosticRequestBus::Events::RecordDiagnosticInfo, entry.m_jobRunKey, JobDiagnosticInfo(11, 22));

    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, entry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, jobResponse));

    // let events bubble through:
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    AZ::Uuid uuid = AssetUtilities::CreateSafeSourceUUIDFromName(relFileName.toUtf8().data());
    AssetJobsInfoRequest request;
    request.m_assetId = AZ::Data::AssetId(uuid, 0);
    request.m_escalateJobs = false;
    AssetJobsInfoResponse response;
    m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(request, response);

    EXPECT_TRUE(response.m_isSuccess);
    EXPECT_EQ(1, response.m_jobList.size());
    ASSERT_GT(response.m_jobList.size(), 0); // Assert on this to exit early if needed, otherwise indexing m_jobList later will crash.
    EXPECT_EQ(JobStatus::Completed, response.m_jobList[0].m_status);
    EXPECT_STRCASEEQ(relFileName.toUtf8().data(), response.m_jobList[0].m_sourceFile.c_str());
    ASSERT_EQ(response.m_jobList[0].m_warningCount, 11);
    ASSERT_EQ(response.m_jobList[0].m_errorCount, 22);

    ASSERT_EQ(m_errorAbsorber->m_numWarningsAbsorbed, 0);
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
    ASSERT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
}


TEST_F(AssetProcessorManagerTest, DeleteFolder_SignalsDeleteOfContainedFiles)
{
    using namespace AssetProcessor;

    static constexpr char folderPathNoScanfolder[] = "folder/folder/foldertest.txt";
    static constexpr char folderPath[] = "subfolder1/folder/folder/foldertest.txt";

    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath(folderPath));

    auto scanFolderInfo = m_config->GetScanFolderByPath(m_assetRootDir.absoluteFilePath("subfolder1"));
    ASSERT_TRUE(scanFolderInfo != nullptr);

    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry(
        scanFolderInfo->ScanFolderID(),
        folderPathNoScanfolder,
        AZ::Uuid::CreateRandom(),
        /*analysisFingerprint - arbitrary*/ "abcdefg");
    m_assetProcessorManager->m_stateData->SetSource(sourceEntry);

    int count = 0;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::SourceDeleted, [&count](const SourceAssetReference& file)
        {
            if (QString(file.RelativePath().c_str()).compare(folderPathNoScanfolder, Qt::CaseInsensitive) == 0)
            {
                count++;
            }
        });

    m_isIdling = false;
    // tell the APM about the files:
    m_assetProcessorManager->AssessAddedFile(m_assetRootDir.absoluteFilePath(folderPath));

    ASSERT_TRUE(BlockUntilIdle(5000));

    EXPECT_TRUE(QDir(m_assetRootDir.absoluteFilePath("subfolder1/folder")).removeRecursively());

    m_isIdling = false;
    m_assetProcessorManager->AssessDeletedFile(m_assetRootDir.absoluteFilePath("subfolder1/folder"));
    ASSERT_TRUE(BlockUntilIdle(5000));

    EXPECT_EQ(1, count);
}

TEST_F(AssetProcessorManagerTest, UnitTestForGettingJobInfoBySourceUUIDFailure)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QString relFileName("assetProcessorManagerTestFailed.txt");

    AZ::Uuid uuid = AssetUtilities::CreateSafeSourceUUIDFromName(relFileName.toUtf8().data());
    AssetJobsInfoRequest request;
    request.m_assetId = AZ::Data::AssetId(uuid, 0);
    request.m_escalateJobs = false;
    AssetJobsInfoResponse response;
    m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(request, response);

    ASSERT_TRUE(response.m_isSuccess == false); //expected result should be false because AP does not know about this asset
    ASSERT_TRUE(response.m_jobList.size() == 0);
}

TEST_F(AssetProcessorManagerTest, UnitTestForCancelledJob)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    JobEntry entry;

    entry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(m_assetRootDir.absoluteFilePath("subfolder1"), relFileName);
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(relFileName.toUtf8().constData());
    bool sourceFound = false;

    //Checking the response of the APM when we cancel a job in progress
    m_assetProcessorManager->OnJobStatusChanged(entry, JobStatus::Queued);
    m_assetProcessorManager->OnJobStatusChanged(entry, JobStatus::InProgress);
    ASSERT_TRUE(m_assetProcessorManager->CheckJobKeyToJobRunKeyMap(entry.m_jobKey.toUtf8().data()));
    m_assetProcessorManager->AssetCancelled(entry);
    ASSERT_FALSE(m_assetProcessorManager->CheckJobKeyToJobRunKeyMap(entry.m_jobKey.toUtf8().data()));
    ASSERT_TRUE(m_assetProcessorManager->GetDatabaseConnection()->QuerySourceBySourceGuid(sourceUUID, [&]([[maybe_unused]] AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source)
    {
        sourceFound = true;
        return false;
    }));

    ASSERT_FALSE(sourceFound);
}

// if the function to compute builder dirtiness is not called, we should always be dirty
TEST_F(AssetProcessorManagerTest, BuilderDirtiness_BeforeComputingDirtiness_AllDirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
}

class MockBuilderResponder
    : public AssetProcessor::AssetBuilderInfoBus::Handler
{
public:
    MockBuilderResponder() {}
    virtual ~MockBuilderResponder() {}

    //! AssetProcessor::AssetBuilderInfoBus Interface
    void GetMatchingBuildersInfo(const AZStd::string& /*assetPath*/, AssetProcessor::BuilderInfoList& /*builderInfoList*/) override
    {
        // not used
        ASSERT_TRUE(false) << "This function should not be called";
        return;
    }

    void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override
    {
        builderInfoList = m_assetBuilderDescs;
    }

    ////////////////////////////////////////////////
    AssetProcessor::BuilderInfoList m_assetBuilderDescs;

    void AddBuilder(const char* name, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& patterns, const AZ::Uuid& busId, int version, const char* fingerprint)
    {
        AssetBuilderSDK::AssetBuilderDesc newDesc;
        newDesc.m_name = name;
        newDesc.m_patterns = patterns;
        newDesc.m_busId = busId;
        newDesc.m_version = version;
        newDesc.m_analysisFingerprint = fingerprint;
        m_assetBuilderDescs.emplace_back(AZStd::move(newDesc));
    }
};

struct BuilderDirtiness : public AssetProcessorManagerTest
{
    void SetUp() override
    {
        AssetProcessorManagerTest::SetUp();

        // Disconnect the mock application manager, our MockBuilderResponder will handle builder registration instead
        m_mockApplicationManager->BusDisconnect();

        m_mockBuilderResponder.BusConnect();
    }

    void TearDown() override
    {
        m_mockBuilderResponder.BusDisconnect();

        AssetProcessorManagerTest::TearDown();
    }

    MockBuilderResponder m_mockBuilderResponder;
};

// if our database was empty before, all builders should be dirty
// note that this requires us to actually register a builder using the mock.
TEST_F(BuilderDirtiness, BuilderDirtiness_EmptyDatabase_AllDirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder(
        "builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint1");
    m_mockBuilderResponder.AddBuilder(
        "builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint2");

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 2);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(m_mockBuilderResponder.m_assetBuilderDescs[0].m_busId));
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(m_mockBuilderResponder.m_assetBuilderDescs[1].m_busId));

    m_mockBuilderResponder.BusDisconnect();
}

// if we have the same set of builders the next time, nothing should register as changed.
TEST_F(BuilderDirtiness, BuilderDirtiness_SameAsLastTime_NoneDirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder(
        "builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint1");
    m_mockBuilderResponder.AddBuilder(
        "builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint2");

    m_assetProcessorManager->ComputeBuilderDirty();

    // now we retrigger the dirty computation, so that nothing has changed:
    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_FALSE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_FALSE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 0);

    m_mockBuilderResponder.BusDisconnect();
}

// when a new builder appears, the new builder should be dirty,
TEST_F(BuilderDirtiness, BuilderDirtiness_MoreThanLastTime_NewOneIsDirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder(
        "builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint1");

    m_assetProcessorManager->ComputeBuilderDirty();

    m_mockBuilderResponder.AddBuilder(
        "builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint2");

    m_assetProcessorManager->ComputeBuilderDirty();

    // one new builder should have been dirty:
    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(m_mockBuilderResponder.m_assetBuilderDescs[1].m_busId));

    m_mockBuilderResponder.BusDisconnect();
}

// when an existing builder disappears there are no dirty builders, but the booleans
// that track dirtiness should be correct:
TEST_F(BuilderDirtiness, BuilderDirtiness_FewerThanLastTime_Dirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder(
        "builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint1");
    m_mockBuilderResponder.AddBuilder(
        "builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint2");

    m_assetProcessorManager->ComputeBuilderDirty();

    // remove one:
    m_mockBuilderResponder.m_assetBuilderDescs.pop_back();

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 0);
}

// if a builder changes its pattern matching, it should be dirty, and also, it should count as add or remove.
TEST_F(BuilderDirtiness, BuilderDirtiness_ChangedPattern_CountsAsNew)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder(
        "builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint1");
    m_mockBuilderResponder.AddBuilder(
        "builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint2");
    m_mockBuilderResponder.AddBuilder(
        "builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint3");
    m_mockBuilderResponder.AddBuilder(
        "builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    // here, we change the actual text of the pattern to match
    size_t whichToChange = 1;
    // here, we change the pattern type but not the pattern to match
    AssetBuilderPattern oldPattern = m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns[0];
    oldPattern.m_pattern = "*.somethingElse";
    m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.clear();
    m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.emplace_back(oldPattern);

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    m_mockBuilderResponder.BusDisconnect();
}

TEST_F(BuilderDirtiness, BuilderDirtiness_ChangedPatternType_CountsAsNew)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder(
        "builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint1");
    m_mockBuilderResponder.AddBuilder(
        "builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint2");
    m_mockBuilderResponder.AddBuilder(
        "builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint3");
    m_mockBuilderResponder.AddBuilder(
        "builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1,
        "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    size_t whichToChange = 2;
    // here, we change the pattern type but not the pattern to match
    AssetBuilderPattern oldPattern = m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns[0];
    oldPattern.m_type = AssetBuilderPattern::Regex;
    m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.clear();
    m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.emplace_back(oldPattern);

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    m_mockBuilderResponder.BusDisconnect();
}

TEST_F(BuilderDirtiness, BuilderDirtiness_NewPattern_CountsAsNewBuilder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    m_mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");
    m_mockBuilderResponder.AddBuilder("builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint3");
    m_mockBuilderResponder.AddBuilder("builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    size_t whichToChange = 3;
    // here, we add an additional pattern that wasn't there before:
    m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.clear();
    m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.buzz", AssetBuilderPattern::Wildcard));

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    m_mockBuilderResponder.BusDisconnect();
}

// changing the "version" of a builder should be equivalent to changing its analysis fingerprint - ie
// it should not count as adding a new builder.
TEST_F(BuilderDirtiness, BuilderDirtiness_NewVersionNumber_IsNotANewBuilder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    m_mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");
    m_mockBuilderResponder.AddBuilder("builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint3");
    m_mockBuilderResponder.AddBuilder("builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    size_t whichToChange = 3;
    m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_version++;

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_FALSE(m_assetProcessorManager->m_buildersAddedOrRemoved); // <-- note, we don't expect this to be considered a 'new builder'
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    m_mockBuilderResponder.BusDisconnect();
}

// changing the "analysis fingerprint" of a builder should not count as an addition or removal
// but should still result in that specific builder being considered as a dirty builder.
TEST_F(BuilderDirtiness, BuilderDirtiness_NewAnalysisFingerprint_IsNotANewBuilder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    m_mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");
    m_mockBuilderResponder.AddBuilder("builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint3");
    m_mockBuilderResponder.AddBuilder("builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    size_t whichToChange = 3;
    m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_analysisFingerprint = "changed!!";

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_FALSE(m_assetProcessorManager->m_buildersAddedOrRemoved); // <-- note, we don't expect this to be considered a 'new builder'
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(m_mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    m_mockBuilderResponder.BusDisconnect();

    m_mockApplicationManager->BusConnect();
}

// ------------------------------------------------------------------------------------------------
//                         QueryAbsolutePathDependenciesRecursive section
// ------------------------------------------------------------------------------------------------

TEST_F(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_BasicTest)
{
    using namespace AzToolsFramework::AssetDatabase;

    //  A depends on B, which depends on both C and D

    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/a.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/b.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/c.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/d.txt"), QString("tempdata\n"));

    SourceFileDependencyEntry newEntry1;  // a depends on B
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_sourceGuid = m_aUuid;
    newEntry1.m_dependsOnSource = PathOrUuid(m_bUuid);

    SourceFileDependencyEntry newEntry2; // b depends on C
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_sourceGuid = m_bUuid;
    newEntry2.m_dependsOnSource = PathOrUuid("c.txt");

    SourceFileDependencyEntry newEntry3;  // b also depends on D
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_sourceGuid = m_bUuid;
    newEntry3.m_dependsOnSource = PathOrUuid("d.txt");

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry1));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry2));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry3));

    AssetProcessor::SourceFilesForFingerprintingContainer dependencies;
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_aUuid, dependencies, SourceFileDependencyEntry::DEP_SourceToSource );
    EXPECT_EQ(dependencies.size(), 4); // a depends on b, c, and d - with the latter two being indirect.

    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    // make sure the corresponding values in the map are also correct
    EXPECT_STREQ(dependencies[m_assetRootDir.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()].c_str(), m_aUuid.ToFixedString(false, false).c_str());
    EXPECT_STREQ(dependencies[m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()].c_str(), m_bUuid.ToFixedString(false, false).c_str());
    EXPECT_STREQ(dependencies[m_assetRootDir.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()].c_str(), "c.txt");
    EXPECT_STREQ(dependencies[m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()].c_str(), "d.txt");

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_bUuid, dependencies, SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(dependencies.size(), 3); // b  depends on c, and d
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    // eliminate b --> c
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->RemoveSourceFileDependency(newEntry2.m_sourceDependencyID));

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_aUuid, dependencies, SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(dependencies.size(), 3); // a depends on b and d, but no longer c
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());
}

TEST_F(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_WithDifferentTypes_BasicTest)
{
    // test to make sure that different TYPES of dependencies work as expected.
    using namespace AzToolsFramework::AssetDatabase;

    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/a.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/b.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/c.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/d.txt"), QString("tempdata\n"));

    SourceFileDependencyEntry newEntry1;  // a depends on B as a SOURCE dependency.
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_sourceGuid = m_aUuid;
    newEntry1.m_dependsOnSource = PathOrUuid(m_bUuid);
    newEntry1.m_typeOfDependency = SourceFileDependencyEntry::DEP_SourceToSource;

    SourceFileDependencyEntry newEntry2; // b depends on C as a JOB dependency
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_sourceGuid = m_bUuid;
    newEntry2.m_dependsOnSource = PathOrUuid("c.txt");
    newEntry2.m_typeOfDependency = SourceFileDependencyEntry::DEP_JobToJob;

    SourceFileDependencyEntry newEntry3;  // b also depends on D as a SOURCE dependency
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_sourceGuid = m_bUuid;
    newEntry3.m_dependsOnSource = PathOrUuid("d.txt");
    newEntry3.m_typeOfDependency = SourceFileDependencyEntry::DEP_SourceToSource;

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry1));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry2));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry3));

    AssetProcessor::SourceFilesForFingerprintingContainer dependencies;
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_aUuid, dependencies, SourceFileDependencyEntry::DEP_SourceToSource);
    // note that a depends on b, c, and d - with the latter two being indirect.
    // however, since b's dependency on C is via JOB, and we're asking for SOURCE only, we should not see C.
    EXPECT_EQ(dependencies.size(), 3);

    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_bUuid, dependencies, SourceFileDependencyEntry::DEP_JobToJob);
    // b  depends on c, and d  - but we're asking for job dependencies only, so we should not get anything except C and B
    EXPECT_EQ(dependencies.size(), 2);
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());

    // now ask for ALL kinds and you should get the full tree.
    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_aUuid, dependencies, SourceFileDependencyEntry::DEP_Any);
    EXPECT_EQ(dependencies.size(), 4);
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());
}

// since we need these files to still produce a 0-based fingerprint, we need them to
// still do a best guess at absolute path, when they are missing.
TEST_F(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_MissingFiles_ReturnsNoPathWithPlaceholders)
{
    using namespace AzToolsFramework::AssetDatabase;

    //  A depends on B, which depends on both C and D

    // Remove b and c files
    AZ::IO::SystemFile::Delete(m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData());
    AZ::IO::SystemFile::Delete(m_assetRootDir.absoluteFilePath("subfolder1/c.txt").toUtf8().constData());

    AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;
    m_assetProcessorManager->m_stateData->GetSourceBySourceGuid(m_bUuid, entry);
    m_assetProcessorManager->m_stateData->RemoveSource(entry.m_sourceID);

    m_assetProcessorManager->m_stateData->GetSourceBySourceGuid(m_cUuid, entry);
    m_assetProcessorManager->m_stateData->RemoveSource(entry.m_sourceID);

    SourceFileDependencyEntry newEntry1;  // a depends on B
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_sourceGuid = m_aUuid;
    newEntry1.m_dependsOnSource = PathOrUuid(m_bUuid);

    SourceFileDependencyEntry newEntry2; // b depends on C
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_sourceGuid = m_bUuid;
    newEntry2.m_dependsOnSource = PathOrUuid("c.txt");

    SourceFileDependencyEntry newEntry3;  // b also depends on D
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_sourceGuid = m_bUuid;
    newEntry3.m_dependsOnSource = PathOrUuid("d.txt");

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry1));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry2));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry3));

    AssetProcessor::SourceFilesForFingerprintingContainer dependencies;
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_aUuid, dependencies, SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(dependencies.size(), 2); // b and c don't exist, so only expect a and d

    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_bUuid, dependencies, SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(dependencies.size(), 1); // c doesn't exist, so only expect d
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    // eliminate b --> c
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->RemoveSourceFileDependency(newEntry2.m_sourceDependencyID));

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_aUuid, dependencies, SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(dependencies.size(), 2); // a depends on b and d, but no longer c
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());
}

// Test to make sure dependencies on non-asset files are included
TEST_F(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_DependenciesOnNonAssetsIncluded)
{
    using namespace AzToolsFramework::AssetDatabase;

    //  A depends on B, which depends on both C and D

    // Delete b and c from the database, making them "non asset" files
    AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;
    m_assetProcessorManager->m_stateData->GetSourceBySourceGuid(m_bUuid, entry);
    m_assetProcessorManager->m_stateData->RemoveSource(entry.m_sourceID);

    m_assetProcessorManager->m_stateData->GetSourceBySourceGuid(m_cUuid, entry);
    m_assetProcessorManager->m_stateData->RemoveSource(entry.m_sourceID);

    SourceFileDependencyEntry newEntry1; // a depends on B
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_sourceGuid = m_aUuid;
    newEntry1.m_dependsOnSource = PathOrUuid("b.txt");

    SourceFileDependencyEntry newEntry2; // b depends on C
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_sourceGuid = m_bUuid;
    newEntry2.m_dependsOnSource = PathOrUuid("c.txt");

    SourceFileDependencyEntry newEntry3; // b also depends on D
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_sourceGuid = m_bUuid;
    newEntry3.m_dependsOnSource = PathOrUuid(m_dUuid);

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry1));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry2));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry3));

    AssetProcessor::SourceFilesForFingerprintingContainer dependencies;
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive(m_aUuid, dependencies, SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(dependencies.size(), 4);

    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(m_assetRootDir.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());
}

TEST_F(AssetProcessorManagerTest, BuilderSDK_API_CreateJobs_HasValidParameters_WithNoOutputFolder)
{
    // here we push a file change through APM and make sure that "CreateJobs" has correct parameters, with no output redirection
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/test_text.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    m_mockApplicationManager->ResetMockBuilderCreateJobCalls();

    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));

    // wait for AP to become idle.
    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_EQ(m_mockApplicationManager->GetMockBuilderCreateJobCalls(), 1);

    AZStd::shared_ptr<AssetProcessor::InternalMockBuilder> builderTxtBuilder;
    ASSERT_TRUE(m_mockApplicationManager->GetBuilderByID("txt files", builderTxtBuilder));

    const AssetBuilderSDK::CreateJobsRequest &req = builderTxtBuilder->GetLastCreateJobRequest();

    EXPECT_STREQ(req.m_watchFolder.c_str(), m_assetRootDir.absoluteFilePath("subfolder1").toUtf8().constData());
    EXPECT_STREQ(req.m_sourceFile.c_str(), "test_text.txt"); // only the name should be there, no output prefix.

    EXPECT_NE(req.m_sourceFileUUID, AZ::Uuid::CreateNull());
    EXPECT_TRUE(req.HasPlatform("pc"));
    EXPECT_TRUE(req.HasPlatformWithTag("desktop"));
}


TEST_F(AssetProcessorManagerTest, BuilderSDK_API_CreateJobs_HasValidParameters_WithOutputRedirectedFolder)
{
    // here we push a file change through APM and make sure that "CreateJobs" has correct parameters, with no output redirection
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder2/test_text.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    m_mockApplicationManager->ResetMockBuilderCreateJobCalls();

    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));

    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_EQ(m_mockApplicationManager->GetMockBuilderCreateJobCalls(), 1);

    AZStd::shared_ptr<AssetProcessor::InternalMockBuilder> builderTxtBuilder;
    ASSERT_TRUE(m_mockApplicationManager->GetBuilderByID("txt files", builderTxtBuilder));

    const AssetBuilderSDK::CreateJobsRequest &req = builderTxtBuilder->GetLastCreateJobRequest();

    // this test looks identical to the above test, but the important piece of information here is that
    // subfolder2 has its output redirected in the cache
    // this test makes sure that the CreateJobs API is completely unaffected by that and none of the internal database stuff
    // is reflected by the API.
    EXPECT_STREQ(req.m_watchFolder.c_str(), m_assetRootDir.absoluteFilePath("subfolder2").toUtf8().constData());
    EXPECT_STREQ(req.m_sourceFile.c_str(), "test_text.txt"); // only the name should be there, no output prefix.

    EXPECT_NE(req.m_sourceFileUUID, AZ::Uuid::CreateNull());
    EXPECT_TRUE(req.HasPlatform("pc"));
    EXPECT_TRUE(req.HasPlatformWithTag("desktop"));
}

void AbsolutePathProductDependencyTest::SetUp()
{
    using namespace AzToolsFramework::AssetDatabase;
    AssetProcessorManagerTest::SetUp();

    m_scanFolderInfo = m_config->GetScanFolderByPath(m_assetRootDir.absoluteFilePath("subfolder4"));
    ASSERT_TRUE(m_scanFolderInfo != nullptr);

    SourceDatabaseEntry sourceEntry(
        m_scanFolderInfo->ScanFolderID(),
        /*sourceName - arbitrary*/ "a.txt",
        AZ::Uuid::CreateRandom(),
        /*analysisFingerprint - arbitrary*/ "abcdefg");
    m_assetProcessorManager->m_stateData->SetSource(sourceEntry);

    AZ::Uuid mockBuilderUuid("{73AC8C3B-C30E-4C0D-97E4-4C5060C4E821}");
    JobDatabaseEntry jobEntry(
        sourceEntry.m_sourceID,
        /*jobKey - arbitrary*/ "Mock Job",
        /*fingerprint - arbitrary*/ 123456,
        m_testPlatform.c_str(),
        mockBuilderUuid,
        AzToolsFramework::AssetSystem::JobStatus::Completed,
        /*jobRunKey - arbitrary*/ 1);
    m_assetProcessorManager->m_stateData->SetJob(jobEntry);

    m_productToHaveDependency = ProductDatabaseEntry(
        jobEntry.m_jobID,
        /*subID - arbitrary*/ 0,
        /*productName - arbitrary*/ "a.output",
        AZ::Data::AssetType::CreateNull());
    m_assetProcessorManager->m_stateData->SetProduct(m_productToHaveDependency);
}


AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry AbsolutePathProductDependencyTest::SetAndReadAbsolutePathProductDependencyFromRelativePath(
    const AZStd::string& relativePath)
{
    using namespace AzToolsFramework::AssetDatabase;
    AZStd::string productAbsolutePath =
        AZStd::string::format("%s/%s", m_scanFolderInfo->ScanPath().toUtf8().data(), relativePath.c_str());
    AssetBuilderSDK::ProductPathDependencySet dependencies;
    dependencies.insert(
        AssetBuilderSDK::ProductPathDependency(productAbsolutePath, AssetBuilderSDK::ProductPathDependencyType::SourceFile));
    m_assetProcessorManager->m_pathDependencyManager->SaveUnresolvedDependenciesToDatabase(dependencies, m_productToHaveDependency, m_testPlatform);

    ProductDependencyDatabaseEntry productDependency;
    auto queryFunc = [&](ProductDependencyDatabaseEntry& productDependencyData)
    {
        productDependency = AZStd::move(productDependencyData);
        return false; // stop iterating after the first one. There should actually only be one entry.
    };
    m_assetProcessorManager->m_stateData->QueryUnresolvedProductDependencies(queryFunc);
    return productDependency;
}

AZStd::string AbsolutePathProductDependencyTest::BuildScanFolderRelativePath(const AZStd::string& relativePath) const
{
    // Scan folders write to the database with the $ character wrapped around the scan folder's ID.
    return AZStd::string::format("$%llu$%s", m_scanFolderInfo->ScanFolderID(), relativePath.c_str());
}

TEST_F(AbsolutePathProductDependencyTest, AbsolutePathProductDependency_MatchingFileNotAvailable_DependencyCorrectWithScanFolder)
{
    using namespace AzToolsFramework::AssetDatabase;

    AZStd::string dependencyRelativePath("some/file/path/filename.txt");
    ProductDependencyDatabaseEntry productDependency(SetAndReadAbsolutePathProductDependencyFromRelativePath(dependencyRelativePath));

    // When an absolute path product dependency is created, if part of that path matches a scan folder,
    // the part that matches is replaced with the scan folder's identifier, such as $1$, instead of the absolute path.
    AZStd::string expectedResult(BuildScanFolderRelativePath(dependencyRelativePath));
    ASSERT_EQ(productDependency.m_unresolvedPath, expectedResult);
    ASSERT_NE(productDependency.m_productDependencyID, InvalidEntryId);
    ASSERT_NE(productDependency.m_productPK, InvalidEntryId);
    ASSERT_TRUE(productDependency.m_dependencySourceGuid.IsNull());
    ASSERT_EQ(productDependency.m_platform, m_testPlatform);
}

TEST_F(AbsolutePathProductDependencyTest, AbsolutePathProductDependency_MixedCasePath_BecomesLowerCaseInDatabase)
{
    using namespace AzToolsFramework::AssetDatabase;

    AZStd::string dependencyRelativePath("Some/Mixed/Case/Path.txt");
    ProductDependencyDatabaseEntry productDependency(SetAndReadAbsolutePathProductDependencyFromRelativePath(dependencyRelativePath));

    AZStd::to_lower(dependencyRelativePath.begin(), dependencyRelativePath.end());
    AZStd::string expectedResult(BuildScanFolderRelativePath(dependencyRelativePath));
    ASSERT_EQ(productDependency.m_unresolvedPath, expectedResult);
    ASSERT_NE(productDependency.m_productDependencyID, InvalidEntryId);
    ASSERT_NE(productDependency.m_productPK, InvalidEntryId);
    ASSERT_TRUE(productDependency.m_dependencySourceGuid.IsNull());
    ASSERT_EQ(productDependency.m_platform, m_testPlatform);
}

TEST_F(AbsolutePathProductDependencyTest, AbsolutePathProductDependency_RetryDeferredDependenciesWithMatchingSource_DependencyResolves)
{
    using namespace AzToolsFramework::AssetDatabase;

    AZStd::string dependencyRelativePath("somefile.txt");
    ProductDependencyDatabaseEntry productDependency(SetAndReadAbsolutePathProductDependencyFromRelativePath(dependencyRelativePath));
    AZStd::string expectedResult(BuildScanFolderRelativePath(dependencyRelativePath));
    ASSERT_EQ(productDependency.m_unresolvedPath, expectedResult);
    ASSERT_NE(productDependency.m_productDependencyID, InvalidEntryId);
    ASSERT_NE(productDependency.m_productPK, InvalidEntryId);
    ASSERT_TRUE(productDependency.m_dependencySourceGuid.IsNull());
    ASSERT_EQ(productDependency.m_platform, m_testPlatform);

    AZ::Uuid sourceUUID("{4C7B8FD0-9D09-4DCB-A0BC-AEE85B063331}");
    SourceDatabaseEntry matchingSource(
        m_scanFolderInfo->ScanFolderID(),
        dependencyRelativePath.c_str(),
        sourceUUID,
        /*analysisFingerprint - arbitrary*/ "asdfasdf");
    m_assetProcessorManager->m_stateData->SetSource(matchingSource);

    AZ::Uuid mockBuilderUuid("{D314C2FD-757C-4FFA-BEA2-11D41925398A}");
    JobDatabaseEntry jobEntry(
        matchingSource.m_sourceID,
        /*jobKey - arbitrary*/ "Mock Job",
        /*fingerprint - arbitrary*/ 7654321,
        m_testPlatform.c_str(),
        mockBuilderUuid,
        AzToolsFramework::AssetSystem::JobStatus::Completed,
        /*jobRunKey - arbitrary*/ 2);
    m_assetProcessorManager->m_stateData->SetJob(jobEntry);
    ProductDatabaseEntry matchingProductForDependency(
        jobEntry.m_jobID,
        /*subID - arbitrary*/ 5,
        // The absolute path dependency here is to the source file, so the product's file and path
        // don't matter when resolving the dependency.
        /*productName - arbitrary*/ "b.output",
        AZ::Data::AssetType::CreateNull());
    m_assetProcessorManager->m_stateData->SetProduct(matchingProductForDependency);

    m_assetProcessorManager->m_pathDependencyManager->QueueSourceForDependencyResolution(matchingSource);
    m_assetProcessorManager->m_pathDependencyManager->ProcessQueuedDependencyResolves();

    // The product dependency ID shouldn't change when it goes from unresolved to resolved.
    AZStd::vector<ProductDependencyDatabaseEntry> resolvedProductDependencies;
    auto queryFunc = [&](ProductDependencyDatabaseEntry& productDependencyData)
    {
        resolvedProductDependencies.push_back(productDependencyData);
        return true;
    };
    m_assetProcessorManager->m_stateData->QueryProductDependencyByProductId(
        m_productToHaveDependency.m_productID,
        queryFunc);
    ASSERT_EQ(resolvedProductDependencies.size(), 1);
    // The path for a resolved entry should be empty.
    ASSERT_EQ(resolvedProductDependencies[0].m_unresolvedPath, "");
    // The ID and PK should not change.
    ASSERT_EQ(resolvedProductDependencies[0].m_productDependencyID, productDependency.m_productDependencyID);
    ASSERT_EQ(resolvedProductDependencies[0].m_productPK, productDependency.m_productPK);
    // The UUID should now be valid.
    ASSERT_EQ(resolvedProductDependencies[0].m_dependencySourceGuid, matchingSource.m_sourceGuid);
    ASSERT_EQ(resolvedProductDependencies[0].m_dependencySubID, matchingProductForDependency.m_subID);
    ASSERT_EQ(productDependency.m_platform, m_testPlatform);
}

void PathDependencyTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();

    AssetRecognizer rec;
    rec.m_name = "txt files2";
    rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
    rec.m_supportsCreateJobs = false;
    m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec);

    m_sharedConnection = m_assetProcessorManager->m_stateData.get();
    ASSERT_TRUE(m_sharedConnection);
}

void PathDependencyTest::TearDown()
{
    ASSERT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);

    AssetProcessorManagerTest::TearDown();
}

void PathDependencyTest::CaptureJobs(AZStd::vector<AssetProcessor::JobDetails>& jobDetailsList, const char* sourceFilePath)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;
    QString absPath(m_assetRootDir.absoluteFilePath(sourceFilePath));
    UnitTestUtils::CreateDummyFile(absPath, QString::number(QDateTime::currentMSecsSinceEpoch()));

    // prepare to capture the job details as the APM inspects the file.
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&jobDetailsList](JobDetails jobDetails)
    {
        jobDetailsList.push_back(jobDetails);
    });

    // tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    // Some tests intentionally finish with mixed slashes, so only use the corrected path to perform the job comparison.
    AZStd::string absPathCorrectSeparator(absPath.toUtf8().constData());
    AZStd::replace(absPathCorrectSeparator.begin(), absPathCorrectSeparator.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

    bool foundJob = false;

    for (const auto& details : jobDetailsList)
    {
        ASSERT_FALSE(details.m_autoFail);

        // we should have gotten at least one request to actually process that job:
        AZStd::string jobPath(details.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData());
        AZStd::replace(jobPath.begin(), jobPath.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

        if (jobPath == absPathCorrectSeparator)
        {
            foundJob = true;
        }
    }

    ASSERT_TRUE(foundJob);

    QObject::disconnect(connection);
}

bool PathDependencyTest::ProcessAsset(TestAsset& asset, const OutputAssetSet& outputAssets, const AssetBuilderSDK::ProductPathDependencySet& dependencies, const AZStd::string& folderPath, const AZStd::string& extension)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    AZStd::vector<JobDetails> capturedDetails;
    CaptureJobs(capturedDetails, (folderPath + asset.m_name + extension).c_str());

    // Make sure both counts are the same.  Otherwise certain code might not trigger
    EXPECT_EQ(capturedDetails.size(), outputAssets.size()) << "The number of captured jobs does not match the number of provided output assets.  This can cause AP to not consider the asset to be completely done.";

    int jobSet = 0;
    int subIdCounter = 1;

    for(const auto& outputSet : outputAssets)
    {
        ProcessJobResponse processJobResponse;
        processJobResponse.m_resultCode = ProcessJobResult_Success;

        for (const char* outputExtension : outputSet)
        {
            if(jobSet >= capturedDetails.size() || capturedDetails[jobSet].m_cachePath.empty())
            {
                return false;
            }

            auto filename = capturedDetails[jobSet].m_relativePath / (asset.m_name + outputExtension);

            AssetUtilities::ProductPath productPath{ filename.Native(), capturedDetails[jobSet].m_jobEntry.m_platformInfo.m_identifier };

            UnitTestUtils::CreateDummyFile(productPath.GetCachePath().c_str(), "this is a test output asset");

            JobProduct jobProduct(productPath.GetRelativePath(), AZ::Uuid::CreateRandom(), subIdCounter);
            jobProduct.m_pathDependencies.insert(dependencies.begin(), dependencies.end());

            processJobResponse.m_outputProducts.push_back(jobProduct);
            asset.m_products.push_back(AZ::Data::AssetId(capturedDetails[jobSet].m_jobEntry.m_sourceFileUUID, subIdCounter));

            subIdCounter++;
        }

        // tell the APM that the asset has been processed and allow it to bubble through its event queue:
        m_isIdling = false;
        m_assetProcessorManager->AssetProcessed(capturedDetails[jobSet].m_jobEntry, processJobResponse);
        m_assetProcessorManager->CheckForIdle();

        jobSet++;
    }

    return BlockUntilIdle(5000);
}

bool SearchDependencies(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer, AZ::Data::AssetId assetId)
{
    for (const auto& containerEntry : dependencyContainer)
    {
        if (containerEntry.m_dependencySourceGuid == assetId.m_guid && containerEntry.m_dependencySubID == assetId.m_subId)
        {
            return true;
        }
    }

    return false;
}

void VerifyDependencies(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer, AZStd::initializer_list<AZ::Data::AssetId> assetIds, AZStd::initializer_list<const char*> unresolvedPaths = {})
{
    EXPECT_EQ(dependencyContainer.size(), assetIds.size() + unresolvedPaths.size());

    for (const AZ::Data::AssetId& assetId : assetIds)
    {
        bool found = false;

        for (const auto& containerEntry : dependencyContainer)
        {
            if (containerEntry.m_dependencySourceGuid == assetId.m_guid && containerEntry.m_dependencySubID == assetId.m_subId)
            {
                found = true;
                break;
            }
        }

        ASSERT_TRUE(found) << "AssetId " << assetId.ToString<AZStd::string>().c_str() << " was not found";
    }

    for (const char* unresolvedPath : unresolvedPaths)
    {
        bool found = false;

        for (const auto& containerEntry : dependencyContainer)
        {
            if (containerEntry.m_unresolvedPath == unresolvedPath &&
                containerEntry.m_dependencySourceGuid.IsNull() &&
                containerEntry.m_dependencySubID == 0)
            {
                found = true;
                break;
            }
        }

        ASSERT_TRUE(found) << "Unresolved path " << unresolvedPath << " was not found";
    }
}

TEST_F(DuplicateProcessTest, SameAssetProcessedTwice_DependenciesResolveWithoutError)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QString sourceFilePath = "subfolder1/test.txt";
    AZStd::vector<JobDetails> jobDetailsList;
    ProductPathDependencySet dependencies = { {"dep1.txt", ProductPathDependencyType::SourceFile}, {"DEP2.asset2", ProductPathDependencyType::ProductFile}, {"Dep2.asset3", ProductPathDependencyType::ProductFile} };

    QString absPath(m_assetRootDir.absoluteFilePath(sourceFilePath));
    UnitTestUtils::CreateDummyFile(absPath);

    // prepare to capture the job details as the APM inspects the file.
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&jobDetailsList](JobDetails jobDetails)
    {
        jobDetailsList.push_back(jobDetails);
    });

    // tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    for(const auto& job : jobDetailsList)
    {
        ProcessJobResponse processJobResponse;
        processJobResponse.m_resultCode = ProcessJobResult_Success;

        {
            auto filename = "test.asset";
            QString outputAssetPath = (job.m_cachePath / filename).AsPosix().c_str();

            UnitTestUtils::CreateDummyFile(outputAssetPath, "this is a test output asset");

            JobProduct jobProduct(filename);
            jobProduct.m_pathDependencies.insert(dependencies.begin(), dependencies.end());

            processJobResponse.m_outputProducts.push_back(jobProduct);
        }

        // tell the APM that the asset has been processed and allow it to bubble through its event queue:
        m_isIdling = false;
        m_assetProcessorManager->AssetProcessed(job.m_jobEntry, processJobResponse);
    }

    ASSERT_TRUE(BlockUntilIdle(5000));

    TestAsset dep1("dep1");
    TestAsset dep2("deP2"); // random casing to make sure the search is case-insensitive

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1", ".asset2"} }));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1", ".asset2", ".asset3"} }));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1],
            dep2.m_products[1],
            dep2.m_products[2]
        }
    );
}

TEST_F(PathDependencyTest, NoLongerProcessedFile_IsRemoved)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockApplicationManager->UnRegisterAllBuilders();

    AssetRecognizer rec;
    rec.m_name = "txt files2";
    rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
    rec.m_supportsCreateJobs = false;
    m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec);

    AzFramework::AssetSystem::AssetNotificationMessage details;

    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetMessage, [&details](AzFramework::AssetSystem::AssetNotificationMessage message)
        {
            details = message;
        });

    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/test1.txt"));

    TestAsset testAsset("test1");

    ASSERT_TRUE(ProcessAsset(testAsset, { {".asset1"} }));

    AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
    m_sharedConnection->GetProductsBySourceName("test1.txt", products);

    ASSERT_EQ(products.size(), 1);
    ASSERT_TRUE(QFile::exists(m_normalizedCacheRootDir.absoluteFilePath("pc/test1.asset1").toUtf8().constData()));

    m_mockApplicationManager->UnRegisterAllBuilders();

    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    products.clear();
    m_sharedConnection->GetProductsBySourceName("test1.txt", products);
    ASSERT_EQ(products.size(), 0);

    ASSERT_FALSE(QFile::exists(m_normalizedCacheRootDir.absoluteFilePath("pc/automatedtesting/test1.asset1").toUtf8().constData()));
}

TEST_F(PathDependencyTest, AssetProcessed_Impl_SelfReferrentialProductDependency_DependencyIsRemoved)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    TestAsset mainFile("testFileName");
    AZStd::vector<JobDetails> capturedDetails;
    CaptureJobs(capturedDetails, ("subfolder1/" + mainFile.m_name + ".txt").c_str());

    ASSERT_FALSE(capturedDetails.empty());

    JobDetails jobDetails = capturedDetails[0];
    AZ::Uuid outputAssetTypeId = AZ::Uuid::CreateRandom();
    int subId = 1;

    ProcessJobResponse processJobResponse;
    processJobResponse.m_resultCode = ProcessJobResult_Success;

    ASSERT_FALSE(jobDetails.m_cachePath.empty());

    // create a product asset
    auto filename = mainFile.m_name + ".asset";
    QString outputAssetPath = (jobDetails.m_cachePath / filename).AsPosix().c_str();
    UnitTestUtils::CreateDummyFile(outputAssetPath, "this is a test output asset");

    // add the new product asset to its own product dependencies list by assetId
    JobProduct jobProduct(filename, outputAssetTypeId, subId);
    AZ::Data::AssetId productAssetId(jobDetails.m_jobEntry.m_sourceFileUUID, subId);
    jobProduct.m_dependencies.push_back(ProductDependency(productAssetId, 5));

    // add the product asset to its own product dependencies list by path
    jobProduct.m_pathDependencies.emplace(ProductPathDependency(AZStd::string::format("%s%s", mainFile.m_name.c_str(), ".asset"), ProductPathDependencyType::ProductFile));

    processJobResponse.m_outputProducts.push_back(jobProduct);
    mainFile.m_products.push_back(productAssetId);

    // tell the APM that the asset has been processed and allow it to bubble through its event queue:
    m_errorAbsorber->Clear();
    m_assetProcessorManager->AssetProcessed(jobDetails.m_jobEntry, processJobResponse);
    ASSERT_TRUE(BlockUntilIdle(5000));

    // Verify we have no entries in the ProductDependencies table
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    m_sharedConnection->GetProductDependencies(dependencyContainer);
    ASSERT_TRUE(dependencyContainer.empty());

    // We are testing 2 different dependencies, so we should get 2 warnings
    ASSERT_EQ(m_errorAbsorber->m_numWarningsAbsorbed, 2);
    m_errorAbsorber->Clear();
}

// This test shows the process of deferring resolution of a path dependency works.
// 1) Resource A comes in with a relative path to resource B which has not been processed yet
// 2) Resource B is processed, resolving the path dependency on resource A
TEST_F(PathDependencyTest, AssetProcessed_Impl_DeferredPathResolution)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    AZStd::vector<TestAsset> dependencySources = { "dep1", "dep2" };
    // Start with mixed casing.
    ProductPathDependencySet dependencies = { {"Dep1.txt", AssetBuilderSDK::ProductPathDependencyType::SourceFile}, {"DEP2.asset2", AssetBuilderSDK::ProductPathDependencyType::ProductFile}, {"dep2.asset3", AssetBuilderSDK::ProductPathDependencyType::ProductFile} }; // Test depending on a source asset, and on a subset of product assets

    TestAsset mainFile("test_text");
    ASSERT_TRUE(ProcessAsset(mainFile, { { ".asset" }, {} }, dependencies));

    // ---------- Verify that we have unresolved path in ProductDependencies table ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));
    ASSERT_EQ(dependencyContainer.size(), dependencies.size());

    // All dependencies are stored lowercase in the database. Make the expected dependencies lowercase here to match that.
    for(auto& dependency : dependencies)
    {
        AZStd::to_lower(dependency.m_dependencyPath.begin(), dependency.m_dependencyPath.end());
    }

    for (const auto& dependency : dependencyContainer)
    {
        AssetBuilderSDK::ProductPathDependency actualDependency(dependency.m_unresolvedPath, (dependency.m_dependencyType == AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType::ProductDep_SourceFile) ? ProductPathDependencyType::SourceFile : ProductPathDependencyType::ProductFile);

        ASSERT_THAT(dependencies, ::testing::Contains(actualDependency));
        // Verify that the unresolved path dependency is null.
        ASSERT_TRUE(dependency.m_dependencySourceGuid.IsNull());
    }

    // -------- Process the dependencies to resolve the path dependencies in the first product -----
    for(TestAsset& dependency : dependencySources)
    {
        ASSERT_TRUE(ProcessAsset(dependency, { { ".asset1", ".asset2" }, { ".asset3" } }, {}));
    }

    // ---------- Verify that path has been found and resolved ----------
    dependencyContainer.clear();
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dependencySources[0].m_products[0],
            dependencySources[0].m_products[1],
            dependencySources[0].m_products[2],
            dependencySources[1].m_products[1],
            dependencySources[1].m_products[2],
        }
    );
}

// This test shows process of how a path dependency is resolved when it is pointing to an asset that has already been processed
// 1) Resource A is processed, and has with no relative path dependencies
// 2) Resource B is processed, has a path dependency on resource A
// 3) An entry is made in the product dependencies table but does not have anything in the unresolved path field
TEST_F(PathDependencyTest, AssetProcessed_Impl_DeferredPathResolutionAlreadyResolvable)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // create dependees
    TestAsset dep1("dep1");
    TestAsset dep2("deP2"); // random casing to make sure the search is case-insensitive

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1", ".asset2"}, {".asset3"} }));

    // -------- Make main test asset, with dependencies on products we just created -----
    TestAsset primaryFile("test_text");
    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} }, { {"dep1.txt", ProductPathDependencyType::SourceFile}, {"DEP2.asset2", ProductPathDependencyType::ProductFile}, {"Dep2.asset3", ProductPathDependencyType::ProductFile} }));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1],
            dep2.m_products[1],
            dep2.m_products[2]
        }
    );
}

// In most cases, it's expected that asset references (simple and regular) will be only to product files, not source files.
// Unfortunately, with some legacy systems, this isn't necessary true. To maximize compatibility, the PathDependencyManager
// does a sanity check on file extensions when for path product dependencies. If it sees a source image format (bmp, tif, jpg, and other supported formats)
// it will swap the dependency from a product dependency to a source dependency.
TEST_F(PathDependencyTest, PathProductDependency_SourceImageFileAsProduct_BecomesSourceDependencyInDB)
{
    using namespace AzToolsFramework::AssetDatabase;

    AZStd::string sourceImageFileExtension("imagefile.bmp");

    TestAsset primaryFile("some_file");
    ASSERT_TRUE(ProcessAsset(
        primaryFile,
        /*outputAssets*/{ { ".asset" }, {} },
        /*dependencies*/{ {sourceImageFileExtension, AssetBuilderSDK::ProductPathDependencyType::ProductFile} }));

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));
    ASSERT_EQ(dependencyContainer.size(), 1);
    ASSERT_STREQ(dependencyContainer[0].m_unresolvedPath.c_str(), sourceImageFileExtension.c_str());
    // Verify the dependency type was swapped from product to source.
    ASSERT_EQ(dependencyContainer[0].m_dependencyType, ProductDependencyDatabaseEntry::DependencyType::ProductDep_SourceFile);
}

TEST_F(PathDependencyTest, PathProductDependency_MixedSlashes_BecomesCorrectSeparatorInDB)
{
    using namespace AzToolsFramework::AssetDatabase;

    AZStd::string dependencyRelativePathMixedSlashes("some\\path/with\\mixed/slashes.txt");

    TestAsset primaryFile("some_file");
    ASSERT_TRUE(ProcessAsset(
        primaryFile,
        /*outputAssets*/ { { ".asset" }, {} },
        /*dependencies*/ { {dependencyRelativePathMixedSlashes, AssetBuilderSDK::ProductPathDependencyType::SourceFile} }));

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {},
        {
            // This string is copy & pasted instead of replacing AZ_WRONG_FILESYSTEM_SEPARATOR with AZ_CORRECT_FILESYSTEM_SEPARATOR
            // to improve readability of this test.
            "some/path/with/mixed/slashes.txt"
        });
}

TEST_F(PathDependencyTest, PathProductDependency_DoubleSlashes_BecomesCorrectSeparatorInDB)
{
    using namespace AzToolsFramework::AssetDatabase;

    AZStd::string dependencyRelativePathMixedSlashes("some\\\\path//with\\double/slashes.txt");

    TestAsset primaryFile("some_file");
    ASSERT_TRUE(ProcessAsset(
        primaryFile,
        /*outputAssets*/{ { ".asset" }, {} },
        /*dependencies*/{ {dependencyRelativePathMixedSlashes, AssetBuilderSDK::ProductPathDependencyType::SourceFile} }));

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {},
        {
            // This string is copy & pasted instead of replacing AZ_WRONG_FILESYSTEM_SEPARATOR with AZ_CORRECT_FILESYSTEM_SEPARATOR
            // to improve readability of this test.
            "some/path/with/double/slashes.txt"
        });
}

TEST_F(PathDependencyTest, WildcardDependencies_Existing_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // create dependees
    TestAsset dep1("dep1");
    TestAsset dep2("deP2"); // random casing to make sure the search is case-insensitive
    TestAsset dep3("dep3");
    TestAsset dep4("1deP1");

    bool result = ProcessAsset(dep1, { {".asset1"}, {".asset2"} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(dep2, { {".asset1", ".asset2"}, {".asset3"} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(dep3, { {".asset1", ".asset2"}, {".asset3"} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(dep4, { {".asset1"}, {".asset3"} }); // This product will match on both dependencies, this will check to make sure we don't get duplicates
    ASSERT_TRUE(result) << "Failed to Process Assets";

    // -------- Make main test asset, with dependencies on products we just created -----
    TestAsset primaryFile("test_text");
    result = ProcessAsset(primaryFile, { { ".asset" }, {} }, { {"*p1.txt", ProductPathDependencyType::SourceFile}, {"*.asset3", ProductPathDependencyType::ProductFile} });
    ASSERT_TRUE(result) << "Failed to Process main test asset";

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    result = m_sharedConnection->GetProductDependencies(dependencyContainer);
    ASSERT_TRUE(result)<< "Failed to Get Product Dependencies";

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1],
            dep2.m_products[2],
            dep3.m_products[2],
            dep4.m_products[0],
            dep4.m_products[1]
        },
        { "*p1.txt", "*.asset3" }
    );
}

TEST_F(PathDependencyTest, WildcardDependencies_ExcludePathsExisting_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // create dependees
    TestAsset dep1("dep1");
    TestAsset depdep1("dep/dep1");
    TestAsset depdepdep1("dep/dep/dep1");
    TestAsset dep2("dep2");
    TestAsset depdep2("dep/dep2");
    TestAsset depdepdep2("dep/dep/dep2");
    TestAsset dep3("dep3");

    bool result = ProcessAsset(dep1, { {".asset1"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(depdep1, { {".asset2"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(depdepdep1, { {".asset2"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(dep2, { {".asset3"}, {".asset4"} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(depdep2, { {".asset3"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(depdepdep2, { {".asset3"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(dep3, { {".asset4"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    // -------- Make two main test assets, with dependencies on products we just created -----
    TestAsset primaryFile1("test_text_1");
    result = ProcessAsset(primaryFile1, { { ".asset" }, {} }, {
        {"*p1.txt", ProductPathDependencyType::SourceFile},
        {"dep3.txt", ProductPathDependencyType::SourceFile},
        {":dep3.txt", ProductPathDependencyType::SourceFile},
        {":dep/dep/*p1.txt", ProductPathDependencyType::SourceFile},
        {":dep/dep1.txt", ProductPathDependencyType::SourceFile},
        {"*.asset3", ProductPathDependencyType::ProductFile},
        {"dep2.asset4", ProductPathDependencyType::ProductFile},
        {":dep/dep/dep2.asset3", ProductPathDependencyType::ProductFile},
        {":dep/dep/dep/dep/*.asset3", ProductPathDependencyType::ProductFile},
        {":dep2.asset4", ProductPathDependencyType::ProductFile}});
    ASSERT_TRUE(result) << "Failed to Process main test asset " << primaryFile1.m_name.c_str();

    TestAsset primaryFile2("test_text_2");
    result = ProcessAsset(primaryFile2, { { ".asset" }, {} }, {
        {"*p1.txt", ProductPathDependencyType::SourceFile},
        {"*.asset3", ProductPathDependencyType::ProductFile} });
    ASSERT_TRUE(result) << "Failed to Process main test asset" << primaryFile2.m_name.c_str();

    AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer productContainer;
    result = m_sharedConnection->GetProducts(productContainer);
    ASSERT_TRUE(result) << "Failed to Get Products";

    // ---------- Verify that the dependency was recorded and excluded paths were not resolved ----------
    auto product = AZStd::find_if(productContainer.begin(), productContainer.end(),
        [&primaryFile1](const auto& product)
    {
        return product.m_productName.ends_with(primaryFile1.m_name + ".asset");
    });
    ASSERT_TRUE(product != productContainer.end()) << "Failed to Get Product of " << primaryFile1.m_name.c_str();

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    result = m_sharedConnection->GetProductDependenciesByProductID(product->m_productID, dependencyContainer);
    ASSERT_TRUE(result) << "Failed to Get Product Dependencies";

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep2.m_products[0]
        },
        { "*p1.txt", "dep3.txt", ":dep3.txt", ":dep/dep/*p1.txt", ":dep/dep1.txt",
        "*.asset3", "dep2.asset4", ":dep/dep/dep2.asset3", ":dep/dep/dep/dep/*.asset3", ":dep2.asset4" }
    );

    // ---------- Verify that the dependency was recorded and the excluded path dependencies defined for another asset didn't effect the product dependencies of the current one ----------
    product = AZStd::find_if(productContainer.begin(), productContainer.end(),
        [&primaryFile2](const auto& product)
    {
        return product.m_productName.ends_with(primaryFile2.m_name + ".asset");
    });
    ASSERT_TRUE(product != productContainer.end()) << "Failed to Get Product of " << primaryFile2.m_name.c_str();

    dependencyContainer.clear();
    result = m_sharedConnection->GetProductDependenciesByProductID(product->m_productID, dependencyContainer);
    ASSERT_TRUE(result) << "Failed to Get Product Dependencies";

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            depdep1.m_products[0],
            depdepdep1.m_products[0],
            dep2.m_products[0],
            depdep2.m_products[0],
            depdepdep2.m_products[0],
        },
        { "*p1.txt", "*.asset3" }
    );

    // Test asset PrimaryFile1 has 4 conflict dependencies
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 4);
    m_errorAbsorber->Clear();
}

TEST_F(PathDependencyTest, WildcardDependencies_Deferred_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // -------- Make main test asset, with dependencies on products that don't exist yet -----
    TestAsset primaryFile("test_text");
    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} }, { {"*p1.txt", ProductPathDependencyType::SourceFile}, {"*.asset3", ProductPathDependencyType::ProductFile} }));

    // create dependees
    TestAsset dep1("dep1");
    TestAsset dep2("deP2"); // random casing to make sure the search is case-insensitive
    TestAsset dep3("dep3");
    TestAsset dep4("1deP1");

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1", ".asset2"}, {".asset3"} }));
    ASSERT_TRUE(ProcessAsset(dep3, { {".asset1", ".asset2"}, {".asset3"} }));
    ASSERT_TRUE(ProcessAsset(dep4, { {".asset1"}, {} }));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1],
            dep2.m_products[2],
            dep3.m_products[2],
            dep4.m_products[0]
        },
        { "*p1.txt", "*.asset3" }
    );
}

TEST_F(PathDependencyTest, WildcardDependencies_ExcludedPathDeferred_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // -------- Make two main test assets, with dependencies on products that don't exist yet -----
    TestAsset primaryFile1("test_text_1");
    bool result = ProcessAsset(primaryFile1, { { ".asset" }, {} }, {
        {"*p1.txt", ProductPathDependencyType::SourceFile},
        {"dep3.txt", ProductPathDependencyType::SourceFile},
        {":dep3.txt", ProductPathDependencyType::SourceFile},
        {":dep/dep/*p1.txt", ProductPathDependencyType::SourceFile},
        {":dep/dep1.txt", ProductPathDependencyType::SourceFile},
        {"*.asset3", ProductPathDependencyType::ProductFile},
        {"dep2.asset4", ProductPathDependencyType::ProductFile},
        {":dep/dep/dep2.asset3", ProductPathDependencyType::ProductFile},
        {":dep/dep/dep/dep/*.asset3", ProductPathDependencyType::ProductFile},
        {":dep2.asset4", ProductPathDependencyType::ProductFile}});
    ASSERT_TRUE(result) << "Failed to Process main test asset";

    TestAsset primaryFile2("test_text_2");
    result = ProcessAsset(primaryFile2, { { ".asset" }, {} }, {
        {"*p1.txt", ProductPathDependencyType::SourceFile},
        {"*.asset3", ProductPathDependencyType::ProductFile} });
    ASSERT_TRUE(result) << "Failed to Process main test asset";

    // create dependees
    TestAsset dep1("dep1");
    TestAsset depdep1("dep/dep1");
    TestAsset depdepdep1("dep/dep/dep1");
    TestAsset dep2("dep2");
    TestAsset depdep2("dep/dep2");
    TestAsset depdepdep2("dep/dep/dep2");
    TestAsset dep3("dep3");

    result = ProcessAsset(dep1, { {".asset1"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(depdep1, { {".asset2"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(depdepdep1, { {".asset2"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(dep2, { {".asset3"}, {".asset4"} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(depdep2, { {".asset3"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(depdepdep2, { {".asset3"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    result = ProcessAsset(dep3, { {".asset4"}, {} });
    ASSERT_TRUE(result) << "Failed to Process Assets";

    AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer productContainer;
    result = m_sharedConnection->GetProducts(productContainer);
    ASSERT_TRUE(result) << "Failed to Get Products";

    // ---------- Verify that the dependency was recorded and exlcuded paths were not resolved ----------
    auto product = AZStd::find_if(productContainer.begin(), productContainer.end(),
        [&primaryFile1](const auto& product)
    {
        return product.m_productName.ends_with(primaryFile1.m_name + ".asset");
    });
    ASSERT_TRUE(product != productContainer.end()) << "Failed to Get Product of " << primaryFile1.m_name.c_str();

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    result = m_sharedConnection->GetProductDependenciesByProductID(product->m_productID, dependencyContainer);
    ASSERT_TRUE(result) << "Failed to Get Product Dependencies";

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep2.m_products[0]
        },
        { "*p1.txt", "dep3.txt", ":dep3.txt", ":dep/dep/*p1.txt", ":dep/dep1.txt",
        "*.asset3", "dep2.asset4", ":dep/dep/dep2.asset3", ":dep/dep/dep/dep/*.asset3", ":dep2.asset4" }
    );

    // ---------- Verify that the dependency was recorded and the excluded path dependencies defined for another asset didn't effect the product dependencies of the current one ----------
    product = AZStd::find_if(productContainer.begin(), productContainer.end(),
        [&primaryFile2](const auto& product)
    {
        return product.m_productName.ends_with(primaryFile2.m_name + ".asset");
    });
    ASSERT_TRUE(product != productContainer.end()) << "Failed to Get Product of " << primaryFile2.m_name.c_str();

    dependencyContainer.clear();
    result = m_sharedConnection->GetProductDependenciesByProductID(product->m_productID, dependencyContainer);
    ASSERT_TRUE(result) << "Failed to Get Product Dependencies";

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            depdep1.m_products[0],
            depdepdep1.m_products[0],
            dep2.m_products[0],
            depdep2.m_products[0],
            depdepdep2.m_products[0],
        },
        { "*p1.txt", "*.asset3" }
    );

    // Test asset PrimaryFile1 has 4 conflict dependencies
    // After test assets dep2 and dep3 are processed,
    // another 2 errors will be raised because of the confliction
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 6);
    m_errorAbsorber->Clear();
}

void PathDependencyTest::RunWildcardTest(bool useCorrectDatabaseSeparator, AssetBuilderSDK::ProductPathDependencyType pathDependencyType, bool buildDependenciesFirst)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // create dependees
    // Wildcard resolution of paths with back slashes is not supported on non-windows platforms, so we need to construct those test cases differently
    TestAsset matchingDepWithForwardSlash("testFolder/someFileName");

    AZStd::string depWithPlatformCompatibleSlash;
    AzFramework::StringFunc::Path::Join("testFolder", "anotherFileName", depWithPlatformCompatibleSlash);
    TestAsset matchingDepWithPlatformCompatibleSlash(depWithPlatformCompatibleSlash.c_str());

    AZStd::string depWithMixedSlashes;
    AzFramework::StringFunc::Path::Join("someRootFolder/testFolder", "anotherFileName", depWithMixedSlashes, true, false);
    TestAsset matchingDepDeeperFolderMixedSlashes(depWithMixedSlashes.c_str());

    TestAsset notMatchingDepInSubfolder("unmatchedFolder/arbitraryFileName");

    if (buildDependenciesFirst)
    {
        ASSERT_TRUE(ProcessAsset(matchingDepWithForwardSlash, { {".asset"}, {} })) << "Failed to Process " << matchingDepWithForwardSlash.m_name.c_str();
        ASSERT_TRUE(ProcessAsset(matchingDepWithPlatformCompatibleSlash, { {".asset"}, {} })) << "Failed to Process " << matchingDepWithPlatformCompatibleSlash.m_name.c_str();
        ASSERT_TRUE(ProcessAsset(matchingDepDeeperFolderMixedSlashes, { {".asset"}, {} })) << "Failed to Process " << matchingDepDeeperFolderMixedSlashes.m_name.c_str();
        ASSERT_TRUE(ProcessAsset(notMatchingDepInSubfolder, { {".asset"}, {} })) << "Failed to Process " << notMatchingDepInSubfolder.m_name.c_str();
    }

    // -------- Make main test asset, with dependencies on products we just created -----
    TestAsset primaryFile("test_text");
    const char* databaseSeparator = useCorrectDatabaseSeparator ? AZ_CORRECT_DATABASE_SEPARATOR_STRING : AZ_WRONG_DATABASE_SEPARATOR_STRING;

    AZStd::string extension = (pathDependencyType == ProductPathDependencyType::SourceFile) ? "txt" : "asset";
    AZStd::string wildcardString = AZStd::string::format("*testFolder%s*.%s", databaseSeparator, extension.c_str());

    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} }, { {wildcardString.c_str(), pathDependencyType}, })) << "Failed to Process " << primaryFile.m_name.c_str();

    if (!buildDependenciesFirst)
    {
        ASSERT_TRUE(ProcessAsset(matchingDepWithForwardSlash, { {".asset"}, {} })) << "Failed to Process " << matchingDepWithForwardSlash.m_name.c_str();
        ASSERT_TRUE(ProcessAsset(matchingDepWithPlatformCompatibleSlash, { {".asset"}, {} })) << "Failed to Process " << matchingDepWithPlatformCompatibleSlash.m_name.c_str();
        ASSERT_TRUE(ProcessAsset(matchingDepDeeperFolderMixedSlashes, { {".asset"}, {} })) << "Failed to Process " << matchingDepDeeperFolderMixedSlashes.m_name.c_str();
        ASSERT_TRUE(ProcessAsset(notMatchingDepInSubfolder, { {".asset"}, {} })) << "Failed to Process " << notMatchingDepInSubfolder.m_name.c_str();
    }

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    // Dependencies are always written to the database in lower case with the correct separator.
    AZStd::to_lower(wildcardString.begin(), wildcardString.end());
    AZStd::replace(wildcardString.begin(), wildcardString.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

    VerifyDependencies(dependencyContainer,
        {
            matchingDepWithForwardSlash.m_products[0],
            matchingDepWithPlatformCompatibleSlash.m_products[0],
            matchingDepDeeperFolderMixedSlashes.m_products[0]
        },
        // Paths become lowercase in the DB
        { wildcardString.c_str() }
    );
}

TEST_F(PathDependencyTest, WildcardSourcePathDependenciesWithForwardSlash_Existing_ResolveCorrectly)
{
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ true,
        AssetBuilderSDK::ProductPathDependencyType::SourceFile,
        /*buildDependenciesFirst*/ true);
}

TEST_F(PathDependencyTest, WildcardSourcePathDependenciesWithBackSlash_Existing_ResolveCorrectly)
{
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ false,
        AssetBuilderSDK::ProductPathDependencyType::SourceFile,
        /*buildDependenciesFirst*/ true);
}

TEST_F(PathDependencyTest, WildcardSourcePathDependenciesWithForwardSlash_Deferred_ResolveCorrectly)
{
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ true,
        AssetBuilderSDK::ProductPathDependencyType::SourceFile,
        /*buildDependenciesFirst*/ false);
}

TEST_F(PathDependencyTest, WildcardSourcePathDependenciesWithBackSlash_Deferred_ResolveCorrectly)
{
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ false,
        AssetBuilderSDK::ProductPathDependencyType::SourceFile,
        /*buildDependenciesFirst*/ false);
}

TEST_F(PathDependencyTest, WildcardProductPathDependenciesWithForwardSlash_Existing_ResolveCorrectly)
{
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ true,
        AssetBuilderSDK::ProductPathDependencyType::ProductFile,
        /*buildDependenciesFirst*/ true);
}

TEST_F(PathDependencyTest, WildcardProductPathDependenciesWithBackSlash_Existing_ResolveCorrectly)
{
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ false,
        AssetBuilderSDK::ProductPathDependencyType::ProductFile,
        /*buildDependenciesFirst*/ true);
}

TEST_F(PathDependencyTest, WildcardProductPathDependenciesWithForwardSlash_Deferred_ResolveCorrectly)
{
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ true,
        AssetBuilderSDK::ProductPathDependencyType::ProductFile,
        /*buildDependenciesFirst*/ false);
}

TEST_F(PathDependencyTest, WildcardProductPathDependenciesWithBackSlash_Deferred_ResolveCorrectly)
{
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ false,
        AssetBuilderSDK::ProductPathDependencyType::ProductFile,
        /*buildDependenciesFirst*/ false);
}

TEST_F(PathDependencyTest, Wildcard_ResolvingTwice_DependenciesNotDuplicated)
{
    // Regression test: make sure resolving the dependencies twice doesn't result in duplicate entries in the database

    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ true,
        AssetBuilderSDK::ProductPathDependencyType::ProductFile,
        /*buildDependenciesFirst*/ true
    );
    RunWildcardTest(
        /*useCorrectDatabaseSeparator*/ true,
        AssetBuilderSDK::ProductPathDependencyType::ProductFile,
        /*buildDependenciesFirst*/ false);
}


// Tests product path dependencies using absolute paths to source files
TEST_F(PathDependencyTest, AbsoluteDependencies_Existing_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/dep1.txt"));

    // create dependees
    TestAsset dep1("dep1");

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }));

    // -------- Make main test asset, with dependencies on products we just created -----
    TestAsset primaryFile("test_text");

    ASSERT_TRUE(ProcessAsset(primaryFile, { {".asset"} , {} }, { {absPath.toUtf8().constData(), ProductPathDependencyType::SourceFile} }));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1]
        }
    );
}

// Tests product path dependencies using absolute paths to source files
TEST_F(PathDependencyTest, AbsoluteDependencies_Deferred_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    AZStd::string relativePathDep1("dep1.txt");
    QString absPathDep1(m_assetRootDir.absoluteFilePath(QString("subfolder4%1%2").arg(QDir::separator()).arg(relativePathDep1.c_str())));

    auto scanfolder4 = m_config->GetScanFolderForFile(absPathDep1);
    // When an absolute path matches a scan folder, the portion of the path matching that scan folder
    // is replaced with the scan folder's ID.
    AZStd::string absPathDep1WithScanfolder(AZStd::string::format("$%" PRId64 "$%s", aznumeric_cast<int64_t>(scanfolder4->ScanFolderID()), relativePathDep1.c_str()));
    QString absPathDep2(m_assetRootDir.absoluteFilePath("subfolder2/redirected/dep2.txt"));
    QString absPathDep3(m_assetRootDir.absoluteFilePath("subfolder1/dep3.txt"));

    // -------- Make main test asset, with dependencies on products that don't exist yet -----
    TestAsset primaryFile("test_text");
    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} },
        {
            {absPathDep1.toUtf8().constData(), ProductPathDependencyType::SourceFile},
            {absPathDep2.toUtf8().constData(), ProductPathDependencyType::SourceFile},
            {absPathDep3.toUtf8().constData(), ProductPathDependencyType::SourceFile},
        }
    ));

    // create dependees
    TestAsset dep1("dep1");
    TestAsset dep2("dep2");
    TestAsset dep3("dep3");

    // Different scanfolder, same relative file name.  This should *not* trigger the dependency.  We can't test with another asset in the proper scanfolder because AssetIds are based on relative file name,
    // which means both assets have the same AssetId and there would be no way to tell which one matched
    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }, {}, "subfolder1/"));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1"}, {".asset2"} }, {}, "subfolder2/redirected/"));
    ASSERT_TRUE(ProcessAsset(dep3, { {".asset1"}, {".asset2"} }, {}, "subfolder1/")); // test a normal dependency with no prefix

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep2.m_products[0],
            dep2.m_products[1],
            dep3.m_products[0],
            dep3.m_products[1],
        }, { absPathDep1WithScanfolder.c_str() }
    );
}

TEST_F(PathDependencyTest, ChangeDependencies_Existing_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/dep1.txt"));

    // create dependees
    TestAsset dep1("dep1");

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }));

    // -------- Make main test asset, with dependencies on products we just created -----
    TestAsset primaryFile("test_text");

    ASSERT_TRUE(ProcessAsset(primaryFile, { {".asset"} , {} }, { {"dep1.*", ProductPathDependencyType::SourceFile} }));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1]
        },
        { "dep1.*" }
    );

    // Update again with different dependencies
    ASSERT_TRUE(ProcessAsset(primaryFile, { {".asset"} , {} }, { {absPath.toUtf8().constData(), ProductPathDependencyType::SourceFile} }));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    dependencyContainer.clear();
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1]
        }
    );
}

TEST_F(PathDependencyTest, MixedPathDependencies_Existing_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // create dependees
    TestAsset dep1("dep1");
    TestAsset dep2("deP2"); // random casing to make sure the search is case-insensitive
    TestAsset dep3("dep3");
    TestAsset dep4("dep4");
    TestAsset dep5("dep5");

    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/folderA/folderB/dep5.txt"));

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }, {}, "subfolder1/folderA/folderB/"));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1", ".asset2"}, {".asset3"} }, {}, "subfolder1/folderA/folderB/"));
    ASSERT_TRUE(ProcessAsset(dep3, { {".asset1", ".asset2"}, {".asset3"} }, {}, "subfolder1/folderA/folderB/"));
    ASSERT_TRUE(ProcessAsset(dep4, { {".asset1", ".asset2"}, {".asset3"} }, {}, "subfolder1/folderA/folderB/"));
    ASSERT_TRUE(ProcessAsset(dep5, { {".asset1"}, {} }, {}, "subfolder1/folderA/folderB/"));

    // -------- Make main test asset, with dependencies on products we just created -----
    TestAsset primaryFile("test_text");
    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} }, {
        {"folderA/folderB\\*1.txt", ProductPathDependencyType::SourceFile}, // wildcard source
        {"folderA/folderB\\*2.asset3", ProductPathDependencyType::ProductFile}, // wildcard product
        {"folderA/folderB\\dep3.txt", ProductPathDependencyType::SourceFile}, // relative source
        {"folderA/folderB\\dep4.asset3", ProductPathDependencyType::ProductFile}, // relative product
        {absPath.toUtf8().constData(), ProductPathDependencyType::SourceFile}, // absolute source
    }));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1],
            dep2.m_products[2],
            dep3.m_products[0],
            dep3.m_products[1],
            dep3.m_products[2],
            dep4.m_products[2],
            dep5.m_products[0],
        }, { "foldera/folderb/*1.txt", "foldera/folderb/*2.asset3" } // wildcard dependencies always leave an unresolved entry
    );
}

TEST_F(PathDependencyTest, MixedPathDependencies_Deferred_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // create dependees
    TestAsset dep1("dep1");
    TestAsset dep2("deP2"); // random casing to make sure the search is case-insensitive
    TestAsset dep3("dep3");
    TestAsset dep4("dep4");
    TestAsset dep5("dep5");

    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/folderA\\folderB/dep5.txt"));

    // -------- Make main test asset, with dependencies on products that don't exist yet -----
    TestAsset primaryFile("test_text");
    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} }, {
        {"folderA/folderB\\*1.txt", ProductPathDependencyType::SourceFile}, // wildcard source
        {"folderA/folderB\\*2.asset3", ProductPathDependencyType::ProductFile}, // wildcard product
        {"folderA/folderB\\dep3.txt", ProductPathDependencyType::SourceFile}, // relative source
        {"folderA/folderB\\dep4.asset3", ProductPathDependencyType::ProductFile}, // relative product
        {absPath.toUtf8().constData(), ProductPathDependencyType::SourceFile}, // absolute source
        }));

    // create dependees
    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }, {}, "subfolder1/folderA/folderB/"));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1", ".asset2"}, {".asset3"} }, {}, "subfolder1/folderA/folderB/"));
    ASSERT_TRUE(ProcessAsset(dep3, { {".asset1", ".asset2"}, {".asset3"} }, {}, "subfolder1/folderA/folderB/"));
    ASSERT_TRUE(ProcessAsset(dep4, { {".asset1", ".asset2"}, {".asset3"} }, {}, "subfolder1/folderA/folderB/"));
    ASSERT_TRUE(ProcessAsset(dep5, { {".asset1"}, {} }, {}, "subfolder1/folderA/folderB/"));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep1.m_products[0],
            dep1.m_products[1],
            dep2.m_products[2],
            dep3.m_products[0],
            dep3.m_products[1],
            dep3.m_products[2],
            dep4.m_products[2],
            dep5.m_products[0],
        }, { "foldera/folderb/*1.txt", "foldera/folderb/*2.asset3" } // wildcard dependencies always leave an unresolved entry
    );
}

void MultiplatformPathDependencyTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();
    m_config = nullptr; // Make sure to clear this out first so the existing config can cleanup before we allocate the new one
    m_config.reset(new AssetProcessor::PlatformConfiguration());
    m_config->EnablePlatform({ "pc", { "host", "renderer", "desktop" } }, true);
    m_config->EnablePlatform({ "provo",{ "console" } }, true);

    m_config->AddScanFolder(ScanFolderInfo(m_assetRootDir.filePath("subfolder1"), "subfolder1", "subfolder1", false, true, m_config->GetEnabledPlatforms()));
    m_config->AddScanFolder(ScanFolderInfo(m_assetRootDir.filePath("subfolder2"), "subfolder2", "subfolder2", false, true, m_config->GetEnabledPlatforms()));
    m_config->AddScanFolder(ScanFolderInfo(m_assetRootDir.filePath("subfolder3"), "subfolder3", "subfolder3", false, true, m_config->GetEnabledPlatforms()));
    m_config->AddIntermediateScanFolder();

    m_assetProcessorManager = nullptr; // we need to destroy the previous instance before creating a new one
    m_assetProcessorManager = AZStd::make_unique<AssetProcessorManager_Test>(m_config.get());

    m_isIdling = false;

    m_idleConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState, [this](bool newState)
    {
        m_isIdling = newState;
    });


    // Get rid of all the other builders, and add a builder that will process for both platforms
    m_mockApplicationManager->UnRegisterAllBuilders();
    AssetRecognizer rec;

    rec.m_name = "multiplatform txt files";
    rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
    rec.m_platformSpecs.insert({"provo", AssetInternalSpec::Copy});
    rec.m_supportsCreateJobs = false;
    m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec);

    AssetRecognizer rec2;

    rec2.m_name = "single platform ini files";
    rec2.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ini", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec2.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
    rec2.m_supportsCreateJobs = false;
    m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec2);
}

TEST_F(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies)
{
    // One product will be pc, one will be console (order is non-deterministic)
    TestAsset asset1("testAsset1");
    ASSERT_TRUE(ProcessAsset(asset1, { { ".asset1" },{ ".asset1b" } }, {}));

    // Create a new asset that will only get processed by one platform, make it depend on both products of testAsset1
    TestAsset asset2("asset2");
    ASSERT_TRUE(ProcessAsset(asset2, { { ".asset1" } }, { { "testAsset1.asset1", AssetBuilderSDK::ProductPathDependencyType::ProductFile },{ "testAsset1.asset1b", AssetBuilderSDK::ProductPathDependencyType::ProductFile } }, "subfolder1/", ".ini"));

    AssetDatabaseConnection* sharedConnection = m_assetProcessorManager->m_stateData.get();
    ASSERT_TRUE(sharedConnection);

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    // Since asset2 was only made for one platform only one of its dependencies should be resolved.
    sharedConnection->GetProductDependencies(dependencyContainer);
    int resolvedCount = 0;
    int unresolvedCount = 0;
    for (const auto& dep : dependencyContainer)
    {
        if (dep.m_unresolvedPath.empty())
        {
            resolvedCount++;
        }
        else
        {
            unresolvedCount++;
        }
    }
    ASSERT_EQ(resolvedCount, 1);
    ASSERT_EQ(unresolvedCount, 1);
    ASSERT_NE(SearchDependencies(dependencyContainer, asset1.m_products[0]), SearchDependencies(dependencyContainer, asset1.m_products[1]));
}

TEST_F(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies_DeferredResolution)
{
    // Create a new asset that will only get processed by one platform, make it depend on both products of testAsset1
    TestAsset asset2("asset2");
    ASSERT_TRUE(ProcessAsset(asset2, { { ".asset1" } }, { { "testAsset1.asset1", AssetBuilderSDK::ProductPathDependencyType::ProductFile },{ "testAsset1.asset1b", AssetBuilderSDK::ProductPathDependencyType::ProductFile } }, "subfolder1/", ".ini"));

    // One product will be pc, one will be console (order is non-deterministic)
    TestAsset asset1("testAsset1");
    ASSERT_TRUE(ProcessAsset(asset1, { { ".asset1" },{ ".asset1b" } }, {}));

    AssetDatabaseConnection* sharedConnection = m_assetProcessorManager->m_stateData.get();
    ASSERT_TRUE(sharedConnection);

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    // Since asset2 was only made for one platform only one of its dependencies should be resolved.
    sharedConnection->GetProductDependencies(dependencyContainer);
    int resolvedCount = 0;
    int unresolvedCount = 0;
    for (const auto& dep : dependencyContainer)
    {
        if (dep.m_unresolvedPath.empty())
        {
            resolvedCount++;
        }
        else
        {
            unresolvedCount++;
        }
    }
    ASSERT_EQ(resolvedCount, 1);
    ASSERT_EQ(unresolvedCount, 1);
    ASSERT_NE(SearchDependencies(dependencyContainer, asset1.m_products[0]), SearchDependencies(dependencyContainer, asset1.m_products[1]));
}

TEST_F(MultiplatformPathDependencyTest, SameFilenameForAllPlatforms)
{
    TestAsset asset2("asset2");
    bool result = ProcessAsset(
        asset2, { { ".output" }, { ".output" } }, { { "*1.output", AssetBuilderSDK::ProductPathDependencyType::ProductFile } }, "subfolder1/",
        ".txt");

    ASSERT_TRUE(result);

    TestAsset asset1("asset1");
    result = ProcessAsset(asset1, { { ".output" }, { ".output" } }, {});

    ASSERT_TRUE(result);

    AssetDatabaseConnection* sharedConnection = m_assetProcessorManager->m_stateData.get();
    ASSERT_TRUE(sharedConnection);

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;

    sharedConnection->GetProductDependencies(dependencyContainer);
    int resolvedCount = 0;
    int unresolvedCount = 0;
    for (const auto& dep : dependencyContainer)
    {
        if (dep.m_unresolvedPath.empty())
        {
            resolvedCount++;
        }
        else
        {
            unresolvedCount++;
        }
    }
    ASSERT_EQ(resolvedCount, 2);
    ASSERT_EQ(unresolvedCount, 2);
    VerifyDependencies(dependencyContainer, { asset1.m_products[0], asset1.m_products[1] }, { "*1.output", "*1.output" });
}

TEST_F(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies_SourcePath)
{
    // One product will be pc, one will be console (order is non-deterministic)
    TestAsset asset1("testAsset1");
    ASSERT_TRUE(ProcessAsset(asset1, { { ".asset1" },{ ".asset1b" } }, {}));

    // Create a new asset that will only get processed by one platform, make it depend on both products of testAsset1
    TestAsset asset2("asset2");
    ASSERT_TRUE(ProcessAsset(asset2, { { ".asset1" } }, { { "testAsset1.txt", AssetBuilderSDK::ProductPathDependencyType::SourceFile } }, "subfolder1/", ".ini"));

    AssetDatabaseConnection* sharedConnection = m_assetProcessorManager->m_stateData.get();
    ASSERT_TRUE(sharedConnection);

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    // Since asset2 was only made for one platform only one of its dependencies should be resolved.
    sharedConnection->GetProductDependencies(dependencyContainer);
    int resolvedCount = 0;
    int unresolvedCount = 0;
    for (const auto& dep : dependencyContainer)
    {
        if (dep.m_unresolvedPath.empty())
        {
            resolvedCount++;
        }
        else
        {
            unresolvedCount++;
        }
    }
    ASSERT_EQ(resolvedCount, 1);
    ASSERT_EQ(unresolvedCount, 0);
    ASSERT_NE(SearchDependencies(dependencyContainer, asset1.m_products[0]), SearchDependencies(dependencyContainer, asset1.m_products[1]));
}

// this tests exists to make sure a bug does not regress.
// when the bug was active, dependencies would be stored in the database incorrectly when different products emitted different dependencies.
// specifically, any dependency emitted by any product of a given source would show up as a dependency of ALL products for that source.
TEST_F(AssetProcessorManagerTest, AssetProcessedImpl_DifferentProductDependenciesPerProduct_SavesCorrectlyToDatabase)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;
    /// --------------------- SETUP PHASE - make an asset exist in the database -------------------

    // Create the source file.
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/test_text.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    // prepare to capture the job details as the APM inspects the file.
    JobDetails capturedDetails;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&capturedDetails](JobDetails jobDetails)
    {
        capturedDetails = jobDetails;
    });

    // tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_FALSE(capturedDetails.m_autoFail);

    QObject::disconnect(connection);

    // we should have gotten at least one request to actually process that job:
    ASSERT_STREQ(capturedDetails.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData(), absPath.toUtf8().constData());

    // now simulate the job being done and actually returning a full job finished details which includes dependencies:
    ProcessJobResponse response;
    response.m_resultCode = ProcessJobResult_Success;

    QString destTestPath1 = (capturedDetails.m_cachePath / "test1.txt").AsPosix().c_str();
    QString destTestPath2 = (capturedDetails.m_cachePath / "test2.txt").AsPosix().c_str();

    UnitTestUtils::CreateDummyFile(destTestPath1, "this is the first output");
    UnitTestUtils::CreateDummyFile(destTestPath2, "this is the second output");

    JobProduct productA("test1.txt", AZ::Uuid::CreateRandom(), 1);
    JobProduct productB("test2.txt", AZ::Uuid::CreateRandom(), 2);
    AZ::Data::AssetId expectedIdOfProductA(capturedDetails.m_jobEntry.m_sourceFileUUID, productA.m_productSubID);
    AZ::Data::AssetId expectedIdOfProductB(capturedDetails.m_jobEntry.m_sourceFileUUID, productB.m_productSubID);

    productA.m_dependencies.push_back(ProductDependency(expectedIdOfProductB, 5));
    productB.m_dependencies.push_back(ProductDependency(expectedIdOfProductA, 6));
    response.m_outputProducts.push_back(productA);
    response.m_outputProducts.push_back(productB);

    // tell the APM that the asset has been processed and allow it to bubble through its event queue:
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(capturedDetails.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));
    // note that there exists different tests (in the AssetStateDatabase tests) to directly test the actual database store/get for this
    // the purpose of this test is to just make sure that the Asset Processor Manager actually understood the job dependencies
    // and correctly stored the results into the dependency table.

    //-------------------------------- EVALUATION PHASE -------------------------
    // at this point, the AP will have filed the asset away in its database and we can now validate that it actually
    // did it correctly.
    // We expect to see two dependencies in the dependency table, each with the correct dependency, no duplicates, no lost data.
    AssetDatabaseConnection* sharedConnection = m_assetProcessorManager->m_stateData.get();

    AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry> capturedTableEntries;

    ASSERT_TRUE(sharedConnection);
    AZStd::size_t countFound = 0;
    bool queryresult = sharedConnection->QueryProductDependenciesTable(
        [&capturedTableEntries, &countFound](AZ::Data::AssetId& asset, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
    {
        ++countFound;
        capturedTableEntries[asset] = entry;
        return true;
    });

    ASSERT_TRUE(queryresult);

    // this also asserts uniqueness.
    ASSERT_EQ(countFound, 2);
    ASSERT_EQ(capturedTableEntries.size(), countFound); // if they were not unique asset IDs, they would have collapsed on top of each other.

    // make sure both assetIds are present:
    ASSERT_NE(capturedTableEntries.find(expectedIdOfProductA), capturedTableEntries.end());
    ASSERT_NE(capturedTableEntries.find(expectedIdOfProductB), capturedTableEntries.end());

    // make sure both refer to the other and nothing else.
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductA].m_dependencySourceGuid, expectedIdOfProductB.m_guid);
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductA].m_dependencySubID, expectedIdOfProductB.m_subId);
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductA].m_dependencyFlags, 5);

    EXPECT_EQ(capturedTableEntries[expectedIdOfProductB].m_dependencySourceGuid, expectedIdOfProductA.m_guid);
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductB].m_dependencySubID, expectedIdOfProductA.m_subId);
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductB].m_dependencyFlags, 6);
}


// this test exists to make sure a bug does not regress.
// when the bug was active, source files with multiple products would cause the asset processor to repeatedly process them
// due to a timing problem.  Specifically, if the products were not successfully moved to the output directory quickly enough
// it would assume something was wrong, and re-trigger the job, which cancelled the already-in-flight job currently busy copying
// the product files to the cache to finalize it.
TEST_F(AssetProcessorManagerTest, AssessDeletedFile_OnJobInFlight_IsIgnored)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // constants to adjust - if this regresses you can turn it up much higher for a stress test.
    const int numOutputsToSimulate = 50;

    // --------------------- SETUP PHASE - make an asset exist in the database as if the job is complete -------------------
    // The asset needs multiple job products.

    // Create the source file.
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/test_text.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    // prepare to capture the job details as the APM inspects the file.
    JobDetails capturedDetails;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&capturedDetails](JobDetails jobDetails)
    {
        capturedDetails = jobDetails;
    });

    // tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    QObject::disconnect(connection);

    // we should have gotten at least one request to actually process that job:
    ASSERT_STREQ(capturedDetails.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData(), absPath.toUtf8().constData());

    // now simulate the job being done and actually returning a full job finished details which includes dependencies:
    ProcessJobResponse response;
    response.m_resultCode = ProcessJobResult_Success;
    for (int outputIdx = 0; outputIdx < numOutputsToSimulate; ++outputIdx)
    {
        auto fileNameToGenerate = AZStd::string::format("test%d.txt", outputIdx);
        QString filePathToGenerate = (capturedDetails.m_cachePath / fileNameToGenerate).AsPosix().c_str();

        UnitTestUtils::CreateDummyFile(filePathToGenerate, "an output");
        JobProduct product(fileNameToGenerate, AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(outputIdx));
        response.m_outputProducts.push_back(product);
    }

    // tell the APM that the asset has been processed and allow it to bubble through its event queue:
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(capturedDetails.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));

    // at this point, everything should be up to date and ready for the test - there should be one source in the database
    // with numOutputsToSimulate products.
    // now, we simulate a job running to process the asset again, by modifying the timestamp on the file to be at least one second later.
    // this is because on some operating systems (such as mac) the resolution of file time stamps is at least one second.
#ifdef AZ_PLATFORM_WINDOWS
    int milliseconds = 10;
#else
    int milliseconds = 1001;
#endif
    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(milliseconds));
    UnitTestUtils::CreateDummyFile(absPath, "Completely different file data");

    // with the source file changed, tell it to process it again:
    // prepare to capture the job details as the APM inspects the file.
    connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&capturedDetails](JobDetails jobDetails)
    {
        capturedDetails = jobDetails;
    });

    // tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    QObject::disconnect(connection);
    // we should have gotten at least one request to actually process that job:
    ASSERT_STREQ(capturedDetails.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData(), absPath.toUtf8().constData());
    ASSERT_FALSE(capturedDetails.m_autoFail);
    ASSERT_FALSE(capturedDetails.m_cachePath.empty());
    // ----------------------------- TEST BEGINS HERE -----------------------------
    // simulte a very slow computer processing the file one output at a time and feeding file change notifies:

    // FROM THIS POINT ON we should see no new job create / cancellation or anything since we're just going to be messing with the cache.
    bool gotUnexpectedAssetToProcess = false;
    connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&gotUnexpectedAssetToProcess](JobDetails /*jobDetails*/)
    {
        gotUnexpectedAssetToProcess = true;
    });

    // this function tells APM about a file and waits for it to idle, if waitForIdle is true.
    // basically, it simulates the file watcher firing on events from the cache since file watcher events
    // come in on the queue at any time a file changes, sourced from a different thread.
    auto notifyAPM = [this, &gotUnexpectedAssetToProcess](const char* functionToCall, QString filePath, bool waitForIdle)
    {
        if (waitForIdle)
        {
            m_isIdling = false;
        }
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), functionToCall, Qt::QueuedConnection, Q_ARG(QString, QString(filePath)));
        if (waitForIdle)
        {
            ASSERT_TRUE(BlockUntilIdle(5000));
        }

        ASSERT_FALSE(gotUnexpectedAssetToProcess);
    };

    response = AssetBuilderSDK::ProcessJobResponse();
    response.m_resultCode = ProcessJobResult_Success;
    for (int outputIdx = 0; outputIdx < numOutputsToSimulate; ++outputIdx)
    {
        // every second one, we dont wait at all and let it rapidly process, to preturb the timing.
        bool shouldBlockAndWaitThisTime = outputIdx % 2 == 0;

        auto fileNameToGenerate = AZStd::string::format("test%d.txt", outputIdx);
        QString filePathToGenerate = (capturedDetails.m_cachePath / fileNameToGenerate).AsPosix().c_str();

        JobProduct product(fileNameToGenerate, AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(outputIdx));
        response.m_outputProducts.push_back(product);

        AssetProcessor::ProcessingJobInfoBus::Broadcast(&AssetProcessor::ProcessingJobInfoBus::Events::BeginCacheFileUpdate, filePathToGenerate.toUtf8().data());

        AZ::IO::SystemFile::Delete(filePathToGenerate.toUtf8().constData());

        // simulate the file watcher showing the deletion occuring:
        notifyAPM("AssessDeletedFile", filePathToGenerate, shouldBlockAndWaitThisTime);
        UnitTestUtils::CreateDummyFile(filePathToGenerate, "an output");

        // let the APM go for a significant amount of time so that it simulates a slow thread copying a large file with lots of events about it pouring in.
        for (int repeatLoop = 0; repeatLoop < 100; ++repeatLoop)
        {
            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, QString(filePathToGenerate)));
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 1);
            ASSERT_FALSE(gotUnexpectedAssetToProcess);
        }

        // also toss it a "cache modified" call to make sure that this does not spawn further jobs
        // note that assessing modified files in the cache should not result in it spawning jobs or even becoming unidle since it
        // actually ignores modified files in the cache.
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, QString(filePathToGenerate)));
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 1);
        ASSERT_FALSE(gotUnexpectedAssetToProcess);

        // now tell it to stop ignoring the cache delete and let it do the next one.
        EBUS_EVENT(AssetProcessor::ProcessingJobInfoBus, EndCacheFileUpdate, filePathToGenerate.toUtf8().data(), false);

        // simulate a "late" deletion notify coming from the file monitor that it outside the "ignore delete" section.  This should STILL not generate additional
        // deletion notifies as it should ignore these if the file in fact actually there when it gets around to checking it
        notifyAPM("AssessDeletedFile", filePathToGenerate, shouldBlockAndWaitThisTime);
    }

    // tell the APM that the asset has been processed and allow it to bubble through its event queue:
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(capturedDetails.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));
    ASSERT_FALSE(gotUnexpectedAssetToProcess);

    QObject::disconnect(connection);
}

void SourceFileDependenciesTest::SetupData(
    const AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceFileDependencies,
    const AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
    bool createFile1Dummies,
    bool createFile2Dummies,
    bool primeMap,
    AssetProcessor::AssetProcessorManager::JobToProcessEntry& job)
{
    // make sure that if we publish some dependencies, they appear:
    m_dummyBuilderUuid = AZ::Uuid::CreateRandom();
    QString relFileName("assetProcessorManagerTest.txt");
    m_absPath = m_assetRootDir.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt");
    m_watchFolderPath = m_assetRootDir.absoluteFilePath("subfolder1");
    m_scanFolder = m_config->GetScanFolderByPath(m_watchFolderPath);
    ASSERT_NE(m_scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    m_dependsOnFile1_Source = m_assetRootDir.absoluteFilePath("subfolder1/a.txt");
    m_dependsOnFile2_Source = m_assetRootDir.absoluteFilePath("subfolder1/b.txt");
    m_dependsOnFile1_Job = m_assetRootDir.absoluteFilePath("subfolder1/c.txt");
    m_dependsOnFile2_Job = m_assetRootDir.absoluteFilePath("subfolder1/d.txt");

    if (createFile1Dummies)
    {
        CreateSourceAndFile("subfolder1/a.txt");
        CreateSourceAndFile("subfolder1/c.txt");
    }

    if (createFile2Dummies)
    {
        CreateSourceAndFile("subfolder1/b.txt");
        CreateSourceAndFile("subfolder1/d.txt");
    }

    // construct the dummy job to feed to the database updater function:
    job.m_sourceFileInfo.m_sourceAssetReference = AssetProcessor::SourceAssetReference(m_absPath);
    job.m_sourceFileInfo.m_scanFolder = m_scanFolder;
    job.m_sourceFileInfo.m_uuid = AssetUtilities::CreateSafeSourceUUIDFromName(job.m_sourceFileInfo.m_sourceAssetReference.RelativePath().c_str());

    if (primeMap)
    {
        m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[job.m_sourceFileInfo.m_uuid] = job.m_sourceFileInfo.m_sourceAssetReference;
    }

    for (const auto& sourceFileDependency : sourceFileDependencies)
    {
        job.m_sourceFileDependencies.emplace_back(m_dummyBuilderUuid, sourceFileDependency);
    }

    // it is currently assumed that the only fields that we care about in JobDetails is the builder busId and the job dependencies
    // themselves:
    JobDetails newDetails;
    newDetails.m_assetBuilderDesc.m_busId = m_dummyBuilderUuid;

    for (const auto& jobDependency : jobDependencies)
    {
        newDetails.m_jobDependencyList.push_back(jobDependency);
    }

    job.m_jobsToAnalyze.push_back(newDetails);

    // this is the one line that this unit test is really testing:
    m_assetProcessorManager->UpdateSourceFileDependenciesDatabase(job);
}

void SourceFileDependenciesTest::PopulateDatabase()
{
    using namespace AzToolsFramework::AssetDatabase;

    AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder(
        m_assetRootDir.absoluteFilePath("subfolder1").toUtf8().constData(), "temp path", "temp path");
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetScanFolder(scanFolder));

    CreateSourceAndFile("subFolder1/assetProcessorManagerTest.txt");
}

AssetBuilderSDK::SourceFileDependency SourceFileDependenciesTest::MakeSourceDependency(const char* file, bool wildcard)
{
    return { file, AZ::Uuid::CreateNull(),
             wildcard ? AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards
                      : AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Absolute };
}

AssetBuilderSDK::SourceFileDependency SourceFileDependenciesTest::MakeSourceDependency(AZ::Uuid uuid)
{
    return { "", uuid };
}

AssetBuilderSDK::JobDependency SourceFileDependenciesTest::MakeJobDependency(const char* file)
{
    return AssetBuilderSDK::JobDependency("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, MakeSourceDependency(file));
}

AssetBuilderSDK::JobDependency SourceFileDependenciesTest::MakeJobDependency(AZ::Uuid uuid)
{
    return AssetBuilderSDK::JobDependency("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, MakeSourceDependency(uuid));
}

auto SourceFileDependenciesTest::GetDependencyList()
{
    AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer deps;
    this->m_assetProcessorManager->m_stateData->GetSourceFileDependenciesByBuilderGUIDAndSource(
        m_dummyBuilderUuid, m_sourceFileUuid,
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency::DEP_Any, deps);

    AZStd::vector<AZStd::string> list;

    for (auto&& entry : deps)
    {
        list.push_back(entry.m_dependsOnSource.ToString());
    }

    return list;
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_BasicTest)
{
    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData({ MakeSourceDependency("a.txt"), MakeSourceDependency(m_uuidOfB) }, { MakeJobDependency("c.txt"), MakeJobDependency(m_uuidOfD) }, true, true, true, job);

    // the rest of this test now performs a series of queries to verify the database was correctly set.
    // this indirectly verifies the QueryAbsolutePathDependenciesRecursive function also but it has its own dedicated tests, above.
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Source.toUtf8().constData()), deps.end());

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Job.toUtf8().constData()), deps.end());

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 5);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Job.toUtf8().constData()), deps.end());
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_UpdateTest)
{
    // make sure that if we remove dependencies that are published, they disappear.
    // so the first part of this test is to put some data in there, the same as before:

    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData({ MakeSourceDependency("a.txt"), MakeSourceDependency(m_uuidOfB) }, { MakeJobDependency("c.txt"), MakeJobDependency(m_uuidOfD) }, true, true, true, job);

    // in this test, though, we delete some after pushing them in there, and update it again:
    job.m_sourceFileDependencies.pop_back(); // erase the 'b' dependency.
    job.m_jobsToAnalyze[0].m_jobDependencyList.pop_back(); // erase the 'd' dependency, which is by guid.
    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);

    // now make sure that the same queries omit b and d:
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid)
{
    // make sure that if we publish some dependencies, they do not appear if they are missing

    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData({ MakeSourceDependency("a.txt"), MakeSourceDependency(m_uuidOfB) }, { MakeJobDependency("c.txt"), MakeJobDependency(m_uuidOfD) }, false, true, true, job);

    // the rest of this test now performs a series of queries to verify the database was correctly set.
    // this indirectly verifies the QueryAbsolutePathDependenciesRecursive function also but it has its own dedicated tests, above.
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource);

    // we should find all of the deps, but not the placeholders.

    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Source.toUtf8().constData()), deps.end()); // b

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Job.toUtf8().constData()), deps.end()); // d

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Source.toUtf8().constData()), deps.end()); // b
    EXPECT_NE(deps.find(m_dependsOnFile2_Job.toUtf8().constData()), deps.end()); // d
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByName)
{
    // make sure that if we publish some dependencies, they do not appear if missing

    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData({ MakeSourceDependency("a.txt"), MakeSourceDependency(m_uuidOfB) }, { MakeJobDependency("c.txt"), MakeJobDependency(m_uuidOfD) }, true, false, false, job);

    // the rest of this test now performs a series of queries to verify the database was correctly set.
    // this indirectly verifies the QueryAbsolutePathDependenciesRecursive function also but it has its own dedicated tests, above.
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource);

    // we should find all of the deps, but a and c are missing and thus should not appear.
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());   // a

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());  // c

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any);
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());  // a
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());     // c
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid_UpdatesWhenTheyAppear)
{
    // this test makes sure that when files DO appear that were previously placeholders, the database is updated
    // so the strategy here is to  have files b, and d missing, which are declared as dependencies by UUID.
    // then, we make them re-appear later, and check that the database has updated them appropriately.

    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData({ MakeSourceDependency("a.txt"), MakeSourceDependency(m_uuidOfB) }, { MakeJobDependency("c.txt"), MakeJobDependency(m_uuidOfD) }, true, false, false, job);

    // so at this point, the database should be in the same state as after the UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid test
    // which was already verified, by that test.

    // now that the database has placeholders, we expect them to resolve themselves when we provide the actual files:
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(m_dependsOnFile2_Source, QString("tempdata\n")));
    // now that B exists, we pretend a job came in to process B. (it doesn't require dependencies to be declared)
    // note that we have to "prime" the map with the UUIDs to the source info for this to work:
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[m_uuidOfB] = SourceAssetReference(m_watchFolderPath, "b.txt");

    AssetProcessorManager::JobToProcessEntry job2;
    job2.m_sourceFileInfo.m_sourceAssetReference = AssetProcessor::SourceAssetReference(m_watchFolderPath, "b.txt");
    job2.m_sourceFileInfo.m_scanFolder = m_scanFolder;
    job2.m_sourceFileInfo.m_uuid = m_uuidOfB;

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job2);

    // b should no longer be a placeholder, so both A and B should be present as their actual path.
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());   // a
    EXPECT_NE(deps.find(m_dependsOnFile2_Source.toUtf8().constData()), deps.end());   // b

    // but d should still be a placeholder, since we have not declared it yet.
    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());  // c

    // now make d exist too and pretend a job came in to process it:
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(m_dependsOnFile2_Job, QString("tempdata\n"))); // create file D
    AssetProcessorManager::JobToProcessEntry job3;
    job3.m_sourceFileInfo.m_sourceAssetReference = AssetProcessor::SourceAssetReference(m_watchFolderPath, "d.txt");
    job3.m_sourceFileInfo.m_scanFolder = m_scanFolder;
    job3.m_sourceFileInfo.m_uuid = m_uuidOfD;
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[m_uuidOfD] = SourceAssetReference{ m_watchFolderPath, "d.txt" };

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job3);

    // all files should now be present:
    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any);
    EXPECT_EQ(deps.size(), 5);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Job.toUtf8().constData()), deps.end());
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByName_UpdatesWhenTheyAppear)
{
    // this test makes sure that when files DO appear that were previously placeholders, the database is updated
    // so the strategy here is to  have files a, and c missing, which are declared as dependencies by name.
    // then, we make them re-appear later, and check that the database has updated them appropriately.

    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData({ MakeSourceDependency("a.txt"), MakeSourceDependency(m_uuidOfB) }, { MakeJobDependency("c.txt"), MakeJobDependency(m_uuidOfD) }, false, true, true, job);

    // so at this point, the database should be in the same state as after the UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid test
    // which was already verified, by that test.

    // now that the database has placeholders, we expect them to resolve themselves when we provide the actual files:
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(m_dependsOnFile1_Source, QString("tempdata\n")));
    // now that A exists, we pretend a job came in to process a. (it doesn't require dependencies to be declared)
    AssetProcessorManager::JobToProcessEntry job2;
    job2.m_sourceFileInfo.m_sourceAssetReference = AssetProcessor::SourceAssetReference(m_watchFolderPath, "a.txt");
    job2.m_sourceFileInfo.m_scanFolder = m_scanFolder;
    job2.m_sourceFileInfo.m_uuid = m_uuidOfA;
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[m_uuidOfA] = SourceAssetReference{ m_watchFolderPath, "a.txt" };

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job2);

    // a should no longer be a placeholder
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());   // a
    EXPECT_NE(deps.find(m_dependsOnFile2_Source.toUtf8().constData()), deps.end());   // b
    deps.clear();

    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Job.toUtf8().constData()), deps.end());  // d

    // now make c exist too and pretend a job came in to process it:
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(m_dependsOnFile1_Job, QString("tempdata\n")));
    AZ::Uuid uuidOfC = AssetUtilities::CreateSafeSourceUUIDFromName("c.txt");
    AssetProcessorManager::JobToProcessEntry job3;
    job3.m_sourceFileInfo.m_sourceAssetReference = AssetProcessor::SourceAssetReference(m_watchFolderPath, "c.txt");
    job3.m_sourceFileInfo.m_scanFolder = m_scanFolder;
    job3.m_sourceFileInfo.m_uuid = uuidOfC;
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[m_uuidOfC] = SourceAssetReference{ m_watchFolderPath, "c.txt" };

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job3);

    // all files should now be present:
    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(m_sourceFileUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any);
    EXPECT_EQ(deps.size(), 5);
    EXPECT_NE(deps.find(m_absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile1_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(m_dependsOnFile2_Job.toUtf8().constData()), deps.end());
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_DuplicateSourceDependencies)
{
    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData({
        MakeSourceDependency("a.txt"),
        MakeSourceDependency("a.txt"),
        MakeSourceDependency(m_uuidOfA),
        MakeSourceDependency(m_uuidOfB),
        MakeSourceDependency(m_uuidOfB) },
        {}, true, true, true, job);

    auto actualDependencies = GetDependencyList();

    EXPECT_THAT(actualDependencies, ::testing::UnorderedElementsAre(
            "a.txt", m_uuidOfA.ToFixedString(false, false).c_str(), m_uuidOfB.ToFixedString(false, false).c_str()
    ));
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_DuplicateJobDependencies)
{
    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData({ }, {
            MakeJobDependency("c.txt"),
            MakeJobDependency("c.txt"),
            MakeJobDependency(m_uuidOfC),
            MakeJobDependency(m_uuidOfD),
        },
        true, true, true, job);

    auto actualDependencies = GetDependencyList();

    EXPECT_THAT(
        actualDependencies,
        ::testing::UnorderedElementsAre(
            "c.txt", m_uuidOfC.ToFixedString(false, false).c_str(), m_uuidOfD.ToFixedString(false, false).c_str()));
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_JobAndSourceDependenciesDuplicated)
{
    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData(
        {
            MakeSourceDependency("a.txt"),
            MakeSourceDependency(m_uuidOfB)
        },
        {
            MakeJobDependency(m_uuidOfA),
            MakeJobDependency("b.txt"),
        },
        true, true, true, job);

    auto actualDependencies = GetDependencyList();

    EXPECT_THAT(
        actualDependencies,
        ::testing::UnorderedElementsAre(
            "a.txt",
            m_uuidOfA.ToFixedString(false, false).c_str(),
            "b.txt",
            m_uuidOfB.ToFixedString(false, false).c_str()));
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_SourceDependenciesDuplicatedWildcard)
{
    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData(
        {
            MakeSourceDependency("a.txt"),
            MakeSourceDependency("a.t*t", true),
            MakeSourceDependency(m_uuidOfB),
        }, {}, true, true, true, job);

    auto actualDependencies = GetDependencyList();

    EXPECT_THAT(
        actualDependencies,
        ::testing::UnorderedElementsAre(
            "a.txt",
            "a.t%t",
            m_uuidOfB.ToFixedString(false, false).c_str()));
}

TEST_F(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_AbsolutePathIsPreserved)
{
    QDir tempPath(m_assetRootDir.path());
    QString absPath = tempPath.absoluteFilePath("subfolder1/a.txt");

    AssetProcessor::AssetProcessorManager::JobToProcessEntry job;
    SetupData(
        {
            MakeSourceDependency(absPath.toUtf8().constData()),
        },
        {},
        true,
        true,
        true,
        job);

    auto actualDependencies = GetDependencyList();

    EXPECT_THAT(actualDependencies, ::testing::UnorderedElementsAre(absPath.toUtf8().constData()));
}

TEST_F(AssetProcessorManagerTest, JobDependencyOrderOnce_MultipleJobs_EmitOK)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QString watchFolderPath = m_assetRootDir.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);
    const char relSourceFileName[] = "a.dummy";
    const char secondRelSourceFile[] = "b.dummy";
    QString sourceFileName = m_assetRootDir.absoluteFilePath("subfolder1/a.dummy");
    QString secondSourceFile = m_assetRootDir.absoluteFilePath("subfolder1/b.dummy");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(sourceFileName, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(secondSourceFile, QString("tempdata\n")));

    AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
    builderDescriptor.m_name = "Test Dummy Builder";
    builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.dummy", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
    builderDescriptor.m_busId = AZ::Uuid::CreateRandom();
    builderDescriptor.m_createJobFunction = [&](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        AssetBuilderSDK::JobDescriptor jobDescriptor;
        jobDescriptor.m_jobKey = builderDescriptor.m_name;
        jobDescriptor.SetPlatformIdentifier("pc");
        if (AzFramework::StringFunc::EndsWith(request.m_sourceFile.c_str(), relSourceFileName))
        {
            AssetBuilderSDK::SourceFileDependency dep = { secondRelSourceFile , AZ::Uuid::CreateNull() };
            AssetBuilderSDK::JobDependency jobDep(builderDescriptor.m_name, "pc", AssetBuilderSDK::JobDependencyType::OrderOnce, dep);
            jobDescriptor.m_jobDependencyList.emplace_back(jobDep);
        }
        response.m_createJobOutputs.emplace_back(jobDescriptor);
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    };
    builderDescriptor.m_processJobFunction = [](const AssetBuilderSDK::ProcessJobRequest& /*request*/, AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success;
    };

    MockApplicationManager::BuilderFilePatternMatcherAndBuilderDesc builderFilePatternMatcher;
    builderFilePatternMatcher.m_builderDesc = builderDescriptor;
    builderFilePatternMatcher.m_internalBuilderName = builderDescriptor.m_name;
    builderFilePatternMatcher.m_internalUuid = builderDescriptor.m_busId;
    builderFilePatternMatcher.m_matcherBuilderPattern = AssetUtilities::BuilderFilePatternMatcher(builderDescriptor.m_patterns.back(), builderDescriptor.m_busId);
    m_mockApplicationManager->m_matcherBuilderPatterns.emplace_back(builderFilePatternMatcher);

    // Capture the job details as the APM inspects the file.
    AZStd::vector<JobDetails> jobDetails;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&jobDetails](JobDetails job)
        {
            jobDetails.emplace_back(job);
        });

    // Tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileName));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, secondSourceFile));
    ASSERT_TRUE(BlockUntilIdle(5000));

    // Although we have processed a.dummy first, APM should send us notification of b.dummy job first and than of a.dummy job
    EXPECT_EQ(jobDetails.size(), 2);
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_sourceAssetReference.AbsolutePath().c_str(), secondSourceFile);
    EXPECT_EQ(jobDetails[1].m_jobEntry.m_sourceAssetReference.AbsolutePath().c_str(), sourceFileName);
    EXPECT_EQ(jobDetails[1].m_jobDependencyList.size(), 1); // there should only be one job dependency
    EXPECT_EQ(jobDetails[1].m_jobDependencyList[0].m_jobDependency.m_sourceFile.m_sourceFileDependencyPath, secondSourceFile.toUtf8().constData()); // there should only be one job dependency

    // Process jobs in APM
    auto destination = jobDetails[0].m_cachePath;
    QString productAFileName = (destination / "aoutput.txt").AsPosix().c_str();
    QString productBFileName = (destination / "boutput.txt").AsPosix().c_str();
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(productBFileName, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(productAFileName, QString("tempdata\n")));

    AssetBuilderSDK::ProcessJobResponse responseB;
    responseB.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    responseB.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("boutput.txt", AZ::Uuid::CreateNull(), 1));

    AssetBuilderSDK::ProcessJobResponse responseA;
    responseA.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    responseA.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("aoutput.txt", AZ::Uuid::CreateNull(), 1));

    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetails[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, responseB));
    ASSERT_TRUE(BlockUntilIdle(5000));

    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetails[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, responseA));
    ASSERT_TRUE(BlockUntilIdle(5000));

    jobDetails.clear();
    m_isIdling = false;

    // Modify source file b.dummy, we should only see one job with source file b.dummy getting processed again even though a.dummy job has an order once job dependency on it .
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(secondSourceFile, QString("temp\n")));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, secondSourceFile));
    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(jobDetails.size(), 1);
    EXPECT_STREQ(jobDetails[0].m_jobEntry.m_sourceAssetReference.AbsolutePath().c_str(), secondSourceFile.toUtf8().constData());

    jobDetails.clear();
    m_isIdling = false;
    // Modify source file a.dummy, we should only see one job with source file a.dummy getting processed in this case.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(sourceFileName, QString("temp\n")));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileName));
    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(jobDetails.size(), 1);
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_sourceAssetReference.AbsolutePath().c_str(), sourceFileName);
    EXPECT_EQ(jobDetails[0].m_jobDependencyList.size(), 0); // there should not be any job dependency since APM has already processed b.dummy before

    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetails[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, responseA));
    ASSERT_TRUE(BlockUntilIdle(5000));

    jobDetails.clear();
    m_isIdling = false;
    // Here first fail the b.dummy job and than tell APM about the modified file
    // This should cause a.dummy job to get emitted again
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(secondSourceFile, QString("tempData\n")));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, secondSourceFile));
    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(jobDetails.size(), 1);
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_sourceAssetReference.AbsolutePath().c_str(), secondSourceFile);

    responseB.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
    m_isIdling = false;

    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetFailed", Qt::QueuedConnection, Q_ARG(JobEntry, jobDetails[0].m_jobEntry));
    ASSERT_TRUE(BlockUntilIdle(5000));

    jobDetails.clear();
    m_isIdling = false;

    // Modify source file b.dummy
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(secondSourceFile, QString("temp\n")));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, secondSourceFile));
    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(jobDetails.size(), 2);
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_sourceAssetReference.AbsolutePath().c_str(), secondSourceFile);
    EXPECT_EQ(jobDetails[1].m_jobEntry.m_sourceAssetReference.AbsolutePath().c_str(), sourceFileName);
    EXPECT_EQ(jobDetails[1].m_jobDependencyList.size(), 1); // there should only be one job dependency
    EXPECT_STREQ(jobDetails[1].m_jobDependencyList[0].m_jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str(), secondSourceFile.toUtf8().constData()); // there should only be one job dependency
}

TEST_F(AssetProcessorManagerTest, SourceFile_With_NonASCII_Characters_Fail_Job_OK)
{
    // This test ensures that asset processor manager detects a source file that has non-ASCII characters
    // and sends a notification for a dummy autofail job.
    // This test also ensure that when we get a folder delete notification, it forwards the relative folder path to the GUI model for removal of jobs.

    QString deletedFolderPath;
    QObject::connect(m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::SourceFolderDeleted,
        [&deletedFolderPath](QString folderPath)
    {
        deletedFolderPath = folderPath;
    });

    JobDetails failedjobDetails;
    QObject::connect(m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetToProcess,
        [&failedjobDetails](JobDetails jobDetails)
    {
        failedjobDetails = jobDetails;
    });

    QString watchFolderPath = m_assetRootDir.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    QString folderPath(m_assetRootDir.absoluteFilePath("subfolder1/Test\xD0"));
    QDir folderPathDir(folderPath);
    QString absPath(folderPathDir.absoluteFilePath("Test.txt"));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(absPath, QString("test\n")));

    m_assetProcessorManager.get()->AssessAddedFile(absPath);

    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(failedjobDetails.m_autoFail, true);
    EXPECT_EQ(failedjobDetails.m_jobEntry.GetAbsoluteSourcePath(), absPath);

    // folder delete notification
    folderPathDir.removeRecursively();
    m_assetProcessorManager.get()->AssessDeletedFile(folderPath);
    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(deletedFolderPath, folderPath);
}

TEST_F(AssetProcessorManagerTest, SourceFileProcessFailure_ClearsFingerprint)
{
    constexpr int idleWaitTime = 5000;
    using namespace AzToolsFramework::AssetDatabase;

    QList<AssetProcessor::JobDetails> processResults;

    auto assetConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&processResults](JobDetails details)
    {
        processResults.push_back(AZStd::move(details));
    });

    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(m_assetRootDir.absoluteFilePath("subfolder1"));
    ASSERT_NE(scanFolder, nullptr);

    QString absPath = m_assetRootDir.absoluteFilePath("subfolder1/test.txt");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(absPath, QString("test\n")));

    //////////////////////////////////////////////////////////////////////////

    // Add a file and signal a successful process event

    m_assetProcessorManager.get()->AssessAddedFile(absPath);
    ASSERT_TRUE(BlockUntilIdle(idleWaitTime));

    for(const auto& processResult : processResults)
    {
        AZStd::string file = (processResult.m_jobEntry.m_sourceAssetReference.RelativePath().Native() + ".arc1");

        // Create the file on disk
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile((processResult.m_cachePath / file).AsPosix().c_str(), "products."));

        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(file, AZ::Uuid::CreateNull(), 1));

        m_assetProcessorManager->AssetProcessed(processResult.m_jobEntry, response);
    }

    ASSERT_TRUE(BlockUntilIdle(idleWaitTime));

    bool found = false;
    SourceDatabaseEntry source;

    auto queryFunc = [&](SourceDatabaseEntry& sourceData)
    {
        source = AZStd::move(sourceData);
        found = true;
        return false; // stop iterating after the first one.  There should actually only be one entry anyway.
    };

    m_assetProcessorManager->m_stateData->QuerySourceBySourceNameScanFolderID("test.txt", scanFolder->ScanFolderID(), queryFunc);

    ASSERT_TRUE(found);
    ASSERT_NE(source.m_analysisFingerprint, "");

    // Modify the file and run it through AP again, but this time signal a failure

    {
        QFile writer(absPath);
        ASSERT_TRUE(writer.open(QFile::WriteOnly));

        QTextStream ts(&writer);
        ts.setCodec("UTF-8");
        ts << "Hello World";
    }

    processResults.clear();
    m_assetProcessorManager.get()->AssessModifiedFile(absPath);
    ASSERT_TRUE(BlockUntilIdle(idleWaitTime));

    for (const auto& processResult : processResults)
    {
        m_assetProcessorManager->AssetFailed(processResult.m_jobEntry);
    }

    ASSERT_TRUE(BlockUntilIdle(idleWaitTime));

    // Check the database, the fingerprint should be erased since the file failed
    found = false;
    m_assetProcessorManager->m_stateData->QuerySourceBySourceNameScanFolderID("test.txt", scanFolder->ScanFolderID(), queryFunc);

    ASSERT_TRUE(found);
    ASSERT_EQ(source.m_analysisFingerprint, "");
}

TEST_F(AssetProcessorManagerTest, SourceFileProcessFailure_ValidLfsPointerFile_ReceiveLFSPointerFileError)
{
    // Override the project and engine root directories in the setting registry to create a custom .gitattributes file for testing.
    auto settingsRegistry = AZ::SettingsRegistry::Get();
    ASSERT_TRUE(settingsRegistry);
    AZ::IO::FixedMaxPathString engineRoot, projectRoot;
    settingsRegistry->Get(engineRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    settingsRegistry->Get(projectRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, m_assetRootDir.path().toUtf8().data());
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, m_assetRootDir.path().toUtf8().data());

    QString gitAttributesPath = m_assetRootDir.absoluteFilePath(".gitattributes");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(gitAttributesPath, QString(
        "#\n"
        "# Git LFS(see https ://git-lfs.github.com/)\n"
        "#\n"
        "*.txt filter=lfs diff=lfs merge=lfs -text\n")));

    QString sourcePath = m_assetRootDir.absoluteFilePath("subfolder1/test.txt");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(sourcePath, QString(
        "version https://git-lfs.github.com/spec/v1\n"
        "oid sha256:ee4799379bfcfa99e95afd6494da51fbeda95f21ea71d267ae7102f048edec85\n"
        "size 63872\n")));

    constexpr int idleWaitTime = 5000;
    using namespace AzToolsFramework::AssetDatabase;

    QList<AssetProcessor::JobDetails> processResults;
    auto assetConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&processResults](JobDetails details)
        {
            processResults.push_back(AZStd::move(details));
        });

    // Add the test file and signal a failed event
    m_assetProcessorManager.get()->AssessAddedFile(sourcePath);
    ASSERT_TRUE(BlockUntilIdle(idleWaitTime));

    for(const auto& processResult : processResults)
    {
        m_assetProcessorManager->AssetFailed(processResult.m_jobEntry);
    }

    ASSERT_TRUE(BlockUntilIdle(idleWaitTime));

    // An error message should be thrown for the valid LFS pointer file.
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 1);

    // Revert the project and engine root directories in the setting registry.
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, engineRoot);
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, projectRoot);
}

TEST_F(AssetProcessorManagerTest, SourceFileProcessFailure_AutoFailedLfsPointerFile_ReceiveLFSPointerFileError)
{
    // Override the project and engine root directories in the setting registry to create a custom .gitattributes file for testing.
    auto settingsRegistry = AZ::SettingsRegistry::Get();
    ASSERT_TRUE(settingsRegistry);
    AZ::IO::FixedMaxPathString engineRoot, projectRoot;
    settingsRegistry->Get(engineRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    settingsRegistry->Get(projectRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, m_assetRootDir.path().toUtf8().data());
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, m_assetRootDir.path().toUtf8().data());

    QDir assetRootDir(m_assetRootDir.path());
    QString gitAttributesPath = assetRootDir.absoluteFilePath(".gitattributes");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(gitAttributesPath, QString(
        "#\n"
        "# Git LFS(see https ://git-lfs.github.com/)\n"
        "#\n"
        "*.txt filter=lfs diff=lfs merge=lfs -text\n")));

    QString sourcePath = assetRootDir.absoluteFilePath("subfolder1/test.txt");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(sourcePath, QString(
        "version https://git-lfs.github.com/spec/v1\n"
        "oid sha256:ee4799379bfcfa99e95afd6494da51fbeda95f21ea71d267ae7102f048edec85\n"
        "size 63872\n")));

    constexpr int idleWaitTime = 5000;
    using namespace AzToolsFramework::AssetDatabase;

    QList<AssetProcessor::JobDetails> processResults;
    auto assetConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&processResults](JobDetails details)
        {
            details.m_jobEntry.m_addToDatabase = false;
            processResults.push_back(AZStd::move(details));
        });

    // Add the test file and signal a failed event
    m_assetProcessorManager.get()->AssessAddedFile(sourcePath);
    ASSERT_TRUE(BlockUntilIdle(idleWaitTime));

    for(const auto& processResult : processResults)
    {
        m_assetProcessorManager->AssetFailed(processResult.m_jobEntry);
    }

    // An error message should be thrown for the valid LFS pointer file.
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 1);

    // Revert the project and engine root directories in the setting registry.
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, engineRoot);
    settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, projectRoot);
}

//////////////////////////////////////////////////////////////////////////

void FingerprintTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();

    // We don't want the mock application manager to provide builder descriptors, mockBuilderInfoHandler will provide our own
    m_mockApplicationManager->BusDisconnect();

    // Create the test file
    const auto& scanFolder = m_config->GetScanFolderAt(1);
    QString relativePathFromWatchFolder("fingerprintTest.txt");
    m_absolutePath = QDir(scanFolder.ScanPath()).absoluteFilePath(relativePathFromWatchFolder);

    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [this](JobDetails jobDetails)
    {
        m_jobResults.push_back(jobDetails);
    });

    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(m_absolutePath, ""));
}

void FingerprintTest::TearDown()
{
    m_jobResults = AZStd::vector<AssetProcessor::JobDetails>{};
    m_mockBuilderInfoHandler = {};

    AssetProcessorManagerTest::TearDown();
}

void FingerprintTest::RunFingerprintTest(QString builderFingerprint, QString jobFingerprint, bool expectedResult)
{
    m_mockBuilderInfoHandler.CreateBuilderDesc(
        "test builder", "{DF09DDC0-FD22-43B6-9E22-22C8574A6E1E}",
        { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) },
        UnitTests::MockMultiBuilderInfoHandler::AssetBuilderExtraInfo{ jobFingerprint, "", "", builderFingerprint, {} });
    m_mockBuilderInfoHandler.BusConnect();

    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, m_absolutePath));

    ASSERT_TRUE(BlockUntilIdle(5000));
    ASSERT_EQ(m_mockBuilderInfoHandler.m_createJobsCount, 1);
    ASSERT_EQ(m_jobResults.size(), 1);
    ASSERT_EQ(m_jobResults[0].m_autoFail, expectedResult);
}

TEST_F(FingerprintTest, FingerprintChecking_JobFingerprint_NoBuilderFingerprint)
{
    RunFingerprintTest("", "Hello World", true);
}

TEST_F(FingerprintTest, FingerprintChecking_NoJobFingerprint_NoBuilderFingerprint)
{
    RunFingerprintTest("", "", false);
}

TEST_F(FingerprintTest, FingerprintChecking_JobFingerprint_BuilderFingerprint)
{
    RunFingerprintTest("Hello", "World", false);
}

TEST_F(FingerprintTest, FingerprintChecking_NoJobFingerprint_BuilderFingerprint)
{
    RunFingerprintTest("Hello World", "", false);
}

TEST_F(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_WildcardMissingFiles_ByName_UpdatesWhenTheyAppear)
{
    // This test checks that wildcard source dependencies are added to the database as "SourceLikeMatch",
    // find existing files which match the dependency and add them as either job or source file dependencies,
    // And recognize matching files as dependencies

    AZ::Uuid dummyBuilderUUID = AZ::Uuid::CreateRandom();
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder1/wildcardTest.txt"));
    QString relFileName("wildcardTest.txt");
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/wildcardTest.txt"));
    QString watchFolderPath = m_assetRootDir.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    QString dependsOnFilea_Source = m_assetRootDir.absoluteFilePath("subfolder1/a.txt");
    QString dependsOnFileb_Source = m_assetRootDir.absoluteFilePath("subfolder1/b.txt");
    QString dependsOnFileb1_Source = m_assetRootDir.absoluteFilePath("subfolder1/b1.txt");
    QString dependsOnFilec_Job = m_assetRootDir.absoluteFilePath("subfolder1/c.txt");
    QString dependsOnFilec1_Job = m_assetRootDir.absoluteFilePath("subfolder1/c1.txt");
    QString dependsOnFiled_Job = m_assetRootDir.absoluteFilePath("subfolder1/d.txt");

    // in this case, we are only creating file b, and d, which are addressed by UUID.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFileb_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFilec_Job, QString("tempdata\n")));

    // construct the dummy job to feed to the database updater function:
    AZ::Uuid wildcardTestUuid = AssetUtilities::CreateSafeSourceUUIDFromName("wildcardTest.txt");
    AssetProcessorManager::JobToProcessEntry job;
    job.m_sourceFileInfo.m_sourceAssetReference = AssetProcessor::SourceAssetReference(absPath);
    job.m_sourceFileInfo.m_scanFolder = scanFolder;
    job.m_sourceFileInfo.m_uuid = wildcardTestUuid;

    // each file we will take a different approach to publishing:  rel path, and UUID:
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "b*.txt", AZ::Uuid::CreateNull(), AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards }));

    // it is currently assumed that the only fields that we care about in JobDetails is the builder busId and the job dependencies themselves:
    JobDetails newDetails;
    newDetails.m_assetBuilderDesc.m_busId = dummyBuilderUUID;

    AssetBuilderSDK::SourceFileDependency dep1 = { "c*.txt", AZ::Uuid::CreateNull(), AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards };
    AssetBuilderSDK::JobDependency jobDep1("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep1);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep1));

    job.m_jobsToAnalyze.push_back(newDetails);

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);

    AzToolsFramework::AssetDatabase::SourceDatabaseEntry wildcard(scanFolder->ScanFolderID(), "wildcardTest.txt", wildcardTestUuid, "fingerprint");
    m_assetProcessorManager->m_stateData->SetSource(wildcard);

    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(wildcardTestUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(dependsOnFileb_Source.toUtf8().constData()), deps.end());
    deps.clear();

    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(wildcardTestUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(dependsOnFilec_Job.toUtf8().constData()), deps.end());
    deps.clear();

    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(wildcardTestUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceOrJob);
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(dependsOnFilec_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFileb_Source.toUtf8().constData()), deps.end());
    deps.clear();

    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(wildcardTestUuid, deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch);
    EXPECT_EQ(deps.size(), 1);
    deps.clear();

    AZStd::vector<AZStd::string> wildcardDeps;
    auto callbackFunction = [&wildcardDeps](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry)
    {
        wildcardDeps.push_back(entry.m_dependsOnSource.ToString());
        return true;
    };

    m_assetProcessorManager.get()->m_stateData->QueryDependsOnSourceBySourceDependency(wildcardTestUuid, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, callbackFunction);
    EXPECT_EQ(wildcardDeps.size(), 2);

    // The database should have the wildcard record and the individual dependency on b and c at this point, now we add new files
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFileb1_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFilec1_Job, QString("tempdata\n")));

    QStringList dependList;
    dependList = m_assetProcessorManager.get()->GetSourceFilesWhichDependOnSourceFile(dependsOnFileb1_Source, {});
    EXPECT_EQ(dependList.size(), 1);
    EXPECT_EQ(dependList[0], absPath.toUtf8().constData());
    dependList.clear();

    dependList = m_assetProcessorManager.get()->GetSourceFilesWhichDependOnSourceFile(dependsOnFilec1_Job, {});
    EXPECT_EQ(dependList.size(), 1);
    EXPECT_EQ(dependList[0], absPath.toUtf8().constData());
    dependList.clear();

    dependList = m_assetProcessorManager.get()->GetSourceFilesWhichDependOnSourceFile(dependsOnFilea_Source, {});
    EXPECT_EQ(dependList.size(), 0);
    dependList.clear();

    dependList = m_assetProcessorManager.get()->GetSourceFilesWhichDependOnSourceFile(dependsOnFiled_Job, {});
    EXPECT_EQ(dependList.size(), 0);

    dependList.clear();
}

TEST_F(AssetProcessorManagerTest, RemoveSource_RemoveCacheFolderIfEmpty_Ok)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QStringList sourceFiles;
    QStringList productFiles;

    // Capture the job details as the APM inspects the file.
    JobDetails jobDetails;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&jobDetails](JobDetails job)
        {
            jobDetails = job;
        });

    static constexpr int NumOfSourceFiles = 2;

    for (int idx = 0; idx < NumOfSourceFiles; idx++)
    {
        sourceFiles.append(m_assetRootDir.absoluteFilePath("subfolder1/subfolder2/source_test%1.txt").arg(idx));
        UnitTestUtils::CreateDummyFile(sourceFiles[idx], "source");
        // Tell the APM about the file:
        m_isIdling = false;
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFiles[idx]));
        ASSERT_TRUE(BlockUntilIdle(5000));

        auto filename = AZStd::string::format("product_test%d.txt", idx);
        productFiles.append((jobDetails.m_cachePath / filename).AsPosix().c_str());
        UnitTestUtils::CreateDummyFile(productFiles.back(), "product");

        // Populate ProcessJobResponse
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        JobProduct product((jobDetails.m_relativePath / filename).StringAsPosix(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(idx));
        response.m_outputProducts.push_back(product);

        // Process the job
        m_isIdling = false;
        m_assetProcessorManager->AssetProcessed(jobDetails.m_jobEntry, response);
        ASSERT_TRUE(BlockUntilIdle(5000));
    }

    QObject::disconnect(connection);

    // ----------------------------- TEST BEGINS HERE -----------------------------
    // We have two source files that create products in the same cache directory.
    // Deleting the first source file should only remove products associated with it
    // Deleting the second source should remove the cache directory along with all products associated with it.

    int firstSourceIdx = 0;
    AZ::IO::SystemFile::Delete(sourceFiles[firstSourceIdx].toUtf8().data());
    m_isIdling = false;
    // Simulate the file watcher notifying a file delete:
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFiles[firstSourceIdx]));
    ASSERT_TRUE(BlockUntilIdle(5000));

    // Ensure that products no longer exists on disk
    ASSERT_FALSE(QFile::exists(productFiles[firstSourceIdx]));

    // Ensure that cache directory exists
    QDir cacheDirectory(jobDetails.m_cachePath.AsPosix().c_str());

    ASSERT_TRUE(cacheDirectory.exists());


    int secondSourceIdx = 1;
    AZ::IO::SystemFile::Delete(sourceFiles[secondSourceIdx].toUtf8().data());
    m_isIdling = false;
    // Simulate the file watcher notifying a file delete:
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFiles[secondSourceIdx]));
    ASSERT_TRUE(BlockUntilIdle(5000));

    // Ensure that products no longer exists on disk
    ASSERT_FALSE(QFile::exists(productFiles[secondSourceIdx]));

    // Ensure that cache directory is removed this time
    ASSERT_FALSE(cacheDirectory.exists());

}

void DuplicateProductsTest::SetupDuplicateProductsTest(QString& sourceFile, QDir& tempPath, QString& productFile, AZStd::vector<AssetProcessor::JobDetails>& jobDetails, AssetBuilderSDK::ProcessJobResponse& response, bool multipleOutputs, QString extension)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // Capture the job details as the APM inspects the file.
    QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&jobDetails](JobDetails job)
        {
            jobDetails.emplace_back(job);
        });

    AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
    builderDescriptor.m_name = "Test Txt Builder";
    builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern(QString("*.%1").arg(extension).toUtf8().constData(), AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
    builderDescriptor.m_busId = AZ::Uuid::CreateRandom();
    builderDescriptor.m_createJobFunction = [&](const AssetBuilderSDK::CreateJobsRequest& /*request*/, AssetBuilderSDK::CreateJobsResponse& response)
    {
        AssetBuilderSDK::JobDescriptor jobDescriptor;
        jobDescriptor.m_jobKey = builderDescriptor.m_name;
        jobDescriptor.SetPlatformIdentifier("pc");
        response.m_createJobOutputs.emplace_back(jobDescriptor);
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        if(multipleOutputs)
        {
            jobDescriptor.m_jobKey = "Duplicate Output";
            response.m_createJobOutputs.emplace_back(jobDescriptor);
        }
    };
    builderDescriptor.m_processJobFunction = [](const AssetBuilderSDK::ProcessJobRequest& /*request*/, AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success;
    };

    MockApplicationManager::BuilderFilePatternMatcherAndBuilderDesc builderFilePatternMatcher;
    builderFilePatternMatcher.m_builderDesc = builderDescriptor;
    builderFilePatternMatcher.m_internalBuilderName = builderDescriptor.m_name;
    builderFilePatternMatcher.m_internalUuid = builderDescriptor.m_busId;
    builderFilePatternMatcher.m_matcherBuilderPattern = AssetUtilities::BuilderFilePatternMatcher(builderDescriptor.m_patterns.back(), builderDescriptor.m_busId);
    m_mockApplicationManager->m_matcherBuilderPatterns.emplace_back(builderFilePatternMatcher);

    sourceFile = tempPath.absoluteFilePath("subfolder1/subfolder2/source_test." + extension);
    UnitTestUtils::CreateDummyFile(sourceFile, "source");
    // Tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));
    ASSERT_TRUE(BlockUntilIdle(5000));

    auto filename = "product_test." + extension;
    productFile.append((jobDetails[0].m_cachePath / filename.toUtf8().constData()).AsPosix().c_str());
    UnitTestUtils::CreateDummyFile(productFile, "product");

    // Populate ProcessJobResponse
    response.m_resultCode = ProcessJobResult_Success;
    JobProduct jobProduct(filename.toUtf8().constData(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(0));
    response.m_outputProducts.push_back(jobProduct);

    // Process the first job
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(jobDetails[0].m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));
}

TEST_F(DuplicateProductsTest, SameSource_MultipleBuilder_DuplicateProductJobs_EmitAutoFailJob)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QString productFile;
    QString sourceFile;
    AZStd::vector<JobDetails> jobDetails;

    ProcessJobResponse response;
    SetupDuplicateProductsTest(sourceFile, m_assetRootDir, productFile, jobDetails, response, false, "txt");

    // ----------------------------- TEST BEGINS HERE -----------------------------
    // We will process another job with the same source file outputting the same product
    JobDetails jobDetail = jobDetails[1];
    jobDetails.clear();
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(jobDetail.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));

    EXPECT_EQ(jobDetails.size(), 1);
    EXPECT_TRUE(jobDetails.back().m_jobParam.find(AZ_CRC(AutoFailReasonKey)) != jobDetails.back().m_jobParam.end());
}

TEST_F(DuplicateProductsTest, SameSource_SameBuilder_DuplicateProductJobs_EmitAutoFailJob)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QString productFile;
    QString sourceFile;
    AZStd::vector<JobDetails> jobDetails;

    ProcessJobResponse response;
    SetupDuplicateProductsTest(sourceFile, m_assetRootDir, productFile, jobDetails, response, true, "png");

    // ----------------------------- TEST BEGINS HERE -----------------------------
    // We will process another job with the same source file outputting the same product
    JobDetails jobDetail = jobDetails[1];
    jobDetails.clear();
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(jobDetail.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));

    EXPECT_EQ(jobDetails.size(), 1);
    EXPECT_TRUE(jobDetails.back().m_jobParam.find(AZ_CRC(AutoFailReasonKey)) != jobDetails.back().m_jobParam.end());
}

TEST_F(DuplicateProductsTest, SameSource_MultipleBuilder_NoDuplicateProductJob_NoWarning)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QString sourceFile;
    QString productFile;

    // Capture the job details as the APM inspects the file.
    AZStd::vector<JobDetails> jobDetails;
    ProcessJobResponse response;
    SetupDuplicateProductsTest(sourceFile, m_assetRootDir, productFile, jobDetails, response, false, "txt");

    // ----------------------------- TEST BEGINS HERE -----------------------------
    // We will process another job with the same source file outputting a different product file

    auto filename = "product_test1.txt";
    productFile = (jobDetails[0].m_cachePath / filename).AsPosix().c_str();
    UnitTestUtils::CreateDummyFile(productFile, "product");

    JobProduct newJobProduct((jobDetails[0].m_relativePath / filename).c_str(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(0));
    response.m_outputProducts.clear();
    response.m_outputProducts.push_back(newJobProduct);

    JobDetails jobDetail = jobDetails[1];
    jobDetails.clear();
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(jobDetail.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));

    EXPECT_EQ(jobDetails.size(), 0);
}

void JobDependencyTest::SetUp()
{
    using namespace AzToolsFramework::AssetDatabase;

    AssetProcessorManagerTest::SetUp();

    m_data = AZStd::make_unique<StaticData>();
    m_data->m_builderUuid = AZ::Uuid("{DE55BCCF-4D40-40FA-AB46-86C2946FBA54}");

    // We don't want the mock application manager to provide builder descriptors, mockBuilderInfoHandler will provide our own
    m_mockApplicationManager->BusDisconnect();

    m_data->m_mockBuilderInfoHandler.CreateBuilderDescInfoRef("test builder", m_data->m_builderUuid.ToString<QString>(), { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) }, m_data->m_assetBuilderConfig);
    m_data->m_mockBuilderInfoHandler.BusConnect();

    QString watchFolderPath = m_assetRootDir.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);

    // Create a dummy file and put entries in the db to simulate a previous successful AP run for this file (source, job, and product entries)
    QString absPath(QDir(watchFolderPath).absoluteFilePath("a.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    SourceDatabaseEntry sourceEntry(scanFolder->ScanFolderID(), "a.txt", AZ::Uuid::CreateRandom(), "abcdefg");
    m_assetProcessorManager->m_stateData->SetSource(sourceEntry);

    JobDatabaseEntry jobEntry(sourceEntry.m_sourceID, "Mock Job", 123456, "pc", m_data->m_builderUuid, AzToolsFramework::AssetSystem::JobStatus::Completed, 1);
    m_assetProcessorManager->m_stateData->SetJob(jobEntry);

    ProductDatabaseEntry productEntry(jobEntry.m_jobID, 0, "a.output", AZ::Data::AssetType::CreateNull());
    m_assetProcessorManager->m_stateData->SetProduct(productEntry);

    // Reboot the APM since we added stuff to the database that needs to be loaded on-startup of the APM
    m_assetProcessorManager = nullptr; // Destroy the existing instance first so we can finish cleanup before creating a new instance
    m_assetProcessorManager.reset(new AssetProcessorManager_Test(m_config.get()));

    m_idleConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState, [this](bool newState)
    {
        m_isIdling = newState;
    });
}

void JobDependencyTest::TearDown()
{
    m_data = nullptr;

    AssetProcessorManagerTest::TearDown();
}

TEST_F(JobDependencyTest, JobDependency_ThatWasPreviouslyRun_IsFound)
{
    AZStd::vector<JobDetails> capturedDetails;

    capturedDetails.clear();
    m_data->m_assetBuilderConfig.m_jobDependencyFilePath = "a.txt";
    CaptureJobs(capturedDetails, "subfolder1/b.txt");

    ASSERT_EQ(capturedDetails.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList[0].m_builderUuidList.size(), 1);
}

TEST_F(JobDependencyTest, JobDependency_ThatWasJustRun_IsFound)
{
    AZStd::vector<JobDetails> capturedDetails;
    CaptureJobs(capturedDetails, "subfolder1/c.txt");

    capturedDetails.clear();
    m_data->m_assetBuilderConfig.m_jobDependencyFilePath = "c.txt";
    CaptureJobs(capturedDetails, "subfolder1/b.txt");

    ASSERT_EQ(capturedDetails.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList[0].m_builderUuidList.size(), 1);
}

TEST_F(JobDependencyTest, JobDependency_ThatHasNotRun_IsNotFound)
{
    AZStd::vector<JobDetails> capturedDetails;

    capturedDetails.clear();
    m_data->m_assetBuilderConfig.m_jobDependencyFilePath = "c.txt";
    CaptureJobs(capturedDetails, "subfolder1/b.txt");

    ASSERT_EQ(capturedDetails.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList[0].m_builderUuidList.size(), 0);
}

void ChainJobDependencyTest::SetUp()
{
    using namespace AzToolsFramework::AssetDatabase;

    AssetProcessorManagerTest::SetUp();

    m_data = AZStd::make_unique<StaticData>();

    m_data->m_rcController.reset(new RCController(/*minJobs*/1, /*maxJobs*/1));
    m_data->m_rcController->SetDispatchPaused(false);

    // We don't want the mock application manager to provide builder descriptors, mockBuilderInfoHandler will provide our own
    m_mockApplicationManager->BusDisconnect();

    for (int i = 0; i < ChainLength; ++i)
    {
        QString jobDependencyPath;

        if (i > 0)
        {
            jobDependencyPath = QString("%1.txt").arg(i - 1);
        }

        m_data->m_mockBuilderInfoHandler.CreateBuilderDesc(QString("test builder %1").arg(i), AZ::Uuid::CreateRandom().ToString<QString>(), { AssetBuilderSDK::AssetBuilderPattern(AZStd::string::format("*%d.txt", i), AssetBuilderSDK::AssetBuilderPattern::Wildcard) },
            UnitTests::MockMultiBuilderInfoHandler::AssetBuilderExtraInfo{ "", "", jobDependencyPath, "", {} });
    }

    m_data->m_mockBuilderInfoHandler.BusConnect();
}

void ChainJobDependencyTest::TearDown()
{
    m_data = nullptr;

    AssetProcessorManagerTest::TearDown();
}

TEST_F(ChainJobDependencyTest, ChainDependency_EndCaseHasNoDependency)
{
    AZStd::vector<JobDetails> capturedDetails;

    CaptureJobs(capturedDetails, AZStd::string::format("subfolder1/%d.txt", 0).c_str());

    ASSERT_EQ(capturedDetails.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList.size(), 0);
}

TEST_F(ChainJobDependencyTest, TestChainDependency_Multi)
{
    AZStd::vector<JobDetails> capturedDetails;

    // Run through the dependencies in forward order so everything gets added to the database
    for (int i = 0; i < ChainLength; ++i)
    {
        CaptureJobs(capturedDetails, AZStd::string::format("subfolder1/%d.txt", i).c_str());

        ASSERT_EQ(capturedDetails.size(), 1);
        ASSERT_EQ(capturedDetails[0].m_jobDependencyList.size(), i > 0 ? 1 : 0);

        capturedDetails.clear();
    }

    QDir tempPath(m_assetRootDir.path());

    // Run through the dependencies in reverse order
    // Each one should trigger a job for every file in front of it
    // Ex: 3 triggers -> 2 -> 1 -> 0
    for (int i = ChainLength - 1; i >= 0; --i)
    {
        CaptureJobs(capturedDetails, AZStd::string::format("subfolder1/%d.txt", i).c_str());

        ASSERT_EQ(capturedDetails.size(), ChainLength - i);
        ASSERT_EQ(capturedDetails[0].m_jobDependencyList.size(), i > 0 ? 1 : 0);

        if (i > 0)
        {
            QString absPath(tempPath.absoluteFilePath(AZStd::string::format("subfolder1/%d.txt", i - 1).c_str()));
            ASSERT_EQ(capturedDetails[0].m_jobDependencyList[0].m_jobDependency.m_sourceFile.m_sourceFileDependencyPath, absPath.toUtf8().constData());

            capturedDetails.clear();
        }
    }

    // Wait for the file compiled event and trigger OnAddedToCatalog with a delay, this is what causes rccontroller to process out of order
    AZStd::vector<JobEntry> finishedJobs;
    QObject::connect(m_data->m_rcController.get(), &RCController::FileCompiled, [this, &finishedJobs](JobEntry entry, AssetBuilderSDK::ProcessJobResponse response)
        {
            finishedJobs.push_back(entry);

            QTimer::singleShot(20, m_data->m_rcController.get(), [this, entry]()
                {
                    QMetaObject::invokeMethod(m_data->m_rcController.get(), "OnAddedToCatalog", Qt::QueuedConnection, Q_ARG(JobEntry, entry));
                });
        });

    // Submit all the jobs to rccontroller
    for (const JobDetails& job : capturedDetails)
    {
        m_data->m_rcController->JobSubmitted(job);
    }

    QElapsedTimer timer;
    timer.start();

    // Wait for all the jobs to finish, up to 5 seconds
    do
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    } while (finishedJobs.size() < capturedDetails.size() && timer.elapsed() < 5000);

    ASSERT_EQ(finishedJobs.size(), capturedDetails.size());

    // Test that the jobs completed in the correct order (captureDetails has the correct ordering)
    for(int i = 0; i < capturedDetails.size(); ++i)
    {
        ASSERT_EQ(capturedDetails[i].m_jobEntry.m_sourceAssetReference, finishedJobs[i].m_sourceAssetReference);
    }
}



void DuplicateProcessTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();

    m_sharedConnection = m_assetProcessorManager->m_stateData.get();
    ASSERT_TRUE(m_sharedConnection);
}

void MetadataFileTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();
    m_config->AddMetaDataType("foo", "txt");
}

TEST_F(MetadataFileTest, MetadataFile_SourceFileExtensionDifferentCase)
{

    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QString relFileName("Dummy.TXT");
    QString absPath(m_assetRootDir.absoluteFilePath("subfolder1/Dummy.TXT"));
    QString watchFolder = m_assetRootDir.absoluteFilePath("subfolder1");
    UnitTestUtils::CreateDummyFile(absPath, "dummy");

    JobEntry entry;
    entry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(watchFolder,relFileName);
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    const char* filename = "outputfile.TXT";
    QString productPath(m_normalizedCacheRootDir.absoluteFilePath(filename));
    UnitTestUtils::CreateDummyFile(productPath);

    AssetBuilderSDK::ProcessJobResponse jobResponse;
    jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("outputfile.TXT"));

    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, entry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, jobResponse));

    ASSERT_TRUE(BlockUntilIdle(5000));

    // Creating a metadata file for the source assets
    // APM should process the source asset if a metadafile is detected
    // We are intentionally having a source file with a different file extension casing than the one specified in the metadata rule.
    QString metadataFile(m_assetRootDir.absoluteFilePath("subfolder1/Dummy.foo"));
    UnitTestUtils::CreateDummyFile(metadataFile, "dummy");

    // Capture the job details as the APM inspects the file.
    JobDetails jobDetails;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&jobDetails](JobDetails job)
        {
            jobDetails = job;
        });

    m_assetProcessorManager->AssessAddedFile(m_assetRootDir.absoluteFilePath(metadataFile));

    ASSERT_TRUE(BlockUntilIdle(5000));
    ASSERT_EQ(jobDetails.m_jobEntry.m_sourceAssetReference.AbsolutePath().c_str(), absPath);
}

AZStd::vector<AZStd::string> QStringListToVector(const QStringList& qstringList)
{
    AZStd::vector<AZStd::string> azVector;
    // Convert to a vector of AZStd::strings because GTest handles this type better when displaying errors
    for (const QString& resolvedPath : qstringList)
    {
        azVector.emplace_back(resolvedPath.toUtf8().constData());
    }

    return azVector;
}

bool WildcardSourceDependencyTest::Test(
    const AZStd::string& dependencyPath, AZStd::vector<AZStd::string>& resolvedPaths)
{
    [[maybe_unused]] QString resolvedName;
    QStringList stringlistPaths;
    AssetBuilderSDK::SourceFileDependency dependency(dependencyPath, AZ::Uuid::CreateNull(), AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards);
    bool result = m_assetProcessorManager->ResolveSourceFileDependencyPath(dependency, resolvedName, stringlistPaths);

    resolvedPaths = QStringListToVector(stringlistPaths);

    return result;
}

AZStd::vector<AZStd::string> WildcardSourceDependencyTest::FileAddedTest(const QString& path)
{
    auto result = m_assetProcessorManager->GetSourceFilesWhichDependOnSourceFile(path, {});

    return QStringListToVector(result);
}

void WildcardSourceDependencyTest::SetUp()
{
    using namespace AzToolsFramework::AssetDatabase;

    AssetProcessorManagerTest::SetUp();

    // Add a non-recursive scan folder.  Only files directly inside of this folder should be picked up, subfolders are ignored
    m_config->AddScanFolder(ScanFolderInfo(m_assetRootDir.filePath("no_recurse"), "no_recurse",
        "no_recurse", false, false, m_config->GetEnabledPlatforms(), 1));

    {
        ExcludeAssetRecognizer excludeFolder;
        excludeFolder.m_name = "Exclude ignored Folder";
        excludeFolder.m_patternMatcher =
            AssetBuilderSDK::FilePatternMatcher(R"REGEX(^(.*\/)?ignored(\/.*)?$)REGEX", AssetBuilderSDK::AssetBuilderPattern::Regex);
        m_config->AddExcludeRecognizer(excludeFolder);
    }

    {
        ExcludeAssetRecognizer excludeFile;
        excludeFile.m_name = "Exclude z.foo Files";
        excludeFile.m_patternMatcher =
            AssetBuilderSDK::FilePatternMatcher(R"REGEX(^(.*\/)?z\.foo$)REGEX", AssetBuilderSDK::AssetBuilderPattern::Regex);
        m_config->AddExcludeRecognizer(excludeFile);
    }

    CreateSourceAndFile("subfolder1/1a.foo");
    CreateSourceAndFile("subfolder1/1b.foo");
    CreateSourceAndFile("subfolder2/a.foo");
    CreateSourceAndFile("subfolder2/b.foo");
    CreateSourceAndFile("subfolder2/folder/one/c.foo");
    CreateSourceAndFile("subfolder2/folder/one/d.foo");

    // Add a file that is not in a scanfolder.  Should always be ignored
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("not/a/scanfolder/e.foo"));

    // Add a file in the non-recursive scanfolder.  Since its not directly in the scan folder, it should always be ignored
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("no_recurse/one/two/three/f.foo"));

    // Add a file to an ignored folder
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder2/folder/ignored/g.foo"));

    // Add an ignored file
    UnitTestUtils::CreateDummyFile(m_assetRootDir.absoluteFilePath("subfolder2/folder/one/z.foo"));

    // Add a file in the cache
    AZStd::string projectCacheRootValue;
    AZ::SettingsRegistry::Get()->Get(projectCacheRootValue, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);
    projectCacheRootValue = AssetUtilities::NormalizeFilePath(projectCacheRootValue.c_str()).toUtf8().constData();
    auto path = AZ::IO::Path(projectCacheRootValue) / "cache.foo";
    UnitTestUtils::CreateDummyFile(path.c_str());

    AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer dependencies;

    auto aUuid = AssetUtilities::CreateSafeSourceUUIDFromName("a.foo");
    auto bUuid = AssetUtilities::CreateSafeSourceUUIDFromName("b.foo");
    auto dUuid = AssetUtilities::CreateSafeSourceUUIDFromName("folder/one/d.foo");

    // Relative path wildcard dependency
    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), aUuid, PathOrUuid("%a.foo"),
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0, ""));

    // Absolute path wildcard dependency
    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), bUuid, PathOrUuid(m_assetRootDir.absoluteFilePath("%b.foo").toUtf8().constData()),
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0, ""));

    // Test what happens when we have 2 dependencies on the same file
    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), dUuid, PathOrUuid("%c.foo"),
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0, ""));

    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), dUuid, PathOrUuid(m_assetRootDir.absoluteFilePath("%c.foo").toUtf8().constData()),
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0, ""));

#ifdef AZ_PLATFORM_WINDOWS
    // Test to make sure a relative wildcard dependency doesn't match an absolute path
    // For example, if the input is C:/project/subfolder1/a.foo
    // This should not match a wildcard of c%.foo
    // Take the first character of the m_assetRootDir and append %.foo onto it for this test, which should produce something like c%.foo
    // This only applies to windows because on other OSes if the dependency starts with /, then its an abs path dependency
    auto test = (m_assetRootDir.absolutePath().left(1) + "%.foo");
    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), dUuid,
        PathOrUuid(test.toUtf8().constData()),
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0, ""));
#endif

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependencies(dependencies));
}

TEST_F(WildcardSourceDependencyTest, Relative_Broad)
{
    // Expect all files except for the 2 invalid ones (e and f)
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test("*.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre("a.foo", "b.foo", "folder/one/c.foo", "folder/one/d.foo", "1a.foo", "1b.foo"));
}

TEST_F(WildcardSourceDependencyTest, Relative_WithFolder)
{
    // Make sure we can filter to files under a folder
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test("folder/*.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre("folder/one/c.foo", "folder/one/d.foo"));
}

TEST_F(WildcardSourceDependencyTest, Relative_WildcardPath)
{
    // Make sure the * wildcard works even if the full filename is given
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test("*a.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre("a.foo", "1a.foo"));
}

TEST_F(WildcardSourceDependencyTest, Absolute_WithFolder)
{
    // Make sure we can use absolute paths to filter to files under a folder
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test(m_assetRootDir.absoluteFilePath("subfolder2/*.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre("a.foo", "b.foo", "folder/one/c.foo", "folder/one/d.foo"));
}

TEST_F(WildcardSourceDependencyTest, Absolute_NotInScanfolder)
{
    // Files outside a scanfolder should not be returned even with an absolute path
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test(m_assetRootDir.absoluteFilePath("not/a/scanfolder/*.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_NotInScanfolder)
{
    // Files outside a scanfolder should not be returned
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test("*/e.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_InNonRecursiveScanfolder)
{
    // Files deep inside non-recursive scanfolders should not be returned
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test("*/f.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Absolute_InNonRecursiveScanfolder)
{
    // Absolute paths to files deep inside non-recursive scanfolders should not be returned
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test(m_assetRootDir.absoluteFilePath("one/two/three/*.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_NoWildcard)
{
    // No wildcard results in a failure
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_FALSE(Test("subfolder1/1a.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Absolute_NoWildcard)
{
    // No wildcard results in a failure
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_FALSE(Test(m_assetRootDir.absoluteFilePath("subfolder1/1a.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_IgnoredFolder)
{
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test("*g.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Absolute_IgnoredFolder)
{
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test(m_assetRootDir.absoluteFilePath("*g.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_IgnoredFile)
{
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test("*z.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Absolute_IgnoredFile)
{
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test(m_assetRootDir.absoluteFilePath("*z.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_CacheFolder)
{
    AZStd::vector<AZStd::string> resolvedPaths;

    ASSERT_TRUE(Test("*cache.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, FilesAddedAfterInitialCache)
{
    AZStd::vector<AZStd::string> resolvedPaths;

    auto excludedFolderCacheInterface = AZ::Interface<ExcludedFolderCacheInterface>::Get();

    ASSERT_TRUE(excludedFolderCacheInterface);

    {
        const auto& excludedFolders = excludedFolderCacheInterface->GetExcludedFolders();

        ASSERT_EQ(excludedFolders.size(), 2);
    }

    // Add a file to a new ignored folder
    QString newFilePath = m_assetRootDir.absoluteFilePath("subfolder2/folder/two/ignored/three/new.foo");
    UnitTestUtils::CreateDummyFile(newFilePath);

    excludedFolderCacheInterface->FileAdded(newFilePath);

    const auto& excludedFolders = excludedFolderCacheInterface->GetExcludedFolders();

    ASSERT_EQ(excludedFolders.size(), 3);
    ASSERT_THAT(excludedFolders, ::testing::Contains(AZStd::string(m_assetRootDir.absoluteFilePath("subfolder2/folder/two/ignored").toUtf8().constData())));
}

TEST_F(WildcardSourceDependencyTest, FilesRemovedAfterInitialCache)
{
    AZStd::vector<AZStd::string> resolvedPaths;

    // Add a file to a new ignored folder
    QString newFilePath = m_assetRootDir.absoluteFilePath("subfolder2/folder/two/ignored/three/new.foo");
    UnitTestUtils::CreateDummyFile(newFilePath);

    auto excludedFolderCacheInterface = AZ::Interface<ExcludedFolderCacheInterface>::Get();

    ASSERT_TRUE(excludedFolderCacheInterface);

    {
        const auto& excludedFolders = excludedFolderCacheInterface->GetExcludedFolders();

        ASSERT_EQ(excludedFolders.size(), 3);
    }

    m_fileStateCache->SignalDeleteEvent(m_assetRootDir.absoluteFilePath("subfolder2/folder/two/ignored"));

    const auto& excludedFolders = excludedFolderCacheInterface->GetExcludedFolders();

    ASSERT_EQ(excludedFolders.size(), 2);
}

TEST_F(WildcardSourceDependencyTest, NewFile_MatchesSavedRelativeDependency)
{
    auto matches = FileAddedTest(m_assetRootDir.absoluteFilePath("subfolder1/1a.foo"));

    ASSERT_THAT(matches, ::testing::UnorderedElementsAre(m_assetRootDir.absoluteFilePath("subfolder2/a.foo").toUtf8().constData()));
}

TEST_F(WildcardSourceDependencyTest, NewFile_MatchesSavedAbsoluteDependency)
{
    auto matches = FileAddedTest(m_assetRootDir.absoluteFilePath("subfolder1/1b.foo"));

    ASSERT_THAT(matches, ::testing::UnorderedElementsAre(m_assetRootDir.absoluteFilePath("subfolder2/b.foo").toUtf8().constData()));
}

TEST_F(WildcardSourceDependencyTest, NewFile_MatchesDuplicatedDependenciesOnce)
{
    auto matches = FileAddedTest(m_assetRootDir.absoluteFilePath("subfolder2/folder/one/c.foo"));

    ASSERT_THAT(matches, ::testing::UnorderedElementsAre(m_assetRootDir.absoluteFilePath("subfolder2/folder/one/d.foo").toUtf8().constData()));
}
