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

    const RepoPath& TestRunWithCoverageJobData::GetCoverageArtifactPath() const
    {
        return m_coverageArtifact;
    }
} // namespace TestImpact
