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
        : public TestEnumerationJobInfoGeneratorBase<NativeTestEnumerator, NativeTestTarget>
    {
    public:
        using TestEnumerationJobInfoGeneratorBase<NativeTestEnumerator, NativeTestTarget>::TestEnumerationJobInfoGeneratorBase;

    protected:
        // TestEnumerationJobInfoGenerator overrides...
        NativeTestEnumerator::JobInfo GenerateJobInfoImpl(
            const NativeTestTarget* testTarget, NativeTestEnumerator::JobInfo::Id jobId) const override;
    };

    //! Generates job information for the different test job runner types.
    class NativeRegularTestRunJobInfoGenerator
        : public TestJobInfoGeneratorBase<NativeRegularTestRunner, NativeTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param sourceDir Root path where source files are found (including subfolders).
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        NativeRegularTestRunJobInfoGenerator(
            const RepoPath& sourceDir, const RepoPath& targetBinaryDir, const ArtifactDir& artifactDir, const RepoPath& testRunnerBinary);

        // TestJobInfoGeneratorBase overrides...
        NativeRegularTestRunner::JobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget, NativeRegularTestRunner::JobInfo::Id jobId) const override;

    private:
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        ArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;
    };

    //! Generates job information for the different test job runner types.
    class NativeInstrumentedTestRunJobInfoGenerator
        : public TestJobInfoGeneratorBase<NativeInstrumentedTestRunner, NativeTestTarget>
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

        //! Sets the coverage level of the job info generator.
        //! @param cachePolicy The cache policy to use for job generation.
        void SetCoverageLevel(CoverageLevel coverageLevel);

        //! Returns the coverage level of the job info generator.
        CoverageLevel GetCoverageLevel() const;

        // TestJobInfoGeneratorBase overrides...
        NativeInstrumentedTestRunner::JobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget, NativeInstrumentedTestRunner::JobInfo::Id jobId) const override;

    private:
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        ArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;
        RepoPath m_instrumentBinary;
        CoverageLevel m_coverageLevel;
    };
}
