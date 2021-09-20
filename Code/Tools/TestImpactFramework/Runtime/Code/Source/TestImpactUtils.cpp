/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    size_t DeleteFiles(const RepoPath& path, const AZStd::string& pattern)
    {
        size_t numFilesDeleted = 0;

        AZ::IO::SystemFile::FindFiles(
            AZStd::string::format("%s/%s", path.c_str(), pattern.c_str()).c_str(),
            [&path, &numFilesDeleted](const char* file, bool isFile)
            {
                if (isFile)
                {
                    AZ::IO::SystemFile::Delete(AZStd::string::format("%s/%s", path.c_str(), file).c_str());
                    numFilesDeleted++;
                }

                return true;
            });

        return numFilesDeleted;
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
        case SuiteType::AWSI:
            return "awsi";
        default:
            throw(Exception("Unexpected suite type"));
        }
    }

    AZStd::string SequenceReportTypeAsString(Client::SequenceReportType type)
    {
        switch (type)
        {
        case Client::SequenceReportType::RegularSequence:
            return "regular";
        case Client::SequenceReportType::SeedSequence:
            return "seed";
        case Client::SequenceReportType::ImpactAnalysisSequence:
            return "impact_analysis";
        case Client::SequenceReportType::SafeImpactAnalysisSequence:
            return "safe_impact_analysis";
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

    AZStd::string ClientTestResultAsString(Client::TestResult result)
    {
        switch (result)
        {
        case Client::TestResult::Failed:
            return "failed";
        case Client::TestResult::NotRun:
            return "not_run";
        case Client::TestResult::Passed:
            return "passed";
        default:
            throw(Exception(AZStd::string::format("Unexpected client test case result: %u", aznumeric_cast<AZ::u32>(result))));
        }
    }

    SuiteType SuiteTypeFromString(const AZStd::string& suiteType)
    {
        if (suiteType == SuiteTypeAsString(SuiteType::Main))
        {
            return SuiteType::Main;
        }
        else if (suiteType == SuiteTypeAsString(SuiteType::Periodic))
        {
            return SuiteType::Periodic;
        }
        else if (suiteType == SuiteTypeAsString(SuiteType::Sandbox))
        {
            return SuiteType::Sandbox;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected suite type: '%s'", suiteType.c_str()));
        }
    }

    Client::SequenceReportType SequenceReportTypeFromString(const AZStd::string& type)
    {
        if (type == SequenceReportTypeAsString(Client::SequenceReportType::ImpactAnalysisSequence))
        {
            return Client::SequenceReportType::ImpactAnalysisSequence;
        }
        else if (type == SequenceReportTypeAsString(Client::SequenceReportType::RegularSequence))
        {
            return Client::SequenceReportType::RegularSequence;
        }
        else if (type == SequenceReportTypeAsString(Client::SequenceReportType::SafeImpactAnalysisSequence))
        {
            return Client::SequenceReportType::SafeImpactAnalysisSequence;
        }
        else if (type == SequenceReportTypeAsString(Client::SequenceReportType::SeedSequence))
        {
            return Client::SequenceReportType::SeedSequence;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected sequence report type: '%s'", type.c_str()));
        }
    }

    Client::TestRunResult TestRunResultFromString(const AZStd::string& result)
    {
        if (result == TestRunResultAsString(Client::TestRunResult::AllTestsPass))
        {
            return Client::TestRunResult::AllTestsPass;
        }
        else if (result == TestRunResultAsString(Client::TestRunResult::FailedToExecute))
        {
            return Client::TestRunResult::FailedToExecute;
        }
        else if (result == TestRunResultAsString(Client::TestRunResult::NotRun))
        {
            return Client::TestRunResult::NotRun;
        }
        else if (result == TestRunResultAsString(Client::TestRunResult::TestFailures))
        {
            return Client::TestRunResult::TestFailures;
        }
        else if (result == TestRunResultAsString(Client::TestRunResult::Timeout))
        {
            return Client::TestRunResult::Timeout;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected client test run result: '%s'", result.c_str()));
        }
    }

    Client::TestResult TestResultFromString(const AZStd::string& result)
    {
        if (result == ClientTestResultAsString(Client::TestResult::Failed))
        {
            return Client::TestResult::Failed;
        }
        else if (result == ClientTestResultAsString(Client::TestResult::NotRun))
        {
            return Client::TestResult::NotRun;
        }
        else if (result == ClientTestResultAsString(Client::TestResult::Passed))
        {
            return Client::TestResult::Passed;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected client test result: '%s'", result.c_str()));
        }
    }

    TestSequenceResult TestSequenceResultFromString(const AZStd::string& result)
    {
        if (result == TestSequenceResultAsString(TestSequenceResult::Failure))
        {
            return TestSequenceResult::Failure;
        }
        else if (result == TestSequenceResultAsString(TestSequenceResult::Success))
        {
            return TestSequenceResult::Success;
        }
        else if (result == TestSequenceResultAsString(TestSequenceResult::Timeout))
        {
            return TestSequenceResult::Timeout;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected test sequence result: '%s'", result.c_str()));
        }
    }

    Policy::ExecutionFailure ExecutionFailurePolicyFromString(const AZStd::string& executionFailurePolicy)
    {
        if (executionFailurePolicy == ExecutionFailurePolicyAsString(Policy::ExecutionFailure::Abort))
        {
            return Policy::ExecutionFailure::Abort;
        }
        else if (executionFailurePolicy == ExecutionFailurePolicyAsString(Policy::ExecutionFailure::Continue))
        {
            return Policy::ExecutionFailure::Continue;
        }
        else if (executionFailurePolicy == ExecutionFailurePolicyAsString(Policy::ExecutionFailure::Ignore))
        {
            return Policy::ExecutionFailure::Ignore;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected execution failure policy: '%s'", executionFailurePolicy.c_str()));
        }
    }

    Policy::FailedTestCoverage FailedTestCoveragePolicyFromString(const AZStd::string& failedTestCoveragePolicy)
    {
        if (failedTestCoveragePolicy == FailedTestCoveragePolicyAsString(Policy::FailedTestCoverage::Discard))
        {
            return Policy::FailedTestCoverage::Discard;
        }
        else if (failedTestCoveragePolicy == FailedTestCoveragePolicyAsString(Policy::FailedTestCoverage::Keep))
        {
            return Policy::FailedTestCoverage::Keep;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected failed test coverage policy: '%s'", failedTestCoveragePolicy.c_str()));
        }
    }

    Policy::TestPrioritization TestPrioritizationPolicyFromString(const AZStd::string& testPrioritizationPolicy)
    {
        if (testPrioritizationPolicy == TestPrioritizationPolicyAsString(Policy::TestPrioritization::DependencyLocality))
        {
            return Policy::TestPrioritization::DependencyLocality;
        }
        else if (testPrioritizationPolicy == TestPrioritizationPolicyAsString(Policy::TestPrioritization::None))
        {
            return Policy::TestPrioritization::None;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected test prioritization policy: '%s'", testPrioritizationPolicy.c_str()));
        }
    }

    Policy::TestFailure TestFailurePolicyFromString(const AZStd::string& testFailurePolicy)
    {
        if (testFailurePolicy == TestFailurePolicyAsString(Policy::TestFailure::Abort))
        {
            return Policy::TestFailure::Abort;
        }
        else if (testFailurePolicy == TestFailurePolicyAsString(Policy::TestFailure::Continue))
        {
            return Policy::TestFailure::Continue;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected test failure policy: '%s'", testFailurePolicy.c_str()));
        }
    }

    Policy::IntegrityFailure IntegrityFailurePolicyFromString(const AZStd::string& integrityFailurePolicy)
    {
        if (integrityFailurePolicy == IntegrityFailurePolicyAsString(Policy::IntegrityFailure::Abort))
        {
            return Policy::IntegrityFailure::Abort;
        }
        else if (integrityFailurePolicy == IntegrityFailurePolicyAsString(Policy::IntegrityFailure::Continue))
        {
            return Policy::IntegrityFailure::Continue;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected integration failure policy: '%s'", integrityFailurePolicy.c_str()));
        }
    }

    Policy::DynamicDependencyMap DynamicDependencyMapPolicyFromString(const AZStd::string& dynamicDependencyMapPolicy)
    {
        if (dynamicDependencyMapPolicy == DynamicDependencyMapPolicyAsString(Policy::DynamicDependencyMap::Discard))
        {
            return Policy::DynamicDependencyMap::Discard;
        }
        else if (dynamicDependencyMapPolicy == DynamicDependencyMapPolicyAsString(Policy::DynamicDependencyMap::Update))
        {
            return Policy::DynamicDependencyMap::Update;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected dynamic dependency map policy: '%s'", dynamicDependencyMapPolicy.c_str()));
        }
    }

    Policy::TestSharding TestShardingPolicyFromString(const AZStd::string& testShardingPolicy)
    {
        if (testShardingPolicy == TestShardingPolicyAsString(Policy::TestSharding::Always))
        {
            return Policy::TestSharding::Always;
        }
        else if (testShardingPolicy == TestShardingPolicyAsString(Policy::TestSharding::Never))
        {
            return Policy::TestSharding::Never;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected test sharding policy: '%s'", testShardingPolicy.c_str()));
        }
    }

    Policy::TargetOutputCapture TargetOutputCapturePolicyFromString(const AZStd::string& targetOutputCapturePolicy)
    {
        if (targetOutputCapturePolicy == TargetOutputCapturePolicyAsString(Policy::TargetOutputCapture::File))
        {
            return Policy::TargetOutputCapture::File;
        }
        else if (targetOutputCapturePolicy == TargetOutputCapturePolicyAsString(Policy::TargetOutputCapture::None))
        {
            return Policy::TargetOutputCapture::None;
        }
        else if (targetOutputCapturePolicy == TargetOutputCapturePolicyAsString(Policy::TargetOutputCapture::StdOut))
        {
            return Policy::TargetOutputCapture::StdOut;
        }
        else if (targetOutputCapturePolicy == TargetOutputCapturePolicyAsString(Policy::TargetOutputCapture::StdOutAndFile))
        {
            return Policy::TargetOutputCapture::StdOutAndFile;
        }
        else
        {
            throw Exception(AZStd::string::format("Unexpected target output capture policy: '%s'", targetOutputCapturePolicy.c_str()));
        }
    }
} // namespace TestImpact
