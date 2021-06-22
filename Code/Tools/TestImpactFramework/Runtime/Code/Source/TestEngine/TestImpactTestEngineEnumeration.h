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

#include <TestEngine/TestImpactTestEngineJob.h>
#include <TestEngine/Enumeration/TestImpactTestEnumeration.h>

namespace TestImpact
{
    //! Represents the generated test enumeration data for a test engine enumeration.
    class TestEngineEnumeration
        : public TestEngineJob
    {
    public:
        TestEngineEnumeration(TestEngineJob&& job, AZStd::optional<TestEnumeration>&& enumeration);

        //! Returns the test enumeration payload for this job (if any).
        const AZStd::optional<TestEnumeration>& GetTestEnumeration() const;
    private:
        AZStd::optional<TestEnumeration> m_enumeration;
    };
} // namespace TestImpact
