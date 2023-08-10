/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/Component/TickBus.h>
#include <MainWindow.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include "CryEdit.h"

namespace CryEditPythonBindingsUnitTests
{

    class CryEditPythonBindingsFixture
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(appDesc, startupParameters);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            m_app.RegisterComponentDescriptor(AzToolsFramework::CryEditPythonHandler::CreateDescriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(CryEditPythonBindingsFixture, CryEditCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("open_level") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("open_level_no_prompt") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("create_level") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("create_level_no_prompt") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_game_folder") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_current_level_name") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_current_level_path") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("load_all_plugins") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_current_view_position") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_current_view_rotation") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_current_view_position") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_current_view_rotation") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("export_to_engine") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_config_platform") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_result_to_success") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_result_to_failure") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("idle_enable") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("is_idle_enabled") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("idle_is_enabled") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("idle_wait") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("idle_wait_frames") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("launch_lua_editor") != behaviorContext->m_methods.end());
    }

    TEST_F(CryEditPythonBindingsFixture, CryEditCheckoutDialogCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("enable_for_all") != behaviorContext->m_methods.end());
    }

    TEST_F(CryEditPythonBindingsFixture, CryEditPython_IdleWaitFramesWorks)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
        ASSERT_TRUE(behaviorContext);

        unsigned int numTicks = 0;
        QEventLoop loop;
        QTimer timer;
        loop.connect(&timer, &QTimer::timeout, [&numTicks]()
        {
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.01f, AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));
            ++numTicks;
        });
        timer.start(100);

        const unsigned int framesToWait = 5;
        AZStd::array<AZ::BehaviorArgument, 1> args;
        args[0].Set(&framesToWait);
        behaviorContext->m_methods.find("idle_wait_frames")->second->Call(args.begin(), static_cast<unsigned int>(args.size()));
        loop.disconnect(&timer);
        timer.stop();
        EXPECT_EQ(numTicks, framesToWait);
    }
}
