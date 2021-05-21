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
