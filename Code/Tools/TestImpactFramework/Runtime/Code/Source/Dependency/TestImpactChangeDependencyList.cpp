/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
