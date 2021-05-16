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

#include <Artifact/Dynamic/TestImpactCoverage.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerator.h>
#include <TestEngine/Run/TestImpactInstrumentedTestRunner.h>
#include <TestEngine/Run/TestImpactTestRunner.h>

#include <AzCore/IO/Path/Path.h>

namespace TestImpact
{
    class TestTarget;

    class TestJobInfoGenerator
    {
    public:
        TestJobInfoGenerator(
            const AZ::IO::Path& sourceDir,
            const AZ::IO::Path& targetBinaryDir,
            const AZ::IO::Path& cacheDir,
            const AZ::IO::Path& artifactDir,
            const AZ::IO::Path& testRunnerBinary,
            const AZ::IO::Path& instrumentBinary);

        const AZ::IO::Path& GetCacheDir() const;
        const AZ::IO::Path& GetArtifactDir() const;

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
    private:
        AZStd::string GenerateLaunchArgument(const TestTarget* testTarget) const;
        AZ::IO::Path GenerateTargetEnumerationCacheFilePath(const TestTarget* testTarget) const;
        AZ::IO::Path GenerateTargetEnumerationArtifactFilePath(const TestTarget* testTarget) const;
        AZ::IO::Path GenerateTargetRunArtifactFilePath(const TestTarget* testTarget) const;
        AZ::IO::Path GenerateTargetCoverageArtifactFilePath(const TestTarget* testTarget) const;
        
        AZ::IO::Path m_sourceDir;
        AZ::IO::Path m_targetBinaryDir;
        AZ::IO::Path m_cacheDir;
        AZ::IO::Path m_artifactDir;
        AZ::IO::Path m_testRunnerBinary;
        AZ::IO::Path m_instrumentBinary;
    };
}
