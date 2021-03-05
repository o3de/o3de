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
#include "CrySystem_precompiled.h"

#include <AzTest/AzTest.h>

#include <XConsole.h>

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/functional.h>

#include <Mocks/ISystemMock.h>

namespace UnitTests
{
    class RemoteConsoleMock
        : public IRemoteConsole
    {
    public:
        MOCK_METHOD0(RegisterConsoleVariables, void());
        MOCK_METHOD0(UnregisterConsoleVariables, void());
        MOCK_METHOD0(Start, void());
        MOCK_METHOD0(Stop, void());
        MOCK_CONST_METHOD0(IsStarted, bool());
        MOCK_METHOD1(AddLogMessage, void(const char*));
        MOCK_METHOD1(AddLogWarning, void(const char*));
        MOCK_METHOD1(AddLogError, void(const char*));
        MOCK_METHOD0(Update, void());
        MOCK_METHOD2(RegisterListener, void(IRemoteConsoleListener*, const char*));
        MOCK_METHOD1(UnregisterListener, void(IRemoteConsoleListener*));
    };

    struct TestTraceMessageCapture
        : public AZ::Debug::TraceMessageBus::Handler
    {
        using Callback = AZStd::function<void(const char* window, const char* message)>;
        Callback m_callback;

        TestTraceMessageCapture()
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        ~TestTraceMessageCapture()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        bool OnError(const char* window, const char* message) override
        { 
            if (m_callback)
            {
                m_callback(window, message);
            }
            return false; 
        }
        
        bool OnWarning(const char* window, const char* message)  override
        { 
            if (m_callback)
            {
                m_callback(window, message);
            }
            return false;
        }
    };

    using SystemAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator>;

    struct CommandRegistrationUnitTests
        : public ::testing::Test
        , public SystemAllocatorScope
    {
        CommandRegistrationUnitTests()
        {
            EXPECT_CALL(m_system, GetIRemoteConsole())
                .WillRepeatedly(::testing::Return(&m_remoteConsole));
        }

        void SetUp() override
        {
            SystemAllocatorScope::ActivateAllocators();

            memset(&m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_stubEnv.pSystem = &m_system;
            m_priorEnv = gEnv;
            gEnv = &m_stubEnv;

            // now it safe to set up the console
            m_console = AZStd::make_unique<CXConsole>();
            m_stubEnv.pConsole = m_console.get();

            EXPECT_CALL(m_system, GetIConsole())
                .WillRepeatedly(::testing::Return(m_stubEnv.pConsole));
        }

        void TearDown() override
        {
            m_console.reset();
            gEnv = m_priorEnv;
            SystemAllocatorScope::DeactivateAllocators();
        }

        ::testing::NiceMock<SystemMock> m_system;
        ::testing::NiceMock<RemoteConsoleMock> m_remoteConsole;
        AZStd::unique_ptr<CXConsole> m_console;
        SSystemGlobalEnvironment m_stubEnv;
        SSystemGlobalEnvironment* m_priorEnv = nullptr;
    };

    TEST_F(CommandRegistrationUnitTests, RegisterUnregisterTest)
    {
        using namespace AzFramework;

        {
            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::RegisterCommand, "foo", "", 0, [](const AZStd::vector<AZStd::string_view>&) -> CommandResult
            {
                return CommandResult::Success;
            });
            EXPECT_TRUE(result);
        }

        {
            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::UnregisterCommand, "foo");
            EXPECT_TRUE(result);
        }
    }

    TEST_F(CommandRegistrationUnitTests, RegisterUnregisterNegativeTest)
    {
        using namespace AzFramework;

        // register too many times
        {
            auto fnFoo = [](const AZStd::vector<AZStd::string_view>&) -> CommandResult
            {
                return CommandResult::Success;
            };
            
            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::RegisterCommand, "foo", "", 0, fnFoo);
            EXPECT_TRUE(result);
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::RegisterCommand, "foo", "", 0, fnFoo);
            EXPECT_FALSE(result);
        }

        // unregister too many times
        {
            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::UnregisterCommand, "foo");
            EXPECT_TRUE(result);
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::UnregisterCommand, "foo");
            EXPECT_FALSE(result);
        }

        // a null callback should fail
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::RegisterCommand, "shouldfail", "", 0, nullptr);
            EXPECT_FALSE(result);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // a null identifier should fail
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::RegisterCommand, "", "", 0, nullptr);
            EXPECT_FALSE(result);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }
    }

    TEST_F(CommandRegistrationUnitTests, DoCallback)
    {
        using namespace AzFramework;

        int count = 0;

        {
            auto fnCommand = [&count](const AZStd::vector<AZStd::string_view>&) -> CommandResult
            {
                ++count;
                return CommandResult::Success;
            };

            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::RegisterCommand, "bar", "bar docs", CommandFlags::Development, fnCommand);
            EXPECT_TRUE(result);
        }

        const bool bSilentMode = true;
        const bool bDeferExecution = false;
        m_console->ExecuteString("bar", bSilentMode, bDeferExecution);
        EXPECT_EQ(1, count);

        {
            bool result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::UnregisterCommand, "bar");
            EXPECT_TRUE(result);
        }
    }

    TEST_F(CommandRegistrationUnitTests, DoCallbackNegativeTests)
    {
        using namespace AzFramework;

        bool result = false;
        CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::RegisterCommand, "bar", "", 0, [](const AZStd::vector<AZStd::string_view>& args)
        {
            if (args.size() > 1)
            {
                return CommandResult::ErrorWrongNumberOfArguments;
            }
            return CommandResult::Error;
        });
        EXPECT_TRUE(result);

        const bool bSilentMode = true;
        const bool bDeferExecution = false;

        // general error
        {
            int found = 0;
            TestTraceMessageCapture capture;
            capture.m_callback = [&found](const char* window, const char* message)
            {
                if (azstrnicmp(window, "console", AZ_ARRAY_SIZE("console") - 1) == 0)
                {
                    if (azstrnicmp(message, "Command returned a generic error\n", AZ_ARRAY_SIZE("Command returned a generic error\n") - 1) == 0)
                    {
                        ++found;
                    }
                }
            };
            m_console->ExecuteString("bar", bSilentMode, bDeferExecution);
            EXPECT_EQ(1, found);
        }

        // too many args
        {
            int found = 0;
            TestTraceMessageCapture capture;
            capture.m_callback = [&found](const char* window, const char* message)
            {
                if (azstrnicmp(window, "console", AZ_ARRAY_SIZE("console") - 1) == 0)
                {
                    if (azstrnicmp(message, "Command does not have the right number of arguments (send = 4)\n", AZ_ARRAY_SIZE("Command does not have the right number of arguments (send = 4)\n") - 1) == 0)
                    {
                        ++found;
                    }
                }
            };
            m_console->ExecuteString("bar 1 2 3", bSilentMode, bDeferExecution);
            EXPECT_EQ(1, found);
        }

        // clean up
        {
            result = false;
            CommandRegistrationBus::BroadcastResult(result, &CommandRegistrationBus::Events::UnregisterCommand, "bar");
            EXPECT_TRUE(result);
        }
    }

} // namespace UnitTests


