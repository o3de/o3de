/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/PythonSystemComponent.h>

#include "PythonTestingUtility.h"
#include "PythonTraceMessageSink.h"
#include <EditorPythonBindings/PythonCommon.h>
#include <Source/PythonTypeCasters.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <EditorPythonBindings/EditorPythonBindingsBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

// an example converter for an "AZ type"
namespace TestTypes
{
    void RegisterAzEntityId(pybind11::module m)
    {
        auto classEntityId = pybind11::class_<AZ::EntityId>(m, AZ::AzTypeInfo<AZ::EntityId>::Name());
        classEntityId.def(pybind11::init<AZ::u64>());
        classEntityId.def("isValid", &AZ::EntityId::IsValid);
        classEntityId.def("setInvalid", &AZ::EntityId::SetInvalid);
        classEntityId.def_property_readonly("id", [](const AZ::EntityId& e) { return static_cast<AZ::u64>(e); });
        classEntityId.def("__repr__", &AZ::EntityId::ToString);
    }
}

// this is called the first time a Python script "import azlmbrtest"
PYBIND11_EMBEDDED_MODULE(azlmbrtest, m)
{
    EditorPythonBindings::EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindings::EditorPythonBindingsNotificationBus::Events::OnImportModule, m.ptr());
    TestTypes::RegisterAzEntityId(m);
}

namespace UnitTest
{
    struct MyPythonBindings final
        : public EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler
    {
        int m_onImportModuleCount = 0;

        MyPythonBindings()
        {
            EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusConnect();
        }

        ~MyPythonBindings()
        {
            EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
        }

        static long DoAdd(int lhs, int rhs)
        {
            return lhs + rhs;
        }

        static void AZPrintf([[maybe_unused]] const AZStd::string& message)
        {
            AZ_TracePrintf("python", "%s", message.c_str());
        }

        void ImportTestSubModule(pybind11::module module)
        {
            pybind11::module subModule = module.def_submodule("tester", "A submodule for 'test'");
            subModule.def("add", &DoAdd);
            subModule.def("print", &AZPrintf);
        }

        void OnImportModule(PyObject* module) override
        {
            pybind11::module m = pybind11::cast<pybind11::module>(module);
            std::string szName = pybind11::cast<std::string>(m.attr("__name__"));
            if (szName == "azlmbrtest")
            {
                m_onImportModuleCount++;
                ImportTestSubModule(m);
            }
        }
    };

    class PythonBindingLibTest
        : public PythonTestingFixture
    {
    protected:

        void SetUp() override
        {
            PythonTestingFixture::SetUp();
            RegisterComponentDescriptors();
        }

        void TearDown() override
        {
            PythonTestingFixture::TearDown();
        }
    };

    TEST_F(PythonBindingLibTest, ImportBaseModule)
    {
        AZ::Entity entity;
        entity.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        entity.Init();
        entity.Activate();

        SimulateEditorBecomingInitialized();

        {
            MyPythonBindings pythonBindings;
            pybind11::module::import("azlmbrtest");
            EXPECT_EQ(pythonBindings.m_onImportModuleCount, 1);
        }

        entity.Deactivate();
    }

    TEST_F(PythonBindingLibTest, ImportBaseModuleTwice)
    {
        const char* script =
            R"(
import azlmbrtest
import azlmbrtest
)";
        AZ::Entity entity;
        entity.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        entity.Init();
        entity.Activate();

        SimulateEditorBecomingInitialized();

        // Python keeps track of the module import count so that multiple attempts should result into a single import count
        {
            MyPythonBindings pythonBindings;
            EXPECT_EQ(PyRun_SimpleString(script), 0);
            EXPECT_EQ(pythonBindings.m_onImportModuleCount, 1);
        }

        entity.Deactivate();
    }

    TEST_F(PythonBindingLibTest, ExecuteSimpleBinding)
    {
        enum class LogTypes
        {
            Skip = 0,
            TesterAdd,
            TesterPrinted
        };

        PythonTraceMessageSink testSink;
        testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            AZStd::string_view w(window);
            if (w == "python")
            {
                AZStd::string_view m(message);
                if (m == "tester add equals 42")
                {
                    return (int)LogTypes::TesterAdd;
                }
                if (m == "tester says yo")
                {
                    return (int)LogTypes::TesterPrinted;
                }
            }
            return (int)LogTypes::Skip;
        };

        const char* script =
            R"(
