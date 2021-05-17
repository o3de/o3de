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

#include <TestImpactFramework/TestImpactTestSelection.h>
namespace TestImpact
{
    TestSelection::TestSelection(AZStd::vector<AZStd::string>&& includedTests, AZStd::vector<AZStd::string>&& excludedTests)
        : m_includedTests(AZStd::move(includedTests))
        , m_excludedTests(AZStd::move(excludedTests))
    {
    }

    const AZStd::vector<AZStd::string>& TestSelection::GetIncludededTests() const
    {
        return m_includedTests;
    }

    const AZStd::vector<AZStd::string>& TestSelection::GetExcludedTests() const
    {
        return m_excludedTests;
    }
}
