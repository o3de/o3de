/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RCcontrollerUnitTests.h"
#include <AzTest/AzTest.h>
#include <QCoreApplication>

#if defined(AZ_PLATFORM_LINUX)
#include <sys/stat.h>
#include <fcntl.h>
#endif

using namespace AssetProcessor;
using namespace AzFramework::AssetSystem;

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

RCcontrollerUnitTests::RCcontrollerUnitTests()
    : m_rcController(1, 4)
{
}

void RCcontrollerUnitTests::Reset()
{
    m_rcController.m_RCJobListModel.m_jobs.clear();
    m_rcController.m_RCJobListModel.m_jobs.clear();
    m_rcController.m_RCJobListModel.m_jobsInFlight.clear();
    m_rcController.m_RCJobListModel.m_jobsInQueueLookup.clear();

    m_rcController.m_pendingCriticalJobsPerPlatform.clear();
    m_rcController.m_jobsCountPerPlatform.clear();

    // Doing this to refresh the SortModel
    m_rcController.m_RCQueueSortModel.AttachToModel(nullptr);
    m_rcController.m_RCQueueSortModel.AttachToModel(&m_rcController.m_RCJobListModel);
    m_rcController.m_RCQueueSortModel.m_currentJobRunKeyToJobEntries.clear();
    m_rcController.m_RCQueueSortModel.m_currentlyConnectedPlatforms.clear();
}

void RCcontrollerUnitTests::StartTest()
{
    PrepareRCController();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    RunRCControllerTests();
}