import azlmbrtest
value = azlmbrtest.tester.add(40, 2)
print ('tester add equals ' + str(value))
value = azlmbrtest.tester.print('tester says yo')
)";

        AZ::Entity entity;
        entity.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        entity.Init();
        entity.Activate();

        SimulateEditorBecomingInitialized();

        {
            MyPythonBindings pythonBindings;
            EXPECT_EQ(PyRun_SimpleString(script), 0);
            EXPECT_EQ(pythonBindings.m_onImportModuleCount, 1);
            EXPECT_EQ(testSink.m_evaluationMap[(int)LogTypes::TesterAdd], 1);
            EXPECT_EQ(testSink.m_evaluationMap[(int)LogTypes::TesterPrinted], 1);
        }

        entity.Deactivate();
    }

    TEST_F(PythonBindingLibTest, ConvertAZTypes)
    {
        enum class LogTypes
        {
            Skip = 0,
            TypeConverted,
            IdIsValid,
            IdHasRepr,
            IdNowInvalid
        };

        PythonTraceMessageSink testSink;
        testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            AZStd::string_view m(message);
            AZStd::string_view w(window);
            if (w == "python")
            {
                if (m == "entityId equals 10")
                {
                    return (int)LogTypes::TypeConverted;
                }
                else if (m == "entityId is valid True")
                {
                    return (int)LogTypes::IdIsValid;
                }
                else if (m == "entityId is repr [10]")
                {
                    return (int)LogTypes::IdHasRepr;
                }
                else if (m == "entityId invalid is 4294967295")
                {
                    return (int)LogTypes::IdNowInvalid;
                }
            }
            return (int)LogTypes::Skip;
        };

        const char* script =
            R"(
import azlmbrtest
entityId = azlmbrtest.EntityId(10)
print ('entityId equals ' + str(entityId.id))
print ('entityId is valid ' + str(entityId.isValid()))
print ('entityId is repr ' + str(entityId))
entityId.setInvalid()
print ('entityId invalid is ' + str(entityId.id))
)";

        AZ::Entity entity;
        entity.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        entity.Init();
        entity.Activate();

        SimulateEditorBecomingInitialized();

        EXPECT_EQ(PyRun_SimpleString(script), 0);
        EXPECT_EQ(testSink.m_evaluationMap[(int)LogTypes::TypeConverted], 1);
        EXPECT_EQ(testSink.m_evaluationMap[(int)LogTypes::IdIsValid], 1);
        EXPECT_EQ(testSink.m_evaluationMap[(int)LogTypes::IdHasRepr], 1);
        EXPECT_EQ(testSink.m_evaluationMap[(int)LogTypes::IdNowInvalid], 1);

        entity.Deactivate();
    }

    TEST_F(PythonBindingLibTest, ImportProjectModules)
    {
        enum class LogTypes
        {
            Skip = 0,
            ImportModule,
            TestCallHit,
            TestTypeDoCall1
        };

        PythonTraceMessageSink testSink;
        testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "ImportModule"))
                {
                    return static_cast<int>(LogTypes::ImportModule);
                }
                else if (AzFramework::StringFunc::Equal(message, "test_call_hit"))
                {
                    return static_cast<int>(LogTypes::TestCallHit);
                }
                else if (AzFramework::StringFunc::Equal(message, "TestType.do_call.1"))
                {
                    return static_cast<int>(LogTypes::TestTypeDoCall1);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import sys, os
                import azlmbr.paths
                sys.path.append(os.path.join(azlmbr.paths.engroot,'Gems','EditorPythonBindings','Code','Tests'))
                from test_package import import_test as itest
                print('ImportModule')
                itest.test_call()
                testInst = itest.TestType()
                testInst.do_call(1)
            )");
        }
        catch ([[maybe_unused]] const std::exception& exception)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", exception.what());
        }

        e.Deactivate();

        EXPECT_EQ(1, testSink.m_evaluationMap[static_cast<int>(LogTypes::ImportModule)]);
        EXPECT_EQ(1, testSink.m_evaluationMap[static_cast<int>(LogTypes::TestCallHit)]);
        EXPECT_EQ(1, testSink.m_evaluationMap[static_cast<int>(LogTypes::TestTypeDoCall1)]);
    }

    TEST_F(PythonBindingLibTest, PyDocHelp_AzlmbrGlobals_Works)
    {
        enum class LogTypes
        {
            Skip = 0,
            Worked
        };

        PythonTraceMessageSink testSink;
        testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "Worked"))
                {
                    return aznumeric_cast<int>(LogTypes::Worked);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import pydoc
                import azlmbr.globals
                pydoc.help(azlmbr.globals)
                print('Worked')
            )");
        }
        catch ([[maybe_unused]] const std::exception& exception)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", exception.what());
        }

        e.Deactivate();

        EXPECT_EQ(1, testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::Worked)]);
    }

    TEST_F(PythonBindingLibTest, ImportAzLmbrTwice)
    {
        enum class LogTypes
        {
            Skip = 0,
            ImportAzLmbrTwice,
            SawEntityId
        };

        PythonTraceMessageSink testSink;
        testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "ImportAzLmbrTwice"))
                {
                    return aznumeric_cast<int>(LogTypes::ImportAzLmbrTwice);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "entity_id 101"))
                {
                    return aznumeric_cast<int>(LogTypes::SawEntityId);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import sys, os
                import azlmbr.paths
                sys.path.append(os.path.join(azlmbr.paths.engroot,'Gems','EditorPythonBindings','Code','Tests'))
                sys.path.append(os.path.join(azlmbr.paths.engroot,'Gems','EditorPythonBindings','Code','Tests','test_package'))

                from test_package import import_many
                import_many.test_many_entity_id()
                print('ImportAzLmbrTwice')
            )");
        }
        catch ([[maybe_unused]] const std::exception& exception)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", exception.what());
        }

        e.Deactivate();

        EXPECT_EQ(1, testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::ImportAzLmbrTwice)]);
        EXPECT_EQ(1, testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::SawEntityId)]);
    }
}
