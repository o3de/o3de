/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactTestUtils.h>

#include <Process/TestImpactProcess.h>
#include <Process/TestImpactProcessLauncher.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/string/conversions.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class ProcessTestFixture
        : public AllocatorsTestFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();
        }

        void TearDown() override
        {
            if (m_process && m_process->IsRunning())
            {
                m_process->Terminate(0);
            }

            UnitTest::AllocatorsTestFixture::TearDown();
        }

        AZStd::unique_ptr<TestImpact::Process> m_process;
        static constexpr const TestImpact::ProcessId m_id = 1;
        static constexpr const TestImpact::ReturnCode m_terminateErrorCode = 666;
    };

    TEST_F(ProcessTestFixture, LaunchValidProcess_ProcessReturnsSuccessfully)
    {
        // Given a process launched with a valid binary
        TestImpact::ProcessInfo processInfo(m_id, ValidProcessPath);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process has exited
        m_process->BlockUntilExit();

        // Expect the process to no longer be running
        EXPECT_FALSE(m_process->IsRunning());
    }

    TEST_F(ProcessTestFixture, LaunchInvalidProcessInfo_ThrowsProcessException)
    {
        try
        {
            // Given a process launched with an invalid binary
            TestImpact::ProcessInfo processInfo(m_id, "");

            // Do not expect this code to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ProcessException& e)
        {
            // Except a process exception to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(ProcessTestFixture, LaunchInvalidBinary_ThrowsProcessException)
    {
        try
        {
            // Given a process launched with an ivalid binary
            TestImpact::ProcessInfo processInfo(m_id, "#!#zz:/z/z/z.exe.z@");

            // When the process is attempted launch
            m_process = TestImpact::LaunchProcess(processInfo);

            // Do not expect this code to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ProcessException& e)
        {
            // Except a process exception to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(ProcessTestFixture, GetProcessInfo_ReturnsProcessInfo)
    {
        // Given a process launched with a valid binary and args
        const AZStd::string args = ConstructTestProcessArgs(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(m_id, ValidProcessPath, args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process has exited
        m_process->BlockUntilExit();

        // Expect the retrieved process info to match to process info used to launch the process
        EXPECT_EQ(m_process->GetProcessInfo().GetId(), m_id);
        EXPECT_EQ(m_process->GetProcessInfo().GetProcessPath(), ValidProcessPath);
        EXPECT_EQ(m_process->GetProcessInfo().GetStartupArgs(), args);
    }

    TEST_F(ProcessTestFixture, GetReturnCodeAfterExit_ReturnsArg)
    {
        // Given a process launched with a valid binary and args
        const AZStd::string args = ConstructTestProcessArgs(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(m_id, ValidProcessPath, args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process has exited
        m_process->BlockUntilExit();

        // Expect the process to no longer be running
        EXPECT_FALSE(m_process->IsRunning());

        // Expect the return value to be the process id
        EXPECT_EQ(m_process->GetReturnCode(), m_id);
    }

    TEST_F(ProcessTestFixture, GetReturnCodeInFlight_ReturnsNullOpt)
    {
        // Given a process launched with a valid binary and args to sleep for 100 seconds
        const AZStd::string args = ConstructTestProcessArgs(m_id, LongSleep);
        TestImpact::ProcessInfo processInfo(m_id, ValidProcessPath, args);

        // When the process is running
        m_process = TestImpact::LaunchProcess(processInfo);

        // Expect the return value to be empty
        EXPECT_EQ(m_process->GetReturnCode(), AZStd::nullopt);
    }

    TEST_F(ProcessTestFixture, TerminateWithErrorCode_ReturnsErrorCode)
    {
        // Given a process launched with a valid binary and args to sleep for 100 seconds
        const AZStd::string args = ConstructTestProcessArgs(m_id, LongSleep);
        TestImpact::ProcessInfo processInfo(m_id, ValidProcessPath, args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process is terminated by the client with the specified error code
        m_process->Terminate(m_terminateErrorCode);

        // Expect the return value to be the error code specified in the Terminate call
        EXPECT_EQ(m_process->GetReturnCode(), m_terminateErrorCode);

        // Expect the process to no longer be running
        EXPECT_FALSE(m_process->IsRunning());
    }

    TEST_F(ProcessTestFixture, CheckIsRunningWhilstRunning_ProcessIsRunning)
    {
        // Given a process launched with a valid binary and args to sleep for 100 seconds
        const AZStd::string args = ConstructTestProcessArgs(m_id, LongSleep);
        TestImpact::ProcessInfo processInfo(m_id, ValidProcessPath, args);

        // When the process is running
        m_process = TestImpact::LaunchProcess(processInfo);

        // Expect the process to be running
        EXPECT_TRUE(m_process->IsRunning());
    }

    TEST_F(ProcessTestFixture, CheckIsRunningWhilstNotRunning_ReturnsFalse)
    {
        // Given a process launched with a valid binary
        TestImpact::ProcessInfo processInfo(m_id, ValidProcessPath);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process is terminated by the client
        m_process->Terminate(0);

        // Expect the process to be running
        EXPECT_FALSE(m_process->IsRunning());
    }

    TEST_F(ProcessTestFixture, RedirectStdOut_OutputIsKnownTestProcessOutputString)
    {
        // Given a process launched with a valid binary and standard output redirected to parent
        const AZStd::string args = ConstructTestProcessArgs(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::None,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process exits
        m_process->BlockUntilExit();

        // Expect stdoutput to be rerouted to the parent
        EXPECT_TRUE(m_process->GetProcessInfo().ParentHasStdOutput());

        // Do not expect stdouterr to be rerouted to the parent
        EXPECT_FALSE(m_process->GetProcessInfo().ParentHasStdError());

        // Expect the output to match the known stdout of the child
        EXPECT_EQ(m_process->ConsumeStdOut(), KnownTestProcessOutputString(m_id));
    }

    TEST_F(ProcessTestFixture, RedirectStdErr_OutputIsKnownTestProcessOutputString)
    {
        // Given a process launched with a valid binary and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgs(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::ToParent,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process exits
        m_process->BlockUntilExit();

        // Do not expect stdoutput to be rerouted to the parent
        EXPECT_FALSE(m_process->GetProcessInfo().ParentHasStdOutput());

        // Expect stderror to be rerouted to the parent
        EXPECT_TRUE(m_process->GetProcessInfo().ParentHasStdError());

        // Expect the output to match the known stdout of the child
        EXPECT_EQ(m_process->ConsumeStdErr(), KnownTestProcessErrorString(m_id));
    }

    TEST_F(ProcessTestFixture, RedirectStdOutAndTerminate_OutputIsEmpty)
    {
        // Given a process launched with a valid binary and standard output redirected to parent
        const AZStd::string args = ConstructTestProcessArgs(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::None,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process is terminated
        m_process->Terminate(0);

        // Expect stdoutput to be rerouted to the parent
        EXPECT_TRUE(m_process->GetProcessInfo().ParentHasStdOutput());

        // Do not expect stdouterr to be rerouted to the parent
        EXPECT_FALSE(m_process->GetProcessInfo().ParentHasStdError());

        // Expect the output to be empty
        EXPECT_FALSE(m_process->ConsumeStdOut().has_value());
    }

    TEST_F(ProcessTestFixture, RedirectStdErrAndTerminate_OutputIsEmpty)
    {
        // Given a process launched with a valid binary and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgs(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::ToParent,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process is terminated
        m_process->Terminate(0);

        // Do not expect stdoutput to be rerouted to the parent
        EXPECT_FALSE(m_process->GetProcessInfo().ParentHasStdOutput());

        // Expect stderror to be rerouted to the parent
        EXPECT_TRUE(m_process->GetProcessInfo().ParentHasStdError());

        // Expect the output to be empty
        EXPECT_FALSE(m_process->ConsumeStdErr().has_value());
    }

    TEST_F(ProcessTestFixture, RedirectStdOutAndStdErrorRouting_OutputsAreKnownTestProcessOutputStrings)
    {
        // Given a process launched with a valid binary and both standard output and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgs(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::ToParent,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process exits
        m_process->BlockUntilExit();

        // Expect stdoutput to be rerouted to the parent
        EXPECT_TRUE(m_process->GetProcessInfo().ParentHasStdOutput());

        // Expect stderror to be rerouted to the parent
        EXPECT_TRUE(m_process->GetProcessInfo().ParentHasStdError());

        // Expect the output to match the known stdout and stderr of the child
        EXPECT_EQ(m_process->ConsumeStdOut(), KnownTestProcessOutputString(m_id));
        EXPECT_EQ(m_process->ConsumeStdErr(), KnownTestProcessErrorString(m_id));
    }

    TEST_F(ProcessTestFixture, NoStdOutOrStdErrRedirect_OutputIsEmpty)
    {
        // Given a process launched with a valid binary and both standard output and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgs(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::None,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process exits
        m_process->BlockUntilExit();

        // Do not expect stdoutput to be rerouted to the parent
        EXPECT_FALSE(m_process->GetProcessInfo().ParentHasStdOutput());

        // Do not expect stdouterr to be rerouted to the parent
        EXPECT_FALSE(m_process->GetProcessInfo().ParentHasStdError());

        // Expect the output to to be empty
        EXPECT_FALSE(m_process->ConsumeStdOut().has_value());
        EXPECT_FALSE(m_process->ConsumeStdErr().has_value());
    }

    TEST_F(ProcessTestFixture, LargePipeNoRedirects_OutputsAreEmpty)
    {
        // Given a process outputting large text with no standard output and no standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgsLargeText(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::None,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process exits
        m_process->BlockUntilExit();

        // Expect the output to to be empty
        EXPECT_FALSE(m_process->ConsumeStdOut().has_value());
        EXPECT_FALSE(m_process->ConsumeStdErr().has_value());
    }

    TEST_F(ProcessTestFixture, LargePipeNoRedirectsAndTerminated_OutputsAreEmpty)
    {
        // Given a process outputting large text with no standard output and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgsLargeText(m_id, LongSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::None,
            ValidProcessPath,
            args);

        // When the process is running
        m_process = TestImpact::LaunchProcess(processInfo);

        // Expect the output to to be empty
        EXPECT_FALSE(m_process->ConsumeStdOut().has_value());
        EXPECT_FALSE(m_process->ConsumeStdErr().has_value());

        // When the process is terminated
        m_process->Terminate(0);

        // Expect the output to to be empty
        EXPECT_FALSE(m_process->ConsumeStdOut().has_value());
        EXPECT_FALSE(m_process->ConsumeStdErr().has_value());
    }

    TEST_F(ProcessTestFixture, LargePipeRedirectsAndReadWhilstRunning_OutputsAreEmpty)
    {
        // Given a process outputting large text with no standard output and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgsLargeText(m_id, ShortSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::None,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the outputs are read as the process is running
        while (m_process->IsRunning())
        {
            // Expect the outputs to be empty
            EXPECT_FALSE(m_process->ConsumeStdOut().has_value());
            EXPECT_FALSE(m_process->ConsumeStdErr().has_value());
        }
    }

    TEST_F(ProcessTestFixture, LargePipeRedirectsAndTerminated_OutputsAreEmpty)
    {
        // Given a process outputting large text with both standard output and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgsLargeText(m_id, LongSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::ToParent,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process is terminated
        m_process->Terminate(0);

        // Expect the outputs to be empty
        EXPECT_FALSE(m_process->ConsumeStdOut().has_value());
        EXPECT_FALSE(m_process->ConsumeStdErr().has_value());
    }

    TEST_F(ProcessTestFixture, LargePipeRedirectsAndBlockedUntilExit_OutputsAreLargeTextFileSize)
    {
        // Given a process outputting large text with both standard output and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgsLargeText(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::ToParent,
            ValidProcessPath,
            args);
        m_process = TestImpact::LaunchProcess(processInfo);

        // When the process exits
        m_process->BlockUntilExit();

        // Expect the output lengths to be that of the large text output from the child process
        EXPECT_EQ(m_process->ConsumeStdOut().value().length(), LargeTextSize);
        EXPECT_EQ(m_process->ConsumeStdErr().value().length(), LargeTextSize);
    }

    TEST_F(ProcessTestFixture, LargePipeRedirectsAndReadWhilstRunning_TotalOutputsAreLargeTextFileSize)
    {
        // Given a process outputting large text with both standard output and standard error redirected to parent
        const AZStd::string args = ConstructTestProcessArgsLargeText(m_id, NoSleep);
        TestImpact::ProcessInfo processInfo(
            m_id,
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::ToParent,
            ValidProcessPath, args);
        m_process = TestImpact::LaunchProcess(processInfo);

        size_t stdOutBytes = 0;
        size_t stdErrBytes = 0;
        while (m_process->IsRunning())
        {
            // When the outputs are read as the process is running
            if (auto output = m_process->ConsumeStdOut(); output.has_value())
            {
                stdOutBytes += output.value().length();
            }

            if (auto output = m_process->ConsumeStdErr(); output.has_value())
            {
                stdErrBytes += output.value().length();
            }
        }

        // Expect the output lengths to be that of the large text output from the child process
        EXPECT_EQ(stdOutBytes, LargeTextSize);
        EXPECT_EQ(stdErrBytes, LargeTextSize);

        // Expect the outputs to be empty
        EXPECT_FALSE(m_process->ConsumeStdOut().has_value());
        EXPECT_FALSE(m_process->ConsumeStdErr().has_value());
    }
} // namespace UnitTest
