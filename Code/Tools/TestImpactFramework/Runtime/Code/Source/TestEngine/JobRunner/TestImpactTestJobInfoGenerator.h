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

#include <TestImpactFramework/TestImpactRuntime.h>

#include <Artifact/Dynamic/TestImpactCoverage.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerator.h>
#include <TestEngine/Run/TestImpactInstrumentedTestRunner.h>
#include <TestEngine/Run/TestImpactTestRunner.h>

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    class TestTarget;

    class TestJobInfoGenerator
    {
    public:
        TestJobInfoGenerator(
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const RepoPath& cacheDir,
            const RepoPath& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary);

        const RepoPath& GetCacheDir() const;
        const RepoPath& GetArtifactDir() const;

        TestEnumerator::JobInfo GenerateTestEnumerationJobInfo(
            const TestTarget* testTarget,
            TestEnumerator::JobInfo::Id jobId,
            TestEnumerator::JobInfo::CachePolicy cachePolicy) const;

        TestRunner::JobInfo GenerateRegularTestRunJobInfo(
            const TestTarget* testTarget,
            TestRunner::JobInfo::Id jobId) const;

        InstrumentedTestRunner::JobInfo GenerateInstrumentedTestRunJobInfo(
            const TestTarget* testTarget,
            InstrumentedTestRunner::JobInfo::Id jobId,
            CoverageLevel coverageLevel) const;

        AZStd::vector<TestEnumerator::JobInfo> GenerateTestEnumerationJobInfos(
            const AZStd::vector<const TestTarget*>& testTargets,
            TestEnumerator::JobInfo::CachePolicy cachePolicy) const;

        AZStd::vector<TestRunner::JobInfo> GenerateRegularTestRunJobInfos(
            const AZStd::vector<const TestTarget*>& testTargets) const;

        AZStd::vector<InstrumentedTestRunner::JobInfo> GenerateInstrumentedTestRunJobInfos(
            const AZStd::vector<const TestTarget*>& testTargets,
            CoverageLevel coverageLevel) const;
    private:
        AZStd::string GenerateLaunchArgument(const TestTarget* testTarget) const;
        RepoPath GenerateTargetEnumerationCacheFilePath(const TestTarget* testTarget) const;
        RepoPath GenerateTargetEnumerationArtifactFilePath(const TestTarget* testTarget) const;
        RepoPath GenerateTargetRunArtifactFilePath(const TestTarget* testTarget) const;
        RepoPath GenerateTargetCoverageArtifactFilePath(const TestTarget* testTarget) const;
        
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        RepoPath m_cacheDir;
        RepoPath m_artifactDir;
        RepoPath m_testRunnerBinary;
        RepoPath m_instrumentBinary;
    };
}
