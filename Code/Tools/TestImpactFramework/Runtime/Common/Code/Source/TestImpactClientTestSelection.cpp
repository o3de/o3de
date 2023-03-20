/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactClientTestSelection.h>

namespace TestImpact
{
    namespace Client
    {
        TestRunSelection::TestRunSelection(const AZStd::vector<AZStd::string>& includedTests, const AZStd::vector<AZStd::string>& excludedTests)
            : m_includedTestRuns(includedTests)
            , m_excludedTestRuns(excludedTests)
        {
        }

        TestRunSelection::TestRunSelection(AZStd::vector<AZStd::string>&& includedTests, AZStd::vector<AZStd::string>&& excludedTests)
            : m_includedTestRuns(AZStd::move(includedTests))
            , m_excludedTestRuns(AZStd::move(excludedTests))
        {
        }

        const AZStd::vector<AZStd::string>& TestRunSelection::GetIncludededTestRuns() const
        {
            return m_includedTestRuns;
        }

        const AZStd::vector<AZStd::string>& TestRunSelection::GetExcludedTestRuns() const
        {
            return m_excludedTestRuns;
        }

        size_t TestRunSelection::GetNumIncludedTestRuns() const
        {
            return m_includedTestRuns.size();
        }

        size_t TestRunSelection::GetNumExcludedTestRuns() const
        {
            return m_excludedTestRuns.size();
        }

        size_t TestRunSelection::GetTotalNumTests() const
        {
            return GetNumIncludedTestRuns() + GetNumExcludedTestRuns();
        }
    } // namespace Client
} // namespace TestImpact
