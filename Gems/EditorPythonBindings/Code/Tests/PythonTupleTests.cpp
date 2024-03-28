/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

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

    struct PythonReflectionTupleTypes
    {
        AZ_TYPE_INFO(PythonReflectionTupleTypes, "{D5C9223B-8F12-49A9-8EDF-603357C3A6DF}");

        template <typename... Args>
        struct TupleOf
        {
            using TupleType = AZStd::tuple<Args...>;
            TupleType m_tuple;

            explicit TupleOf(const TupleType& tuple)
            {
                m_tuple = tuple;
            }

            const TupleType& ReturnTuple() const
            {
                return m_tuple;
            }

            void AcceptTuple(const TupleType& other)
            {
                m_tuple = other;
            }

            void RegisterGenericType(AZ::SerializeContext& serializeContext)
            {
                serializeContext.RegisterGenericType<TupleType>();
            }
        };

        TupleOf<> m_tupleOfEmptiness{ AZStd::make_tuple() };
        TupleOf<bool, int, float> m_tupleOfBasicTypes{ AZStd::make_tuple(true, 2, 3.0f) };
        TupleOf<AZStd::string, AZStd::string> m_tupleOfStrings { AZStd::make_tuple("one", "two")};
        TupleOf<bool, AZStd::string, MyCustomType> m_tupleWithCustomType{ AZStd::make_tuple(true, "one", MyCustomType()) };

        void Reflect(AZ::ReflectContext* context)
        {
            MyCustomType::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                m_tupleOfEmptiness.RegisterGenericType(*serializeContext);
                m_tupleOfBasicTypes.RegisterGenericType(*serializeContext);
                m_tupleOfStrings.RegisterGenericType(*serializeContext);
                m_tupleWithCustomType.RegisterGenericType(*serializeContext);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionTupleTypes>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test.tuple")
                    ->Method(
                        "return_empty_tuple",
                        [](PythonReflectionTupleTypes* self)
                        {
                            return self->m_tupleOfEmptiness.ReturnTuple();
                        },
                        nullptr, "")
                    ->Method(
                        "accept_empty_tuple",
                        [](PythonReflectionTupleTypes* self, const TupleOf<>::TupleType& tuple)
                        {
                            self->m_tupleOfEmptiness.AcceptTuple(tuple);
                        },
                        nullptr, "")
                    ->Method(
                        "return_tuple_of_basic_types",
                        [](PythonReflectionTupleTypes* self)
                        {
                            return self->m_tupleOfBasicTypes.ReturnTuple();
                        },
                        nullptr, "")
                    ->Method(
                        "accept_tuple_of_basic_types",
                        [](PythonReflectionTupleTypes* self, const TupleOf<bool, int, float>::TupleType& tuple)
                        {
                            self->m_tupleOfBasicTypes.AcceptTuple(tuple);
                        },
                        nullptr, "")
                    ->Method(
                        "return_tuple_of_strings",
                        [](PythonReflectionTupleTypes* self)
                        {
                            return self->m_tupleOfStrings.ReturnTuple();
                        },
                        nullptr, "")
                    ->Method(
                        "accept_tuple_of_strings",
                        [](PythonReflectionTupleTypes* self, const TupleOf<AZStd::string, AZStd::string>::TupleType& tuple)
                        {
                            self->m_tupleOfStrings.AcceptTuple(tuple);
                        },
                        nullptr, "")
                    ->Method(
                        "return_tuple_with_custom_type",
                        [](PythonReflectionTupleTypes* self)
                        {
                            return self->m_tupleWithCustomType.ReturnTuple();
                        },
                        nullptr, "")
                    ->Method(
                        "accept_tuple_with_custom_type",
                        [](PythonReflectionTupleTypes* self, const TupleOf<bool, AZStd::string, MyCustomType>::TupleType& tuple)
                        {
                            self->m_tupleWithCustomType.AcceptTuple(tuple);
                        },
                        nullptr, "")
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonReflectionTupleTests
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

    TEST_F(PythonReflectionTupleTests, SimpleTuplesConstructed)
    {
        enum class LogTypes
        {
            Skip = 0,
            TupleTypeTest_ConstructWithDefaultValues,
            TupleTypeTest_ConstructWithParameters,
            TupleTypeTest_UseConstructedAsParameterToCpp,
            TupleTypeTest_ReturnTupleAsListFromCpp,
            TupleTypeTest_TupleReturnedAsListWithCorrectValues,
            TupleTypeTest_TupleReturnedWithCorrectValues,
            TupleTypeTest_UsePythonListAsParameter,
            TupleTypeTest_PythonListReturnedWithCorrectValues
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_ConstructWithDefaultValues"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_ConstructWithDefaultValues);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_ConstructWithParameters"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_ConstructWithParameters);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_UseConstructedAsParameterToCpp"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_UseConstructedAsParameterToCpp);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_ReturnTupleAsListFromCpp"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_ReturnTupleAsListFromCpp);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_TupleReturnedAsListWithCorrectValues"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_TupleReturnedAsListWithCorrectValues);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_TupleReturnedWithCorrectValues"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_TupleReturnedWithCorrectValues);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_UsePythonListAsParameter"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_UsePythonListAsParameter);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_PythonListReturnedWithCorrectValues"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_PythonListReturnedWithCorrectValues);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionTupleTypes pythonReflectionTupleTypes;
        pythonReflectionTupleTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionTupleTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.tuple
                import azlmbr.object
                import azlmbr.std

                # Create the test fixture
                test = azlmbr.object.create('PythonReflectionTupleTypes')

                # Create a tuple with default values
                test_tuple = azlmbr.object.create('AZStd::tuple<bool, int, float>')
                if (test_tuple):
                    print ('TupleTypeTest_ConstructWithDefaultValues')

                # Create a tuple with parameters and verify that the parameters can be read back correctly.
                test_tuple = azlmbr.object.construct('AZStd::tuple<bool, int, float>', True, 5, 10.0)
                if (test_tuple and test_tuple.Get0() == True and test_tuple.Get1() == 5 and test_tuple.Get2() == 10.0):
                    print ('TupleTypeTest_ConstructWithParameters')

                # Use the tuple as a parameter to a reflected C++ method
                test.accept_tuple_of_basic_types(test_tuple)
                print ('TupleTypeTest_UseConstructedAsParameterToCpp')

                # Test out the tuple as a single return value that's a list
                result = test.return_tuple_of_basic_types()
                if (result and len(result) == 3):
                    print ('TupleTypeTest_ReturnTupleAsListFromCpp')

                # Verify the results that were returned are the same ones we sent in.
                if (result and len(result) == 3 and result[0] == True and result[1] == 5 and result[2] == 10.0):
                    print ('TupleTypeTest_TupleReturnedAsListWithCorrectValues')

                # Test out the tuple as comma-separated return values extracted from the list
                a, b, c = test.return_tuple_of_basic_types()
                if (a == True and b == 5 and c == 10.0):
                    print ('TupleTypeTest_TupleReturnedWithCorrectValues')

                # Use the tuple as a parameter to a reflected C++ method
                test.accept_tuple_of_basic_types([False, 10, 20.0])
                print ('TupleTypeTest_UsePythonListAsParameter')

                a, b, c = test.return_tuple_of_basic_types()
                if (a == False and b == 10 and c == 20.0):
                    print ('TupleTypeTest_PythonListReturnedWithCorrectValues')

            )");
        } catch ([[maybe_unused]] const pybind11::error_already_set& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_ConstructWithDefaultValues)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_ConstructWithParameters)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_UseConstructedAsParameterToCpp)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_ReturnTupleAsListFromCpp)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_TupleReturnedAsListWithCorrectValues)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_TupleReturnedWithCorrectValues)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_UsePythonListAsParameter)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_PythonListReturnedWithCorrectValues)]);
    }

    TEST_F(PythonReflectionTupleTests, EmptyTupleConvertedCorrectly)
    {
        enum class LogTypes
        {
            Skip = 0,
            TupleTypeTest_Input,
            TupleTypeTest_Output,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_Input"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_Input);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_Output"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_Output);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionTupleTypes pythonReflectionTupleTypes;
        pythonReflectionTupleTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionTupleTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.tuple
                import azlmbr.object
                import azlmbr.std

                # Create the test fixture
                test = azlmbr.object.create('PythonReflectionTupleTypes')

                # Verify that an empty tuple is returned correctly
                result = test.return_empty_tuple()
                if (len(result) == 0):
                    print ('TupleTypeTest_Output_empty')

                # Create a tuple from a Python list and verify the values are correct
                test.accept_empty_tuple([])
                result = test.return_empty_tuple()
                if (len(result) == 0):
                    print ('TupleTypeTest_Input_empty_list')

                # Create a tuple from a Python tuple and verify the values are correct
                test.accept_empty_tuple(())
                result = test.return_empty_tuple()
                if (len(result) == 0):
                    print ('TupleTypeTest_Input_empty')
            )");
        } catch ([[maybe_unused]] const pybind11::error_already_set& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(2, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_Input)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_Output)]);
    }

    TEST_F(PythonReflectionTupleTests, InputsAndOutputsConvertedCorrectly)
    {
        enum class LogTypes
        {
            Skip = 0,
            TupleTypeTest_Input,
            TupleTypeTest_Output,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_Input"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_Input);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleTypeTest_Output"))
                {
                    return static_cast<int>(LogTypes::TupleTypeTest_Output);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionTupleTypes pythonReflectionTupleTypes;
        pythonReflectionTupleTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionTupleTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.tuple
                import azlmbr.object
                import azlmbr.std

                # Create the test fixture
                test = azlmbr.object.create('PythonReflectionTupleTypes')

                # Verify that a tuple of basic types (bool, int, float) is returned correctly
                result = test.return_tuple_of_basic_types()
                if (len(result) == 3):
                    print ('TupleTypeTest_Output_bool_int_float')

                # Create a tuple from a Python list and verify the values are correct
                test.accept_tuple_of_basic_types([True, 42, 1000.0])
                result = test.return_tuple_of_basic_types()
                if (len(result) == 3 and result[0] == True and result[1] == 42 and result[2] == 1000.0):
                        print ('TupleTypeTest_Input_bool_int_float_list')

                # Create a tuple from a Python tuple and verify the values are correct
                test.accept_tuple_of_basic_types((False, 24, -25.0))
                result = test.return_tuple_of_basic_types()
                if (len(result) == 3 and result[0] == False and result[1] == 24 and result[2] == -25.0):
                        print ('TupleTypeTest_Input_bool_int_float')

                # Verify that a tuple of strings (string, string) is returned correctly
                result = test.return_tuple_of_strings()
                if (len(result) == 2):
                    print ('TupleTypeTest_Output_string_string')

                # Create a tuple from a Python list and verify the values are correct
                test.accept_tuple_of_strings(['ghi', 'jkl'])
                result = test.return_tuple_of_strings()
                if (len(result) == 2 and result[0] == 'ghi' and result[1] == 'jkl'):
                    print ('TupleTypeTest_Input_string_string_list')

                # Create a tuple from a Python tuple and verify the values are correct
                test.accept_tuple_of_strings(('abc', 'def'))
                result = test.return_tuple_of_strings()
                if (len(result) == 2 and result[0] == 'abc' and result[1] == 'def'):
                    print ('TupleTypeTest_Input_string_string')
            )");
        } catch ([[maybe_unused]] const pybind11::error_already_set& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(4, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_Input)]);
        EXPECT_EQ(2, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleTypeTest_Output)]);
    }

    TEST_F(PythonReflectionTupleTests, CustomTypesConvertedCorrectly)
    {
        enum class LogTypes
        {
            Skip = 0,
            TupleCustomTypeTest_Input,
            TupleCustomTypeTest_Output,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "TupleCustomTypeTest_Input"))
                {
                    return static_cast<int>(LogTypes::TupleCustomTypeTest_Input);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "TupleCustomTypeTest_Output"))
                {
                    return static_cast<int>(LogTypes::TupleCustomTypeTest_Output);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionTupleTypes pythonReflectionPairTypes;
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

                # Create the test fixture
                test = azlmbr.object.create('PythonReflectionTupleTypes')

                result = test.return_tuple_with_custom_type()
                if (len(result) == 3):
                    print ('TupleCustomTypeTest_Output')

                custom = azlmbr.object.create('MyCustomType')
                custom.set_data(42)
                test.accept_tuple_with_custom_type((True, 'def', custom))
                result = test.return_tuple_with_custom_type()
                if (len(result) == 3 and result[0] == True and result[1] == 'def' and result[2].get_data() == 42):
                        print ('TupleCustomTypeTest_Input')
            )");
        } catch ([[maybe_unused]] const pybind11::error_already_set& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleCustomTypeTest_Input)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleCustomTypeTest_Output)]);
    }

    TEST_F(PythonReflectionTupleTests, UnsupportedTypesLogErrors)
    {
        enum class LogTypes
        {
            Skip = 0,
            TupleUnsupportedTypeTest_CannotConvert
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Contains(message, "accept_tuple_of_basic_types"))
                {
                    return static_cast<int>(LogTypes::TupleUnsupportedTypeTest_CannotConvert);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonReflectionTupleTypes pythonReflectionDictionaryTypes;
        pythonReflectionDictionaryTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionDictionaryTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        AZ_TEST_START_TRACE_SUPPRESSION;
        try
        {
            pybind11::exec(R"(
                import azlmbr.test.pair
                import azlmbr.object
                import azlmbr.std

                # Create the test fixture
                test = azlmbr.object.create('PythonReflectionTupleTypes')

                # This should fail because it's passing [bool, int] to a tuple expecting [bool, int, float]
                test.accept_tuple_of_basic_types([True, 5])

                # This should fail because it's passing [int, string, bool] to a tuple expecting [bool, int, float]
                test.accept_tuple_of_basic_types([5, 'abc', True])

                # This should fail because it's passing [bool, int, float, float] to a tuple expecting [bool, int, float]
                test.accept_tuple_of_basic_types([True, 5, 10.0, 10.0])

                # This should fail because it's passing a set instead of a tuple or list
                test.accept_tuple_of_basic_types({True, 5, 10.0})
            )");
        } catch ([[maybe_unused]] const pybind11::error_already_set& ex)
        {
            AZ_Warning("UnitTest", false, "Failed with Python exception of %s", ex.what());
            FAIL();
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

        e.Deactivate();

        EXPECT_EQ(4, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::TupleUnsupportedTypeTest_CannotConvert)]);
    }
}


