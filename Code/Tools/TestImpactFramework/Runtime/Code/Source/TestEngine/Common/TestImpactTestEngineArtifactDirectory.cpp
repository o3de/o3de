/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestEngine/Common/TestImpactTestEngineArtifactDirectory.h>

namespace TestImpact
{   
    ArtifactDirectory::ArtifactDirectory(RepoPath path, AZStd::string filter)
        : m_path(AZStd::move(path))
        , m_filter(AZStd::move(filter))
    {
    }

    const RepoPath& ArtifactDirectory::GetPath() const
    {
        return m_path;
    }

    const AZStd::string& ArtifactDirectory::GetFilter() const
    {
        return m_filter;
    }

    void ArtifactDirectory::DeleteContents() const
    {
        DeleteFiles(m_path, m_filter);
    }

    size_t ArtifactDirectory::CountContents() const
    {
        return CountFiles(m_path, m_filter);
    }
} // namespace TestImpact
