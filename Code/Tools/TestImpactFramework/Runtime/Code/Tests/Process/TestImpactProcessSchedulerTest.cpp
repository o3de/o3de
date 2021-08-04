/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactTestUtils.h>

#include <Process/Scheduler/TestImpactProcessScheduler.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/tuple.h>
#include <AzTest/AzTest.h>

#include <vector>

namespace UnitTest
{
    using SchedulerPermutation = AZStd::tuple
        <
        size_t,                     // Number of concurrent processes
        size_t                      // Number of processes to launch
        >;

    class ProcessSchedulerTestFixtureWithParams
        : public AllocatorsTestFixture
        , private AZ::Debug::TraceMessageBus::Handler
        , public ::testing::WithParamInterface<SchedulerPermutation>
    {
    private:
        bool OnOutput(const char*, const char*) override;
        void SetUp() override;
        void TearDown() override;

    protected:
        void ConfigurePermutation();
        void ExpectSuccessfulLaunch(TestImpact::ProcessId pid);
        void ExpectGracefulExit(TestImpact::ProcessId pid);
        void ExpectUnsuccessfulLaunch(TestImpact::ProcessId pid);
        void ExpectExitCondition(
            TestImpact::ProcessId pid, AZStd::optional<TestImpact::ExitCondition> expectedExitCondition = AZStd::nullopt);
        void DoNotExpectExitCondition(TestImpact::ProcessId pid);
        void ExpectTerminatedProcess(TestImpact::ProcessId pid);
        void ExpectTimeoutProcess(TestImpact::ProcessId pid);
        void ExpectUnlaunchedProcess(TestImpact::ProcessId pid);
        void ExpectLargeStdOutput(TestImpact::ProcessId pid);
        void ExpectLargeStdError(TestImpact::ProcessId pid);
        void ExpectSmallStdOutput(TestImpact::ProcessId pid);
        void ExpectSmallStdError(TestImpact::ProcessId pid);
        void DoNotExpectStdOutput(TestImpact::ProcessId pid);
        void DoNotExpectStdError(TestImpact::ProcessId pid);

        struct ProcessResult
        {
            AZStd::optional<TestImpact::LaunchResult> m_launchResult;
            AZStd::optional<TestImpact::ExitCondition> m_exitStatus;
            AZStd::optional<AZStd::chrono::high_resolution_clock::time_point> m_createTime;
            AZStd::optional<AZStd::chrono::high_resolution_clock::time_point> m_exitTime;
            AZStd::chrono::milliseconds m_duration = AZStd::chrono::milliseconds(0);
            AZStd::optional<TestImpact::ReturnCode> m_returnCode;
            TestImpact::StdContent m_std;
        };

        size_t m_numMaxConcurrentProcesses = 0;
        size_t m_numProcessesToLaunch = 0;

        AZStd::unique_ptr<TestImpact::ProcessScheduler> m_processScheduler;
        TestImpact::ProcessLaunchCallback m_processLaunchCallback;
        TestImpact::ProcessExitCallback m_processExitCallback;
        AZStd::vector<TestImpact::ProcessInfo> m_processInfos;
        AZStd::vector<ProcessResult> m_processResults;
    };

    // Permutation values for small process batches
    const std::vector<size_t> SmallNumMaxConcurrentProcesses = {1, 4, 8};
    const std::vector<size_t> SmallNumProcessesToLaunch = {1, 8, 32};

    // Permutation values for large process batches
    const std::vector<size_t> LargeNumMaxConcurrentProcesses = {16, 32, 64};
    const std::vector<size_t> LargeNumProcessesToLaunch = {128, 256, 512};

    void ProcessSchedulerTestFixtureWithParams::ConfigurePermutation()
    {
        const auto& [numMaxConcurrentProcesses, numProcessesToLaunch] = GetParam();
        m_numMaxConcurrentProcesses = numMaxConcurrentProcesses;
        m_numProcessesToLaunch = numProcessesToLaunch;
    }

    // Consume AZ log spam
    bool ProcessSchedulerTestFixtureWithParams::OnOutput([[maybe_unused]] const char*, [[maybe_unused]] const char*)
    {
        return true;
    }

