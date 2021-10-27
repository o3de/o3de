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
#include "native/tests/AssetProcessorTest.h"
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "native/assetprocessor.h"
#include "native/unittests/UnitTestRunner.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/unittests/MockApplicationManager.h"
#include <AssetManager/FileStateCache.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QTemporaryDir>
#include <QMetaObject>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include "resourcecompiler/rccontroller.h"

class AssetProcessorManager_Test;

class MockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
{
public:
    MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
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

    QTemporaryDir m_tempDir;

    AZStd::unique_ptr<AssetProcessorManager_Test> m_assetProcessorManager;
    AZStd::unique_ptr<AssetProcessor::MockApplicationManager> m_mockApplicationManager;
    AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_config;
    UnitTestUtils::AssertAbsorber m_assertAbsorber; // absorb asserts/warnings/errors so that the unit test output is not cluttered
    QString m_gameName;
    QDir m_normalizedCacheRootDir;
    AZStd::atomic_bool m_isIdling;
    QMetaObject::Connection m_idleConnection;

    struct StaticData
    {
        AZStd::string m_databaseLocation;
        ::testing::NiceMock<MockDatabaseLocationListener> m_databaseLocationListener;
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

struct MockBuilderInfoHandler
    : public AssetProcessor::AssetBuilderInfoBus::Handler
{
    ~MockBuilderInfoHandler();

    //! AssetProcessor::AssetBuilderInfoBus Interface
    void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) override;
    void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override;

    void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
    void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

    AssetBuilderSDK::AssetBuilderDesc CreateBuilderDesc(const QString& builderName, const QString& builderId, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns);

    AssetBuilderSDK::AssetBuilderDesc m_builderDesc;
    QString m_jobFingerprint;
    QString m_dependencyFilePath;
    QString m_jobDependencyFilePath;
    int m_createJobsCount = 0;
};

struct ModtimeScanningTest
    : public AssetProcessorManagerTest
{
    void SetUp() override;
    void TearDown() override;

    void ProcessAssetJobs();
    void SimulateAssetScanner(QSet<AssetProcessor::AssetFileInfo> filePaths);
    QSet<AssetProcessor::AssetFileInfo> BuildFileSet();
    void ExpectWork(int createJobs, int processJobs);
    void ExpectNoWork();
    void SetFileContents(QString filePath, QString contents);

    struct StaticData
    {
        QString m_relativePathFromWatchFolder[3];
        AZStd::vector<QString> m_absolutePath;
        AZStd::vector<AssetProcessor::JobDetails> m_processResults;
        AZStd::unordered_multimap<AZStd::string, QString> m_productPaths;
        AZStd::vector<QString> m_deletedSources;
        AZStd::shared_ptr<AssetProcessor::InternalMockBuilder> m_builderTxtBuilder;
        MockBuilderInfoHandler m_mockBuilderInfoHandler;
    };

    AZStd::unique_ptr<StaticData> m_data;
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
    MockBuilderInfoHandler m_mockBuilderInfoHandler;
    AZStd::vector<AssetProcessor::JobDetails> m_jobResults;
};

struct JobDependencyTest
    : public PathDependencyTest
{
    void SetUp() override;
    void TearDown() override;

    struct StaticData
    {
        MockBuilderInfoHandler m_mockBuilderInfoHandler;
        AZ::Uuid m_builderUuid;
    };

    AZStd::unique_ptr<StaticData> m_data;
};

struct MockMultiBuilderInfoHandler
    : public AssetProcessor::AssetBuilderInfoBus::Handler
{
    ~MockMultiBuilderInfoHandler();

    struct AssetBuilderExtraInfo
    {
        QString m_jobDependencyFilePath;
    };

    //! AssetProcessor::AssetBuilderInfoBus Interface
    void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) override;
    void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override;

    void CreateJobs(AssetBuilderExtraInfo extraInfo, const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
    void ProcessJob(AssetBuilderExtraInfo extraInfo, const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

    void CreateBuilderDesc(const QString& builderName, const QString& builderId, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns, AssetBuilderExtraInfo extraInfo);

    AZStd::vector<AssetBuilderSDK::AssetBuilderDesc> m_builderDesc;
    AZStd::vector<AssetUtilities::BuilderFilePatternMatcher> m_matcherBuilderPatterns;
    AZStd::unordered_map<AZ::Uuid, AssetBuilderSDK::AssetBuilderDesc> m_builderDescMap;

    int m_createJobsCount = 0;
};

struct ChainJobDependencyTest
    : public PathDependencyTest
{
    void SetUp() override;
    void TearDown() override;

    struct StaticData
    {
        MockMultiBuilderInfoHandler m_mockBuilderInfoHandler;
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

struct DeleteTest
    : public ModtimeScanningTest
{
    void SetUp() override;
};
