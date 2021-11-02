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

using namespace AssetProcessor;

class AssetProcessorManager_Test
    : public AssetProcessorManager
{
public:
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, AssetProcessedImpl_DifferentProductDependenciesPerProduct_SavesCorrectlyToDatabase);

    friend class GTEST_TEST_CLASS_NAME_(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies);
    friend class GTEST_TEST_CLASS_NAME_(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies_DeferredResolution);

    friend class GTEST_TEST_CLASS_NAME_(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies_SourcePath);

    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, DeleteFolder_SignalsDeleteOfContainedFiles);

    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_BasicTest);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_WithDifferentTypes_BasicTest);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_Reverse_BasicTest);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_MissingFiles_ReturnsNoPathWithPlaceholders);

    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_BeforeComputingDirtiness_AllDirty);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_EmptyDatabase_AllDirty);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_SameAsLastTime_NoneDirty);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_MoreThanLastTime_NewOneIsDirty);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_FewerThanLastTime_Dirty);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_ChangedPattern_CountsAsNew);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_ChangedPatternType_CountsAsNew);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_NewPattern_CountsAsNewBuilder);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_NewVersionNumber_IsNotANewBuilder);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, BuilderDirtiness_NewAnalysisFingerprint_IsNotANewBuilder);

    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_BasicTest);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_UpdateTest);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByName);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid_UpdatesWhenTheyAppear);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByName_UpdatesWhenTheyAppear);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_WildcardMissingFiles_ByName_UpdatesWhenTheyAppear);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, JobDependencyOrderOnce_MultipleJobs_EmitOK);

    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, SourceFileProcessFailure_ClearsFingerprint);

    friend class GTEST_TEST_CLASS_NAME_(AbsolutePathProductDependencyTest, UnresolvedProductPathDependency_AssetProcessedTwice_DoesNotDuplicateDependency);
    friend class GTEST_TEST_CLASS_NAME_(AbsolutePathProductDependencyTest, AbsolutePathProductDependency_RetryDeferredDependenciesWithMatchingSource_DependencyResolves);
    friend class GTEST_TEST_CLASS_NAME_(AbsolutePathProductDependencyTest, UnresolvedProductPathDependency_AssetProcessedTwice_ValidatePathDependenciesMap);
    friend class GTEST_TEST_CLASS_NAME_(AbsolutePathProductDependencyTest, UnresolvedSourceFileTypeProductPathDependency_DependencyHasNoProductOutput_ValidatePathDependenciesMap);

    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_FileUnchanged_WithoutModtimeSkipping);

    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_FileUnchanged);

    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_EnablePlatform_ShouldProcessFilesForPlatform);

    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_ModifyFile);
    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_ModifyFile_AndThenRevert_ProcessesAgain);
    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_ModifyFilesSameHash_BothProcess);
    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_ModifyTimestamp);
    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_ModifyTimestampNoHashing_ProcessesFile);
    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_ModifyMetadataFile);
    friend class GTEST_TEST_CLASS_NAME_(ModtimeScanningTest, ModtimeSkipping_DeleteFile);
    friend class GTEST_TEST_CLASS_NAME_(DeleteTest, DeleteFolderSharedAcrossTwoScanFolders_CorrectFileAndFolderAreDeletedFromCache);
    friend class GTEST_TEST_CLASS_NAME_(MetadataFileTest, MetadataFile_SourceFileExtensionDifferentCase);

    friend class AssetProcessorManagerTest;
    friend struct ModtimeScanningTest;
    friend struct JobDependencyTest;
    friend struct ChainJobDependencyTest;
    friend struct DeleteTest;
    friend struct PathDependencyTest;
    friend struct DuplicateProductsTest;
    friend struct DuplicateProcessTest;
    friend struct AbsolutePathProductDependencyTest;
    friend struct WildcardSourceDependencyTest;

    explicit AssetProcessorManager_Test(PlatformConfiguration* config, QObject* parent = nullptr);
    ~AssetProcessorManager_Test() override;

    bool CheckJobKeyToJobRunKeyMap(AZStd::string jobKey);

    int CountDirtyBuilders() const
    {
        int numDirty = 0;
        for (const auto& element : m_builderDataCache)
        {
            if (element.second.m_isDirty)
            {
                ++numDirty;
            }
        }
        return numDirty;
    }

    bool IsBuilderDirty(const AZ::Uuid& builderBusId) const
    {
        auto finder = m_builderDataCache.find(builderBusId);
        if (finder == m_builderDataCache.end())
        {
            return true;
        }
        return finder->second.m_isDirty;
    }
};

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

    if(AZ::Debug::Trace::IsDebuggerPresent())
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

    AssetProcessorTest::SetUp();

    m_data = AZStd::make_unique<StaticData>();

    m_config.reset(new AssetProcessor::PlatformConfiguration());
    m_mockApplicationManager.reset(new AssetProcessor::MockApplicationManager());

    AssetUtilities::ResetAssetRoot();

    m_scopeDir = AZStd::make_unique<UnitTestUtils::ScopedDir>();
    m_scopeDir->Setup(m_tempDir.path());
    QDir tempPath(m_tempDir.path());

    auto registry = AZ::SettingsRegistry::Get();
    auto cacheRootKey =
        AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_cache_path";
    registry->Set(cacheRootKey, tempPath.absoluteFilePath("Cache").toUtf8().constData());
    auto projectPathKey =
        AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
    registry->Set(projectPathKey, "AutomatedTesting");
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

    m_data->m_databaseLocationListener.BusConnect();

    // in other unit tests we may open the database called ":memory:" to use an in-memory database instead of one on disk.
    // in this test, however, we use a real database, because the file processor shares it and opens its own connection to it.
    // ":memory:" databases are one-instance-only, and even if another connection is opened to ":memory:" it would
    // not share with others created using ":memory:" and get a unique database instead.
    m_data->m_databaseLocation = tempPath.absoluteFilePath("test_database.sqlite").toUtf8().constData();

    ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
        .WillByDefault(
            DoAll( // set the 0th argument ref (string) to the database location and return true.
                SetArgReferee<0>(m_data->m_databaseLocation),
                Return(true)));

    m_gameName = AssetUtilities::ComputeProjectName("AutomatedTesting", true);

    AssetUtilities::ResetAssetRoot();
    QDir newRoot;
    AssetUtilities::ComputeEngineRoot(newRoot, &tempPath);

    QDir cacheRoot;
    AssetUtilities::ComputeProjectCacheRoot(cacheRoot);
    QString normalizedCacheRoot = AssetUtilities::NormalizeDirectoryPath(cacheRoot.absolutePath());

    m_normalizedCacheRootDir.setPath(normalizedCacheRoot);

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));

    m_config->EnablePlatform({ "pc", { "host", "renderer", "desktop" } }, true);

    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", false, true, m_config->GetEnabledPlatforms(), 1));
    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2/redirected"), "subfolder2", "subfolder2", false, true, m_config->GetEnabledPlatforms()));
    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3", false, true, m_config->GetEnabledPlatforms(), 1));
    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", false, true, m_config->GetEnabledPlatforms(), 1));
    m_config->AddMetaDataType("assetinfo", "");
    AssetRecognizer rec;
    AssetPlatformSpec specpc;

    rec.m_name = "txt files";
    rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec.m_platformSpecs.insert("pc", specpc);
    rec.m_supportsCreateJobs = false;
    ASSERT_TRUE(m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec));
    m_mockApplicationManager->BusConnect();

    AZ_Printf("UnitTest", "Allocating APM\n")
    m_assetProcessorManager.reset(new AssetProcessorManager_Test(m_config.get()));
    AZ_Printf("UnitTest", "APM ready\n");
    m_assertAbsorber.Clear();

    m_isIdling = false;

    m_idleConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState, [this](bool newState)
    {
        m_isIdling = newState;
    });
}

void AssetProcessorManagerTest::TearDown()
{
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

    AssetProcessor::AssetProcessorTest::TearDown();
}

TEST_F(AssetProcessorManagerTest, UnitTestForGettingJobInfoBySourceUUIDSuccess)
{
    // Here we first mark a job for an asset complete and than fetch jobs info using the job log api to verify
    // Next we mark another job for that same asset as queued, and we again fetch jobs info from the api to verify,

    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QDir tempPath(m_tempDir.path());

    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolder = tempPath.absoluteFilePath("subfolder1");

    JobEntry entry;
    entry.m_watchFolderPath = watchFolder;
    entry.m_databaseSourceName = entry.m_pathRelativeToWatchFolder = relFileName;
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    UnitTestUtils::CreateDummyFile(m_normalizedCacheRootDir.absoluteFilePath("outputfile.txt"));

    AssetBuilderSDK::ProcessJobResponse jobResponse;
    jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(m_normalizedCacheRootDir.absoluteFilePath("outputfile.txt").toUtf8().data()));

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
    EXPECT_STRCASEEQ(tempPath.filePath("subfolder1").toUtf8().data(), response.m_jobList[0].m_watchFolder.c_str());

    ASSERT_EQ(m_assertAbsorber.m_numWarningsAbsorbed, 0);
    ASSERT_EQ(m_assertAbsorber.m_numErrorsAbsorbed, 0);
    ASSERT_EQ(m_assertAbsorber.m_numAssertsAbsorbed, 0);
}

TEST_F(AssetProcessorManagerTest, WarningsAndErrorsReported_SuccessfullySavedToDatabase)
{
    // This tests the JobDiagnosticTracker:  Warnings/errors reported to it should be recorded in the database when AssetProcessed is fired and able to be retrieved when querying job status

    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QDir tempPath(m_tempDir.path());

    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolder = tempPath.absoluteFilePath("subfolder1");

    JobEntry entry;
    entry.m_watchFolderPath = watchFolder;
    entry.m_databaseSourceName = entry.m_pathRelativeToWatchFolder = relFileName;
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    UnitTestUtils::CreateDummyFile(m_normalizedCacheRootDir.absoluteFilePath("outputfile.txt"));

    AssetBuilderSDK::ProcessJobResponse jobResponse;
    jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(m_normalizedCacheRootDir.absoluteFilePath("outputfile.txt").toUtf8().data()));

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

    ASSERT_EQ(m_assertAbsorber.m_numWarningsAbsorbed, 0);
    ASSERT_EQ(m_assertAbsorber.m_numErrorsAbsorbed, 0);
    ASSERT_EQ(m_assertAbsorber.m_numAssertsAbsorbed, 0);
}


TEST_F(AssetProcessorManagerTest, DeleteFolder_SignalsDeleteOfContainedFiles)
{
    using namespace AssetProcessor;
    QDir tempPath(m_tempDir.path());

    static constexpr char folderPathNoScanfolder[] = "folder/folder/foldertest.txt";
    static constexpr char folderPath[] = "subfolder1/folder/folder/foldertest.txt";

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath(folderPath));

    auto scanFolderInfo = m_config->GetScanFolderByPath(tempPath.absoluteFilePath("subfolder1"));
    ASSERT_TRUE(scanFolderInfo != nullptr);

    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry(
        scanFolderInfo->ScanFolderID(),
        folderPathNoScanfolder,
        AZ::Uuid::CreateRandom(),
        /*analysisFingerprint - arbitrary*/ "abcdefg");
    m_assetProcessorManager->m_stateData->SetSource(sourceEntry);

    int count = 0;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::SourceDeleted, [&count](QString file)
        {
            if (file.compare(folderPathNoScanfolder, Qt::CaseInsensitive) == 0)
            {
                count++;
            }
        });

    m_isIdling = false;
    // tell the APM about the files:
    m_assetProcessorManager->AssessAddedFile(tempPath.absoluteFilePath(folderPath));

    ASSERT_TRUE(BlockUntilIdle(5000));

    EXPECT_TRUE(QDir(tempPath.absoluteFilePath("subfolder1/folder")).removeRecursively());

    m_isIdling = false;
    m_assetProcessorManager->AssessDeletedFile(tempPath.absoluteFilePath("subfolder1/folder"));
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

    QDir tempPath(m_tempDir.path());
    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    JobEntry entry;

    entry.m_watchFolderPath = tempPath.absolutePath();
    entry.m_databaseSourceName = entry.m_pathRelativeToWatchFolder = relFileName;

    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(entry.m_databaseSourceName.toUtf8().data());
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

// if our database was empty before, all builders should be dirty
// note that this requires us to actually register a builder using the mock.
TEST_F(AssetProcessorManagerTest, BuilderDirtiness_EmptyDatabase_AllDirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 2);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(mockBuilderResponder.m_assetBuilderDescs[0].m_busId));
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(mockBuilderResponder.m_assetBuilderDescs[1].m_busId));

    mockBuilderResponder.BusDisconnect();
}

// if we have the same set of builders the next time, nothing should register as changed.
TEST_F(AssetProcessorManagerTest, BuilderDirtiness_SameAsLastTime_NoneDirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");

    m_assetProcessorManager->ComputeBuilderDirty();

    // now we retrigger the dirty computation, so that nothing has changed:
    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_FALSE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_FALSE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 0);

    mockBuilderResponder.BusDisconnect();
}

// when a new builder appears, the new builder should be dirty,
TEST_F(AssetProcessorManagerTest, BuilderDirtiness_MoreThanLastTime_NewOneIsDirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");

    m_assetProcessorManager->ComputeBuilderDirty();

    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");

    m_assetProcessorManager->ComputeBuilderDirty();

    // one new builder should have been dirty:
    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(mockBuilderResponder.m_assetBuilderDescs[1].m_busId));

    mockBuilderResponder.BusDisconnect();
}


// when an existing builder disappears there are no dirty builders, but the booleans
// that track dirtiness should be correct:
TEST_F(AssetProcessorManagerTest, BuilderDirtiness_FewerThanLastTime_Dirty)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");

    m_assetProcessorManager->ComputeBuilderDirty();

    // remove one:
    mockBuilderResponder.m_assetBuilderDescs.pop_back();

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 0);
}

