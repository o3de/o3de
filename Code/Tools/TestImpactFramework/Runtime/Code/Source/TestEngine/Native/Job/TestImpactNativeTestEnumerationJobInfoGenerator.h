/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/Common/Job/TestImpactTestJobInfoGenerator.h>
#include <TestRunner/Native/TestImpactNativeTestEnumerator.h>

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
            const RepoPath& artifactDir,
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
        RepoPath m_artifactDir;
        RepoPath m_testRunnerBinary;

        NativeTestEnumerator::JobInfo::CachePolicy m_cachePolicy;
    };
}
