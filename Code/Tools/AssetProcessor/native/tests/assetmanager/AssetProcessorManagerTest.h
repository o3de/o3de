/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/std/parallel/atomic.h>
#include <qcoreapplication.h>
#include <native/tests/AssetProcessorTest.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <native/assetprocessor.h>
#include <native/unittests/UnitTestUtils.h>
#include <native/AssetManager/assetProcessorManager.h>
#include <native/utilities/PlatformConfiguration.h>
#include <native/unittests/MockApplicationManager.h>
#include <AssetManager/FileStateCache.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QMetaObject>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <tests/UnitTestUtilities.h>

#include "resourcecompiler/rccontroller.h"

class AssetProcessorManager_Test;

class AssetProcessorManager_Test : public AssetProcessor::AssetProcessorManager
{
public:
    friend class GTEST_TEST_CLASS_NAME_(
        AssetProcessorManagerTest, AssetProcessedImpl_DifferentProductDependenciesPerProduct_SavesCorrectlyToDatabase);

    friend class GTEST_TEST_CLASS_NAME_(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies);
    friend class GTEST_TEST_CLASS_NAME_(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies_DeferredResolution);
    friend class GTEST_TEST_CLASS_NAME_(MultiplatformPathDependencyTest, SameFilenameForAllPlatforms);

    friend class GTEST_TEST_CLASS_NAME_(MultiplatformPathDependencyTest, AssetProcessed_Impl_MultiplatformDependencies_SourcePath);

    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, DeleteFolder_SignalsDeleteOfContainedFiles);

    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_BasicTest);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_WithDifferentTypes_BasicTest);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_Reverse_BasicTest);
    friend class GTEST_TEST_CLASS_NAME_(
        AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_MissingFiles_ReturnsNoPathWithPlaceholders);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, QueryAbsolutePathDependenciesRecursive_DependenciesOnNonAssetsIncluded);

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

    friend class GTEST_TEST_CLASS_NAME_(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_BasicTest);
    friend class GTEST_TEST_CLASS_NAME_(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_UpdateTest);
    friend class GTEST_TEST_CLASS_NAME_(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid);
    friend class GTEST_TEST_CLASS_NAME_(SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByName);
    friend class GTEST_TEST_CLASS_NAME_(
        SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByUuid_UpdatesWhenTheyAppear);
    friend class GTEST_TEST_CLASS_NAME_(
        SourceFileDependenciesTest, UpdateSourceFileDependenciesDatabase_MissingFiles_ByName_UpdatesWhenTheyAppear);
    friend class GTEST_TEST_CLASS_NAME_(
        AssetProcessorManagerTest, UpdateSourceFileDependenciesDatabase_WildcardMissingFiles_ByName_UpdatesWhenTheyAppear);
    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, JobDependencyOrderOnce_MultipleJobs_EmitOK);

    friend class GTEST_TEST_CLASS_NAME_(AssetProcessorManagerTest, SourceFileProcessFailure_ClearsFingerprint);

    friend class GTEST_TEST_CLASS_NAME_(
        AbsolutePathProductDependencyTest, UnresolvedProductPathDependency_AssetProcessedTwice_DoesNotDuplicateDependency);
    friend class GTEST_TEST_CLASS_NAME_(
        AbsolutePathProductDependencyTest, AbsolutePathProductDependency_RetryDeferredDependenciesWithMatchingSource_DependencyResolves);
    friend class GTEST_TEST_CLASS_NAME_(
        AbsolutePathProductDependencyTest, UnresolvedProductPathDependency_AssetProcessedTwice_ValidatePathDependenciesMap);
    friend class GTEST_TEST_CLASS_NAME_(
        AbsolutePathProductDependencyTest,
        UnresolvedSourceFileTypeProductPathDependency_DependencyHasNoProductOutput_ValidatePathDependenciesMap);

    friend class GTEST_TEST_CLASS_NAME_(DeleteTest, DeleteFolderSharedAcrossTwoScanFolders_CorrectFileAndFolderAreDeletedFromCache);
    friend class GTEST_TEST_CLASS_NAME_(MetadataFileTest, MetadataFile_SourceFileExtensionDifferentCase);

    friend class AssetProcessorManagerTest;
    friend struct JobDependencyTest;
    friend struct ChainJobDependencyTest;
    friend struct DeleteTest;
    friend struct PathDependencyTest;
    friend struct DuplicateProductsTest;
    friend struct DuplicateProcessTest;
    friend struct AbsolutePathProductDependencyTest;
    friend struct WildcardSourceDependencyTest;
    friend struct SourceFileDependenciesTest;

    explicit AssetProcessorManager_Test(AssetProcessor::PlatformConfiguration* config, QObject* parent = nullptr);
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

    void RecomputeDirtyBuilders()
    {
        // Run this twice so the test builder doesn't get counted as a "new" builder and bypass the modtime skipping
        ComputeBuilderDirty();
        ComputeBuilderDirty();
    }

    using AssetProcessorManager::m_stateData;
    using AssetProcessorManager::ComputeBuilderDirty;

    using AssetProcessorManager::m_anyBuilderChange;
    using AssetProcessorManager::m_buildersAddedOrRemoved;
};