// if a builder changes its pattern matching, it should be dirty, and also, it should count as add or remove.
TEST_F(AssetProcessorManagerTest, BuilderDirtiness_ChangedPattern_CountsAsNew)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");
    mockBuilderResponder.AddBuilder("builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint3");
    mockBuilderResponder.AddBuilder("builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    // here, we change the actual text of the pattern to match
    size_t whichToChange = 1;
    // here, we change the pattern type but not the pattern to match
    AssetBuilderPattern oldPattern = mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns[0];
    oldPattern.m_pattern = "*.somethingElse";
    mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.clear();
    mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.emplace_back(oldPattern);

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    mockBuilderResponder.BusDisconnect();
}

TEST_F(AssetProcessorManagerTest, BuilderDirtiness_ChangedPatternType_CountsAsNew)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");
    mockBuilderResponder.AddBuilder("builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint3");
    mockBuilderResponder.AddBuilder("builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    size_t whichToChange = 2;
    // here, we change the pattern type but not the pattern to match
    AssetBuilderPattern oldPattern = mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns[0];
    oldPattern.m_type = AssetBuilderPattern::Regex;
    mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.clear();
    mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.emplace_back(oldPattern);

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    mockBuilderResponder.BusDisconnect();
}

TEST_F(AssetProcessorManagerTest, BuilderDirtiness_NewPattern_CountsAsNewBuilder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");
    mockBuilderResponder.AddBuilder("builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint3");
    mockBuilderResponder.AddBuilder("builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    size_t whichToChange = 3;
    // here, we add an additional pattern that wasn't there before:
    mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.clear();
    mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.buzz", AssetBuilderPattern::Wildcard));

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_TRUE(m_assetProcessorManager->m_buildersAddedOrRemoved);
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    mockBuilderResponder.BusDisconnect();
}

// changing the "version" of a builder should be equivalent to changing its analysis fingerprint - ie
// it should not count as adding a new builder.
TEST_F(AssetProcessorManagerTest, BuilderDirtiness_NewVersionNumber_IsNotANewBuilder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");
    mockBuilderResponder.AddBuilder("builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint3");
    mockBuilderResponder.AddBuilder("builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    size_t whichToChange = 3;
    mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_version++;

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_FALSE(m_assetProcessorManager->m_buildersAddedOrRemoved); // <-- note, we don't expect this to be considered a 'new builder'
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    mockBuilderResponder.BusDisconnect();
}

// changing the "analysis fingerprint" of a builder should not count as an addition or removal
// but should still result in that specific builder being considered as a dirty builder.
TEST_F(AssetProcessorManagerTest, BuilderDirtiness_NewAnalysisFingerprint_IsNotANewBuilder)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    m_mockApplicationManager->BusDisconnect();

    MockBuilderResponder mockBuilderResponder;
    mockBuilderResponder.BusConnect();

    mockBuilderResponder.AddBuilder("builder1", { AssetBuilderSDK::AssetBuilderPattern("*.egg", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint1");
    mockBuilderResponder.AddBuilder("builder2", { AssetBuilderSDK::AssetBuilderPattern("*.foo", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint2");
    mockBuilderResponder.AddBuilder("builder3", { AssetBuilderSDK::AssetBuilderPattern("*.bar", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint3");
    mockBuilderResponder.AddBuilder("builder4", { AssetBuilderSDK::AssetBuilderPattern("*.baz", AssetBuilderPattern::Wildcard) }, AZ::Uuid::CreateRandom(), 1, "fingerprint4");

    m_assetProcessorManager->ComputeBuilderDirty();

    size_t whichToChange = 3;
    mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_analysisFingerprint = "changed!!";

    m_assetProcessorManager->ComputeBuilderDirty();

    EXPECT_TRUE(m_assetProcessorManager->m_anyBuilderChange);
    EXPECT_FALSE(m_assetProcessorManager->m_buildersAddedOrRemoved); // <-- note, we don't expect this to be considered a 'new builder'
    EXPECT_EQ(m_assetProcessorManager->CountDirtyBuilders(), 1);
    EXPECT_TRUE(m_assetProcessorManager->IsBuilderDirty(mockBuilderResponder.m_assetBuilderDescs[whichToChange].m_busId));

    mockBuilderResponder.BusDisconnect();

    m_mockApplicationManager->BusConnect();
}

// ------------------------------------------------------------------------------------------------
//                         QueryAbsolutePathDependenciesRecursive section
// ------------------------------------------------------------------------------------------------

TEST_F(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_BasicTest)
{
    using SourceFileDependencyEntry = AzToolsFramework::AssetDatabase::SourceFileDependencyEntry;

    //  A depends on B, which depends on both C and D

    QDir tempPath(m_tempDir.path());

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/a.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/b.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/c.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/d.txt"), QString("tempdata\n"));

    SourceFileDependencyEntry newEntry1;  // a depends on B
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_source = "a.txt";
    newEntry1.m_dependsOnSource = "b.txt";

    SourceFileDependencyEntry newEntry2; // b depends on C
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_source = "b.txt";
    newEntry2.m_dependsOnSource = "c.txt";

    SourceFileDependencyEntry newEntry3;  // b also depends on D
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_source = "b.txt";
    newEntry3.m_dependsOnSource = "d.txt";

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry1));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry2));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry3));

    AssetProcessor::SourceFilesForFingerprintingContainer dependencies;
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("a.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, false );
    EXPECT_EQ(dependencies.size(), 4); // a depends on b, c, and d - with the latter two being indirect.

    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    // make sure the corresponding values in the map are also correct (ie, database path only)
    EXPECT_STREQ(dependencies[tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()].c_str(), "a.txt");
    EXPECT_STREQ(dependencies[tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()].c_str(), "b.txt");
    EXPECT_STREQ(dependencies[tempPath.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()].c_str(), "c.txt");
    EXPECT_STREQ(dependencies[tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()].c_str(), "d.txt");

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("b.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, false);
    EXPECT_EQ(dependencies.size(), 3); // b  depends on c, and d
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    // eliminate b --> c
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->RemoveSourceFileDependency(newEntry2.m_sourceDependencyID));

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("a.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, false);
    EXPECT_EQ(dependencies.size(), 3); // a depends on b and d, but no longer c
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());
}

TEST_F(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_WithDifferentTypes_BasicTest)
{
    // test to make sure that different TYPES of dependencies work as expected.
    using SourceFileDependencyEntry = AzToolsFramework::AssetDatabase::SourceFileDependencyEntry;

    QDir tempPath(m_tempDir.path());

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/a.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/b.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/c.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/d.txt"), QString("tempdata\n"));

    SourceFileDependencyEntry newEntry1;  // a depends on B as a SOURCE dependency.
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_source = "a.txt";
    newEntry1.m_dependsOnSource = "b.txt";
    newEntry1.m_typeOfDependency = SourceFileDependencyEntry::DEP_SourceToSource;

    SourceFileDependencyEntry newEntry2; // b depends on C as a JOB dependency
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_source = "b.txt";
    newEntry2.m_dependsOnSource = "c.txt";
    newEntry2.m_typeOfDependency = SourceFileDependencyEntry::DEP_JobToJob;

    SourceFileDependencyEntry newEntry3;  // b also depends on D as a SOURCE dependency
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_source = "b.txt";
    newEntry3.m_dependsOnSource = "d.txt";
    newEntry3.m_typeOfDependency = SourceFileDependencyEntry::DEP_SourceToSource;

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry1));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry2));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry3));

    AssetProcessor::SourceFilesForFingerprintingContainer dependencies;
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("a.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, false);
    // note that a depends on b, c, and d - with the latter two being indirect.
    // however, since b's dependency on C is via JOB, and we're asking for SOURCE only, we should not see C.
    EXPECT_EQ(dependencies.size(), 3);

    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("b.txt", dependencies, SourceFileDependencyEntry::DEP_JobToJob, false);
    // b  depends on c, and d  - but we're asking for job dependencies only, so we should not get anything except C and B
    EXPECT_EQ(dependencies.size(), 2);
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());

    // now ask for ALL kinds and you should get the full tree.
    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("a.txt", dependencies, SourceFileDependencyEntry::DEP_Any, false);
    EXPECT_EQ(dependencies.size(), 4);
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());
}

// ------------------------------------------------------------------------------------------------
//                         QueryAbsolutePathDependenciesRecursive REVERSE section
// ------------------------------------------------------------------------------------------------

TEST_F(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_Reverse_BasicTest)
{
    using SourceFileDependencyEntry = AzToolsFramework::AssetDatabase::SourceFileDependencyEntry;

    //  A depends on B, which depends on both C and D

    QDir tempPath(m_tempDir.path());

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/a.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/b.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/c.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/d.txt"), QString("tempdata\n"));

    SourceFileDependencyEntry newEntry1;  // a depends on B
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_source = "a.txt";
    newEntry1.m_dependsOnSource = "b.txt";

    SourceFileDependencyEntry newEntry2; // b depends on C
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_source = "b.txt";
    newEntry2.m_dependsOnSource = "c.txt";

    SourceFileDependencyEntry newEntry3;  // b also depends on D
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_source = "b.txt";
    newEntry3.m_dependsOnSource = "d.txt";

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry1));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry2));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry3));

    AssetProcessor::SourceFilesForFingerprintingContainer dependencies;
    // sanity: what Depends on a?  the only result should be a itself.
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("a.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, true /*reverse*/);
    EXPECT_EQ(dependencies.size(), 1);
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());

    dependencies.clear();
    // what depends on d?  b and a should (indirectly)
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("d.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, true);
    EXPECT_EQ(dependencies.size(), 3);
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    // what depends on c?  b and a should.
    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("c.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, true);
    EXPECT_EQ(dependencies.size(), 3); // b  depends on c, and d
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/b.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());

    // eliminate b --> c
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->RemoveSourceFileDependency(newEntry2.m_sourceDependencyID));

    // what depends on c?  nothing.
    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("c.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, true);
    EXPECT_EQ(dependencies.size(), 1); // a depends on b and d, but no longer c
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/c.txt").toUtf8().constData()), dependencies.end());
}

// since we need these files to still produce a 0-based fingerprint, we need them to
// still do a best guess at absolute path, when they are missing.
TEST_F(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_MissingFiles_ReturnsNoPathWithPlaceholders)
{
    using SourceFileDependencyEntry = AzToolsFramework::AssetDatabase::SourceFileDependencyEntry;

    //  A depends on B, which depends on both C and D

    QDir tempPath(m_tempDir.path());

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/a.txt"), QString("tempdata\n"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/d.txt"), QString("tempdata\n"));
    // note that we don't actually create b and c here, they are missing.

    SourceFileDependencyEntry newEntry1;  // a depends on B
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_source = "a.txt";
    newEntry1.m_dependsOnSource = "b.txt";

    SourceFileDependencyEntry newEntry2; // b depends on C
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_source = "b.txt";
    newEntry2.m_dependsOnSource = "c.txt";

    SourceFileDependencyEntry newEntry3;  // b also depends on D
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_source = "b.txt";
    newEntry3.m_dependsOnSource = "d.txt";

    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry1));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry2));
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->SetSourceFileDependency(newEntry3));

    AssetProcessor::SourceFilesForFingerprintingContainer dependencies;
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("a.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, false);
    EXPECT_EQ(dependencies.size(), 2); // a depends on b, c, and d - with the latter two being indirect.

    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("b.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, false);
    EXPECT_EQ(dependencies.size(), 1); // b  depends on c, and d
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());

    // eliminate b --> c
    ASSERT_TRUE(m_assetProcessorManager->m_stateData->RemoveSourceFileDependency(newEntry2.m_sourceDependencyID));

    dependencies.clear();
    m_assetProcessorManager->QueryAbsolutePathDependenciesRecursive("a.txt", dependencies, SourceFileDependencyEntry::DEP_SourceToSource, false);
    EXPECT_EQ(dependencies.size(), 2); // a depends on b and d, but no longer c
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/a.txt").toUtf8().constData()), dependencies.end());
    EXPECT_NE(dependencies.find(tempPath.absoluteFilePath("subfolder1/d.txt").toUtf8().constData()), dependencies.end());
}

TEST_F(AssetProcessorManagerTest, BuilderSDK_API_CreateJobs_HasValidParameters_WithNoOutputFolder)
{
    QDir tempPath(m_tempDir.path());
    // here we push a file change through APM and make sure that "CreateJobs" has correct parameters, with no output redirection
    QString absPath(tempPath.absoluteFilePath("subfolder1/test_text.txt"));
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

    EXPECT_STREQ(req.m_watchFolder.c_str(), tempPath.absoluteFilePath("subfolder1").toUtf8().constData());
    EXPECT_STREQ(req.m_sourceFile.c_str(), "test_text.txt"); // only the name should be there, no output prefix.

    EXPECT_NE(req.m_sourceFileUUID, AZ::Uuid::CreateNull());
    EXPECT_TRUE(req.HasPlatform("pc"));
    EXPECT_TRUE(req.HasPlatformWithTag("desktop"));
}


TEST_F(AssetProcessorManagerTest, BuilderSDK_API_CreateJobs_HasValidParameters_WithOutputRedirectedFolder)
{
    QDir tempPath(m_tempDir.path());
    // here we push a file change through APM and make sure that "CreateJobs" has correct parameters, with no output redirection
    QString absPath(tempPath.absoluteFilePath("subfolder2/redirected/test_text.txt"));
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
    EXPECT_STREQ(req.m_watchFolder.c_str(), tempPath.absoluteFilePath("subfolder2/redirected").toUtf8().constData());
    EXPECT_STREQ(req.m_sourceFile.c_str(), "test_text.txt"); // only the name should be there, no output prefix.

    EXPECT_NE(req.m_sourceFileUUID, AZ::Uuid::CreateNull());
    EXPECT_TRUE(req.HasPlatform("pc"));
    EXPECT_TRUE(req.HasPlatformWithTag("desktop"));
}

