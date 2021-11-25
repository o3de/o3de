/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Dependency/TestImpactSourceDependency.h>

namespace TestImpact
{
    //! Representation of a change list where all CRUD sources have been resolved to source dependencies from the dynamic dependency map.
    template<typename BuildTargetTraits>
    class ChangeDependencyList
    {
    public:
        ChangeDependencyList(
            AZStd::vector<SourceDependency<BuildTargetTraits>>&& createSourceDependencies,
            AZStd::vector<SourceDependency<BuildTargetTraits>>&& updateSourceDependencies,
            AZStd::vector<SourceDependency<BuildTargetTraits>>&& deleteSourceDependencies);

        //! Gets the sources dependencies of the created source files from the change list.
        const AZStd::vector<SourceDependency<BuildTargetTraits>>& GetCreateSourceDependencies() const;

        //! Gets the sources dependencies of the updated source files from the change list.
        const AZStd::vector<SourceDependency<BuildTargetTraits>>& GetUpdateSourceDependencies() const;

        //! Gets the sources dependencies of the deleted source files from the change list.
        const AZStd::vector<SourceDependency<BuildTargetTraits>>& GetDeleteSourceDependencies() const;
    private:
        AZStd::vector<SourceDependency<BuildTargetTraits>> m_createSourceDependencies;
        AZStd::vector<SourceDependency<BuildTargetTraits>> m_updateSourceDependencies;
        AZStd::vector<SourceDependency<BuildTargetTraits>> m_deleteSourceDependencies;
    };

    template<typename BuildTargetTraits>
    ChangeDependencyList<BuildTargetTraits>::ChangeDependencyList(
        AZStd::vector<SourceDependency<BuildTargetTraits>>&& createSourceDependencies,
        AZStd::vector<SourceDependency<BuildTargetTraits>>&& updateSourceDependencies,
        AZStd::vector<SourceDependency<BuildTargetTraits>>&& deleteSourceDependencies)
        : m_createSourceDependencies(AZStd::move(createSourceDependencies))
        , m_updateSourceDependencies(AZStd::move(updateSourceDependencies))
        , m_deleteSourceDependencies(AZStd::move(deleteSourceDependencies))
    {
    }

    template<typename BuildTargetTraits>
    const AZStd::vector<SourceDependency<BuildTargetTraits>>& ChangeDependencyList<BuildTargetTraits>::GetCreateSourceDependencies() const
    {
        return m_createSourceDependencies;
    }

    template<typename BuildTargetTraits>
    const AZStd::vector<SourceDependency<BuildTargetTraits>>& ChangeDependencyList<BuildTargetTraits>::GetUpdateSourceDependencies() const
    {
        return m_updateSourceDependencies;
    }

    template<typename BuildTargetTraits>
    const AZStd::vector<SourceDependency<BuildTargetTraits>>& ChangeDependencyList<BuildTargetTraits>::GetDeleteSourceDependencies() const
    {
        return m_deleteSourceDependencies;
    }
} // namespace TestImpact
