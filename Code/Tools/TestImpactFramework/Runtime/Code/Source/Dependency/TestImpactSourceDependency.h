/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Target/Common/TestImpactTarget.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Representation of a source dependency's parent target.


    // TODO: remove this and just have BuildTarget
    template<typename BuildTargetTraits>
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
        const typename BuildTargetTraits::BuildTarget& GetBuildTarget() const;

        bool operator==(const ParentTarget& other) const;
    private:
        typename BuildTargetTraits::BuildTarget m_target; //! The specialized target pointer for this parent.
    };

    template<typename BuildTargetTraits>
    bool ParentTarget<BuildTargetTraits>::operator==(const ParentTarget& other) const
    {
        return GetTarget() == other.GetTarget();
    }

    template<typename BuildTargetTraits>
    const Target* ParentTarget<BuildTargetTraits>::GetTarget() const
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

    template<typename BuildTargetTraits>
    const typename BuildTargetTraits::BuildTarget& ParentTarget<BuildTargetTraits>::GetBuildTarget() const
    {
        return m_target;
    }
}

namespace AZStd
{
    //! Hash function for ParentTarget types for use in maps and sets.
    template<typename BuildTargetTraits>
    struct hash<TestImpact::ParentTarget<BuildTargetTraits>>
    {
        size_t operator()(const TestImpact::ParentTarget<BuildTargetTraits>& parentTarget) const noexcept
        {
            // TODO: get address of derived so we can not rely on Target as common ancestor
            return reinterpret_cast<size_t>(parentTarget.GetTarget());
        }
    };
}

namespace TestImpact
{
    template<typename BuildTargetTraits>
    struct DependencyData
    {
        AZStd::unordered_set<ParentTarget<BuildTargetTraits>> m_parentTargets;
        AZStd::unordered_set<const typename BuildTargetTraits::TestTarget*> m_coveringTestTargets;
    };

    //! Test target coverage and build target dependency information for a given source file in the dynamic dependency map.
    template<typename BuildTargetTraits>
    class SourceDependency
    {
    public:
        SourceDependency(const RepoPath& path, DependencyData<BuildTargetTraits>&& dependencyData);

        //! Returns the path of this source file.
        const RepoPath& GetPath() const;

        //! Returns the number of parent build targets this source belongs to.
        size_t GetNumParentTargets() const;

        //! Returns the number of test targets covering this source file.
        size_t GetNumCoveringTestTargets() const;

        //! Returns the parent targets that this source file belongs to.
        const AZStd::unordered_set<ParentTarget<BuildTargetTraits>>& GetParentTargets() const;

        //! Returns the test targets covering this source file.
        const AZStd::unordered_set<const typename BuildTargetTraits::TestTarget*>& GetCoveringTestTargets() const;
    private:
        RepoPath m_path; //!< The path of this source file.
        DependencyData<BuildTargetTraits> m_dependencyData; //!< The dependency data for this source file.
    };

    template<typename BuildTargetTraits>
    SourceDependency<BuildTargetTraits>::SourceDependency(const RepoPath& path, DependencyData<BuildTargetTraits>&& dependencyData)
        : m_path(path)
        , m_dependencyData(AZStd::move(dependencyData))
    {
    }

    template<typename BuildTargetTraits>
    const RepoPath& SourceDependency<BuildTargetTraits>::GetPath() const
    {
        return m_path;
    }

    template<typename BuildTargetTraits>
    size_t SourceDependency<BuildTargetTraits>::GetNumParentTargets() const
    {
        return m_dependencyData.m_parentTargets.size();
    }

    template<typename BuildTargetTraits>
    size_t SourceDependency<BuildTargetTraits>::GetNumCoveringTestTargets() const
    {
        return m_dependencyData.m_coveringTestTargets.size();
    }

    template<typename BuildTargetTraits>
    const AZStd::unordered_set<ParentTarget<BuildTargetTraits>>& SourceDependency<BuildTargetTraits>::GetParentTargets() const
    {
        return m_dependencyData.m_parentTargets;
    }

    template<typename BuildTargetTraits>
    const AZStd::unordered_set<const typename BuildTargetTraits::TestTarget*>& SourceDependency<BuildTargetTraits>::GetCoveringTestTargets() const
    {
        return m_dependencyData.m_coveringTestTargets;
    }
} // namespace TestImpact
