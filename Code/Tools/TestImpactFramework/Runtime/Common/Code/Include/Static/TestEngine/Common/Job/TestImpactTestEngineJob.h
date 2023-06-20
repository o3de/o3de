/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <Process/JobRunner/TestImpactProcessJobMeta.h>

namespace TestImpact
{
    //! Represents the meta-data describing a test engine run.
    template<typename TestTarget>
    class TestEngineJob
        : public JobMetaWrapper
    {
    public:
        TestEngineJob(
            const TestTarget* testTarget,
            const AZStd::string& commandString,
            const JobMeta& jobMeta,
            Client::TestRunResult testResult,
            AZStd::string&& stdOut,
            AZStd::string&& stdErr);

        //! Returns the test target that was run for this job.
        const TestTarget* GetTestTarget() const;

        //! Returns the result of the job that was run.
        Client::TestRunResult GetTestResult() const;

        //! Returns the command string that was used to execute this job.
        const AZStd::string& GetCommandString() const;

        //! Returns the standard output of this job (if captured).
        const AZStd::string& GetStdOutput() const;

        //! Returns the standard error of this job (if captured).
        const AZStd::string& GetStdError() const;

    private:
        const TestTarget* m_testTarget;
        AZStd::string m_commandString;
        Client::TestRunResult m_testResult;
        AZStd::string m_stdOut;
        AZStd::string m_stdErr;
    };

    template<typename TestTarget>
    TestEngineJob<TestTarget>::TestEngineJob(
        const TestTarget* testTarget,
        const AZStd::string& commandString,
        const JobMeta& jobMeta,
        Client::TestRunResult testResult,
        AZStd::string&& stdOut,
        AZStd::string&& stdErr)
        : JobMetaWrapper(jobMeta)
        , m_testTarget(testTarget)
        , m_commandString(commandString)
        , m_testResult(testResult)
        , m_stdOut(AZStd::move(stdOut))
        , m_stdErr(AZStd::move(stdErr))
    {
    }

    template<typename TestTarget>
    const TestTarget* TestEngineJob<TestTarget>::GetTestTarget() const
    {
        return m_testTarget;
    }

    template<typename TestTarget>
    const AZStd::string& TestEngineJob<TestTarget>::GetCommandString() const
    {
        return m_commandString;
    }

    template<typename TestTarget>
    Client::TestRunResult TestEngineJob<TestTarget>::GetTestResult() const
    {
        return m_testResult;
    }

    template<typename TestTarget>
    const AZStd::string& TestEngineJob<TestTarget>::GetStdOutput() const
    {
        return m_stdOut;
    }

    template<typename TestTarget>
    const AZStd::string& TestEngineJob<TestTarget>::GetStdError() const
    {
        return m_stdErr;
    }
} // namespace TestImpact
