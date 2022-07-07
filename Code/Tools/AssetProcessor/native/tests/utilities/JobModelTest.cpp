/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/utilities/JobModelTest.h>
#include <AzCore/std/string/string.h>

TEST_F(JobModelUnitTests, Test_RemoveMiddleJob)
{
    VerifyModel(); // verify up front for sanity.

    AzToolsFramework::AssetSystem::JobInfo jobInfo;
    jobInfo.m_sourceFile = "source2.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId("source2.txt", "platform", "jobKey");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 1); // second job
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 6);
    m_unitTestJobModel->OnJobRemoved(jobInfo);
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 5);
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_EQ(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    AssetProcessor::CachedJobInfo* cachedJobInfo = m_unitTestJobModel->m_cachedJobs[jobIndex];
    ASSERT_EQ(cachedJobInfo->m_elementId.GetInputAssetName(), QString("source3.txt"));
    // Checking index of last job
    elementId.SetInputAssetName("source6.txt");
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 4);
    VerifyModel();
}

TEST_F(JobModelUnitTests, Test_RemoveFirstJob)
{
    VerifyModel(); // verify up front for sanity.

    AzToolsFramework::AssetSystem::JobInfo jobInfo;
    jobInfo.m_sourceFile = "source1.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId("source1.txt", "platform", "jobKey");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 0); //first job
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 6);
    m_unitTestJobModel->OnJobRemoved(jobInfo);
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 5);
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_EQ(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    AssetProcessor::CachedJobInfo* cachedJobInfo = m_unitTestJobModel->m_cachedJobs[jobIndex];
    ASSERT_EQ(cachedJobInfo->m_elementId.GetInputAssetName(), QString("source2.txt"));
    // Checking index of last job
    elementId.SetInputAssetName("source6.txt");
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 4);
    VerifyModel();
}

TEST_F(JobModelUnitTests, Test_RemoveLastJob)
{
    VerifyModel(); // verify up front for sanity.

    AzToolsFramework::AssetSystem::JobInfo jobInfo;
    jobInfo.m_sourceFile = "source6.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId("source6.txt", "platform", "jobKey");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 5); //last job
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 6);
    m_unitTestJobModel->OnJobRemoved(jobInfo);
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 5);
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_EQ(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    AssetProcessor::CachedJobInfo* cachedJobInfo = m_unitTestJobModel->m_cachedJobs[jobIndex - 1];
    ASSERT_EQ(cachedJobInfo->m_elementId.GetInputAssetName(), QString("source5.txt"));
    // Checking index of first job
    elementId.SetInputAssetName("source1.txt");
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 0);
    VerifyModel();
}


TEST_F(JobModelUnitTests, Test_RemoveAllJobsBySource)
{
    VerifyModel(); // verify up front for sanity.

    AssetProcessor::CachedJobInfo* jobInfo1 = new AssetProcessor::CachedJobInfo();
    jobInfo1->m_elementId.SetInputAssetName("source3.txt"); // this is the second job for this source - the fixture creates one
    jobInfo1->m_elementId.SetPlatform("platform_2"); // differing job keys
    jobInfo1->m_elementId.SetJobDescriptor("jobKey_3"); // differing descriptor

    jobInfo1->m_jobState = AzToolsFramework::AssetSystem::JobStatus::Completed;
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo1);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo1->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::QueueElementID elementId("source3.txt", "platform_2", "jobKey_3");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 6); //last job

    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 7);
    m_unitTestJobModel->OnSourceRemoved("source3.txt");

    // both  sources should be removed!
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 5);
    VerifyModel();

    // make sure source3 is completely gone.
    for (int idx = 0; idx < m_unitTestJobModel->m_cachedJobs.size(); idx++)
    {
        AssetProcessor::CachedJobInfo* jobInfo = m_unitTestJobModel->m_cachedJobs[idx];
        ASSERT_NE(jobInfo->m_elementId.GetInputAssetName(), QString::fromUtf8("source3.txt"));
    }
}

TEST_F(JobModelUnitTests, Test_PopulateJobsFromDatabase)
{

}

