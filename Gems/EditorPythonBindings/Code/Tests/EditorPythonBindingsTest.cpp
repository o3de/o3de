/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>

#include "PythonTraceMessageSink.h"
#include "PythonTestingUtility.h"
#include <Source/PythonSystemComponent.h>
#include <EditorPythonBindings/EditorPythonBindingsBus.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

namespace UnitTest
{
    struct EditorPythonBindingsNotificationBusSink final
        : public EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler
    {
        EditorPythonBindingsNotificationBusSink()
        {
            EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusConnect();
        }

        ~EditorPythonBindingsNotificationBusSink()
        {
            EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        // handles EditorPythonBindingsNotificationBus

        int m_OnPreInitializeCount = 0;
        int m_OnPostInitializeCount = 0;
        int m_OnPreFinalizeCount = 0;
        int m_OnPostFinalizeCount = 0;

        void OnPreInitialize() override { m_OnPreInitializeCount++;  }
        void OnPostInitialize() override { m_OnPostInitializeCount++; }
        void OnPreFinalize() override { m_OnPreFinalizeCount++; }
        void OnPostFinalize() override { m_OnPostFinalizeCount++; }

    };

    class EditorPythonBindingsTest
        : public PythonTestingFixture
    {
    public:
        PythonTraceMessageSink m_testSink;
        EditorPythonBindingsNotificationBusSink m_notificationSink;

        void SetUp() override
        {
            PythonTestingFixture::SetUp();
            m_app.RegisterComponentDescriptor(EditorPythonBindings::PythonSystemComponent::CreateDescriptor());
        }

        void TearDown() override
        {
            // clearing up memory
            m_notificationSink = EditorPythonBindingsNotificationBusSink();
            m_testSink.CleanUp();

            // shutdown time!
            PythonTestingFixture::TearDown();
        }
    };

    TEST_F(EditorPythonBindingsTest, FireUpPythonVM)
    {
        enum class LogTypes
        {
            Skip = 0,
            General,
            RedirectOutputInstalled
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "RedirectOutput installed"))
                {
                    return static_cast<int>(LogTypes::RedirectOutputInstalled);
                }
                return static_cast<int>(LogTypes::General);
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        e.Deactivate();

        EXPECT_GT(m_testSink.m_evaluationMap[(int)LogTypes::General], 0);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::RedirectOutputInstalled], 1);
        EXPECT_EQ(m_notificationSink.m_OnPreInitializeCount, 1);
        EXPECT_EQ(m_notificationSink.m_OnPostFinalizeCount, 1);
        EXPECT_EQ(m_notificationSink.m_OnPreFinalizeCount, 1);
        EXPECT_EQ(m_notificationSink.m_OnPostFinalizeCount, 1);
    }

    TEST_F(EditorPythonBindingsTest, RunScriptTextBuffer)
    {
        enum class LogTypes
        {
            Skip = 0,
            ScriptWorked
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "EditorPythonBindingsTest_RunScriptTextBuffer"))
                {
                    return static_cast<int>(LogTypes::ScriptWorked);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        const char* script = 
R"(
import sys
print ('EditorPythonBindingsTest_RunScriptTextBuffer')
)";
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script, false);

        e.Deactivate();

        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::ScriptWorked], 1);
    }

    TEST_F(EditorPythonBindingsTest, RunScriptTextBufferAndPrint)
    {
        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        AZStd::string capturedOutput;
        m_testSink.m_evaluateMessage = [&capturedOutput](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                capturedOutput.append(message);
            }
            return 0;
        };

        // Expressions should log their result
        // Any other statement shouldn't log anything

        capturedOutput.clear();
        const char* script = "5+5";
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script, true);
        EXPECT_EQ(capturedOutput, "10\n");

        capturedOutput.clear();
        script = 
