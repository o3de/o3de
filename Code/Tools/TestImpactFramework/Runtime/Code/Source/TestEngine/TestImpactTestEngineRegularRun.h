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

#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <TestEngine/TestImpactTestEngineJob.h>
#include <TestEngine/Run/TestImpactTestRun.h>

namespace TestImpact
{
    //! Represents the generated test run data for a regular test engine run. 
    class TestEngineRegularRun
        : public TestEngineJob
    {
    public:
        TestEngineRegularRun(TestEngineJob&& testJob, AZStd::optional<TestRun>&& testRun);

        //! Returns the test run payload for this job (if any).
        const AZStd::optional<TestRun>& GetTestRun() const;
    private:
        AZStd::optional<TestRun> m_testRun;
    };
} // namespace TestImpact
