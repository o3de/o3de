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
