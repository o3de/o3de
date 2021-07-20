/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
