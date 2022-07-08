/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactRepoPath.h>
#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/TestImpactClientSequenceReport.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>

#pragma once

namespace TestImpact
{
    //! Attempts to read the contents of the specified file into a string.
    //! @tparam ExceptionType The exception type to throw upon failure.
    //! @param path The path to the file to read the contents of.
    //! @returns The contents of the file.
    template<typename ExceptionType>
    AZStd::string ReadFileContents(const RepoPath& path)
    {
        const auto fileSize = AZ::IO::SystemFile::Length(path.c_str());
        AZ_TestImpact_Eval(fileSize > 0, ExceptionType, AZStd::string::format("File %s does not exist", path.c_str()));

        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = '\0';
        AZ_TestImpact_Eval(
            AZ::IO::SystemFile::Read(path.c_str(), buffer.data()),
            ExceptionType,
            AZStd::string::format("Could not read contents of file %s", path.c_str()));

        return AZStd::string(buffer.begin(), buffer.end());
    }

    //! Attempts to write the contents of the specified string to a file.
    //! @tparam ExceptionType The exception type to throw upon failure.
    //! @param contents The contents to write to the file.
    //! @param path The path to the file to write the contents to.
    template<typename ExceptionType>
    void WriteFileContents(const AZStd::string& contents, const RepoPath& path)
    {
        AZ::IO::SystemFile file;
        const AZStd::vector<char> bytes(contents.begin(), contents.end());
        AZ_TestImpact_Eval(
            file.Open(path.c_str(),
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY),
            ExceptionType,
            AZStd::string::format("Couldn't open file %s for writing", path.c_str()));

        AZ_TestImpact_Eval(
            file.Write(bytes.data(), bytes.size()), ExceptionType, AZStd::string::format("Couldn't write contents for file %s", path.c_str()));
    }

    //! Delete the files that match the pattern from the specified directory.
    //! @param path The path to the directory to pattern match the files for deletion.
    //! @param pattern The pattern to match files for deletion.
    //! @return The number of files that were deleted.
    size_t DeleteFiles(const RepoPath& path, const AZStd::string& pattern);

    //! Deletes the specified file.
    void DeleteFile(const RepoPath& file);

    //! User-friendly names for the test suite types.
    AZStd::string SuiteTypeAsString(SuiteType suiteType);

    //! User-friendly names for the sequence report types.
    AZStd::string SequenceReportTypeAsString(Client::SequenceReportType type);

    //! User-friendly names for the sequence result types.
    AZStd::string TestSequenceResultAsString(TestSequenceResult result);

    //! User-friendly names for the test run result types.
    AZStd::string TestRunResultAsString(Client::TestRunResult result);

    //! User-friendly names for the execution failure policy types.
    AZStd::string ExecutionFailurePolicyAsString(Policy::ExecutionFailure executionFailurePolicy);

    //! User-friendly names for the failed test coverage policy types.
    AZStd::string FailedTestCoveragePolicyAsString(Policy::FailedTestCoverage failedTestCoveragePolicy);

    //! User-friendly names for the test prioritization policy types.
    AZStd::string TestPrioritizationPolicyAsString(Policy::TestPrioritization testPrioritizationPolicy);

    //! User-friendly names for the test failure policy types.
    AZStd::string TestFailurePolicyAsString(Policy::TestFailure testFailurePolicy);

    //! User-friendly names for the integrity failure policy types.
    AZStd::string IntegrityFailurePolicyAsString(Policy::IntegrityFailure integrityFailurePolicy);

    //! User-friendly names for the dynamic dependency map policy types.
    AZStd::string DynamicDependencyMapPolicyAsString(Policy::DynamicDependencyMap dynamicDependencyMapPolicy);

    //! User-friendly names for the test sharding policy types.
    AZStd::string TestShardingPolicyAsString(Policy::TestSharding testShardingPolicy);

    //! User-friendly names for the target output capture policy types.
    AZStd::string TargetOutputCapturePolicyAsString(Policy::TargetOutputCapture targetOutputCapturePolicy);

    //! User-friendly names for the client test result types.
    AZStd::string ClientTestResultAsString(Client::TestResult result);

    //! User-friendly names for the suite types.
    SuiteType SuiteTypeFromString(const AZStd::string& suiteType);

    //! Returns the sequence report type for the specified string.
    Client::SequenceReportType SequenceReportTypeFromString(const AZStd::string& type);

    //! Returns the test run result for the specified string.
    Client::TestRunResult TestRunResultFromString(const AZStd::string& result);

    //! Returns the test result for the specified string.
    Client::TestResult TestResultFromString(const AZStd::string& result);

    //! Returns the test sequence result for the specified string.
    TestSequenceResult TestSequenceResultFromString(const AZStd::string& result);

    //! Returns the execution failure policy for the specified string.
    Policy::ExecutionFailure ExecutionFailurePolicyFromString(const AZStd::string& executionFailurePolicy);

    //! Returns the failed test coverage policy for the specified string.
    Policy::FailedTestCoverage FailedTestCoveragePolicyFromString(const AZStd::string& failedTestCoveragePolicy);

    //! Returns the test prioritization policy for the specified string.
    Policy::TestPrioritization TestPrioritizationPolicyFromString(const AZStd::string& testPrioritizationPolicy);

    //! Returns the test failure policy for the specified string.
    Policy::TestFailure TestFailurePolicyFromString(const AZStd::string& testFailurePolicy);

    //! Returns the integrity failure policy for the specified string.
    Policy::IntegrityFailure IntegrityFailurePolicyFromString(const AZStd::string& integrityFailurePolicy);

    //! Returns the dynamic dependency map policy for the specified string.
    Policy::DynamicDependencyMap DynamicDependencyMapPolicyFromString(const AZStd::string& dynamicDependencyMapPolicy);

    //! Returns the test sharding policy for the specified string.
    Policy::TestSharding TestShardingPolicyFromString(const AZStd::string& testShardingPolicy);

    //! Returns the target output capture policy for the specified string.
    Policy::TargetOutputCapture TargetOutputCapturePolicyFromString(const AZStd::string& targetOutputCapturePolicy);
} // namespace TestImpact
