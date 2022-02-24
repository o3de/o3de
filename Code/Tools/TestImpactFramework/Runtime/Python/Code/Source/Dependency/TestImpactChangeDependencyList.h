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
    template<typename TestTarget, typename ProductionTarget>
    class ChangeDependencyList
    {
    public:
        ChangeDependencyList(
            AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>&& createSourceDependencies,
            AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>&& updateSourceDependencies,
            AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>&& deleteSourceDependencies);

        //! Gets the sources dependencies of the created source files from the change list.
        const AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>& GetCreateSourceDependencies() const;

        //! Gets the sources dependencies of the updated source files from the change list.
        const AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>& GetUpdateSourceDependencies() const;

        //! Gets the sources dependencies of the deleted source files from the change list.
        const AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>& GetDeleteSourceDependencies() const;
    private:
        AZStd::vector<SourceDependency<TestTarget, ProductionTarget>> m_createSourceDependencies;
        AZStd::vector<SourceDependency<TestTarget, ProductionTarget>> m_updateSourceDependencies;
        AZStd::vector<SourceDependency<TestTarget, ProductionTarget>> m_deleteSourceDependencies;
    };

    template<typename TestTarget, typename ProductionTarget>
    ChangeDependencyList<TestTarget, ProductionTarget>::ChangeDependencyList(
        AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>&& createSourceDependencies,
        AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>&& updateSourceDependencies,
        AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>&& deleteSourceDependencies)
        : m_createSourceDependencies(AZStd::move(createSourceDependencies))
        , m_updateSourceDependencies(AZStd::move(updateSourceDependencies))
        , m_deleteSourceDependencies(AZStd::move(deleteSourceDependencies))
    {
    }

    template<typename TestTarget, typename ProductionTarget>
    const AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>& ChangeDependencyList<TestTarget, ProductionTarget>::GetCreateSourceDependencies() const
    {
        return m_createSourceDependencies;
    }

    template<typename TestTarget, typename ProductionTarget>
    const AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>& ChangeDependencyList<TestTarget, ProductionTarget>::GetUpdateSourceDependencies() const
    {
        return m_updateSourceDependencies;
    }

    template<typename TestTarget, typename ProductionTarget>
    const AZStd::vector<SourceDependency<TestTarget, ProductionTarget>>& ChangeDependencyList<TestTarget, ProductionTarget>::GetDeleteSourceDependencies() const
    {
        return m_deleteSourceDependencies;
    }
} // namespace TestImpact
