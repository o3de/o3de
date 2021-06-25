/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/TestImpactTestEngineRegularRun.h>

namespace TestImpact
{
    TestEngineRegularRun::TestEngineRegularRun(TestEngineJob&& testJob, AZStd::optional<TestRun>&& testRun)
        : TestEngineJob(AZStd::move(testJob))
        , m_testRun(AZStd::move(testRun))
    {
    }

    const AZStd::optional<TestRun>& TestEngineRegularRun::GetTestRun() const
    {
        return m_testRun;
    }
} // namespace TestImpact
