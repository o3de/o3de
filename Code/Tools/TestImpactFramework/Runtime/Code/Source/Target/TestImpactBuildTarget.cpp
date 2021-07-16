/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestImpactBuildTarget.h"

namespace TestImpact
{
    BuildTarget::BuildTarget(BuildTargetDescriptor&& descriptor, TargetType type)
        : m_buildMetaData(AZStd::move(descriptor.m_buildMetaData))
        , m_sources(AZStd::move(descriptor.m_sources))
        , m_type(type)
    {
    }

    const AZStd::string& BuildTarget::GetName() const
    {
        return m_buildMetaData.m_name;
    }

    const AZStd::string& BuildTarget::GetOutputName() const
    {
        return m_buildMetaData.m_outputName;
    }

    const RepoPath& BuildTarget::GetPath() const
    {
        return m_buildMetaData.m_path;
    }

    const TargetSources& BuildTarget::GetSources() const
    {
        return m_sources;
    }

    TargetType BuildTarget::GetType() const
    {
        return m_type;
    }
} // namespace TestImpact
