/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! List for excluding entire test targets (or a subset of the target's tests) from test runs.
    template<typename TestTarget>
    class TestTargetExclusionList
    {
    public:
        //! Mapping of test targets and their tests that should not be included in test runs.
        //! @note if the vector is empty, all tests in the test target are excluded, otherwise
        //! only those tests specified in the vector.
        using TestTargetExclusionMap = AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>;

        //! Constructs the test target exclusion list from the specified test target exclusion map.
        TestTargetExclusionList(AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>&& excludedTestTargets);

        //! Retrieves the test target exclusion map.
        const AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>& GetExcludedTargets() const;

        //! Retrieves the excluded tests for the specified test target.
        //! @returns If the specified test target is excluded in any way, the vector of excluded tests 
        //! (empty means all tests are excluded) for the specified test target, otherwise nullptr.
        const AZStd::vector<AZStd::string>* GetExcludedTestsForTarget(const TestTarget* testTarget) const;

        //! Returns true if the specified test target is fully excluded (all tests are excluded), otherwise false.
        bool IsTestTargetFullyExcluded(const TestTarget* testTarget) const;

        //! Returns the number of excluded test targets in the list.
        size_t GetNumTargets() const;

        //! Returns true if the list has no excluded test targets, otherwise false.
        bool IsEmpty() const;

    private:
        //! The the test target exclusion map.
        AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>> m_excludedTestTargets;
    };

    template<typename TestTarget>
    TestTargetExclusionList<TestTarget>::TestTargetExclusionList(
        AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>&& excludedTestTargets)
        : m_excludedTestTargets(AZStd::move(excludedTestTargets))
    {
    }

    template<typename TestTarget>
    const AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>& TestTargetExclusionList<TestTarget>::GetExcludedTargets() const
    {
        return m_excludedTestTargets;
    }

    template<typename TestTarget>
    const AZStd::vector<AZStd::string>* TestTargetExclusionList<TestTarget>::GetExcludedTestsForTarget(const TestTarget* testTarget) const
    {
        const auto it = m_excludedTestTargets.find(testTarget);
        return it != m_excludedTestTargets.end()
            ? &it->second
            : nullptr;
    }

    template<typename TestTarget>
    bool TestTargetExclusionList<TestTarget>::IsTestTargetFullyExcluded(const TestTarget* testTarget) const
    {
        if (const auto* excludedTests = GetExcludedTestsForTarget(testTarget);
            excludedTests)
        {
            return excludedTests->empty();
        }

        return false;
    }

    template<typename TestTarget>
    size_t TestTargetExclusionList<TestTarget>::GetNumTargets() const
    {
        return m_excludedTestTargets.size();
    }

    template<typename TestTarget>
    bool TestTargetExclusionList<TestTarget>::IsEmpty() const
    {
        return m_excludedTestTargets.empty();
    }
} // namespace TestImpact
