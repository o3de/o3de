/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <native/resourcecompiler/rccontroller.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <native/unittests/RCcontrollerUnitTests.h>
#include <native/unittests/UnitTestUtils.h>

#include <QCoreApplication>

#if defined(AZ_PLATFORM_LINUX)
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <tests/UnitTestUtilities.h>
#include <tests/AssetProcessorTest.h>

using namespace AssetProcessor;
using namespace AzFramework::AssetSystem;

namespace
{
    constexpr NetworkRequestID RequestID(1, 1234);
    constexpr int MaxProcessingWaitTimeMs = 60 * 1000; // Wait up to 1 minute.  Give a generous amount of time to allow for slow CPUs
    const ScanFolderInfo TestScanFolderInfo("c:/samplepath", "sampledisplayname", "samplekey", false, false);
    const AZ::Uuid BuilderUuid = AZ::Uuid::CreateRandom();
}

class MockRCJob
    : public RCJob
{
public:
    MockRCJob(QObject* parent = nullptr)
        :RCJob(parent)
    {
    }

    void DoWork(AssetBuilderSDK::ProcessJobResponse& /*result*/, BuilderParams& builderParams, AssetUtilities::QuitListener& /*listener*/) override
    {
        m_DoWorkCalled = true;
        m_capturedParams = builderParams;
    }
public:
    bool m_DoWorkCalled = false;
    BuilderParams m_capturedParams;
};

void RCcontrollerUnitTests::FinishJob(AssetProcessor::RCJob* rcJob)
{
    m_rcController->FinishJob(rcJob);
}

