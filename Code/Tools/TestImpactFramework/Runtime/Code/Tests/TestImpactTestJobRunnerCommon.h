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

#pragma once

#include <TestImpactTestUtils.h>

#include <TestEngine/Enumeration/TestImpactTestEnumeration.h>
#include <TestEngine/Run/TestImpactTestRun.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

template<typename Flags>
constexpr bool IsFlagSet(Flags flags, Flags flag)
{
    return TestImpact::Bitwise::IsFlagSet(flags, flag);
}

namespace UnitTest
{
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

    // Validates that the specified job was executed but was terminated by the job runner
    template<typename Job>
    void ValidateJobTimeout(const Job& job)
    {
        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::Timeout);
        EXPECT_TRUE(job.GetDuration() > AZStd::chrono::milliseconds(0));
        EXPECT_TRUE(job.GetReturnCode().has_value());
        EXPECT_EQ(job.GetReturnCode().value(), TestImpact::ProcessTimeoutErrorCode);
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

    // Delete any existing data in the test run folder as not to pollute tests with data from previous test runs
    // Note: the file IO operations of this fixture means it cannot be sharded by the test sharder due to file race conditions
    inline void DeleteFiles(const AZStd::string& path, const AZStd::string& pattern)
    {
        AZ::IO::SystemFile::FindFiles(AZStd::string::format("%s/%s", path.c_str(), pattern.c_str()).c_str(), [&path](const char* file, bool isFile)
        {
            if (isFile)
            {
                AZ::IO::SystemFile::Delete(AZStd::string::format("%s/%s", path.c_str(), file).c_str());
            }

            return true;
        });
    }
} // namespace UnitTest
