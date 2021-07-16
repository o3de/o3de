/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Dependency/TestImpactChangeDependencyList.h>

namespace TestImpact
{
    ChangeDependencyList::ChangeDependencyList(
        AZStd::vector<SourceDependency>&& createSourceDependencies,
        AZStd::vector<SourceDependency>&& updateSourceDependencies,
        AZStd::vector<SourceDependency>&& deleteSourceDependencies)
        : m_createSourceDependencies(AZStd::move(createSourceDependencies))
        , m_updateSourceDependencies(AZStd::move(updateSourceDependencies))
        , m_deleteSourceDependencies(AZStd::move(deleteSourceDependencies))
    {
    }

    const AZStd::vector<SourceDependency>& ChangeDependencyList::GetCreateSourceDependencies() const
    {
        return m_createSourceDependencies;
    }

    const AZStd::vector<SourceDependency>& ChangeDependencyList::GetUpdateSourceDependencies() const
    {
        return m_updateSourceDependencies;
    }

    const AZStd::vector<SourceDependency>& ChangeDependencyList::GetDeleteSourceDependencies() const
    {
        return m_deleteSourceDependencies;
    }
} // namespace TestImpact
