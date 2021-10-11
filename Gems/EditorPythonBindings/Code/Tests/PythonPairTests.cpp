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
#include <Source/PythonProxyObject.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include "PythonPairTests.h"

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // test class/structs

    struct PythonReflectionPairTypes
    {
        AZ_TYPE_INFO(PythonReflectionPairTypes, "{037C067F-7A03-47BE-A30E-124D8157EDA2}");

        template <typename K, typename V>
        struct PairOf
        {
            using PairType = AZStd::pair<K, V>;
            PairType m_pair;

            explicit PairOf(const PairType& pair)
            {
                m_pair = pair;
            }

            explicit PairOf(const K& k, const V& v)
            {
                m_pair = PairType(k, v);
            }

            const PairType& ReturnPair() const
            {
                return m_pair;
            }

            void AcceptPair(const PairType& other)
            {
                m_pair = other;
            }

            void RegisterGenericType(AZ::SerializeContext& serializeContext)
            {
                serializeContext.RegisterGenericType<PairType>();
            }
        };

        PairOf<bool, bool> m_pairOfBoolToBool { false, true };
        PairOf<AZ::u8, AZ::u32> m_pairOfu8tou32 {1, 4};
        PairOf<AZ::u16, float> m_pairOfu16toFloat {1, 0.4f};
        PairOf<AZStd::string, AZ::s32> m_pairOfStringTos32 {"1", -4};
        PairOf<AZStd::string, AZStd::string> m_pairOfStringToString {"one", "foo"};
        PairOf<AZStd::string, MyCustomType> m_pairOfStringToCustomType{ "foo", MyCustomType() };

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                m_pairOfBoolToBool.RegisterGenericType(*serializeContext);
                m_pairOfu8tou32.RegisterGenericType(*serializeContext);
                m_pairOfu16toFloat.RegisterGenericType(*serializeContext);
                m_pairOfStringTos32.RegisterGenericType(*serializeContext);
                m_pairOfStringToString.RegisterGenericType(*serializeContext);
                m_pairOfStringToCustomType.RegisterGenericType(*serializeContext);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionPairTypes>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test.pair")
                    ->Method("return_pair_of_boolToBool", [](PythonReflectionPairTypes* self) { return self->m_pairOfBoolToBool.ReturnPair(); }, nullptr, "")
                    ->Method("accept_pair_of_boolToBool", [](PythonReflectionPairTypes* self, const PairOf<bool, bool>::PairType& pair) { self->m_pairOfBoolToBool.AcceptPair(pair); }, nullptr, "")
                    ->Method("return_pair_of_u8u32", [](PythonReflectionPairTypes* self) { return self->m_pairOfu8tou32.ReturnPair(); }, nullptr, "")
                    ->Method("accept_pair_of_u8u32", [](PythonReflectionPairTypes* self, const PairOf<AZ::u8, AZ::u32>::PairType& pair) { self->m_pairOfu8tou32.AcceptPair(pair); }, nullptr, "")
                    ->Method("return_pair_of_u16toFloat", [](PythonReflectionPairTypes* self) { return self->m_pairOfu16toFloat.ReturnPair(); }, nullptr, "")
                    ->Method("accept_pair_of_u16toFloat", [](PythonReflectionPairTypes* self, const PairOf<AZ::u16, float>::PairType& pair) { self->m_pairOfu16toFloat.AcceptPair(pair); }, nullptr, "")
                    ->Method("return_pair_of_stringTos32", [](PythonReflectionPairTypes* self) { return self->m_pairOfStringTos32.ReturnPair(); }, nullptr, "")
                    ->Method("accept_pair_of_stringTos32", [](PythonReflectionPairTypes* self, const PairOf<AZStd::string, AZ::s32>::PairType& pair) { self->m_pairOfStringTos32.AcceptPair(pair); }, nullptr, "")
                    ->Method("return_pair_of_stringToString", [](PythonReflectionPairTypes* self) { return self->m_pairOfStringToString.ReturnPair(); }, nullptr, "")
                    ->Method("accept_pair_of_stringToString", [](PythonReflectionPairTypes* self, const PairOf<AZStd::string, AZStd::string>::PairType& pair) { self->m_pairOfStringToString.AcceptPair(pair); }, nullptr, "")
                    ->Method("return_pair_of_stringToCustomType", [](PythonReflectionPairTypes* self) { return self->m_pairOfStringToCustomType.ReturnPair(); }, nullptr, "")
                    ->Method("accept_pair_of_stringToCustomType", [](PythonReflectionPairTypes* self, const PairOf<AZStd::string, MyCustomType>::PairType& pair) { self->m_pairOfStringToCustomType.AcceptPair(pair); }, nullptr, "")
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonReflectionPairTests
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

    TEST_F(PythonReflectionPairTests, SimpleTypes_Constructed)
    {
        enum class LogTypes
        {
            Skip = 0,
            PairTypeTest_ConstructBoolDefault,
            PairTypeTest_ConstructBoolParams,
            PairTypeTest_UseConstructed
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "PairTypeTest_ConstructBoolDefault"))
                {
                    return static_cast<int>(LogTypes::PairTypeTest_ConstructBoolDefault);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "PairTypeTest_ConstructBoolParams"))
                {
                    return static_cast<int>(LogTypes::PairTypeTest_ConstructBoolParams);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "PairTypeTest_UseConstructed"))
                {
                    return static_cast<int>(LogTypes::PairTypeTest_UseConstructed);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        MyCustomType::Reflect(m_app.GetSerializeContext());
        MyCustomType::Reflect(m_app.GetBehaviorContext());

        PythonReflectionPairTypes pythonReflectionPairTypes;
        pythonReflectionPairTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionPairTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.pair
                import azlmbr.object
                import azlmbr.std

                test = azlmbr.object.create('PythonReflectionPairTypes')
                test_pair = azlmbr.object.create('AZStd::pair<bool, bool>')
                if (test_pair):
                    print ('PairTypeTest_ConstructBoolDefault')

                test_pair = azlmbr.object.construct('AZStd::pair<bool, bool>', True, False)
                if (test_pair and test_pair.first == True and test_pair.second == False):
                    print ('PairTypeTest_ConstructBoolParams')

                test_pair.first = False
                test_pair.second = True

                test.accept_pair_of_boolToBool(test_pair)
                result = test.return_pair_of_boolToBool()
                if (len(result) == 2 and result[0] == False and result[1] == True):
                    print ('PairTypeTest_UseConstructed')
            )");
        }
        catch ([[maybe_unused]] const std::exception& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PairTypeTest_ConstructBoolDefault)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PairTypeTest_ConstructBoolParams)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PairTypeTest_UseConstructed)]);
    }

    TEST_F(PythonReflectionPairTests, SimpleTypes_ConvertedCorrectly)
    {
        enum class LogTypes
        {
            Skip = 0,
            PairTypeTest_Input,
            PairTypeTest_Output,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "PairTypeTest_Input"))
                {
                    return static_cast<int>(LogTypes::PairTypeTest_Input);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "PairTypeTest_Output"))
                {
                    return static_cast<int>(LogTypes::PairTypeTest_Output);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        MyCustomType::Reflect(m_app.GetSerializeContext());
        MyCustomType::Reflect(m_app.GetBehaviorContext());

        PythonReflectionPairTypes pythonReflectionPairTypes;
        pythonReflectionPairTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionPairTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.pair
                import azlmbr.object
                import azlmbr.std

                test = azlmbr.object.create('PythonReflectionPairTypes')
                result = test.return_pair_of_u8u32()
                if (len(result) == 2):
                    print ('PairTypeTest_Output_u8u32')

                test.accept_pair_of_u8u32([42, 0])
                result = test.return_pair_of_u8u32()
                if (len(result) == 2 and result[0] == 42 and result[1] == 0):
                        print ('PairTypeTest_Input_u8u32_list')

                test.accept_pair_of_u8u32((1, 2))
                result = test.return_pair_of_u8u32()
                if (len(result) == 2 and result[0] == 1 and result[1] == 2):
                        print ('PairTypeTest_Input_u8u32')

                result = test.return_pair_of_u16toFloat()
                if (len(result) == 2):
                    print ('PairTypeTest_Output_u16toFloat')
                test.accept_pair_of_u16toFloat((4, -0.01))
                result = test.return_pair_of_u16toFloat()
                if (len(result) == 2 and result[0] == 4 and result[1] < 0):
                    print ('PairTypeTest_Input_u16toFloat')

                result = test.return_pair_of_stringTos32()
                if (len(result) == 2):
                    print ('PairTypeTest_Output_stringTos32')
                test.accept_pair_of_stringTos32(('abc', -1))
                result = test.return_pair_of_stringTos32()
                if (len(result) == 2 and result[0] == 'abc' and result[1] == -1):
                    print ('PairTypeTest_Input_stringTos32')

                result = test.return_pair_of_stringToString()
                if (len(result) == 2):
                    print ('PairTypeTest_Output_stringToString')
                test.accept_pair_of_stringToString(('one', 'two'))
                result = test.return_pair_of_stringToString()
                if (len(result) == 2 and result[0] == 'one' and result[1] == 'two'):
                    print ('PairTypeTest_Input_stringToString')
            )");
        }
        catch ([[maybe_unused]] const std::exception& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(5, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PairTypeTest_Input)]);
        EXPECT_EQ(4, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PairTypeTest_Output)]);
    }

    TEST_F(PythonReflectionPairTests, CustomTypes_ConvertedCorrectly)
    {
        enum class LogTypes
        {
            Skip = 0,
            PairCustomTypeTest_Input,
            PairCustomTypeTest_Output,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "PairCustomTypeTest_Input"))
                {
                    return static_cast<int>(LogTypes::PairCustomTypeTest_Input);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "PairCustomTypeTest_Output"))
                {
                    return static_cast<int>(LogTypes::PairCustomTypeTest_Output);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        MyCustomType::Reflect(m_app.GetSerializeContext());
        MyCustomType::Reflect(m_app.GetBehaviorContext());

        PythonReflectionPairTypes pythonReflectionPairTypes;
        pythonReflectionPairTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionPairTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.pair
                import azlmbr.object
                import azlmbr.std

                test = azlmbr.object.create('PythonReflectionPairTypes')
                result = test.return_pair_of_stringToCustomType()
                if (len(result) == 2):
                    print ('PairCustomTypeTest_Output_stringToCustomType')

                custom = azlmbr.object.create('MyCustomType')
                custom.set_data(42)
                test.accept_pair_of_stringToCustomType(('def', custom))
                result = test.return_pair_of_stringToCustomType()
                if (len(result) == 2):
                    if (result[0] == 'def' and result[1].get_data() == 42):
                        print ('PairCustomTypeTest_Input_stringToCustomType_tuple')
            )");
        }
        catch ([[maybe_unused]] const std::exception& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PairCustomTypeTest_Input)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PairCustomTypeTest_Output)]);
    }

    TEST_F(PythonReflectionPairTests, UnsupportedTypes_ErrorLogged)
    {
        enum class LogTypes
        {
            Skip = 0,
            PairUnsupportedTypeTest_CannotConvert
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "Cannot convert pair container for"))
                {
                    return static_cast<int>(LogTypes::PairUnsupportedTypeTest_CannotConvert);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        MyCustomType::Reflect(m_app.GetSerializeContext());
        MyCustomType::Reflect(m_app.GetBehaviorContext());

        PythonReflectionPairTypes pythonReflectionDictionaryTypes;
        pythonReflectionDictionaryTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionDictionaryTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.pair
                import azlmbr.object
                import azlmbr.std

                test = azlmbr.object.create('PythonReflectionPairTypes')

                test.accept_pair_of_u8u32([42, 0, 1])
                test.accept_pair_of_u8u32({42, 0})
            )");
        }
        catch ([[maybe_unused]] const std::exception& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(2, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::PairUnsupportedTypeTest_CannotConvert)]);
    }
}