void RCcontrollerUnitTests::PrepareRCController()
{
    // Create 6 jobs
    using namespace AssetProcessor;
    RCJobListModel* rcJobListModel = m_rcController.GetQueueModel();

    AssetProcessor::JobDetails jobDetails;
    jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = "someFile0.txt";
    jobDetails.m_jobEntry.m_watchFolderPath = QCoreApplication::applicationDirPath();
    jobDetails.m_jobEntry.m_databaseSourceName = "someFile0.txt";
    jobDetails.m_jobEntry.m_platformInfo = { "pc", { "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";

    RCJob* job0 = new RCJob(rcJobListModel);
    job0->Init(jobDetails);
    rcJobListModel->addNewJob(job0);

    RCJob* job1 = new RCJob(rcJobListModel);
    jobDetails.m_jobEntry.m_databaseSourceName = jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = "someFile1.txt";
    jobDetails.m_jobEntry.m_platformInfo = { "pc", { "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job1->Init(jobDetails);
    rcJobListModel->addNewJob(job1);

    RCJob* job2 = new RCJob(rcJobListModel);
    jobDetails.m_jobEntry.m_databaseSourceName = jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = "someFile2.txt";
    jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job2->Init(jobDetails);
    rcJobListModel->addNewJob(job2);

    RCJob* job3 = new RCJob(rcJobListModel);
    jobDetails.m_jobEntry.m_databaseSourceName = jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = "someFile3.txt";
    jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job3->Init(jobDetails);
    rcJobListModel->addNewJob(job3);

    RCJob* job4 = new RCJob(rcJobListModel);
    jobDetails.m_jobEntry.m_databaseSourceName = jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = "someFile4.txt";
    jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job4->Init(jobDetails);
    rcJobListModel->addNewJob(job4);

    RCJob* job5 = new RCJob(rcJobListModel);
    jobDetails.m_jobEntry.m_databaseSourceName = jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = "someFile5.txt";
    jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
    jobDetails.m_jobEntry.m_jobKey = "Text files";
    job5->Init(jobDetails);
    rcJobListModel->addNewJob(job5);

    // Complete 1 job
    RCJob* rcJob = job0;
    rcJobListModel->markAsProcessing(rcJob);
    rcJob->SetState(RCJob::completed);
    rcJobListModel->markAsCompleted(rcJob);

    // Put 1 job in processing state
    rcJob = job1;
    rcJobListModel->markAsProcessing(rcJob);
}

void RCcontrollerUnitTests::RunRCControllerTests()
{
    int jobsInQueueCount = 0;
    QString platformInQueueCount;
    bool gotJobsInQueueCall = false;

    QObject::connect(&m_rcController, &RCController::JobsInQueuePerPlatform, this, [&](QString platformName, int newCount)
        {
            gotJobsInQueueCall = true;
            platformInQueueCount = platformName;
            jobsInQueueCount = newCount;
        });

    RCJobListModel* rcJobListModel = m_rcController.GetQueueModel();
    int returnedCount = rcJobListModel->rowCount(QModelIndex());
    int expectedCount = 5; // finished ones should be removed, so it shouldn't show up

    if (returnedCount != expectedCount)
    {
        Q_EMIT UnitTestFailed("RCJobListModel has " + QString(returnedCount) + " elements, which is invalid. Expected " + QString(expectedCount));
        return;
    }

    QModelIndex rcJobIndex;
    QString rcJobCommand;
    QString rcJobState;

    for (int i = 0; i < expectedCount; i++)
    {
        rcJobIndex = rcJobListModel->index(i, 0, QModelIndex());

        if (!rcJobIndex.isValid())
        {
            Q_EMIT UnitTestFailed("ModelIndex for row " + QString(i) + " is invalid.");
            return;
        }

        if (rcJobIndex.row() >= expectedCount)
        {
            Q_EMIT UnitTestFailed("ModelIndex for row " + QString(i) + " is invalid (outside expected range).");
            return;
        }

        rcJobCommand = rcJobListModel->data(rcJobIndex, RCJobListModel::displayNameRole).toString();
        rcJobState = rcJobListModel->data(rcJobIndex, RCJobListModel::stateRole).toString();
    }

    // ----------------- test the Compile Group functionality
    // add some jobs to play with:

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

    QList<RCJob*> createdJobs;



    for (QString name : tempJobNames)
    {
        AZ::Uuid uuidOfSource = AZ::Uuid::CreateName(name.toUtf8().constData());
        RCJob* job = new RCJob(rcJobListModel);
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_watchFolderPath = "c:/somerandomfolder/dev";
        jobDetails.m_jobEntry.m_databaseSourceName = jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = name;
        jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
        jobDetails.m_jobEntry.m_jobKey = "Compile Stuff";
        jobDetails.m_jobEntry.m_sourceFileUUID = uuidOfSource;
        job->Init(jobDetails);
        rcJobListModel->addNewJob(job);
        createdJobs.push_back(job);
    }

    // double them up for "android" to make sure that platform is respected
    for (QString name : tempJobNames)
    {
        AZ::Uuid uuidOfSource = AZ::Uuid::CreateName(name.toUtf8().constData());
        RCJob* job0 = new RCJob(rcJobListModel);
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_databaseSourceName = jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = name;
        jobDetails.m_jobEntry.m_platformInfo = { "android" ,{ "mobile", "renderer" } };
        jobDetails.m_jobEntry.m_jobKey = "Compile Other Stuff";
        jobDetails.m_jobEntry.m_sourceFileUUID = uuidOfSource;
        job0->Init(jobDetails);
        rcJobListModel->addNewJob(job0);
    }

    bool gotCreated = false;
    bool gotCompleted = false;
    NetworkRequestID gotGroupID;
    AssetStatus gotStatus = AssetStatus_Unknown;
    QObject::connect(&m_rcController, &RCController::CompileGroupCreated, this, [&](NetworkRequestID groupID, AssetStatus status)
        {
            gotCreated = true;
            gotGroupID = groupID;
            gotStatus = status;
        });

    QObject::connect(&m_rcController, &RCController::CompileGroupFinished, this, [&](NetworkRequestID groupID, AssetStatus status)
        {
            gotCompleted = true;
            gotGroupID = groupID;
            gotStatus = status;
        });


    // EXACT MATCH TEST (with prefixes and such)
    NetworkRequestID requestID(1, 1234);
    m_rcController.OnRequestCompileGroup(requestID, "pc", "@products@/blah/test.dds", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // this should have matched exactly one item, and when we finish that item, it should terminate:
    UNIT_TEST_EXPECT_TRUE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Queued);

    gotCreated = false;
    gotCompleted = false;

    // FINISH that job, we expect the finished message:

    rcJobListModel->markAsProcessing(createdJobs[0]);
    createdJobs[0]->SetState(RCJob::completed);
    m_rcController.FinishJob(createdJobs[0]);
    m_rcController.OnJobComplete(createdJobs[0]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_FALSE(gotCreated);
    UNIT_TEST_EXPECT_TRUE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Compiled);

    // ----------------------- NO MATCH TEST -----------------
    // give it a name that for sure does not match:
    gotCreated = false;
    gotCompleted = false;
    m_rcController.OnRequestCompileGroup(requestID, "pc", "bibbidybobbidy.boo", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Unknown);

    // ----------------------- NO MATCH TEST (invalid platform) -----------------
    // give it a name that for sure does not match due to platform
    gotCreated = false;
    gotCompleted = false;
    m_rcController.OnRequestCompileGroup(requestID, "aaaaaa", "blah/test.cre", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Unknown);

    // -------------------------------- test - Multiple match, ignoring extensions
    // Give it something that should match 3 and 4  but not 5 and not 6

    // in this test, we create a group with two assets in it
    // so that when the one finishes, it shouldn't complete the group, until the other also finishes
    // because compile groups are only finished when all assets in them are complete (or any have failed)
    gotCreated = false;
    gotCompleted = false;
    m_rcController.OnRequestCompileGroup(requestID, "pc", "abc/123.nnn", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Queued);

    // complete one of them.  It should still be a busy group
    gotCreated = false;
    gotCompleted = false;
    rcJobListModel->markAsProcessing(createdJobs[3]);
    createdJobs[3]->SetState(RCJob::completed);
    m_rcController.FinishJob(createdJobs[3]);
    m_rcController.OnJobComplete(createdJobs[3]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // despite us finishing the one job, its still an open compile group with remaining work.
    UNIT_TEST_EXPECT_FALSE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);

    // finish the other
    gotCreated = false;
    gotCompleted = false;
    rcJobListModel->markAsProcessing(createdJobs[4]);
    createdJobs[4]->SetState(RCJob::completed);
    m_rcController.FinishJob(createdJobs[4]);
    m_rcController.OnJobComplete(createdJobs[4]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCompleted);
    UNIT_TEST_EXPECT_FALSE(gotCreated);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Compiled);

    // test - Multiple match, wide search, ignoring extensions and postfixes like underscore:
    // we expect 7, 8, and 9 to match

    gotCreated = false;
    gotCompleted = false;
    m_rcController.OnRequestCompileGroup(requestID, "pc", "aaa/bbb/123_45.abc", AZ::Data::AssetId());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Queued);

    // complete two of them.  It should still be a busy group!
    gotCreated = false;
    gotCompleted = false;

    rcJobListModel->markAsProcessing(createdJobs[7]);
    createdJobs[7]->SetState(RCJob::completed);
    m_rcController.FinishJob(createdJobs[7]);
    m_rcController.OnJobComplete(createdJobs[7]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    rcJobListModel->markAsProcessing(createdJobs[8]);
    createdJobs[8]->SetState(RCJob::completed);
    m_rcController.FinishJob(createdJobs[8]);
    m_rcController.OnJobComplete(createdJobs[8]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_FALSE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);

    // finish the final one
    rcJobListModel->markAsProcessing(createdJobs[9]);
    createdJobs[9]->SetState(RCJob::completed);
    m_rcController.FinishJob(createdJobs[9]);
    m_rcController.OnJobComplete(createdJobs[9]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCompleted);
    UNIT_TEST_EXPECT_FALSE(gotCreated);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Compiled);

    // ---------------- TEST :  Ensure that a group fails when any member of it fails.
    gotCreated = false;
    gotCompleted = false;
    m_rcController.OnRequestCompileGroup(requestID, "pc", "mmmnnnoo/123.ZZZ", AZ::Data::AssetId()); // should match exactly 2 elements
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Queued);

    gotCreated = false;
    gotCompleted = false;

    rcJobListModel->markAsProcessing(createdJobs[12]);
    createdJobs[12]->SetState(RCJob::failed);
    m_rcController.FinishJob(createdJobs[12]);
    m_rcController.OnJobComplete(createdJobs[12]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Failed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // this should have failed it immediately.

    UNIT_TEST_EXPECT_TRUE(gotCompleted);
    UNIT_TEST_EXPECT_FALSE(gotCreated);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Failed);

    /// --- TEST ------------- compile group but with UUID instead of file name.
    gotCreated = false;
    gotCompleted = false;
    AZ::Data::AssetId sourceDataID(createdJobs[14]->GetJobEntry().m_sourceFileUUID);
    m_rcController.OnRequestCompileGroup(requestID, "pc", "", sourceDataID); // should match exactly 1 element.
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCreated);
    UNIT_TEST_EXPECT_FALSE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Queued);

    gotCreated = false;
    gotCompleted = false;

    rcJobListModel->markAsProcessing(createdJobs[14]);
    createdJobs[14]->SetState(RCJob::completed);
    m_rcController.FinishJob(createdJobs[14]);
    m_rcController.OnJobComplete(createdJobs[14]->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotCompleted);
    UNIT_TEST_EXPECT_TRUE(gotGroupID == requestID);
    UNIT_TEST_EXPECT_TRUE(gotStatus == AssetStatus_Compiled);

    /// --- TEST ------------- RC controller should not accept duplicate jobs.

    AZ::Uuid sourceId = AZ::Uuid("{2206A6E0-FDBC-45DE-B6FE-C2FC63020BD5}");
    JobDetails details;
    details.m_jobEntry = JobEntry("d:/test", "test1.txt", "test1.txt", AZ::Uuid("{7954065D-CFD1-4666-9E4C-3F36F417C7AC}"), { "pc" , {"desktop", "renderer"} }, "Test Job", 1234, 1, sourceId);
    gotJobsInQueueCall = false;
    int priorJobs = jobsInQueueCount;
    m_rcController.JobSubmitted(details);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotJobsInQueueCall);
    UNIT_TEST_EXPECT_TRUE(jobsInQueueCount == priorJobs + 1);
    priorJobs = jobsInQueueCount;
    gotJobsInQueueCall = false;

    // submit same job, different run key
    details.m_jobEntry = JobEntry("d:/test", "/test1.txt", "test1.txt", AZ::Uuid("{7954065D-CFD1-4666-9E4C-3F36F417C7AC}"), { "pc" ,{ "desktop", "renderer" } }, "Test Job", 1234, 2, sourceId);
    m_rcController.JobSubmitted(details);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    UNIT_TEST_EXPECT_FALSE(gotJobsInQueueCall);

    // submit same job but different platform:
    details.m_jobEntry = JobEntry("d:/test", "test1.txt", "test1.txt", AZ::Uuid("{7954065D-CFD1-4666-9E4C-3F36F417C7AC}"), { "android" ,{ "mobile", "renderer" } }, "Test Job", 1234, 3, sourceId);
    m_rcController.JobSubmitted(details);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    UNIT_TEST_EXPECT_TRUE(gotJobsInQueueCall);
    UNIT_TEST_EXPECT_TRUE(jobsInQueueCount == priorJobs);


    ////--------------- RCJob Test with critical locking TRUE
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    // test task generation while a file is in still in use
    QString fileInUsePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder4/needsLock.tiff"));

    UNIT_TEST_EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileInUsePath, "xxx"));

    QFile lockFileTest(fileInUsePath);
