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
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <PythonEditorFuncs.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <Mocks/ICVarMock.h>
#include <Mocks/ISystemMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ILogMock.h>
#include <Mocks/IConsoleMock.h>
#include "IEditorMock.h"

namespace EditorPythonBindingsUnitTests
{
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::Invoke;
    using ::testing::A;
    using ::testing::_;

    struct MockEditor final
    {
        ~MockEditor()
        {
            SetIEditor(nullptr);
        }

        template <typename T>
        void PrepareSetCVar(int cvarType, AZStd::function<void(const T)> func)
        {
            m_cvarType = cvarType;
            ON_CALL(m_cvarMock, GetType()).WillByDefault(Return(m_cvarType));
            ON_CALL(m_cvarMock, Set(A<T>())).WillByDefault(Invoke(func));
            ON_CALL(m_console, GetCVar(_)).WillByDefault(Return(&m_cvarMock));
            ON_CALL(m_system, GetIConsole()).WillByDefault(Return(&m_console));
            ON_CALL(m_editorMock, GetSystem()).WillByDefault(Return(&m_system));
            SetIEditor(&m_editorMock);
        }

        void PrepareGetCVarString(const char* value)
        {
            ON_CALL(m_cvarMock, GetString()).WillByDefault(Return(value));
            ON_CALL(m_console, GetCVar(_)).WillByDefault(Return(&m_cvarMock));
            ON_CALL(m_system, GetIConsole()).WillByDefault(Return(&m_console));
            ON_CALL(m_editorMock, GetSystem()).WillByDefault(Return(&m_system));
            SetIEditor(&m_editorMock);
        }

        void PrepareGetIConsole()
        {
            ON_CALL(m_system, GetIConsole()).WillByDefault(Return(&m_console));
            ON_CALL(m_editorMock, GetSystem()).WillByDefault(Return(&m_system));
            SetIEditor(&m_editorMock);
        }

        int m_cvarType = CVAR_INT;
        ::testing::NiceMock<CEditorMock> m_editorMock;
        ::testing::NiceMock<ConsoleMock> m_console;
        ::testing::NiceMock<SystemMock> m_system;
        ::testing::NiceMock<CVarMock> m_cvarMock;
    };

    class EditorPythonBindingsFixture
        : public testing::Test
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            appDesc.m_enableDrilling = false;