void AbsolutePathProductDependencyTest::SetUp()
{
    using namespace AzToolsFramework::AssetDatabase;
    AssetProcessorManagerTest::SetUp();

    QDir tempPath(m_tempDir.path());
    m_scanFolderInfo = m_config->GetScanFolderByPath(tempPath.absoluteFilePath("subfolder4"));
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

    m_assetProcessorManager->m_pathDependencyManager->RetryDeferredDependencies(matchingSource);

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
    AssetPlatformSpec specpc;

    rec.m_name = "txt files2";
    rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec.m_platformSpecs.insert("pc", specpc);
    rec.m_supportsCreateJobs = false;
    m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec);

    m_sharedConnection = m_assetProcessorManager->m_stateData.get();
    ASSERT_TRUE(m_sharedConnection);
}

void PathDependencyTest::TearDown()
{
    ASSERT_EQ(m_assertAbsorber.m_numAssertsAbsorbed, 0);
    ASSERT_EQ(m_assertAbsorber.m_numErrorsAbsorbed, 0);

    AssetProcessorManagerTest::TearDown();
}

void PathDependencyTest::CaptureJobs(AZStd::vector<AssetProcessor::JobDetails>& jobDetailsList, const char* sourceFilePath)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;
    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath(sourceFilePath));
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
            if(jobSet >= capturedDetails.size() || capturedDetails[jobSet].m_destinationPath.isEmpty())
            {
                return false;
            }

            QString outputAssetPath = QDir(capturedDetails[jobSet].m_destinationPath).absoluteFilePath(QString(asset.m_name.c_str()) + outputExtension);

            UnitTestUtils::CreateDummyFile(outputAssetPath, "this is a test output asset");

            JobProduct jobProduct(outputAssetPath.toUtf8().constData(), AZ::Uuid::CreateRandom(), subIdCounter);
            jobProduct.m_pathDependencies.insert(dependencies.begin(), dependencies.end());

            processJobResponse.m_outputProducts.push_back(jobProduct);
            asset.m_products.push_back(AZ::Data::AssetId(capturedDetails[jobSet].m_jobEntry.m_sourceFileUUID, subIdCounter));

            subIdCounter++;
        }

        // tell the APM that the asset has been processed and allow it to bubble through its event queue:
        m_isIdling = false;
        m_assetProcessorManager->AssetProcessed(capturedDetails[jobSet].m_jobEntry, processJobResponse);

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

    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath(sourceFilePath));
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
            QString outputAssetPath = QDir(job.m_destinationPath).absoluteFilePath("test.asset");

            UnitTestUtils::CreateDummyFile(outputAssetPath, "this is a test output asset");

            JobProduct jobProduct(outputAssetPath.toUtf8().constData());
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
    AssetPlatformSpec specpc;

    rec.m_name = "txt files2";
    rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec.m_platformSpecs.insert("pc", specpc);
    rec.m_supportsCreateJobs = false;
    m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec);

    AzFramework::AssetSystem::AssetNotificationMessage details;

    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetMessage, [&details](AzFramework::AssetSystem::AssetNotificationMessage message)
        {
            details = message;
        });

    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/test1.txt"));

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

    ASSERT_FALSE(jobDetails.m_destinationPath.isEmpty());

    // create a product asset
    QString outputAssetPath = QDir(jobDetails.m_destinationPath).absoluteFilePath(QString(mainFile.m_name.c_str()) + ".asset");
    UnitTestUtils::CreateDummyFile(outputAssetPath, "this is a test output asset");

    // add the new product asset to its own product dependencies list by assetId
    JobProduct jobProduct(outputAssetPath.toUtf8().constData(), outputAssetTypeId, subId);
    AZ::Data::AssetId productAssetId(jobDetails.m_jobEntry.m_sourceFileUUID, subId);
    jobProduct.m_dependencies.push_back(ProductDependency(productAssetId, 5));

    // add the product asset to its own product dependencies list by path
    jobProduct.m_pathDependencies.emplace(ProductPathDependency(AZStd::string::format("%s%s", mainFile.m_name.c_str(), ".asset"), ProductPathDependencyType::ProductFile));

    processJobResponse.m_outputProducts.push_back(jobProduct);
    mainFile.m_products.push_back(productAssetId);

    // tell the APM that the asset has been processed and allow it to bubble through its event queue:
    m_assertAbsorber.Clear();
    m_assetProcessorManager->AssetProcessed(jobDetails.m_jobEntry, processJobResponse);
    ASSERT_TRUE(BlockUntilIdle(5000));

    // Verify we have no entries in the ProductDependencies table
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    m_sharedConnection->GetProductDependencies(dependencyContainer);
    ASSERT_TRUE(dependencyContainer.empty());

    // We are testing 2 different dependencies, so we should get 2 warnings
    ASSERT_EQ(m_assertAbsorber.m_numWarningsAbsorbed, 2);
    m_assertAbsorber.Clear();
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
    ASSERT_TRUE(result) << "Failed to Process main test asset" << primaryFile2.m_name.c_str();;

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
    ASSERT_EQ(m_assertAbsorber.m_numErrorsAbsorbed, 4);
    m_assertAbsorber.Clear();
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
    ASSERT_EQ(m_assertAbsorber.m_numErrorsAbsorbed, 6);
    m_assertAbsorber.Clear();
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

// Tests product path dependencies using absolute paths to source files
TEST_F(PathDependencyTest, AbsoluteDependencies_Existing_ResolveCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/dep1.txt"));

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

    QDir tempPath(m_tempDir.path());
    AZStd::string relativePathDep1("dep1.txt");
    QString absPathDep1(tempPath.absoluteFilePath(QString("subfolder4%1%2").arg(QDir::separator()).arg(relativePathDep1.c_str())));
    // When an absolute path matches a scan folder, the portion of the path matching that scan folder
    // is replaced with the scan folder's ID.
    AZStd::string absPathDep1WithScanfolder(AZStd::string::format("$4$%s", relativePathDep1.c_str()));
    QString absPathDep2(tempPath.absoluteFilePath("subfolder2/redirected/dep2.txt"));
    QString absPathDep3(tempPath.absoluteFilePath("subfolder1/dep3.txt"));

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

    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/dep1.txt"));

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

    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/folderA/folderB/dep5.txt"));

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

    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/folderA\\folderB/dep5.txt"));

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

// This test ensures product path *product* file dependencies are matched by exact path
// Dep1 is output as test.asset#, Dep2 is output as redirected/test.asset#
// Dependencies on test.asset# should point to dep1 and never dep2
TEST_F(PathDependencyTest, AssetProcessed_Impl_DeferredPathResolution_CorrectlyMatchesWithScanFolderPrefix)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // -------- Make main test asset, with dependencies on products that don't exist yet -----
    TestAsset primaryFile("test_text");
    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} }, { {"test.asset1", ProductPathDependencyType::ProductFile}, {"test.asset2", ProductPathDependencyType::ProductFile} }));

    // create dependees
    TestAsset dep1("test");
    TestAsset dep2("test");

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }, {}, "subfolder1/"));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1"}, {".asset2"} }, {}, "subfolder2/redirected/"));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer, { dep1.m_products[0], dep2.m_products[1] });
}

// This test ensures product path *source* file dependencies are matched by exact path
TEST_F(PathDependencyTest, SourceFileDependencyWithPrefix_Deferred_ResolvesCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // -------- Make main test asset, with dependencies on products that don't exist yet -----
    TestAsset primaryFile("test_text");

    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} }, { {"test.txt", ProductPathDependencyType::SourceFile} }));

    // create dependees
    TestAsset dep1("test");
    TestAsset dep2("test");

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }, {}, "subfolder1/"));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1"}, {".asset2"} }, {}, "subfolder2/redirected/"));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep2.m_products[0],
            dep2.m_products[1],
        }
    );
}

TEST_F(PathDependencyTest, SourceFileDependencyWithPrefix_Existing_ResolvesCorrectly)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // create dependees
    TestAsset dep1("test");
    TestAsset dep2("test");

    ASSERT_TRUE(ProcessAsset(dep1, { {".asset1"}, {".asset2"} }, {}, "subfolder1/"));
    ASSERT_TRUE(ProcessAsset(dep2, { {".asset1"}, {".asset2"} }, {}, "subfolder2/redirected/"));

    // -------- Make main test asset, with dependencies on products we just created -----
    TestAsset primaryFile("test_text");

    ASSERT_TRUE(ProcessAsset(primaryFile, { { ".asset" }, {} }, { {"test.txt", ProductPathDependencyType::SourceFile} }));

    // ---------- Verify that the dependency was recorded, and did not keep the path after resolution ----------
    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
    ASSERT_TRUE(m_sharedConnection->GetProductDependencies(dependencyContainer));

    VerifyDependencies(dependencyContainer,
        {
            dep2.m_products[0],
            dep2.m_products[1],
        }
    );
}

void MultiplatformPathDependencyTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();
    m_config.reset(new AssetProcessor::PlatformConfiguration());
    m_config->EnablePlatform({ "pc", { "host", "renderer", "desktop" } }, true);
    m_config->EnablePlatform({ "provo",{ "console" } }, true);
    QDir tempPath(m_tempDir.path());

    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", false, true, m_config->GetEnabledPlatforms() ));
    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2", false, true, m_config->GetEnabledPlatforms()));
    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3", false, true, m_config->GetEnabledPlatforms()));

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
    rec.m_platformSpecs.insert("pc", AssetPlatformSpec());
    rec.m_platformSpecs.insert("provo", AssetPlatformSpec());
    rec.m_supportsCreateJobs = false;
    m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec);

    AssetRecognizer rec2;

    rec2.m_name = "single platform ini files";
    rec2.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ini", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec2.m_platformSpecs.insert("pc", AssetPlatformSpec());
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
    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/test_text.txt"));
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

    QString destTestPath1 = QDir(capturedDetails.m_destinationPath).absoluteFilePath("test1.txt");
    QString destTestPath2 = QDir(capturedDetails.m_destinationPath).absoluteFilePath("test2.txt");

    UnitTestUtils::CreateDummyFile(destTestPath1, "this is the first output");
    UnitTestUtils::CreateDummyFile(destTestPath2, "this is the second output");

    JobProduct productA(destTestPath1.toUtf8().constData(), AZ::Uuid::CreateRandom(), 1);
    JobProduct productB(destTestPath2.toUtf8().constData(), AZ::Uuid::CreateRandom(), 2);
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
    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/test_text.txt"));
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
        QString fileNameToGenerate = QString("test%1.txt").arg(outputIdx);
        QString filePathToGenerate = QDir(capturedDetails.m_destinationPath).absoluteFilePath(fileNameToGenerate);

        UnitTestUtils::CreateDummyFile(filePathToGenerate, "an output");
        JobProduct product(filePathToGenerate.toUtf8().constData(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(outputIdx));
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
    ASSERT_FALSE(capturedDetails.m_destinationPath.isEmpty());
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

        QString fileNameToGenerate = QString("test%1.txt").arg(outputIdx);
        QString filePathToGenerate = QDir(capturedDetails.m_destinationPath).absoluteFilePath(fileNameToGenerate);

        JobProduct product(filePathToGenerate.toUtf8().constData(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(outputIdx));
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

TEST_F(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_BasicTest)
{
    // make sure that if we publish some dependencies, they appear:
    AZ::Uuid dummyBuilderUUID = AZ::Uuid::CreateRandom();
    QDir tempPath(m_tempDir.path());
    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    QString dependsOnFile1_Source = tempPath.absoluteFilePath("subfolder1/a.txt");
    QString dependsOnFile2_Source = tempPath.absoluteFilePath("subfolder1/b.txt");
    QString dependsOnFile1_Job = tempPath.absoluteFilePath("subfolder1/c.txt");
    QString dependsOnFile2_Job = tempPath.absoluteFilePath("subfolder1/d.txt");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Job, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Job, QString("tempdata\n")));

    // construct the dummy job to feed to the database updater function:
    AssetProcessorManager::JobToProcessEntry job;
    job.m_sourceFileInfo.m_databasePath = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_pathRelativeToScanFolder = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_scanFolder = scanFolder;
    job.m_sourceFileInfo.m_uuid = AssetUtilities::CreateSafeSourceUUIDFromName(job.m_sourceFileInfo.m_databasePath.toUtf8().data());

    // note that we have to "prime" the map with the UUIDs to the source info for this to work:
    AZ::Uuid uuidOfB = AssetUtilities::CreateSafeSourceUUIDFromName("b.txt");
    AZ::Uuid uuidOfD = AssetUtilities::CreateSafeSourceUUIDFromName("d.txt");
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfB] = { watchFolderPath, "b.txt", "b.txt" };
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfD] = { watchFolderPath, "d.txt", "d.txt" };

    // each file we will take a different approach to publishing:  rel path, and UUID:
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "a.txt", AZ::Uuid::CreateNull() }));
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "", uuidOfB }));

    // it is currently assumed that the only fields that we care about in JobDetails is the builder busId and the job dependencies themselves:
    JobDetails newDetails;
    newDetails.m_assetBuilderDesc.m_busId = dummyBuilderUUID;

    AssetBuilderSDK::SourceFileDependency dep1 = {"c.txt", AZ::Uuid::CreateNull()};
    AssetBuilderSDK::JobDependency jobDep1("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep1);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep1));

    AssetBuilderSDK::SourceFileDependency dep2 = { "",uuidOfD };
    AssetBuilderSDK::JobDependency jobDep2("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep2);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep2));

    job.m_jobsToAnalyze.push_back(newDetails);

    // this is the one line that this unit test is really testing:
    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);

    // the rest of this test now performs a series of queries to verify the database was correctly set.
    // this indirectly verifies the QueryAbsolutePathDependenciesRecursive function also but it has its own dedicated tests, above.
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource, false);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Source.toUtf8().constData()), deps.end());

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob, false);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Job.toUtf8().constData()), deps.end());

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, false);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 5);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Job.toUtf8().constData()), deps.end());
}