class AssetProcessorManagerTest
    : public AssetProcessor::AssetProcessorTest
{
public:


    AssetProcessorManagerTest();
    virtual ~AssetProcessorManagerTest()
    {
    }

    // utility function.  Blocks and runs the QT event pump for up to millisecondsMax and will break out as soon as the APM is idle.
    bool BlockUntilIdle(int millisecondsMax);

protected:
    void SetUp() override;
    void TearDown() override;

    virtual void CreateSourceAndFile(const char* tempFolderRelativePath);
    virtual void PopulateDatabase();

    QDir m_assetRootDir;

    AZStd::unique_ptr<AssetProcessorManager_Test> m_assetProcessorManager;
    AZStd::unique_ptr<AssetProcessor::MockApplicationManager> m_mockApplicationManager;
    AssetProcessor::MockAssetDatabaseRequestsHandler m_databaseLocationListener;
    AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_config;
    ::UnitTests::MockVirtualFileIO m_virtualFileIO;
    AzToolsFramework::UuidUtilComponent m_uuidUtil;
    AzToolsFramework::MetadataManager m_metadataManager;
    AssetProcessor::UuidManager m_uuidManager;
    QString m_gameName;
    QDir m_normalizedCacheRootDir;
    AZStd::atomic_bool m_isIdling;
    QMetaObject::Connection m_idleConnection;

    AZ::Uuid m_aUuid;
    AZ::Uuid m_bUuid;
    AZ::Uuid m_cUuid;
    AZ::Uuid m_dUuid;

    struct StaticData
    {
        AZStd::string m_databaseLocation;
        AZ::Entity* m_jobManagerEntity{};
        AZ::ComponentDescriptor* m_descriptor{};
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    AZStd::unique_ptr<StaticData> m_data;

private:
    int         m_argc;
    char**      m_argv;
    AZStd::unique_ptr<UnitTestUtils::ScopedDir> m_scopeDir;

    AZStd::unique_ptr<QCoreApplication> m_qApp;
};

struct AbsolutePathProductDependencyTest
    : public AssetProcessorManagerTest
{
    void SetUp() override;

    AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry SetAndReadAbsolutePathProductDependencyFromRelativePath(
        const AZStd::string& relativePath);

    AZStd::string BuildScanFolderRelativePath(const AZStd::string& relativePath) const;

    AzToolsFramework::AssetDatabase::ProductDatabaseEntry m_productToHaveDependency;
    const AssetProcessor::ScanFolderInfo* m_scanFolderInfo = nullptr;
    AZStd::string m_testPlatform = "SomePlatform";
};

struct SourceFileDependenciesTest : AssetProcessorManagerTest
{
    void SetUp() override;

    void SetupData(
        const AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceFileDependencies,
        const AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
        bool createFile1Dummies,
        bool createFile2Dummies,
        bool primeMap,
        AssetProcessor::AssetProcessorManager::JobToProcessEntry& job);

    void PopulateDatabase() override;

    auto GetDependencyList();

    AssetBuilderSDK::SourceFileDependency MakeSourceDependency(const char* file, bool wildcard = false);
    AssetBuilderSDK::SourceFileDependency MakeSourceDependency(AZ::Uuid uuid);
    AssetBuilderSDK::JobDependency MakeJobDependency(const char* file);
    AssetBuilderSDK::JobDependency MakeJobDependency(AZ::Uuid uuid);

    QString m_absPath;
    QString m_watchFolderPath;
    QString m_dependsOnFile1_Source;
    QString m_dependsOnFile2_Source;
    QString m_dependsOnFile1_Job;
    QString m_dependsOnFile2_Job;

    const AssetProcessor::ScanFolderInfo* m_scanFolder = nullptr;

    AZ::Uuid m_dummyBuilderUuid;
    AZ::Uuid m_sourceFileUuid;
    AZ::Uuid m_uuidOfA;
    AZ::Uuid m_uuidOfB;
    AZ::Uuid m_uuidOfC;
    AZ::Uuid m_uuidOfD;
};


struct PathDependencyTest
    : public AssetProcessorManagerTest
{
    void SetUp() override;
    void TearDown() override;

    using OutputAssetSet = AZStd::vector<AZStd::vector<const char*>>;

    struct TestAsset
    {
        TestAsset() = default;
        TestAsset(const char* name) : m_name(name) {}

        AZStd::string m_name;
        AZStd::vector<AZ::Data::AssetId> m_products;
    };

    void CaptureJobs(AZStd::vector<AssetProcessor::JobDetails>& jobDetails, const char* sourceFilePath);
    bool ProcessAsset(TestAsset& asset, const OutputAssetSet& outputAssets, const AssetBuilderSDK::ProductPathDependencySet& dependencies = {}, const AZStd::string& folderPath = "subfolder1/", const AZStd::string& extension = ".txt");

    void RunWildcardTest(bool useCorrectDatabaseSeparator, AssetBuilderSDK::ProductPathDependencyType pathDependencyType, bool buildDependenciesFirst);

    void RunWildcardDependencyTestOnPaths(
        const AZStd::string& wildcardDependency,
        const AZStd::vector<AZStd::string>& expectedMatchingPaths,
        const AZStd::vector<AZStd::string>& expectedNotMatchingPaths);

    AssetProcessor::AssetDatabaseConnection* m_sharedConnection{};
};

struct DuplicateProcessTest
    : public PathDependencyTest
{
    void SetUp() override;
};

struct MultiplatformPathDependencyTest
    : public PathDependencyTest
{
    void SetUp() override;
};

struct WildcardSourceDependencyTest
    :  AssetProcessorManagerTest
{
    bool Test(const AZStd::string& dependencyPath, AZStd::vector<AZStd::string>& resolvedPaths);
    AZStd::vector<AZStd::string> FileAddedTest(const QString& path);

    void SetUp() override;
};


struct MetadataFileTest
    : public AssetProcessorManagerTest
{
    void SetUp() override;
};

struct FingerprintTest
    : public AssetProcessorManagerTest
{
    void SetUp() override;
    void TearDown() override;

    void RunFingerprintTest(QString builderFingerprint, QString jobFingerprint, bool expectedResult);

    QString m_absolutePath;
    UnitTests::MockMultiBuilderInfoHandler m_mockBuilderInfoHandler;
    AZStd::vector<AssetProcessor::JobDetails> m_jobResults;
};

struct JobDependencyTest
    : public PathDependencyTest
{
    void SetUp() override;
    void TearDown() override;

    struct StaticData
    {
        UnitTests::MockMultiBuilderInfoHandler m_mockBuilderInfoHandler;
        UnitTests::MockMultiBuilderInfoHandler::AssetBuilderExtraInfo m_assetBuilderConfig;
        AZ::Uuid m_builderUuid;
    };

    AZStd::unique_ptr<StaticData> m_data;
};

struct ChainJobDependencyTest
    : public PathDependencyTest
{
    void SetUp() override;
    void TearDown() override;

    struct StaticData
    {
        UnitTests::MockMultiBuilderInfoHandler m_mockBuilderInfoHandler;
        AZStd::unique_ptr<AssetProcessor::RCController> m_rcController;
    };

    static constexpr int ChainLength = 10;
    AZStd::unique_ptr<StaticData> m_data;
};

struct DuplicateProductsTest
    : public AssetProcessorManagerTest
{
    void SetupDuplicateProductsTest(QString& sourceFile, QDir& tempPath, QString& productFile, AZStd::vector<AssetProcessor::JobDetails>& jobDetails, AssetBuilderSDK::ProcessJobResponse& response, bool multipleOutputs, QString extension);
};
