/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomation/ScriptAutomationBus.h>
#include <ScriptAutomationApplicationFixture.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UnitTest/UnitTest.h>

namespace UnitTest
{
    TEST_F(ScriptAutomationApplicationFixture, GetAutomationContext_FromScriptAutomationInterface_HasCoreMethods)
    {
        CreateApplication({ });

        auto automationSystem = AZ::ScriptAutomation::ScriptAutomationInterface::Get();
        ASSERT_TRUE(automationSystem);

        auto behaviorContext = automationSystem->GetAutomationContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("Print") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("Warning") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("Error") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("ExecuteConsoleCommand") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("IdleFrames") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("IdleSeconds") != behaviorContext->m_methods.end());
    }

    class TrackedAutomationFixture
        : public ScriptAutomationApplicationFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override
        {
            ScriptAutomationApplicationFixture::SetUp();

            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

            m_automationWarnings.set_capacity(0);
            m_automationLogs.set_capacity(0);

            ScriptAutomationApplicationFixture::TearDown();
        }

    protected:
        // AZ::Debug::TraceMessageBus implementation
        bool OnError(const char* window, const char* message) override
        {
            AZ_UNUSED(window);
            AZ_UNUSED(message);
            // Do nothing. Errors will cause the test to fail. If we suppress errors,
            // this function won't get called. We test errors by suppressing them and 
            // then counting how many errors were suppressed.
            return false;
        }

        bool OnWarning(const char* window, const char* message) override
        {
            if (azstricmp(window, "ScriptAutomation") == 0)
            {
                m_automationWarnings.push_back(message);
            }
            return false;
        }

        bool OnPrintf(const char* window, const char* message) override
        {
            if (azstricmp(window, "ScriptAutomation") == 0)
            {
                m_automationLogs.push_back(message);
            }
            return false;
        }

        AZStd::vector<AZStd::string> m_automationWarnings;
        AZStd::vector<AZStd::string> m_automationLogs;
    };

    TEST_F(TrackedAutomationFixture, ScriptCommandLineArgument_UsesPrintMethods_AllOperationsLogged)
    {
        const char* scriptPath = "@gemroot:ScriptAutomation@/Code/Tests/Scripts/print_test.lua";
        auto application = CreateApplication(scriptPath);

        // We expect our "Hello World" error message to be printed by the test script.
        // There should be exactly one error message.
        AZ_TEST_START_TRACE_SUPPRESSION;
        application->RunMainLoop();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        AZStd::string executeScriptLog = AZStd::string::format("Running script '%s'...\n", scriptPath);
        const char* scriptLog = "Script: Hello World\n";

        ASSERT_EQ(m_automationLogs.size(), 2);
        EXPECT_EQ(m_automationLogs[0], executeScriptLog);
        EXPECT_EQ(m_automationLogs[1], scriptLog);

        ASSERT_EQ(m_automationWarnings.size(), 1);
        EXPECT_EQ(m_automationWarnings[0], scriptLog);
    }

    TEST_F(TrackedAutomationFixture, ScriptCommandLineArgument_UsesIdleFramesMethod_AllOperationsLogged)
    {
        auto application = CreateApplication("@gemroot:ScriptAutomation@/Code/Tests/Scripts/idle_five_frames_test.lua");

        application->RunMainLoop();

        ASSERT_EQ(m_automationLogs.size(), 3);
        // first log is the "running script ..." line
        EXPECT_EQ(m_automationLogs[1], "Script: Going to idle for 5 frames\n");
        EXPECT_EQ(m_automationLogs[2], "Script: Idled for 5 frames\n");
    }


    TEST_F(TrackedAutomationFixture, ScriptCommandLineArgument_UsesIdleSecondsMethod_AllOperationsLogged)
    {
        auto application = CreateApplication("@gemroot:ScriptAutomation@/Code/Tests/Scripts/idle_one_second_test.lua");

        application->RunMainLoop();

        ASSERT_EQ(m_automationLogs.size(), 3);
        // first log is the "running script ..." line
        EXPECT_EQ(m_automationLogs[1], "Script: Going to idle for 1 second(s)\n");
        EXPECT_EQ(m_automationLogs[2], "Script: Idled for 1 second(s)\n");
    }
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