TEST_F(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_UpdateTest)
{
    // make sure that if we remove dependencies that are published, they disappear.
    // so the first part of this test is to put some data in there, the same as before:

    AZ::Uuid dummyBuilderUUID = AZ::Uuid::CreateRandom();
    QDir tempPath(m_tempDir.path());
    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    QString dependsOnFile1_Source = tempPath.absoluteFilePath("subfolder1/a.txt");
    QString dependsOnFile2_Source = tempPath.absoluteFilePath("subfolder1/b.txt");
    QString dependsOnFile1_Job = tempPath.absoluteFilePath("subfolder1/c.txt");
    QString dependsOnFile2_Job = tempPath.absoluteFilePath("subfolder1/d.txt");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Job, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Job, QString("tempdata\n")));

    // construct the dummy job to feed to the database updater function:
    AssetProcessorManager::JobToProcessEntry job;
    job.m_sourceFileInfo.m_databasePath = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_pathRelativeToScanFolder = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_scanFolder = scanFolder;
    job.m_sourceFileInfo.m_uuid = AssetUtilities::CreateSafeSourceUUIDFromName(job.m_sourceFileInfo.m_databasePath.toUtf8().data());

    // note that we have to "prime" the map with the UUIDs to the source info for this to work:
    AZ::Uuid uuidOfB = AssetUtilities::CreateSafeSourceUUIDFromName("b.txt");
    AZ::Uuid uuidOfD = AssetUtilities::CreateSafeSourceUUIDFromName("d.txt");
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfB] = { watchFolderPath, "b.txt", "b.txt" };
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfD] = { watchFolderPath, "d.txt", "d.txt" };

    // each file we will take a different approach to publishing:  rel path, and UUID:
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "a.txt", AZ::Uuid::CreateNull() }));
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "", uuidOfB }));

    // it is currently assumed that the only fields that we care about in JobDetails is the builder busId and the job dependencies themselves:
    JobDetails newDetails;
    newDetails.m_assetBuilderDesc.m_busId = dummyBuilderUUID;

    AssetBuilderSDK::SourceFileDependency dep1 = { "c.txt", AZ::Uuid::CreateNull() };
    AssetBuilderSDK::JobDependency jobDep1("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep1);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep1));

    AssetBuilderSDK::SourceFileDependency dep2 = { "",uuidOfD };
    AssetBuilderSDK::JobDependency jobDep2("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep2);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep2));
    job.m_jobsToAnalyze.push_back(newDetails);

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);

    // in this test, though, we delete some after pushing them in there, and update it again:
    job.m_sourceFileDependencies.pop_back(); // erase the 'b' dependency.
    job.m_jobsToAnalyze[0].m_jobDependencyList.pop_back(); // erase the 'd' dependency, which is by guid.
    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);

    // now make sure that the same queries omit b and d:
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource, false);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob, false);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, false);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());
}


TEST_F(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid)
{
    // make sure that if we publish some dependencies, they do not appear if they are missing
    AZ::Uuid dummyBuilderUUID = AZ::Uuid::CreateRandom();
    QDir tempPath(m_tempDir.path());
    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    QString dependsOnFile1_Source = tempPath.absoluteFilePath("subfolder1/a.txt");
    QString dependsOnFile2_Source = tempPath.absoluteFilePath("subfolder1/b.txt");
    QString dependsOnFile1_Job = tempPath.absoluteFilePath("subfolder1/c.txt");
    QString dependsOnFile2_Job = tempPath.absoluteFilePath("subfolder1/d.txt");

    // in this case, we are only creating file b, and d (which are input by UUID)
    // and we will be missing a and c, which are input by name.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Job, QString("tempdata\n")));

    // construct the dummy job to feed to the database updater function:
    AssetProcessorManager::JobToProcessEntry job;
    job.m_sourceFileInfo.m_databasePath = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_pathRelativeToScanFolder = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_scanFolder = scanFolder;
    job.m_sourceFileInfo.m_uuid = AssetUtilities::CreateSafeSourceUUIDFromName(job.m_sourceFileInfo.m_databasePath.toUtf8().data());

    // note that we have to "prime" the map with the UUIDs to the source info for this to work:
    AZ::Uuid uuidOfB = AssetUtilities::CreateSafeSourceUUIDFromName("b.txt");
    AZ::Uuid uuidOfD = AssetUtilities::CreateSafeSourceUUIDFromName("d.txt");
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfB] = { watchFolderPath, "b.txt", "b.txt" };
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfD] = { watchFolderPath, "d.txt", "d.txt" };

    // each file we will take a different approach to publishing:  rel path, and UUID:
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "a.txt", AZ::Uuid::CreateNull() }));
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "", uuidOfB }));

    // it is currently assumed that the only fields that we care about in JobDetails is the builder busId and the job dependencies themselves:
    JobDetails newDetails;
    newDetails.m_assetBuilderDesc.m_busId = dummyBuilderUUID;

    AssetBuilderSDK::SourceFileDependency dep1 = { "c.txt", AZ::Uuid::CreateNull() };
    AssetBuilderSDK::JobDependency jobDep1("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep1);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep1));

    AssetBuilderSDK::SourceFileDependency dep2 = { "",uuidOfD };
    AssetBuilderSDK::JobDependency jobDep2("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep2);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep2));

    job.m_jobsToAnalyze.push_back(newDetails);

    // this is the one line that this unit test is really testing:
    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);

    // the rest of this test now performs a series of queries to verify the database was correctly set.
    // this indirectly verifies the QueryAbsolutePathDependenciesRecursive function also but it has its own dedicated tests, above.
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource, false);

    // we should find all of the deps, but not the placeholders.

    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Source.toUtf8().constData()), deps.end()); // b

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob, false);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Job.toUtf8().constData()), deps.end()); // d

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, false);
    // the above function includes the actual source, as an absolute path.
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Source.toUtf8().constData()), deps.end()); // b
    EXPECT_NE(deps.find(dependsOnFile2_Job.toUtf8().constData()), deps.end()); // d
}

TEST_F(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByName)
{
    // make sure that if we publish some dependencies, they do not appear if missing
    AZ::Uuid dummyBuilderUUID = AZ::Uuid::CreateRandom();
    QDir tempPath(m_tempDir.path());
    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    QString dependsOnFile1_Source = tempPath.absoluteFilePath("subfolder1/a.txt");
    QString dependsOnFile2_Source = tempPath.absoluteFilePath("subfolder1/b.txt");
    QString dependsOnFile1_Job = tempPath.absoluteFilePath("subfolder1/c.txt");
    QString dependsOnFile2_Job = tempPath.absoluteFilePath("subfolder1/d.txt");

    // in this case, we are only creating file a, and c, which are input by name
    // and we we will be making b and d missing, which are input by UUID.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Job, QString("tempdata\n")));

    // construct the dummy job to feed to the database updater function:
    AssetProcessorManager::JobToProcessEntry job;
    job.m_sourceFileInfo.m_databasePath = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_pathRelativeToScanFolder = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_scanFolder = scanFolder;
    job.m_sourceFileInfo.m_uuid = AssetUtilities::CreateSafeSourceUUIDFromName(job.m_sourceFileInfo.m_databasePath.toUtf8().data());

    // note that we have to "prime" the map with the UUIDs to the source info for this to work:
    AZ::Uuid uuidOfB = AssetUtilities::CreateSafeSourceUUIDFromName("b.txt");
    AZ::Uuid uuidOfD = AssetUtilities::CreateSafeSourceUUIDFromName("d.txt");

    // each file we will take a different approach to publishing:  rel path, and UUID:
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "a.txt", AZ::Uuid::CreateNull() }));
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "", uuidOfB }));

    // it is currently assumed that the only fields that we care about in JobDetails is the builder busId and the job dependencies themselves:
    JobDetails newDetails;
    newDetails.m_assetBuilderDesc.m_busId = dummyBuilderUUID;

    AssetBuilderSDK::SourceFileDependency dep1 = { "c.txt", AZ::Uuid::CreateNull() };
    AssetBuilderSDK::JobDependency jobDep1("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep1);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep1));

    AssetBuilderSDK::SourceFileDependency dep2 = { "",uuidOfD };
    AssetBuilderSDK::JobDependency jobDep2("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep2);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep2));

    job.m_jobsToAnalyze.push_back(newDetails);

    // this is the one line that this unit test is really testing:
    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);

    // the rest of this test now performs a series of queries to verify the database was correctly set.
    // this indirectly verifies the QueryAbsolutePathDependenciesRecursive function also but it has its own dedicated tests, above.
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource, false);

    // we should find all of the deps, but a and c are missing and thus should not appear.
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());   // a

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob, false);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());  // c

    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, false);
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());  // a
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());     // c
}


TEST_F(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid_UpdatesWhenTheyAppear)
{
    // this test makes sure that when files DO appear that were previously placeholders, the database is updated
    // so the strategy here is to  have files b, and d missing, which are declared as dependencies by UUID.
    // then, we make them re-appear later, and check that the database has updated them appropriately.

    AZ::Uuid dummyBuilderUUID = AZ::Uuid::CreateRandom();
    QDir tempPath(m_tempDir.path());
    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    QString dependsOnFile1_Source = tempPath.absoluteFilePath("subfolder1/a.txt");
    QString dependsOnFile2_Source = tempPath.absoluteFilePath("subfolder1/b.txt");
    QString dependsOnFile1_Job = tempPath.absoluteFilePath("subfolder1/c.txt");
    QString dependsOnFile2_Job = tempPath.absoluteFilePath("subfolder1/d.txt");

    // in this case, we are only creating file b, and d, which are addressed by UUID.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Job, QString("tempdata\n")));

    // construct the dummy job to feed to the database updater function:
    AssetProcessorManager::JobToProcessEntry job;
    job.m_sourceFileInfo.m_databasePath = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_pathRelativeToScanFolder = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_scanFolder = scanFolder;
    job.m_sourceFileInfo.m_uuid = AssetUtilities::CreateSafeSourceUUIDFromName(job.m_sourceFileInfo.m_databasePath.toUtf8().data());

    AZ::Uuid uuidOfD = AssetUtilities::CreateSafeSourceUUIDFromName("d.txt");
    AZ::Uuid uuidOfB = AssetUtilities::CreateSafeSourceUUIDFromName("b.txt");

    // each file we will take a different approach to publishing:  rel path, and UUID:
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "a.txt", AZ::Uuid::CreateNull() }));
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "", uuidOfB }));

    // it is currently assumed that the only fields that we care about in JobDetails is the builder busId and the job dependencies themselves:
    JobDetails newDetails;
    newDetails.m_assetBuilderDesc.m_busId = dummyBuilderUUID;

    AssetBuilderSDK::SourceFileDependency dep1 = { "c.txt", AZ::Uuid::CreateNull() };
    AssetBuilderSDK::JobDependency jobDep1("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep1);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep1));

    AssetBuilderSDK::SourceFileDependency dep2 = { "",uuidOfD };
    AssetBuilderSDK::JobDependency jobDep2("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep2);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep2));

    job.m_jobsToAnalyze.push_back(newDetails);

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);
    // so at this point, the database should be in the same state as after the UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid test
    // which was already verified, by that test.

    // now that the database has placeholders, we expect them to resolve themselves when we provide the actual files:
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Source, QString("tempdata\n")));
    // now that B exists, we pretend a job came in to process B. (it doesn't require dependencies to be declared)
    // note that we have to "prime" the map with the UUIDs to the source info for this to work:
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfB] = { watchFolderPath, "b.txt", "b.txt" };

    AssetProcessorManager::JobToProcessEntry job2;
    job2.m_sourceFileInfo.m_databasePath = "b.txt";
    job2.m_sourceFileInfo.m_pathRelativeToScanFolder = "b.txt";
    job2.m_sourceFileInfo.m_scanFolder = scanFolder;
    job2.m_sourceFileInfo.m_uuid = uuidOfB;

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job2);

    // b should no longer be a placeholder, so both A and B should be present as their actual path.
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource, false);
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());   // a
    EXPECT_NE(deps.find(dependsOnFile2_Source.toUtf8().constData()), deps.end());   // b

    // but d should still be a placeholder, since we have not declared it yet.
    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob, false);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());  // c

    // in addition, we expect to have the original file that depends on B appear in the analysis queue, since something it depends on appeared:
    QString normalizedSourcePath = AssetUtilities::NormalizeFilePath(absPath);
    EXPECT_TRUE(m_assetProcessorManager->m_alreadyActiveFiles.contains(normalizedSourcePath));

    // now make d exist too and pretend a job came in to process it:
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Job, QString("tempdata\n"))); // create file D
    AssetProcessorManager::JobToProcessEntry job3;
    job3.m_sourceFileInfo.m_databasePath = "d.txt";
    job3.m_sourceFileInfo.m_pathRelativeToScanFolder = "d.txt";
    job3.m_sourceFileInfo.m_scanFolder = scanFolder;
    job3.m_sourceFileInfo.m_uuid = uuidOfD;
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfD] = { watchFolderPath, "d.txt", "d.txt" };

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job3);

    // all files should now be present:
    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, false);
    EXPECT_EQ(deps.size(), 5);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Job.toUtf8().constData()), deps.end());
}


