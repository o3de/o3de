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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonMarshalComponent.h>

namespace UnitTest
{
    struct PythonReflectUnorderedSet
    {
        AZ_TYPE_INFO(PythonReflectUnorderedSet, "{A596466F-2F29-4479-A721-0E50FA704962}");
        
        AZStd::unordered_set<AZ::u8> m_u8Set {1,2};
        AZStd::unordered_set<AZ::u16> m_u16Set {4,8};
        AZStd::unordered_set<AZ::u32> m_u32Set {16,32};
        AZStd::unordered_set<AZ::u64> m_u64Set {64,128};
        AZStd::unordered_set<AZ::s8> m_s8Set {-1,-2};
        AZStd::unordered_set<AZ::s16> m_s16Set {-4,-8};
        AZStd::unordered_set<AZ::s32> m_s32Set {-16,-32};
        AZStd::unordered_set<AZ::s64> m_s64Set {-64,-128};
        AZStd::unordered_set<double> m_floatSet {1.0f, 2.0f};
        AZStd::unordered_set<float> m_doubleSet {0.1, 0.2};
        AZStd::unordered_set<AZStd::string> m_stringSet {"one", "two"};

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<decltype(m_u8Set)>();
                serializeContext->RegisterGenericType<decltype(m_u16Set)>();
                serializeContext->RegisterGenericType<decltype(m_u32Set)>();
                serializeContext->RegisterGenericType<decltype(m_u64Set)>();
                serializeContext->RegisterGenericType<decltype(m_s8Set)>();
                serializeContext->RegisterGenericType<decltype(m_s16Set)>(); 
                serializeContext->RegisterGenericType<decltype(m_s32Set)>(); 
                serializeContext->RegisterGenericType<decltype(m_s64Set)>();
                serializeContext->RegisterGenericType<decltype(m_floatSet)>();
                serializeContext->RegisterGenericType<decltype(m_doubleSet)>(); 
                serializeContext->RegisterGenericType<decltype(m_stringSet)>(); 
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectUnorderedSet>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test.set")
                    ->Property("u8Set", BehaviorValueProperty(&PythonReflectUnorderedSet::m_u8Set) )
                    ->Property("u16Set", BehaviorValueProperty(&PythonReflectUnorderedSet::m_u16Set))
                    ->Property("u32Set", BehaviorValueProperty(&PythonReflectUnorderedSet::m_u32Set))
                    ->Property("u64Set", BehaviorValueProperty(&PythonReflectUnorderedSet::m_u64Set))
                    ->Property("s8Set", BehaviorValueProperty(&PythonReflectUnorderedSet::m_s8Set))
                    ->Property("s16Set", BehaviorValueProperty(&PythonReflectUnorderedSet::m_s16Set))
                    ->Property("s32Set", BehaviorValueProperty(&PythonReflectUnorderedSet::m_s32Set))
                    ->Property("s64Set", BehaviorValueProperty(&PythonReflectUnorderedSet::m_s64Set))
                    ->Property("floatSet", BehaviorValueProperty(&PythonReflectUnorderedSet::m_floatSet))
                    ->Property("doubleSet", BehaviorValueProperty(&PythonReflectUnorderedSet::m_doubleSet))
                    ->Property("stringSet", BehaviorValueProperty(&PythonReflectUnorderedSet::m_stringSet))
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonAssociativeTest
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

    TEST_F(PythonAssociativeTest, SimpleUnorderedSet_Assignment)
    {
        enum class LogTypes
        {
            Skip = 0,
            Update,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AZ::StringFunc::Equal(window, "python"))
            {
                if (AZ::StringFunc::StartsWith(message, "Update"))
                {
                    return aznumeric_cast<int>(LogTypes::Update);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        PythonReflectUnorderedSet pythonReflectUnorderedSet;
        pythonReflectUnorderedSet.Reflect(m_app.GetSerializeContext());
        pythonReflectUnorderedSet.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.set
                tester = azlmbr.test.set.PythonReflectUnorderedSet()

                def updateNumberDataSet(memberSet, dataSet):
                    memberSet = dataSet
                    for value in memberSet:
                        if (value in dataSet):
                            print ('Update_worked_{}'.format(memberSet))

                updateNumberDataSet(tester.u8Set, {2, 1})
                updateNumberDataSet(tester.u16Set, {8, 4})
                updateNumberDataSet(tester.u32Set, {32, 16})
                updateNumberDataSet(tester.u64Set, {128, 64})
                updateNumberDataSet(tester.s8Set, {-2, -1})
                updateNumberDataSet(tester.s16Set, {-8, -4})
                updateNumberDataSet(tester.s32Set, {-32, -16})
                updateNumberDataSet(tester.s64Set, {-128, -64})

                from azlmbr.math import Math_IsClose

                def updateFloatDataSet(memberFloatSet, dataSet):
                    memberFloatSet = dataSet
                    for dataItem in dataSet:
                        for memberItem in memberFloatSet:
                            if (Math_IsClose(dataItem, memberItem)):
                                print ('Update_float_worked_{}'.format(memberFloatSet))

                updateFloatDataSet(tester.floatSet, {4.0, 8.0})
                updateFloatDataSet(tester.doubleSet, {0.4, 0.8})

                stringDataSet = {'three','four'}
                tester.stringSet = stringDataSet
                for dataItem in stringDataSet:
                    for memberItem in tester.stringSet:
                        if (dataItem == memberItem):
                            print ('Update_string_worked')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed with Python exception of %s", e.what());
        }

        e.Deactivate();

        EXPECT_EQ(22, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::Update)]);
    }

    TEST_F(PythonAssociativeTest, SimpleUnorderedSet_Creation)
    {
        enum class LogTypes
        {
            Skip = 0,
            Create,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AZ::StringFunc::Equal(window, "python"))
            {
                if (AZ::StringFunc::StartsWith(message, "Create"))
                {
                    return aznumeric_cast<int>(LogTypes::Create);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        PythonReflectUnorderedSet pythonReflectUnorderedSet;
        pythonReflectUnorderedSet.Reflect(m_app.GetSerializeContext());
        pythonReflectUnorderedSet.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test.set

                tester = azlmbr.test.set.PythonReflectUnorderedSet()
                if (tester.u8Set == {1, 2}):
                    print ('Create_Works_u8Set')
                if (tester.u16Set == {4, 8}):
                    print ('Create_Works_u16Set')
                if (tester.u32Set == {16, 32}):
                    print ('Create_Works_u32Set')
                if (tester.u64Set == {64, 128}):
                    print ('Create_Works_u64Set')
                if (tester.s8Set == {-1, -2}):
                    print ('Create_Works_s8Set')
                if (tester.s16Set == {-4, -8}):
                    print ('Create_Works_s16Set')
                if (tester.s32Set == {-16, -32}):
                    print ('Create_Works_s32Set')
                if (tester.s64Set == {-64, -128}):
                    print ('Create_Works_s64Set')

                from azlmbr.math import Math_IsClose
                for value in tester.floatSet:
                    if (Math_IsClose(value, 1.0) or Math_IsClose(value, 2.0)):
                        print ('Create_Works_floatSet')
                for value in tester.doubleSet:
                    if (Math_IsClose(value, 0.1) or Math_IsClose(value, 0.2)):
                        print ('Create_Works_doubleSet')
                for value in tester.stringSet:
                    if ((value == 'one') or (value == 'two')):
                        print ('Create_Works_stringSet')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed with Python exception of %s", e.what());
        }

        e.Deactivate();

        EXPECT_EQ(14, m_testSink.m_evaluationMap[aznumeric_cast<int>(LogTypes::Create)]);
    }

}
