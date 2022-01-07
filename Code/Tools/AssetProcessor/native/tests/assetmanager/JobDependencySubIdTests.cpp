/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCoreApplication>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <native/tests/assetmanager/JobDependencySubIdTests.h>
#include <unittests/UnitTestRunner.h>

namespace UnitTests
{
    bool DatabaseLocationListener::GetAssetDatabaseLocation(AZStd::string& location)
    {
        location = m_databaseLocation;
        return true;
    }

    void MockAssetProcessorManager::CheckActiveFiles(int count)
    {
        ASSERT_EQ(m_activeFiles.size(), count);
    }

    void MockAssetProcessorManager::CheckFilesToExamine(int count)
    {
        ASSERT_EQ(m_filesToExamine.size(), count);
    }

    void MockAssetProcessorManager::CheckJobEntries(int count)
    {
        ASSERT_EQ(m_jobEntries.size(), count);
    }

    void JobDependencySubIdTest::SetUp()
    {
        ScopedAllocatorSetupFixture::SetUp();

        // File IO is needed to hash the files
        if (AZ::IO::FileIOBase::GetInstance() == nullptr)
        {
            m_localFileIo = aznew AZ::IO::LocalFileIO();
            AZ::IO::FileIOBase::SetInstance(m_localFileIo);
        }

        // Specify the database lives in the temp directory
        AZ::IO::Path tempDir(m_tempDir.GetDirectory());
        m_databaseLocationListener.m_databaseLocation = (tempDir / "test_database.sqlite").Native();

        // We need a settings registry in order for APM to figure out the cache path
        m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
        AZ::SettingsRegistry::Register(m_settingsRegistry.get());

        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        m_settingsRegistry->Set(projectPathKey, m_tempDir.GetDirectory());
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

        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_platformConfig->PopulatePlatformsForScanFolder(platforms);

        m_platformConfig->AddScanFolder(AssetProcessor::ScanFolderInfo{
            (tempDir/"folder").c_str(), "folder", "folder", false, true, platforms});

        // Create the APM
        m_assetProcessorManager = AZStd::make_unique<MockAssetProcessorManager>(m_platformConfig.get());

        // Cache the db pointer because the TEST_F generates a subclass which can't access this private member
        m_stateData = m_assetProcessorManager->m_stateData;

        // Configure our mock builder so APM can find the builder and run CreateJobs
        m_builderInfoHandler.m_builderDesc = m_builderInfoHandler.CreateBuilderDesc("test", AZ::Uuid::CreateRandom().ToString<QString>(), { AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard) });
        m_builderInfoHandler.BusConnect();
    }

    void JobDependencySubIdTest::TearDown()
    {
        m_builderInfoHandler.BusDisconnect();

        AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

        if (m_localFileIo)
        {
            delete m_localFileIo;
            m_localFileIo = nullptr;
            AZ::IO::FileIOBase::SetInstance(nullptr);
        }

        ScopedAllocatorSetupFixture::TearDown();
    }

    void JobDependencySubIdTest::CreateTestData(AZ::u64 hashA, AZ::u64 hashB, bool useSubId)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AZ::IO::Path tempDir(m_tempDir.GetDirectory());
        m_scanfolder = { (tempDir / "folder").c_str(), "folder", "folder", 0 };

        ASSERT_TRUE(m_stateData->SetScanFolder(m_scanfolder));

        SourceDatabaseEntry source1{ m_scanfolder.m_scanFolderID, "parent.txt", AZ::Uuid::CreateRandom(), "fingerprint" };
        SourceDatabaseEntry source2{ m_scanfolder.m_scanFolderID, "child.txt", AZ::Uuid::CreateRandom(), "fingerprint" };

        m_parentFile = AZ::IO::Path(m_scanfolder.m_scanFolder) / "parent.txt";
        m_childFile = AZ::IO::Path(m_scanfolder.m_scanFolder) / "child.txt";
        UnitTestUtils::CreateDummyFile(m_parentFile.Native().c_str(), QString("tempdata"));
        UnitTestUtils::CreateDummyFile(m_childFile.Native().c_str(), QString("tempdata"));

        ASSERT_TRUE(m_stateData->SetSource(source1));
        ASSERT_TRUE(m_stateData->SetSource(source2));

        JobDatabaseEntry job1{
            source1.m_sourceID, "Mock Job", 1234, "pc", m_builderInfoHandler.m_builderDesc.m_busId, AzToolsFramework::AssetSystem::JobStatus::Completed, 999
        };

        ASSERT_TRUE(m_stateData->SetJob(job1));

        ProductDatabaseEntry product1{
            job1.m_jobID, 0, "product.txt", m_assetType, AZ::Uuid::CreateName("product.txt"),
            hashA
        };
        ProductDatabaseEntry product2{ job1.m_jobID,
                                       777,
                                       "product777.txt",
                                       m_assetType,
                                       AZ::Uuid::CreateName("product777.txt"),
                                       hashB };

        ASSERT_TRUE(m_stateData->SetProduct(product1));
        ASSERT_TRUE(m_stateData->SetProduct(product2));

        SourceFileDependencyEntry dependency1{ AZ::Uuid::CreateRandom(),
                                               source2.m_sourceName.c_str(),
                                               source1.m_sourceName.c_str(),
                                               SourceFileDependencyEntry::DEP_JobToJob,
                                               0,
                                                useSubId ? AZStd::to_string(product2.m_subID).c_str() : "" };

        ASSERT_TRUE(m_stateData->SetSourceFileDependency(dependency1));
    }

    void JobDependencySubIdTest::RunTest(bool firstProductChanged, bool secondProductChanged)
    {
        AZ::IO::Path cacheDir(m_tempDir.GetDirectory());
        cacheDir /= "Cache";

        AZStd::string productPath = (cacheDir / "product.txt").AsPosix().c_str();
        AZStd::string product2Path = (cacheDir / "product777.txt").AsPosix().c_str();

        UnitTestUtils::CreateDummyFile(productPath.c_str(), "unit test file");
        UnitTestUtils::CreateDummyFile(product2Path.c_str(), "unit test file");

        AZ::u64 hashA = firstProductChanged ? 0 : AssetUtilities::GetFileHash(productPath.c_str());
        AZ::u64 hashB = secondProductChanged ? 0 : AssetUtilities::GetFileHash(product2Path.c_str());

        CreateTestData(hashA, hashB, true);

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, m_parentFile.c_str()));
        QCoreApplication::processEvents();

        m_assetProcessorManager->CheckActiveFiles(1);

        // AssessModifiedFile is going to set up a OneShotTimer with a 1ms delay on it.  We have to wait a short time for that timer to
        // elapse before we can process that event. If we use the alternative processEvents that loops for X milliseconds we could
        // accidentally process too many events.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        QCoreApplication::processEvents();

        m_assetProcessorManager->CheckActiveFiles(0);
        m_assetProcessorManager->CheckFilesToExamine(1);

        QCoreApplication::processEvents();

        m_assetProcessorManager->CheckJobEntries(1);

        AZStd::vector<AssetProcessor::JobDetails> jobDetailsList;
        QObject::connect(
            m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetToProcess,
            [&jobDetailsList](AssetProcessor::JobDetails jobDetails)
            {
                jobDetailsList.push_back(jobDetails);
            });

        QCoreApplication::processEvents();

        ASSERT_EQ(jobDetailsList.size(), 1);

        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productPath, m_assetType, 0));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2Path, m_assetType, 777));

        m_assetProcessorManager->AssetProcessed(jobDetailsList[0].m_jobEntry, response);

        ASSERT_TRUE(true);

        // We're only really interested in ActiveFiles but check the others to be sure
        m_assetProcessorManager->CheckFilesToExamine(0);
        m_assetProcessorManager->CheckActiveFiles(secondProductChanged ? 1 : 0); // The 2nd product is the one we have a dependency on, only if that changed should we see the other file process
        m_assetProcessorManager->CheckJobEntries(0);
    }

    TEST_F(JobDependencySubIdTest, RegularJobDependency_NoSubId_ProcessDependent)
    {
        CreateTestData(0, 0, false);

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, m_parentFile.c_str()));
        QCoreApplication::processEvents();

        m_assetProcessorManager->CheckActiveFiles(1);

        // AssessModifiedFile is going to set up a OneShotTimer with a 1ms delay on it.  We have to wait a short time for that timer to
        // elapse before we can process that event. If we use the alternative processEvents that loops for X milliseconds we could
        // accidentally process too many events.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        QCoreApplication::processEvents();

        m_assetProcessorManager->CheckActiveFiles(0);
        m_assetProcessorManager->CheckFilesToExamine(2);
    }

    TEST_F(JobDependencySubIdTest, JobDependencyWithSubID_SameHash_DependentDoesNotProcess)
    {
        RunTest(false, false);
    }

    TEST_F(JobDependencySubIdTest, JobDependencyWithSubID_DifferentHashOnCorrectSubId_DependentProcesses)
    {
        RunTest(false, true);
    }

    TEST_F(JobDependencySubIdTest, JobDependencyWithSubID_BothHashesDifferent_DependentProcesses)
    {
        // Should be the same result as above but check just in case
        RunTest(true, true);
    }

    TEST_F(JobDependencySubIdTest, JobDependencyWithSubID_DifferentHashOnIncorrectSubId_DependentDoesNotProcess)
    {
        RunTest(true, false);
    }
}
