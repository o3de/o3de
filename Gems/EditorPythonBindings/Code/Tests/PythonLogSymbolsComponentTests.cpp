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
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonMarshalComponent.h>
#include <Source/PythonLogSymbolsComponent.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>


namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // test classes/structs

    class PythonLogSymbolsTestComponent :
        public EditorPythonBindings::PythonLogSymbolsComponent
    {
    public:
        AZ_COMPONENT(PythonLogSymbolsTestComponent, "{D5802A34-1B57-470B-8C30-FFC273C9F4ED}", EditorPythonBindings::PythonLogSymbolsComponent);

        AZStd::string_view FetchPythonTypeAndTraitsWrapper(const AZ::TypeId& typeId, AZ::u32 traits)
        {
            return FetchPythonTypeAndTraits(typeId, traits);
        }

        AZStd::string FetchPythonTypeWrapper(const AZ::BehaviorParameter& param)
        {
            return FetchPythonTypeName(param);
        }
    };

    class SimpleClass
    {
    public:
        AZ_TYPE_INFO(SimpleClass, "{DFA153D8-F168-44F9-8DEF-55CDBBAA5AA2}")
    };

    class CustomClass
    {
    public:
        AZ_TYPE_INFO(CustomClass, "{361A9A18-40E6-4D16-920A-0F38F55D63BF}")

        void NoOp() const
        {}
    };

    struct TestTypesReflectionContainer
    {
        AZ_TYPE_INFO(TestTypesReflectionContainer, "{5DE28B62-F9A1-4307-9684-6C95B9EE3225}")

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<AZStd::vector<int>>();
                serializeContext->RegisterGenericType<AZStd::vector<SimpleClass>>();
                serializeContext->RegisterGenericType<AZStd::vector<CustomClass>>();
                serializeContext->RegisterGenericType<AZStd::map<int, int>>();
                serializeContext->RegisterGenericType<AZStd::map<int, SimpleClass>>();
                serializeContext->RegisterGenericType<AZStd::map<int, CustomClass>>();
                serializeContext->RegisterGenericType<AZ::Outcome<int, int>>();
                serializeContext->RegisterGenericType<AZ::Outcome<int, SimpleClass>>();
                serializeContext->RegisterGenericType<AZ::Outcome<int, CustomClass>>();
                serializeContext->Class<CustomClass>()
                    ->Version(1)
                    ;
                // SimpleClass registration ommited for testing cases where type cannot be determined.
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonLogSymbolsComponentTest
        : public PythonTestingFixture
    {

        void SetUp() override
        {
            PythonTestingFixture::SetUp();
            PythonTestingFixture::RegisterComponentDescriptors();

            // Registering test types
            TestTypesReflectionContainer typesContainer;
            typesContainer.Reflect(m_app.GetSerializeContext());
            typesContainer.Reflect(m_app.GetBehaviorContext());
        }

        void TearDown() override
        {
            // clearing up memory
            PythonTestingFixture::TearDown();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // tests

    TEST_F(PythonLogSymbolsComponentTest, FetchSupportedTypesByTypeAndTraits_PythonTypeReturned)
    {
        PythonLogSymbolsTestComponent pythonLogSymbolsComponent;
        AZStd::vector<AZStd::tuple<AZ::TypeId, AZ::u32, AZStd::string>> typesToTest =
        {
            // Simple types

            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::string_view>::Uuid(), AZ::BehaviorParameter::TR_NONE, "str"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::string>::Uuid(), AZ::BehaviorParameter::TR_NONE, "str"),
            AZStd::make_tuple(AZ::AzTypeInfo<char>::Uuid(), AZ::BehaviorParameter::TR_POINTER | AZ::BehaviorParameter::TR_CONST, "str"),

            AZStd::make_tuple(AZ::AzTypeInfo<float>::Uuid(), AZ::BehaviorParameter::TR_NONE, "float"),
            AZStd::make_tuple(AZ::AzTypeInfo<double>::Uuid(), AZ::BehaviorParameter::TR_NONE, "float"),

            AZStd::make_tuple(AZ::AzTypeInfo<bool>::Uuid(), AZ::BehaviorParameter::TR_NONE, "bool"),

            AZStd::make_tuple(AZ::AzTypeInfo<AZ::s8>::Uuid(), AZ::BehaviorParameter::TR_NONE, "int"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::u8>::Uuid(), AZ::BehaviorParameter::TR_NONE, "int"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::s16>::Uuid(), AZ::BehaviorParameter::TR_NONE, "int"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::u16>::Uuid(), AZ::BehaviorParameter::TR_NONE, "int"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::s32>::Uuid(), AZ::BehaviorParameter::TR_NONE, "int"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::u32>::Uuid(), AZ::BehaviorParameter::TR_NONE, "int"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::s64>::Uuid(), AZ::BehaviorParameter::TR_NONE, "int"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::u64>::Uuid(), AZ::BehaviorParameter::TR_NONE, "int"),

            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::vector<AZ::u8>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "bytes"),

            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::any>::Uuid(), AZ::BehaviorParameter::TR_NONE, "object"),

            AZStd::make_tuple(AZ::AzTypeInfo<void>::Uuid(), AZ::BehaviorParameter::TR_NONE, "None"),

            // Container types

            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::vector<SimpleClass>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "list"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::vector<int>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "List[int]"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::vector<CustomClass>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "List[CustomClass]"),

            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::map<int, SimpleClass>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "dict"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::map<int, int>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "Dict[int, int]"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZStd::map<int, CustomClass>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "Dict[int, CustomClass]"),

            AZStd::make_tuple(AZ::AzTypeInfo<AZ::Outcome<int, SimpleClass>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "Outcome"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::Outcome<int, int>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "Outcome[int, int]"),
            AZStd::make_tuple(AZ::AzTypeInfo<AZ::Outcome<int, CustomClass>>::Uuid(), AZ::BehaviorParameter::TR_NONE, "Outcome[int, CustomClass]"),

            // Fallback to name

            AZStd::make_tuple(AZ::AzTypeInfo<SimpleClass>::Uuid(), AZ::BehaviorParameter::TR_NONE, ""),
            AZStd::make_tuple(AZ::AzTypeInfo<CustomClass>::Uuid(), AZ::BehaviorParameter::TR_NONE, "CustomClass")
        };

        auto stringViewHelper = [](const AZStd::string_view& s)
        {
            return AZStd::string::format(AZ_STRING_FORMAT, AZ_STRING_ARG(s));
        };

        for (auto& typeInfo : typesToTest)
        {
            AZStd::string_view result = pythonLogSymbolsComponent.FetchPythonTypeAndTraitsWrapper(AZStd::get<0>(typeInfo), AZStd::get<1>(typeInfo));
            EXPECT_EQ(result, AZStd::get<2>(typeInfo))
                << "Expected '" << stringViewHelper(AZStd::get<2>(typeInfo)).c_str()
                << "' when converting type with id " << AZStd::get<0>(typeInfo).ToFixedString().c_str()
                << " but got '" << stringViewHelper(result).c_str() << "'.";
        }
    }

    TEST_F(PythonLogSymbolsComponentTest, FetchByParam_ReturnPythonType)
    {
        PythonLogSymbolsTestComponent pythonLogSymbolsComponent;
        AZ::BehaviorParameter intParam;
        intParam.m_name = "foo";
        intParam.m_typeId = AZ::AzTypeInfo<AZ::s8>::Uuid(); // Uuid for a supported type
        intParam.m_traits = AZ::BehaviorParameter::TR_NONE;

        AZStd::string result = pythonLogSymbolsComponent.FetchPythonTypeWrapper(intParam);
        EXPECT_EQ(result, "int");
    }

    TEST_F(PythonLogSymbolsComponentTest, FetchVoidByParam_ReturnNone)
    {
        PythonLogSymbolsTestComponent m_pythonLogSymbolsComponent;
        AZ::BehaviorParameter voidParam;
        voidParam.m_name = "void";
        voidParam.m_typeId = AZ::Uuid("{9B3E8886-B749-418E-A696-6D7E9EB4D691}"); // A random Uuid
        voidParam.m_traits = AZ::BehaviorParameter::TR_NONE;
        
        AZStd::string result = m_pythonLogSymbolsComponent.FetchPythonTypeWrapper(voidParam);
        EXPECT_EQ(result, "None");
    }
}
