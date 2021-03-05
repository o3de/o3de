/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "RCControllerTest.h"


#include "native/resourcecompiler/rccontroller.h"
#include "AzCore/std/parallel/binary_semaphore.h"

TEST_F(RCcontrollerTest, CompileGroupCreatedWithUnknownStatusForFailedJobs)
{
    //Strategy Add a failed job to the job queue list and than ask the rc controller to request compile, it should emit unknown status  
    using namespace AssetProcessor;
    // we have to initialize this to something other than Assetstatus_Unknown here because later on we will be testing the value of assetstatus  
    AzFramework::AssetSystem::AssetStatus assetStatus = AzFramework::AssetSystem::AssetStatus_Failed; 
    RCController rcController;
    QObject::connect(&rcController, &RCController::CompileGroupCreated,
        [&assetStatus]([[maybe_unused]] AssetProcessor::NetworkRequestID groupID, AzFramework::AssetSystem::AssetStatus status)
    {
        assetStatus = status;
    }
    );
    RCJobListModel* rcJobListModel = rcController.GetQueueModel();
    RCJob* job = new RCJob(rcJobListModel);
    AssetProcessor::JobDetails jobDetails;
    jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = jobDetails.m_jobEntry.m_databaseSourceName = "somepath/failed.dds";
    jobDetails.m_jobEntry.m_platformInfo = { "pc", {"desktop", "renderer"} };
    jobDetails.m_jobEntry.m_jobKey = "Compile Stuff";
    job->SetState(RCJob::failed);
    job->Init(jobDetails);
    rcJobListModel->addNewJob(job);
    // Exact Match
    NetworkRequestID requestID(1, 1234);
    rcController.OnRequestCompileGroup(requestID, "pc", "somepath/failed.dds", AZ::Data::AssetId());
    ASSERT_TRUE(assetStatus == AzFramework::AssetSystem::AssetStatus_Unknown);

    assetStatus = AzFramework::AssetSystem::AssetStatus_Failed;
    // Broader Match
    rcController.OnRequestCompileGroup(requestID, "pc", "somepath", AZ::Data::AssetId() );
    ASSERT_TRUE(assetStatus == AzFramework::AssetSystem::AssetStatus_Unknown);
}

class RCcontrollerTest_Cancellation
    : public RCcontrollerTest
{
public:
    RCcontrollerTest_Cancellation()
    {
    }
    virtual ~RCcontrollerTest_Cancellation()
    {
    }

    void SetUp() override
    {
        RCcontrollerTest::SetUp();
        using namespace AssetProcessor;
        m_rcController.reset(new RCController());
        m_rcController->SetDispatchPaused(true);
        m_rcJobListModel = m_rcController->GetQueueModel();

        {
            RCJob* job = new RCJob(m_rcJobListModel);
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_jobEntry.m_computedFingerprint = 1;
            jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = jobDetails.m_jobEntry.m_databaseSourceName = "somepath/failed.dds";
            jobDetails.m_jobEntry.m_platformInfo = { "ios",{ "mobile", "renderer" } };
            jobDetails.m_jobEntry.m_jobRunKey = 1;
            jobDetails.m_jobEntry.m_jobKey = "tiff";
            job->SetState(RCJob::JobState::pending);
            job->Init(jobDetails);
            m_rcJobListModel->addNewJob(job);
        }

        {
            RCJob* job = new RCJob(m_rcJobListModel);
            // note that Init() is a move operation.  we cannot reuse jobDetails.
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_jobEntry.m_computedFingerprint = 1;
            jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = jobDetails.m_jobEntry.m_databaseSourceName = "somepath/failed.dds";
            jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
            jobDetails.m_jobEntry.m_jobRunKey = 2;
            jobDetails.m_jobEntry.m_jobKey = "tiff";
            job->SetState(RCJob::JobState::pending);
            job->Init(jobDetails);
            m_rcJobListModel->addNewJob(job);
            m_rcJobListModel->markAsStarted(job);
            m_rcJobListModel->markAsProcessing(job); // job is now "in flight"
        }

    }

    void TearDown() override
    {
        m_rcJobListModel = nullptr;
        m_rcController.reset();
        RCcontrollerTest::TearDown();
    }

    AZStd::unique_ptr<AssetProcessor::RCController> m_rcController;
    AssetProcessor::RCJobListModel* m_rcJobListModel = nullptr; // convenience pointer into m_rcController->GetQueueModel()
};