TEST_F(AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByName_UpdatesWhenTheyAppear)
{
    // this test makes sure that when files DO appear that were previously placeholders, the database is updated
    // so the strategy here is to  have files a, and c missing, which are declared as dependencies by name.
    // then, we make them re-appear later, and check that the database has updated them appropriately.

    AZ::Uuid dummyBuilderUUID = AZ::Uuid::CreateRandom();
    QDir tempPath(m_tempDir.path());
    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    QString dependsOnFile1_Source = tempPath.absoluteFilePath("subfolder1/a.txt");
    QString dependsOnFile2_Source = tempPath.absoluteFilePath("subfolder1/b.txt");
    QString dependsOnFile1_Job = tempPath.absoluteFilePath("subfolder1/c.txt");
    QString dependsOnFile2_Job = tempPath.absoluteFilePath("subfolder1/d.txt");

    // in this case, we are only creating file b, and d, which are addressed by UUID.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile2_Job, QString("tempdata\n")));

    // construct the dummy job to feed to the database updater function:
    AssetProcessorManager::JobToProcessEntry job;
    job.m_sourceFileInfo.m_databasePath = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_pathRelativeToScanFolder = "assetProcessorManagerTest.txt";
    job.m_sourceFileInfo.m_scanFolder = scanFolder;
    job.m_sourceFileInfo.m_uuid = AssetUtilities::CreateSafeSourceUUIDFromName(job.m_sourceFileInfo.m_databasePath.toUtf8().data());

    // note that we have to "prime" the map with the UUIDs to the source info for this to work:
    AZ::Uuid uuidOfB = AssetUtilities::CreateSafeSourceUUIDFromName("b.txt");
    AZ::Uuid uuidOfD = AssetUtilities::CreateSafeSourceUUIDFromName("d.txt");
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfB] = { watchFolderPath, "b.txt", "b.txt" };
    m_assetProcessorManager->m_sourceUUIDToSourceInfoMap[uuidOfD] = { watchFolderPath, "d.txt", "d.txt" };

    // each file we will take a different approach to publishing:  rel path, and UUID:
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "a.txt", AZ::Uuid::CreateNull() }));
    job.m_sourceFileDependencies.push_back(AZStd::make_pair<AZ::Uuid, AssetBuilderSDK::SourceFileDependency>(dummyBuilderUUID, { "", uuidOfB }));

    // it is currently assumed that the only fields that we care about in JobDetails is the builder busId and the job dependencies themselves:
    JobDetails newDetails;
    newDetails.m_assetBuilderDesc.m_busId = dummyBuilderUUID;

    AssetBuilderSDK::SourceFileDependency dep1 = { "c.txt", AZ::Uuid::CreateNull() };
    AssetBuilderSDK::JobDependency jobDep1("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep1);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep1));

    AssetBuilderSDK::SourceFileDependency dep2 = { "",uuidOfD };
    AssetBuilderSDK::JobDependency jobDep2("pc build", "pc", AssetBuilderSDK::JobDependencyType::Order, dep2);
    newDetails.m_jobDependencyList.push_back(JobDependencyInternal(jobDep2));

    job.m_jobsToAnalyze.push_back(newDetails);

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job);
    // so at this point, the database should be in the same state as after the UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid test
    // which was already verified, by that test.

    // now that the database has placeholders, we expect them to resolve themselves when we provide the actual files:
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Source, QString("tempdata\n")));
    // now that A exists, we pretend a job came in to process a. (it doesn't require dependencies to be declared)
    AZ::Uuid uuidOfA = AssetUtilities::CreateSafeSourceUUIDFromName("a.txt");
    AssetProcessorManager::JobToProcessEntry job2;
    job2.m_sourceFileInfo.m_databasePath = "a.txt";
    job2.m_sourceFileInfo.m_pathRelativeToScanFolder = "a.txt";
    job2.m_sourceFileInfo.m_scanFolder = scanFolder;
    job2.m_sourceFileInfo.m_uuid = uuidOfA;
    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job2);

    // a should no longer be a placeholder
    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource, false);
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());   // a
    EXPECT_NE(deps.find(dependsOnFile2_Source.toUtf8().constData()), deps.end());   // b
    deps.clear();

    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob, false);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Job.toUtf8().constData()), deps.end());  // d

    // in addition, we expect to have the original file that depends on A appear in the analysis queue, since something it depends on appeared:
    QString normalizedSourcePath = AssetUtilities::NormalizeFilePath(absPath);
    EXPECT_TRUE(m_assetProcessorManager->m_alreadyActiveFiles.contains(normalizedSourcePath));

    // now make c exist too and pretend a job came in to process it:
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFile1_Job, QString("tempdata\n")));
    AZ::Uuid uuidOfC = AssetUtilities::CreateSafeSourceUUIDFromName("c.txt");
    AssetProcessorManager::JobToProcessEntry job3;
    job3.m_sourceFileInfo.m_databasePath = "c.txt";
    job3.m_sourceFileInfo.m_pathRelativeToScanFolder = "c.txt";
    job3.m_sourceFileInfo.m_scanFolder = scanFolder;
    job3.m_sourceFileInfo.m_uuid = uuidOfC;

    m_assetProcessorManager.get()->UpdateSourceFileDependenciesDatabase(job3);

    // all files should now be present:
    deps.clear();
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("assetProcessorManagerTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, false);
    EXPECT_EQ(deps.size(), 5);
    EXPECT_NE(deps.find(absPath.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Source.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile1_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFile2_Job.toUtf8().constData()), deps.end());
}

TEST_F(AssetProcessorManagerTest, JobDependencyOrderOnce_MultipleJobs_EmitOK)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QDir tempPath(m_tempDir.path());
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);
    const char relSourceFileName[] = "a.dummy";
    const char secondRelSourceFile[] = "b.dummy";
    QString sourceFileName = tempPath.absoluteFilePath("subfolder1/a.dummy");
    QString secondSourceFile = tempPath.absoluteFilePath("subfolder1/b.dummy");
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
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_databaseSourceName, secondRelSourceFile);
    EXPECT_EQ(jobDetails[1].m_jobEntry.m_databaseSourceName, relSourceFileName);
    EXPECT_EQ(jobDetails[1].m_jobDependencyList.size(), 1); // there should only be one job dependency
    EXPECT_EQ(jobDetails[1].m_jobDependencyList[0].m_jobDependency.m_sourceFile.m_sourceFileDependencyPath, secondRelSourceFile); // there should only be one job dependency

    // Process jobs in APM
    QDir destination(jobDetails[0].m_destinationPath);
    QString productAFileName = destination.absoluteFilePath("aoutput.txt");
    QString productBFileName = destination.absoluteFilePath("boutput.txt");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(productBFileName, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(productAFileName, QString("tempdata\n")));

    AssetBuilderSDK::ProcessJobResponse responseB;
    responseB.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    responseB.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productBFileName.toUtf8().constData(), AZ::Uuid::CreateNull(), 1));

    AssetBuilderSDK::ProcessJobResponse responseA;
    responseA.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    responseA.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productAFileName.toUtf8().constData(), AZ::Uuid::CreateNull(), 1));

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
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_databaseSourceName, secondRelSourceFile);

    jobDetails.clear();
    m_isIdling = false;
    // Modify source file a.dummy, we should only see one job with source file a.dummy getting processed in this case.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(sourceFileName, QString("temp\n")));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFileName));
    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(jobDetails.size(), 1);
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_databaseSourceName, relSourceFileName);
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
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_databaseSourceName, secondRelSourceFile);

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
    EXPECT_EQ(jobDetails[0].m_jobEntry.m_databaseSourceName, secondRelSourceFile);
    EXPECT_EQ(jobDetails[1].m_jobEntry.m_databaseSourceName, relSourceFileName);
    EXPECT_EQ(jobDetails[1].m_jobDependencyList.size(), 1); // there should only be one job dependency
    EXPECT_EQ(jobDetails[1].m_jobDependencyList[0].m_jobDependency.m_sourceFile.m_sourceFileDependencyPath, secondRelSourceFile); // there should only be one job dependency
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

    QDir tempPath(m_tempDir.path());
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    QString folderPath(tempPath.absoluteFilePath("subfolder1/Test\xD0"));
    QDir folderPathDir(folderPath);
    QString absPath(folderPathDir.absoluteFilePath("Test.txt"));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(absPath, QString("test\n")));

    m_assetProcessorManager.get()->AssessAddedFile(absPath);

    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(failedjobDetails.m_autoFail, true);
    QDir dir(failedjobDetails.m_jobEntry.m_watchFolderPath);
    EXPECT_EQ(dir.absoluteFilePath(failedjobDetails.m_jobEntry.m_pathRelativeToWatchFolder), absPath);

    // folder delete notification
    folderPathDir.removeRecursively();
    m_assetProcessorManager.get()->AssessDeletedFile(folderPath);
    ASSERT_TRUE(BlockUntilIdle(5000));
    EXPECT_EQ(deletedFolderPath, "Test\xD0");
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

    QDir tempPath(m_tempDir.path());

    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(tempPath.absoluteFilePath("subfolder1"));
    ASSERT_NE(scanFolder, nullptr);

    QString absPath = tempPath.absoluteFilePath("subfolder1/test.txt");
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(absPath, QString("test\n")));

    //////////////////////////////////////////////////////////////////////////

    // Add a file and signal a successful process event

    m_assetProcessorManager.get()->AssessAddedFile(absPath);
    ASSERT_TRUE(BlockUntilIdle(idleWaitTime));

    for(const auto& processResult : processResults)
    {
        auto file = QDir(processResult.m_destinationPath).absoluteFilePath(processResult.m_jobEntry.m_databaseSourceName + ".arc1");

        // Create the file on disk
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(file, "products."));

        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(file.toUtf8().constData(), AZ::Uuid::CreateNull(), 1));

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

void ModtimeScanningTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();

    m_data = AZStd::make_unique<StaticData>();

    // We don't want the mock application manager to provide builder descriptors, mockBuilderInfoHandler will provide our own
    m_mockApplicationManager->BusDisconnect();

    m_data->m_mockBuilderInfoHandler.m_builderDesc = m_data->m_mockBuilderInfoHandler.CreateBuilderDesc("test builder", "{DF09DDC0-FD22-43B6-9E22-22C8574A6E1E}", { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) });
    m_data->m_mockBuilderInfoHandler.BusConnect();

    ASSERT_TRUE(m_mockApplicationManager->GetBuilderByID("txt files", m_data->m_builderTxtBuilder));

    // Run this twice so the test builder doesn't get counted as a "new" builder and bypass the modtime skipping
    m_assetProcessorManager->ComputeBuilderDirty();
    m_assetProcessorManager->ComputeBuilderDirty();

    auto assetConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [this](JobDetails details)
    {
        m_data->m_processResults.push_back(AZStd::move(details));
    });

    auto deletedConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::SourceDeleted, [this](QString file)
    {
        m_data->m_deletedSources.push_back(file);
    });

    // Create the test file
    const auto& scanFolder = m_config->GetScanFolderAt(0);
    m_data->m_relativePathFromWatchFolder[0] = "modtimeTestFile.txt";
    m_data->m_absolutePath.push_back(QDir(scanFolder.ScanPath()).absoluteFilePath(m_data->m_relativePathFromWatchFolder[0]));

    m_data->m_relativePathFromWatchFolder[1] = "modtimeTestDependency.txt";
    m_data->m_absolutePath.push_back(QDir(scanFolder.ScanPath()).absoluteFilePath(m_data->m_relativePathFromWatchFolder[1]));

    m_data->m_relativePathFromWatchFolder[2] = "modtimeTestDependency.txt.assetinfo";
    m_data->m_absolutePath.push_back(QDir(scanFolder.ScanPath()).absoluteFilePath(m_data->m_relativePathFromWatchFolder[2]));

    for (const auto& path : m_data->m_absolutePath)
    {
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(path, ""));
    }

    m_data->m_mockBuilderInfoHandler.m_dependencyFilePath = m_data->m_absolutePath[1].toUtf8().data();

    // Add file to database with no modtime
    {
        AssetDatabaseConnection connection;
        ASSERT_TRUE(connection.OpenDatabase());
        AzToolsFramework::AssetDatabase::FileDatabaseEntry fileEntry;
        fileEntry.m_fileName = m_data->m_relativePathFromWatchFolder[0].toUtf8().data();
        fileEntry.m_modTime = 0;
        fileEntry.m_isFolder = false;
        fileEntry.m_scanFolderPK = scanFolder.ScanFolderID();

        bool entryAlreadyExists;
        ASSERT_TRUE(connection.InsertFile(fileEntry, entryAlreadyExists));
        ASSERT_FALSE(entryAlreadyExists);

        fileEntry.m_fileID = AzToolsFramework::AssetDatabase::InvalidEntryId; // Reset the id so we make a new entry
        fileEntry.m_fileName = m_data->m_relativePathFromWatchFolder[1].toUtf8().data();
        ASSERT_TRUE(connection.InsertFile(fileEntry, entryAlreadyExists));
        ASSERT_FALSE(entryAlreadyExists);

        fileEntry.m_fileID = AzToolsFramework::AssetDatabase::InvalidEntryId; // Reset the id so we make a new entry
        fileEntry.m_fileName = m_data->m_relativePathFromWatchFolder[2].toUtf8().data();
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
        auto file = QDir(processResult.m_destinationPath).absoluteFilePath(processResult.m_jobEntry.m_databaseSourceName.toLower() + ".arc1");
        m_data->m_productPaths.emplace(
            QDir(processResult.m_jobEntry.m_watchFolderPath)
                .absoluteFilePath(processResult.m_jobEntry.m_databaseSourceName)
                .toUtf8()
                .constData(),
            file);

        // Create the file on disk
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(file, "products."));

        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(file.toUtf8().constData(), AZ::Uuid::CreateNull(), 1));

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResult.m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
    }

    ASSERT_TRUE(BlockUntilIdle(5000));

    m_isIdling = false;
}