#if defined(AZ_PLATFORM_WINDOWS)
    // on windows, its enough to just open the file:
    lockFileTest.open(QFile::ReadOnly);
#elif defined(AZ_PLATFORM_LINUX)
    int handleOfLock = open(fileInUsePath.toUtf8().constData(), O_RDONLY | O_EXCL | O_NONBLOCK);
    UNIT_TEST_EXPECT_TRUE(handleOfLock != -1);
#else
    int handleOfLock = open(fileInUsePath.toUtf8().constData(), O_RDONLY | O_EXLOCK | O_NONBLOCK);
    UNIT_TEST_EXPECT_TRUE(handleOfLock != -1);
#endif

    AZ::Uuid uuidOfSource = AZ::Uuid("{D013122E-CF2C-4534-A87D-F82570FBC2CD}");
    MockRCJob rcJob;
    ScanFolderInfo scanFolderInfo("samplepath", "sampledisplayname", "samplekey", false, false);
    AssetProcessor::JobDetails jobDetailsToInitWith;
    jobDetailsToInitWith.m_jobEntry.m_watchFolderPath = tempPath.absoluteFilePath("subfolder4");
    jobDetailsToInitWith.m_jobEntry.m_databaseSourceName = jobDetailsToInitWith.m_jobEntry.m_pathRelativeToWatchFolder = "needsLock.tiff";
    jobDetailsToInitWith.m_jobEntry.m_platformInfo = { "pc", { "tools", "editor"} };
    jobDetailsToInitWith.m_jobEntry.m_jobKey = "Text files";
    jobDetailsToInitWith.m_jobEntry.m_sourceFileUUID = uuidOfSource;
    jobDetailsToInitWith.m_scanFolder = &scanFolderInfo;
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
    UNIT_TEST_EXPECT_FALSE(UnitTestUtils::BlockUntil(beginWork, 5000));

    // Once we release the file, it should process normally
    lockFileTest.close();