TEST_F(RCcontrollerTest_Cancellation, JobSubmitted_SameFingerprint_DoesNotCancelTheJob)
{
    // submit a new job for the same details as the already running one.
    {
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_computedFingerprint = 1; // same as above in SetUp
        jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = jobDetails.m_jobEntry.m_databaseSourceName = "somepath/failed.dds";
        jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
        jobDetails.m_jobEntry.m_jobKey = "tiff";
        jobDetails.m_jobEntry.m_jobRunKey = 3;
        m_rcController->JobSubmitted(jobDetails);
    }

    for (int idx = 0; idx < m_rcJobListModel->itemCount(); idx++)
    {
        // neither job should be cancelled.
        AssetProcessor::RCJob* rcJob = m_rcJobListModel->getItem(idx);
        ASSERT_TRUE(rcJob->GetState() != AssetProcessor::RCJob::JobState::cancelled);
    }
}

TEST_F(RCcontrollerTest_Cancellation, JobSubmitted_DifferentFingerprint_CancelsTheJob_OnlyIfInProgress)
{
    // submit a new job for the same details as the already running one.
    {
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_computedFingerprint = 2; // different from setup.
        jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = jobDetails.m_jobEntry.m_databaseSourceName = "somepath/failed.dds";
        jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
        jobDetails.m_jobEntry.m_jobKey = "tiff";
        jobDetails.m_jobEntry.m_jobRunKey = 3;
        m_rcController->JobSubmitted(jobDetails);
    }

    for (int idx = 0; idx < m_rcJobListModel->itemCount(); idx++)
    {
        // neither job should be cancelled.
        AssetProcessor::RCJob* rcJob = m_rcJobListModel->getItem(idx);
        if (rcJob->GetJobEntry().m_jobRunKey == 2)
        {
            // the one with run key 2 should have been cancelled and replaced with run key 3
            ASSERT_TRUE(rcJob->GetState() == AssetProcessor::RCJob::JobState::cancelled);
        }
        else
        {
            // the other one should have been left alone since it had not yet begun.
            ASSERT_TRUE(rcJob->GetState() != AssetProcessor::RCJob::JobState::cancelled);
        }
    }
}

class RCcontrollerTest_Simple
    : public RCcontrollerTest
{
public:

    void SetUp() override
    {
        RCcontrollerTest::SetUp();
        using namespace AssetProcessor;
        m_rcController.reset(new RCController(/*minJobs*/1, /*maxJobs*/1));
        m_rcController->SetDispatchPaused(false);
        m_rcJobListModel = m_rcController->GetQueueModel();

        qRegisterMetaType<AssetBuilderSDK::ProcessJobResponse>("ProcessJobResponse");

        QObject::connect(m_rcController.get(), &RCController::BecameIdle, [this]()
        {
            m_wait.release();
        });
    }

    void TearDown() override
    {
        m_rcJobListModel = nullptr;
        m_rcController.reset();
        RCcontrollerTest::TearDown();
    }
    void SubmitJob();
    AZStd::binary_semaphore m_wait;
    AZStd::unique_ptr<AssetProcessor::RCController> m_rcController;
    AssetProcessor::RCJobListModel* m_rcJobListModel = nullptr; // convenience pointer into m_rcController->GetQueueModel()
};

void RCcontrollerTest_Simple::SubmitJob()
{
    using namespace AssetBuilderSDK;

    {
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_computedFingerprint = 123;
        jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = jobDetails.m_jobEntry.m_databaseSourceName = "somepath/a.dds";
        jobDetails.m_jobEntry.m_platformInfo = { "pc",{ "desktop", "renderer" } };
        jobDetails.m_jobEntry.m_jobKey = "tiff";
        jobDetails.m_jobEntry.m_jobRunKey = 3;
        jobDetails.m_assetBuilderDesc.m_processJobFunction = []([[maybe_unused]] const ProcessJobRequest& request, ProcessJobResponse& response)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        };
        m_rcController->JobSubmitted(jobDetails);
    }

    // Numbers are a bit arbitrary but this should result in a max wait time of 5s
    int retryCount = 100;
    do
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    } while (m_wait.try_acquire_for(AZStd::chrono::milliseconds(5)) == false && --retryCount > 0);

    ASSERT_GT(retryCount, 0);
}