void ModtimeScanningTest::SimulateAssetScanner(QSet<AssetFileInfo> filePaths)
{
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "OnAssetScannerStatusChange", Qt::QueuedConnection, Q_ARG(AssetProcessor::AssetScanningStatus, AssetProcessor::AssetScanningStatus::Started));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessFilesFromScanner", Qt::QueuedConnection, Q_ARG(QSet<AssetFileInfo>, filePaths));
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "OnAssetScannerStatusChange", Qt::QueuedConnection, Q_ARG(AssetProcessor::AssetScanningStatus, AssetProcessor::AssetScanningStatus::Completed));
}

QSet<AssetFileInfo> ModtimeScanningTest::BuildFileSet()
{
    QSet<AssetFileInfo> filePaths;

    for (const auto& path : m_data->m_absolutePath)
    {
        QFileInfo fileInfo(path);
        auto modtime = fileInfo.lastModified();
        AZ::u64 fileSize = fileInfo.size();
        filePaths.insert(AssetFileInfo(path, modtime, fileSize, m_config->GetScanFolderForFile(path), false));
    }

    return filePaths;
}

void ModtimeScanningTest::ExpectWork(int createJobs, int processJobs)
{
    ASSERT_TRUE(BlockUntilIdle(5000));

    EXPECT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, createJobs);
    EXPECT_EQ(m_data->m_processResults.size(), processJobs);
    EXPECT_FALSE(m_data->m_processResults[0].m_autoFail);
    EXPECT_FALSE(m_data->m_processResults[1].m_autoFail);
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
    m_assetProcessorManager->m_allowModtimeSkippingFeature = false;

    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    // 2 create jobs but 0 process jobs because the file has already been processed before in SetUp
    ExpectWork(2, 0);
}

TEST_F(ModtimeScanningTest, ModtimeSkipping_FileUnchanged)
{
    using namespace AzToolsFramework::AssetSystem;

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
    AssetUtilities::SetUseFileHashOverride(true, true);

    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    ExpectNoWork();
}

TEST_F(ModtimeScanningTest, ModtimeSkipping_EnablePlatform_ShouldProcessFilesForPlatform)
{
    using namespace AzToolsFramework::AssetSystem;

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
    AssetUtilities::SetUseFileHashOverride(true, true);

    // Enable android platform after the initial SetUp has already processed the files for pc
    QDir tempPath(m_tempDir.path());
    AssetBuilderSDK::PlatformInfo androidPlatform("android", { "host", "renderer" });
    m_config->EnablePlatform(androidPlatform, true);

    // There's no way to remove scanfolders and adding a new one after enabling the platform will cause the pc assets to build as well, which we don't want
    // Instead we'll just const cast the vector and modify the enabled platforms for the scanfolder
    auto& platforms = const_cast<AZStd::vector<AssetBuilderSDK::PlatformInfo>&>(m_config->GetScanFolderAt(0).GetPlatforms());
    platforms.push_back(androidPlatform);

    // We need the builder fingerprints to be updated to reflect the newly enabled platform
    m_assetProcessorManager->ComputeBuilderDirty();

    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    ExpectWork(4, 2); // CreateJobs = 4, 2 files * 2 platforms.  ProcessJobs = 2, just the android platform jobs (pc is already processed)

    ASSERT_TRUE(m_data->m_processResults[0].m_destinationPath.contains("android"));
    ASSERT_TRUE(m_data->m_processResults[1].m_destinationPath.contains("android"));
}

TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyTimestamp)
{
    // Update the timestamp on a file without changing its contents
    // This should not cause any job to run since the hash of the file is the same before/after
    // Additionally, the timestamp stored in the database should be updated
    using namespace AzToolsFramework::AssetSystem;

    uint64_t timestamp = 1594923423;

    QString databaseName, scanfolderName;
    m_config->ConvertToRelativePath(m_data->m_absolutePath[1], databaseName, scanfolderName);
    auto* scanFolder = m_config->GetScanFolderForFile(m_data->m_absolutePath[1]);

    AzToolsFramework::AssetDatabase::FileDatabaseEntry fileEntry;

    m_assetProcessorManager.get()->m_stateData->GetFileByFileNameAndScanFolderId(databaseName, scanFolder->ScanFolderID(), fileEntry);

    ASSERT_NE(fileEntry.m_modTime, timestamp);
    uint64_t existingTimestamp = fileEntry.m_modTime;

    // Modify the timestamp on just one file
    AzToolsFramework::ToolsFileUtils::SetModificationTime(m_data->m_absolutePath[1].toUtf8().data(), timestamp);

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
    AssetUtilities::SetUseFileHashOverride(true, true);

    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    ExpectNoWork();

    m_assetProcessorManager.get()->m_stateData->GetFileByFileNameAndScanFolderId(databaseName, scanFolder->ScanFolderID(), fileEntry);

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
    AzToolsFramework::ToolsFileUtils::SetModificationTime(m_data->m_absolutePath[1].toUtf8().data(), timestamp);

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
    AssetUtilities::SetUseFileHashOverride(true, false);

    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    ExpectWork(2, 2);
}

TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyFile)
{
    using namespace AzToolsFramework::AssetSystem;

    SetFileContents(m_data->m_absolutePath[1].toUtf8().constData(), "hello world");

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
    AssetUtilities::SetUseFileHashOverride(true, true);

    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    // Even though we're only updating one file, we're expecting 2 createJob calls because our test file is a dependency that triggers the other test file to process as well
    ExpectWork(2, 2);
}

TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyFile_AndThenRevert_ProcessesAgain)
{
    using namespace AzToolsFramework::AssetSystem;
    auto theFile = m_data->m_absolutePath[1].toUtf8();
    const char* theFileString = theFile.constData();

    SetFileContents(theFileString, "hello world");

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
    AssetUtilities::SetUseFileHashOverride(true, true);

    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    // Even though we're only updating one file, we're expecting 2 createJob calls because our test file is a dependency that triggers the other test file to process as well
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

struct LockedFileTest
    : ModtimeScanningTest
    , AssetProcessor::ConnectionBus::Handler
{
    MOCK_METHOD3(SendRaw, size_t (unsigned, unsigned, const QByteArray&));
    MOCK_METHOD3(SendPerPlatform, size_t (unsigned, const AzFramework::AssetSystem::BaseAssetProcessorMessage&, const QString&));
    MOCK_METHOD4(SendRawPerPlatform, size_t (unsigned, unsigned, const QByteArray&, const QString&));
    MOCK_METHOD2(SendRequest, unsigned (const AzFramework::AssetSystem::BaseAssetProcessorMessage&, const ResponseCallback&));
    MOCK_METHOD2(SendResponse, size_t (unsigned, const AzFramework::AssetSystem::BaseAssetProcessorMessage&));
    MOCK_METHOD1(RemoveResponseHandler, void (unsigned));

    size_t Send(unsigned, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) override
    {
        using SourceFileNotificationMessage = AzToolsFramework::AssetSystem::SourceFileNotificationMessage;
        switch (message.GetMessageType())
        {
        case SourceFileNotificationMessage::MessageType:
            if (const auto sourceFileMessage = azrtti_cast<const SourceFileNotificationMessage*>(&message);
                sourceFileMessage != nullptr && sourceFileMessage->m_type == SourceFileNotificationMessage::NotificationType::FileRemoved
                && m_callback)
            {
                m_callback();
            }
            break;
        default:
            break;
        }

        return 0;
    }

    void SetUp() override
    {
        ModtimeScanningTest::SetUp();

        ConnectionBus::Handler::BusConnect(0);
    }

    void TearDown() override
    {
        ConnectionBus::Handler::BusDisconnect();

        ModtimeScanningTest::TearDown();
    }

    AZStd::function<void()> m_callback;
};

TEST_F(LockedFileTest, DeleteFile_LockedProduct_DeleteFails)
{
    auto theFile = m_data->m_absolutePath[1].toUtf8();
    const char* theFileString = theFile.constData();
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
    auto theFile = m_data->m_absolutePath[1].toUtf8();
    const char* theFileString = theFile.constData();
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
        AZStd::thread workerThread;

        m_callback = [&product, &workerThread]() {
            workerThread = AZStd::thread([&product]() {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(60));
                product.close();
            });
        };

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, QString(theFileString)));

        EXPECT_TRUE(BlockUntilIdle(5000));

        EXPECT_FALSE(QFile::exists(productPath));
        EXPECT_EQ(m_data->m_deletedSources.size(), 1);

        workerThread.join();
    }
    else
    {
        SUCCEED() << "Skipping test.  OS does not lock open files.";
    }
}

TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyFilesSameHash_BothProcess)
{
    using namespace AzToolsFramework::AssetSystem;

    SetFileContents(m_data->m_absolutePath[1].toUtf8().constData(), "hello world");

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
    AssetUtilities::SetUseFileHashOverride(true, true);

    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    // Even though we're only updating one file, we're expecting 2 createJob calls because our test file is a dependency that triggers the other test file to process as well
    ExpectWork(2, 2);
    ProcessAssetJobs();

    m_data->m_mockBuilderInfoHandler.m_createJobsCount = 0;
    m_data->m_processResults.clear();
    m_data->m_deletedSources.clear();

    // Make file 0 have the same contents as file 1
    SetFileContents(m_data->m_absolutePath[0].toUtf8().constData(), "hello world");

    filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    ExpectWork(1, 1);
}

TEST_F(ModtimeScanningTest, ModtimeSkipping_ModifyMetadataFile)
{
    using namespace AzToolsFramework::AssetSystem;

    SetFileContents(m_data->m_absolutePath[2].toUtf8().constData(), "hello world");

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
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

    // Enable the features we're testing
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;
    AssetUtilities::SetUseFileHashOverride(true, true);

    ASSERT_TRUE(QFile::remove(m_data->m_absolutePath[0]));

    // Feed in ONLY one file (the one we didn't delete)
    QSet<AssetFileInfo> filePaths;
    QFileInfo fileInfo(m_data->m_absolutePath[1]);
    auto modtime = fileInfo.lastModified();
    AZ::u64 fileSize = fileInfo.size();
    filePaths.insert(AssetFileInfo(m_data->m_absolutePath[1], modtime, fileSize, &m_config->GetScanFolderAt(0), false));

    SimulateAssetScanner(filePaths);

    QElapsedTimer timer;
    timer.start();

    do
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    } while (m_data->m_deletedSources.size() < m_data->m_relativePathFromWatchFolder[0].size() && timer.elapsed() < 5000);

    ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 0);
    ASSERT_EQ(m_data->m_processResults.size(), 0);
    ASSERT_THAT(m_data->m_deletedSources, testing::ElementsAre(m_data->m_relativePathFromWatchFolder[0]));
}

TEST_F(ModtimeScanningTest, ReprocessRequest_FileNotModified_FileProcessed)
{
    using namespace AzToolsFramework::AssetSystem;

    m_assetProcessorManager->RequestReprocess(m_data->m_absolutePath[0]);

    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 1);
    ASSERT_EQ(m_data->m_processResults.size(), 1);
}

TEST_F(ModtimeScanningTest, ReprocessRequest_SourceWithDependency_BothWillProcess)
{
    using namespace AzToolsFramework::AssetSystem;

    using SourceFileDependencyEntry = AzToolsFramework::AssetDatabase::SourceFileDependencyEntry;

    SourceFileDependencyEntry newEntry1;
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_source = m_data->m_absolutePath[0].toUtf8().constData();
    newEntry1.m_dependsOnSource = m_data->m_absolutePath[1].toUtf8().constData();
    newEntry1.m_typeOfDependency = SourceFileDependencyEntry::DEP_SourceToSource;

    m_assetProcessorManager->RequestReprocess(m_data->m_absolutePath[0]);
    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 1);
    ASSERT_EQ(m_data->m_processResults.size(), 1);

    m_assetProcessorManager->RequestReprocess(m_data->m_absolutePath[1]);
    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 3);
    ASSERT_EQ(m_data->m_processResults.size(), 3);
}

TEST_F(ModtimeScanningTest, ReprocessRequest_RequestFolder_SourceAssetsWillProcess)
{
    using namespace AzToolsFramework::AssetSystem;

    const auto& scanFolder = m_config->GetScanFolderAt(0);

    QString scanPath = scanFolder.ScanPath();
    m_assetProcessorManager->RequestReprocess(scanPath);
    ASSERT_TRUE(BlockUntilIdle(5000));

    // two text files are source assets, assetinfo is not
    ASSERT_EQ(m_data->m_mockBuilderInfoHandler.m_createJobsCount, 2);
    ASSERT_EQ(m_data->m_processResults.size(), 2);
}

//////////////////////////////////////////////////////////////////////////

MockBuilderInfoHandler::~MockBuilderInfoHandler()
{
    BusDisconnect();
    m_builderDesc = {};
}

void MockBuilderInfoHandler::GetMatchingBuildersInfo([[maybe_unused]] const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
{
    builderInfoList.push_back(m_builderDesc);
}

void MockBuilderInfoHandler::GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList)
{
    builderInfoList.push_back(m_builderDesc);
}

void MockBuilderInfoHandler::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
{
    response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

    for (const auto& platform : request.m_enabledPlatforms)
    {
        AssetBuilderSDK::JobDescriptor jobDescriptor;
        jobDescriptor.m_priority = 0;
        jobDescriptor.m_critical = true;
        jobDescriptor.m_jobKey = "Mock Job";
        jobDescriptor.SetPlatformIdentifier(platform.m_identifier.c_str());
        jobDescriptor.m_additionalFingerprintInfo = m_jobFingerprint.toUtf8().data();

        if (!m_jobDependencyFilePath.isEmpty())
        {
            jobDescriptor.m_jobDependencyList.push_back(AssetBuilderSDK::JobDependency("Mock Job", "pc", AssetBuilderSDK::JobDependencyType::Order,
                AssetBuilderSDK::SourceFileDependency(m_jobDependencyFilePath.toUtf8().constData(), AZ::Uuid::CreateNull())));
        }

        if (!m_dependencyFilePath.isEmpty())
        {
            response.m_sourceFileDependencyList.push_back(AssetBuilderSDK::SourceFileDependency(m_dependencyFilePath.toUtf8().data(), AZ::Uuid::CreateNull()));
        }
        response.m_createJobOutputs.push_back(jobDescriptor);
        m_createJobsCount++;
    }
}

