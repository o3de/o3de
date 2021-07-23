/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/utilities/JobModelTest.h>

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

TEST_F(JobModelUnitTests, Test_RemoveAllJobsBySourceFolder)
{
    VerifyModel(); // verify up front for sanity.

    AssetProcessor::CachedJobInfo* testJobInfo = new AssetProcessor::CachedJobInfo();
    testJobInfo->m_elementId.SetInputAssetName("sourceFolder1/source.txt");
    testJobInfo->m_elementId.SetPlatform("platform");
    testJobInfo->m_elementId.SetJobDescriptor("jobKey");

    testJobInfo->m_jobState = AzToolsFramework::AssetSystem::JobStatus::Completed;
    m_unitTestJobModel->m_cachedJobs.push_back(testJobInfo);
    m_unitTestJobModel->m_cachedJobsLookup.insert(testJobInfo->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::QueueElementID elementId("sourceFolder1/source.txt", "platform", "jobKey");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 6); //last job

    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 7);
    m_unitTestJobModel->OnFolderRemoved("sourceFolder1");

    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 6);
    VerifyModel();

    // make sure sourceFolder1/source.txt is completely gone.
    for (int idx = 0; idx < m_unitTestJobModel->m_cachedJobs.size(); idx++)
    {
        AssetProcessor::CachedJobInfo* jobInfo = m_unitTestJobModel->m_cachedJobs[idx];
        ASSERT_NE(jobInfo->m_elementId.GetInputAssetName(), QString::fromUtf8("sourceFolder1/source.txt"));
    }
}

void JobModelUnitTests::SetUp()
{
    AssetProcessorTest::SetUp();

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
