/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Dependency/TestImpactSourceDependency.h>
#include <Target/native/TestImpactNativeTarget.h>
#include <Target/native/TestImpactNativeProductionTarget.h>
#include <Target/native/TestImpactNativeTestTarget.h>

namespace TestImpact
{
    bool ParentTarget::operator==(const ParentTarget& other) const
    {
        return GetBuildTarget() == other.GetBuildTarget();
    }

    const NativeTarget* ParentTarget::GetBuildTarget() const
    {
        const NativeTarget* buildTarget;
        AZStd::visit([&buildTarget](auto&& target)
        {
            buildTarget = target;

        }, m_target);

        return buildTarget;
    }

    const SpecializedNativeTarget& ParentTarget::GetTarget() const
    {
        return m_target;
    }

    SourceDependency::SourceDependency(
        const RepoPath& path,
        DependencyData&& dependencyData)
        : m_path(path)
        , m_dependencyData(AZStd::move(dependencyData))
    {        
    }

    const RepoPath& SourceDependency::GetPath() const
    {
        return m_path;
    }

    size_t SourceDependency::GetNumParentTargets() const
    {
        return m_dependencyData.m_parentTargets.size();
    }

    size_t SourceDependency::GetNumCoveringTestTargets() const
    {
        return m_dependencyData.m_coveringTestTargets.size();
    }

    const AZStd::unordered_set<ParentTarget>& SourceDependency::GetParentTargets() const
    {
        return m_dependencyData.m_parentTargets;
    }

    const AZStd::unordered_set<const NativeTestTarget*>& SourceDependency::GetCoveringTestTargets() const
    {
        return m_dependencyData.m_coveringTestTargets;
    }

} // namespace TestImpact
