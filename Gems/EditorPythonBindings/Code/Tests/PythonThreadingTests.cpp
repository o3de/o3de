/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include "PythonTraceMessageSink.h"
#include "PythonTestingUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // behavior

    struct PythonThreadNotifications
        : public AZ::EBusTraits
    {
        virtual AZ::s64 OnNotification(AZ::s64 value) = 0;
    };
    using PythonThreadNotificationBus = AZ::EBus<PythonThreadNotifications>;

    struct PythonThreadNotificationBusHandler final
        : public PythonThreadNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
        AZ_EBUS_BEHAVIOR_BINDER(PythonThreadNotificationBusHandler, "{CADEF35D-D88C-4DE0-B5FC-A88D383C124E}", AZ::SystemAllocator,
            OnNotification);

        virtual ~PythonThreadNotificationBusHandler() = default;

        AZ::s64 OnNotification(AZ::s64 value) override
        {
            AZ::s64 result = 0;
            CallResult(result, FN_OnNotification, value);
            return result;
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PythonThreadNotificationBus>("PythonThreadNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Handler<PythonThreadNotificationBusHandler>()
                    ->Event("OnNotification", &PythonThreadNotificationBus::Events::OnNotification)
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonThreadingTest
        : public PythonTestingFixture
    {
        PythonTraceMessageSink m_testSink;

        void SetUp() override
        {
            PythonTestingFixture::SetUp();
            PythonTestingFixture::RegisterComponentDescriptors();
        }

        void TearDown() override
        {
            // clearing up memory
            m_testSink.CleanUp();
            PythonTestingFixture::TearDown();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // tests

    TEST_F(PythonThreadingTest, PythonInterface_ThreadLogic_Runs)
    {
        enum class LogTypes
        {
            Skip = 0,
            RanInThread
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "RanInThread"))
                {
                    return aznumeric_cast<int>(LogTypes::RanInThread);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        PythonThreadNotificationBusHandler pythonThreadNotificationBusHandler;
        pythonThreadNotificationBusHandler.Reflect(m_app.GetSerializeContext());
        pythonThreadNotificationBusHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            // prepare handler on this thread
            const AZStd::string_view script =
                "import azlmbr.test\n"
                "\n"
                "def on_notification(args) :\n"
                "    value = args[0] + 2\n"
                "    print('RanInThread')\n"
                "    return value\n"
                "\n"
                "handler = azlmbr.test.PythonThreadNotificationBusHandler()\n"
                "handler.connect()\n"
                "handler.add_callback('OnNotification', on_notification)\n";

            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script, false /*printResult*/);

            // start thread; in thread issue notification
            auto threadCallback = []()
            {
                AZ::s64 result = 0;
                PythonThreadNotificationBus::BroadcastResult(result, &PythonThreadNotificationBus::Events::OnNotification, 40);

                EXPECT_EQ(42, result);
            };
            AZStd::thread theThread(threadCallback);
            theThread.join();
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed during thread test with %s", e.what());
        }

        e.Deactivate();
        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::RanInThread)]);
    }

    TEST_F(PythonThreadingTest, PythonInterface_ThreadLogic_HandlesPythonException)
    {
        PythonThreadNotificationBusHandler pythonThreadNotificationBusHandler;
        pythonThreadNotificationBusHandler.Reflect(m_app.GetSerializeContext());
        pythonThreadNotificationBusHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            AZ_TEST_START_TRACE_SUPPRESSION;

            // prepare handler on this thread, but will throw a Python exception 
            pybind11::exec(R"(
                import azlmbr.test

                def on_notification(args):
                    raise NotImplementedError("boom")

                handler = azlmbr.test.PythonThreadNotificationBusHandler()
                handler.connect()
                handler.add_callback('OnNotification', on_notification)
                )");

            // start thread; in thread issue notification
            auto threadCallback = []()
            {
                AZ::s64 result = 0;
                PythonThreadNotificationBus::BroadcastResult(result, &PythonThreadNotificationBus::Events::OnNotification, 40);

                EXPECT_EQ(0, result);
            };
            AZStd::thread theThread(threadCallback);
            theThread.join();

            // the Python script above raises an exception which causes two AZ_Error() message lines:
            // "Python callback threw an exception NotImplementedError : boom At : <string>(6) : on_notification"
            // "Python callback threw an exception TypeError : 'NoneType' object is not callable At : <string>(7) : on_notification"
            AZ_TEST_STOP_TRACE_SUPPRESSION(2);
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed during thread test with %s", e.what());
        }

        e.Deactivate();
    }

    TEST_F(PythonThreadingTest, PythonInterface_DebugTrace_CallsOnTick)
    {
        enum class LogTypes
        {
            Skip = 0,
            OnPrewarning
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "OnPrewarning"))
                {
                    return aznumeric_cast<int>(LogTypes::OnPrewarning);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            // prepare handler on this thread
            pybind11::exec(R"(
                import azlmbr.debug

                def on_prewarning(args):
                    print ('OnPrewarning: ' + args[0])

                handler = azlmbr.debug.TraceMessageBusHandler()
                handler.connect()
                handler.add_callback('OnPreWarning', on_prewarning)
                )");

            const size_t numWarnings = 64;
            auto doWarning = []()
            {
                AZ_Warning("PythonThreadingTest", false, "This is a warning message");
            };

            // start threads. In thread issue a warning.
            AZStd::vector<AZStd::thread> threads;
            threads.reserve(numWarnings);
            for (size_t i = 0; i < numWarnings; ++i)
            {
                threads.emplace_back(doWarning);
            }
            for (AZStd::thread& thread : threads)
            {
                thread.join();
            }

            // No prewarning calls should have happened because all of them were queued
            EXPECT_EQ(0, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::OnPrewarning)]);

            // Do one tick
            const float timeOneFrameSeconds = 0.016f; //approx 60 fps
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
                timeOneFrameSeconds,
                AZ::ScriptTimePoint(AZStd::chrono::system_clock::now()));

            // After one tick all the queued calls should have been processed
            EXPECT_EQ(numWarnings, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::OnPrewarning)]);
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed during thread test with %s", e.what());
        }

        e.Deactivate();
    }
}
