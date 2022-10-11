/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Job/TestImpactTestEnumerationJobData.h>

namespace TestImpact
{
    TestEnumerationJobData::TestEnumerationJobData(const RepoPath& enumerationArtifact, AZStd::optional<Cache>&& cache)
        : m_enumerationArtifact(enumerationArtifact)
        , m_cache(AZStd::move(cache))
    {
    }

    const RepoPath& TestEnumerationJobData::GetEnumerationArtifactPath() const
    {
        return m_enumerationArtifact;
    }

    const AZStd::optional<TestEnumerationJobData::Cache>& TestEnumerationJobData::GetCache() const
    {
        return m_cache;
    }
} // namespace TestImpact
