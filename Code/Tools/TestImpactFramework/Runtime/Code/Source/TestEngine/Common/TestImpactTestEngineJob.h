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
    //! Represents the meta-data describing a test engine run.
    template<typename BuildTargetTraits>
    class TestEngineJob
        : public JobMetaWrapper
    {
    public:
        TestEngineJob(
            const typename BuildTargetTraits::TestTarget* testTarget,
            const AZStd::string& commandString,
            const JobMeta& jobMeta,
            Client::TestRunResult testResult,
            AZStd::string&& stdOut,
            AZStd::string&& stdErr);

        //! Returns the test target that was run for this job.
        const typename BuildTargetTraits::TestTarget* GetTestTarget() const;

        //! Returns the result of the job that was run.
        Client::TestRunResult GetTestResult() const;

        //! Returns the command string that was used to execute this job.
        const AZStd::string& GetCommandString() const;

        //! Returns the standard output of this job (if captured).
        const AZStd::string& GetStdOutput() const;

        //! Returns the standard error of this job (if captured).
        const AZStd::string& GetStdError() const;

    private:
        const typename BuildTargetTraits::TestTarget* m_testTarget;
        AZStd::string m_commandString;
        Client::TestRunResult m_testResult;
        AZStd::string m_stdOut;
        AZStd::string m_stdErr;
    };

    template<typename BuildTargetTraits>
    TestEngineJob<BuildTargetTraits>::TestEngineJob(
        const typename BuildTargetTraits::TestTarget* testTarget,
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

    template<typename BuildTargetTraits>
    const typename BuildTargetTraits::TestTarget* TestEngineJob<BuildTargetTraits>::GetTestTarget() const
    {
        return m_testTarget;
    }

    template<typename BuildTargetTraits>
    const AZStd::string& TestEngineJob<BuildTargetTraits>::GetCommandString() const
    {
        return m_commandString;
    }

    template<typename BuildTargetTraits>
    Client::TestRunResult TestEngineJob<BuildTargetTraits>::GetTestResult() const
    {
        return m_testResult;
    }

    template<typename BuildTargetTraits>
    const AZStd::string& TestEngineJob<BuildTargetTraits>::GetStdOutput() const
    {
        return m_stdOut;
    }

    template<typename BuildTargetTraits>
    const AZStd::string& TestEngineJob<BuildTargetTraits>::GetStdError() const
    {
        return m_stdErr;
    }
} // namespace TestImpact