void JobModelUnitTests::SetUp()
{
    AssetProcessorTest::SetUp();

    {
        //! Setup Asset Database connection
        using namespace testing;
        m_data.reset(new StaticData());
        m_data->m_databaseLocationListener.BusConnect();
        ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
            .WillByDefault(DoAll(SetArgReferee<0>(":memory:"), Return(true)));
        m_data->m_connection.ClearData();
    }

    m_unitTestJobModel = new UnitTestJobModel();
    AssetProcessor::CachedJobInfo* jobInfo1 = new AssetProcessor::CachedJobInfo();
    jobInfo1->m_elementId.SetInputAssetName("source1.txt");
    jobInfo1->m_elementId.SetPlatform("platform");
    jobInfo1->m_elementId.SetJobDescriptor("jobKey");
    jobInfo1->m_jobState = AzToolsFramework::AssetSystem::JobStatus::Completed;
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo1);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo1->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo2 = new AssetProcessor::CachedJobInfo();
    jobInfo2->m_elementId.SetInputAssetName("source2.txt");
    jobInfo2->m_elementId.SetPlatform("platform");
    jobInfo2->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo2);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo2->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo3 = new AssetProcessor::CachedJobInfo();
    jobInfo3->m_elementId.SetInputAssetName("source3.txt");
    jobInfo3->m_elementId.SetPlatform("platform");
    jobInfo3->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo3);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo3->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo4 = new AssetProcessor::CachedJobInfo();
    jobInfo4->m_elementId.SetInputAssetName("source4.txt");
    jobInfo4->m_elementId.SetPlatform("platform");
    jobInfo4->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo4);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo4->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo5 = new AssetProcessor::CachedJobInfo();
    jobInfo5->m_elementId.SetInputAssetName("source5.txt");
    jobInfo5->m_elementId.SetPlatform("platform");
    jobInfo5->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo5);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo5->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo6 = new AssetProcessor::CachedJobInfo();
    jobInfo6->m_elementId.SetInputAssetName("source6.txt");
    jobInfo6->m_elementId.SetPlatform("platform");
    jobInfo6->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo6);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo6->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));
}

void JobModelUnitTests::TearDown()
{
    delete m_unitTestJobModel;

    m_data->m_databaseLocationListener.BusDisconnect();
    m_data.reset();

    AssetProcessorTest::TearDown();
}

void JobModelUnitTests::VerifyModel()
{
    // Every job should exist in the lookup map as well.
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), m_unitTestJobModel->m_cachedJobsLookup.size());

    // every job in the vector should have a corresponding element in the lookup table.
    for (int idx = 0; idx < m_unitTestJobModel->m_cachedJobs.size(); idx++)
    {
        AssetProcessor::CachedJobInfo* jobInfo = m_unitTestJobModel->m_cachedJobs[idx];
        auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(jobInfo->m_elementId);
        ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    }

    // this tests the other direction - every job in the lookup table should map to a job in the vector
    // we also verify that its the appropriate job and not an off-by-one type of problem.
    for (const auto& key : m_unitTestJobModel->m_cachedJobsLookup.keys())
    {
        int expectedIndex = m_unitTestJobModel->m_cachedJobsLookup[key];
        ASSERT_LT(expectedIndex, m_unitTestJobModel->m_cachedJobs.size());
        ASSERT_EQ(m_unitTestJobModel->m_cachedJobs[expectedIndex]->m_elementId, key);
    }
}

void JobModelUnitTests::PopulateDatabaseTestData()
{
    using namespace AzToolsFramework::AssetDatabase;
    ScanFolderDatabaseEntry scanFolderEntry;
    SourceDatabaseEntry sourceEntry;
    const AZStd::string sourceName{ "theFile.fbx" };
    AZStd::vector<JobDatabaseEntry> jobEntries;
    StatDatabaseEntry statEntry;

    scanFolderEntry = { "c:/O3DE/dev", "dev", "rootportkey" };
    ASSERT_TRUE(m_data->m_connection.SetScanFolder(scanFolderEntry));
    sourceEntry = { scanFolderEntry.m_scanFolderID, sourceName.c_str(), AZ::Uuid::CreateRandom(), "AFPAFPAFP1" };
    ASSERT_TRUE(m_data->m_connection.SetSource(sourceEntry));

    jobEntries.emplace_back(
        sourceEntry.m_sourceID, "jobKey1", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1);
    jobEntries.emplace_back(
        sourceEntry.m_sourceID, "jobKey2", 456, "linux", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Failed, 2);
    jobEntries.emplace_back(
        sourceEntry.m_sourceID, "jobKey3", 789, "mac", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 3);
    for (auto& jobEntry : jobEntries)
    {
        ASSERT_TRUE(m_data->m_connection.SetJob(jobEntry));
    }

    for (const auto& jobEntry : jobEntries)
    {
        statEntry = { "ProcessJobs," + sourceName + "," + jobEntry.m_jobKey + "," + jobEntry.m_platform,
                      aznumeric_cast<AZ::s64>(jobEntry.m_fingerprint),
                      aznumeric_cast<AZ::s64>(jobEntry.m_jobRunKey) };
        ASSERT_TRUE(m_data->m_connection.ReplaceStat(statEntry));
    }
}
