/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
