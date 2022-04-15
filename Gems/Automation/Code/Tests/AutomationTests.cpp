/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AutomationApplicationFixture.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UnitTest/UnitTest.h>

namespace UnitTest
{
    // there is an issue when running multiple script tests where the second test will
    // consistently deadlock on loading the script asset, so until that can get sorted
    // only one script test can be enabled.
    #define ScriptTest_Print 1
    #define ScriptTest_IdleFrames 2
    #define ScriptTest_IdleSeconds 3

    #define ENABLE_SCRIPT_TEST ScriptTest_Print


    TEST_F(AutomationApplicationFixture, GetAutomationContext_FromAutomationInterface_HasCoreMethods)
    {
        CreateApplication({ });

        auto automationSystem = Automation::AutomationInterface::Get();
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
        : public AutomationApplicationFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override
        {
            AutomationApplicationFixture::SetUp();

            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

            m_automationErrors.set_capacity(0);
            m_automationWarnings.set_capacity(0);
            m_automationLogs.set_capacity(0);

            AutomationApplicationFixture::TearDown();
        }

    protected:
        // AZ::Debug::TraceMessageBus implementation
        bool OnError(const char* window, const char* message) override
        {
            if (azstricmp(window, "Automation") == 0)
            {
                m_automationErrors.push_back(message);
            }
            return false;
        }

        bool OnWarning(const char* window, const char* message) override
        {
            if (azstricmp(window, "Automation") == 0)
            {
                m_automationWarnings.push_back(message);
            }
            return false;
        }

        bool OnPrintf(const char* window, const char* message) override
        {
            if (azstricmp(window, "Automation") == 0)
            {
                m_automationLogs.push_back(message);
            }
            return false;
        }


        AZStd::vector<AZStd::string> m_automationErrors;
        AZStd::vector<AZStd::string> m_automationWarnings;
        AZStd::vector<AZStd::string> m_automationLogs;
    };

#if ENABLE_SCRIPT_TEST == ScriptTest_Print
    TEST_F(TrackedAutomationFixture, ScriptCommandLineArgument_UsesPrintMethods_AllOperationsLogged)
#else
    TEST_F(TrackedAutomationFixture, DISABLED_ScriptCommandLineArgument_UsesPrintMethods_AllOperationsLogged)
#endif
    {
        const char* scriptPath = "@gemroot:Automation@/Code/Tests/Scripts/PrintTest.lua";
        auto application = CreateApplication(scriptPath);

        application->RunMainLoop();

        AZStd::string executeScriptLog = AZStd::string::format("Running script '%s'...\n", scriptPath);
        const char* scriptLog = "Script: Hello World\n";

        ASSERT_EQ(m_automationLogs.size(), 2);
        EXPECT_STREQ(m_automationLogs[0].c_str(), executeScriptLog.c_str());
        EXPECT_STREQ(m_automationLogs[1].c_str(), scriptLog);

        ASSERT_EQ(m_automationWarnings.size(), 1);
        EXPECT_STREQ(m_automationWarnings[0].c_str(), scriptLog);

        //ASSERT_EQ(m_automationErrors.size(), 1);
        //EXPECT_STREQ(m_automationErrors[0].c_str(), scriptLog);
    }

#if ENABLE_SCRIPT_TEST == ScriptTest_IdleFrames
    TEST_F(TrackedAutomationFixture, ScriptCommandLineArgument_UsesIdleFramesMethod_AllOperationsLogged)
#else
    TEST_F(TrackedAutomationFixture, DISABLED_ScriptCommandLineArgument_UsesIdleFramesMethod_AllOperationsLogged)
#endif
    {
        auto application = CreateApplication("@gemroot:Automation@/Code/Tests/Scripts/IdleFiveFramesTest.lua");

        application->RunMainLoop();

        ASSERT_EQ(m_automationLogs.size(), 3);
        // first log is the "running script ..." line
        EXPECT_STREQ(m_automationLogs[1].c_str(), "Script: Going to idle for 5 frames\n");
        EXPECT_STREQ(m_automationLogs[2].c_str(), "Script: Idled for 5 frames\n");
    }


#if ENABLE_SCRIPT_TEST == ScriptTest_IdleSeconds
    TEST_F(TrackedAutomationFixture, ScriptCommandLineArgument_UsesIdleSecondsMethod_AllOperationsLogged)
#else
    TEST_F(TrackedAutomationFixture, DISABLED_ScriptCommandLineArgument_UsesIdleSecondsMethod_AllOperationsLogged)
#endif
    {
        auto application = CreateApplication("@gemroot:Automation@/Code/Tests/Scripts/IdleOneSecondTest.lua");

        application->RunMainLoop();

        ASSERT_EQ(m_automationLogs.size(), 3);
        // first log is the "running script ..." line
        EXPECT_STREQ(m_automationLogs[1].c_str(), "Script: Going to idle for 1 second(s)\n");
        EXPECT_STREQ(m_automationLogs[2].c_str(), "Script: Idled for 1 second(s)\n");
    }
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
