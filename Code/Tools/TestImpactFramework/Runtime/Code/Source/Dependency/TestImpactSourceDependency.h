/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildSystem/Common/TestImpactBuildTarget.h>
#include <Target/Common/TestImpactTarget.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Representation of a source dependency's parent target.
    template<typename BuildSystem>
    class ParentTarget
    {
    public:
        template<typename TargetType>
        ParentTarget(const TargetType* target)
            : m_target(target)
        {
        }

        //! Returns the generic target pointer for this parent.
        const Target* GetTarget() const;

        //! Returns the build target pointer for this parent.
        const typename BuildSystem::BuildTarget& GetBuildTarget() const;

        bool operator==(const ParentTarget& other) const;
    private:
        typename BuildSystem::BuildTarget m_target; //! The specialized target pointer for this parent.
    };

    template<typename BuildSystem>
    bool ParentTarget<BuildSystem>::operator==(const ParentTarget& other) const
    {
        return GetTarget() == other.GetTarget();
    }

    template<typename BuildSystem>
    const Target* ParentTarget<BuildSystem>::GetTarget() const
    {
        const Target* buildTarget;
        AZStd::visit(
        [&buildTarget](auto&& target)
        {
            buildTarget = target;
        },
        m_target);

        return buildTarget;
    }

    template<typename BuildSystem>
    const typename BuildSystem::BuildTarget& ParentTarget<BuildSystem>::GetBuildTarget() const
    {
        return m_target;
    }
}

namespace AZStd
{
    //! Hash function for ParentTarget types for use in maps and sets.
    template<typename BuildSystem>
    struct hash<TestImpact::ParentTarget<BuildSystem>>
    {
        size_t operator()(const TestImpact::ParentTarget<BuildSystem>& parentTarget) const noexcept
        {
            return reinterpret_cast<size_t>(parentTarget.GetTarget());
        }
    };
}

namespace TestImpact
{
    template<typename BuildSystem>
    struct DependencyData
    {
        AZStd::unordered_set<ParentTarget<BuildSystem>> m_parentTargets;
        AZStd::unordered_set<const typename BuildSystem::TestTarget*> m_coveringTestTargets;
    };

    //! Test target coverage and build target dependency information for a given source file in the dynamic dependency map.
    template<typename BuildSystem>
    class SourceDependency
    {
    public:
        SourceDependency(const RepoPath& path, DependencyData<BuildSystem>&& dependencyData);

        //! Returns the path of this source file.
        const RepoPath& GetPath() const;

        //! Returns the number of parent build targets this source belongs to.
        size_t GetNumParentTargets() const;

        //! Returns the number of test targets covering this source file.
        size_t GetNumCoveringTestTargets() const;

        //! Returns the parent targets that this source file belongs to.
        const AZStd::unordered_set<ParentTarget<BuildSystem>>& GetParentTargets() const;

        //! Returns the test targets covering this source file.
        const AZStd::unordered_set<const typename BuildSystem::TestTarget*>& GetCoveringTestTargets() const;
    private:
        RepoPath m_path; //!< The path of this source file.
        DependencyData<BuildSystem> m_dependencyData; //!< The dependency data for this source file.
    };

    template<typename BuildSystem>
    SourceDependency<BuildSystem>::SourceDependency(const RepoPath& path, DependencyData<BuildSystem>&& dependencyData)
        : m_path(path)
        , m_dependencyData(AZStd::move(dependencyData))
    {
    }

    template<typename BuildSystem>
    const RepoPath& SourceDependency<BuildSystem>::GetPath() const
    {
        return m_path;
    }

    template<typename BuildSystem>
    size_t SourceDependency<BuildSystem>::GetNumParentTargets() const
    {
        return m_dependencyData.m_parentTargets.size();
    }

    template<typename BuildSystem>
    size_t SourceDependency<BuildSystem>::GetNumCoveringTestTargets() const
    {
        return m_dependencyData.m_coveringTestTargets.size();
    }

    template<typename BuildSystem>
    const AZStd::unordered_set<ParentTarget<BuildSystem>>& SourceDependency<BuildSystem>::GetParentTargets() const
    {
        return m_dependencyData.m_parentTargets;
    }

    template<typename BuildSystem>
    const AZStd::unordered_set<const typename BuildSystem::TestTarget*>& SourceDependency<BuildSystem>::GetCoveringTestTargets() const
    {
        return m_dependencyData.m_coveringTestTargets;
    }
} // namespace TestImpact
