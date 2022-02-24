///*
// * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
// * its licensors.
// *
// * For complete copyright and license terms please see the LICENSE at the root of this
// * distribution (the "License"). All use of this software is governed by the License,
// * or, if provided, by the license below or the license accompanying this file. Do not
// * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// *
// */
//
//#include <TestImpactFramework/TestImpactRuntime.h>
//#include <TestImpactFramework/TestImpactUtils.h>
//    
//#include <TestImpactTestJobRunnerCommon.h>
//#include <TestImpactTestUtils.h>
//
//#include <Artifact/TestImpactArtifactException.h>
//#include <TestRunner/Run/TestImpactTestRunner.h>
//
//#include <AzCore/UnitTest/TestTypes.h>
//#include <AzCore/std/containers/array.h>
//#include <AzCore/std/containers/vector.h>
//#include <AzCore/std/optional.h>
//#include <AzCore/std/smart_ptr/unique_ptr.h>
//#include <AzCore/std/string/string.h>
//#include <AzTest/AzTest.h>
//
//namespace UnitTest
//{
//    TestImpact::TestRunner::Command GetRunCommandForTarget(AZStd::pair<TestImpact::RepoPath, TestImpact::RepoPath> testTarget)
//    {
//        return TestImpact::TestRunner::Command{ AZStd::string::format(
//            "%s %s AzRunUnitTests --gtest_output=xml:%s", LY_TEST_IMPACT_AZ_TESTRUNNER_BIN, testTarget.first.c_str(),
//            testTarget.second.c_str()) };
//    }
//
//    class TestRunnerFixture
//        : public AllocatorsTestFixture
//    {
//    public:
//        void SetUp() override;
//
//    protected:
//        using JobInfo = TestImpact::TestRunner::JobInfo;
//        using JobData = TestImpact::TestRunner::JobData;
//        using Command = TestImpact::TestRunner::Command;
//
//        AZStd::vector<JobInfo> m_jobInfos;
//        AZStd::unique_ptr<TestImpact::TestRunner> m_testRunner;
//        AZStd::vector<Command> m_testTargetJobArgs;
//        AZStd::vector<AZStd::pair<TestImpact::RepoPath, TestImpact::RepoPath>> m_testTargetPaths;
//        AZStd::vector<TestImpact::TestRun> m_expectedTestTargetRuns;
//        AZStd::vector<TestImpact::TestRunResult> m_expectedTestTargetResult;
//        size_t m_maxConcurrency = 0;
//    };
//
//    void TestRunnerFixture::SetUp()
//    {
//        UnitTest::AllocatorsTestFixture::SetUp();
//
//        TestImpact::DeleteFiles(LY_TEST_IMPACT_TEST_TARGET_RESULTS_DIR, "*.xml");
//
//        // first: path to test target bin
//        // second: path to test target gtest results file in XML format
//        const AZStd::string runPath = AZStd::string(LY_TEST_IMPACT_TEST_TARGET_RESULTS_DIR) + "/%s.Run.xml";
//        m_testTargetPaths.emplace_back(
//            LY_TEST_IMPACT_TEST_TARGET_A_BIN, AZStd::string::format(runPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_A_BASE_NAME));
//        m_testTargetPaths.emplace_back(
//            LY_TEST_IMPACT_TEST_TARGET_B_BIN, AZStd::string::format(runPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_B_BASE_NAME));
//        m_testTargetPaths.emplace_back(
//            LY_TEST_IMPACT_TEST_TARGET_C_BIN, AZStd::string::format(runPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_C_BASE_NAME));
//        m_testTargetPaths.emplace_back(
//            LY_TEST_IMPACT_TEST_TARGET_D_BIN, AZStd::string::format(runPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_D_BASE_NAME));
//
//        m_expectedTestTargetRuns.emplace_back(GetTestTargetATestRunSuites(), AZStd::chrono::milliseconds{500});
//        m_expectedTestTargetRuns.emplace_back(GetTestTargetBTestRunSuites(), AZStd::chrono::milliseconds{500});
//        m_expectedTestTargetRuns.emplace_back(GetTestTargetCTestRunSuites(), AZStd::chrono::milliseconds{500});
//        m_expectedTestTargetRuns.emplace_back(GetTestTargetDTestRunSuites(), AZStd::chrono::milliseconds{500});
//
//        m_expectedTestTargetResult.emplace_back(TestImpact::TestRunResult::Failed);
//        m_expectedTestTargetResult.emplace_back(TestImpact::TestRunResult::Passed);
//        m_expectedTestTargetResult.emplace_back(TestImpact::TestRunResult::Passed);
//        m_expectedTestTargetResult.emplace_back(TestImpact::TestRunResult::Passed);
//
//        for (const auto& testTarget : m_testTargetPaths)
//        {
//            m_testTargetJobArgs.emplace_back(GetRunCommandForTarget(testTarget));
//        }
//    }
//
//    // Fixture parameterized for different max number of concurrent jobs
//    class TestRunnerFixtureWithConcurrencyParams
//        : public TestRunnerFixture
//        , public ::testing::WithParamInterface<size_t>
//    {
//    public:
//        void SetUp() override
//        {
//            TestRunnerFixture::SetUp();
//            m_maxConcurrency = GetParam();
//        }
//    };
//
//    // Fixture parameterized for different max number of concurrent jobs and different job exception policies
//    class TestRunnerFixtureWithFailedToLaunchExceptionParams
//        : public TestRunnerFixture
//        , public ::testing::WithParamInterface<JobExceptionPolicy>
//    {
//    public:
//        void SetUp() override
//        {
//            TestRunnerFixture::SetUp();
//            m_maxConcurrency = 1;
//            m_jobExceptionPolicy = GetParam();
//        }
//
//    protected:
//        JobExceptionPolicy m_jobExceptionPolicy = JobExceptionPolicy::Never;
//    };
//
//    using ConcurrencyAndJobExceptionPermutation = AZStd::tuple
//        <
//            size_t, // Max number of concurrent processes
//            JobExceptionPolicy // Test job exception policy
//        >;
//
//    class TestRunnerFixtureWithExecutedWithFailureExceptionParams
//        : public TestRunnerFixture
//        , public ::testing::WithParamInterface<ConcurrencyAndJobExceptionPermutation>
//    {
//    public:
//        void SetUp() override
//        {
//            TestRunnerFixture::SetUp();
//            const auto& [maxConcurrency, jobExceptionPolicy] = GetParam();
//            m_maxConcurrency = maxConcurrency;
//            m_jobExceptionPolicy = jobExceptionPolicy;
//        }
//
//    protected:
//        JobExceptionPolicy m_jobExceptionPolicy = JobExceptionPolicy::Never;
//    };
//
//    namespace
//    {
//        AZStd::array<size_t, 4> MaxConcurrentRuns = { 1, 2, 3, 4 };
//
//        AZStd::array<JobExceptionPolicy, 2> FailedToLaunchExceptionPolicies = {
//            JobExceptionPolicy::Never, JobExceptionPolicy::OnFailedToExecute};
//
//        AZStd::array<JobExceptionPolicy, 2> ExecutedWithFailureExceptionPolicies = {
//            JobExceptionPolicy::Never, JobExceptionPolicy::OnExecutedWithFailure};
//    } // namespace
//
//    TEST_P(
//        TestRunnerFixtureWithFailedToLaunchExceptionParams,
//        InvalidCommandArgument_ExpectJobResulFailedToExecuteOrUnexecutedJobs)
//    {
//        // Given a test runner with no client callback or run timeout or runner timeout
//        m_testRunner = AZStd::make_unique<TestImpact::TestRunner>(m_maxConcurrency);
//
//        // Given a mixture of test run jobs with valid and invalid command arguments
//        for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//        {
//            const Command args = (jobId % 2) ? Command{ InvalidProcessPath } : m_testTargetJobArgs[jobId];
//            JobData jobData(m_testTargetPaths[jobId].second);
//            m_jobInfos.emplace_back(JobInfo({jobId}, args, AZStd::move(jobData)));
//        }
//
//        // When the test run jobs are executed with different exception policies
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::nullopt,
//        [this]([[maybe_unused]]const JobInfo& jobInfo, const TestImpact::JobMeta& meta)
//        {
//            if (m_jobExceptionPolicy == JobExceptionPolicy::OnFailedToExecute && meta.m_result == TestImpact::JobResult::FailedToExecute)
//            {
//                return TestImpact::ProcessCallbackResult::Abort;
//            }
//            else
//            {
//                return TestImpact::ProcessCallbackResult::Continue;
//            }
//        });
//
//
//        if (m_jobExceptionPolicy == JobExceptionPolicy::OnFailedToExecute)
//        {
//            ValidateTestRunCompleted(runnerJobs[0], m_expectedTestTargetResult[runnerJobs[0].GetJobInfo().GetId().m_value]);
//            ValidateTestTargetRun(runnerJobs[0].GetPayload().value(), m_expectedTestTargetRuns[runnerJobs[0].GetJobInfo().GetId().m_value]);
//
//            ValidateJobFailedToExecute(runnerJobs[1]);
//
//            for (auto i = 2; i < runnerJobs.size(); i++)
//            {
//                ValidateJobNotExecuted(runnerJobs[i]);
//            }
//
//            EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::UserAborted);
//        }
//        else
//        {
//            for (const auto& job : runnerJobs)
//            {
//                const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//                if (jobId % 2)
//                {
//                    // Expect invalid jobs have a job result of FailedToExecute
//                    ValidateJobFailedToExecute(job);
//                }
//                else
//                {
//                    // Expect the valid jobs to successfully result in a test run that matches the expected test run data
//                    ValidateTestRunCompleted(job, m_expectedTestTargetResult[jobId]);
//                    ValidateTestTargetRun(job.GetPayload().value(), m_expectedTestTargetRuns[jobId]);
//                }
//            }
//
//            EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);
//        }
//    }
//
//    TEST_P(
//        TestRunnerFixtureWithExecutedWithFailureExceptionParams,
//        ErroneousReturnCode_ExpectJobResultExecutedWithFailureOrInFlightTimeoutJobs)
//    {
//        // Given a test runner with no client callback or run timeout or runner timeout
//        m_testRunner = AZStd::make_unique<TestImpact::TestRunner>(m_maxConcurrency);
//
//        // Given a mixture of test run jobs that execute and return either successfully or with failure
//        for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//        {
//            JobData jobData(m_testTargetPaths[jobId].second);
//            const Command args = (jobId != 0)
//                ? Command{ AZStd::string::format("%s %s", ValidProcessPath, ConstructTestProcessArgs(0, MediumSleep).c_str()) }
//            : m_testTargetJobArgs[jobId];
//            m_jobInfos.emplace_back(JobInfo({jobId}, args, AZStd::move(jobData)));
//        }
//
//        // When the test run jobs are executed with different exception policies
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::nullopt,
//        [this]([[maybe_unused]] const JobInfo& jobInfo, const TestImpact::JobMeta& meta)
//        {
//            if (m_jobExceptionPolicy == JobExceptionPolicy::OnExecutedWithFailure && meta.m_result == TestImpact::JobResult::ExecutedWithFailure)
//            {
//                return TestImpact::ProcessCallbackResult::Abort;
//            }
//            else
//            {
//                return TestImpact::ProcessCallbackResult::Continue;
//            }
//        });
//
//        if (m_jobExceptionPolicy == JobExceptionPolicy::OnExecutedWithFailure)
//        {
//            ValidateJobExecutedWithFailedTests(runnerJobs[0]);
//            ValidateTestTargetRun(runnerJobs[0].GetPayload().value(), m_expectedTestTargetRuns[runnerJobs[0].GetJobInfo().GetId().m_value]);
//        
//            for (auto jobId = 1; jobId < runnerJobs.size(); jobId++)
//            {
//                if (jobId < m_maxConcurrency)
//                {
//                    ValidateJobTerminated(runnerJobs[jobId]);
//                }
//                else
//                {
//                    ValidateJobNotExecuted(runnerJobs[jobId]);
//                }
//            }
//
//            EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::UserAborted);
//        }
//        else
//        {
//            ValidateJobExecutedWithFailedTests(runnerJobs[0]);
//        
//            for (auto jobId = 1; jobId < runnerJobs.size(); jobId++)
//            {
//                // Expect the valid jobs to successfully result in a test run that matches the expected test run data
//                ValidateJobExecutedSuccessfullyNoPayload(runnerJobs[jobId]);
//            }
//
//            EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);
//        }        
//    }
//
//    TEST_F(TestRunnerFixture, EmptyArtifact_ExpectCompletedTestWithEmptyArtifact)
//    {
//        // Given a test runner with no client callback, concurrency, run timeout or runner timeout
//        m_testRunner = AZStd::make_unique<TestImpact::TestRunner>(OneConcurrentProcess);
//
//        // Given an test runner job that will return successfully but with an empty artifact string
//        m_jobInfos.emplace_back(JobInfo({0}, m_testTargetJobArgs[0], JobData("")));
//
//        // When the test runner job is executed
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt);
//
//        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);
//
//        // Expect each job to have completed with test failures albeit with an empty payload
//        ValidateJobExecutedWithFailure(runnerJobs[0]);
//        EXPECT_FALSE(runnerJobs[0].GetPayload().has_value());
//    }
//
//    TEST_F(TestRunnerFixture, InvalidRunArtifact_ExpectArtifactException)
//    {
//        // Given a test run artifact with invalid contents
//        TestImpact::WriteFileContents<TestImpact::Exception>("There is nothing valid here", m_testTargetPaths[TestTargetA].second);
//
//        // Given a job command that will write the test run artifact to a different location that what we will read from
//        auto invalidRunArtifact = m_testTargetPaths[TestTargetA];
//        invalidRunArtifact.second = invalidRunArtifact.second.String() + ".xml";
//        const Command args = GetRunCommandForTarget(invalidRunArtifact);
//
//        // Given a test runner with no client callback, concurrency, run timeout or runner timeout
//        m_testRunner = AZStd::make_unique<TestImpact::TestRunner>(OneConcurrentProcess);
//
//        // Given an test runner job that will return successfully but not produce an artifact
//        JobData jobData(m_testTargetPaths[TestTargetA].second);
//        m_jobInfos.emplace_back(JobInfo({TestTargetA}, args, AZStd::move(jobData)));
//
//        // When the test runner job is executed
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt);
//
//        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);
//
//        // Expect each job to have completed with test failures albeit with an empty payload
//        ValidateJobExecutedWithFailure(runnerJobs[0]);
//        EXPECT_FALSE(runnerJobs[0].GetPayload().has_value());
//    }
//
//    TEST_P(TestRunnerFixtureWithConcurrencyParams, RunTestTargets_RunsMatchTestSuitesInTarget)
//    {
//        // Given a test runner with no client callback, runner timeout or runner timeout
//        m_testRunner = AZStd::make_unique<TestImpact::TestRunner>(m_maxConcurrency);
//
//        // Given an test runner job for each test target with no runner caching
//        for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//        {
//            JobData jobData(m_testTargetPaths[jobId].second);
//            m_jobInfos.emplace_back(JobInfo({jobId}, m_testTargetJobArgs[jobId], AZStd::move(jobData)));
//        }
//
//        // When the test runner jobs are executed
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt);
//
//        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);
//
//        // Expect each job to successfully result in a test runner that matches the expected test runner data for that test target
//        for (const auto& job : runnerJobs)
//        {
//            const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//            ValidateTestRunCompleted(job, m_expectedTestTargetResult[jobId]);
//            ValidateTestTargetRun(job.GetPayload().value(), m_expectedTestTargetRuns[jobId]);
//        }
//    }
//
//    TEST_P(TestRunnerFixtureWithConcurrencyParams, RunTestTargetsWithArbitraryJobIds_RunsMatchTestSuitesInTarget)
//    {
//        // Given a set of arbitrary job ids to be used for the test target jobs
//        enum
//        {
//            ArbitraryA = 36,
//            ArbitraryB = 890,
//            ArbitraryC = 19,
//            ArbitraryD = 1
//        };
//
//        const AZStd::unordered_map<JobInfo::IdType, JobInfo::IdType> sequentialToArbitrary =
//        {
//            {TestTargetA, ArbitraryA},
//            {TestTargetB, ArbitraryB},
//            {TestTargetC, ArbitraryC},
//            {TestTargetD, ArbitraryD},
//        };
//
//        const AZStd::unordered_map<JobInfo::IdType, JobInfo::IdType> arbitraryToSequential =
//        {
//            {ArbitraryA, TestTargetA},
//            {ArbitraryB, TestTargetB},
//            {ArbitraryC, TestTargetC},
//            {ArbitraryD, TestTargetD},
//        };
//
//        // Given a test runner with no client callback, run timeout or runner timeout
//        m_testRunner = AZStd::make_unique<TestImpact::TestRunner>(m_maxConcurrency);
//
//        // Given an test run job for each test target
//        for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//        {
//            JobData jobData(m_testTargetPaths[jobId].second);
//            m_jobInfos.emplace_back(JobInfo({sequentialToArbitrary.at(jobId)}, m_testTargetJobArgs[jobId], AZStd::move(jobData)));
//        }
//
//        // When the test run jobs are executed
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt);
//
//        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);
//
//        // Expect each job to successfully result in a test run that matches the expected test run data for that test target
//        for (const auto& job : runnerJobs)
//        {
//            const JobInfo::IdType jobId = arbitraryToSequential.at(job.GetJobInfo().GetId().m_value);
//            ValidateTestRunCompleted(job, m_expectedTestTargetResult[jobId]);
//            ValidateTestTargetRun(job.GetPayload().value(), m_expectedTestTargetRuns[jobId]);
//        }
//    }
//
//    TEST_P(TestRunnerFixtureWithConcurrencyParams, RunTestTargetsWithCallback_RunsMatchTestSuitesInTarget)
//    {
//        // Given a client callback function that tracks the number of successful runs
//        size_t numSuccesses = 0;
//        const auto jobCallback =
//            [&numSuccesses]([[maybe_unused]] const TestImpact::TestRunner::JobInfo& jobInfo, const TestImpact::JobMeta& meta) {
//                if (meta.m_result == TestImpact::JobResult::ExecutedWithSuccess)
//                {
//                    numSuccesses++;
//                }
//
//                return TestImpact::ProcessCallbackResult::Continue;
//            };
//
//        // Given a test runner with no run timeout or runner timeout
//        m_testRunner = AZStd::make_unique<TestImpact::TestRunner>(m_maxConcurrency);
//
//        // Given an test run job for each test target
//        for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//        {
//            JobData jobData(m_testTargetPaths[jobId].second);
//            m_jobInfos.emplace_back(JobInfo({jobId}, m_testTargetJobArgs[jobId], AZStd::move(jobData)));
//        }
//
//        // When the test run jobs are executed
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::nullopt, jobCallback);
//
//        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);
//
//        // Expect the number of successful runs tracked in the callback to match the number of test targets run with no failures
//        EXPECT_EQ(
//            numSuccesses,
//            AZStd::count_if(m_expectedTestTargetResult.begin(), m_expectedTestTargetResult.end(), [](TestImpact::TestRunResult result) {
//                return result == TestImpact::TestRunResult::Passed;
//            }));
//
//        // Expect each job to successfully result in a test run that matches the expected test run data for that test target
//        for (const auto& job : runnerJobs)
//        {
//            const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//            ValidateTestRunCompleted(job, m_expectedTestTargetResult[jobId]);
//            ValidateTestTargetRun(job.GetPayload().value(), m_expectedTestTargetRuns[jobId]);
//        }
//    }
//
//    TEST_F(TestRunnerFixture, JobRunnerTimeout_InFlightJobsTimeoutAndQueuedJobsUnlaunched)
//    {
//        // Given a test runner with no client callback or runner timeout and 500ms run timeout
//        m_testRunner =
//            AZStd::make_unique<TestImpact::TestRunner>(1);
//
//        // Given an test run job for each test target where half will sleep indefinitely
//        for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//        {
//            JobData jobData(m_testTargetPaths[jobId].second);
//            const Command args = (jobId == 2)
//                ? Command{ AZStd::string::format("%s %s", ValidProcessPath, ConstructTestProcessArgs(jobId, LongSleep).c_str()) }
//                : m_testTargetJobArgs[jobId];
//            m_jobInfos.emplace_back(JobInfo({jobId}, args, AZStd::move(jobData)));
//        }
//
//        // When the test run jobs are executed
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::chrono::milliseconds(500), AZStd::nullopt);
//
//        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Timeout);
//
//        // Expect half the jobs to successfully result in a test run that matches the expected test run data for that test target
//        // with the other half having timed out
//        for (const auto& job : runnerJobs)
//        {
//            const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//            if (jobId < 2)
//            {
//                ValidateTestRunCompleted(job, m_expectedTestTargetResult[jobId]);
//                ValidateTestTargetRun(job.GetPayload().value(), m_expectedTestTargetRuns[jobId]);
//            }
//            else if (jobId == 2)
//            {
//                ValidateJobTimeout(job);
//            }
//            else
//            {
//                ValidateJobNotExecuted(job);
//            }
//        }
//    }
//
//    TEST_F(TestRunnerFixture, JobTimeout_InFlightJobTimeoutAndQueuedJobsUnlaunched)
//    {
//        // Given a test runner with no client callback or run timeout and a 5 second runner timeout
//        m_testRunner = AZStd::make_unique<TestImpact::TestRunner>(FourConcurrentProcesses);
//
//        // Given an test run job for each test target where half will sleep indefinitely
//        for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//        {
//            JobData jobData(m_testTargetPaths[jobId].second);
//            const Command args = (jobId % 2)
//                ? Command{ AZStd::string::format("%s %s", ValidProcessPath, ConstructTestProcessArgs(jobId, LongSleep).c_str()) }
//                : m_testTargetJobArgs[jobId];
//            m_jobInfos.emplace_back(JobInfo({jobId}, args, AZStd::move(jobData)));
//        }
//
//        // When the test run jobs are executed
//        const auto [result, runnerJobs] = m_testRunner->RunTests(m_jobInfos, AZStd::nullopt, AZStd::chrono::milliseconds(5000), AZStd::nullopt);
//
//        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Timeout);
//
//        // Expect half the jobs to successfully result in a test run that matches the expected test run data for that test target
//        // with the other half having timed out
//        for (const auto& job : runnerJobs)
//        {
//            const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//            if (jobId % 2)
//            {
//                ValidateJobTimeout(job);
//            }
//            else
//            {
//                ValidateTestRunCompleted(job, m_expectedTestTargetResult[jobId]);
//                ValidateTestTargetRun(job.GetPayload().value(), m_expectedTestTargetRuns[jobId]);
//            }
//        }
//    }
//
//    INSTANTIATE_TEST_CASE_P(
//        ,
//        TestRunnerFixtureWithFailedToLaunchExceptionParams,
//        ::testing::ValuesIn(FailedToLaunchExceptionPolicies));
//
//    INSTANTIATE_TEST_CASE_P(
//        ,
//        TestRunnerFixtureWithExecutedWithFailureExceptionParams,
//        ::testing::Combine(
//            ::testing::ValuesIn(MaxConcurrentRuns),
//            ::testing::ValuesIn(ExecutedWithFailureExceptionPolicies)));
//
//    INSTANTIATE_TEST_CASE_P(, TestRunnerFixtureWithConcurrencyParams, ::testing::ValuesIn(MaxConcurrentRuns));
//} // namespace UnitTest
