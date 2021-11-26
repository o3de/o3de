/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <TestEngine/Common/TestImpactTestEngineJob.h>
#include <TestRunner/Common/Run/TestImpactTestRun.h>

namespace TestImpact
{
    //! Represents the generated test run data for a regular test engine run. 
    template<typename BuildTargetTraits>
    class TestEngineRegularRun
        : public TestEngineJob<BuildTargetTraits>
    {
    public:
        TestEngineRegularRun(TestEngineJob<BuildTargetTraits>&& testJob, AZStd::optional<TestRun>&& testRun);

        //! Returns the test run payload for this job (if any).
        const AZStd::optional<TestRun>& GetTestRun() const;
    private:
        AZStd::optional<TestRun> m_testRun;
    };

    template<typename BuildTargetTraits>
    TestEngineRegularRun<BuildTargetTraits>::TestEngineRegularRun(
        TestEngineJob<BuildTargetTraits>&& testJob, AZStd::optional<TestRun>&& testRun)
        : TestEngineJob<NativeBuildTargetTraits>(AZStd::move(testJob))
        , m_testRun(AZStd::move(testRun))
    {
    }

    template<typename BuildTargetTraits>
    const AZStd::optional<TestRun>& TestEngineRegularRun<BuildTargetTraits>::GetTestRun() const
    {
        return m_testRun;
    }
} // namespace TestImpact
