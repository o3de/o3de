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
    jobInfo.m_watchFolder = "c:/test";
    jobInfo.m_sourceFile = "source2.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId(AssetProcessor::SourceAssetReference("c:/test/source2.txt"), "platform", "jobKey");
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
    ASSERT_EQ(cachedJobInfo->m_elementId.GetSourceAssetReference().AbsolutePath().Native(), "c:/test/source3.txt");
    // Checking index of last job
    elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source6.txt"));
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
    jobInfo.m_watchFolder = "c:/test";
    jobInfo.m_sourceFile = "source1.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId(AssetProcessor::SourceAssetReference ("c:/test/source1.txt"), "platform", "jobKey");
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
    ASSERT_EQ(cachedJobInfo->m_elementId.GetSourceAssetReference().AbsolutePath().Native(), "c:/test/source2.txt");
    // Checking index of last job
    elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source6.txt"));
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
    jobInfo.m_watchFolder = "c:/test";
    jobInfo.m_sourceFile = "source6.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId(AssetProcessor::SourceAssetReference("c:/test/source6.txt"), "platform", "jobKey");
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
    ASSERT_EQ(cachedJobInfo->m_elementId.GetSourceAssetReference().AbsolutePath().Native(), "c:/test/source5.txt");
    // Checking index of first job
    elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source1.txt"));
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
    jobInfo1->m_elementId.SetSourceAssetReference(
        AssetProcessor::SourceAssetReference("c:/test/source3.txt")); // this is the second job for this source - the fixture creates one
    jobInfo1->m_elementId.SetPlatform("platform_2"); // differing job keys
    jobInfo1->m_elementId.SetJobDescriptor("jobKey_3"); // differing descriptor

    jobInfo1->m_jobState = AzToolsFramework::AssetSystem::JobStatus::Completed;
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo1);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo1->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::QueueElementID elementId(AssetProcessor::SourceAssetReference("c:/test/source3.txt"), "platform_2", "jobKey_3");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 6); //last job

    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 7);
    m_unitTestJobModel->OnSourceRemoved(AssetProcessor::SourceAssetReference("c:/test/source3.txt"));

    // both  sources should be removed!
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 5);
    VerifyModel();

    // make sure source3 is completely gone.
    for (int idx = 0; idx < m_unitTestJobModel->m_cachedJobs.size(); idx++)
    {
        AssetProcessor::CachedJobInfo* jobInfo = m_unitTestJobModel->m_cachedJobs[idx];
        ASSERT_NE(jobInfo->m_elementId.GetSourceAssetReference().AbsolutePath().Native(), "c:/test/source3.txt");
    }
}

TEST_F(JobModelUnitTests, Test_PopulateJobsFromDatabase)
{
    VerifyModel(); // verify up front for sanity.

    CreateDatabaseTestData();
    m_unitTestJobModel->PopulateJobsFromDatabase();

    for (const auto& jobEntry : m_data->m_jobEntries)
    {
        AssetProcessor::QueueElementID elementId(AssetProcessor::SourceAssetReference("c:/test", m_data->m_sourceName.c_str()), jobEntry.m_platform.c_str(), jobEntry.m_jobKey.c_str());
        auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
        ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());

        //! expect the three jobs from database are populated into m_unitTestJobModel
        EXPECT_EQ(m_unitTestJobModel->m_cachedJobs.at(iter.value())->m_elementId, elementId);

        //! and that they have a valid m_processDuration, which is set to be equivalent to jobEntry.m_fingerprint
        EXPECT_EQ(
            m_unitTestJobModel->m_cachedJobs.at(iter.value())->m_processDuration.msecsSinceStartOfDay(),
            aznumeric_cast<int>(jobEntry.m_fingerprint));
    }

    // expect one AZ_Warning emitted, because we have one stat entry in the database that has 5 tokens.
    m_errorAbsorber->ExpectWarnings(1);

    VerifyModel();
}

