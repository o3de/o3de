/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>
#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/TestImpactClientSequenceReport.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Simple helper class for tracking basic timing information.
    class Timer
    {
    public:
        Timer();

        //! Returns the time point that the timer was instantiated.
        AZStd::chrono::steady_clock::time_point GetStartTimePoint() const;

        //! Returns the time point that the timer was instantiated relative to the specified starting time point.
        AZStd::chrono::steady_clock::time_point GetStartTimePointRelative(const Timer& start) const;

        //! Returns the time elapsed (in milliseconds) since the timer was instantiated.
        AZStd::chrono::milliseconds GetElapsedMs() const;

        //! Returns the current time point relative to the time point the timer was instantiated.
        AZStd::chrono::steady_clock::time_point GetElapsedTimepoint() const;

        //! Resets the timer start point to now.
        void ResetStartTimePoint();

    private:
        AZStd::chrono::steady_clock::time_point m_startTime;
    };

    //! Attempts to read the contents of the specified file into a string.
    //! @tparam ExceptionType The exception type to throw upon failure.
    //! @param path The path to the file to read the contents of.
    //! @returns The contents of the file.
    template<typename ExceptionType>
    [[nodiscard]] AZStd::string ReadFileContents(const RepoPath& path)
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

    //! Returns a string of the concatenated container contents separated by the specified separator.
    template<typename Container>
    AZStd::string ConcatenateContainerContentsAsString(const Container& container, const AZStd::string& separator)
    {
        AZStd::string concatenatedString;
        size_t i = 1;
        for (const auto& value : container)
        {
            concatenatedString += value;
            if (i != container.size())
            {
                concatenatedString += separator;
            }
            i++;
        }

        return concatenatedString;
    }

    //! Delete the files that match the pattern from the specified directory.
    //! @param path The path to the directory to pattern match the files for deletion.
    //! @param pattern The pattern to match files for deletion.
    //! @return The number of files that were deleted.
    size_t DeleteFiles(const RepoPath& path, const AZStd::string& pattern);

    //! Deletes the specified file.
    void DeleteFile(const RepoPath& file);

    //! Returns the list of files in the specified path that match the specified pattern.
    [[nodiscard]] AZStd::vector<AZStd::string> ListFiles(const RepoPath& path, const AZStd::string& pattern);

    //! Counts the number files that match the pattern from the specified directory.
    //! @param path The path to the directory to pattern match the files for deletion.
    //! @param pattern The pattern to match files for counting.
    [[nodiscard]] size_t FileCount(const RepoPath& path, const AZStd::string& pattern);

    //! User-friendly names for the test suite types.
    AZStd::string SuiteSetAsString(const SuiteSet& suiteSet);

    //! User-friendly names for the test suite label excludes.
    AZStd::string SuiteLabelExcludeSetAsString(const SuiteLabelExcludeSet& suiteLabelExcludeSet);

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

    //! User-friendly names for the target output capture policy types.
    AZStd::string TargetOutputCapturePolicyAsString(Policy::TargetOutputCapture targetOutputCapturePolicy);

    //! User-friendly names for the client test result types.
    AZStd::string ClientTestResultAsString(Client::TestResult result);
} // namespace TestImpact
