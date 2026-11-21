/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PythonTestingUtility.h"
#include "PythonTraceMessageSink.h"
#include <EditorPythonBindings/PythonCommon.h>
#include <EditorPythonBindings/PythonUtility.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonProxyObject.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/any.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    void AcceptTwoStrings([[maybe_unused]] AZStd::string stringValue1, [[maybe_unused]] AZStd::string stringValue2)
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
    // structured call test types

    //! A reflected value that counts its live instances, so tests can check that structured construction
    //! and structured calls destroy every object they create
    struct StructuredConstructValue
    {
        AZ_TYPE_INFO(StructuredConstructValue, "{62762668-3D5C-4894-8D46-23C4DFB10BE6}");

        //! Any negative value makes the int constructor set a Python error without throwing
        static constexpr int ValueThatSetsPythonError = -1;

        static int s_liveCount;

        StructuredConstructValue()
        {
            ++s_liveCount;
        }

        StructuredConstructValue(const StructuredConstructValue& other)
            : m_value(other.m_value)
        {
            ++s_liveCount;
        }

        explicit StructuredConstructValue(int value)
            : m_value(value)
        {
            ++s_liveCount;
            if (value < 0)
            {
                PyErr_SetString(PyExc_RuntimeError, "Constructor reported a Python error");
            }
        }

        ~StructuredConstructValue()
        {
            --s_liveCount;
        }

        int Read() const
        {
            return m_value;
        }

        int m_value = 0;
    };

    int StructuredConstructValue::s_liveCount = 0;

    //! A result type with no Python marshaller of its own
    struct StructuredUnmarshalable
    {
        AZ_TYPE_INFO(StructuredUnmarshalable, "{88EA5E91-520A-497D-B874-70579531B177}");
    };

    //! A registered marshaller that declines every value, which a structured call must report as an error
    class StructuredDeclinedConverter final : public EditorPythonBindings::PythonMarshalComponent::TypeConverter
    {
    public:
        using Requests = EditorPythonBindings::PythonMarshalTypeRequests;

        // PythonMarshalComponent::TypeConverter overrides
        AZStd::optional<Requests::BehaviorValueResult> PythonToBehaviorValueParameter(
            Requests::BehaviorTraits, pybind11::object, AZ::BehaviorArgument&) override
        {
            return AZStd::nullopt;
        }

        AZStd::optional<Requests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument&) override
        {
            return AZStd::nullopt;
        }

        bool CanConvertPythonToBehaviorValue(Requests::BehaviorTraits, pybind11::object) const override
        {
            return false;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // tests

    TEST_F(PythonGlobalsTests, StructuredFixtureUsesConfiguredEngine)
    {
        const auto expected = AZ::IO::FixedMaxPath(EDITOR_PYTHON_BINDINGS_TEST_ENGINE_ROOT).LexicallyNormal();
        EXPECT_EQ(AZ::Utils::GetEnginePath(), expected.Native());
        EXPECT_TRUE(AZ::IO::SystemFile::Exists((expected / "engine.json").c_str()));
    }

    TEST_F(PythonGlobalsTests, StructuredCallErrorsAndValues)
    {
        using EditorPythonBindings::Call::StructuredMethod;

        PythonGlobalsTester tester;
        tester.Reflect(m_app.GetBehaviorContext());
        m_app.GetBehaviorContext()
            ->Method(
                "structured_int_arg",
                +[](int)
                {
                })
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

        AZ::Entity entity;
        Activate(entity);
        SimulateEditorBecomingInitialized();
        {
            auto* context = m_app.GetBehaviorContext();
            auto noArgs = pybind11::args(pybind11::make_tuple());

            // a result comes back as a Python value, and a method without a result returns None
            auto ping = StructuredMethod(context->m_methods.at("ping"), pybind11::none(), noArgs);
            ASSERT_TRUE(ping);
            EXPECT_GT(ping.GetValue().cast<unsigned int>(), 0u);

            auto reset = StructuredMethod(context->m_methods.at("reset"), pybind11::none(), noArgs);
            ASSERT_TRUE(reset);
            EXPECT_TRUE(reset.GetValue().is_none());

            // a missing method, too few or too many arguments, and a target that is not a proxy are errors
            EXPECT_FALSE(StructuredMethod(nullptr, pybind11::none(), noArgs));

            auto missingArgs = StructuredMethod(context->m_methods.at("accept_two_strings"), pybind11::none(), noArgs);
            ASSERT_FALSE(missingArgs);
            EXPECT_NE(missingArgs.GetError().find("expects"), AZStd::string::npos);

            auto excessArgs = StructuredMethod(context->m_methods.at("ping"), pybind11::none(), pybind11::args(pybind11::make_tuple(1)));
            EXPECT_FALSE(excessArgs);

            EXPECT_FALSE(StructuredMethod(context->m_methods.at("ping"), pybind11::int_(1), noArgs));

            // an argument that cannot be converted is reported with its index
            auto conversion = StructuredMethod(
                context->m_methods.at("structured_int_arg"), pybind11::none(), pybind11::args(pybind11::make_tuple(pybind11::list())));
            ASSERT_FALSE(conversion);
            EXPECT_NE(conversion.GetError().find("Argument 0 conversion failed"), AZStd::string::npos);
        }
        Deactivate(entity);
    }

    TEST_F(PythonGlobalsTests, StructuredConstructorSelectionAndOwnership)
    {
        using EditorPythonBindings::Call::StructuredConstruct;
        using EditorPythonBindings::Call::StructuredMethod;

        constexpr int ConstructedValue = 42;
        constexpr size_t IntConstructorIndex = 0;

        auto* context = m_app.GetBehaviorContext();
        context->Class<StructuredConstructValue>("StructuredConstructValue")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
            ->Constructor<int>()
            ->Property("value", BehaviorValueProperty(&StructuredConstructValue::m_value))
            ->Method("Read", &StructuredConstructValue::Read)
            ->Method(
                "Copy",
                +[](const StructuredConstructValue& value)
                {
                    return value;
                });

        AZ::Entity entity;
        Activate(entity);
        SimulateEditorBecomingInitialized();
        {
            pybind11::module::import("azlmbr.object");
            EXPECT_EQ(StructuredConstructValue::s_liveCount, 0);

            auto* behaviorClass = context->m_classes.at("StructuredConstructValue");
            auto noArgs = pybind11::args(pybind11::make_tuple());
            auto valueArgs = pybind11::args(pybind11::make_tuple(ConstructedValue));
            auto errorArgs = pybind11::args(pybind11::make_tuple(StructuredConstructValue::ValueThatSetsPythonError));

            // these constructions fail and leave no live object and no pending Python error: an index past the last
            // constructor, arguments for the default constructor, no arguments for the int constructor, and a
            // constructor that sets a Python error
            EXPECT_FALSE(StructuredConstruct(*behaviorClass, behaviorClass->m_constructors.size(), valueArgs));
            EXPECT_FALSE(StructuredConstruct(*behaviorClass, AZStd::nullopt, valueArgs));
            EXPECT_FALSE(StructuredConstruct(*behaviorClass, IntConstructorIndex, noArgs));
            EXPECT_FALSE(StructuredConstruct(*behaviorClass, IntConstructorIndex, errorArgs));
            EXPECT_EQ(StructuredConstructValue::s_liveCount, 0);
            EXPECT_FALSE(PyErr_Occurred());

            auto constructed = StructuredConstruct(*behaviorClass, IntConstructorIndex, valueArgs);
            ASSERT_TRUE(constructed);
            EXPECT_EQ(constructed.GetValue().attr("value").cast<int>(), ConstructedValue);

            auto defaultConstructed = StructuredConstruct(*behaviorClass, AZStd::nullopt, noArgs);
            ASSERT_TRUE(defaultConstructed);
            EXPECT_EQ(defaultConstructed.GetValue().attr("value").cast<int>(), 0);

            // a proxy target is passed as the first argument, and the returned value is a copy that Python owns
            auto copied = StructuredMethod(behaviorClass->m_methods.at("Copy"), constructed.GetValue(), noArgs);
            ASSERT_TRUE(copied);
            EXPECT_EQ(copied.GetValue().attr("value").cast<int>(), ConstructedValue);

            // a bound method that is kept keeps its object alive after the object itself is dropped
            pybind11::object retainedRead = constructed.GetValue().attr("Read");
            constructed.GetValue() = pybind11::none();
            EXPECT_EQ(StructuredConstructValue::s_liveCount, 3); // constructed, defaultConstructed and copied
            EXPECT_EQ(retainedRead().cast<int>(), ConstructedValue);

            retainedRead = pybind11::none();
            EXPECT_EQ(StructuredConstructValue::s_liveCount, 2); // defaultConstructed and copied
        }
        EXPECT_EQ(StructuredConstructValue::s_liveCount, 0);
        Deactivate(entity);
    }

    TEST_F(PythonGlobalsTests, StructuredCompletionAndEmptyAny)
    {
        using EditorPythonBindings::Call::StructuredMethod;

        // static so that the captureless lambda below can count its calls
        static int callCount = 0;
        callCount = 0;

        auto* context = m_app.GetBehaviorContext();

        // reflected without the Automation scope, so a result of this type cannot be copied into a proxy
        context->Class<StructuredUnmarshalable>("StructuredUnmarshalable");
        context
            ->Method(
                "structured_unmarshalable",
                +[]()
                {
                    ++callCount;
                    return StructuredUnmarshalable{};
                })
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        context
            ->Method(
                "structured_empty_any",
                +[]()
                {
                    return AZStd::any();
                })
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

        AZ::Entity entity;
        Activate(entity);
        SimulateEditorBecomingInitialized();
        {
            pybind11::module::import("azlmbr.object");
            auto noArgs = pybind11::args(pybind11::make_tuple());

            // the method ran, and only marshaling its result failed
            bool didInvoke = false;
            auto unmarshalable =
                StructuredMethod(context->m_methods.at("structured_unmarshalable"), pybind11::none(), noArgs, &didInvoke);
            EXPECT_FALSE(unmarshalable);
            EXPECT_TRUE(didInvoke);
            EXPECT_EQ(callCount, 1);

            // an empty AZStd::any result is None, not an error
            auto emptyAny = StructuredMethod(context->m_methods.at("structured_empty_any"), pybind11::none(), noArgs, &didInvoke);
            ASSERT_TRUE(emptyAny);
            EXPECT_TRUE(didInvoke);
            EXPECT_TRUE(emptyAny.GetValue().is_none());
        }
        Deactivate(entity);
    }

    TEST_F(PythonGlobalsTests, StructuredDeclinedMarshallerIsAnError)
    {
        using EditorPythonBindings::Call::StructuredMethod;

        auto* context = m_app.GetBehaviorContext();
        context->Class<StructuredUnmarshalable>("StructuredUnmarshalable")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        context
            ->Method(
                "structured_declined",
                +[]()
                {
                    return StructuredUnmarshalable{};
                })
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

        AZ::Entity entity;
        Activate(entity);
        SimulateEditorBecomingInitialized();
        entity.FindComponent<EditorPythonBindings::PythonMarshalComponent>()->RegisterTypeConverter(
            azrtti_typeid<StructuredUnmarshalable>(), AZStd::make_shared<StructuredDeclinedConverter>());
        {
            pybind11::module::import("azlmbr.object");
            auto noArgs = pybind11::args(pybind11::make_tuple());

            // the registered marshaller's refusal is reported instead of falling back to a proxy
            bool didInvoke = false;
            auto result = StructuredMethod(context->m_methods.at("structured_declined"), pybind11::none(), noArgs, &didInvoke);
            EXPECT_FALSE(result);
            EXPECT_TRUE(didInvoke);
            EXPECT_NE(result.GetError().find("marshaller refused"), AZStd::string::npos);
        }
        Deactivate(entity);
    }

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
