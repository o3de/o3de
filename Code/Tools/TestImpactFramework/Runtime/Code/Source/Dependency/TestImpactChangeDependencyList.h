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
    template<typename BuildSystem>
    class ChangeDependencyList
    {
    public:
        ChangeDependencyList(
            AZStd::vector<SourceDependency<BuildSystem>>&& createSourceDependencies,
            AZStd::vector<SourceDependency<BuildSystem>>&& updateSourceDependencies,
            AZStd::vector<SourceDependency<BuildSystem>>&& deleteSourceDependencies);

        //! Gets the sources dependencies of the created source files from the change list.
        const AZStd::vector<SourceDependency<BuildSystem>>& GetCreateSourceDependencies() const;

        //! Gets the sources dependencies of the updated source files from the change list.
        const AZStd::vector<SourceDependency<BuildSystem>>& GetUpdateSourceDependencies() const;

        //! Gets the sources dependencies of the deleted source files from the change list.
        const AZStd::vector<SourceDependency<BuildSystem>>& GetDeleteSourceDependencies() const;
    private:
        AZStd::vector<SourceDependency<BuildSystem>> m_createSourceDependencies;
        AZStd::vector<SourceDependency<BuildSystem>> m_updateSourceDependencies;
        AZStd::vector<SourceDependency<BuildSystem>> m_deleteSourceDependencies;
    };

    template<typename BuildSystem>
    ChangeDependencyList<BuildSystem>::ChangeDependencyList(
        AZStd::vector<SourceDependency<BuildSystem>>&& createSourceDependencies,
        AZStd::vector<SourceDependency<BuildSystem>>&& updateSourceDependencies,
        AZStd::vector<SourceDependency<BuildSystem>>&& deleteSourceDependencies)
        : m_createSourceDependencies(AZStd::move(createSourceDependencies))
        , m_updateSourceDependencies(AZStd::move(updateSourceDependencies))
        , m_deleteSourceDependencies(AZStd::move(deleteSourceDependencies))
    {
    }

    template<typename BuildSystem>
    const AZStd::vector<SourceDependency<BuildSystem>>& ChangeDependencyList<BuildSystem>::GetCreateSourceDependencies() const
    {
        return m_createSourceDependencies;
    }

    template<typename BuildSystem>
    const AZStd::vector<SourceDependency<BuildSystem>>& ChangeDependencyList<BuildSystem>::GetUpdateSourceDependencies() const
    {
        return m_updateSourceDependencies;
    }

    template<typename BuildSystem>
    const AZStd::vector<SourceDependency<BuildSystem>>& ChangeDependencyList<BuildSystem>::GetDeleteSourceDependencies() const
    {
        return m_deleteSourceDependencies;
    }
} // namespace TestImpact
