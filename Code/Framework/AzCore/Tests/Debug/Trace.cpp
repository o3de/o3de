/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/SystemFile.h>

namespace UnitTest
{
    struct TraceTests
        : ScopedAllocatorSetupFixture
        , AZ::Debug::TraceMessageBus::Handler
    {
        bool OnPreAssert(const char*, int, const char*, const char*) override
        {
            m_assert = true;

            return true;
        }

        bool OnPreError(const char*, const char*, int, const char*, const char*) override
        {
            m_error = true;

            return true;
        }

        bool OnPreWarning(const char*, const char*, int, const char*, const char*) override
        {
            m_warning = true;

            return true;
        }

        bool OnPrintf(const char*, const char*) override
        {
            m_printf = true;

            return true;
        }

        void SetUp() override
        {
            BusConnect();

            if (!AZ::Interface<AZ::IConsole>::Get())
            {
                m_console = aznew AZ::Console();
                m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
                AZ::Interface<AZ::IConsole>::Register(m_console);
            }

            AZ_TEST_START_TRACE_SUPPRESSION;
        }

        void TearDown() override
        {
            if (m_console)
            {
                AZ::Interface<AZ::IConsole>::Unregister(m_console);
                delete m_console;
                m_console = nullptr;
            }
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
            BusDisconnect();
        }

        bool m_assert = false;
        bool m_error = false;
        bool m_warning = false;
        bool m_printf = false;
        AZ::Console* m_console{ nullptr };
    };

    TEST_F(TraceTests, Level0)
    {
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("bg_traceLogLevel 0");

        AZ_Assert(false, "test");
        AZ_Error("UnitTest", false, "test");
        AZ_Warning("UnitTest", false, "test");
        AZ_TracePrintf("UnitTest", "test");

        ASSERT_TRUE(m_assert);
        ASSERT_FALSE(m_error);
        ASSERT_FALSE(m_warning);
        ASSERT_FALSE(m_printf);
    }

    TEST_F(TraceTests, Level1)
    {
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("bg_traceLogLevel 1");

        AZ_Assert(false, "test");
        AZ_Error("UnitTest", false, "test");
        AZ_Warning("UnitTest", false, "test");
        AZ_TracePrintf("UnitTest", "test");

        ASSERT_TRUE(m_assert);
        ASSERT_TRUE(m_error);
        ASSERT_FALSE(m_warning);
        ASSERT_FALSE(m_printf);
    }

    TEST_F(TraceTests, Level2)
    {
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("bg_traceLogLevel 2");

        AZ_Assert(false, "test");
        AZ_Error("UnitTest", false, "test");
        AZ_Warning("UnitTest", false, "test");
        AZ_TracePrintf("UnitTest", "test");

        ASSERT_TRUE(m_assert);
        ASSERT_TRUE(m_error);
        ASSERT_TRUE(m_warning);
        ASSERT_FALSE(m_printf);
    }

    TEST_F(TraceTests, Level3)
    {
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("bg_traceLogLevel 3");

        AZ_Assert(false, "test");
        AZ_Error("UnitTest", false, "test");
        AZ_Warning("UnitTest", false, "test");
        AZ_TracePrintf("UnitTest", "test");

        ASSERT_TRUE(m_assert);
        ASSERT_TRUE(m_error);
        ASSERT_TRUE(m_warning);
        ASSERT_TRUE(m_printf);
    }

    TEST_F(TraceTests, RedirectToRawOutputToStderr_DoesNotOutputToStdout)
    {
        constexpr const char* errorMessage = "Test Error\n";
        bool testErrorMessageFound{};
        auto GetStdout = [&testErrorMessageFound, expectedMessage = errorMessage](AZStd::span<const AZStd::byte> capturedBytes)
        {
            if (AZStd::string_view capturedStrView(reinterpret_cast<const char*>(capturedBytes.data()), capturedBytes.size());
                capturedStrView.contains(expectedMessage))
            {
                testErrorMessageFound = true;
            }
        };

        // file descriptor 1 is stdout
        constexpr int StdoutDescriptor = 1;
        AZ::IO::FileDescriptorCapturer capturer(StdoutDescriptor);
        capturer.Start();
        // This message should get captured
        AZ::Debug::Trace::Instance().RawOutput("UnitTest", errorMessage);
        fflush(stdout);
        capturer.Stop(GetStdout);
        // The message should be found
        EXPECT_TRUE(testErrorMessageFound);

        // Reset the error message found boolean back to false
        testErrorMessageFound = false;

        // Redirect the Trace output to stderr
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand({ "bg_redirectrawoutput", "1" });
        AZ::Debug::Trace::Instance().RawOutput("UnitTest", errorMessage);
        fflush(stdout);
        // The message should not be found since trace output should be going to stderr
        EXPECT_FALSE(testErrorMessageFound);

        // Redirect the Trace output back to stdout
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand({ "bg_redirectrawoutput", "0" });
    }

    TEST_F(TraceTests, RedirectToRawOutputNone_DoesNotOutputToStdout_NorStderr)
    {
        constexpr const char* errorMessage = "Test Error\n";
        bool testErrorMessageFound{};
        auto GetOutput = [&testErrorMessageFound, expectedMessage = errorMessage](AZStd::span<const AZStd::byte> capturedBytes)
        {
            if (AZStd::string_view capturedStrView(reinterpret_cast<const char*>(capturedBytes.data()), capturedBytes.size());
                capturedStrView.contains(expectedMessage))
            {
                testErrorMessageFound = true;
            }
        };

        // file descriptor 1 is stdout
        constexpr int StdoutDescriptor = 1;
        // file descriptor 2 is stderr
        constexpr int StderrDescriptor = 2;

        AZ::IO::FileDescriptorCapturer stdoutCapturer(StdoutDescriptor);
        AZ::IO::FileDescriptorCapturer stderrCapturer(StderrDescriptor);
        stdoutCapturer.Start();
        stderrCapturer.Start();
        // This message should get captured
        AZ::Debug::Trace::Instance().RawOutput("UnitTest", errorMessage);
        // flush both stdout and stderr to make sure it capturer is able to get the data
        fflush(stdout);
        fflush(stderr);
        stdoutCapturer.Stop(GetOutput);
        stderrCapturer.Stop(GetOutput);
        // The message should be found
        EXPECT_TRUE(testErrorMessageFound);

        // Reset the error message found boolean back to false
        testErrorMessageFound = false;

        // Redirect the Trace output to None
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand({ "bg_redirectrawoutput", "0" });
        AZ::Debug::Trace::Instance().RawOutput("UnitTest", errorMessage);
        fflush(stdout);
        fflush(stderr);
        // The message should not be found since trace output should be going to stderr
        EXPECT_FALSE(testErrorMessageFound);

        // Redirect the Trace output back to stdout
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand({ "bg_redirectrawoutput", "0" });
    }
}
