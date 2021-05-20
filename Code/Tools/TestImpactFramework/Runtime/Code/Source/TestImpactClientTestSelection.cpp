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

#include <TestImpactFramework/TestImpactClientTestSelection.h>

namespace TestImpact
{
    namespace Client
    {
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

        size_t TestRunSelection::GetNumNumExcludedTestRuns() const
        {
            return m_excludedTestRuns.size();
        }

    }
}
