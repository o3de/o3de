/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/PythonCommon.h>
#include <AzCore/PlatformDef.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

#include "PythonTraceMessageSink.h"
#include "PythonTestingUtility.h"

#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonMarshalComponent.h>
#include <Source/PythonProxyObject.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace CustomTest
{
    template <typename T>
    struct MyTemplate
    {
        MyTemplate() = default;
        MyTemplate(T value) : m_value(value) {}
        T m_value = {};
    };
}


namespace AZ
{
    template<typename T>
    struct OnDemandReflection<CustomTest::MyTemplate<T>>
    {
        using MyTemplateType = CustomTest::MyTemplate<T>;

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<MyTemplateType>()
                    ->Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(Script::Attributes::Module, "test.template")
                    ->Property("Value", 
                        [] (MyTemplateType* that) -> T { return that->m_value; },
                        [] (MyTemplateType* that, const T& value) { that->m_value = value; })
                    ;
            }
        }
    };

    AZ_TYPE_INFO_TEMPLATE(CustomTest::MyTemplate, "{82B9D060-F077-4FAA-9EF4-EF4C3A2A6332}", AZ_TYPE_INFO_CLASS);
}

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // test class/struts
    struct CustomTypeHolder
    {
        AZ_TYPE_INFO(MyTemplate, "{46543B40-D8AF-4498-BCD0-2FF2A040B42C}");

        CustomTest::MyTemplate<float> m_testFloat;
        CustomTest::MyTemplate<AZStd::string> m_testString;
        CustomTest::MyTemplate<int> m_testInt;

        CustomTypeHolder()
            : m_testFloat(42.0f)
            , m_testString("42")
            , m_testInt(42)
        {
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<CustomTest::MyTemplate<float>>();
                serializeContext->RegisterGenericType<CustomTest::MyTemplate<AZStd::string>>();
                serializeContext->RegisterGenericType<CustomTest::MyTemplate<int>>();
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<CustomTypeHolder>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Method("set_float", [](CustomTypeHolder& self, float value) { self.m_testFloat.m_value = value; })
                    ->Property("test_float", BehaviorValueProperty(&CustomTypeHolder::m_testFloat))
                    ->Property("test_string", BehaviorValueProperty(&CustomTypeHolder::m_testString))
                    ->Property("test_int", BehaviorValueProperty(&CustomTypeHolder::m_testInt))
                    ;
            }
        }
    };

    struct Descriptor final
    {
        AZ_TYPE_INFO(Descriptor, "{0DFEE628-EFE2-4B9B-BAF2-40ED2965E663}");

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<Descriptor>();
                serializeContext->RegisterGenericType<AZStd::vector<Descriptor>>();
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<Descriptor>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Property("s32", BehaviorValueProperty(&Descriptor::m_s32))
                    ->Property("u32", BehaviorValueProperty(&Descriptor::m_u32))
                    ->Property("scalar", BehaviorValueProperty(&Descriptor::m_scalar))
                    ->Property("bool_value", BehaviorValueProperty(&Descriptor::m_bool))
                    ->Property("string_value", BehaviorValueProperty(&Descriptor::m_stringValue))
                    ->Method("return_dummy_descriptor", []() { static Descriptor dummy; return dummy; }, nullptr, "")
                    ->Method("return_dummy_vector_descriptor", []() { static AZStd::vector<Descriptor> dummy; return dummy; }, nullptr, "")
                    ;
            }
        }

        Descriptor() = default;
        ~Descriptor() = default;

        AZ::s32 m_s32 = -1234;
        AZ::u32 m_u32 = 0xDEADBEEF;
        float m_scalar = -456.0f;
        bool m_bool = true;
        AZStd::string m_stringValue;
    };

    struct PythonReflectionAnyContainer
    {
        AZ_TYPE_INFO(PythonReflectionAnyContainer, "{D7D45479-9A46-469E-BE75-F305EBE8F848}");

        AZStd::any m_anyList; // will store a container like vector

        PythonReflectionAnyContainer()
        {
            AZStd::vector<AZ::s64> numbers{ 1,2,3,5,8,13 };
            m_anyList = AZStd::make_any<AZStd::vector<AZ::s64>>(numbers);
        }

        void MutateAnyContainer(const AZStd::any& value)
        {
            m_anyList = value;

            if(m_anyList.is<AZStd::vector<Descriptor>>())
            {
                const AZStd::vector<Descriptor>* ptr = AZStd::any_cast<AZStd::vector<Descriptor>>(&m_anyList);
                if (!ptr->empty())
                {
                    AZ_Printf("python", "ReplaceAnyList_AZStd::vector<Descriptor>", ptr->size());
                }
            }
        }

        const AZStd::any& AccessAnyContainer() const
        {
            if (m_anyList.is<AZStd::vector<Descriptor>>())
            {
                const AZStd::vector<Descriptor>* ptr = AZStd::any_cast<AZStd::vector<Descriptor>>(&m_anyList);
                if (!ptr->empty())
                {
                    AZ_Printf("python", "AccessAnyList_AZStd::vector<Descriptor>", ptr->size());
                }
            }

            return m_anyList;
        }

        void Reflect(AZ::ReflectContext* context)
        {
            using namespace EditorPythonBindings;

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::any>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::s64>>();
                serializeContext->RegisterGenericType<AZStd::vector<double>>();
                serializeContext->RegisterGenericType<AZStd::vector<bool>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::string>>();
                serializeContext->RegisterGenericType<AZStd::vector<PythonProxyObject>>();
                ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionAnyContainer>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Method("mutate_any_container", &PythonReflectionAnyContainer::MutateAnyContainer, nullptr, "")
                    ->Method("access_any_container", &PythonReflectionAnyContainer::AccessAnyContainer, nullptr, "")
                    ->Method("return_dummy_vector_integer", []() { static AZStd::vector<AZ::s64> dummy; return dummy; }, nullptr, "")
                    ->Method("return_dummy_vector_double", []() { static AZStd::vector<double> dummy; return dummy; }, nullptr, "")
                    ->Method("return_dummy_vector_bool", []() { static AZStd::vector<bool> dummy; return dummy; }, nullptr, "")
                    ->Method("return_dummy_vector_string", []() { static AZStd::vector<AZStd::string> dummy; return dummy; }, nullptr, "")
                    ->Method("return_dummy_vector_proxy", []() { static AZStd::vector<PythonProxyObject> dummy; return dummy; }, nullptr, "")
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonReflectAnyContainerTests
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

    TEST_F(PythonReflectAnyContainerTests, AccessReplaceVectorTypes)
    {
        enum class LogTypes
        {
            Skip = 0,
            AccessAnyList,
            ReplaceAnyList,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "AccessAnyList"))
                {
                    return aznumeric_cast<int>(LogTypes::AccessAnyList);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "ReplaceAnyList"))
                {
                    return aznumeric_cast<int>(LogTypes::ReplaceAnyList);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        PythonReflectionAnyContainer pythonReflectionAnyContainer;
        pythonReflectionAnyContainer.Reflect(m_app.GetBehaviorContext());
        pythonReflectionAnyContainer.Reflect(m_app.GetSerializeContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();
        try
        {
            pybind11::exec(R"(
                import azlmbr.test as test
                testObject = test.PythonReflectionAnyContainer()

                target = [1,2,3,5,8,13]
                values = testObject.access_any_container()
                if (len(values) > 0):
                    print ('AccessAnyList_for_values')
                if (values == target):
                    print ('AccessAnyList_matching_ends')
                target.reverse()
                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                if (values == target):
                    print ('ReplaceAnyList_replaced_as_reversed')

                target = [True,False,True,True]
                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                if( type(values[0]) is bool):
                    print ('AccessAnyList_matching_bools')
                target.reverse()
                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                if (values == target):
                    print ('ReplaceAnyList_replaced_bools')

                target = [-1.0,1.0,-10.0,10.0]
                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                if (values == target):
                    print ('AccessAnyList_matching_floats')
                target.reverse()
                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                if (values == target):
                    print ('ReplaceAnyList_replaced_floats')

                target = ['one','2','three','0x4']
                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                if (values == target):
                    print ('AccessAnyList_matching_strings')
                target.reverse()
                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                if (values == target):
                    print ('ReplaceAnyList_strings')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed with %s", e.what());
            FAIL();
        }
        e.Deactivate();
        EXPECT_EQ(5, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::AccessAnyList)]);
        EXPECT_EQ(4, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::ReplaceAnyList)]);
    }

    TEST_F(PythonReflectAnyContainerTests, AccessReplaceComplexTypes)
    {
        enum class LogTypes
        {
            Skip = 0,
            AccessAnyList,
            ReplaceAnyList,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "AccessAnyList"))
                {
                    return aznumeric_cast<int>(LogTypes::AccessAnyList);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "ReplaceAnyList"))
                {
                    return aznumeric_cast<int>(LogTypes::ReplaceAnyList);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        PythonReflectionAnyContainer pythonReflectionAnyContainer;
        pythonReflectionAnyContainer.Reflect(m_app.GetBehaviorContext());
        pythonReflectionAnyContainer.Reflect(m_app.GetSerializeContext());

        Descriptor descriptor;
        descriptor.Reflect(m_app.GetBehaviorContext());
        descriptor.Reflect(m_app.GetSerializeContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();
        try
        {
            pybind11::exec(R"(
                import azlmbr.test as test
                import azlmbr.object
                testObject = test.PythonReflectionAnyContainer()

                def create_descriptor(s32, u32, scalar, bool_value, string_value):
                    descriptor = test.Descriptor()
                    descriptor.s32 = s32
                    descriptor.u32 = u32
                    descriptor.scalar = scalar
                    descriptor.bool_value = bool_value
                    descriptor.string_value = string_value
                    return descriptor

                def equals_descriptor(lhs, rhs):
                    return (lhs.s32 == rhs.s32 and
                            lhs.u32 == rhs.u32 and
                            lhs.scalar == rhs.scalar and
                            lhs.bool_value == rhs.bool_value and
                            lhs.string_value == rhs.string_value)

                target = []
                target.append(create_descriptor(-1, 2, 3.0, True, 'one'))
                target.append(create_descriptor(-2, 3, 4.0, False, '0X2'))
                target.append(create_descriptor(-3, 4, 5.0, True, 'T H R E E'))

                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                if( isinstance(values[0], azlmbr.object.PythonProxyObject) and values[0].typename == 'Descriptor'):
                    print ('AccessAnyList_matches_descriptor_type')
                target.reverse()
                testObject.mutate_any_container(target)
                values = testObject.access_any_container()
                for x in range(0, len(values)):
                    if ( equals_descriptor(values[x], target[x]) ):
                        print ('ReplaceAnyList_replaced_descriptors')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed with %s", e.what());
            FAIL();
        }
        e.Deactivate();
        EXPECT_EQ(3, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::AccessAnyList)]);
        EXPECT_EQ(5, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::ReplaceAnyList)]);
    }

    TEST_F(PythonReflectAnyContainerTests, CustomTypeTemplates)
    {
        enum class LogTypes
        {
            Skip = 0,
            Float,
            String,
            Integer
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "Float"))
                {
                    return aznumeric_cast<int>(LogTypes::Float);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "String"))
                {
                    return aznumeric_cast<int>(LogTypes::String);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "Integer"))
                {
                    return aznumeric_cast<int>(LogTypes::Integer);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        CustomTypeHolder customTypeHolder;
        customTypeHolder.Reflect(m_app.GetBehaviorContext());
        customTypeHolder.Reflect(m_app.GetSerializeContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();
        try
        {
            pybind11::exec(R"(
                import azlmbr.test
                import azlmbr.test.template

                templateFloat = azlmbr.test.template.CustomTest_MyTemplate_float(40.0 + 2.0)
                print('Float - created template with float')

                templateString = azlmbr.test.template.CustomTest_MyTemplate_string('forty-two')
                print('String - created template with string')

                templateInt = azlmbr.test.template.CustomTest_MyTemplate_int(40 + 2)
                print('Integer - created template with int')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed with %s", e.what());
        }
        e.Deactivate();
        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::Float)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::String)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::Integer)]);
    }
}