void MockBuilderInfoHandler::ProcessJob([[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
{
    response.m_resultCode = AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success;
}

AssetBuilderSDK::AssetBuilderDesc MockBuilderInfoHandler::CreateBuilderDesc(const QString& builderName, const QString& builderId, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns)
{
    AssetBuilderSDK::AssetBuilderDesc builderDesc;

    builderDesc.m_name = builderName.toUtf8().data();
    builderDesc.m_patterns = builderPatterns;
    builderDesc.m_busId = AZ::Uuid::CreateString(builderId.toUtf8().data());
    builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;
    builderDesc.m_createJobFunction = AZStd::bind(&MockBuilderInfoHandler::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
    builderDesc.m_processJobFunction = AZStd::bind(&MockBuilderInfoHandler::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
    return builderDesc;
}

void FingerprintTest::SetUp()
{
    AZ_Printf("FingerprintTest", "SetUp start\n");
    AssetProcessorManagerTest::SetUp();
    AZ_Printf("FingerprintTest", "SetUp self\n");

    // We don't want the mock application manager to provide builder descriptors, mockBuilderInfoHandler will provide our own
    m_mockApplicationManager->BusDisconnect();

    m_mockBuilderInfoHandler.m_builderDesc = m_mockBuilderInfoHandler.CreateBuilderDesc("test builder", "{DF09DDC0-FD22-43B6-9E22-22C8574A6E1E}", { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) });
    m_mockBuilderInfoHandler.BusConnect();

    // Create the test file
    const auto& scanFolder = m_config->GetScanFolderAt(0);
    QString relativePathFromWatchFolder("fingerprintTest.txt");
    m_absolutePath = QDir(scanFolder.ScanPath()).absoluteFilePath(relativePathFromWatchFolder);

    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [this](JobDetails jobDetails)
    {
        m_jobResults.push_back(jobDetails);
    });

    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(m_absolutePath, ""));
    AZ_Printf("FingerprintTest", "SetUp end\n");
}

void FingerprintTest::TearDown()
{
    AZ_Printf("FingerprintTest", "TearDown start\n");
    m_jobResults = AZStd::vector<AssetProcessor::JobDetails>{};
    m_mockBuilderInfoHandler = {};

    AZ_Printf("FingerprintTest", "TearDown parent\n");
    AssetProcessorManagerTest::TearDown();
    AZ_Printf("FingerprintTest", "TearDown end\n");
}

void FingerprintTest::RunFingerprintTest(QString builderFingerprint, QString jobFingerprint, bool expectedResult)
{
    AZ_Printf("FingerprintTest", "Fingerprint Test Start\n");
    m_mockBuilderInfoHandler.m_builderDesc.m_analysisFingerprint = builderFingerprint.toUtf8().data();
    m_mockBuilderInfoHandler.m_jobFingerprint = jobFingerprint;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, m_absolutePath));

    ASSERT_TRUE(BlockUntilIdle(5000));
    ASSERT_EQ(m_mockBuilderInfoHandler.m_createJobsCount, 1);
    ASSERT_EQ(m_jobResults.size(), 1);
    ASSERT_EQ(m_jobResults[0].m_autoFail, expectedResult);
    AZ_Printf("FingerprintTest", "Fingerprint Test End\n");
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
    QDir tempPath(m_tempDir.path());
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/wildcardTest.txt"));
    QString relFileName("wildcardTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/wildcardTest.txt"));
    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
    const ScanFolderInfo* scanFolder = m_config->GetScanFolderByPath(watchFolderPath);
    ASSERT_NE(scanFolder, nullptr);

    // the above file (assetProcessorManagerTest.txt) will depend on these four files:
    QString dependsOnFilea_Source = tempPath.absoluteFilePath("subfolder1/a.txt");
    QString dependsOnFileb_Source = tempPath.absoluteFilePath("subfolder1/b.txt");
    QString dependsOnFileb1_Source = tempPath.absoluteFilePath("subfolder1/b1.txt");
    QString dependsOnFilec_Job = tempPath.absoluteFilePath("subfolder1/c.txt");
    QString dependsOnFilec1_Job = tempPath.absoluteFilePath("subfolder1/c1.txt");
    QString dependsOnFiled_Job = tempPath.absoluteFilePath("subfolder1/d.txt");

    // in this case, we are only creating file b, and d, which are addressed by UUID.
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFileb_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFilec_Job, QString("tempdata\n")));

    // construct the dummy job to feed to the database updater function:
    AssetProcessorManager::JobToProcessEntry job;
    job.m_sourceFileInfo.m_databasePath = "wildcardTest.txt";
    job.m_sourceFileInfo.m_pathRelativeToScanFolder = "wildcardTest.txt";
    job.m_sourceFileInfo.m_scanFolder = scanFolder;
    job.m_sourceFileInfo.m_uuid = AssetUtilities::CreateSafeSourceUUIDFromName(job.m_sourceFileInfo.m_databasePath.toUtf8().data());

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

    AssetProcessor::SourceFilesForFingerprintingContainer deps;
    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("wildcardTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceToSource, false);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(dependsOnFileb_Source.toUtf8().constData()), deps.end());
    deps.clear();

    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("wildcardTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_JobToJob, false);
    EXPECT_EQ(deps.size(), 2);
    EXPECT_NE(deps.find(dependsOnFilec_Job.toUtf8().constData()), deps.end());
    deps.clear();

    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("wildcardTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceOrJob, false);
    EXPECT_EQ(deps.size(), 3);
    EXPECT_NE(deps.find(dependsOnFilec_Job.toUtf8().constData()), deps.end());
    EXPECT_NE(deps.find(dependsOnFileb_Source.toUtf8().constData()), deps.end());
    deps.clear();

    m_assetProcessorManager.get()->QueryAbsolutePathDependenciesRecursive(QString::fromUtf8("wildcardTest.txt"), deps, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, false);
    EXPECT_EQ(deps.size(), 1);
    deps.clear();

    AZStd::vector<AZStd::string> wildcardDeps;
    auto callbackFunction = [&wildcardDeps](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry)
    {
        wildcardDeps.push_back(entry.m_dependsOnSource.c_str());
        return true;
    };

    m_assetProcessorManager.get()->m_stateData->QueryDependsOnSourceBySourceDependency("wildcardTest.txt", nullptr, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, callbackFunction);
    EXPECT_EQ(wildcardDeps.size(), 2);

    // The database should have the wildcard record and the individual dependency on b and c at this point, now we add new files
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFileb1_Source, QString("tempdata\n")));
    ASSERT_TRUE(UnitTestUtils::CreateDummyFile(dependsOnFilec1_Job, QString("tempdata\n")));

    QStringList dependList;
    dependList = m_assetProcessorManager.get()->GetSourceFilesWhichDependOnSourceFile(dependsOnFileb1_Source);
    EXPECT_EQ(dependList.size(), 1);
    EXPECT_EQ(dependList[0], absPath.toUtf8().constData());
    dependList.clear();

    dependList = m_assetProcessorManager.get()->GetSourceFilesWhichDependOnSourceFile(dependsOnFilec1_Job);
    EXPECT_EQ(dependList.size(), 1);
    EXPECT_EQ(dependList[0], absPath.toUtf8().constData());
    dependList.clear();

    dependList = m_assetProcessorManager.get()->GetSourceFilesWhichDependOnSourceFile(dependsOnFilea_Source);
    EXPECT_EQ(dependList.size(), 0);
    dependList.clear();

    dependList = m_assetProcessorManager.get()->GetSourceFilesWhichDependOnSourceFile(dependsOnFiled_Job);
    EXPECT_EQ(dependList.size(), 0);

    dependList.clear();
}

TEST_F(AssetProcessorManagerTest, RemoveSource_RemoveCacheFolderIfEmpty_Ok)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    QDir tempPath(m_tempDir.path());
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
        sourceFiles.append(tempPath.absoluteFilePath("subfolder1/subfolder2/source_test%1.txt").arg(idx));
        UnitTestUtils::CreateDummyFile(sourceFiles[idx], "source");
        // Tell the APM about the file:
        m_isIdling = false;
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFiles[idx]));
        ASSERT_TRUE(BlockUntilIdle(5000));

        productFiles.append(QDir(jobDetails.m_destinationPath).absoluteFilePath("product_test%1.txt").arg(idx));
        UnitTestUtils::CreateDummyFile(productFiles.back(), "product");

        // Populate ProcessJobResponse
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        JobProduct product(productFiles.back().toUtf8().constData(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(idx));
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
    QDir cacheDirectory(jobDetails.m_destinationPath);

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

    productFile.append(QDir(jobDetails[0].m_destinationPath).absoluteFilePath("product_test." + extension));
    UnitTestUtils::CreateDummyFile(productFile, "product");

    // Populate ProcessJobResponse
    response.m_resultCode = ProcessJobResult_Success;
    JobProduct jobProduct(productFile.toUtf8().constData(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(0));
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
    QDir tempPath(m_tempDir.path());
    QString sourceFile;
    AZStd::vector<JobDetails> jobDetails;

    ProcessJobResponse response;
    SetupDuplicateProductsTest(sourceFile, tempPath, productFile, jobDetails, response, false, "txt");

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
    QDir tempPath(m_tempDir.path());
    QString sourceFile;
    AZStd::vector<JobDetails> jobDetails;

    ProcessJobResponse response;
    SetupDuplicateProductsTest(sourceFile, tempPath, productFile, jobDetails, response, true, "png");

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

    QDir tempPath(m_tempDir.path());
    QString sourceFile;
    QString productFile;

    // Capture the job details as the APM inspects the file.
    AZStd::vector<JobDetails> jobDetails;
    ProcessJobResponse response;
    SetupDuplicateProductsTest(sourceFile, tempPath, productFile, jobDetails, response, false, "txt");

    // ----------------------------- TEST BEGINS HERE -----------------------------
    // We will process another job with the same source file outputting a different product file

    productFile = QDir(jobDetails[0].m_destinationPath).absoluteFilePath("product_test1.txt");
    UnitTestUtils::CreateDummyFile(productFile, "product");

    JobProduct newJobProduct(productFile.toUtf8().constData(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(0));
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

    m_data->m_mockBuilderInfoHandler.m_builderDesc = m_data->m_mockBuilderInfoHandler.CreateBuilderDesc("test builder", m_data->m_builderUuid.ToString<QString>(), { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) });
    m_data->m_mockBuilderInfoHandler.BusConnect();

    QDir tempPath(m_tempDir.path());

    QString watchFolderPath = tempPath.absoluteFilePath("subfolder1");
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
    m_data->m_mockBuilderInfoHandler.m_jobDependencyFilePath = "a.txt";
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
    m_data->m_mockBuilderInfoHandler.m_jobDependencyFilePath = "c.txt";
    CaptureJobs(capturedDetails, "subfolder1/b.txt");

    ASSERT_EQ(capturedDetails.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList.size(), 1);
    ASSERT_EQ(capturedDetails[0].m_jobDependencyList[0].m_builderUuidList.size(), 1);
}

TEST_F(JobDependencyTest, JobDependency_ThatHasNotRun_IsNotFound)
{
    AZStd::vector<JobDetails> capturedDetails;

    capturedDetails.clear();
    m_data->m_mockBuilderInfoHandler.m_jobDependencyFilePath = "c.txt";
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
            MockMultiBuilderInfoHandler::AssetBuilderExtraInfo{ jobDependencyPath });
    }

    m_data->m_mockBuilderInfoHandler.BusConnect();
}

void ChainJobDependencyTest::TearDown()
{
    m_data = nullptr;

    AssetProcessorManagerTest::TearDown();
}

MockMultiBuilderInfoHandler::~MockMultiBuilderInfoHandler()
{
    BusDisconnect();
}

void MockMultiBuilderInfoHandler::GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
{
    AZStd::set<AZ::Uuid>  uniqueBuilderDescIDs;

    for (AssetUtilities::BuilderFilePatternMatcher& matcherPair : m_matcherBuilderPatterns)
    {
        if (uniqueBuilderDescIDs.find(matcherPair.GetBuilderDescID()) != uniqueBuilderDescIDs.end())
        {
            continue;
        }
        if (matcherPair.MatchesPath(assetPath))
        {
            const AssetBuilderSDK::AssetBuilderDesc& builderDesc = m_builderDescMap[matcherPair.GetBuilderDescID()];
            uniqueBuilderDescIDs.insert(matcherPair.GetBuilderDescID());
            builderInfoList.push_back(builderDesc);
        }
    }
}

void MockMultiBuilderInfoHandler::GetAllBuildersInfo([[maybe_unused]] AssetProcessor::BuilderInfoList& builderInfoList)
{
    // Only here to fulfill the interface requirement, this won't be called as part of the test
    ASSERT_TRUE(false) << "Not implemented";
}

