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
    class TestTarget;

    class TestTargetExclusionList
    {
    public:
        //!
        TestTargetExclusionList(AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>&& excludedTestTargets);

        //!
        const AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>>& GetExcludedTargets() const;

        //!
        const AZStd::vector<AZStd::string>* GetExcludedTestsForTarget(const TestTarget* testTarget) const;

        //!
        bool IsTestTargetFullyExcluded(const TestTarget* testTarget) const;

        //! Returns the number of excluded test targets in the list.
        size_t GetNumTargets() const;

        bool IsEmpty() const;

    private:
        AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>> m_excludedTestTargets;
    };
} // namespace TestImpact
