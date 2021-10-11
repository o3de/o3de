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

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    struct PythonReflectionDictionaryTypes final
    {
        AZ_TYPE_INFO(PythonReflectionDictionaryTypes, "{478AD363-467D-4285-BE40-4D1CB1A09A19}");

        template <typename K, typename V>
        struct MapOf
        {
            using MapType = AZStd::unordered_map<K,V>;
            MapType m_map;

            explicit MapOf(const std::initializer_list<AZStd::pair<K, V>> map)
            {
                m_map = map;
            }

            const MapType& ReturnMap() const
            {
                return m_map;
            }

            void AcceptMap(const MapType& other)
            {
                m_map = other;
            }

            void RegisterGenericType(AZ::SerializeContext& serializeContext)
            {
                serializeContext.RegisterGenericType<AZStd::unordered_map<K,V>>();
            }
        };

        MapOf<AZ::u8, AZ::u32> m_indexOfu8tou32 { {AZ::u8(1), 4u}, {AZ::u8(2), 5u}, {AZ::u8(3), 6u}, {AZ::u8(4), 7u} };
        MapOf<AZ::u16, float> m_indexOfu16toFloat { {AZ::u16(1u), 0.4f}, {AZ::u16(2u), 0.5f}, {AZ::u16(3u), 0.6f}, {AZ::u16(4u), 0.7f} };
        MapOf<AZStd::string, AZ::s32> m_indexOfStringTos32 { {"1", -4}, {"2", 5}, {"3", -6}, {"4", 7} };
        MapOf<AZStd::string, AZStd::string> m_indexOfStringToString { {"hello", "foo"}, {"world", "bar"}, {"bye", "baz"}, {"sky", "qux"} };
        MapOf<AZStd::string, AZ::Vector3> m_indexOfStringToVec3{ {"up", AZ::Vector3{ 0, 1.0, 0 }}, {"down", AZ::Vector3{0, -1.0, 0}},
                                                                 {"left", AZ::Vector3{1.0, 0, 0}}, {"right", AZ::Vector3{-1, 0, 0}} };
        
        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                m_indexOfu8tou32.RegisterGenericType(*serializeContext);
                m_indexOfu16toFloat.RegisterGenericType(*serializeContext);
                m_indexOfStringTos32.RegisterGenericType(*serializeContext);
                m_indexOfStringToString.RegisterGenericType(*serializeContext);
                m_indexOfStringToVec3.RegisterGenericType(*serializeContext);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionDictionaryTypes>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test.dictionary")
                    ->Method("return_dict_of_u8u32", [](PythonReflectionDictionaryTypes* self) { return self->m_indexOfu8tou32.ReturnMap(); }, nullptr, "")
                    ->Method("accept_dict_of_u8u32", [](PythonReflectionDictionaryTypes* self, const MapOf<AZ::u8, AZ::u32>::MapType& map) { self->m_indexOfu8tou32.AcceptMap(map); }, nullptr, "")
                    ->Method("return_dict_of_u16toFloat", [](PythonReflectionDictionaryTypes* self) { return self->m_indexOfu16toFloat.ReturnMap(); }, nullptr, "")
                    ->Method("accept_dict_of_u16toFloat", [](PythonReflectionDictionaryTypes* self, const MapOf<AZ::u16, float>::MapType& map) { self->m_indexOfu16toFloat.AcceptMap(map); }, nullptr, "")
                    ->Method("return_dict_of_stringTos32", [](PythonReflectionDictionaryTypes* self) { return self->m_indexOfStringTos32.ReturnMap(); }, nullptr, "")
                    ->Method("accept_dict_of_stringTos32", [](PythonReflectionDictionaryTypes* self, const MapOf<AZStd::string, AZ::s32>::MapType& map) { self->m_indexOfStringTos32.AcceptMap(map); }, nullptr, "")
                    ->Method("return_dict_of_stringToString", [](PythonReflectionDictionaryTypes* self) { return self->m_indexOfStringToString.ReturnMap(); }, nullptr, "")
                    ->Method("accept_dict_of_stringToString", [](PythonReflectionDictionaryTypes* self, const MapOf<AZStd::string, AZStd::string>::MapType& map) { self->m_indexOfStringToString.AcceptMap(map); }, nullptr, "")
                    ->Method("return_dict_of_stringToVec3", [](PythonReflectionDictionaryTypes* self) { return self->m_indexOfStringToVec3.ReturnMap(); }, nullptr, "")
                    ->Method("accept_dict_of_stringToVec3", [](PythonReflectionDictionaryTypes* self, const MapOf<AZStd::string, AZ::Vector3>::MapType& map) { self->m_indexOfStringToVec3.AcceptMap(map); }, nullptr, "")
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonReflectionDictionaryTests
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

    TEST_F(PythonReflectionDictionaryTests, InstallingPythonDictionaries)
    {
        AZ::Entity e;
        Activate(e);
        EXPECT_EQ(AZ::Entity::State::Active, e.GetState());
        SimulateEditorBecomingInitialized();
        e.Deactivate();
    }

    TEST_F(PythonReflectionDictionaryTests, MapSimpleTypes)
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

        PythonReflectionDictionaryTypes pythonReflectionDictionaryTypes;
        pythonReflectionDictionaryTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionDictionaryTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.dictionary
                import azlmbr.object

                test = azlmbr.object.create('PythonReflectionDictionaryTypes')
                result = test.return_dict_of_u8u32()
                if (len(result.items()) == 4):
                    print ('ContainerTypes_Output_u8u32')
                test.accept_dict_of_u8u32({4: 1, 3: 2})
                result = test.return_dict_of_u8u32()
                if (len(result.items()) == 2):
                    print ('ContainerTypes_Input_u8u32')

                result = test.return_dict_of_u16toFloat()
                if (len(result.items()) == 4):
                    print ('ContainerTypes_Output_u16toFloat')
                test.accept_dict_of_u16toFloat({4: 0.1, 3: 0.2})
                result = test.return_dict_of_u16toFloat()
                if (len(result.items()) == 2):
                    print ('ContainerTypes_Input_u16toFloat')

                result = test.return_dict_of_stringTos32()
                if (len(result.items()) == 4):
                    print ('ContainerTypes_Output_stringTos32')
                test.accept_dict_of_stringTos32({'4': -1, '3': 2})
                result = test.return_dict_of_stringTos32()
                if (len(result.items()) == 2):
                    print ('ContainerTypes_Input_stringTos32')

                result = test.return_dict_of_stringToString()
                if (len(result.items()) == 4):
                    print ('ContainerTypes_Output_stringToString')
                test.accept_dict_of_stringToString({'one': '1', 'two': '2'})
                result = test.return_dict_of_stringToString()
                if (len(result.items()) == 2):
                    print ('ContainerTypes_Input_stringToString')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(4, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::ContainerTypes_Input)]);
        EXPECT_EQ(4, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::ContainerTypes_Output)]);
    }

    TEST_F(PythonReflectionDictionaryTests, MapTypes_Mismatch_Detected)
    {
        enum class LogTypes
        {
            Skip = 0,
            Detection,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            constexpr AZStd::string_view warningTypeMismatch =
                "Could not convert to pair element type value2 for the pair<>; failed to marshal Python input <class 'int'>";
            constexpr AZStd::string_view warningSizeMismatch =
                "Python Dict size:2 does not match the size of the unordered_map:0";

            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, warningTypeMismatch))
                {
                    return aznumeric_cast<int>(LogTypes::Detection);
                }
                else if (AzFramework::StringFunc::StartsWith(message, warningSizeMismatch))
                {
                    return aznumeric_cast<int>(LogTypes::Detection);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        PythonReflectionDictionaryTypes pythonReflectionDictionaryTypes;
        pythonReflectionDictionaryTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionDictionaryTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.dictionary
                import azlmbr.object

                test = azlmbr.object.create('PythonReflectionDictionaryTypes')

                mismatchMap = {'one': 1, 'two': 2}      
                test.accept_dict_of_stringToString(mismatchMap)
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed with Python exception of %s", e.what());
        }

        e.Deactivate();

        EXPECT_EQ(3, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::Detection)]);
    }

    TEST_F(PythonReflectionDictionaryTests, MapComplexTypes)
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

        PythonReflectionDictionaryTypes pythonReflectionDictionaryTypes;
        pythonReflectionDictionaryTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionDictionaryTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.dictionary
                import azlmbr.object

                test = azlmbr.object.create('PythonReflectionDictionaryTypes')

                result = test.return_dict_of_stringToVec3()
                if (len(result.items()) == 4):
                    print ('ContainerTypes_Output_stringToVec3')
                vec3dict = {}
                vec3dict['120'] = azlmbr.math.Vector3(1.0, -2.0, 0.0)
                vec3dict['456'] = azlmbr.math.Vector3(0.4, 0.5, 0.6)
                test.accept_dict_of_stringToVec3(vec3dict)
                result = test.return_dict_of_stringToVec3()
                if (len(result.items()) == 2):
                    if (result['120'].x > 0 and result['120'].y < 0 and result['120'].z == 0):
                        print ('ContainerTypes_Input_stringToVec3')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::ContainerTypes_Input)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::ContainerTypes_Output)]);
    }
}
