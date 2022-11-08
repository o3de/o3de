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
#include <unittests/UnitTestUtils.h>

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

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.stage1";
        m_testFilePath = (scanFolderDir / testFilename).AsPosix().c_str();

        UnitTestUtils::CreateDummyFile(m_testFilePath.c_str(), "unit test file");

        m_rc = AZStd::make_unique<AssetProcessor::RCController>(1, 1);
        m_rc->SetDispatchPaused(false);

        QObject::connect(
            m_rc.get(),
            &AssetProcessor::RCController::FileFailed,
            [this](auto entryIn)
            {
                m_fileFailed = true;
            });

        QObject::connect(
            m_rc.get(),
            &AssetProcessor::RCController::FileCompiled,
            [this](auto jobEntry, auto response)
            {
                m_fileCompiled = true;
                m_processedJobEntry = jobEntry;
                m_processJobResponse = response;
            });

        m_localFileIo->SetAlias("@log@", (AZ::IO::Path(m_databaseLocationListener.GetAssetRootDir()) / "logs").c_str());
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

    AssetBuilderSDK::CreateJobFunction AssetManagerTestingBase::CreateJobStage(
        const AZStd::string& name, bool commonPlatform, const AZStd::string& sourceDependencyPath)
    {
        using namespace AssetBuilderSDK;

        // Note: capture by copy because we need these to stay around for a long time
        return [name, commonPlatform, sourceDependencyPath]([[maybe_unused]] const CreateJobsRequest& request, CreateJobsResponse& response)
        {
            if (commonPlatform)
            {
                response.m_createJobOutputs.push_back(JobDescriptor{ "fingerprint", name, CommonPlatformName });
            }
            else
            {
                for (const auto& platform : request.m_enabledPlatforms)
                {
                    response.m_createJobOutputs.push_back(JobDescriptor{ "fingerprint", name, platform.m_identifier.c_str() });
                }
            }

            if (!sourceDependencyPath.empty())
            {
                response.m_sourceFileDependencyList.push_back(SourceFileDependency{ sourceDependencyPath, AZ::Uuid::CreateNull() });
            }

            response.m_result = CreateJobsResultCode::Success;
        };
    }

    AssetBuilderSDK::ProcessJobFunction AssetManagerTestingBase::ProcessJobStage(
        const AZStd::string& outputExtension, AssetBuilderSDK::ProductOutputFlags flags, bool outputExtraFile)
    {
        using namespace AssetBuilderSDK;

        // Capture by copy because we need these to stay around a long time
        return [outputExtension, flags, outputExtraFile](const ProcessJobRequest& request, ProcessJobResponse& response)
        {
            AZ::IO::Path outputFile = request.m_sourceFile;
            outputFile.ReplaceExtension(outputExtension.c_str());

            AZ::IO::LocalFileIO::GetInstance()->Copy(
                request.m_fullPath.c_str(), (AZ::IO::Path(request.m_tempDirPath) / outputFile).c_str());

            auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName(outputExtension.c_str()), 1 };

            product.m_outputFlags = flags;
            product.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(product);

            if (outputExtraFile)
            {
                auto extraFilePath =
                    AZ::IO::Path(request.m_tempDirPath) / "z_extra.txt"; // Z prefix to place at end of list when sorting for processing

                UnitTestUtils::CreateDummyFile(extraFilePath.c_str(), "unit test file");

                auto extraProduct = JobProduct{ extraFilePath.c_str(), AZ::Data::AssetType::CreateName("extra"), 2 };

                extraProduct.m_outputFlags = flags;
                extraProduct.m_dependenciesHandled = true;
                response.m_outputProducts.push_back(extraProduct);
            }

            response.m_resultCode = ProcessJobResult_Success;
        };
    }

    void AssetManagerTestingBase::CreateBuilder(
        const char* name,
        const char* inputFilter,
        const char* outputExtension,
        bool createJobCommonPlatform,
        AssetBuilderSDK::ProductOutputFlags outputFlags,
        bool outputExtraFile)
    {
        using namespace AssetBuilderSDK;

        m_builderInfoHandler.CreateBuilderDesc(
            name,
            AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderPattern{ inputFilter, AssetBuilderPattern::Wildcard } },
            CreateJobStage(name, createJobCommonPlatform),
            ProcessJobStage(outputExtension, outputFlags, outputExtraFile),
            "fingerprint");
    }

    AZStd::string AssetManagerTestingBase::MakePath(const char* filename, bool intermediate)
    {
        auto cacheDir = AZ::IO::Path(m_databaseLocationListener.GetAssetRootDir()) / "Cache";

        if (intermediate)
        {
            cacheDir = AssetUtilities::GetIntermediateAssetsFolder(cacheDir);

            return (cacheDir / filename).StringAsPosix();
        }

        return (cacheDir / "pc" / filename).StringAsPosix();
    }

    void AssetManagerTestingBase::CheckProduct(const char* relativePath, bool exists)
    {
        auto expectedProductPath = MakePath(relativePath, false);
        EXPECT_EQ(AZ::IO::SystemFile::Exists(expectedProductPath.c_str()), exists) << expectedProductPath.c_str();
    }

    void AssetManagerTestingBase::CheckIntermediate(const char* relativePath, bool exists)
    {
        auto expectedIntermediatePath = MakePath(relativePath, true);
        EXPECT_EQ(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str()), exists) << expectedIntermediatePath.c_str();
    }

    void AssetManagerTestingBase::ProcessSingleStep(int expectedJobCount, int expectedFileCount, int jobToRun, bool expectSuccess)
    {
        // Reset state
        m_jobDetailsList.clear();
        m_fileCompiled = false;
        m_fileFailed = false;

        RunFile(expectedJobCount, expectedFileCount);

        std::stable_sort(
            m_jobDetailsList.begin(),
            m_jobDetailsList.end(),
            [](const AssetProcessor::JobDetails& a, const AssetProcessor::JobDetails& b) -> bool
            {
                return a.m_jobEntry.m_sourceAssetReference < b.m_jobEntry.m_sourceAssetReference;
            });

        ProcessJob(*m_rc, m_jobDetailsList[jobToRun]);

        if (expectSuccess)
        {
            ASSERT_TRUE(m_fileCompiled);
            m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);
        }
        else
        {
            ASSERT_TRUE(m_fileFailed);
        }
    }

    void AssetManagerTestingBase::ProcessFileMultiStage(
        int endStage, bool doProductOutputCheck, const char* file, int startStage, bool expectAutofail, bool hasExtraFile)
    {
        auto cacheDir = AZ::IO::Path(m_databaseLocationListener.GetAssetRootDir()) / "Cache";
        auto intermediatesDir = AssetUtilities::GetIntermediateAssetsFolder(cacheDir);

        if (file == nullptr)
        {
            file = m_testFilePath.c_str();
        }

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, file));
        QCoreApplication::processEvents();

        for (int i = startStage; i <= endStage; ++i)
        {
            int expectedJobCount = 1;
            int expectedFileCount = 1;
            int jobToRun = 0;

            // If there's an extra file output, it'll only show up after the 1st iteration
            if (i > startStage && hasExtraFile)
            {
                expectedJobCount = 2;
                expectedFileCount = 2;
            }
            else if (expectAutofail)
            {
                expectedJobCount = 2;
                jobToRun = 1;
            }

            ProcessSingleStep(expectedJobCount, expectedFileCount, jobToRun, true);

            if (expectAutofail)
            {
                ASSERT_GT(m_jobDetailsList.size(), 0);
                EXPECT_TRUE(m_jobDetailsList[0].m_autoFail);
            }

            if (i < endStage)
            {
                auto expectedIntermediatePath = intermediatesDir / AZStd::string::format("test.stage%d", i + 1);
                EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str())) << expectedIntermediatePath.c_str();
            }

            // Only first job should have an autofail due to a conflict
            expectAutofail = false;
        }

        m_assetProcessorManager->CheckFilesToExamine(0);
        m_assetProcessorManager->CheckActiveFiles(0);
        m_assetProcessorManager->CheckJobEntries(0);

        if (doProductOutputCheck)
        {
            CheckProduct(AZStd::string::format("test.stage%d", endStage + 1).c_str());
        }
    }
} // namespace UnitTests
