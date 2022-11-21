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
    template<typename ProductionTarget, typename TestTarget>
    class ChangeDependencyList
    {
    public:
        ChangeDependencyList(
            AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>&& createSourceDependencies,
            AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>&& updateSourceDependencies,
            AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>&& deleteSourceDependencies);

        //! Gets the sources dependencies of the created source files from the change list.
        const AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>& GetCreateSourceDependencies() const;

        //! Gets the sources dependencies of the updated source files from the change list.
        const AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>& GetUpdateSourceDependencies() const;

        //! Gets the sources dependencies of the deleted source files from the change list.
        const AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>& GetDeleteSourceDependencies() const;
    private:
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>> m_createSourceDependencies;
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>> m_updateSourceDependencies;
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>> m_deleteSourceDependencies;
    };

    template<typename ProductionTarget, typename TestTarget>
    ChangeDependencyList<ProductionTarget, TestTarget>::ChangeDependencyList(
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>&& createSourceDependencies,
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>&& updateSourceDependencies,
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>&& deleteSourceDependencies)
        : m_createSourceDependencies(AZStd::move(createSourceDependencies))
        , m_updateSourceDependencies(AZStd::move(updateSourceDependencies))
        , m_deleteSourceDependencies(AZStd::move(deleteSourceDependencies))
    {
    }

    template<typename ProductionTarget, typename TestTarget>
    const AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>& ChangeDependencyList<ProductionTarget, TestTarget>::GetCreateSourceDependencies() const
    {
        return m_createSourceDependencies;
    }

    template<typename ProductionTarget, typename TestTarget>
    const AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>& ChangeDependencyList<ProductionTarget, TestTarget>::GetUpdateSourceDependencies() const
    {
        return m_updateSourceDependencies;
    }

    template<typename ProductionTarget, typename TestTarget>
    const AZStd::vector<SourceDependency<ProductionTarget, TestTarget>>& ChangeDependencyList<ProductionTarget, TestTarget>::GetDeleteSourceDependencies() const
    {
        return m_deleteSourceDependencies;
    }
} // namespace TestImpact
