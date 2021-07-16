/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRuntime.h>

#include <Artifact/Dynamic/TestImpactCoverage.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerator.h>
#include <TestEngine/Run/TestImpactInstrumentedTestRunner.h>
#include <TestEngine/Run/TestImpactTestRunner.h>

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    class TestTarget;

    //! Generates job information for the different test job runner types.
    class TestJobInfoGenerator
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param sourceDir Root path where source files are found (including subfolders).
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param cacheDir Path to the persistent folder where test target enumerations are cached.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        //! @param instrumentBinary Path to the binary responsible for launching test targets with test coverage instrumentation.
        TestJobInfoGenerator(
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
        TestEnumerator::JobInfo GenerateTestEnumerationJobInfo(
            const TestTarget* testTarget,
            TestEnumerator::JobInfo::Id jobId,
            TestEnumerator::JobInfo::CachePolicy cachePolicy) const;

        //! Generates the information for a test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        TestRunner::JobInfo GenerateRegularTestRunJobInfo(
            const TestTarget* testTarget,
            TestRunner::JobInfo::Id jobId) const;

        //! Generates the information for an instrumented test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        //! @param coverageLevel The coverage level to use for this job.
        InstrumentedTestRunner::JobInfo GenerateInstrumentedTestRunJobInfo(
            const TestTarget* testTarget,
            InstrumentedTestRunner::JobInfo::Id jobId,
            CoverageLevel coverageLevel) const;

        //! Generates the information for the batch of test enumeration jobs.
        AZStd::vector<TestEnumerator::JobInfo> GenerateTestEnumerationJobInfos(
            const AZStd::vector<const TestTarget*>& testTargets,
            TestEnumerator::JobInfo::CachePolicy cachePolicy) const;

        //! Generates the information for the batch of test run jobs.
        AZStd::vector<TestRunner::JobInfo> GenerateRegularTestRunJobInfos(
            const AZStd::vector<const TestTarget*>& testTargets) const;

        //! Generates the information for the batch of instrumented test run jobs.
        AZStd::vector<InstrumentedTestRunner::JobInfo> GenerateInstrumentedTestRunJobInfos(
            const AZStd::vector<const TestTarget*>& testTargets,
            CoverageLevel coverageLevel) const;
    private:
        //! Generates the command string to launch the specified test target.
        AZStd::string GenerateLaunchArgument(const TestTarget* testTarget) const;

        //! Generates the path to the enumeration cache file for the specified test target.
        RepoPath GenerateTargetEnumerationCacheFilePath(const TestTarget* testTarget) const;

        //! Generates the path to the enumeration artifact file for the specified test target.
        RepoPath GenerateTargetEnumerationArtifactFilePath(const TestTarget* testTarget) const;

        //! Generates the path to the test run artifact file for the specified test target.
        RepoPath GenerateTargetRunArtifactFilePath(const TestTarget* testTarget) const;

        //! Generates the path to the test coverage artifact file for the specified test target.
        RepoPath GenerateTargetCoverageArtifactFilePath(const TestTarget* testTarget) const;
        
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        RepoPath m_cacheDir;
        RepoPath m_artifactDir;
        RepoPath m_testRunnerBinary;
        RepoPath m_instrumentBinary;
    };
}
