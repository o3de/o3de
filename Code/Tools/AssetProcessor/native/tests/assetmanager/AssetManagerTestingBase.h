/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <API/AssetDatabaseBus.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <Tests/Utils/Utils.h>
#include <native/AssetManager/assetProcessorManager.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <resourcecompiler/rccontroller.h>
#include <tests/UnitTestUtilities.h>
#include <QCoreApplication>

namespace AZ::IO
{
    class LocalFileIO;
}

namespace UnitTests
{
    class MockDiskSpaceResponder : public AZ::Interface<AssetProcessor::IDiskSpaceInfo>::Registrar
    {
    public:
        MOCK_METHOD2(CheckSufficientDiskSpace, bool(qint64, bool));
    };

    class JobSignalReceiver : AZ::Interface<AssetProcessor::IRCJobSignal>::Registrar
    {
    public:
        AZ_RTTI(JobSignalReceiver, "{8C1BEBF9-655C-4352-84DB-3BBB421CB3D3}", AssetProcessor::IRCJobSignal);

        void Finished() override
        {
            m_signal.release();
        }

        void WaitForFinish()
        {
            m_signal.acquire();
        }

    protected:
        AZStd::binary_semaphore m_signal;
    };

    class AssetManagerTestingBase;

    class TestingAssetProcessorManager : public AssetProcessor::AssetProcessorManager
    {
    public:
        friend class AssetManagerTestingBase;

        TestingAssetProcessorManager(AssetProcessor::PlatformConfiguration* config, QObject* parent = nullptr)
            : AssetProcessorManager(config, parent)
        {
        }

        void CheckActiveFiles(int count);
        void CheckFilesToExamine(int count);
        void CheckJobEntries(int count);
    };

    class AssetManagerTestingBase : public ::UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;

    protected:
        void RunFile(int expectedJobCount, int expectedFileCount = 1, int dependencyFileCount = 0);
        void ProcessJob(AssetProcessor::RCController& rcController, const AssetProcessor::JobDetails& jobDetails);

        AZStd::string MakePath(const char* filename, bool intermediate);

        void CheckProduct(const char* relativePath, bool exists = true);
        void CheckIntermediate(const char* relativePath, bool exists = true);
        void ProcessSingleStep(int expectedJobCount = 1, int expectedFileCount = 1, int jobToRun = 0, bool expectSuccess = true);

        void ProcessFileMultiStage(
            int endStage,
            bool doProductOutputCheck,
            const char* file = nullptr,
            int startStage = 1,
            bool expectAutofail = false,
            bool hasExtraFile = false);

        AssetBuilderSDK::CreateJobFunction CreateJobStage(
            const AZStd::string& name, bool commonPlatform, const AZStd::string& sourceDependencyPath = "");

        AssetBuilderSDK::ProcessJobFunction ProcessJobStage(
            const AZStd::string& outputExtension, AssetBuilderSDK::ProductOutputFlags flags, bool outputExtraFile);

        void CreateBuilder(
            const char* name,
            const char* inputFilter,
            const char* outputExtension,
            bool createJobCommonPlatform,
            AssetBuilderSDK::ProductOutputFlags outputFlags,
            bool outputExtraFile = false);

        int m_argc = 0;
        char** m_argv{};

        AssetProcessor::FileStatePassthrough m_fileStateCache;

        AZStd::unique_ptr<QCoreApplication> m_qApp;
        AZStd::unique_ptr<TestingAssetProcessorManager> m_assetProcessorManager;
        AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_platformConfig;
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
        AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> m_stateData;
        AZStd::unique_ptr<::testing::NiceMock<MockDiskSpaceResponder>> m_diskSpaceResponder;
        AssetProcessor::MockAssetDatabaseRequestsHandler m_databaseLocationListener;
        AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry m_scanfolder;
        MockMultiBuilderInfoHandler m_builderInfoHandler;
        AZ::IO::LocalFileIO* m_localFileIo;

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZ::Entity* m_jobManagerEntity{};
        AZ::ComponentDescriptor* m_descriptor{};

        AZStd::unique_ptr<AssetProcessor::RCController> m_rc;

        AZStd::vector<AssetProcessor::JobDetails> m_jobDetailsList;

        bool m_fileCompiled = false;
        bool m_fileFailed = false;
        AZStd::string m_testFilePath;

        AssetProcessor::JobEntry m_processedJobEntry;
        AssetBuilderSDK::ProcessJobResponse m_processJobResponse;
    };
} // namespace UnitTests