            m_app.Start(appDesc);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            m_app.RegisterComponentDescriptor(AzToolsFramework::PythonEditorFuncsHandler::CreateDescriptor());
            // Disable saving global user settings to prevent failure due to detecting file updates
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        using TestEditorUtilityCommandFunc = AZStd::function<void(AZ::BehaviorContext* context)>;
        void RunEditorUtilityCommandTest(TestEditorUtilityCommandFunc func)
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
            ASSERT_TRUE(behaviorContext);
            func(behaviorContext);
        }
    };

    TEST_F(EditorPythonBindingsFixture, EditorUtilityCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("get_cvar") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_cvar_string") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_cvar_integer") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_cvar_float") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("run_console") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("enter_game_mode") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("is_in_game_mode") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("exit_game_mode") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("run_file") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("run_file_parameters") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("execute_command") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("message_box") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("message_box_yes_no") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("message_box_ok") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("edit_box") != behaviorContext->m_methods.end());

        // Blocked by LY-101816
        //EXPECT_TRUE(behaviorContext->m_methods.find("edit_box_check_data_type") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("open_file_box") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_axis_constraint") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_axis_constraint") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_pak_from_file") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("log") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("undo") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("redo") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("draw_label") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("combo_box") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("crash") != behaviorContext->m_methods.end());
    }

    TEST_F(EditorPythonBindingsFixture, EditorPythonBindingsComponent_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        auto itPythonEditorBus = behaviorContext->m_ebuses.find("PythonEditorBus");
        if (itPythonEditorBus != behaviorContext->m_ebuses.end())
        {
            AZ::BehaviorEBus* behaviorBus = itPythonEditorBus->second;
            EXPECT_TRUE(behaviorBus->m_events.find("GetCVar") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetCVar") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetCVarFromString") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetCVarFromInteger") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetCVarFromFloat") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("PyRunConsole") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("EnterGameMode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("IsInGameMode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("ExitGameMode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("EnterSimulationMode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("IsInSimulationMode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("ExitSimulationMode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("RunLua") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("RunFile") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("RunFileParameters") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("ExecuteCommand") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("MessageBoxOkCancel") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("MessageBoxYesNo") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("MessageBoxOk") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("EditBox") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("EditBoxCheckDataType") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("OpenFileBox") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetAxisConstraint") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetAxisConstraint") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetPakFromFile") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("Log") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("Undo") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("Redo") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("DrawLabel") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("ComboBox") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetHidemaskAll") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetHidemaskNone") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetHidemaskInvert") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetHidemask") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetHidemask") != behaviorBus->m_events.end());
        }
    }

    TEST_F(EditorPythonBindingsFixture, EditorUtilityCommands_set_cvar_integer)
    {
        RunEditorUtilityCommandTest([](AZ::BehaviorContext* context)
        {
            int testInt = -1;
            MockEditor mockEditor;
            mockEditor.PrepareSetCVar<int>(CVAR_INT, [&testInt](const int value) { testInt = value; });

            const char* testCvar = "test.cvar.int";
            int intArg = 1;
            AZStd::array<AZ::BehaviorValueParameter, 2> args;
            args[0].Set(&testCvar);
            args[1].Set(&intArg);

            context->m_methods.find("set_cvar_integer")->second->Call(args.begin(), static_cast<unsigned int>(args.size()));
            EXPECT_EQ(1, testInt);
        });
    }

    TEST_F(EditorPythonBindingsFixture, EditorUtilityCommands_set_cvar_float)
    {
        RunEditorUtilityCommandTest([](AZ::BehaviorContext* context)
        {
            float testFloat = 0.0f;
            MockEditor mockEditor;
            mockEditor.PrepareSetCVar<float>(CVAR_FLOAT, [&testFloat](const float value) { testFloat = value; });

            const char* testCvar = "test.cvar.float";
            float input = 1.234f;
            AZStd::array<AZ::BehaviorValueParameter, 2> args;
            args[0].Set(&testCvar);
            args[1].Set(&input);

            context->m_methods.find("set_cvar_float")->second->Call(args.begin(), static_cast<unsigned int>(args.size()));
            EXPECT_FLOAT_EQ(1.234f, testFloat);
        });
    }

    TEST_F(EditorPythonBindingsFixture, EditorUtilityCommands_set_cvar_string)
    {
        RunEditorUtilityCommandTest([](AZ::BehaviorContext* context)
        {
            AZStd::string testString;
            MockEditor mockEditor;
            mockEditor.PrepareSetCVar<const char*>(CVAR_STRING, [&testString](const char* value) { testString = value; });

            const char* testCvar = "test.cvar.string";
            const char* input = "testvalue";
            AZStd::array<AZ::BehaviorValueParameter, 2> args;
            args[0].Set(&testCvar);
            args[1].Set(&input);

            context->m_methods.find("set_cvar_string")->second->Call(args.begin(), static_cast<unsigned int>(args.size()));
            EXPECT_STREQ("testvalue", testString.c_str());
        });
    }

    TEST_F(EditorPythonBindingsFixture, EditorUtilityCommands_get_cvar)
    {
        RunEditorUtilityCommandTest([](AZ::BehaviorContext* context)
        {
            MockEditor mockEditor;
            mockEditor.PrepareGetCVarString("atestvalue");

            const char* testCvar = "test.cvar.string";
            AZStd::array<AZ::BehaviorValueParameter, 1> args;
            args[0].Set(&testCvar);

            const char* data;
            AZ::BehaviorObject obj;
            obj.m_typeId = azrtti_typeid<const char*>();
            obj.m_address = &data;
    
            AZ::BehaviorValueParameter result;
            result.Set(&obj);

            context->m_methods.find("get_cvar")->second->Call(args.begin(), static_cast<unsigned int>(args.size()), &result);
            EXPECT_STREQ("atestvalue", reinterpret_cast<const char*>(obj.m_address));
        });
    }

    TEST_F(EditorPythonBindingsFixture, EditorUtilityCommands_run_console)
    {
        RunEditorUtilityCommandTest([](AZ::BehaviorContext* context)
        {
            AZStd::string data;
            MockEditor mockEditor;
            mockEditor.PrepareGetIConsole();
            ON_CALL(mockEditor.m_console, ExecuteString(_,_,_)).WillByDefault(Invoke([&data](const char* command, const bool, const bool) { data = command; }));

            const char* testCvar = "enable_feature game.sim";
            AZStd::array<AZ::BehaviorValueParameter, 1> args;
            args[0].Set(&testCvar);

            context->m_methods.find("run_console")->second->Call(args.begin(), static_cast<unsigned int>(args.size()));
            EXPECT_STREQ(testCvar, data.c_str());
        });
    }
}

