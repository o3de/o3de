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
    class ChangeDependencyList
    {
    public:
        ChangeDependencyList(
            AZStd::vector<SourceDependency>&& createSourceDependencies,
            AZStd::vector<SourceDependency>&& updateSourceDependencies,
            AZStd::vector<SourceDependency>&& deleteSourceDependencies);

        //! Gets the sources dependencies of the created source files from the change list.
        const AZStd::vector<SourceDependency>& GetCreateSourceDependencies() const;

        //! Gets the sources dependencies of the updated source files from the change list.
        const AZStd::vector<SourceDependency>& GetUpdateSourceDependencies() const;

        //! Gets the sources dependencies of the deleted source files from the change list.
        const AZStd::vector<SourceDependency>& GetDeleteSourceDependencies() const;
    private:
        AZStd::vector<SourceDependency> m_createSourceDependencies;
        AZStd::vector<SourceDependency> m_updateSourceDependencies;
        AZStd::vector<SourceDependency> m_deleteSourceDependencies;
    };
} // namespace TestImpact