void MockMultiBuilderInfoHandler::CreateJobs(AssetBuilderExtraInfo extraInfo, const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
{
    response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

    for (const auto& platform : request.m_enabledPlatforms)
    {
        AssetBuilderSDK::JobDescriptor jobDescriptor;
        jobDescriptor.m_priority = 0;
        jobDescriptor.m_critical = true;
        jobDescriptor.m_jobKey = "Mock Job";
        jobDescriptor.SetPlatformIdentifier(platform.m_identifier.c_str());

        if (!extraInfo.m_jobDependencyFilePath.isEmpty())
        {
            jobDescriptor.m_jobDependencyList.push_back(AssetBuilderSDK::JobDependency("Mock Job", "pc", AssetBuilderSDK::JobDependencyType::Order,
                AssetBuilderSDK::SourceFileDependency(extraInfo.m_jobDependencyFilePath.toUtf8().constData(), AZ::Uuid::CreateNull())));
        }

        response.m_createJobOutputs.push_back(jobDescriptor);
        m_createJobsCount++;
    }
}

void MockMultiBuilderInfoHandler::ProcessJob(AssetBuilderExtraInfo extraInfo, [[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
{
    response.m_resultCode = AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success;
}

void MockMultiBuilderInfoHandler::CreateBuilderDesc(const QString& builderName, const QString& builderId, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns, AssetBuilderExtraInfo extraInfo)
{
    AssetBuilderSDK::AssetBuilderDesc builderDesc;

    builderDesc.m_name = builderName.toUtf8().data();
    builderDesc.m_patterns = builderPatterns;
    builderDesc.m_busId = AZ::Uuid::CreateString(builderId.toUtf8().data());
    builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;
    builderDesc.m_createJobFunction = AZStd::bind(&MockMultiBuilderInfoHandler::CreateJobs, this, extraInfo, AZStd::placeholders::_1, AZStd::placeholders::_2);
    builderDesc.m_processJobFunction = AZStd::bind(&MockMultiBuilderInfoHandler::ProcessJob, this, extraInfo, AZStd::placeholders::_1, AZStd::placeholders::_2);

    m_builderDescMap[builderDesc.m_busId] = builderDesc;

    for (const AssetBuilderSDK::AssetBuilderPattern& pattern : builderDesc.m_patterns)
    {
        AssetUtilities::BuilderFilePatternMatcher patternMatcher(pattern, builderDesc.m_busId);
        m_matcherBuilderPatterns.push_back(patternMatcher);
    }
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
            ASSERT_EQ(capturedDetails[0].m_jobDependencyList[0].m_jobDependency.m_sourceFile.m_sourceFileDependencyPath, AZStd::string::format("%d.txt", i - 1));

            capturedDetails.clear();
        }
    }

    // Wait for the file compiled event and trigger OnddedToCatalog with a delay, this is what causes rccontroller to process out of order
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
        ASSERT_STREQ(capturedDetails[i].m_jobEntry.m_databaseSourceName.toUtf8().constData(), finishedJobs[i].m_databaseSourceName.toUtf8().constData());
    }
}

void DeleteTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();

    m_data = AZStd::make_unique<StaticData>();

    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;

    // We don't want the mock application manager to provide builder descriptors, mockBuilderInfoHandler will provide our own
    m_mockApplicationManager->BusDisconnect();

    m_data->m_mockBuilderInfoHandler.m_builderDesc = m_data->m_mockBuilderInfoHandler.CreateBuilderDesc("test builder", "{DF09DDC0-FD22-43B6-9E22-22C8574A6E1E}", { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) });
    m_data->m_mockBuilderInfoHandler.BusConnect();

    ASSERT_TRUE(m_mockApplicationManager->GetBuilderByID("txt files", m_data->m_builderTxtBuilder));

    // Run this twice so the test builder doesn't get counted as a "new" builder and bypass the modtime skipping
    m_assetProcessorManager->ComputeBuilderDirty();
    m_assetProcessorManager->ComputeBuilderDirty();

    auto setupConnectionsFunc = [this]()
    {
        QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [this](JobDetails details)
        {
            m_data->m_processResults.push_back(AZStd::move(details));
        });

        QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::SourceDeleted, [this](QString file)
        {
            m_data->m_deletedSources.push_back(file);
        });
    };

    auto createFileAndAddToDatabaseFunc = [this](const AssetProcessor::ScanFolderInfo* scanFolder, QString file)
    {
        using namespace AzToolsFramework::AssetDatabase;

        QString watchFolderPath = scanFolder->ScanPath();
        QString absPath(QDir(watchFolderPath).absoluteFilePath(file));
        UnitTestUtils::CreateDummyFile(absPath);

        m_data->m_absolutePath.push_back(absPath);

        AzToolsFramework::AssetDatabase::FileDatabaseEntry fileEntry;
        fileEntry.m_fileName = file.toUtf8().constData();
        fileEntry.m_modTime = 0;
        fileEntry.m_isFolder = false;
        fileEntry.m_scanFolderPK = scanFolder->ScanFolderID();

        bool entryAlreadyExists;
        ASSERT_TRUE(m_assetProcessorManager->m_stateData->InsertFile(fileEntry, entryAlreadyExists));
        ASSERT_FALSE(entryAlreadyExists);
    };

    setupConnectionsFunc();

    // Create test files
    QDir tempPath(m_tempDir.path());
    const auto* scanFolder1 = m_config->GetScanFolderByPath(tempPath.absoluteFilePath("subfolder1"));
    const auto* scanFolder4 = m_config->GetScanFolderByPath(tempPath.absoluteFilePath("subfolder4"));

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
    m_assetProcessorManager.reset(new AssetProcessorManager_Test(m_config.get()));

    m_idleConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState, [this](bool newState)
    {
        m_isIdling = newState;
    });

    setupConnectionsFunc();

    m_assetProcessorManager->ComputeBuilderDirty();
}

TEST_F(DeleteTest, DeleteFolderSharedAcrossTwoScanFolders_CorrectFileAndFolderAreDeletedFromCache)
{
    // There was a bug where AP wasn't repopulating the "known folders" list when modtime skipping was enabled and no work was needed
    // As a result, deleting a folder didn't count as a "folder", so the wrong code path was taken.  This test makes sure the correct deletion events fire

    using namespace AzToolsFramework::AssetSystem;

    // Modtime skipping has to be on for this
    m_assetProcessorManager->m_allowModtimeSkippingFeature = true;

    // Feed in the files from the asset scanner, no jobs should run since they're already up-to-date
    QSet<AssetFileInfo> filePaths = BuildFileSet();
    SimulateAssetScanner(filePaths);

    ExpectNoWork();

    // Delete one of the folders
    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/textures"));
    QDir(absPath).removeRecursively();

    AZStd::vector<AZStd::string> deletedFolders;
    QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::SourceFolderDeleted, [&deletedFolders](QString file)
    {
        deletedFolders.push_back(file.toUtf8().constData());
    });

    m_assetProcessorManager->AssessDeletedFile(absPath);
    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_THAT(m_data->m_deletedSources, testing::UnorderedElementsAre("textures/a.txt"));
    ASSERT_THAT(deletedFolders, testing::UnorderedElementsAre("textures"));
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

    QDir tempPath(m_tempDir.path());

    QString relFileName("Dummy.TXT");
    QString absPath(tempPath.absoluteFilePath("subfolder1/Dummy.TXT"));
    QString watchFolder = tempPath.absoluteFilePath("subfolder1");
    UnitTestUtils::CreateDummyFile(absPath, "dummy");

    JobEntry entry;
    entry.m_watchFolderPath = watchFolder;
    entry.m_databaseSourceName = entry.m_pathRelativeToWatchFolder = relFileName;
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    QString productPath(m_normalizedCacheRootDir.absoluteFilePath("outputfile.TXT"));
    UnitTestUtils::CreateDummyFile(productPath);

    AssetBuilderSDK::ProcessJobResponse jobResponse;
    jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productPath.toUtf8().data()));

    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, entry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, jobResponse));

    ASSERT_TRUE(BlockUntilIdle(5000));

    // Creating a metadata file for the source assets
    // APM should process the source asset if a metadafile is detected
    // We are intentionally having a source file with a different file extension casing than the one specified in the metadata rule.
    QString metadataFile(tempPath.absoluteFilePath("subfolder1/Dummy.foo"));
    UnitTestUtils::CreateDummyFile(metadataFile, "dummy");

    // Capture the job details as the APM inspects the file.
    JobDetails jobDetails;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&jobDetails](JobDetails job)
        {
            jobDetails = job;
        });

    m_assetProcessorManager->AssessAddedFile(tempPath.absoluteFilePath(metadataFile));

    ASSERT_TRUE(BlockUntilIdle(5000));
    ASSERT_EQ(jobDetails.m_jobEntry.m_pathRelativeToWatchFolder, relFileName);
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
    auto result = m_assetProcessorManager->GetSourceFilesWhichDependOnSourceFile(path);

    return QStringListToVector(result);
}

void WildcardSourceDependencyTest::SetUp()
{
    AssetProcessorManagerTest::SetUp();

    QDir tempPath(m_tempDir.path());

    // Add a non-recursive scan folder.  Only files directly inside of this folder should be picked up, subfolders are ignored
    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("no_recurse"), "no_recurse",
        "no_recurse", false, false, m_config->GetEnabledPlatforms(), 1));

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/1a.foo"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/1b.foo"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder2/redirected/a.foo"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder2/redirected/b.foo"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder2/redirected/folder/one/c.foo"));
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder2/redirected/folder/one/d.foo"));

    // Add a file that is not in a scanfolder.  Should always be ignored
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("not/a/scanfolder/e.foo"));

    // Add a file in the non-recursive scanfolder.  Since its not directly in the scan folder, it should always be ignored
    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("no_recurse/one/two/three/f.foo"));

    AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer dependencies;

    // Relative path wildcard dependency
    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), "a.foo", "%a.foo",
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0));

    // Absolute path wildcard dependency
    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), "b.foo", tempPath.absoluteFilePath("%b.foo").toUtf8().constData(),
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0));

    // Test what happens when we have 2 dependencies on the same file
    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), "folder/one/d.foo", "%c.foo",
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0));

    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), "folder/one/d.foo", tempPath.absoluteFilePath("%c.foo").toUtf8().constData(),
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0));

#ifdef AZ_PLATFORM_WINDOWS
    // Test to make sure a relative wildcard dependency doesn't match an absolute path
    // For example, if the input is C:/project/subfolder1/a.foo
    // This should not match a wildcard of c%.foo
    // Take the first character of the tempPath and append %.foo onto it for this test, which should produce something like c%.foo
    // This only applies to windows because on other OSes if the dependency starts with /, then its an abs path dependency
    auto test = (tempPath.absolutePath().left(1) + "%.foo");
    dependencies.push_back(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry(
        AZ::Uuid::CreateRandom(), "folder/one/d.foo",
        (test).toUtf8().constData(),
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch, 0));
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
    QDir tempPath(m_tempDir.path());

    ASSERT_TRUE(Test(tempPath.absoluteFilePath("subfolder2/redirected/*.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre("a.foo", "b.foo", "folder/one/c.foo", "folder/one/d.foo"));
}

TEST_F(WildcardSourceDependencyTest, Absolute_NotInScanfolder)
{
    // Files outside a scanfolder should not be returned even with an absolute path
    AZStd::vector<AZStd::string> resolvedPaths;
    QDir tempPath(m_tempDir.path());

    ASSERT_TRUE(Test(tempPath.absoluteFilePath("not/a/scanfolder/*.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_NotInScanfolder)
{
    // Files outside a scanfolder should not be returned
    AZStd::vector<AZStd::string> resolvedPaths;
    QDir tempPath(m_tempDir.path());

    ASSERT_TRUE(Test("*/e.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_InNonRecursiveScanfolder)
{
    // Files deep inside non-recursive scanfolders should not be returned
    AZStd::vector<AZStd::string> resolvedPaths;
    QDir tempPath(m_tempDir.path());

    ASSERT_TRUE(Test("*/f.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Absolute_InNonRecursiveScanfolder)
{
    // Absolute paths to files deep inside non-recursive scanfolders should not be returned
    AZStd::vector<AZStd::string> resolvedPaths;
    QDir tempPath(m_tempDir.path());

    ASSERT_TRUE(Test(tempPath.absoluteFilePath("one/two/three/*.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Relative_NoWildcard)
{
    // No wildcard results in a failure
    AZStd::vector<AZStd::string> resolvedPaths;
    QDir tempPath(m_tempDir.path());

    ASSERT_FALSE(Test("subfolder1/1a.foo", resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, Absolute_NoWildcard)
{
    // No wildcard results in a failure
    AZStd::vector<AZStd::string> resolvedPaths;
    QDir tempPath(m_tempDir.path());

    ASSERT_FALSE(Test(tempPath.absoluteFilePath("subfolder1/1a.foo").toUtf8().constData(), resolvedPaths));
    ASSERT_THAT(resolvedPaths, ::testing::UnorderedElementsAre());
}

TEST_F(WildcardSourceDependencyTest, NewFile_MatchesSavedRelativeDependency)
{
    QDir tempPath(m_tempDir.path());

    auto matches = FileAddedTest(tempPath.absoluteFilePath("subfolder1/1a.foo"));

    ASSERT_THAT(matches, ::testing::UnorderedElementsAre(tempPath.absoluteFilePath("subfolder2/redirected/a.foo").toUtf8().constData()));
}

TEST_F(WildcardSourceDependencyTest, NewFile_MatchesSavedAbsoluteDependency)
{
    QDir tempPath(m_tempDir.path());

    auto matches = FileAddedTest(tempPath.absoluteFilePath("subfolder1/1b.foo"));

    ASSERT_THAT(matches, ::testing::UnorderedElementsAre(tempPath.absoluteFilePath("subfolder2/redirected/b.foo").toUtf8().constData()));
}

TEST_F(WildcardSourceDependencyTest, NewFile_MatchesDuplicatedDependenciesOnce)
{
    QDir tempPath(m_tempDir.path());

    auto matches = FileAddedTest(tempPath.absoluteFilePath("subfolder2/redirected/folder/one/c.foo"));

    ASSERT_THAT(matches, ::testing::UnorderedElementsAre(tempPath.absoluteFilePath("subfolder2/redirected/folder/one/d.foo").toUtf8().constData()));
}
