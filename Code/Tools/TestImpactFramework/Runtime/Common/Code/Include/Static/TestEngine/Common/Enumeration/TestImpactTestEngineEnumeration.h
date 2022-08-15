/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/Job/TestImpactTestEngineJob.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>

namespace TestImpact
{
    //! Represents the generated test enumeration data for a test engine enumeration.
    template<typename TestTarget>
    class TestEngineEnumeration
        : public TestEngineJob<TestTarget>
    {
    public:
        TestEngineEnumeration(TestEngineJob<TestTarget>&& job, AZStd::optional<TestEnumeration>&& enumeration);

        //! Returns the test enumeration payload for this job (if any).
        const AZStd::optional<TestEnumeration>& GetTestEnumeration() const;
    private:
        AZStd::optional<TestEnumeration> m_enumeration;
    };

    template<typename TestTarget>
    TestEngineEnumeration<TestTarget>::TestEngineEnumeration(
        TestEngineJob<TestTarget>&& job, AZStd::optional<TestEnumeration>&& enumeration)
        : TestEngineJob(AZStd::move(job))
        , m_enumeration(AZStd::move(enumeration))
    {
    }

    template<typename TestTarget>
    const AZStd::optional<TestEnumeration>& TestEngineEnumeration<TestTarget>::GetTestEnumeration() const
    {
        return m_enumeration;
    }
} // namespace TestImpact
