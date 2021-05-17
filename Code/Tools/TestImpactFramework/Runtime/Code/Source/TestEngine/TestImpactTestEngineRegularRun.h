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

#include <TestImpactFramework/TestImpactTest.h>

#include <TestEngine/TestImpactTestEngineJob.h>
#include <TestEngine/Run/TestImpactTestRun.h>

namespace TestImpact
{
    class TestEngineRegularRun
        : public TestEngineJob
    {
    public:
        TestEngineRegularRun(TestEngineJob&& testJob, AZStd::optional<TestRun>&& testRun, TestResult testResult);
        TestResult GetTestResult() const;
        const AZStd::optional<TestRun>& GetTestRun() const;
    private:
        AZStd::optional<TestRun> m_testRun;
        TestResult m_testResult;
    };
} // namespace TestImpact
