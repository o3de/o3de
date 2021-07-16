/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Run/TestImpactTestRunJobData.h>

namespace TestImpact
{
    TestRunJobData::TestRunJobData(const RepoPath& resultsArtifact)
        : m_runArtifact(resultsArtifact)
    {
    }

    const RepoPath& TestRunJobData::GetRunArtifactPath() const
    {
        return m_runArtifact;
    }
} // namespace TestImpact