void RCcontrollerUnitTests::PrepareRCJobListModelTest()
{
    // Create 6 jobs
    using namespace AssetProcessor;
    m_rcJobListModel = m_rcController->GetQueueModel();
    m_rcQueueSortModel = &m_rcController->m_RCQueueSortModel;

    AssetProcessor::JobDetails jobDetails;
    jobDetails.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference("c:/somerandomfolder/someFile0.txt");
    jobDetails.m_jobEntry.m_platformInfo = { "pc", { "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";

    RCJob* job0 = new RCJob(m_rcJobListModel);
    job0->Init(jobDetails);
    m_rcJobListModel->addNewJob(job0);

    RCJob* job1 = new RCJob(m_rcJobListModel);
    jobDetails.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference("c:/somerandomfolder/someFile1.txt");
    jobDetails.m_jobEntry.m_platformInfo = { "pc", { "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job1->Init(jobDetails);
    m_rcJobListModel->addNewJob(job1);

    RCJob* job2 = new RCJob(m_rcJobListModel);
    jobDetails.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference("c:/somerandomfolder/someFile2.txt");
    jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job2->Init(jobDetails);
    m_rcJobListModel->addNewJob(job2);

    RCJob* job3 = new RCJob(m_rcJobListModel);
    jobDetails.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference("c:/somerandomfolder/someFile3.txt");
    jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job3->Init(jobDetails);
    m_rcJobListModel->addNewJob(job3);

    RCJob* job4 = new RCJob(m_rcJobListModel);
    jobDetails.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference("c:/somerandomfolder/someFile4.txt");
    jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job4->Init(jobDetails);
    m_rcJobListModel->addNewJob(job4);

    RCJob* job5 = new RCJob(m_rcJobListModel);
    jobDetails.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference("c:/somerandomfolder/someFile5.txt");
    jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job5->Init(jobDetails);
    m_rcJobListModel->addNewJob(job5);

    // Complete 1 job
    RCJob* rcJob = job0;
    m_rcJobListModel->markAsProcessing(rcJob);
    rcJob->SetState(RCJob::completed);
    m_rcJobListModel->markAsCompleted(rcJob);

    // Put 1 job in processing state
    rcJob = job1;
    m_rcJobListModel->markAsProcessing(rcJob);

    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

void RCcontrollerUnitTests::PrepareCompileGroupTests(bool& gotCreated, bool& gotCompleted, AssetProcessor::NetworkRequestID& gotGroupID, AzFramework::AssetSystem::AssetStatus& gotStatus)
{
    QStringList tempJobNames;

    // Note that while this is an OS-SPECIFIC path, this test does not actually invoke the file system
    // or file operators, so is purely doing in-memory testing.  So the path does not actually matter and the
    // test should function on other operating systems too.

    // test - exact match
    tempJobNames << "c:/somerandomfolder/dev/blah/test.dds";
    tempJobNames << "c:/somerandomfolder/dev/blah/test.cre"; // must not match

    // test - NO MATCH
    tempJobNames << "c:/somerandomfolder/dev/wap/wap.wap";

    // test - Multiple match, ignoring extensions
    tempJobNames << "c:/somerandomfolder/dev/abc/123.456";
    tempJobNames << "c:/somerandomfolder/dev/abc/123.567";
    tempJobNames << "c:/somerandomfolder/dev/def/123.456"; // must not match
    tempJobNames << "c:/somerandomfolder/dev/def/123.567"; // must not match

    // test - Multiple match, wide search, ignoring extensions and postfixes like underscore
    tempJobNames << "c:/somerandomfolder/dev/aaa/bbb/123.456";
    tempJobNames << "c:/somerandomfolder/dev/aaa/bbb/123.567";
    tempJobNames << "c:/somerandomfolder/dev/aaa/bbb/123.890";
    tempJobNames << "c:/somerandomfolder/dev/aaa/ccc/123.567"; // must not match!
    tempJobNames << "c:/somerandomfolder/dev/aaa/ccc/456.567"; // must not match

    // test - compile group fails the moment any file in it fails
    tempJobNames << "c:/somerandomfolder/mmmnnnoo/123.456";
    tempJobNames << "c:/somerandomfolder/mmmnnnoo/123.567";

    // test - compile group by uuid is always an exact match.  Its the same code path
    // as the others in terms of success/failure.
    tempJobNames << "c:/somerandomfolder/pqr/123.456";

    // test - compile group for an exact ID succeeds when that exact ID is called.
    for (QString name : tempJobNames)
    {
        AZ::Uuid uuidOfSource = AZ::Uuid::CreateName(name.toUtf8().constData());
        RCJob* job = new RCJob(m_rcJobListModel);
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference("c:/somerandomfolder/dev", name);
        jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
        jobDetails.m_jobEntry.m_jobKey = "Compile Stuff";
        jobDetails.m_jobEntry.m_sourceFileUUID = uuidOfSource;
        job->Init(jobDetails);
        m_rcJobListModel->addNewJob(job);
        m_createdJobs.push_back(job);
    }

    // double them up for "android" to make sure that platform is respected
    for (QString name : tempJobNames)
    {
        AZ::Uuid uuidOfSource = AZ::Uuid::CreateName(name.toUtf8().constData());
        RCJob* job0 = new RCJob(m_rcJobListModel);
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference("c:/somerandomfolder/dev", name);
        jobDetails.m_jobEntry.m_platformInfo = { "android" ,{ "mobile", "renderer" } };
        jobDetails.m_jobEntry.m_jobKey = "Compile Other Stuff";
        jobDetails.m_jobEntry.m_sourceFileUUID = uuidOfSource;
        job0->Init(jobDetails);
        m_rcJobListModel->addNewJob(job0);
    }

    ConnectCompileGroupSignalsAndSlots(gotCreated, gotCompleted, gotGroupID, gotStatus);
}

void RCcontrollerUnitTests::Reset()
{
    m_rcController->m_RCJobListModel.m_jobs.clear();
    m_rcController->m_RCJobListModel.m_jobsInFlight.clear();
    m_rcController->m_RCJobListModel.m_jobsInQueueLookup.clear();

    m_rcController->m_pendingCriticalJobsPerPlatform.clear();
    m_rcController->m_jobsCountPerPlatform.clear();

    // Doing this to refresh the SortModel
    m_rcController->m_RCQueueSortModel.AttachToModel(nullptr);
    m_rcController->m_RCQueueSortModel.AttachToModel(&m_rcController->m_RCJobListModel);
    m_rcController->m_RCQueueSortModel.m_currentJobRunKeyToJobEntries.clear();
    m_rcController->m_RCQueueSortModel.m_currentlyConnectedPlatforms.clear();
}

void RCcontrollerUnitTests::ConnectCompileGroupSignalsAndSlots(bool& gotCreated, bool& gotCompleted, NetworkRequestID& gotGroupID, AssetStatus& gotStatus)
{
    QObject::connect(m_rcController.get(), &RCController::CompileGroupCreated, this, [&](NetworkRequestID groupID, AssetStatus status)
        {
            gotCreated = true;
            gotGroupID = groupID;
            gotStatus = status;
        });

    QObject::connect(m_rcController.get(), &RCController::CompileGroupFinished, this, [&](NetworkRequestID groupID, AssetStatus status)
        {
            gotCompleted = true;
            gotGroupID = groupID;
            gotStatus = status;
        });
}

void RCcontrollerUnitTests::ConnectJobSignalsAndSlots(bool& allJobsCompleted, JobEntry& completedJob)
{
    QObject::connect(m_rcController.get(), &RCController::FileCompiled, this, [&](JobEntry entry, AssetBuilderSDK::ProcessJobResponse response)
        {
            completedJob = entry;
        });

    QObject::connect(m_rcController.get(), &RCController::FileCancelled, this, [&](JobEntry entry)
        {
            completedJob = entry;
        });

    QObject::connect(m_rcController.get(), &RCController::FileFailed, this, [&](JobEntry entry)
        {
            completedJob = entry;
        });
    QObject::connect(m_rcController.get(), &RCController::ActiveJobsCountChanged, this, [&](unsigned int /*count*/)
        {
            m_rcController->OnAddedToCatalog(completedJob);
            completedJob = {};
        });

    QObject::connect(m_rcController.get(), &RCController::BecameIdle, this, [&]()
        {
            allJobsCompleted = true;
        }
    );
}

void RCcontrollerUnitTests::SetUp()
{
    UnitTest::AssetProcessorUnitTestBase::SetUp();
    m_rcController = AZStd::make_unique<AssetProcessor::RCController>(1, 4);

    QDir assetRootPath(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());

    m_appManager->m_platformConfig->AddScanFolder(TestScanFolderInfo);
    m_appManager->m_platformConfig->AddScanFolder(
        AssetProcessor::ScanFolderInfo{ "c:/somerandomfolder", "scanfolder", "scanfolder", true, true, {}, 0, 1 });
    m_appManager->m_platformConfig->AddScanFolder(
        AssetProcessor::ScanFolderInfo{ "d:/test", "scanfolder2", "scanfolder2", true, true, {}, 0, 2 });
    m_appManager->m_platformConfig->AddScanFolder(
        AssetProcessor::ScanFolderInfo{ assetRootPath.absoluteFilePath("subfolder4"), "subfolder4", "subfolder4", false, true, {}, 0, 3 });

    using namespace AssetProcessor;
    m_rcJobListModel = m_rcController->GetQueueModel();
    m_rcQueueSortModel = &m_rcController->m_RCQueueSortModel;
}

void RCcontrollerUnitTests::TearDown()
{
    m_rcJobListModel = nullptr;
    m_rcQueueSortModel = nullptr;

    m_rcController.reset();

    UnitTest::AssetProcessorUnitTestBase::TearDown();
}

TEST_F(RCcontrollerUnitTests, TestRCJobListModel_AddJobEntries_Succeeds)
{
    PrepareRCJobListModelTest();

    int returnedCount = m_rcJobListModel->rowCount(QModelIndex());
    int expectedCount = 5; // Finished jobs should be removed, so they shouldn't show up

    ASSERT_EQ(returnedCount, expectedCount) << AZStd::string::format("RCJobListModel has %d elements, which is invalid. Expected %d", returnedCount, expectedCount).c_str();

    QModelIndex rcJobIndex;
    QString rcJobCommand;
    QString rcJobState;

    for (int i = 0; i < expectedCount; i++)
    {
        rcJobIndex = m_rcJobListModel->index(i, 0, QModelIndex());

        ASSERT_TRUE(rcJobIndex.isValid()) << AZStd::string::format("ModelIndex for row %d is invalid.", i).c_str();

        ASSERT_LT(rcJobIndex.row(), expectedCount) << AZStd::string::format("ModelIndex for row %d is invalid (outside expected range).", i).c_str();

        rcJobCommand = m_rcJobListModel->data(rcJobIndex, RCJobListModel::displayNameRole).toString();
        rcJobState = m_rcJobListModel->data(rcJobIndex, RCJobListModel::stateRole).toString();
    }
}

TEST_F(RCcontrollerUnitTests, TestCompileGroup_RequestExactMatchCompileGroup_Succeeds)
{
    bool gotCreated = false;
    bool gotCompleted = false;
    NetworkRequestID gotGroupID;
    AssetStatus gotStatus = AssetStatus_Unknown;
    PrepareCompileGroupTests(gotCreated, gotCompleted, gotGroupID, gotStatus);

    // EXACT MATCH TEST (with prefixes and such)
    m_rcController->OnRequestCompileGroup(RequestID, "pc", "@products@/blah/test.dds", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // this should have matched exactly one item, and when we finish that item, it should terminate:
    EXPECT_TRUE(gotCreated);
    EXPECT_FALSE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Queued);

    gotCreated = false;
    gotCompleted = false;

    // FINISH that job, we expect the finished message:

    m_rcJobListModel->markAsProcessing(m_createdJobs[0]);
    m_createdJobs[0]->SetState(RCJob::completed);
    FinishJob(m_createdJobs[0]);
    m_rcController->OnJobComplete(m_createdJobs[0]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_FALSE(gotCreated);
    EXPECT_TRUE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Compiled);
}

TEST_F(RCcontrollerUnitTests, TestCompileGroup_RequestNoMatchCompileGroup_Succeeds)
{
    bool gotCreated = false;
    bool gotCompleted = false;
    NetworkRequestID gotGroupID;
    AssetStatus gotStatus = AssetStatus_Unknown;
    PrepareCompileGroupTests(gotCreated, gotCompleted, gotGroupID, gotStatus);

    // give it a name that for sure does not match:
    m_rcController->OnRequestCompileGroup(RequestID, "pc", "bibbidybobbidy.boo", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCreated);
    EXPECT_FALSE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Unknown);
}

TEST_F(RCcontrollerUnitTests, TestCompileGroup_RequestCompileGroupWithInvalidPlatform_Succeeds)
{
    bool gotCreated = false;
    bool gotCompleted = false;
    NetworkRequestID gotGroupID;
    AssetStatus gotStatus = AssetStatus_Unknown;
    PrepareCompileGroupTests(gotCreated, gotCompleted, gotGroupID, gotStatus);

    // give it a name that for sure does not match due to platform.
    m_rcController->OnRequestCompileGroup(RequestID, "aaaaaa", "blah/test.cre", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCreated);
    EXPECT_FALSE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Unknown);
}

TEST_F(RCcontrollerUnitTests, TestCompileGroup_FinishEachAssetsInGroup_Succeeds)
{
    // in this test, we create a group with two assets in it
    // so that when the one finishes, it shouldn't complete the group, until the other also finishes
    // because compile groups are only finished when all assets in them are complete (or any have failed)
    bool gotCreated = false;
    bool gotCompleted = false;
    NetworkRequestID gotGroupID;
    AssetStatus gotStatus = AssetStatus_Unknown;
    PrepareCompileGroupTests(gotCreated, gotCompleted, gotGroupID, gotStatus);

    m_rcController->OnRequestCompileGroup(RequestID, "pc", "abc/123.nnn", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCreated);
    EXPECT_FALSE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Queued);

    // complete one of them.  It should still be a busy group
    gotCreated = false;
    gotCompleted = false;
    m_rcJobListModel->markAsProcessing(m_createdJobs[3]);
    m_createdJobs[3]->SetState(RCJob::completed);
    FinishJob(m_createdJobs[3]);
    m_rcController->OnJobComplete(m_createdJobs[3]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // despite us finishing the one job, its still an open compile group with remaining work.
    EXPECT_FALSE(gotCreated);
    EXPECT_FALSE(gotCompleted);

    // finish the other
    gotCreated = false;
    gotCompleted = false;
    m_rcJobListModel->markAsProcessing(m_createdJobs[4]);
    m_createdJobs[4]->SetState(RCJob::completed);
    FinishJob(m_createdJobs[4]);
    m_rcController->OnJobComplete(m_createdJobs[4]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCompleted);
    EXPECT_FALSE(gotCreated);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Compiled);
}

TEST_F(RCcontrollerUnitTests, TestCompileGroup_RequestWideSearchCompileGroup_Succeeds)
{
    // Multiple match, wide search, ignoring extensions and postfixes like underscore.
    bool gotCreated = false;
    bool gotCompleted = false;
    NetworkRequestID gotGroupID;
    AssetStatus gotStatus = AssetStatus_Unknown;
    PrepareCompileGroupTests(gotCreated, gotCompleted, gotGroupID, gotStatus);

    m_rcController->OnRequestCompileGroup(RequestID, "pc", "aaa/bbb/123_45.abc", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCreated);
    EXPECT_FALSE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Queued);

    // complete two of them.  It should still be a busy group!
    gotCreated = false;
    gotCompleted = false;

    m_rcJobListModel->markAsProcessing(m_createdJobs[7]);
    m_createdJobs[7]->SetState(RCJob::completed);
    FinishJob(m_createdJobs[7]);
    m_rcController->OnJobComplete(m_createdJobs[7]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    m_rcJobListModel->markAsProcessing(m_createdJobs[8]);
    m_createdJobs[8]->SetState(RCJob::completed);
    FinishJob(m_createdJobs[8]);
    m_rcController->OnJobComplete(m_createdJobs[8]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_FALSE(gotCreated);
    EXPECT_FALSE(gotCompleted);

    // finish the final one
    m_rcJobListModel->markAsProcessing(m_createdJobs[9]);
    m_createdJobs[9]->SetState(RCJob::completed);
    FinishJob(m_createdJobs[9]);
    m_rcController->OnJobComplete(m_createdJobs[9]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCompleted);
    EXPECT_FALSE(gotCreated);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Compiled);
}

TEST_F(RCcontrollerUnitTests, TestCompileGroup_GroupMemberFails_GroupFails)
{
    // Ensure that a group fails when any member of it fails.
    bool gotCreated = false;
    bool gotCompleted = false;
    NetworkRequestID gotGroupID;
    AssetStatus gotStatus = AssetStatus_Unknown;
    PrepareCompileGroupTests(gotCreated, gotCompleted, gotGroupID, gotStatus);

    m_rcController->OnRequestCompileGroup(RequestID, "pc", "mmmnnnoo/123.ZZZ", AZ::Data::AssetId()); // should match exactly 2 elements
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCreated);
    EXPECT_FALSE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Queued);

    gotCreated = false;
    gotCompleted = false;

    m_rcJobListModel->markAsProcessing(m_createdJobs[12]);
    m_createdJobs[12]->SetState(RCJob::failed);
    FinishJob(m_createdJobs[12]);
    m_rcController->OnJobComplete(m_createdJobs[12]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Failed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // this should have failed it immediately.

    EXPECT_TRUE(gotCompleted);
    EXPECT_FALSE(gotCreated);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Failed);
}

