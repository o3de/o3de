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
#include <AzCore/Utils/Utils.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>

namespace UnitTests
{
    const char* JOB_PROCESS_FAIL_TEXT = "AUTO_FAIL_JOB";

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
        LeakDetectionFixture::SetUp();

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

        // Make sure that the entire system doesn't somehow find the "real" project but instead finds our fake project folder. 
        m_settingsRegistry->Set("/O3DE/Runtime/Internal/project_root_scan_up_path", assetRootDir.c_str());

        // The engine is actually pretty good at finding the real project folder and tries to do so a number of ways, including
        // overwriting all the keys we're about to set if we allow it to.
        auto projectPathKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        m_settingsRegistry->Set(projectPathKey, assetRootDir.c_str());

        // we need to also set up the cache root:
        auto cacheRootPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);
        m_settingsRegistry->Set(cacheRootPathKey, (assetRootDir / "Cache").c_str());

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*m_settingsRegistry);

        // We need a QCoreApplication set up in order for QCoreApplication::processEvents to function
        m_qApp = AZStd::make_unique<QCoreApplication>(m_argc, m_argv);
        qRegisterMetaType<AssetProcessor::JobEntry>("JobEntry");
        qRegisterMetaType<AssetBuilderSDK::ProcessJobResponse>("ProcessJobResponse");
        qRegisterMetaType<AZStd::string>("AZStd::string");
        qRegisterMetaType<AssetProcessor::AssetScanningStatus>("AssetProcessor::AssetScanningStatus");
        qRegisterMetaType<QSet<AssetProcessor::AssetFileInfo>>("QSet<AssetFileInfo>");
        qRegisterMetaType<AssetProcessor::SourceAssetReference>("SourceAssetReference");

        // Platform config with an enabled platform and scanfolder required by APM to function and find the files
        m_platformConfig = AZStd::make_unique<AssetProcessor::PlatformConfiguration>();
        m_platformConfig->EnablePlatform(AssetBuilderSDK::PlatformInfo{ "pc", { "test" } });

        m_platformConfig->EnableCommonPlatform();

        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_platformConfig->PopulatePlatformsForScanFolder(platforms);
        m_platformConfig->ReadMetaDataFromSettingsRegistry();

        SetupScanfolders(assetRootDir, platforms);

        m_platformConfig->AddIntermediateScanFolder();

        // Create the APM
        m_assetProcessorManager = AZStd::make_unique<TestingAssetProcessorManager>(m_platformConfig.get());
        m_assetProcessorManager->SetMetaCreationDelay(0);

        // Cache the db pointer because the TEST_F generates a subclass which can't access this private member
        m_stateData = m_assetProcessorManager->m_stateData;

        // Cache the scanfolder db entry, for convenience
        ASSERT_TRUE(m_stateData->GetScanFolderByPortableKey("folder", m_scanfolder));

        // Configure our mock builder so APM can find the builder and run CreateJobs
        m_builderInfoHandler.CreateBuilderDesc(
            "test", AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) }, {});
        m_builderInfoHandler.BusConnect();

        // Set up the Job Context, required for the PathDependencyManager to do its work
        // Set up serialize and json context
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
        m_componentApplication = AZStd::make_unique<testing::NiceMock<MockComponentApplication>>();

        using namespace testing;

        ON_CALL(*m_componentApplication.get(), GetSerializeContext()).WillByDefault(Return(m_serializeContext.get()));
        ON_CALL(*m_componentApplication.get(), GetJsonRegistrationContext()).WillByDefault(Return(m_jsonRegistrationContext.get()));
        ON_CALL(*m_componentApplication.get(), AddEntity(_)).WillByDefault(Return(true));

        AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());

        m_descriptor = AZ::JobManagerComponent::CreateDescriptor();
        m_descriptor->Reflect(m_serializeContext.get());

        m_jobManagerEntity = aznew AZ::Entity{};
        m_jobManagerEntity->CreateComponent<AZ::JobManagerComponent>();
        m_jobManagerEntity->Init();
        m_jobManagerEntity->Activate();

        AzToolsFramework::MetadataManager::Reflect(m_serializeContext.get());
        AzToolsFramework::UuidUtilComponent::Reflect(m_serializeContext.get());

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

        AZ::Utils::WriteFile("unit test file", m_testFilePath);

        m_rc = AZStd::make_unique<TestingRCController>(1, 1);
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

        AZ::IO::FileIOBase::GetInstance()->SetAlias("@log@", (AZ::IO::Path(m_databaseLocationListener.GetAssetRootDir()) / "logs").c_str());
    }

    void AssetManagerTestingBase::TearDown()
    {
        m_diskSpaceResponder = nullptr;
        m_builderInfoHandler.BusDisconnect();

        AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

        m_jsonRegistrationContext->EnableRemoveReflection();
        AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());
        m_jsonRegistrationContext->DisableRemoveReflection();

        m_jsonRegistrationContext.reset();
        m_serializeContext.reset();

        if (m_localFileIo)
        {
            delete m_localFileIo;
            m_localFileIo = nullptr;
            AZ::IO::FileIOBase::SetInstance(nullptr);
        }

        m_jobManagerEntity->Deactivate();
        delete m_jobManagerEntity;

        delete m_descriptor;

        m_stateData.reset();
        m_assetProcessorManager.reset();

        LeakDetectionFixture::TearDown();
    }

    void AssetManagerTestingBase::SetupScanfolders(AZ::IO::Path assetRootDir, const AZStd::vector<AssetBuilderSDK::PlatformInfo>& platforms)
    {
        m_platformConfig->AddScanFolder(
            AssetProcessor::ScanFolderInfo{ (assetRootDir / "folder").c_str(), "folder", "folder", false, true, platforms });
    }

    void AssetManagerTestingBase::RunFile(int expectedJobCount, int expectedFileCount, int dependencyFileCount)
    {
        m_jobDetailsList.clear();

        m_assetProcessorManager->CheckActiveFiles(expectedFileCount);

        AZStd::atomic_bool delayed = false;

        QObject::connect(
            m_assetProcessorManager.get(),
            &AssetProcessor::AssetProcessorManager::ProcessingDelayed,
            [&delayed](QString filePath)
            {
                delayed = true;
            });

        QObject::connect(
            m_assetProcessorManager.get(),
            &AssetProcessor::AssetProcessorManager::ProcessingResumed,
            [&delayed](QString filePath)
            {
                delayed = false;
            });

        QCoreApplication::processEvents(); // execute CheckSource

        if (delayed)
        {
            // Wait for the QTimer to elapse.  This should be a very quick, sub 10ms wait.
            // Add 5ms just to be sure the required time has elapsed.
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(MetadataProcessingDelayMs + 5));

            ASSERT_TRUE(delayed);

            QCoreApplication::processEvents(); // Process the timer

            // Sometimes the above processEvents runs CheckSource
            if (delayed)
            {
                QCoreApplication::processEvents(); // execute CheckSource again
            }

            ASSERT_FALSE(delayed);
        }

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
        UnitTests::JobSignalReceiver receiver;
        WaitForNextJobToProcess(receiver);
    }

    void AssetManagerTestingBase::WaitForNextJobToProcess(UnitTests::JobSignalReceiver &receiver)
    {
        QCoreApplication::processEvents(); // RCController::DispatchJobsImpl : Once to get the job started
        receiver.WaitForFinish(); // Wait for the RCJob to signal it has completed working
        QCoreApplication::processEvents(); // RCJob::Finished : Once more to trigger the JobFinished event
        QCoreApplication::processEvents(); // RCController::FinishJob : Again to trigger the Finished event
    }

    AssetBuilderSDK::CreateJobFunction AssetManagerTestingBase::CreateJobStage(
        const AZStd::string& name, bool commonPlatform, const AzToolsFramework::AssetDatabase::PathOrUuid& sourceDependency)
    {
        using namespace AssetBuilderSDK;

        // Note: capture by copy because we need these to stay around for a long time
        return [name, commonPlatform, sourceDependency]([[maybe_unused]] const CreateJobsRequest& request, CreateJobsResponse& response)
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

            if (sourceDependency)
            {
                response.m_sourceFileDependencyList.push_back(
                    SourceFileDependency{ sourceDependency.IsUuid() ? "" : sourceDependency.GetPath(), sourceDependency.IsUuid() ? sourceDependency.GetUuid() : AZ::Uuid::CreateNull() });
            }

            response.m_result = CreateJobsResultCode::Success;
        };
    }

    AssetBuilderSDK::ProcessJobFunction AssetManagerTestingBase::ProcessJobStage(
        const AZStd::string& outputExtension, AssetBuilderSDK::ProductOutputFlags flags, bool outputExtraFile, AZ::Data::AssetId dependencyId)
    {
        using namespace AssetBuilderSDK;

        // Capture by copy because we need these to stay around a long time
        return [outputExtension, flags, outputExtraFile, dependencyId](const ProcessJobRequest& request, ProcessJobResponse& response)
        {
            // If tests put the text "FAIL_JOB" at the beginning of the source file, then fail this job instead.
            // This lets tests easily handle cases where they want job processing to fail.
            auto readResult = AZ::Utils::ReadFile<AZStd::string>(request.m_fullPath, AZStd::numeric_limits<size_t>::max());
            // Don't fail if the read fails, there may be existing tests that create unreadable files.
            if (readResult.IsSuccess())
            {
                if (readResult.GetValue().starts_with(JOB_PROCESS_FAIL_TEXT))
                {
                    response.m_resultCode = ProcessJobResult_Failed;
                    return;
                }
            }

            AZ::IO::FixedMaxPath outputFile = AZ::IO::FixedMaxPath(request.m_sourceFile);
            outputFile.ReplaceExtension(outputExtension.c_str());
            outputFile = outputFile.Filename();

            AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Copy(
                request.m_fullPath.c_str(), (AZ::IO::FixedMaxPath(request.m_tempDirPath) / outputFile).c_str());

            EXPECT_TRUE(result);

            auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName(outputExtension.c_str()), AssetSubId };

            product.m_outputFlags = flags;
            product.m_dependenciesHandled = true;
            if (dependencyId.IsValid())
            {
                product.m_dependencies.push_back(AssetBuilderSDK::ProductDependency(dependencyId, {}));
            }
            response.m_outputProducts.push_back(product);

            if (outputExtraFile)
            {
                auto extraFilePath =
                    AZ::IO::Path(request.m_tempDirPath) / "z_extra.txt"; // Z prefix to place at end of list when sorting for processing

                AZ::Utils::WriteFile("unit test file", extraFilePath.Native());

                auto extraProduct = JobProduct{ extraFilePath.c_str(), AZ::Data::AssetType::CreateName("extra"), ExtraAssetSubId };

                extraProduct.m_outputFlags = flags;
                extraProduct.m_dependenciesHandled = true;
                response.m_outputProducts.push_back(extraProduct);
            }

            response.m_resultCode = ProcessJobResult_Success;
        };
    }

    const char* AssetManagerTestingBase::GetJobProcessFailText()
    {
        return JOB_PROCESS_FAIL_TEXT;
    }

    AZ::IO::Path AssetManagerTestingBase::GetCacheDir()
    {
        return AZ::IO::Path(m_databaseLocationListener.GetAssetRootDir()) / "Cache";
    }

    AZ::IO::FixedMaxPath AssetManagerTestingBase::GetIntermediateAssetsDir()
    {
        return AssetUtilities::GetIntermediateAssetsFolder(GetCacheDir());
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

    void AssetManagerTestingBase::SetCatalogToUpdateOnJobCompletion()
    {
        using namespace AssetBuilderSDK;
        QObject::connect(
            m_rc.get(),
            &AssetProcessor::RCController::FileCompiled,
            [this](AssetProcessor::JobEntry entry, AssetBuilderSDK::ProcessJobResponse response)
            {
                QMetaObject::invokeMethod(m_rc.get(), "OnAddedToCatalog", Qt::QueuedConnection, Q_ARG(AssetProcessor::JobEntry, entry));
            });
    }

    AZStd::string AssetManagerTestingBase::MakePath(const char* filename, bool intermediate)
    {
        auto cacheDir = GetCacheDir();

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

    void AssetManagerTestingBase::CheckIntermediate(const char* relativePath, bool exists, bool hasMetadata)
    {
        auto expectedIntermediatePath = MakePath(relativePath, true);
        auto expectedMetadataPath = AzToolsFramework::MetadataManager::ToMetadataPath(expectedIntermediatePath);

        EXPECT_EQ(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str()), exists) << expectedIntermediatePath.c_str();
        EXPECT_EQ(AZ::IO::SystemFile::Exists(expectedMetadataPath.c_str()), hasMetadata) << expectedMetadataPath.c_str();
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
        int endStage, bool doProductOutputCheck, AssetProcessor::SourceAssetReference sourceAsset, int startStage, bool expectAutofail, bool hasExtraFile)
    {
        if (!sourceAsset)
        {
            sourceAsset = AssetProcessor::SourceAssetReference(m_testFilePath.c_str());
        }

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, sourceAsset.AbsolutePath().c_str()));
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
                auto expectedIntermediatePath =
                    MakePath(sourceAsset.RelativePath().ReplaceExtension(AZStd::string::format("stage%d", i + 1).c_str()).c_str(), true);
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
            CheckProduct(sourceAsset.RelativePath().ReplaceExtension(AZStd::string::format("stage%d", endStage + 1).c_str()).c_str());
        }
    }
} // namespace UnitTests