    void ProcessSchedulerTestFixtureWithParams::SetUp()
    {
        UnitTest::AllocatorsTestFixture::SetUp();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        ConfigurePermutation();

        m_processLaunchCallback = [this](
            TestImpact::ProcessId pid,
            TestImpact::LaunchResult launchResult,
            AZStd::chrono::high_resolution_clock::time_point createTime)
        {
            m_processResults[pid].m_launchResult = launchResult;
            m_processResults[pid].m_createTime.emplace(createTime);
            return TestImpact::ProcessCallbackResult::Continue;
        };

        m_processExitCallback = [this](
            TestImpact::ProcessId pid,
            TestImpact::ExitCondition exitStatus,
            TestImpact::ReturnCode returnCode,
            TestImpact::StdContent&& std,
            AZStd::chrono::high_resolution_clock::time_point exitTime)
        {
            m_processResults[pid].m_std = std::move(std);
            m_processResults[pid].m_returnCode.emplace(returnCode);
            m_processResults[pid].m_exitStatus = exitStatus;
            m_processResults[pid].m_exitTime = exitTime;
            m_processResults[pid].m_duration =
                AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(exitTime - *m_processResults[pid].m_createTime);
            return TestImpact::ProcessCallbackResult::Continue;
        };

        m_processResults.resize(m_numProcessesToLaunch);
    }

