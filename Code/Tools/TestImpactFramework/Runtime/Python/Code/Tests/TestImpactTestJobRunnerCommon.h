/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactTestUtils.h>

#include <TestRunner/Enumeration/TestImpactTestEnumeration.h>
#include <TestRunner/Run/TestImpactTestRun.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    enum class JobExceptionPolicy
    {
        Never,
        OnFailedToExecute,
        OnExecutedWithFailure
    };

    // Named constants for array of targets lookup
    enum
    {
        TestTargetA,
        TestTargetB,
        TestTargetC,
        TestTargetD
    };

    // Named constants for max concurrency values
    enum
    {
        OneConcurrentProcess = 1,
        FourConcurrentProcesses = 4
    };

    // Validates that the specified job was executed and returned successfully but for jobs that produce no payload
    template<typename Job>
    void ValidateJobExecutedSuccessfullyNoPayload(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::ExecutedWithSuccess);
        EXPECT_TRUE(job.GetDuration() > AZStd::chrono::milliseconds(0));
        EXPECT_TRUE(job.GetReturnCode().has_value());
        EXPECT_EQ(job.GetReturnCode(), 0);
        EXPECT_FALSE(job.GetPayload().has_value());
    }

    // Validates that the specified job was executed and returned successfully
    template<typename Job>
    void ValidateJobExecutedSuccessfully(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::ExecutedWithSuccess);
        EXPECT_TRUE(job.GetDuration() > AZStd::chrono::milliseconds(0));
        EXPECT_TRUE(job.GetReturnCode().has_value());
        EXPECT_EQ(job.GetReturnCode(), 0);
        EXPECT_TRUE(job.GetPayload().has_value());
    }

    // Validates that the specified job has not been executed
    template<typename Job>
    void ValidateJobNotExecuted(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::NotExecuted);
        EXPECT_EQ(job.GetStartTime(), AZStd::chrono::high_resolution_clock::time_point());
        EXPECT_EQ(job.GetEndTime(), AZStd::chrono::high_resolution_clock::time_point());
        EXPECT_EQ(job.GetDuration(), AZStd::chrono::milliseconds(0));
        EXPECT_FALSE(job.GetReturnCode().has_value());
        EXPECT_FALSE(job.GetPayload().has_value());
    }

    // Validates that the specified job failed to execute
    template<typename Job>
    void ValidateJobFailedToExecute(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::FailedToExecute);
        EXPECT_EQ(job.GetStartTime(), AZStd::chrono::high_resolution_clock::time_point());
        EXPECT_EQ(job.GetEndTime(), AZStd::chrono::high_resolution_clock::time_point());
        EXPECT_EQ(job.GetDuration(), AZStd::chrono::milliseconds(0));
        EXPECT_FALSE(job.GetReturnCode().has_value());
        EXPECT_FALSE(job.GetPayload().has_value());
    }

    // Validates that the specified job executed but returned with error
    template<typename Job>
    void ValidateJobExecutedWithFailure(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::ExecutedWithFailure);
        EXPECT_TRUE(job.GetDuration() > AZStd::chrono::milliseconds(0));
        EXPECT_TRUE(job.GetReturnCode().has_value());
        EXPECT_GT(job.GetReturnCode().value(), 0);
        EXPECT_FALSE(job.GetPayload().has_value());
    }

    // Validates that the specified job was executed but was terminated by the job runner due to timing out
    template<typename Job>
    void ValidateJobTimeout(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::Timeout);
        EXPECT_TRUE(job.GetDuration() > AZStd::chrono::milliseconds(0));
        EXPECT_TRUE(job.GetReturnCode().has_value());
        EXPECT_EQ(job.GetReturnCode().value(), TestImpact::ProcessTimeoutErrorCode);
        EXPECT_FALSE(job.GetPayload().has_value());
    }

    // Validates that the specified job was executed but was terminated by the job runner due to another job cauising the sequence to enf prematurely
    template<typename Job>
    void ValidateJobTerminated(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::Terminated);
        EXPECT_TRUE(job.GetDuration() > AZStd::chrono::milliseconds(0));
        EXPECT_TRUE(job.GetReturnCode().has_value());
        EXPECT_EQ(job.GetReturnCode().value(), TestImpact::ProcessTerminateErrorCode);
        EXPECT_FALSE(job.GetPayload().has_value());
    }

    // Validates that the specified job executed but returned with error and no payload produced
    template<typename Job>
    void ValidateJobExecutedWithFailedTestsNoPayload(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::ExecutedWithFailure);
        EXPECT_TRUE(job.GetDuration() > AZStd::chrono::milliseconds(0));
        EXPECT_TRUE(job.GetReturnCode().has_value());
        EXPECT_GT(job.GetReturnCode().value(), 0);
        EXPECT_FALSE(job.GetPayload().has_value());
    }

    // Validates that the specified job executed but returned with error
    template<typename Job>
    void ValidateJobExecutedWithFailedTests(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::ExecutedWithFailure);
        EXPECT_TRUE(job.GetDuration() > AZStd::chrono::milliseconds(0));
        EXPECT_TRUE(job.GetReturnCode().has_value());
        EXPECT_GT(job.GetReturnCode().value(), 0);
        EXPECT_TRUE(job.GetPayload().has_value());
    }

    // Validates whether a test run completed (passed/failed)
    template<typename Job>
    void ValidateTestRunCompleted(const Job& job, TestImpact::TestRunResult result)
    {
        if (result == TestImpact::TestRunResult::Passed)
        {
            ValidateJobExecutedSuccessfully(job);
        }
        else
        {
            ValidateJobExecutedWithFailedTests(job);
        }
    }

    // Validates that the specified test run matches the expected output
    inline void ValidateTestTargetRun(const TestImpact::TestRun& actualResult, const TestImpact::TestRun& expectedResult)
    {
        EXPECT_TRUE(CheckTestRunsAreEqualIgnoreDurations(actualResult, expectedResult));
        EXPECT_EQ(actualResult.GetNumTestSuites(), CalculateNumTestSuites(expectedResult.GetTestSuites()));
        EXPECT_EQ(actualResult.GetNumTests(), CalculateNumTests(expectedResult.GetTestSuites()));
        EXPECT_EQ(actualResult.GetNumEnabledTests(), CalculateNumEnabledTests(expectedResult.GetTestSuites()));
        EXPECT_EQ(actualResult.GetNumDisabledTests(), CalculateNumDisabledTests(expectedResult.GetTestSuites()));
        EXPECT_GT(actualResult.GetDuration(), AZStd::chrono::milliseconds{0});
        EXPECT_EQ(actualResult.GetNumPasses(), CalculateNumPassedTests(expectedResult.GetTestSuites()));
        EXPECT_EQ(actualResult.GetNumFailures(), CalculateNumFailedTests(expectedResult.GetTestSuites()));
        EXPECT_EQ(actualResult.GetNumRuns(), CalculateNumRunTests(expectedResult.GetTestSuites()));
        EXPECT_EQ(actualResult.GetNumNotRuns(), CalculateNumNotRunTests(expectedResult.GetTestSuites()));
    }
} // namespace UnitTest
