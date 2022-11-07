/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <QCoreApplication>
#include <native/tests/assetmanager/AssetManagerTestingBase.h>
#include <native/utilities/AssetUtilEBusHelper.h>

namespace UnitTests
{
    void TestingAssetProcessorManager::CheckActiveFiles(int count)
    {
        ASSERT_EQ(m_activeFiles.size(), count);
    }

    void TestingAssetProcessorManager::CheckFilesToExamine(int count)
    {
        ASSERT_EQ(m_filesToExamine.size(), count);
    }

    void TestingAssetProcessorManager::CheckJobEntries(int count)
    {
        ASSERT_EQ(m_jobEntries.size(), count);
    }

    void AssetManagerTestingBase::SetUp()
    {
        ScopedAllocatorSetupFixture::SetUp();

        // File IO is needed to hash the files
        if (AZ::IO::FileIOBase::GetInstance() == nullptr)
        {
            m_localFileIo = aznew AZ::IO::LocalFileIO();
            AZ::IO::FileIOBase::SetInstance(m_localFileIo);
        }

        // Specify the database lives in the temp directory
        AZ::IO::Path assetRootDir(m_databaseLocationListener.GetAssetRootDir());

        // We need a settings registry in order for APM to figure out the cache path
        m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
        AZ::SettingsRegistry::Register(m_settingsRegistry.get());

        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        m_settingsRegistry->Set(projectPathKey, m_databaseLocationListener.GetAssetRootDir().c_str());
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*m_settingsRegistry);

        // We need a QCoreApplication set up in order for QCoreApplication::processEvents to function
        m_qApp = AZStd::make_unique<QCoreApplication>(m_argc, m_argv);
        qRegisterMetaType<AssetProcessor::JobEntry>("JobEntry");
        qRegisterMetaType<AssetBuilderSDK::ProcessJobResponse>("ProcessJobResponse");
        qRegisterMetaType<AZStd::string>("AZStd::string");
        qRegisterMetaType<AssetProcessor::AssetScanningStatus>("AssetProcessor::AssetScanningStatus");
        qRegisterMetaType<QSet<AssetProcessor::AssetFileInfo>>("QSet<AssetFileInfo>");

        // Platform config with an enabled platform and scanfolder required by APM to function and find the files
        m_platformConfig = AZStd::make_unique<AssetProcessor::PlatformConfiguration>();
        m_platformConfig->EnablePlatform(AssetBuilderSDK::PlatformInfo{ "pc", { "test" } });

        m_platformConfig->EnableCommonPlatform();

        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_platformConfig->PopulatePlatformsForScanFolder(platforms);

        m_platformConfig->AddScanFolder(
            AssetProcessor::ScanFolderInfo{ (assetRootDir / "folder").c_str(), "folder", "folder", false, true, platforms });

        m_platformConfig->AddIntermediateScanFolder();

        // Create the APM
        m_assetProcessorManager = AZStd::make_unique<TestingAssetProcessorManager>(m_platformConfig.get());

        // Cache the db pointer because the TEST_F generates a subclass which can't access this private member
        m_stateData = m_assetProcessorManager->m_stateData;

        // Cache the scanfolder db entry, for convenience
        ASSERT_TRUE(m_stateData->GetScanFolderByPortableKey("folder", m_scanfolder));

        // Configure our mock builder so APM can find the builder and run CreateJobs
        m_builderInfoHandler.CreateBuilderDesc(
            "test", AZ::Uuid::CreateRandom().ToString<QString>(),
            { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) }, {});
        m_builderInfoHandler.BusConnect();

        // Set up the Job Context, required for the PathDependencyManager to do its work
        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_descriptor = AZ::JobManagerComponent::CreateDescriptor();
        m_descriptor->Reflect(m_serializeContext.get());

        m_jobManagerEntity = aznew AZ::Entity{};
        m_jobManagerEntity->CreateComponent<AZ::JobManagerComponent>();
        m_jobManagerEntity->Init();
        m_jobManagerEntity->Activate();

        // Set up a mock disk space responder, required for RCController to process a job
        m_diskSpaceResponder = AZStd::make_unique<::testing::NiceMock<MockDiskSpaceResponder>>();

        ON_CALL(*m_diskSpaceResponder, CheckSufficientDiskSpace(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(true));

        QObject::connect(
            m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetToProcess,
            [this](AssetProcessor::JobDetails jobDetails)
            {
                m_jobDetailsList.push_back(jobDetails);
            });
    }

    void AssetManagerTestingBase::TearDown()
    {
        m_diskSpaceResponder = nullptr;
        m_builderInfoHandler.BusDisconnect();

        AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

        if (m_localFileIo)
        {
            delete m_localFileIo;
            m_localFileIo = nullptr;
            AZ::IO::FileIOBase::SetInstance(nullptr);
        }

        m_jobManagerEntity->Deactivate();
        delete m_jobManagerEntity;

        delete m_descriptor;

        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

        m_stateData.reset();
        m_assetProcessorManager.reset();

        ScopedAllocatorSetupFixture::TearDown();
    }

    void AssetManagerTestingBase::RunFile(int expectedJobCount, int expectedFileCount, int dependencyFileCount)
    {
        m_jobDetailsList.clear();

        m_assetProcessorManager->CheckActiveFiles(expectedFileCount);

        QCoreApplication::processEvents();

        m_assetProcessorManager->CheckActiveFiles(0);
        m_assetProcessorManager->CheckFilesToExamine(expectedFileCount + dependencyFileCount);

        QCoreApplication::processEvents(); // execute ProcessFilesToExamineQueue

        if (expectedJobCount > 0)
        {
            m_assetProcessorManager->CheckJobEntries(expectedFileCount + dependencyFileCount);

            QCoreApplication::processEvents(); // execute CheckForIdle
        }

        ASSERT_EQ(m_jobDetailsList.size(), expectedJobCount + dependencyFileCount);
    }

    void AssetManagerTestingBase::ProcessJob(AssetProcessor::RCController& rcController, const AssetProcessor::JobDetails& jobDetails)
    {
        rcController.JobSubmitted(jobDetails);

        JobSignalReceiver receiver;
        QCoreApplication::processEvents(); // Once to get the job started
        receiver.WaitForFinish(); // Wait for the RCJob to signal it has completed working
        QCoreApplication::processEvents(); // Once more to trigger the JobFinished event
        QCoreApplication::processEvents(); // Again to trigger the Finished event
    }
} // namespace UnitTests
