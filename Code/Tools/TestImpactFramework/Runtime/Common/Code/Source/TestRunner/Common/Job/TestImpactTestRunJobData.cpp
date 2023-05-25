/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Job/TestImpactTestRunJobData.h>

namespace TestImpact
{
    TestRunJobData::TestRunJobData(const RepoPath& resultsArtifact)
        : m_runArtifact(resultsArtifact)
    {
    }

    TestRunJobData::TestRunJobData(const TestRunJobData& other)
        : m_runArtifact(other.m_runArtifact)
    {
    }

    TestRunJobData::TestRunJobData(TestRunJobData&& other)
        : m_runArtifact(AZStd::move(other.m_runArtifact))
    {
    }

    TestRunJobData& TestRunJobData::operator=(const TestRunJobData& other)
    {
        m_runArtifact = other.m_runArtifact;
        return *this;
    }

    TestRunJobData& TestRunJobData::operator=(TestRunJobData&& other)
    {
        m_runArtifact = AZStd::move(other.m_runArtifact);
        return *this;
    }

    const RepoPath& TestRunJobData::GetRunArtifactPath() const
    {
        return m_runArtifact;
    }
} // namespace TestImpact
