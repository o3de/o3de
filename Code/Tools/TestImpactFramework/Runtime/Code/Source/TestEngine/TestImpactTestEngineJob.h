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

#include <Process/JobRunner/TestImpactProcessJobMeta.h>

namespace TestImpact
{
    class TestTarget;

    //! Represents the meta-data describing a test engine run.
    class TestEngineJob
        : public JobMetaWrapper
    {
    public:
        TestEngineJob(const TestTarget* testTarget, const AZStd::string& commandString, const JobMeta& jobMeta, Client::TestRunResult testResult);

        //! Returns the test target that was run for this job.
        const TestTarget* GetTestTarget() const;

        //! Returns the result of the job that was run.
        Client::TestRunResult GetTestResult() const;

        //! Returns the command string that was used to execute this job.
        const AZStd::string& GetCommandString() const;
    private:
        const TestTarget* m_testTarget;
        AZStd::string m_commandString;
        Client::TestRunResult m_testResult;
    };
} // namespace TestImpact
