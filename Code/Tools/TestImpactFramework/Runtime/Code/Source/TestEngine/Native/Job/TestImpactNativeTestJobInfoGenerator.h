/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Dynamic/TestImpactCoverage.h>
#include <TestRunner/Native/TestImpactNativeTestEnumerator.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    class NativeTestTarget;

    //! Generates job information for the different test job runner types.
    class NativeTestJobInfoGenerator
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param sourceDir Root path where source files are found (including subfolders).
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param cacheDir Path to the persistent folder where test target enumerations are cached.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        //! @param instrumentBinary Path to the binary responsible for launching test targets with test coverage instrumentation.
        NativeTestJobInfoGenerator(
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const RepoPath& cacheDir,
            const RepoPath& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary);

        //! Generates the information for a test enumeration job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        //! @param cachePolicy The cache policy to use for this job.
        NativeTestEnumerator::JobInfo GenerateTestEnumerationJobInfo(
            const NativeTestTarget* testTarget,
            NativeTestEnumerator::JobInfo::Id jobId,
            NativeTestEnumerator::JobInfo::CachePolicy cachePolicy) const;

        //! Generates the information for a test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        NativeRegularTestRunner::JobInfo GenerateRegularTestRunJobInfo(
            const NativeTestTarget* testTarget, NativeRegularTestRunner::JobInfo::Id jobId) const;

        //! Generates the information for an instrumented test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        //! @param coverageLevel The coverage level to use for this job.
        NativeInstrumentedTestRunner::JobInfo GenerateInstrumentedTestRunJobInfo(
            const NativeTestTarget* testTarget, NativeInstrumentedTestRunner::JobInfo::Id jobId, CoverageLevel coverageLevel) const;

        //! Generates the information for the batch of test enumeration jobs.
        AZStd::vector<NativeTestEnumerator::JobInfo> GenerateTestEnumerationJobInfos(
            const AZStd::vector<const NativeTestTarget*>& testTargets, NativeTestEnumerator::JobInfo::CachePolicy cachePolicy) const;

        //! Generates the information for the batch of test run jobs.
        AZStd::vector<NativeRegularTestRunner::JobInfo> GenerateRegularTestRunJobInfos(
            const AZStd::vector<const NativeTestTarget*>& testTargets) const;

        //! Generates the information for the batch of instrumented test run jobs.
        AZStd::vector<NativeInstrumentedTestRunner::JobInfo> GenerateInstrumentedTestRunJobInfos(
            const AZStd::vector<const NativeTestTarget*>& testTargets, CoverageLevel coverageLevel) const;

    private:
        //! Generates the command string to launch the specified test target.
        AZStd::string GenerateLaunchArgument(const NativeTestTarget* testTarget) const;

        //! Generates the path to the enumeration cache file for the specified test target.
        RepoPath GenerateTargetEnumerationCacheFilePath(const NativeTestTarget* testTarget) const;

        //! Generates the path to the enumeration artifact file for the specified test target.
        RepoPath GenerateTargetEnumerationArtifactFilePath(const NativeTestTarget* testTarget) const;

        //! Generates the path to the test run artifact file for the specified test target.
        RepoPath GenerateTargetRunArtifactFilePath(const NativeTestTarget* testTarget) const;

        //! Generates the path to the test coverage artifact file for the specified test target.
        RepoPath GenerateTargetCoverageArtifactFilePath(const NativeTestTarget* testTarget) const;

        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        RepoPath m_cacheDir;
        RepoPath m_artifactDir;
        RepoPath m_testRunnerBinary;
        RepoPath m_instrumentBinary;
    };
}
