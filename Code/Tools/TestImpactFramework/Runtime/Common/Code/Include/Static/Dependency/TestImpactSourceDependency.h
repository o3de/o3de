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
    template<typename TestTarget, typename ProductionTarget>
    struct DependencyData
    {
        AZStd::unordered_set<BuildTarget<TestTarget, ProductionTarget>> m_parentTargets;
        AZStd::unordered_set<const TestTarget*> m_coveringTestTargets;
    };

    //! Test target coverage and build target dependency information for a given source file in the dynamic dependency map.
    template<typename TestTarget, typename ProductionTarget>
    class SourceDependency
    {
    public:
        SourceDependency(const RepoPath& path, DependencyData<TestTarget, ProductionTarget>&& dependencyData);

        //! Returns the path of this source file.
        const RepoPath& GetPath() const;

        //! Returns the number of parent build targets this source belongs to.
        size_t GetNumParentTargets() const;

        //! Returns the number of test targets covering this source file.
        size_t GetNumCoveringTestTargets() const;

        //! Returns the parent targets that this source file belongs to.
        const AZStd::unordered_set<BuildTarget<TestTarget, ProductionTarget>>& GetParentTargets() const;

        //! Returns the test targets covering this source file.
        const AZStd::unordered_set<const TestTarget*>& GetCoveringTestTargets() const;
    private:
        RepoPath m_path; //!< The path of this source file.
        DependencyData<TestTarget, ProductionTarget> m_dependencyData; //!< The dependency data for this source file.
    };

    template<typename TestTarget, typename ProductionTarget>
    SourceDependency<TestTarget, ProductionTarget>::SourceDependency(const RepoPath& path, DependencyData<TestTarget, ProductionTarget>&& dependencyData)
        : m_path(path)
        , m_dependencyData(AZStd::move(dependencyData))
    {
    }

    template<typename TestTarget, typename ProductionTarget>
    const RepoPath& SourceDependency<TestTarget, ProductionTarget>::GetPath() const
    {
        return m_path;
    }

    template<typename TestTarget, typename ProductionTarget>
    size_t SourceDependency<TestTarget, ProductionTarget>::GetNumParentTargets() const
    {
        return m_dependencyData.m_parentTargets.size();
    }

    template<typename TestTarget, typename ProductionTarget>
    size_t SourceDependency<TestTarget, ProductionTarget>::GetNumCoveringTestTargets() const
    {
        return m_dependencyData.m_coveringTestTargets.size();
    }

    template<typename TestTarget, typename ProductionTarget>
    const AZStd::unordered_set<BuildTarget<TestTarget, ProductionTarget>>& SourceDependency<TestTarget, ProductionTarget>::GetParentTargets() const
    {
        return m_dependencyData.m_parentTargets;
    }

    template<typename TestTarget, typename ProductionTarget>
    const AZStd::unordered_set<const TestTarget*>& SourceDependency<TestTarget, ProductionTarget>::GetCoveringTestTargets() const
    {
        return m_dependencyData.m_coveringTestTargets;
    }
} // namespace TestImpact