void JobModelUnitTests::SetUp()
{
    AssetProcessorTest::SetUp();

    {
        using namespace testing;
        m_data.reset(new StaticData());

        m_data->m_connection.ClearData();
    }

    m_unitTestJobModel = new UnitTestJobModel();
    AssetProcessor::CachedJobInfo* jobInfo1 = new AssetProcessor::CachedJobInfo();
    jobInfo1->m_elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source1.txt"));
    jobInfo1->m_elementId.SetPlatform("platform");
    jobInfo1->m_elementId.SetJobDescriptor("jobKey");
    jobInfo1->m_jobState = AzToolsFramework::AssetSystem::JobStatus::Completed;
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo1);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo1->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo2 = new AssetProcessor::CachedJobInfo();
    jobInfo2->m_elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source2.txt"));
    jobInfo2->m_elementId.SetPlatform("platform");
    jobInfo2->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo2);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo2->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo3 = new AssetProcessor::CachedJobInfo();
    jobInfo3->m_elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source3.txt"));
    jobInfo3->m_elementId.SetPlatform("platform");
    jobInfo3->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo3);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo3->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo4 = new AssetProcessor::CachedJobInfo();
    jobInfo4->m_elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source4.txt"));
    jobInfo4->m_elementId.SetPlatform("platform");
    jobInfo4->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo4);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo4->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo5 = new AssetProcessor::CachedJobInfo();
    jobInfo5->m_elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source5.txt"));
    jobInfo5->m_elementId.SetPlatform("platform");
    jobInfo5->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo5);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo5->m_elementId, aznumeric_caster(m_unitTestJobModel->m_cachedJobs.size() - 1));

    AssetProcessor::CachedJobInfo* jobInfo6 = new AssetProcessor::CachedJobInfo();
    jobInfo6->m_elementId.SetSourceAssetReference(AssetProcessor::SourceAssetReference("c:/test/source6.txt"));
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

void JobModelUnitTests::CreateDatabaseTestData()
{
    //! This method puts jobs and ProcessJob metrics into the database.

    using namespace AzToolsFramework::AssetDatabase;
    ScanFolderDatabaseEntry scanFolderEntry;
    SourceDatabaseEntry sourceEntry;
    StatDatabaseEntry statEntry;

    scanFolderEntry = { "c:/test", "dev", "rootportkey" };
    ASSERT_TRUE(m_data->m_connection.SetScanFolder(scanFolderEntry));
    sourceEntry = { scanFolderEntry.m_scanFolderID, m_data->m_sourceName.c_str(), AZ::Uuid::CreateRandom(), "AFPAFPAFP1" };
    ASSERT_TRUE(m_data->m_connection.SetSource(sourceEntry));

    //! Insert job entries
    m_data->m_jobEntries.clear();
    m_data->m_jobEntries.emplace_back(
        sourceEntry.m_sourceID, "jobKey1", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1);
    m_data->m_jobEntries.emplace_back(
        sourceEntry.m_sourceID, "jobKey2", 456, "linux", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Failed, 2);
    m_data->m_jobEntries.emplace_back(
        sourceEntry.m_sourceID, "jobKey3", 789, "mac", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 3);
    for (auto& jobEntry : m_data->m_jobEntries)
    {
        ASSERT_TRUE(m_data->m_connection.SetJob(jobEntry));
    }

    //! Insert valid stat entries, one per job
    for (const auto& jobEntry : m_data->m_jobEntries)
    {
        AZStd::string statName = AZStd::string::format(
            "ProcessJob,%s,%s,%s,%s,%s",
            scanFolderEntry.m_scanFolder.c_str(),
            m_data->m_sourceName.c_str(),
            jobEntry.m_jobKey.c_str(),
            jobEntry.m_platform.c_str(),
            jobEntry.m_builderGuid.ToString<AZStd::string>().c_str());
        statEntry = { statName /* StatName */,
                      aznumeric_cast<AZ::s64>(jobEntry.m_fingerprint) /* StatValue */,
                      aznumeric_cast<AZ::s64>(jobEntry.m_jobRunKey) /* LastLogTime */ };
        ASSERT_TRUE(m_data->m_connection.ReplaceStat(statEntry));
    }

    //! Insert an invalid stat entry (7 tokens)
    statEntry = { "ProcessJob,apple,peach,banana,carrot,dog,{FDAF4363-C530-476C-B382-579A43B3E2FC}", 123, 456 };
    ASSERT_TRUE(m_data->m_connection.ReplaceStat(statEntry));
}
