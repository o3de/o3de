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
#include <Source/PythonMarshalComponent.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // test class/struts

    struct PythonReflectionTestDoPrint
    {
        AZ_TYPE_INFO(PythonReflectionTestDoPrint, "{CA1146E1-A2DF-4AE3-A712-5333CE60D65C}");

        static const char* DoTest([[maybe_unused]] const char* label)
        {
            return "proxy_do_test";
        }

        static void DoPrint([[maybe_unused]] const char* msg)
        {
            AZ_TracePrintf("python", msg);
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionTestDoPrint>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.test")
                    ->Method("do_test", DoTest, nullptr, "Hook to perform a test with the legacy Python hook handler.")
                    ->Method("do_print", DoPrint, nullptr, "Hook to perform a test print action to a console.")
                    ;
            }
        }
    };

    struct PythonReflectionTestSimple
    {
        AZ_TYPE_INFO(PythonReflectionTestSimple, "{03277B3D-DEC2-4113-9FCA-D37D527FCE77}");

        static void DoWork()
        {
            AZ_TracePrintf("python", "PythonReflectionTestSimple_DoWork");
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionTestSimple>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    //AZ::Script::Attributes::Module, "default" //< this is optional; if not there then the BC is placed in the "azlmbr.default" module
                    ->Method("do_work", DoWork, nullptr, "Test do work.")
                    ;
            }
        }
    };

    struct PythonReflectionContainerSimpleTypes
    {
        AZ_TYPE_INFO(PythonReflectionContainerSimpleTypes, "{378AD363-467D-4285-BE40-4D1CB1A09A19}");

        AZStd::vector<float> m_floatValues{ 1.0, 2.2, 3.3, 4.4 };

        AZStd::vector<float>& ReturnVectorOfFloats()
        {
            return m_floatValues;
        }

        void AcceptVectorOfFloats(const AZStd::vector<float>& values)
        {
            m_floatValues.assign(values.begin(), values.end());
        }

        AZStd::vector<double> m_doubleValues{ 1.0, 2.2, 3.3, 4.4 };

        AZStd::vector<double>& ReturnVectorOfDoubles()
        {
            return m_doubleValues;
        }

        void AcceptVectorOfDoubles(const AZStd::vector<double>& values)
        {
            m_doubleValues.assign(values.begin(), values.end());
        }

        template <typename T>
        struct VectorOf
        {
            AZStd::vector<T> m_values;

            explicit VectorOf(std::initializer_list<T> list)
            {
                m_values = list;
            }

            const AZStd::vector<T>& ReturnValues() const
            {
                return m_values;
            }

            void AcceptValues(const AZStd::vector<T>& values)
            {
                m_values.assign(values.begin(), values.end());
            }
        };

        VectorOf<AZ::s8> m_s8ValueValues { 4, 5, 6, 7 };
        VectorOf<AZ::u8> m_u8ValueValues { 4, 5, 6, 7 };
        VectorOf<AZ::s16> m_s16ValueValues { 4, 5, 6, 7 };
        VectorOf<AZ::u16> m_u16ValueValues { 4, 5, 6, 7 };
        VectorOf<AZ::s32> m_s32ValueValues { 4, 5, 6, 7 };
        VectorOf<AZ::u32> m_u32ValueValues { 4, 5, 6, 7 };
        VectorOf<AZ::s64> m_s64ValueValues { 4, 5, 6, 7 };
        VectorOf<AZ::u64> m_u64ValueValues { 4, 5, 6, 7 };
        VectorOf<bool> m_boolValueValues { true, false, false };

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<AZStd::vector<bool>>();
                serializeContext->RegisterGenericType<AZStd::vector<float>>();
                serializeContext->RegisterGenericType<AZStd::vector<double>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::s8>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::u8>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::s16>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::u16>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::s32>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::u32>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::s64>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::u64>>();
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionContainerSimpleTypes>("PythonReflectionContainerSimpleTypes")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test.containers")
                    ->Method("return_vector_of_floats", &PythonReflectionContainerSimpleTypes::ReturnVectorOfFloats, nullptr, "")
                    ->Method("accept_vector_of_floats", &PythonReflectionContainerSimpleTypes::AcceptVectorOfFloats, nullptr, "")
                    ->Method("return_vector_of_doubles", &PythonReflectionContainerSimpleTypes::ReturnVectorOfDoubles, nullptr, "")
                    ->Method("accept_vector_of_doubles", &PythonReflectionContainerSimpleTypes::AcceptVectorOfDoubles, nullptr, "")
                    ->Property("vector_of_s8",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_s8ValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<AZ::s8>& values) { return self->m_s8ValueValues.AcceptValues(values); })
                    ->Property("vector_of_u8",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_u8ValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<AZ::u8>& values) { return self->m_u8ValueValues.AcceptValues(values); })
                    ->Property("vector_of_s16",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_s16ValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<AZ::s16>& values) { return self->m_s16ValueValues.AcceptValues(values); })
                    ->Property("vector_of_u16",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_u16ValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<AZ::u16>& values) { return self->m_u16ValueValues.AcceptValues(values); })
                    ->Property("vector_of_s32",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_s32ValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<AZ::s32>& values) { return self->m_s32ValueValues.AcceptValues(values); })
                    ->Property("vector_of_u32",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_u32ValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<AZ::u32>& values) { return self->m_u32ValueValues.AcceptValues(values); })
                    ->Property("vector_of_s64",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_s64ValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<AZ::s64>& values) { return self->m_s64ValueValues.AcceptValues(values); })
                    ->Property("vector_of_u64",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_u64ValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<AZ::u64>& values) { return self->m_u64ValueValues.AcceptValues(values); })
                    ->Property("vector_of_bool",
                        [](PythonReflectionContainerSimpleTypes* self) { return self->m_boolValueValues.ReturnValues(); },
                        [](PythonReflectionContainerSimpleTypes* self, const AZStd::vector<bool>& values) { return self->m_boolValueValues.AcceptValues(values); })
                    ;
            }
        }
    };

    struct PythonReflectionStringTypes
    {
        AZ_TYPE_INFO(PythonReflectionStringTypes, "{A6BF24DB-50E2-435B-A896-0192D24974B1}");

        static void RawPointerIn([[maybe_unused]] const char* value)
        {
            AZ_TracePrintf("python", value);
        }

        static const char* RawPointerOut()
        {
            return "PythonReflectStringTypes_RawStringOut";
        }

        static void StringViewIn([[maybe_unused]] AZStd::string_view value)
        {
            AZ_TracePrintf("python", "%.*s", static_cast<int>(value.size()), value.data());
        }

        static AZStd::string_view StringViewOut()
        {
            return { "PythonReflectStringTypes_StringViewOut" };
        }

        static void AZStdStringIn([[maybe_unused]] const AZStd::string& value)
        {
            AZ_TracePrintf("python", "%s", value.c_str());
        }

        static AZStd::string AZStdStringOut()
        {
            return { "PythonReflectStringTypes_AZStdStringOut" };
        }

        static AZStd::vector<AZStd::string> OutputStringList()
        {
            AZStd::vector<AZStd::string> listOfStrings;
            listOfStrings.push_back("one");
            listOfStrings.push_back("two");
            listOfStrings.push_back("three");
            return listOfStrings;
        }

        static bool InputStringList(AZStd::vector<AZStd::string> input)
        {
            if (input.size() == 3 && input[0] == "1" && input[1] == "2" && input[2] == "3")
            {
                return true;
            }
            return false;
        }

        static AZStd::vector<AZStd::string_view> OutputStringViewList()
        {
            AZStd::vector<AZStd::string_view> listOfStrings;
            listOfStrings.push_back("one");
            listOfStrings.push_back("two");
            listOfStrings.push_back("three");
            return listOfStrings;
        }

        static bool InputStringViewList(AZStd::vector<AZStd::string_view> input)
        {
            if (input.size() == 3 && input[0] == "a" && input[1] == "b" && input[2] == "c")
            {
                return true;
            }
            return false;
        }

        static AZStd::string OutputEmptyString()
        {
            return AZStd::string("");
        }

        static bool InputEmptyString(AZStd::string emptyString)
        {
            return emptyString.empty();
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionStringTypes>("Strings")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test.strings")
                    ->Method("raw_pointer_in", RawPointerIn, nullptr, "")
                    ->Method("raw_pointer_out", RawPointerOut, nullptr, "")
                    ->Method("string_view_in", StringViewIn, nullptr, "")
                    ->Method("string_view_out", StringViewOut, nullptr, "")
                    ->Method("azstd_string_in", AZStdStringIn, nullptr, "")
                    ->Method("azstd_string_out", AZStdStringOut, nullptr, "")
                    ->Method("output_string_list", OutputStringList, nullptr, "")
                    ->Method("input_string_list", InputStringList, nullptr, "")
                    ->Method("output_empty_string", OutputEmptyString, nullptr, "")
                    ->Method("input_empty_string", InputEmptyString, nullptr, "")
                    ->Method("output_string_view_list", OutputStringViewList, nullptr, "")
                    ->Method("input_string_view_list", InputStringViewList, nullptr, "")
                    ;
            }
        }
    };

    struct FakeEntityIdType final
    {
        AZ_TYPE_INFO(FakeEntityIdType, "{33FF7076-50AD-42E7-9DFF-19FA5026264A}");

    public:
        static const AZ::u64 InvalidEntityId = 0x00000000FFFFFFFFull;

        explicit FakeEntityIdType(AZ::u64 id = InvalidEntityId)
            : m_id(id)
        {}

        explicit operator AZ::u64() const
        {
            return m_id;
        }

        bool IsValid() const
        {
            return m_id != InvalidEntityId;
        }

        void SetInvalid()
        {
            m_id = InvalidEntityId;
        }

        AZStd::string ToString() const
        {
            return AZStd::string::format("[%llu]", m_id);
        }

        AZ::u64 m_id;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<FakeEntityIdType>("FakeEntityId")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "entity")
                    ->Method("IsValid", &FakeEntityIdType::IsValid)
                    ->Method("ToString", &FakeEntityIdType::ToString)
                    ;
            }
        }
    };

    struct PythonReflectionComplexContainer
    {
        AZ_TYPE_INFO(PythonReflectionComplexContainer, "{A1935F7F-6A22-4CA3-BEE7-A2F8E8D5D35F}");

        AZStd::vector<FakeEntityIdType> SendListOfIds() const
        {
            AZStd::vector<FakeEntityIdType> fakeIds;
            fakeIds.push_back(FakeEntityIdType{ 101 });
            fakeIds.push_back(FakeEntityIdType{ 202 });
            fakeIds.push_back(FakeEntityIdType{ 303 });
            return fakeIds;
        }

        void Reflect(AZ::ReflectContext* context)
        {
            FakeEntityIdType::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<AZStd::vector<FakeEntityIdType>>();
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionComplexContainer>("ComplexContainer")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Method("send_list_of_ids", &PythonReflectionComplexContainer::SendListOfIds)
                    ;
            }
        }
    };

    struct PythonReflectionAny
    {
        AZ_TYPE_INFO(PythonReflectionAny, "{1B6617E1-C259-48A4-A337-232782024B5D}");

        AZ::s32 m_intValue = 0;
        AZStd::any m_anyValue;
        AZ::Data::AssetId m_assetId;

        PythonReflectionAny()
        {
            m_assetId.m_guid = AZ::Uuid::CreateRandom();
        }

        void ReportMutate(const AZStd::any& value) const
        {
            if (value.is<PythonReflectionAny>())
            {
                AZ_TracePrintf("python", "MutateAny");
            }
            else if (value.is<AZ::Data::AssetId>())
            {
                AZ_TracePrintf("python", "MutateAssetId");
            }
        }

        void ReportAccess() const
        {
            if (m_anyValue.is<PythonReflectionAny>())
            {
                AZ_TracePrintf("python", "AccessAny");
            }
            else if (m_anyValue.is<AZ::Data::AssetId>())
            {
                AZ_TracePrintf("python", "AccessAssetId");
            }
        }

        void MutateAnyRef(const AZStd::any& value)
        {
            ReportMutate(value);
            m_anyValue = value;
        }

        const AZStd::any& AccessAnyRef() const
        {
            ReportAccess();
            return m_anyValue;
        }

        void MutateAnyValue(AZStd::any value)
        {
            ReportMutate(value);
            m_anyValue = value;
        }

        AZStd::any AccessAnyValue() const
        {
            ReportAccess();
            return m_anyValue;
        }

        void MutateAnyPointer(const AZStd::any* value)
        {
            ReportMutate(*value);
            m_anyValue = *value;
        }

        bool CompareAssetIds(const AZ::Data::AssetId& lhs, const AZ::Data::AssetId& rhs) const
        {
            return lhs == rhs;
        }

        AZStd::any m_anySimple;

        void MutateAnySimple(const AZStd::any& value)
        {
            if (value.is<double>())
            {
                AZ_TracePrintf("python", "MutateAnySimple_double");
            }
            else if (value.is<AZ::s64>())
            {
                AZ_TracePrintf("python", "MutateAnySimple_s64");
            }
            else if (value.is<bool>())
            {
                AZ_TracePrintf("python", "MutateAnySimple_bool");
            }
            else if (value.is<AZStd::string_view>())
            {
                AZ_TracePrintf("python", "MutateAnySimple_string_view");
            }
            m_anySimple = value;
        }

        const AZStd::any& AccessAnySimple() const
        {
            if (m_anySimple.is<double>())
            {
                AZ_TracePrintf("python", "AccessAnySimple_double");
            }
            else if (m_anySimple.is<AZ::s64>())
            {
                AZ_TracePrintf("python", "AccessAnySimple_s64");
            }
            else if (m_anySimple.is<bool>())
            {
                AZ_TracePrintf("python", "AccessAnySimple_bool");
            }
            else if (m_anySimple.is<AZStd::string_view>())
            {
                AZ_TracePrintf("python", "AccessAnySimple_string_view");
            }
            return m_anySimple;
        }


        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionAny>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Method("mutate_any_ref", &PythonReflectionAny::MutateAnyRef, nullptr, "Mutate any value ref.")
                    ->Method("access_any_ref", &PythonReflectionAny::AccessAnyRef, nullptr, "Access any value ref.")
                    ->Method("mutate_any_pointer", &PythonReflectionAny::MutateAnyPointer, nullptr, "Mutate any value ptr.")
                    ->Method("mutate_any_value", &PythonReflectionAny::MutateAnyValue, nullptr, "Mutate any value by value.")
                    ->Method("access_any_value", &PythonReflectionAny::AccessAnyValue, nullptr, "Access any value by value.")
                    ->Property("theInt", BehaviorValueProperty(&PythonReflectionAny::m_intValue))
                    ->Property("theAsset", BehaviorValueProperty(&PythonReflectionAny::m_assetId))
                    ->Method("compare_asset_ids", &PythonReflectionAny::CompareAssetIds)
                    ->Method("mutate_any_simple", &PythonReflectionAny::MutateAnySimple)
                    ->Method("access_any_simple", &PythonReflectionAny::AccessAnySimple)
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonReflectionComponentTests
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

    TEST_F(PythonReflectionComponentTests, InstallingPythonReflectionComponent)
    {
        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.CreateComponent<EditorPythonBindings::PythonReflectionComponent>();
        e.CreateComponent<EditorPythonBindings::PythonMarshalComponent>();
        e.Init();
        EXPECT_EQ(AZ::Entity::State::Init, e.GetState());
        e.Activate();
        EXPECT_EQ(AZ::Entity::State::Active, e.GetState());

        SimulateEditorBecomingInitialized();

        e.Deactivate();
    }

    TEST_F(PythonReflectionComponentTests, MakeSureTheAzlmbrModuleExists)
    {
        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();
        {
            auto m = pybind11::module::import("azlmbr");
            EXPECT_TRUE(m);
        }

        e.Deactivate();
    }

    TEST_F(PythonReflectionComponentTests, GetProxyCommand)
    {
        enum class LogTypes
        {
            Skip = 0,
            GotProxyFromPython
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "proxy_do_test"))
                {
                    return static_cast<int>(LogTypes::GotProxyFromPython);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionTestDoPrint handler;
        handler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.legacy.test as test
                v = test.PythonReflectionTestDoPrint_do_test('does_not_exist')
                test.PythonReflectionTestDoPrint_do_print(str(v))
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on GetProxyCommand with %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GotProxyFromPython)]);
    }

    // a non-named 'editor' flagged BehaviorClass methods end up in the 'azlmbr.default' namespace
    TEST_F(PythonReflectionComponentTests, DefaultModule)
    {
        enum class LogTypes
        {
            Skip = 0,
            PythonReflectionTestSimple_DoWork
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "PythonReflectionTestSimple_DoWork"))
                {
                    return static_cast<int>(LogTypes::PythonReflectionTestSimple_DoWork);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionTestSimple handler;
        handler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.default
                azlmbr.default.PythonReflectionTestSimple_do_work()
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on DefaultModule with %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectionTestSimple_DoWork)]);
    }

    // at least 3 deep in a sub module thing like test.that.space
    TEST_F(PythonReflectionComponentTests, AtLeast3DeepModules)
    {
        enum class LogTypes
        {
            Skip = 0,
            PythonReflectionTestSimple_DoWork,
            PythonReflection_DoPrint,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "PythonReflectionTestSimple_DoWork"))
                {
                    return static_cast<int>(LogTypes::PythonReflectionTestSimple_DoWork);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflection_DoPrint"))
                {
                    return static_cast<int>(LogTypes::PythonReflection_DoPrint);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionTestSimple pythonReflectionTestSimple;
        pythonReflectionTestSimple.Reflect(m_app.GetBehaviorContext());
        PythonReflectionTestDoPrint pythonReflectionTestDoPrint;
        pythonReflectionTestDoPrint.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.default
                import azlmbr.legacy.test
                azlmbr.default.PythonReflectionTestSimple_do_work()
                azlmbr.legacy.test.PythonReflectionTestDoPrint_do_print('PythonReflection_DoPrint')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on DefaultModule with %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectionTestSimple_DoWork)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflection_DoPrint)]);
    }

    // Access Mutate Any types
    TEST_F(PythonReflectionComponentTests, AccessMutateAny)
    {
        enum class LogTypes
        {
            Skip = 0,
            MutateAny,
            AccessAny,
            MutateAssetId,
            AccessAssetId
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "MutateAny"))
                {
                    return aznumeric_cast<int>(LogTypes::MutateAny);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "AccessAny"))
                {
                    return aznumeric_cast<int>(LogTypes::AccessAny);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "MutateAssetId"))
                {
                    return aznumeric_cast<int>(LogTypes::MutateAssetId);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "AccessAssetId"))
                {
                    return aznumeric_cast<int>(LogTypes::AccessAssetId);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        PythonReflectionAny pythonReflectionAny;
        pythonReflectionAny.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test as test
                testObject = test.PythonReflectionAny()

                # by reference
                testObject.theInt = 10
                reflectAny = test.PythonReflectionAny()
                reflectAny.mutate_any_ref(testObject)
                value = reflectAny.access_any_ref()
                if( value.theInt == 10 ):
                    print ('AccessAny')

                # by value
                testObject.theInt = testObject.theInt + 1
                reflectAny = test.PythonReflectionAny()
                reflectAny.mutate_any_value(testObject)
                value = reflectAny.access_any_value()
                if( value.theInt == 11 ):
                    print ('AccessAny')

                # access and mutate using an AssetId
                theAsset = testObject.theAsset
                reflectAny = test.PythonReflectionAny()
                reflectAny.mutate_any_ref(theAsset)
                theAsset = reflectAny.access_any_ref()
                if( reflectAny.compare_asset_ids(theAsset,testObject.theAsset) ):
                    print ('MutateAssetId')

            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed with %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(2, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::MutateAny)]);
        EXPECT_EQ(4, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::AccessAny)]);
        EXPECT_EQ(2, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::MutateAssetId)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::AccessAssetId)]);
    }

    TEST_F(PythonReflectionComponentTests, AccessMutateAnySimpleTypes)
    {
        enum class LogTypes
        {
            Skip = 0,
            MutateAnySimple,
            AccessAnySimple,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "MutateAnySimple"))
                {
                    return aznumeric_cast<int>(LogTypes::MutateAnySimple);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "AccessAnySimple"))
                {
                    return aznumeric_cast<int>(LogTypes::AccessAnySimple);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        PythonReflectionAny pythonReflectionAny;
        pythonReflectionAny.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test as test
                import math

                # access mutate float
                reflectAny = test.PythonReflectionAny()
                reflectAny.mutate_any_simple(float(10.0))
                if( math.floor(reflectAny.access_any_simple()) == 10.0 ):
                    print ('AccessAnySimple_double')

                # access mutate int
                reflectAny = test.PythonReflectionAny()
                reflectAny.mutate_any_simple(int(11))
                if( reflectAny.access_any_simple() == 11 ):
                    print ('AccessAnySimple_s64')

                # access mutate bool
                reflectAny = test.PythonReflectionAny()
                reflectAny.mutate_any_simple(False)
                if( reflectAny.access_any_simple() is not True ):
                    print ('AccessAnySimple_bool')

                # access mutate string
                reflectAny = test.PythonReflectionAny()
                reflectAny.mutate_any_simple('a string value')
                if( reflectAny.access_any_simple() == 'a string value' ):
                    print ('AccessAnySimple_string_view')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed with %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(4, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::MutateAnySimple)]);
        EXPECT_EQ(8, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::AccessAnySimple)]);
    }

    // test for the Python container types like List, Dictionary, Set, and Tuple
    TEST_F(PythonReflectionComponentTests, ContainerTypes)
    {
        enum class LogTypes
        {
            Skip = 0,
            ContainerTypes_Input,
            ContainerTypes_Output,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "ContainerTypes_Input"))
                {
                    return static_cast<int>(LogTypes::ContainerTypes_Input);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "ContainerTypes_Output"))
                {
                    return static_cast<int>(LogTypes::ContainerTypes_Output);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionContainerSimpleTypes PythonReflectionContainerSimpleTypes;
        PythonReflectionContainerSimpleTypes.Reflect(m_app.GetSerializeContext());
        PythonReflectionContainerSimpleTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.containers
                import azlmbr.object

                def real_number_list_equals(list1, list2):
                    for a, b in zip(list1, list2):
                        if abs(a-b) > 0.0001:
                            return False
                    return True

                def test_vector_of_reals(test, label, get, put, values):
                    list = test.invoke(get)
                    if (real_number_list_equals(values, list)):
                        print ('ContainerTypes_Output{}'.format(label))

                    list.reverse()
                    test.invoke(put, list)
                    list = test.invoke(get)
                    values.reverse()
                    if (real_number_list_equals(values, list)):
                        print ('ContainerTypes_Input{}'.format(label))

                def test_vector_of(test, label, values):
                    list = test.get_property('vector_of_{}'.format(label))
                    if (list == values):
                        print ('ContainerTypes_Output{}'.format(label))

                    list.reverse()
                    test.set_property('vector_of_{}'.format(label), list)
                    list = test.get_property('vector_of_{}'.format(label))
                    values.reverse()
                    if (list == values):
                        print ('ContainerTypes_Input{}'.format(label))

                test = azlmbr.object.create('PythonReflectionContainerSimpleTypes')

                test_vector_of_reals(test, 'doubles', 'return_vector_of_doubles', 'accept_vector_of_doubles', [ 1.0, 2.2, 3.3, 4.4 ])
                test_vector_of_reals(test, 'floats', 'return_vector_of_floats', 'accept_vector_of_floats', [ 1.0, 2.2, 3.3, 4.4 ])

                test_vector_of(test, 'bool', [True,False,False])
                test_vector_of(test, 's8', [4,5,6,7])
                test_vector_of(test, 'u8', [4,5,6,7])
                test_vector_of(test, 's16', [4,5,6,7])
                test_vector_of(test, 'u16', [4,5,6,7])
                test_vector_of(test, 's32', [4,5,6,7])
                test_vector_of(test, 'u32', [4,5,6,7])
                test_vector_of(test, 's64', [4,5,6,7])
                test_vector_of(test, 'u64', [4,5,6,7])
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(11, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::ContainerTypes_Input)]);
        EXPECT_EQ(11, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::ContainerTypes_Output)]);
    }

    // all accepted types tested with this test class
    struct PythonReflectionTypesTester
    {
        AZ_TYPE_INFO(PythonReflectionTestDoPrint, "{CA1146E2-A2DF-4AE3-A712-5333CE60D65C}");

        static AZ::u32 s_returned;
        static AZ::u32 s_accepted;
        static AZ::u32 s_successCount;
        static AZStd::any s_theValue;

        template <typename T>
        static T ReturnValue()
        {
            ++s_returned;
            return AZStd::any_cast<T>(s_theValue);
        }

        template <typename T>
        static void AcceptValue(T value)
        {
            s_theValue = value;
            ++s_accepted;
        }

        static void SignalSuccess()
        {
            ++s_successCount;
        }

        void Reflect(AZ::ReflectContext* context)
        {
            PythonReflectionTypesTester::s_accepted = 0;
            PythonReflectionTypesTester::s_returned = 0;
            PythonReflectionTypesTester::s_successCount = 0;
            PythonReflectionTypesTester::s_theValue = AZStd::any();

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionTypesTester>("TypeTests")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test.types")
                    ->Method("signal_success", &SignalSuccess, nullptr, "")
                    ->Method("return_bool", &ReturnValue<bool>, nullptr, "")
                    ->Method("accept_bool", &AcceptValue<bool>, nullptr, "")
                    ->Method("return_char", &ReturnValue<char>, nullptr, "")
                    ->Method("accept_char", &AcceptValue<char>, nullptr, "")
                    ->Method("return_float", &ReturnValue<float>, nullptr, "")
                    ->Method("accept_float", &AcceptValue<float>, nullptr, "")
                    ->Method("return_double", &ReturnValue<double>, nullptr, "")
                    ->Method("accept_double", &AcceptValue<double>, nullptr, "")
                    ->Method("return_s8", &ReturnValue<AZ::s8>, nullptr, "")
                    ->Method("accept_s8", &AcceptValue<AZ::s8>, nullptr, "")
                    ->Method("return_u8", &ReturnValue<AZ::u8>, nullptr, "")
                    ->Method("accept_u8", &AcceptValue<AZ::u8>, nullptr, "")
                    ->Method("return_s16", &ReturnValue<AZ::s16>, nullptr, "")
                    ->Method("accept_s16", &AcceptValue<AZ::s16>, nullptr, "")
                    ->Method("return_u16", &ReturnValue<AZ::u16>, nullptr, "")
                    ->Method("accept_u16", &AcceptValue<AZ::u16>, nullptr, "")
                    ->Method("return_s32", &ReturnValue<AZ::s32>, nullptr, "")
                    ->Method("accept_s32", &AcceptValue<AZ::s32>, nullptr, "")
                    ->Method("return_u32", &ReturnValue<AZ::u32>, nullptr, "")
                    ->Method("accept_u32", &AcceptValue<AZ::u32>, nullptr, "")
                    ->Method("return_s64", &ReturnValue<AZ::s64>, nullptr, "")
                    ->Method("accept_s64", &AcceptValue<AZ::s64>, nullptr, "")
                    ->Method("return_u64", &ReturnValue<AZ::u64>, nullptr, "")
                    ->Method("accept_u64", &AcceptValue<AZ::u64>, nullptr, "")
                    ->Method("return_vector4", &ReturnValue<AZ::Vector4>, nullptr, "")
                    ->Method("accept_vector4", &AcceptValue<AZ::Vector4>, nullptr, "")
                    ;
            }
        }
    };
    AZ::u32 PythonReflectionTypesTester::s_accepted = 0;
    AZ::u32 PythonReflectionTypesTester::s_returned = 0;
    AZ::u32 PythonReflectionTypesTester::s_successCount = 0;
    AZStd::any PythonReflectionTypesTester::s_theValue;

    TEST_F(PythonReflectionComponentTests, PythonReflectionTypes)
    {
        PythonReflectionTypesTester pythonReflectionTypesTester;
        pythonReflectionTypesTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            int testCount = 0;
            pybind11::exec(R"(import azlmbr.test.types)");

            pybind11::exec(R"(
                azlmbr.test.types.TypeTests_accept_bool(False)
                azlmbr.test.types.TypeTests_return_bool()
            )");
            ++testCount;
            EXPECT_EQ(testCount, PythonReflectionTypesTester::s_accepted);
            EXPECT_EQ(testCount, PythonReflectionTypesTester::s_returned);

            pybind11::exec(R"(
                azlmbr.test.types.TypeTests_accept_char(chr(97))
                azlmbr.test.types.TypeTests_return_char()
            )");
            ++testCount;
            EXPECT_EQ(testCount, PythonReflectionTypesTester::s_accepted);
            EXPECT_EQ(testCount, PythonReflectionTypesTester::s_returned);

            pybind11::exec(R"(
                azlmbr.test.types.TypeTests_accept_float(0.01)
                azlmbr.test.types.TypeTests_return_float()
            )");
            ++testCount;
            EXPECT_EQ(testCount, PythonReflectionTypesTester::s_accepted);
            EXPECT_EQ(testCount, PythonReflectionTypesTester::s_returned);

            pybind11::exec(R"(
                azlmbr.test.types.TypeTests_accept_double(0.1234)
                azlmbr.test.types.TypeTests_return_double()
            )");
            ++testCount;
            EXPECT_EQ(testCount, PythonReflectionTypesTester::s_accepted);
            EXPECT_EQ(testCount, PythonReflectionTypesTester::s_returned);

            const char* intTypes[]{ "s8", "u8", "s16", "u16", "s32", "u32", "s64", "u64" };
            for (const char* intType : intTypes)
            {
                AZStd::string intTypeTest = AZStd::string::format("azlmbr.test.types.TypeTests_accept_%s(1) \nazlmbr.test.types.TypeTests_return_%s() \n", intType, intType);
                pybind11::exec(intTypeTest.c_str());
                ++testCount;
                EXPECT_EQ(testCount, PythonReflectionTypesTester::s_accepted);
                EXPECT_EQ(testCount, PythonReflectionTypesTester::s_returned);
            }
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception %s", e.what());
            FAIL();
        }

        e.Deactivate();
    }

    TEST_F(PythonReflectionComponentTests, PythonReflectStringTypes)
    {
        if (AZ::SerializeContext* serializeContext = m_app.GetSerializeContext())
        {
            serializeContext->RegisterGenericType<AZStd::vector<AZStd::string>>();
            serializeContext->RegisterGenericType<AZStd::vector<AZStd::string_view>>();
        }

        PythonReflectionStringTypes pythonReflectionStringTypes;
        pythonReflectionStringTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            PythonReflectStringTypes_RawStringIn,
            PythonReflectStringTypes_RawStringOut,
            PythonReflectStringTypes_StringViewIn,
            PythonReflectStringTypes_StringViewOut,
            PythonReflectStringTypes_AZStdStringIn,
            PythonReflectStringTypes_AZStdStringOut,
            PythonReflectStringTypes_AZStdVectorStringOut,
            PythonReflectStringTypes_AZStdVectorStringIn,
            PythonReflectStringTypes_EmptyStringOut,
            PythonReflectStringTypes_EmptyStringIn,
            PythonReflectStringTypes_AZStdVectorStringViewOut,
            PythonReflectStringTypes_AZStdVectorStringViewIn
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_RawStringIn"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_RawStringIn);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_RawStringOut"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_RawStringOut);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_StringViewIn"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_StringViewIn);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_StringViewOut"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_StringViewOut);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_AZStdStringIn"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdStringIn);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_AZStdStringOut"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdStringOut);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_AZStdVectorStringOut"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdVectorStringOut);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_AZStdVectorStringIn"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdVectorStringIn);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_EmptyStringOut"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_EmptyStringOut);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_EmptyStringIn"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_EmptyStringIn);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_AZStdVectorStringViewOut"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdVectorStringViewOut);
                }
                else if (AzFramework::StringFunc::Equal(message, "PythonReflectStringTypes_AZStdVectorStringViewIn"))
                {
                    return static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdVectorStringViewIn);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.strings

                azlmbr.test.strings.Strings_raw_pointer_in('PythonReflectStringTypes_RawStringIn')
                print (azlmbr.test.strings.Strings_raw_pointer_out())

                azlmbr.test.strings.Strings_string_view_in('PythonReflectStringTypes_StringViewIn')
                print (azlmbr.test.strings.Strings_string_view_out())

                azlmbr.test.strings.Strings_azstd_string_in('PythonReflectStringTypes_AZStdStringIn')
                print (azlmbr.test.strings.Strings_azstd_string_out())

                stringList = azlmbr.test.strings.Strings_output_string_list()
                if (stringList[0] == 'one' and stringList[1] == 'two' and stringList[2] == 'three'):
                    print ('PythonReflectStringTypes_AZStdVectorStringOut')

                newStringList = ['1','2','3']
                if (azlmbr.test.strings.Strings_input_string_list(newStringList) == True):
                    print ('PythonReflectStringTypes_AZStdVectorStringIn')

                stringList = azlmbr.test.strings.Strings_output_string_view_list()
                if (stringList[0] == 'one' and stringList[1] == 'two' and stringList[2] == 'three'):
                    print ('PythonReflectStringTypes_AZStdVectorStringViewOut')

                newStringList = ['a','b','c']
                if (azlmbr.test.strings.Strings_input_string_view_list(newStringList) == True):
                    print ('PythonReflectStringTypes_AZStdVectorStringViewIn')

                emptyString = azlmbr.test.strings.Strings_output_empty_string()
                if (isinstance(emptyString, str) and len(emptyString) == 0):
                    print ('PythonReflectStringTypes_EmptyStringOut')

                emptyString = ''
                if (azlmbr.test.strings.Strings_input_empty_string(emptyString)):
                    print ('PythonReflectStringTypes_EmptyStringIn')

            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_RawStringIn)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_RawStringOut)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_StringViewIn)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_StringViewOut)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdStringIn)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdStringOut)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdVectorStringOut)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdVectorStringIn)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdVectorStringViewOut)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_AZStdVectorStringViewIn)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_EmptyStringOut)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PythonReflectStringTypes_EmptyStringIn)]);
    }

    TEST_F(PythonReflectionComponentTests, MathReflectionTests)
    {
        enum class LogTypes
        {
            Skip = 0,
            MathColor,
            MathStaticMembers
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "MathColor"))
                {
                    return static_cast<int>(LogTypes::MathColor);
                }
                else if (AzFramework::StringFunc::Equal(message, "MathStaticMembers"))
                {
                    return static_cast<int>(LogTypes::MathStaticMembers);
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
                import azlmbr.math as math
                import azlmbr.object
                # testing math type Color
                color = azlmbr.object.create('Color')
                if( color is not None ):
                    print ('MathColor')
                color = azlmbr.object.construct('Color', 0.15, 0.25, 0.35, 0.45)
                if( color is not None ):
                    print ('MathColor')
                if( math.Math_IsClose(color.r, 0.15) == True):
                    print ('MathColor')
                if( math.Math_IsClose(color.g, 0.25) == True):
                    print ('MathColor')
                if( math.Math_IsClose(color.b, 0.35) == True):
                    print ('MathColor')
                if( math.Math_IsClose(color.a, 0.45) == True):
                    print ('MathColor')
                color.r = 0.51
                color.g = 0.52
                color.b = 0.53
                color.a = 0.54
                if( math.Math_IsClose(color.r, 0.51) == True):
                    print ('MathColor')
                if( math.Math_IsClose(color.g, 0.52) == True):
                    print ('MathColor')
                if( math.Math_IsClose(color.b, 0.53) == True):
                    print ('MathColor')
                if( math.Math_IsClose(color.a, 0.54) == True):
                    print ('MathColor')
                # testing the Vector3 math type member like functions
                vec3 = azlmbr.object.create('Vector3')
                vec3.x = 0.0
                vec3.y = 0.0
                vec3.z = 0.0
                if( vec3.ToString() == '(x=0.0000000,y=0.0000000,z=0.0000000)'):
                    print ('MathStaticMembers')
                # testing the Uuid math type member like functions
                uuidString = '{E866B520-D667-48A2-82F6-6AEBE1EC9C58}'
                uuid = azlmbr.math.Uuid_CreateString(uuidString, 0)
                if( uuid.ToString() == uuidString):
                    print ('MathStaticMembers')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZStd::string failReason = AZStd::string::format("Failed on DefaultModule with %s", e.what());
            GTEST_FATAL_FAILURE_(failReason.c_str());
        }

        e.Deactivate();

        EXPECT_EQ(10, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::MathColor)]);
        EXPECT_EQ(2, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::MathStaticMembers)]);
    }

    TEST_F(PythonReflectionComponentTests, ComplexContainers)
    {
        PythonReflectionComplexContainer pythonReflectionComplexContainer;
        pythonReflectionComplexContainer.Reflect(m_app.GetBehaviorContext());
        pythonReflectionComplexContainer.Reflect(m_app.GetSerializeContext());

        enum class LogTypes
        {
            Skip = 0,
            GotListOfIds
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "GotListOfIds"))
                {
                    return static_cast<int>(LogTypes::GotListOfIds);
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
                import azlmbr.test
                import azlmbr.object
                container = azlmbr.object.create('ComplexContainer')
                entityIdList = container.send_list_of_ids()
                if( len(entityIdList) == 3):
                    print ('GotListOfIds')
                if( entityIdList[0].ToString() == '[101]'):
                    print ('GotListOfIds')
                if( entityIdList[1].ToString() == '[202]'):
                    print ('GotListOfIds')
                if( entityIdList[2].ToString() == '[303]'):
                    print ('GotListOfIds')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on DefaultModule with %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(4, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::GotListOfIds)]);
    }

    TEST_F(PythonReflectionComponentTests, ProjectPaths)
    {
        enum class LogTypes
        {
            Skip = 0,
            EngrootIs,
            pathResolvedTo,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                AZStd::string_view m(message);
                if (AzFramework::StringFunc::StartsWith(message, "engroot is "))
                {
                    return static_cast<int>(LogTypes::EngrootIs);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "path resolved to "))
                {
                    return static_cast<int>(LogTypes::pathResolvedTo);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.CreateComponent<EditorPythonBindings::PythonReflectionComponent>();
        e.CreateComponent<EditorPythonBindings::PythonMarshalComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.paths
                if (len(azlmbr.paths.engroot) != 0):
                   print ('engroot is {}'.format(azlmbr.paths.engroot))

                path = azlmbr.paths.resolve_path('@engroot@/engineassets/texturemsg/defaultsolids.mtl')
                if (path.find('@engroot@') == -1):
                    print ('path resolved to {}'.format(path))
           )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::EngrootIs], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::pathResolvedTo], 1);
    }
}
