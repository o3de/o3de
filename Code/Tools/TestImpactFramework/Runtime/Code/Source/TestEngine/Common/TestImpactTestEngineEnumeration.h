/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/TestImpactTestEngineJob.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>

namespace TestImpact
{
    //! Represents the generated test enumeration data for a test engine enumeration.
    template<typename BuildTargetTraits>
    class TestEngineEnumeration
        : public TestEngineJob<BuildTargetTraits>
    {
    public:
        TestEngineEnumeration(TestEngineJob<BuildTargetTraits>&& job, AZStd::optional<TestEnumeration>&& enumeration);

        //! Returns the test enumeration payload for this job (if any).
        const AZStd::optional<TestEnumeration>& GetTestEnumeration() const;
    private:
        AZStd::optional<TestEnumeration> m_enumeration;
    };

    template<typename BuildTargetTraits>
    TestEngineEnumeration<BuildTargetTraits>::TestEngineEnumeration(
        TestEngineJob<BuildTargetTraits>&& job, AZStd::optional<TestEnumeration>&& enumeration)
        : TestEngineJob(AZStd::move(job))
        , m_enumeration(AZStd::move(enumeration))
    {
    }

    template<typename BuildTargetTraits>
    const AZStd::optional<TestEnumeration>& TestEngineEnumeration<BuildTargetTraits>::GetTestEnumeration() const
    {
        return m_enumeration;
    }
} // namespace TestImpact
