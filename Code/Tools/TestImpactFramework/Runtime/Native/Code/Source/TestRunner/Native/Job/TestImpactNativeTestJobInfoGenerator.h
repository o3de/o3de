/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestRunner/Common/Job/TestImpactTestJobInfoGenerator.h>
#include <TestRunner/Native/TestImpactNativeTestEnumerator.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>

namespace TestImpact
{
    //! Generates job information for the different test job runner types.
    class NativeTestEnumerationJobInfoGenerator
        : public TestJobInfoGenerator<NativeTestEnumerator, NativeTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param cacheDir Path to the persistent folder where test target enumerations are cached.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        NativeTestEnumerationJobInfoGenerator(
            const RepoPath& targetBinaryDir,
            const RepoPath& cacheDir,
            const ArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary);

        //! Generates the information for a test enumeration job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        NativeTestEnumerator::JobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget,
            NativeTestEnumerator::JobInfo::Id jobId) const override;

        //!
        //! @param cachePolicy The cache policy to use for job generation.
        void SetCachePolicy(NativeTestEnumerator::JobInfo::CachePolicy cachePolicy);

        //!
        NativeTestEnumerator::JobInfo::CachePolicy GetCachePolicy() const;

    private:
        RepoPath m_targetBinaryDir;
        RepoPath m_cacheDir;
        ArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;

        NativeTestEnumerator::JobInfo::CachePolicy m_cachePolicy;
    };

    //! Generates job information for the different test job runner types.
    class NativeRegularTestRunJobInfoGenerator
        : public TestJobInfoGenerator<NativeRegularTestRunner, NativeTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param sourceDir Root path where source files are found (including subfolders).
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        NativeRegularTestRunJobInfoGenerator(
            const RepoPath& sourceDir, const RepoPath& targetBinaryDir, const ArtifactDir& artifactDir, const RepoPath& testRunnerBinary);

        //! Generates the information for a test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        NativeRegularTestRunner::JobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget, NativeRegularTestRunner::JobInfo::Id jobId) const;

    private:
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        ArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;
    };

    //! Generates job information for the different test job runner types.
    class NativeInstrumentedTestRunJobInfoGenerator
        : public TestJobInfoGenerator<NativeInstrumentedTestRunner, NativeTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param sourceDir Root path where source files are found (including subfolders).
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        //! @param instrumentBinary Path to the binary responsible for launching test targets with test coverage instrumentation.
        NativeInstrumentedTestRunJobInfoGenerator(
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const ArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary,
            CoverageLevel coverageLevel = CoverageLevel::Source);

        //! Generates the information for a test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        NativeInstrumentedTestRunner::JobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget, NativeInstrumentedTestRunner::JobInfo::Id jobId) const;

        //!
        //! @param cachePolicy The cache policy to use for job generation.
        void SetCoverageLevel(CoverageLevel coverageLevel);

        //!
        CoverageLevel GetCoverageLevel() const;

    private:
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        RepoPath m_cacheDir;
        ArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;
        RepoPath m_instrumentBinary;
        CoverageLevel m_coverageLevel;
    };
}
