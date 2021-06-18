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
