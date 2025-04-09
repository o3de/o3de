/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Common/TestImpactTarget.h>

namespace TestImpact
{
    Target::Target(TargetDescriptor&& descriptor)
        : m_descriptor(AZStd::move(descriptor))
    {
    }

    const AZStd::string& Target::GetName() const
    {
        return m_descriptor.m_name;
    }

    TargetType Target::GetType() const
    {
        return m_descriptor.m_type;
    }

    const RepoPath& Target::GetPath() const
    {
        return m_descriptor.m_path;
    }

    const TargetSources& Target::GetSources() const
    {
        return m_descriptor.m_sources;
    }

    const AZStd::string& Target::GetOutputName() const
    {
        return m_descriptor.m_outputName;
    }

    const TargetDependencies& Target::GetDependencies() const
    {
        return m_descriptor.m_dependencies;
    }
} // namespace TestImpact
