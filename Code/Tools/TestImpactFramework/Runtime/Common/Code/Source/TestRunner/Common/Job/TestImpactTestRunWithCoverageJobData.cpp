/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Job/TestImpactTestRunWithCoverageJobData.h>

namespace TestImpact
{
    TestRunWithCoverageJobData::TestRunWithCoverageJobData(const RepoPath& resultsArtifact, const RepoPath& coverageArtifact)
        : TestRunJobData(resultsArtifact)
        , m_coverageArtifact(coverageArtifact)
    {
    }

    TestRunWithCoverageJobData::TestRunWithCoverageJobData(const TestRunWithCoverageJobData& other)
        : TestRunJobData(other)
        , m_coverageArtifact(other.m_coverageArtifact)
    {
    }

    TestRunWithCoverageJobData::TestRunWithCoverageJobData(TestRunWithCoverageJobData&& other)
        : TestRunJobData(AZStd::move(other))
        , m_coverageArtifact(AZStd::move(other.m_coverageArtifact))
    {
    }

    TestRunWithCoverageJobData& TestRunWithCoverageJobData::operator=(const TestRunWithCoverageJobData& other)
    {
        m_coverageArtifact = other.m_coverageArtifact;
        return *this;
    }

    TestRunWithCoverageJobData& TestRunWithCoverageJobData::operator=(TestRunWithCoverageJobData&& other)
    {
        m_coverageArtifact = AZStd::move(other.m_coverageArtifact);
        return *this;
    }


    const RepoPath& TestRunWithCoverageJobData::GetCoverageArtifactPath() const
    {
        return m_coverageArtifact;
    }
} // namespace TestImpact
