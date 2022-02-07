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

#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonProxyObject.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    void AcceptTwoStrings(AZStd::string stringValue1, AZStd::string stringValue2)
    {
        AZ_TracePrintf("python", stringValue1.empty() ? "stringValue1_is_empty" : "stringValue1_has_data");
        AZ_TracePrintf("python", stringValue2.empty() ? "stringValue2_is_empty" : "stringValue2_has_data");
    }

    //////////////////////////////////////////////////////////////////////////
    // test class/struts
    struct PythonGlobalsTester
    {
        AZ_TYPE_INFO(PythonGlobalsTester, "{00EC83FE-2E9D-42D0-8A59-2940669C7BCA}");

        enum GlobalEnums : AZ::u16
        {
            GE_NONE,
            GE_LUMBER = 101,
            GE_YARD
        };

        enum class MyTypes
        {
            One = 1,
            Two = 2,
        };

        static AZ::s32 s_staticValue;
        static AZ::u32 s_pingCount;
        static GlobalEnums s_result1;
        static GlobalEnums s_result2;
        static constexpr AZ::u8 s_one = 1;
        static AZ::Uuid s_myTypeId;
        static AZStd::string s_myString;

        static AZ::s32 GetValue()
        {
            return s_staticValue;
        }

        static void SetValue(AZ::s32 value)
        {
            s_staticValue = value;
        }

        static AZ::u32 Ping()
        {
            ++s_pingCount;
            return s_pingCount;
        }

        static void Reset()
        {
            s_pingCount = 0;
            s_staticValue = 0;
            s_result1 = GlobalEnums::GE_NONE;
            s_result2 = GlobalEnums::GE_NONE;
            s_myTypeId = AZ::TypeId::CreateString("{DEADBEE5-F983-4153-848A-EE9F99502811}");
            s_myString = AZStd::string("my string");
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                // Methods

                behaviorContext->Method("ping", &PythonGlobalsTester::Ping)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "test.pinger");

                behaviorContext->Method("reset", &PythonGlobalsTester::Reset)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                behaviorContext->Method("accept_two_strings", AcceptTwoStrings)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                // Property

                behaviorContext->Property("constantNumber", []() { return PythonGlobalsTester::GetValue(); }, nullptr)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                behaviorContext->Property("coolProperty", &PythonGlobalsTester::GetValue, &PythonGlobalsTester::SetValue)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                behaviorContext->Property("pingCount", BehaviorValueGetter(&s_pingCount), BehaviorValueSetter(&s_pingCount))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                // Enums

                behaviorContext->EnumProperty<GlobalEnums::GE_LUMBER>("GE_LUMBER")
                  ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                behaviorContext->EnumProperty<GlobalEnums::GE_YARD>("GE_YARD")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                // azlmbr.my.enum.One
                behaviorContext->EnumProperty<aznumeric_cast<int>(MyTypes::One)>("One")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "my.enum") 
                    ;

                // azlmbr.my.enum.Two
                behaviorContext->EnumProperty<aznumeric_cast<int>(MyTypes::Two)>("Two")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "my.enum")
                    ;

                behaviorContext->Property("result1", []() { return s_result1; }, [](GlobalEnums value) { s_result1 = value; })
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                behaviorContext->Property("result2", []() { return s_result2; }, [](GlobalEnums value) { s_result2 = value; })
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                // Constants

                behaviorContext->ConstantProperty("ONE", []() { return s_one; })
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                // azlmbr.constant.MY_TYPE
                behaviorContext->ConstantProperty("MY_TYPE", []() { return s_myTypeId; })
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "constant") 
                    ;

                // azlmbr.constant.MY_STRING
                behaviorContext->ConstantProperty("MY_STRING", []() { return s_myString; })
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "constant") 
                    ;
            }
        }
    };
    AZ::s32 PythonGlobalsTester::s_staticValue = 0;
    AZ::u32 PythonGlobalsTester::s_pingCount = 0;
    PythonGlobalsTester::GlobalEnums PythonGlobalsTester::s_result1 = PythonGlobalsTester::GlobalEnums::GE_NONE;
    PythonGlobalsTester::GlobalEnums PythonGlobalsTester::s_result2 = PythonGlobalsTester::GlobalEnums::GE_NONE;
    AZ::Uuid PythonGlobalsTester::s_myTypeId;
    AZStd::string PythonGlobalsTester::s_myString;

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonGlobalsTests
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

        void Deactivate(AZ::Entity& entity)
        {
            auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
            if (editorPythonEventsInterface)
            {
                editorPythonEventsInterface->StopPython();
            }

            entity.Deactivate();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // tests

    TEST_F(PythonGlobalsTests, GlobalMethodTest)
    {
        PythonGlobalsTester pythonGlobalsTester;
        pythonGlobalsTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                # testing global methods
                import azlmbr.globals
                import azlmbr.test.pinger
                azlmbr.globals.reset()
                for i in range(830):
                    azlmbr.test.pinger.ping()
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        Deactivate(e);

        EXPECT_EQ(830, PythonGlobalsTester::s_pingCount);
    }

    TEST_F(PythonGlobalsTests, GlobalPropertyTest)
    {
        enum class LogTypes
        {
            Skip = 0,
            GlobalPropertyTest_NotNone,
            GlobalPropertyTest_Is40,
            GlobalPropertyTest_Is42,
            GlobalPropertyTest_PingWorked,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "GlobalPropertyTest_NotNone"))
                {
                    return static_cast<int>(LogTypes::GlobalPropertyTest_NotNone);
                }
                else if (AzFramework::StringFunc::Equal(message, "GlobalPropertyTest_Is40"))
                {
                    return static_cast<int>(LogTypes::GlobalPropertyTest_Is40);
                }
                else if (AzFramework::StringFunc::Equal(message, "GlobalPropertyTest_Is42"))
                {
                    return static_cast<int>(LogTypes::GlobalPropertyTest_Is42);
                }
                else if (AzFramework::StringFunc::Equal(message, "GlobalPropertyTest_PingWorked"))
                {
                    return static_cast<int>(LogTypes::GlobalPropertyTest_PingWorked);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonGlobalsTester pythonGlobalsTester;
        pythonGlobalsTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.globals
                import azlmbr.test.pinger

                # testing global properties
                if (azlmbr.globals.property.constantNumber == 0):
                    print ('GlobalPropertyTest_NotNone')

                azlmbr.globals.property.coolProperty = 40
                if (azlmbr.globals.property.coolProperty == 40):
                    print ('GlobalPropertyTest_Is40')

                azlmbr.globals.property.coolProperty = azlmbr.globals.property.coolProperty + 2
                if (azlmbr.globals.property.constantNumber == 42):
                    print ('GlobalPropertyTest_Is42')

                azlmbr.globals.property.pingCount = 0
                for i in range(830):
                    azlmbr.test.pinger.ping()

                if (azlmbr.globals.property.pingCount == 830):
                    print ('GlobalPropertyTest_PingWorked')

            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        Deactivate(e);

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GlobalPropertyTest_NotNone)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GlobalPropertyTest_Is40)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GlobalPropertyTest_Is42)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GlobalPropertyTest_PingWorked)]);
    }

    TEST_F(PythonGlobalsTests, GlobalEnumTest)
    {
        enum class LogTypes
        {
            Skip = 0,
            GlobalEnumTest_Lumber,
            GlobalEnumTest_Yard
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "GlobalEnumTest_Lumber"))
                {
                    return static_cast<int>(LogTypes::GlobalEnumTest_Lumber);
                }
                else if (AzFramework::StringFunc::Equal(message, "GlobalEnumTest_Yard"))
                {
                    return static_cast<int>(LogTypes::GlobalEnumTest_Yard);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonGlobalsTester pythonGlobalsTester;
        pythonGlobalsTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.globals
                azlmbr.globals.reset()

                # testing global enum constant values
                if (azlmbr.globals.property.GE_LUMBER == 101):
                    print ('GlobalEnumTest_Lumber')

                if (azlmbr.globals.property.GE_YARD == 102):
                    print ('GlobalEnumTest_Yard')

                azlmbr.globals.property.result1 = azlmbr.globals.property.GE_LUMBER
                azlmbr.globals.property.result2 = azlmbr.globals.property.GE_YARD
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        Deactivate(e);

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GlobalEnumTest_Lumber)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GlobalEnumTest_Yard)]);
        EXPECT_EQ(PythonGlobalsTester::GlobalEnums::GE_LUMBER, PythonGlobalsTester::s_result1);
        EXPECT_EQ(PythonGlobalsTester::GlobalEnums::GE_YARD, PythonGlobalsTester::s_result2);
    }

    TEST_F(PythonGlobalsTests, GlobalConstantTest)
    {
        enum class LogTypes
        {
            Skip = 0,
            GlobalConstantTest_Fetch,
            GlobalConstantTest_Adds
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "GlobalConstantTest_Fetch"))
                {
                    return static_cast<int>(LogTypes::GlobalConstantTest_Fetch);
                }
                else if (AzFramework::StringFunc::Equal(message, "GlobalConstantTest_Adds"))
                {
                    return static_cast<int>(LogTypes::GlobalConstantTest_Adds);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonGlobalsTester pythonGlobalsTester;
        pythonGlobalsTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.globals
                azlmbr.globals.reset()

                # testing global enum constant values
                if (azlmbr.globals.property.ONE == 1):
                    print ('GlobalConstantTest_Fetch')

                a = azlmbr.globals.property.ONE
                b = azlmbr.globals.property.ONE
                if ((a + b) == 2):
                    print ('GlobalConstantTest_Adds')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        Deactivate(e);

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GlobalConstantTest_Fetch)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GlobalConstantTest_Adds)]);
    }

    TEST_F(PythonGlobalsTests, TryAcceptTwoStrings)
    {
        PythonGlobalsTester pythonGlobalsTester;
        pythonGlobalsTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            stringValue1_has_data,
            stringValue2_is_empty
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "stringValue1_has_data"))
                {
                    return aznumeric_cast<int>(LogTypes::stringValue1_has_data);
                }
                else if (AzFramework::StringFunc::Equal(message, "stringValue2_is_empty"))
                {
                    return aznumeric_cast<int>(LogTypes::stringValue2_is_empty);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        try
        {
            pybind11::exec(R"(
                import azlmbr.globals
                azlmbr.globals.accept_two_strings("Test 01", "")
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        Deactivate(e);

        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::stringValue1_has_data)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::stringValue2_is_empty)]);
    }

    TEST_F(PythonGlobalsTests, GlobalListAllClasses)
    {
        PythonGlobalsTester pythonGlobalsTester;
        pythonGlobalsTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            ClassesFound
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "ClassListFound"))
                {
                    return aznumeric_cast<int>(LogTypes::ClassesFound);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        try
        {
            pybind11::exec(R"(
                import azlmbr.object
                classList = azlmbr.object.list_classes()
                if (len(classList) > 0):
                    print ('ClassListFound')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        Deactivate(e);

        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::ClassesFound)]);
    }

    TEST_F(PythonGlobalsTests, GlobalModuleDefinedTypeId)
    {
        PythonGlobalsTester pythonGlobalsTester;
        pythonGlobalsTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            TypeIsValid,
            StringTypeIsValid,
            EnumIsValid,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "TypeIsValid"))
                {
                    return aznumeric_cast<int>(LogTypes::TypeIsValid);
                }
                else if (AzFramework::StringFunc::Equal(message, "StringTypeIsValid"))
                {
                    return aznumeric_cast<int>(LogTypes::StringTypeIsValid);
                }
                else if (AzFramework::StringFunc::Equal(message, "EnumIsValid"))
                {
                    return aznumeric_cast<int>(LogTypes::EnumIsValid);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };
        
        try
        {
            pybind11::exec(R"(
                import azlmbr.constant
                import azlmbr.my.enum
                import azlmbr.globals
                azlmbr.globals.reset()
                type = azlmbr.constant.MY_TYPE
                if (type.ToString().startswith('{DEADBEE5-')):
                    print ('TypeIsValid')
                if (azlmbr.constant.MY_STRING == 'my string'):
                    print ('StringTypeIsValid')
                if (azlmbr.my.enum.One == 1):
                    print ('EnumIsValid')
                if (azlmbr.my.enum.Two == 2):
                    print ('EnumIsValid')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        Deactivate(e);

        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::TypeIsValid)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::StringTypeIsValid)]);
        EXPECT_EQ(2, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::EnumIsValid)]);
    }

    TEST_F(PythonGlobalsTests, CompareEqualityOperators)
    {
        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            IsGreaterThan,
            IsGreaterEqualTo,
            IsLessThan,
            IsLessEqualTo,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "IsGreaterThan"))
                {
                    return aznumeric_cast<int>(LogTypes::IsGreaterThan);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "IsGreaterEqualTo"))
                {
                    return aznumeric_cast<int>(LogTypes::IsGreaterEqualTo);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "IsLessThan"))
                {
                    return aznumeric_cast<int>(LogTypes::IsLessThan);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "IsLessEqualTo"))
                {
                    return aznumeric_cast<int>(LogTypes::IsLessEqualTo);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        try
        {
            pybind11::exec(R"(
                import azlmbr.math
                import azlmbr.globals
                pointA = azlmbr.math.Vector2(40.0)
                pointB = azlmbr.math.Vector2(2.0)
                if (pointB < pointA):
                    print ('IsLessThan')
                if (pointB <= pointA):
                    print ('IsLessEqualTo')
                if (pointB <= pointB):
                    print ('IsLessEqualTo')
                if (pointA > pointB):
                    print ('IsGreaterThan')
                if (pointA >= pointB):
                    print ('IsGreaterEqualTo')
                if (pointA >= pointA):

                    print ('IsGreaterEqualTo')
                if (pointB >= pointA):
                    print ('IsGreaterEqualTo')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        Deactivate(e);

        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::IsGreaterThan)]);
        EXPECT_EQ(2, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::IsGreaterEqualTo)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::IsLessThan)]);
        EXPECT_EQ(2, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::IsLessEqualTo)]);
    }
}
