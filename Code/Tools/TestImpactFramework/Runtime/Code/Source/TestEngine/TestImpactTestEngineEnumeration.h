/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