TEST_F(RCcontrollerUnitTests, TestCompileGroup_RequestCompileGroupWithUuid_Succeeds)
{
    // compile group but with UUID instead of file name.
    bool gotCreated = false;
    bool gotCompleted = false;
    NetworkRequestID gotGroupID;
    AssetStatus gotStatus = AssetStatus_Unknown;
    PrepareCompileGroupTests(gotCreated, gotCompleted, gotGroupID, gotStatus);

    AZ::Data::AssetId sourceDataID(m_createdJobs[14]->GetJobEntry().m_sourceFileUUID);
    m_rcController->OnRequestCompileGroup(RequestID, "pc", "", sourceDataID); // should match exactly 1 element.
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCreated);
    EXPECT_FALSE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Queued);

    gotCreated = false;
    gotCompleted = false;

    m_rcJobListModel->markAsProcessing(m_createdJobs[14]);
    m_createdJobs[14]->SetState(RCJob::completed);
    FinishJob(m_createdJobs[14]);
    m_rcController->OnJobComplete(m_createdJobs[14]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotCompleted);
    EXPECT_EQ(gotGroupID, RequestID);
    EXPECT_EQ(gotStatus, AssetStatus_Compiled);
}

TEST_F(RCcontrollerUnitTests, TestRCController_FeedDuplicateJobs_NotAccept)
{
    bool gotJobsInQueueCall = false;
    QString platformInQueueCount;
    int jobsInQueueCount = 0;
    QObject::connect(m_rcController.get(), &RCController::JobsInQueuePerPlatform, this, [&gotJobsInQueueCall, &platformInQueueCount, &jobsInQueueCount](QString platformName, int newCount)
        {
            gotJobsInQueueCall = true;
            platformInQueueCount = platformName;
            jobsInQueueCount = newCount;
        });

    AZ::Uuid sourceId = AZ::Uuid("{2206A6E0-FDBC-45DE-B6FE-C2FC63020BD5}");
    JobDetails details;
    details.m_jobEntry = JobEntry(AssetProcessor::SourceAssetReference("d:/test", "test1.txt"), AZ::Uuid("{7954065D-CFD1-4666-9E4C-3F36F417C7AC}"), { "pc" , {"desktop", "renderer"} }, "Test Job", 1234, 1, sourceId);
    gotJobsInQueueCall = false;
    int priorJobs = jobsInQueueCount;
    m_rcController->JobSubmitted(details);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotJobsInQueueCall);
    EXPECT_EQ(jobsInQueueCount, priorJobs + 1);
    priorJobs = jobsInQueueCount;
    gotJobsInQueueCall = false;

    // submit same job, different run key
    details.m_jobEntry = JobEntry(AssetProcessor::SourceAssetReference("d:/test", "test1.txt"), AZ::Uuid("{7954065D-CFD1-4666-9E4C-3F36F417C7AC}"), { "pc" ,{ "desktop", "renderer" } }, "Test Job", 1234, 2, sourceId);
    m_rcController->JobSubmitted(details);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    EXPECT_FALSE(gotJobsInQueueCall);

    // submit same job but different platform:
    details.m_jobEntry = JobEntry(AssetProcessor::SourceAssetReference("d:/test", "test1.txt"), AZ::Uuid("{7954065D-CFD1-4666-9E4C-3F36F417C7AC}"), { "android" ,{ "mobile", "renderer" } }, "Test Job", 1234, 3, sourceId);
    m_rcController->JobSubmitted(details);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    EXPECT_TRUE(gotJobsInQueueCall);
    EXPECT_EQ(jobsInQueueCount, priorJobs);
}