// This is a regresssion test to ensure the rccontroller can handle multiple jobs for the same file being completed before
// the APM has a chance to send OnFinishedProcesssingJob events
TEST_F(RCcontrollerTest_Simple, SameJobIsCompletedMultipleTimes_CompletesWithoutError)
{
    using namespace AssetProcessor;
    
    AZStd::vector<JobEntry> jobEntries;
    QObject::connect(m_rcController.get(), &RCController::FileCompiled, [&jobEntries](JobEntry entry, AssetBuilderSDK::ProcessJobResponse response)
    {
        jobEntries.push_back(entry);
    });

    SubmitJob();
    SubmitJob();

    ASSERT_EQ(jobEntries.size(), 2);

    for (const JobEntry& entry : jobEntries)
    {
        m_rcController->OnAddedToCatalog(entry);
    }

    ASSERT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 4); // Expected that there are 4 errors related to the files not existing on disk.  Error message: GenerateFingerprint was called but no input files were requested for fingerprinting.
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
}

// makes sure to expose parts of RCJob to the unit test
class TestRCJob : public AssetProcessor::RCJob
{
    friend class GTEST_TEST_CLASS_NAME_(RCcontrollerTest, BuilderSDK_API_ProcessJob_HasValidParameters_WithOutputFolder);
public:
    explicit TestRCJob(QObject* parent = 0) : AssetProcessor::RCJob(parent) {};
};

TEST_F(RCcontrollerTest, BuilderSDK_API_ProcessJob_HasValidParameters_WithOutputFolder)
{
    // this test makes sure that the BuilderSDK API is not exposed to any database internals.

    AZ::Uuid sourceUUID = AZ::Uuid::CreateRandom();
    AZ::Uuid builderGuid = AZ::Uuid::CreateRandom();
    
    AssetBuilderSDK::ProcessJobRequest req;
    {
        // note that this scope is intentional.  job.Init(jobDetails) below is actually a by-ref operation that destroys JobDetails in the process.
        AssetProcessor::JobDetails jobDetails;

        // the crux of this test: the database source name differs from the relative path to watch folder:
        jobDetails.m_jobEntry.m_pathRelativeToWatchFolder = "SomeThing.tif"; // case sensitive

        // Note that while this is an OS-SPECIFIC path, this unit test does not actually invoke the file system at all
        // and only operates on in-memory structures, so it should work on every platform.
        jobDetails.m_jobEntry.m_watchFolderPath = "c:/test/a/B/c"; // just to make sure case is preserved
        jobDetails.m_jobEntry.m_databaseSourceName = "somepath/SomeThing.tif"; // case sensitive but outputprefixes are generally lowcase

        jobDetails.m_jobEntry.m_sourceFileUUID = sourceUUID;
        jobDetails.m_jobEntry.m_platformInfo = { "ios",{ "mobile", "renderer" } };
        jobDetails.m_jobEntry.m_jobRunKey = 1;
        jobDetails.m_jobEntry.m_jobKey = "tiff";
        jobDetails.m_jobEntry.m_builderGuid = builderGuid;

        TestRCJob job;
        job.Init(jobDetails);
        job.PopulateProcessJobRequest(req);
    }

    EXPECT_STREQ(req.m_sourceFile.c_str(), "SomeThing.tif");
    EXPECT_STREQ(req.m_watchFolder.c_str(), "c:/test/a/B/c");

    // the crux of the test, tested:  make sure that it does not contain 'somepath' in there just becuase its part of the Database Source Name.
    EXPECT_STREQ(req.m_fullPath.c_str(), "c:/test/a/B/c/SomeThing.tif");
    EXPECT_EQ(req.m_builderGuid, builderGuid);
    EXPECT_TRUE(req.m_platformInfo.HasTag("renderer"));
    EXPECT_TRUE(req.m_platformInfo.HasTag("mobile"));
    EXPECT_STREQ(req.m_platformInfo.m_identifier.c_str(), "ios");
    EXPECT_EQ(req.m_sourceFileUUID, sourceUUID);
}