    void ProcessSchedulerTestFixtureWithParams::TearDown()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        UnitTest::AllocatorsTestFixture::TearDown();
    }

    // Expects the process to have exited under the specified circumstances
    void ProcessSchedulerTestFixtureWithParams::ExpectExitCondition(
        TestImpact::ProcessId pid, AZStd::optional<TestImpact::ExitCondition> expectedExitCondition)
    {
        const auto& [launchResult, exitStatus, createTime, exitTime, duration, returnCode, std] = m_processResults[pid];

        // Expect the processes to have exited
        EXPECT_TRUE(exitStatus.has_value());

        // Expect the return code to be valid
        EXPECT_TRUE(returnCode.has_value());

        if (expectedExitCondition.has_value())
        {
            // Expect the process to have exited under the specified conditions
            EXPECT_EQ(exitStatus, expectedExitCondition);

            // Expect the return code to be that of the process's is (for gracefull exists) or that of the error code
            // associated with the abnormal exit condition
            EXPECT_EQ(returnCode,
                expectedExitCondition == TestImpact::ExitCondition::Gracefull
                ? pid
                : static_cast<TestImpact::ReturnCode>(*exitStatus));
        }

        // Expect the duration to be non zero and the create time and exit time to have values as the processes has been in-flight
        EXPECT_GT(duration.count(), 0);
        EXPECT_TRUE(createTime.has_value());
        EXPECT_TRUE(exitTime.has_value());

        // Expect the exit time to be later than the start time
        EXPECT_GT(exitTime, createTime);
    }

    // Expects the process to not have exited for to lack of being in-flight
    void ProcessSchedulerTestFixtureWithParams::DoNotExpectExitCondition(TestImpact::ProcessId pid)
    {
        const auto& [launchResult, exitStatus, createTime, exitTime, duration, returnCode, std] = m_processResults[pid];

        // Expect the processes to not have exited as the process has not been launched successfully
        EXPECT_FALSE(exitStatus.has_value());

        // Expect the return code to be invalid
        EXPECT_FALSE(returnCode.has_value());

        // Expect the duration to be zero as the process has not been in-flight
        EXPECT_EQ(duration.count(), 0);

        // Do not expect the exit time to have a value
        EXPECT_FALSE(exitTime.has_value());
    }

    // Expects the process to have been launched successfully, makes no assumptions about how it exited
    void ProcessSchedulerTestFixtureWithParams::ExpectSuccessfulLaunch(TestImpact::ProcessId pid)
    {
        const auto& [launchResult, exitStatus, createTime, exitTime, duration, returnCode, std] = m_processResults[pid];

        // Expect a launch to have been attempted by this process (not still in queue)
        EXPECT_TRUE(launchResult.has_value());

        // Expect the process to have launched successfully
        EXPECT_EQ(launchResult, TestImpact::LaunchResult::Success);
    }

    // Expects the process to have failed to launch
    void ProcessSchedulerTestFixtureWithParams::ExpectUnsuccessfulLaunch(TestImpact::ProcessId pid)
    {
        const auto& [launchResult, exitStatus, createTime, exitTime, duration, returnCode, std] = m_processResults[pid];

        // Expect a launch to have been attempted by this process (not still in queue)
        EXPECT_TRUE(launchResult.has_value());

        // Expect the process to have launched unsuccessfully
        EXPECT_EQ(launchResult, TestImpact::LaunchResult::Failure);

        // Expect the create time to have a value as the process was technically created
        EXPECT_TRUE(createTime.has_value());

        DoNotExpectExitCondition(pid);
    }

    // Expects the process to have exited gracefully of its own accord (i.e. not terminated for any reason by the scheduler)
    void ProcessSchedulerTestFixtureWithParams::ExpectGracefulExit(TestImpact::ProcessId pid)
    {
        ExpectSuccessfulLaunch(pid);
        ExpectExitCondition(pid, TestImpact::ExitCondition::Gracefull);
    }

    // Expects the process to have been terminated by the client or scheduler
    void ProcessSchedulerTestFixtureWithParams::ExpectTerminatedProcess(TestImpact::ProcessId pid)
    {
        ExpectSuccessfulLaunch(pid);
        ExpectExitCondition(pid, TestImpact::ExitCondition::Terminated);
    }

    // Expects the process to have been terminated by the scheduler due to the process or scheduler timing out
    void ProcessSchedulerTestFixtureWithParams::ExpectTimeoutProcess(TestImpact::ProcessId pid)
    {
        ExpectSuccessfulLaunch(pid);
        ExpectExitCondition(pid, TestImpact::ExitCondition::Timeout);
    }

    // Expects the process to have not been attempted to launch (was still in the queue)
    void ProcessSchedulerTestFixtureWithParams::ExpectUnlaunchedProcess(TestImpact::ProcessId pid)
    {
        const auto& [launchResult, exitStatus, createTime, exitTime, duration, returnCode, std] = m_processResults[pid];

        // Expect a launch to not have been attempted by this process (still in queue)
        EXPECT_FALSE(launchResult.has_value());

        // Expect the create time to have a value as the process was technically created
        EXPECT_FALSE(createTime.has_value());

        DoNotExpectExitCondition(pid);
    }

    // Expects the std output to have been captured from the process with a large volume of text
    void ProcessSchedulerTestFixtureWithParams::ExpectLargeStdOutput(TestImpact::ProcessId pid)
    {
        const auto& stdOut = m_processResults[pid].m_std.m_out;

        // Expect standard output to have a value
        EXPECT_TRUE(stdOut.has_value());

        // Expect the output length to be that of the large text output from the child process
        EXPECT_EQ(stdOut.value().length(), LargeTextSize);
    }

    // Expects the std error to have been captured from the process with a large volume of text
    void ProcessSchedulerTestFixtureWithParams::ExpectLargeStdError(TestImpact::ProcessId pid)
    {
        const auto& stdErr = m_processResults[pid].m_std.m_err;

        // Expect standard error to have a value
        EXPECT_TRUE(stdErr.has_value());

        // Expect the error length to be that of the large text output from the child process
        EXPECT_EQ(stdErr.value().length(), LargeTextSize);
    }

    // Expects the std output to have been captured from the process with a small known text string
    void ProcessSchedulerTestFixtureWithParams::ExpectSmallStdOutput(TestImpact::ProcessId pid)
    {
        const auto& stdOut = m_processResults[pid].m_std.m_out;

        // Expect standard output to have a value
        EXPECT_TRUE(stdOut.has_value());

        // Expect the output to match the known stdout of the child
        EXPECT_EQ(stdOut.value(), KnownTestProcessOutputString(pid));
    }

    // Expects the std error to have been captured from the process with a small known text string
    void ProcessSchedulerTestFixtureWithParams::ExpectSmallStdError(TestImpact::ProcessId pid)
    {
        const auto& stdErr = m_processResults[pid].m_std.m_err;

        // Expect standard error to have a value
        EXPECT_TRUE(stdErr.has_value());

        // Expect the output to match the known stderr of the child
        EXPECT_EQ(stdErr.value(), KnownTestProcessErrorString(pid));
    }

    // Expects no standard output from the child process
    void ProcessSchedulerTestFixtureWithParams::DoNotExpectStdOutput(TestImpact::ProcessId pid)
    {
        const auto& stdOut = m_processResults[pid].m_std.m_out;

        // Do not expect standard output to have a value
        EXPECT_FALSE(stdOut.has_value());
    }

    // Expects no standard error from the child process
    void ProcessSchedulerTestFixtureWithParams::DoNotExpectStdError(TestImpact::ProcessId pid)
    {
        const auto& stdErr = m_processResults[pid].m_std.m_err;

        // Do not expect standard error to have a value
        EXPECT_FALSE(stdErr.has_value());
    }

    TEST_P(ProcessSchedulerTestFixtureWithParams, ValidProcesses_SuccessfulLaunches)
    {
        // Given a set of processes to launch with valid arguments
        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            m_processInfos.emplace_back(
                pid,
                ValidProcessPath,
                ConstructTestProcessArgs(pid, NoSleep));
        }

        // When the process scheduler launches the processes
        m_processScheduler = AZStd::make_unique<TestImpact::ProcessScheduler>(m_numMaxConcurrentProcesses);

        const auto result = m_processScheduler->Execute(
            m_processInfos,
            AZStd::nullopt,
            AZStd::nullopt,
            m_processLaunchCallback,
            m_processExitCallback
            );

        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);

        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            ExpectGracefulExit(pid);
        }
    }

    TEST_P(ProcessSchedulerTestFixtureWithParams, ValidAndInvalidProcesses_LaunchSuccessesAndFailures)
    {
        const size_t invalidProcessGroup = 4;

        // Given a mixture of processes to launch with valid and invalid arguments
        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            if (pid % invalidProcessGroup == 0)
            {
                m_processInfos.emplace_back(
                    pid,
                    InvalidProcessPath,
                    ConstructTestProcessArgs(pid, NoSleep));
            }
            else
            {
                m_processInfos.emplace_back(
                    pid,
                    ValidProcessPath,
                    ConstructTestProcessArgs(pid, NoSleep));
            }
        }

        // When the process scheduler launches the processes
        m_processScheduler = AZStd::make_unique<TestImpact::ProcessScheduler>(m_numMaxConcurrentProcesses);

        const auto result = m_processScheduler->Execute(
            m_processInfos,
            AZStd::nullopt,
            AZStd::nullopt,
            m_processLaunchCallback,
            m_processExitCallback
        );

        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);

        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            if (pid % invalidProcessGroup == 0)
            {
                ExpectUnsuccessfulLaunch(pid);
            }
            else
            {
                ExpectGracefulExit(pid);
            }

            DoNotExpectStdOutput(pid);
            DoNotExpectStdError(pid);
        }
    }

    TEST_P(ProcessSchedulerTestFixtureWithParams, ProcessTimeouts_InFlightProcessesTimeout)
    {
        const size_t timeoutProcessGroup = 4;

        // Given a mixture of processes to launch with some processes sleeping indefinately
        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            if (pid % timeoutProcessGroup == 0)
            {
                m_processInfos.emplace_back(pid, ValidProcessPath, ConstructTestProcessArgs(pid, LongSleep));
            }
            else
            {
                m_processInfos.emplace_back(pid, ValidProcessPath, ConstructTestProcessArgs(pid, NoSleep));
            }
        }

        // When the process scheduler launches the processes with a process timeout value
        m_processScheduler = AZStd::make_unique<TestImpact::ProcessScheduler>(m_numMaxConcurrentProcesses);

        const auto result = m_processScheduler->Execute(
            m_processInfos,
            AZStd::chrono::milliseconds(100),
            AZStd::nullopt,
            m_processLaunchCallback,
            m_processExitCallback
        );

        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);

        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            if (pid % timeoutProcessGroup == 0)
            {
                ExpectTimeoutProcess(pid);
            }
            else
            {
                ExpectExitCondition(pid);
            }

            DoNotExpectStdOutput(pid);
            DoNotExpectStdError(pid);
        }
    }

    TEST_P(
        ProcessSchedulerTestFixtureWithParams, ProcessLaunchCallbackAbort_InFlightProcessesTerminatedAndQueuedProcessesUnlaunched)
    {
        const size_t processToAbort = 8;

        // Given a set of processes to launch
        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            m_processInfos.emplace_back(
                pid,
                ValidProcessPath,
                ConstructTestProcessArgs(pid, NoSleep));
        }

        // Given a launch callback that will return the abort value for the process to abort
        const auto abortingLaunchCallback = [this, processToAbort](
            TestImpact::ProcessId pid,
            TestImpact::LaunchResult launchResult,
            AZStd::chrono::high_resolution_clock::time_point createTime)
        {
            m_processLaunchCallback(pid, launchResult, createTime);

            if (pid == processToAbort)
            {
                return TestImpact::ProcessCallbackResult::Abort;
            }
            else
            {
                return TestImpact::ProcessCallbackResult::Continue;
            }
        };

        // When the process scheduler launches the processes
        m_processScheduler = AZStd::make_unique<TestImpact::ProcessScheduler>(m_numMaxConcurrentProcesses);

        const auto result = m_processScheduler->Execute(
            m_processInfos,
            AZStd::nullopt,
            AZStd::nullopt,
            abortingLaunchCallback,
            m_processExitCallback
        );

        //EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);

        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            if (pid < processToAbort)
            {
                ExpectSuccessfulLaunch(pid);
            }
            else if (pid == processToAbort)
            {
                ExpectTerminatedProcess(pid);
            }
            else
            {
                ExpectUnlaunchedProcess(pid);
            }

            DoNotExpectStdOutput(pid);
            DoNotExpectStdError(pid);
        }
    }

    TEST_P(ProcessSchedulerTestFixtureWithParams, ProcessExitCallbackAbort_InFlightProcessesTerminated)
    {
        const size_t processToAbort = 4;

        // Given a set of processes to launch
        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            m_processInfos.emplace_back(
                pid,
                ValidProcessPath,
                ConstructTestProcessArgs(pid, NoSleep));
        }

        // Given an exit callback that will return the abort value for the process to abort
        const auto abortingExitCallback = [this, processToAbort](
            TestImpact::ProcessId pid,
            TestImpact::ExitCondition exitStatus,
            TestImpact::ReturnCode returnCode,
            TestImpact::StdContent&& std,
            AZStd::chrono::high_resolution_clock::time_point exitTime)
        {
            m_processExitCallback(pid, exitStatus, returnCode, std::move(std), exitTime);

            if (pid == processToAbort)
            {
                return TestImpact::ProcessCallbackResult::Abort;
            }
            else
            {
                return TestImpact::ProcessCallbackResult::Continue;
            }
        };

        // When the process scheduler launches the processes
        m_processScheduler = AZStd::make_unique<TestImpact::ProcessScheduler>(m_numMaxConcurrentProcesses);

        const auto result = m_processScheduler->Execute(
            m_processInfos,
            AZStd::nullopt,
            AZStd::nullopt,
            m_processLaunchCallback,
            abortingExitCallback
        );

        //EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);

        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            if (pid < processToAbort)
            {
                ExpectSuccessfulLaunch(pid);
            }
            else if (pid == processToAbort)
            {
                ExpectGracefulExit(pid);
            }
            else
            {
                if (m_processResults[pid].m_launchResult.has_value())
                {
                    ExpectSuccessfulLaunch(pid);
                }
                else
                {
                    ExpectUnlaunchedProcess(pid);
                }
            }

            DoNotExpectStdOutput(pid);
            DoNotExpectStdError(pid);
        }
    }

    TEST_P(ProcessSchedulerTestFixtureWithParams, SchedulerTimeout_QueuedAndInFlightProcessesTerminated)
    {
        const size_t processToTimeout = 0;

        // Given a set of processes to launch where one process will sleep indefinately
        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            if (pid == processToTimeout)
            {
                m_processInfos.emplace_back(
                    pid,
                    ValidProcessPath,
                    ConstructTestProcessArgs(pid, LongSleep));
            }
            else
            {
                m_processInfos.emplace_back(
                    pid,
                    ValidProcessPath,
                    ConstructTestProcessArgs(pid, NoSleep));
            }
        }

        // When the process scheduler launches the processes with a scheduler timeout value
        m_processScheduler = AZStd::make_unique<TestImpact::ProcessScheduler>(m_numMaxConcurrentProcesses);

        const auto result = m_processScheduler->Execute(
            m_processInfos,
            AZStd::nullopt,
            AZStd::chrono::milliseconds(100),
            m_processLaunchCallback,
            m_processExitCallback
        );

        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Timeout);

        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            if (pid == processToTimeout)
            {
                ExpectTimeoutProcess(pid);
            }
            else
            {
                if (m_processResults[pid].m_launchResult.has_value())
                {
                    ExpectSuccessfulLaunch(pid);
                }
                else
                {
                    ExpectUnlaunchedProcess(pid);
                }
            }

            DoNotExpectStdOutput(pid);
            DoNotExpectStdError(pid);
        }
    }

    TEST_P(ProcessSchedulerTestFixtureWithParams, RedirectStdOut_StdOutputIsLargeTextString)
    {
        // Given a set of processes to launch
        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            m_processInfos.emplace_back(
                pid,
                TestImpact::StdOutputRouting::ToParent,
                TestImpact::StdErrorRouting::None,
                ValidProcessPath,
                ConstructTestProcessArgsLargeText(pid, NoSleep));
        }

        // When the process scheduler launches the processes
        m_processScheduler = AZStd::make_unique<TestImpact::ProcessScheduler>(m_numMaxConcurrentProcesses);

        const auto result = m_processScheduler->Execute(
            m_processInfos,
            AZStd::nullopt,
            AZStd::nullopt,
            m_processLaunchCallback,
            m_processExitCallback
        );

        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);

        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            ExpectGracefulExit(pid);
            ExpectLargeStdOutput(pid);
            DoNotExpectStdError(pid);
        }
    }

    TEST_P(ProcessSchedulerTestFixtureWithParams, RedirectStdError_StdErrorIsLargeTextString)
    {
        // Given a set of processes to launch
        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            m_processInfos.emplace_back(
                pid,
                TestImpact::StdOutputRouting::None,
                TestImpact::StdErrorRouting::ToParent,
                ValidProcessPath,
                ConstructTestProcessArgsLargeText(pid, NoSleep));
        }

        // When the process scheduler launches the processes
        m_processScheduler = AZStd::make_unique<TestImpact::ProcessScheduler>(m_numMaxConcurrentProcesses);

        const auto result = m_processScheduler->Execute(
            m_processInfos,
            AZStd::nullopt,
            AZStd::nullopt,
            m_processLaunchCallback,
            m_processExitCallback
        );

        EXPECT_EQ(result, TestImpact::ProcessSchedulerResult::Graceful);

        for (size_t pid = 0; pid < m_numProcessesToLaunch; pid++)
        {
            ExpectGracefulExit(pid);
            DoNotExpectStdOutput(pid);
            ExpectLargeStdError(pid);
        }
    }

    INSTANTIATE_TEST_CASE_P(
        SmallConcurrentProcesses,
        ProcessSchedulerTestFixtureWithParams,
        ::testing::Combine(
            ::testing::ValuesIn(SmallNumMaxConcurrentProcesses),
            ::testing::ValuesIn(SmallNumProcessesToLaunch)
        ));

    INSTANTIATE_TEST_CASE_P(
        LargeConcurrentProcesses,
        ProcessSchedulerTestFixtureWithParams,
        ::testing::Combine(
            ::testing::ValuesIn(LargeNumMaxConcurrentProcesses),
            ::testing::ValuesIn(LargeNumProcessesToLaunch)
        ));
} // namespace UnitTest