TEST_F(RCcontrollerUnitTests, TestRCController_StartRCJobWithCriticalLocking_BlocksOnceLockReleased)
{
    QDir assetRootPath(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    // test task generation while a file is in still in use
    QString fileInUsePath = AssetUtilities::NormalizeFilePath(assetRootPath.absoluteFilePath("subfolder4/needsLock.tiff"));

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileInUsePath, "xxx"));

    QFile lockFileTest(fileInUsePath);
#if defined(AZ_PLATFORM_WINDOWS)
    // on windows, its enough to just open the file:
    lockFileTest.open(QFile::ReadOnly);
#elif defined(AZ_PLATFORM_LINUX)
    int handleOfLock = open(fileInUsePath.toUtf8().constData(), O_RDONLY | O_EXCL | O_NONBLOCK);
    EXPECT_NE(handleOfLock, -1);
#else
    int handleOfLock = open(fileInUsePath.toUtf8().constData(), O_RDONLY | O_EXLOCK | O_NONBLOCK);
    EXPECT_NE(handleOfLock, -1);
#endif

    AZ::Uuid uuidOfSource = AZ::Uuid("{D013122E-CF2C-4534-A87D-F82570FBC2CD}");
    MockRCJob rcJob;
    AssetProcessor::JobDetails jobDetailsToInitWith;
    jobDetailsToInitWith.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(fileInUsePath);
    jobDetailsToInitWith.m_jobEntry.m_platformInfo = { "pc", { "tools", "editor"} };
    jobDetailsToInitWith.m_jobEntry.m_jobKey = "Text files";
    jobDetailsToInitWith.m_jobEntry.m_sourceFileUUID = uuidOfSource;
    jobDetailsToInitWith.m_scanFolder = &TestScanFolderInfo;
    rcJob.Init(jobDetailsToInitWith);

    bool beginWork = false;
    QObject::connect(&rcJob, &RCJob::BeginWork, this, [&beginWork]()
        {
            beginWork = true;
        }
        );
    bool jobFinished = false;
    QObject::connect(&rcJob, &RCJob::JobFinished, this, [&jobFinished](AssetBuilderSDK::ProcessJobResponse /*result*/)
        {
            jobFinished = true;
        }
        );
    rcJob.SetCheckExclusiveLock(true);
    rcJob.Start();

