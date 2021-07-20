/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactException.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <AzCore/std/functional.h>

namespace TestImpact
{
    //! Delete the files that match the pattern from the specified directory.
    //! @param path The path to the directory to pattern match the files for deletion.
    //! @param pattern The pattern to match files for deletion.
    void DeleteFiles(const RepoPath& path, const AZStd::string& pattern)
    {
        AZ::IO::SystemFile::FindFiles(
            AZStd::string::format("%s/%s", path.c_str(), pattern.c_str()).c_str(),
            [&path](const char* file, bool isFile)
            {
                if (isFile)
                {
                    AZ::IO::SystemFile::Delete(AZStd::string::format("%s/%s", path.c_str(), file).c_str());
                }

                return true;
            });
    }

    //! Deletes the specified file.
    void DeleteFile(const RepoPath& file)
    {
        DeleteFiles(file.ParentPath(), file.Filename().Native());
    }

    //! User-friendly names for the test suite types.
    AZStd::string SuiteTypeAsString(SuiteType suiteType)
    {
        switch (suiteType)
        {
        case SuiteType::Main:
            return "main";
        case SuiteType::Periodic:
            return "periodic";
        case SuiteType::Sandbox:
            return "sandbox";
        default:
            throw(Exception("Unexpected suite type"));
        }
    }

    AZStd::string SequenceReportTypeAsString(Client::SequenceReportType type)
    {
        switch (type)
        {
        case Client::SequenceReportType::Sequence:
            return "sequence";

        case Client::SequenceReportType::ImpactAnalysisSequence:
            return "impact_analysis_sequence";

        case Client::SequenceReportType::SafeImpactAnalysisSequence:
            return "safe_impact_analysis_sequence";

        default:
            throw(Exception(AZStd::string::format("Unexpected sequence report type: %u", aznumeric_cast<AZ::u32>(type))));
        }
    }

    AZStd::string TestSequenceResultAsString(TestSequenceResult result)
    {
        switch (result)
        {
        case TestSequenceResult::Failure:
            return "failure";

        case TestSequenceResult::Success:
            return "success";

        case TestSequenceResult::Timeout:
            return "timeout";

        default:
            throw(Exception(AZStd::string::format("Unexpected test sequence result: %u", aznumeric_cast<AZ::u32>(result))));
        }
    }

    AZStd::string TestRunResultAsString(Client::TestRunResult result)
    {
        switch (result)
        {
        case Client::TestRunResult::AllTestsPass:
            return "all_tests_pass";

        case Client::TestRunResult::FailedToExecute:
            return "failed_to_execute";

        case Client::TestRunResult::NotRun:
            return "not_run";

        case Client::TestRunResult::TestFailures:
            return "test_failures";

        case Client::TestRunResult::Timeout:
            return "timeout";

        default:
            throw(Exception(AZStd::string::format("Unexpected test run result: %u", aznumeric_cast<AZ::u32>(result))));
        }
    }

    AZStd::string ExecutionFailurePolicyAsString(Policy::ExecutionFailure executionFailurePolicy)
    {
        switch (executionFailurePolicy)
        {
        case Policy::ExecutionFailure::Abort:
            return "abort";

        case Policy::ExecutionFailure::Continue:
            return "continue";

        case Policy::ExecutionFailure::Ignore:
            return "ignore";

        default:
            throw(Exception(
                AZStd::string::format("Unexpected execution failure policy: %u", aznumeric_cast<AZ::u32>(executionFailurePolicy))));
        }
    }

    AZStd::string FailedTestCoveragePolicyAsString(Policy::FailedTestCoverage failedTestCoveragePolicy)
    {
        switch (failedTestCoveragePolicy)
        {
        case Policy::FailedTestCoverage::Discard:
            return "discard";

        case Policy::FailedTestCoverage::Keep:
            return "keep";

        default:
            throw(Exception(
                AZStd::string::format("Unexpected failed test coverage policy: %u", aznumeric_cast<AZ::u32>(failedTestCoveragePolicy))));
        }
    }

    AZStd::string TestPrioritizationPolicyAsString(Policy::TestPrioritization testPrioritizationPolicy)
    {
        switch (testPrioritizationPolicy)
        {
        case Policy::TestPrioritization::DependencyLocality:
            return "dependency_locality";

        case Policy::TestPrioritization::None:
            return "none";

        default:
            throw(Exception(
                AZStd::string::format("Unexpected test prioritization policy: %u", aznumeric_cast<AZ::u32>(testPrioritizationPolicy))));
        }
    }

    AZStd::string TestFailurePolicyAsString(Policy::TestFailure testFailurePolicy)
    {
        switch (testFailurePolicy)
        {
        case Policy::TestFailure::Abort:
            return "abort";

        case Policy::TestFailure::Continue:
            return "continue";

        default:
            throw(
                Exception(AZStd::string::format("Unexpected test failure policy: %u", aznumeric_cast<AZ::u32>(testFailurePolicy))));
        }
    }

    AZStd::string IntegrityFailurePolicyAsString(Policy::IntegrityFailure integrityFailurePolicy)
    {
        switch (integrityFailurePolicy)
        {
        case Policy::IntegrityFailure::Abort:
            return "abort";

        case Policy::IntegrityFailure::Continue:
            return "continue";

        default:
            throw(Exception(
                AZStd::string::format("Unexpected integration failure policy: %u", aznumeric_cast<AZ::u32>(integrityFailurePolicy))));
        }
    }

    AZStd::string DynamicDependencyMapPolicyAsString(Policy::DynamicDependencyMap dynamicDependencyMapPolicy)
    {
        switch (dynamicDependencyMapPolicy)
        {
        case Policy::DynamicDependencyMap::Discard:
            return "discard";

        case Policy::DynamicDependencyMap::Update:
            return "update";

        default:
            throw(Exception(AZStd::string::format(
                "Unexpected dynamic dependency map policy: %u", aznumeric_cast<AZ::u32>(dynamicDependencyMapPolicy))));
        }
    }

    AZStd::string TestShardingPolicyAsString(Policy::TestSharding testShardingPolicy)
    {
        switch (testShardingPolicy)
        {
        case Policy::TestSharding::Always:
            return "always";

        case Policy::TestSharding::Never:
            return "never";

        default:
            throw(Exception(
                AZStd::string::format("Unexpected test sharding policy: %u", aznumeric_cast<AZ::u32>(testShardingPolicy))));
        }
    }

    AZStd::string TargetOutputCapturePolicyAsString(Policy::TargetOutputCapture targetOutputCapturePolicy)
    {
        switch (targetOutputCapturePolicy)
        {
        case Policy::TargetOutputCapture::File:
            return "file";

        case Policy::TargetOutputCapture::None:
            return "none";

        case Policy::TargetOutputCapture::StdOut:
            return "stdout";

        case Policy::TargetOutputCapture::StdOutAndFile:
            return "stdout_file";

        default:
            throw(Exception(
                AZStd::string::format("Unexpected target output capture policy: %u", aznumeric_cast<AZ::u32>(targetOutputCapturePolicy))));
        }
    }

    AZStd::string ClientTestCaseResultAsString(Client::TestCaseResult result)
    {
        switch (result)
        {
        case Client::TestCaseResult::Failed:
            return "failed";

        case Client::TestCaseResult::NotRun:
            return "not_run";

        case Client::TestCaseResult::Passed:
            return "passed";

        default:
            throw(Exception(AZStd::string::format("Unexpected client test case result: %u", aznumeric_cast<AZ::u32>(result))));
        }
    }
} // namespace TestImpact