#else
    close(handleOfLock);
#endif

    //Once we release the lock we should see jobStarted and jobFinished
    UNIT_TEST_EXPECT_TRUE(UnitTestUtils::BlockUntil(jobFinished, 5000));
    UNIT_TEST_EXPECT_TRUE(beginWork);
    UNIT_TEST_EXPECT_TRUE(rcJob.m_DoWorkCalled);

    // make sure the source UUID made its way all the way from create jobs to process jobs.
    UNIT_TEST_EXPECT_TRUE(rcJob.m_capturedParams.m_processJobRequest.m_sourceFileUUID == uuidOfSource);
    ////-----------------------UNIT TEST Order Job Dependency

    QString fileA = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("FileA.txt"));
    QString fileB = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("FileB.txt"));
    QString fileC = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("FileC.txt"));
    QString fileD = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("FileD.txt"));

    UNIT_TEST_EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileA, "xxx"));
    UNIT_TEST_EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileB, "xxx"));
    UNIT_TEST_EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileC, "xxx"));
    UNIT_TEST_EXPECT_TRUE(UnitTestUtils::CreateDummyFile(fileD, "xxx"));

    Reset();
    AZ::Uuid builderUuid = AZ::Uuid::CreateRandom();
    m_assetBuilderDesc.m_name = "Job Dependency UnitTest";
    m_assetBuilderDesc.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.txt", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
    m_assetBuilderDesc.m_busId = builderUuid;
    m_assetBuilderDesc.m_processJobFunction = []
    ([[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    };

    m_rcController.SetDispatchPaused(true);

    // Job B has an order job dependency on Job A

    // Setting up JobA
    MockRCJob* jobA = new MockRCJob(&m_rcController.m_RCJobListModel);
    JobDetails jobdetailsA;
    jobdetailsA.m_scanFolder = &scanFolderInfo;
    jobdetailsA.m_assetBuilderDesc = m_assetBuilderDesc;
    jobdetailsA.m_jobEntry.m_databaseSourceName = jobdetailsA.m_jobEntry.m_pathRelativeToWatchFolder = "fileA.txt";
    jobdetailsA.m_jobEntry.m_watchFolderPath = scanFolderInfo.ScanPath();
    jobdetailsA.m_jobEntry.m_platformInfo = { "pc" ,{ "desktop", "renderer" } };
    jobdetailsA.m_jobEntry.m_jobKey = "TestJobA";
    jobdetailsA.m_jobEntry.m_builderGuid = builderUuid;

    jobA->Init(jobdetailsA);
    m_rcController.m_RCQueueSortModel.AddJobIdEntry(jobA);
    m_rcController.m_RCJobListModel.addNewJob(jobA);

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
    jobdetailsB.m_scanFolder = &scanFolderInfo;
    jobdetailsA.m_assetBuilderDesc = m_assetBuilderDesc;
    jobdetailsB.m_jobEntry.m_databaseSourceName = jobdetailsB.m_jobEntry.m_pathRelativeToWatchFolder = "fileB.txt";
    jobdetailsB.m_jobEntry.m_platformInfo = { "pc" ,{ "desktop", "renderer" } };
    jobdetailsB.m_jobEntry.m_watchFolderPath = scanFolderInfo.ScanPath();
    jobdetailsB.m_jobEntry.m_jobKey = "TestJobB";
    jobdetailsB.m_jobEntry.m_builderGuid = builderUuid;

    jobdetailsB.m_critical = true; //make jobB critical so that it will be analyzed first even though we want JobA to run first

    AssetBuilderSDK::SourceFileDependency sourceFileBDependency;
    sourceFileBDependency.m_sourceFileDependencyPath = "fileB.txt";

    AssetBuilderSDK::SourceFileDependency sourceFileADependency;
    sourceFileADependency.m_sourceFileDependencyPath = "fileA.txt";

    // Make job B has an order job dependency on Job A
    AssetBuilderSDK::JobDependency jobDependencyA("TestJobA", "pc", AssetBuilderSDK::JobDependencyType::Order, sourceFileADependency);
    jobdetailsB.m_jobDependencyList.push_back({ jobDependencyA });

    //Setting JobB
    MockRCJob* jobB = new MockRCJob(&m_rcController.m_RCJobListModel);
    jobB->Init(jobdetailsB);
    m_rcController.m_RCQueueSortModel.AddJobIdEntry(jobB);
    m_rcController.m_RCJobListModel.addNewJob(jobB);

    bool beginWorkB = false;
    QMetaObject::Connection conn = QObject::connect(jobB, &RCJob::BeginWork, this, [this, &beginWorkB, &jobFinishedA]()
    {
        // JobA should finish first before JobB starts
        UNIT_TEST_EXPECT_TRUE(jobFinishedA);
        beginWorkB = true;
    }
    );

    bool jobFinishedB = false;
    QObject::connect(jobB, &RCJob::JobFinished, this, [&jobFinishedB](AssetBuilderSDK::ProcessJobResponse /*result*/)
    {
        jobFinishedB = true;
    }
    );
    bool allJobsCompleted = false;
    QObject::connect(&m_rcController, &RCController::BecameIdle, this, [&allJobsCompleted]()
    {
        allJobsCompleted = true;
    }
    );

    JobEntry completedJob;

    QObject::connect(&m_rcController, &RCController::FileCompiled, this, [&completedJob](JobEntry entry, AssetBuilderSDK::ProcessJobResponse response)
    {
        completedJob = entry;
    });

    QObject::connect(&m_rcController, &RCController::FileCancelled, this, [&completedJob](JobEntry entry)
    {
        completedJob = entry;
    });

    QObject::connect(&m_rcController, &RCController::FileFailed, this, [&completedJob](JobEntry entry)
    {
        completedJob = entry;
    });

    QObject::connect(&m_rcController, &RCController::ActiveJobsCountChanged, this, [this, &completedJob](unsigned int /*count*/)
    {
        m_rcController.OnAddedToCatalog(completedJob);
        completedJob = {};
    });

    m_rcController.SetDispatchPaused(false);

    m_rcController.DispatchJobs();
    UNIT_TEST_EXPECT_TRUE(UnitTestUtils::BlockUntil(allJobsCompleted, 5000));
    UNIT_TEST_EXPECT_TRUE(jobFinishedB);

    // Now test the use case where we have a cyclic dependency,
    // although the order in which these job will start is not defined but we can ensure that
    // all the job finishes and RCController goes Idle
    allJobsCompleted = false;
    Reset();
    m_rcController.SetDispatchPaused(true);
    QObject::disconnect(conn);

    //Setting up JobC
    JobDetails jobdetailsC;
    jobdetailsC.m_scanFolder = &scanFolderInfo;
    jobdetailsC.m_assetBuilderDesc = m_assetBuilderDesc;
    jobdetailsC.m_jobEntry.m_databaseSourceName = jobdetailsC.m_jobEntry.m_pathRelativeToWatchFolder = "fileC.txt";
    jobdetailsC.m_jobEntry.m_platformInfo = { "pc" ,{ "desktop", "renderer" } };
    jobdetailsC.m_jobEntry.m_watchFolderPath = scanFolderInfo.ScanPath();
    jobdetailsC.m_jobEntry.m_jobKey = "TestJobC";
    jobdetailsC.m_jobEntry.m_builderGuid = builderUuid;

    AssetBuilderSDK::SourceFileDependency sourceFileCDependency;
    sourceFileCDependency.m_sourceFileDependencyPath = "fileC.txt";

    //Setting up Job D
    JobDetails jobdetailsD;
    jobdetailsD.m_scanFolder = &scanFolderInfo;
    jobdetailsD.m_assetBuilderDesc = m_assetBuilderDesc;
    jobdetailsD.m_jobEntry.m_databaseSourceName = jobdetailsD.m_jobEntry.m_pathRelativeToWatchFolder = "fileD.txt";
    jobdetailsD.m_jobEntry.m_platformInfo = { "pc" ,{ "desktop", "renderer" } };
    jobdetailsD.m_jobEntry.m_watchFolderPath = scanFolderInfo.ScanPath();
    jobdetailsD.m_jobEntry.m_jobKey = "TestJobD";
    jobdetailsD.m_jobEntry.m_builderGuid = builderUuid;
    AssetBuilderSDK::SourceFileDependency sourceFileDDependency;
    sourceFileDDependency.m_sourceFileDependencyPath = "fileD.txt";

    //creating cyclic job order dependencies i.e  JobC and JobD have order job dependency on each other
    AssetBuilderSDK::JobDependency jobDependencyC("TestJobC", "pc", AssetBuilderSDK::JobDependencyType::Order, sourceFileCDependency);
    AssetBuilderSDK::JobDependency jobDependencyD("TestJobD", "pc", AssetBuilderSDK::JobDependencyType::Order, sourceFileDDependency);
    jobdetailsC.m_jobDependencyList.push_back({ jobDependencyD });
    jobdetailsD.m_jobDependencyList.push_back({ jobDependencyC });

    MockRCJob* jobD = new MockRCJob(&m_rcController.m_RCJobListModel);
    MockRCJob* jobC = new MockRCJob(&m_rcController.m_RCJobListModel);

    jobC->Init(jobdetailsC);
    m_rcController.m_RCQueueSortModel.AddJobIdEntry(jobC);
    m_rcController.m_RCJobListModel.addNewJob(jobC);

    jobD->Init(jobdetailsD);
    m_rcController.m_RCQueueSortModel.AddJobIdEntry(jobD);
    m_rcController.m_RCJobListModel.addNewJob(jobD);

    m_rcController.SetDispatchPaused(false);
    m_rcController.DispatchJobs();
    UNIT_TEST_EXPECT_TRUE(UnitTestUtils::BlockUntil(allJobsCompleted, 5000));

    // Test case when source file is deleted before it started processing
    {
        int prevJobCount = rcJobListModel->itemCount();
        MockRCJob rcJobAddAndDelete;
        AssetProcessor::JobDetails jobDetailsToInitWithInsideScope;
        jobDetailsToInitWithInsideScope.m_jobEntry.m_pathRelativeToWatchFolder = jobDetailsToInitWithInsideScope.m_jobEntry.m_databaseSourceName = "someFile0.txt";
        jobDetailsToInitWithInsideScope.m_jobEntry.m_platformInfo = { "pc",{ "tools", "editor" } };
        jobDetailsToInitWithInsideScope.m_jobEntry.m_jobKey = "Text files";
        jobDetailsToInitWithInsideScope.m_jobEntry.m_sourceFileUUID = jobDetailsToInitWith.m_jobEntry.m_sourceFileUUID;
        rcJobAddAndDelete.Init(jobDetailsToInitWithInsideScope);
        rcJobListModel->addNewJob(&rcJobAddAndDelete);
        // verify that job was added
        UNIT_TEST_EXPECT_TRUE(rcJobListModel->itemCount() == prevJobCount + 1);
        m_rcController.RemoveJobsBySource("someFile0.txt");
        // verify that job was removed
        UNIT_TEST_EXPECT_TRUE(rcJobListModel->itemCount() == prevJobCount);
    }
    Q_EMIT UnitTestPassed();
}

#if !AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS
REGISTER_UNIT_TEST(RCcontrollerUnitTests)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_PROCESSOR_TESTS

