/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactTestTargetExclusionList.h>

namespace TestImpact
{
    TestTargetExclusionList::TestTargetExclusionList(
        AZStd::unordered_map<const NativeTestTarget*, AZStd::vector<AZStd::string>>&& excludedTestTargets)
        : m_excludedTestTargets(AZStd::move(excludedTestTargets))
    {
    }

    const AZStd::unordered_map<const NativeTestTarget*, AZStd::vector<AZStd::string>>& TestTargetExclusionList::GetExcludedTargets() const
    {
        return m_excludedTestTargets;
    }

    const AZStd::vector<AZStd::string>* TestTargetExclusionList::GetExcludedTestsForTarget(const NativeTestTarget* testTarget) const
    {
        const auto it = m_excludedTestTargets.find(testTarget);
        return it != m_excludedTestTargets.end()
            ? &it->second
            : nullptr;
    }

    bool TestTargetExclusionList::IsTestTargetFullyExcluded(const NativeTestTarget* testTarget) const
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