#if defined(AZ_PLATFORM_WINDOWS)
    // on windows, opening a file for reading locks it
    // but on other platforms, this is not the case.
    // we only expect work to begin when we can gain an exclusive lock on this file.

    // Use a short wait time here because the test will have to wait this entire time to detect the failure
    static constexpr int WaitTimeMs = 500;
    EXPECT_FALSE(UnitTestUtils::BlockUntil(beginWork, WaitTimeMs));

    // Once we release the file, it should process normally
    lockFileTest.close();
#else
    close(handleOfLock);
#endif

    //Once we release the lock we should see jobStarted and jobFinished
    EXPECT_TRUE(UnitTestUtils::BlockUntil(jobFinished, MaxProcessingWaitTimeMs));
    EXPECT_TRUE(beginWork);
    EXPECT_TRUE(rcJob.m_DoWorkCalled);

    // make sure the source UUID made its way all the way from create jobs to process jobs.
    EXPECT_EQ(rcJob.m_capturedParams.m_processJobRequest.m_sourceFileUUID, uuidOfSource);
}

TEST_F(RCcontrollerUnitTests, TestRCController_FeedJobsWithDependencies_DispatchJobsInOrder)
{
    QDir assetRootPath(m_assetDatabaseRequestsHandler->GetAssetRootDir().c_str());
    QString fileA = AssetUtilities::NormalizeFilePath(assetRootPath.absoluteFilePath("FileA.txt"));
    QString fileB = AssetUtilities::NormalizeFilePath(assetRootPath.absoluteFilePath("FileB.txt"));
    QString fileC = AssetUtilities::NormalizeFilePath(assetRootPath.absoluteFilePath("FileC.txt"));
    QString fileD = AssetUtilities::NormalizeFilePath(assetRootPath.absoluteFilePath("FileD.txt"));

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileA, "xxx"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileB, "xxx"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileC, "xxx"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileD, "xxx"));

    Reset();
    m_assetBuilderDesc.m_name = "Job Dependency UnitTest";
    m_assetBuilderDesc.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
    m_assetBuilderDesc.m_busId = BuilderUuid;
    m_assetBuilderDesc.m_processJobFunction = []
    ([[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    };

    m_rcController->SetDispatchPaused(true);

    // Job B has an order job dependency on Job A

    // Setting up JobA
    MockRCJob* jobA = new MockRCJob(m_rcJobListModel);
    JobDetails jobdetailsA;
    jobdetailsA.m_scanFolder = &TestScanFolderInfo;
    jobdetailsA.m_assetBuilderDesc = m_assetBuilderDesc;
    jobdetailsA.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(TestScanFolderInfo.ScanPath(), "fileA.txt");
    jobdetailsA.m_jobEntry.m_platformInfo = { "pc" ,{ "desktop", "renderer" } };
    jobdetailsA.m_jobEntry.m_jobKey = "TestJobA";
    jobdetailsA.m_jobEntry.m_builderGuid = BuilderUuid;

    jobA->Init(jobdetailsA);
    m_rcQueueSortModel->AddJobIdEntry(jobA);
    m_rcJobListModel->addNewJob(jobA);

    bool beginWorkA = false;
    QObject::connect(jobA, &RCJob::BeginWork, this, [&beginWorkA]()
    {
        beginWorkA = true;
    }
    );

    bool jobFinishedA = false;
    QObject::connect(jobA, &RCJob::JobFinished, this, [&jobFinishedA](AssetBuilderSDK::ProcessJobResponse /*result*/)
    {
        jobFinishedA = true;
    }
    );

    // Setting up JobB
    JobDetails jobdetailsB;
    jobdetailsB.m_scanFolder = &TestScanFolderInfo;
    jobdetailsA.m_assetBuilderDesc = m_assetBuilderDesc;
    jobdetailsB.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(TestScanFolderInfo.ScanPath(), "fileB.txt");
    jobdetailsB.m_jobEntry.m_platformInfo = { "pc" ,{ "desktop", "renderer" } };
    jobdetailsB.m_jobEntry.m_jobKey = "TestJobB";
    jobdetailsB.m_jobEntry.m_builderGuid = BuilderUuid;

    jobdetailsB.m_critical = true; //make jobB critical so that it will be analyzed first even though we want JobA to run first

    AssetBuilderSDK::SourceFileDependency sourceFileADependency;
    sourceFileADependency.m_sourceFileDependencyPath = (AZ::IO::Path(TestScanFolderInfo.ScanPath().toUtf8().constData()) / "fileA.txt").Native();

    // Make job B has an order job dependency on Job A
    AssetBuilderSDK::JobDependency jobDependencyA("TestJobA", "pc", AssetBuilderSDK::JobDependencyType::Order, sourceFileADependency);
    jobdetailsB.m_jobDependencyList.push_back({ jobDependencyA });

    //Setting JobB
    MockRCJob* jobB = new MockRCJob(m_rcJobListModel);
    jobB->Init(jobdetailsB);
    m_rcQueueSortModel->AddJobIdEntry(jobB);
    m_rcJobListModel->addNewJob(jobB);

    bool beginWorkB = false;
    QMetaObject::Connection conn = QObject::connect(jobB, &RCJob::BeginWork, this, [&beginWorkB, &jobFinishedA]()
    {
        // JobA should finish first before JobB starts
        EXPECT_TRUE(jobFinishedA);
        beginWorkB = true;
    }
    );

    bool jobFinishedB = false;
    QObject::connect(jobB, &RCJob::JobFinished, this, [&jobFinishedB](AssetBuilderSDK::ProcessJobResponse /*result*/)
    {
        jobFinishedB = true;
    }
    );

    JobEntry completedJob;
    bool allJobsCompleted = false;
    ConnectJobSignalsAndSlots(allJobsCompleted, completedJob);

    m_rcController->SetDispatchPaused(false);

    m_rcController->DispatchJobs();
    EXPECT_TRUE(UnitTestUtils::BlockUntil(allJobsCompleted, MaxProcessingWaitTimeMs));
    EXPECT_TRUE(jobFinishedB);
}

TEST_F(RCcontrollerUnitTests, TestRCController_FeedJobsWithCyclicDependencies_AllJobsFinish)
{
    // Now test the use case where we have a cyclic dependency,
    // although the order in which these job will start is not defined but we can ensure that
    // all the jobs finish and RCController goes Idle
    JobEntry completedJob;
    bool allJobsCompleted = false;
    ConnectJobSignalsAndSlots(allJobsCompleted, completedJob);

    m_rcController->SetDispatchPaused(true);

    //Setting up JobC
    JobDetails jobdetailsC;
    jobdetailsC.m_scanFolder = &TestScanFolderInfo;
    jobdetailsC.m_assetBuilderDesc = m_assetBuilderDesc;
    jobdetailsC.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(TestScanFolderInfo.ScanPath(), "fileC.txt");
    jobdetailsC.m_jobEntry.m_platformInfo = { "pc" ,{ "desktop", "renderer" } };
    jobdetailsC.m_jobEntry.m_jobKey = "TestJobC";
    jobdetailsC.m_jobEntry.m_builderGuid = BuilderUuid;

    AssetBuilderSDK::SourceFileDependency sourceFileCDependency;
    sourceFileCDependency.m_sourceFileDependencyPath =
        (AZ::IO::Path(TestScanFolderInfo.ScanPath().toUtf8().constData()) / "fileC.txt").Native();

    //Setting up Job D
    JobDetails jobdetailsD;
    jobdetailsD.m_scanFolder = &TestScanFolderInfo;
    jobdetailsD.m_assetBuilderDesc = m_assetBuilderDesc;
    jobdetailsD.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(TestScanFolderInfo.ScanPath(), "fileD.txt");
    jobdetailsD.m_jobEntry.m_platformInfo = { "pc" ,{ "desktop", "renderer" } };
    jobdetailsD.m_jobEntry.m_jobKey = "TestJobD";
    jobdetailsD.m_jobEntry.m_builderGuid = BuilderUuid;
    AssetBuilderSDK::SourceFileDependency sourceFileDDependency;
    sourceFileDDependency.m_sourceFileDependencyPath =
        (AZ::IO::Path(TestScanFolderInfo.ScanPath().toUtf8().constData()) / "fileD.txt").Native();

    //creating cyclic job order dependencies i.e  JobC and JobD have order job dependency on each other
    AssetBuilderSDK::JobDependency jobDependencyC("TestJobC", "pc", AssetBuilderSDK::JobDependencyType::Order, sourceFileCDependency);
    AssetBuilderSDK::JobDependency jobDependencyD("TestJobD", "pc", AssetBuilderSDK::JobDependencyType::Order, sourceFileDDependency);
    jobdetailsC.m_jobDependencyList.push_back({ jobDependencyD });
    jobdetailsD.m_jobDependencyList.push_back({ jobDependencyC });

    MockRCJob* jobD = new MockRCJob(m_rcJobListModel);
    MockRCJob* jobC = new MockRCJob(m_rcJobListModel);

    jobC->Init(jobdetailsC);
    m_rcQueueSortModel->AddJobIdEntry(jobC);
    m_rcJobListModel->addNewJob(jobC);

    jobD->Init(jobdetailsD);
    m_rcQueueSortModel->AddJobIdEntry(jobD);
    m_rcJobListModel->addNewJob(jobD);

    m_rcController->SetDispatchPaused(false);
    m_rcController->DispatchJobs();
    EXPECT_TRUE(UnitTestUtils::BlockUntil(allJobsCompleted, MaxProcessingWaitTimeMs));

    // Test case when source file is deleted before it started processing
    {
        int prevJobCount = m_rcJobListModel->itemCount();
        MockRCJob rcJobAddAndDelete;
        AssetProcessor::JobDetails jobDetailsToInitWithInsideScope;
        jobDetailsToInitWithInsideScope.m_jobEntry.m_sourceAssetReference =
            AssetProcessor::SourceAssetReference(TestScanFolderInfo.ScanPath(), "someFile0.txt");
        jobDetailsToInitWithInsideScope.m_jobEntry.m_platformInfo = { "pc",{ "tools", "editor" } };
        jobDetailsToInitWithInsideScope.m_jobEntry.m_jobKey = "Text files";
        jobDetailsToInitWithInsideScope.m_jobEntry.m_sourceFileUUID = AZ::Uuid("{D013122E-CF2C-4534-A87D-F82570FBC2CD}");
        rcJobAddAndDelete.Init(jobDetailsToInitWithInsideScope);

        m_rcJobListModel->addNewJob(&rcJobAddAndDelete);

        // verify that job was added
        EXPECT_EQ(m_rcJobListModel->itemCount(), prevJobCount + 1);
        m_rcController->RemoveJobsBySource(AssetProcessor::SourceAssetReference(TestScanFolderInfo.ScanPath(), "someFile0.txt"));
        // verify that job was removed
        EXPECT_EQ(m_rcJobListModel->itemCount(), prevJobCount);
    }
}
