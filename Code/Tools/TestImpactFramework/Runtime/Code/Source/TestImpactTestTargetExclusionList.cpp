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

#include <TestImpactTestTargetExclusionList.h>

namespace TestImpact
{
    TestTargetExclusionList::TestTargetExclusionList(
        AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>&& excludedTestTargets)
        : m_excludedTestTargets(AZStd::move(excludedTestTargets))
    {
    }

    const AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>& TestTargetExclusionList::GetExcludedTargets() const
    {
        return m_excludedTestTargets;
    }

    const AZStd::vector<AZStd::string>* TestTargetExclusionList::GetExcludedTestsForTarget(const TestTarget* testTarget) const
    {
        const auto it = m_excludedTestTargets.find(testTarget);
        return it != m_excludedTestTargets.end()
            ? &it->second
            : nullptr;
    }

    bool TestTargetExclusionList::IsTestTargetFullyExcluded(const TestTarget* testTarget) const
    {
        if (const auto* excludedTests = GetExcludedTestsForTarget(testTarget);
            excludedTests)
        {
            return excludedTests->empty();
        }

        return false;
    }

    size_t TestTargetExclusionList::GetNumTargets() const
    {
        return m_excludedTestTargets.size();
    }

    bool TestTargetExclusionList::IsEmpty() const
    {
        return m_excludedTestTargets.empty();
    }
}
