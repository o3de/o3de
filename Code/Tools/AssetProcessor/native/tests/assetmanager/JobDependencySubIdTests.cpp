/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/assetmanager/JobDependencySubIdTests.h>
#include <native/unittests/UnitTestUtils.h>
#include <QApplication>

namespace UnitTests
{
    void JobDependencySubIdTest::CreateTestData(AZ::u64 hashA, AZ::u64 hashB, bool useSubId)
    {
        using namespace AzToolsFramework::AssetDatabase;

        SourceDatabaseEntry source1{ m_scanfolder.m_scanFolderID, "parent.txt", AZ::Uuid::CreateRandom(), "fingerprint" };
        SourceDatabaseEntry source2{ m_scanfolder.m_scanFolderID, "child.txt", AZ::Uuid::CreateRandom(), "fingerprint" };

        m_parentFile = AZ::IO::Path(m_scanfolder.m_scanFolder) / "parent.txt";
        m_childFile = AZ::IO::Path(m_scanfolder.m_scanFolder) / "child.txt";
        UnitTestUtils::CreateDummyFile(m_parentFile.Native().c_str(), QString("tempdata"));
        UnitTestUtils::CreateDummyFile(m_childFile.Native().c_str(), QString("tempdata"));

        ASSERT_TRUE(m_stateData->SetSource(source1));
        ASSERT_TRUE(m_stateData->SetSource(source2));

        JobDatabaseEntry job1{
            source1.m_sourceID, "Mock Job", 1234, "pc", m_builderInfoHandler.m_builderDescMap.begin()->second.m_busId, AzToolsFramework::AssetSystem::JobStatus::Completed, 999
        };

        ASSERT_TRUE(m_stateData->SetJob(job1));

        ProductDatabaseEntry product1{ job1.m_jobID, 0, "pc/product.txt", m_assetType,
            AZ::Uuid::CreateName("product.txt"), hashA, static_cast<int>(AssetBuilderSDK::ProductOutputFlags::ProductAsset) };
        ProductDatabaseEntry product2{ job1.m_jobID, 777, "pc/product777.txt", m_assetType,
            AZ::Uuid::CreateName("product777.txt"), hashB, static_cast<int>(AssetBuilderSDK::ProductOutputFlags::ProductAsset) };

        ASSERT_TRUE(m_stateData->SetProduct(product1));
        ASSERT_TRUE(m_stateData->SetProduct(product2));

        SourceFileDependencyEntry dependency1{ AZ::Uuid::CreateRandom(),
                                               source2.m_sourceGuid,
                                               PathOrUuid(source1.m_sourceName),
                                               SourceFileDependencyEntry::DEP_JobToJob,
                                               0,
                                                useSubId ? AZStd::to_string(product2.m_subID).c_str() : "" };

        ASSERT_TRUE(m_stateData->SetSourceFileDependency(dependency1));
    }

    void JobDependencySubIdTest::RunTest(bool firstProductChanged, bool secondProductChanged)
    {
        AZ::IO::Path cacheDir(m_databaseLocationListener.GetAssetRootDir());
        cacheDir /= "Cache";
        cacheDir /= "pc";

        AZStd::string productFilename = "product.txt";
        AZStd::string product2Filename = "product777.txt";

        AZStd::string productPath = (cacheDir / productFilename).AsPosix().c_str();
        AZStd::string product2Path = (cacheDir / product2Filename).AsPosix().c_str();

        UnitTestUtils::CreateDummyFile(productPath.c_str(), "unit test file");
        UnitTestUtils::CreateDummyFile(product2Path.c_str(), "unit test file");

        AZ::u64 hashA = firstProductChanged ? 0 : AssetUtilities::GetFileHash(productPath.c_str());
        AZ::u64 hashB = secondProductChanged ? 0 : AssetUtilities::GetFileHash(product2Path.c_str());

        CreateTestData(hashA, hashB, true);

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, m_parentFile.c_str()));
        QCoreApplication::processEvents();

        m_assetProcessorManager->CheckActiveFiles(1);

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
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(productFilename, m_assetType, 0));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(product2Filename, m_assetType, 777));

        m_assetProcessorManager->AssetProcessed(jobDetailsList[0].m_jobEntry, response);

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