R"(
import sys
sys.version
)";
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script, true);
        EXPECT_EQ(capturedOutput, "");

        capturedOutput.clear();
        script = "variable = 'test'";
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script, true);
        EXPECT_EQ(capturedOutput, "");

        capturedOutput.clear();
        script = "variable";
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script, true);
        EXPECT_EQ(capturedOutput, "test\n");
    }

    TEST_F(EditorPythonBindingsTest, RunScriptFile)
    {
        enum class LogTypes
        {
            Skip = 0,
            RanFromFile
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                AZStd::string_view m(message);
                if (AzFramework::StringFunc::Equal(message, "EditorPythonBindingsTest_RunScriptFile"))
                {
                    return static_cast<int>(LogTypes::RanFromFile);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::IO::Path filename = AZ::IO::PathView(m_engineRoot / "Gems" / "EditorPythonBindings" / "Code" / "Tests" / "EditorPythonBindingsTest.py");

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilename, filename.c_str());

        e.Deactivate();

        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::RanFromFile], 1);
    }

    TEST_F(EditorPythonBindingsTest, RunScriptFileWithArgs)
    {
        enum class LogTypes
        {
            Skip = 0,
            RanFromFile,
            NumArgsCorrect,
            ScriptNameCorrect,
            Arg1Correct,
            Arg2Correct,
            Arg3Correct
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                AZStd::string_view m(message);
                if (AzFramework::StringFunc::Equal(message, "EditorPythonBindingsTestWithArgs_RunScriptFile"))
                {
                    return static_cast<int>(LogTypes::RanFromFile);
                }
                else if (AzFramework::StringFunc::Equal(message, "num args: 4"))
                {
                    return static_cast<int>(LogTypes::NumArgsCorrect);
                }
                else if (AzFramework::StringFunc::Equal(message, "script name: EditorPythonBindingsTestWithArgs.py"))
                {
                    return static_cast<int>(LogTypes::ScriptNameCorrect);
                }
                else if (AzFramework::StringFunc::Equal(message, "arg 1: arg1"))
                {
                    return static_cast<int>(LogTypes::Arg1Correct);
                }
                else if (AzFramework::StringFunc::Equal(message, "arg 2: 2"))
                {
                    return static_cast<int>(LogTypes::Arg2Correct);
                }
                else if (AzFramework::StringFunc::Equal(message, "arg 3: arg3"))
                {
                    return static_cast<int>(LogTypes::Arg3Correct);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::IO::Path filename = AZ::IO::PathView(m_engineRoot / "Gems" / "EditorPythonBindings" / "Code" / "Tests" / "EditorPythonBindingsTestWithArgs.py");

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        AZStd::vector<AZStd::string_view> args;
        args.push_back("arg1");
        args.push_back("2");
        args.push_back("arg3");

        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, filename.c_str(), args);

        e.Deactivate();

        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::RanFromFile], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::NumArgsCorrect], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::ScriptNameCorrect], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::Arg1Correct], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::Arg2Correct], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::Arg3Correct], 1);
    }

    //
    // Tests that makes sure that basic Python libraries can be loaded
    //
    class EditorPythonBindingsLibraryTest
        : public PythonTestingFixture
    {
    public:
        PythonTraceMessageSink m_testSink;
        EditorPythonBindingsNotificationBusSink m_notificationSink;

        void SetUp() override
        {
            PythonTestingFixture::SetUp();

            auto registry = AZ::SettingsRegistry::Get();
            auto projectPathKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)
                + "/project_path";
            registry->Set(projectPathKey, "AutomatedTesting");
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            m_app.RegisterComponentDescriptor(EditorPythonBindings::PythonSystemComponent::CreateDescriptor());
        }

        void TearDown() override
        {
            // clearing up memory
            m_notificationSink = EditorPythonBindingsNotificationBusSink();
            m_testSink.CleanUp();

            // shutdown time!
            PythonTestingFixture::TearDown();
        }

        void DoLibraryTest(const char* libName)
        {
            bool executedLine = false;

            m_testSink.m_evaluateMessage = [&executedLine](const char* window, const char* message)
            {
                if (AzFramework::StringFunc::Equal(window, "python"))
                {
                    if (AzFramework::StringFunc::Equal(message, "python_vm_loaded_lib"))
                    {
                        executedLine = true;
                    }
                }
                return false;
            };

            AZ::Entity e;
            e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
            e.Init();
            e.Activate();

            try
            {
                SimulateEditorBecomingInitialized();

                const bool printResult = false;
                AZStd::string script(AZStd::string::format("import %s\nprint ('python_vm_loaded_lib')", libName));
                AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                    &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString,
                    script.c_str(),
                    printResult);
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
            }
            e.Deactivate();

            EXPECT_TRUE(executedLine);
        }
    };

    // This test makes sure that some of the expected built-in libraries
    // Are present in the version of python we are using (the ones most problematic for building)
    TEST_F(EditorPythonBindingsTest, VerifyExpectedLibrariesPresent)
    {
        enum class LogTypes
        {
            Skip = 0,
            ScriptWorked
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "EditorPythonBindingsTest_VerifyExpectedLibrariesPresent"))
                {
                    return static_cast<int>(LogTypes::ScriptWorked);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        const char* script = 
            R"(
import sys
import sqlite3
import ssl
print ('EditorPythonBindingsTest_VerifyExpectedLibrariesPresent')
)";
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script, true);

        e.Deactivate();

        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::ScriptWorked], 1);
    }

    TEST_F(EditorPythonBindingsLibraryTest, PythonVMLoads_sys_Works)
    {
        DoLibraryTest("sys");
    }

    TEST_F(EditorPythonBindingsLibraryTest, PythonVMLoads_ctypes_Works)
    {
        DoLibraryTest("ctypes");
    }

    TEST_F(EditorPythonBindingsLibraryTest, PythonVMLoads_bz2_Works)
    {
        DoLibraryTest("bz2");
    }

    TEST_F(EditorPythonBindingsLibraryTest, PythonVMLoads_lzma_Works)
    {
        DoLibraryTest("lzma");
    }

    TEST_F(EditorPythonBindingsLibraryTest, PythonVMLoads_socket_Works)
    {
        DoLibraryTest("socket");
    }

    TEST_F(EditorPythonBindingsLibraryTest, PythonVMLoads_sqlite3_Works)
    {
        DoLibraryTest("sqlite3");
    }

    TEST_F(EditorPythonBindingsLibraryTest, PythonVMLoads_ssl_Works)
    {
        DoLibraryTest("ssl");
    }

    // This library lives in Editor/Scripts. We're testing that our sys.path extension code in ExtendSysPath works as expected
    TEST_F(EditorPythonBindingsLibraryTest, PythonVMLoads_SysPathExtendedToGemScripts_EditorPythonBindingsValidaitonFound)
    {
        DoLibraryTest("editor_script_validation");
    }
}


AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

