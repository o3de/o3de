/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Tests/Mocks/BehaviorMethodMock.h>

#include <ScriptCanvas/Core/SlotConfigurations.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes::Core;

    namespace MethodUnitTestStructures
    {
        class TestMethod : public Method
        {
        public:
            void PopulateTestMethodNode(const AZ::BehaviorMethod& method)
            {
                ConfigureMethod(method);
            }

            template<typename T>
            void PopulateTestMethodSlot(const T& dataValue, const ConnectionType connectionType, const size_t numSlots)
            {
                for (size_t i = 0; i < numSlots; i++)
                {
                    DataSlotConfiguration dataSlotConfiguration;
                    dataSlotConfiguration.m_name = "TestSlot" + i;
                    dataSlotConfiguration.SetConnectionType(connectionType);
                    dataSlotConfiguration.SetDefaultValue<T>(dataValue);
                    AddSlot(dataSlotConfiguration);
                }
            }
        };
    };

    class ScriptCanvasMethodUnitTestFixture
        : public ScriptCanvasUnitTestFixture
    {
    protected:
        MethodUnitTestStructures::TestMethod* m_testMethod;
        BehaviorMethodMock* m_behaviorMethodMock;
        AZ::BehaviorParameter* m_behaviorParameter;

        void SetUp() override
        {
            using ::testing::Return;

            ScriptCanvasUnitTestFixture::SetUp();

            m_testMethod = new MethodUnitTestStructures::TestMethod();
            m_behaviorMethodMock = new BehaviorMethodMock();
            ON_CALL(*m_behaviorMethodMock, HasResult()).WillByDefault(Return(false));
            m_behaviorParameter = new AZ::BehaviorParameter();
        };

        void TearDown() override
        {
            delete m_testMethod;
            delete m_behaviorMethodMock;
            delete m_behaviorParameter;

            ScriptCanvasUnitTestFixture::TearDown();
        };
    };

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NoMethodFoundInContext_ReturnTrue)
    {
        bool actualResult = m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NodeAndMethodDoNotHaveOutputOrInput_ReturnFalse)
    {
        using ::testing::Return;

        m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(0));
        bool actualResult = m_testMethod->IsOutOfDate();
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_MethodHasOutputButNodeDoesNot_ReturnTrue)
    {
        using ::testing::Return;

        m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(0));
        bool actualResult = m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NodeHasOutputButMethodDoesNot_ReturnTrue)
    {
        using ::testing::Return;

        m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);
        m_testMethod->PopulateTestMethodSlot<int>(0, ConnectionType::Output, 1);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(0));
        bool actualResult = m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NodeAndMethodHaveSameOutputType_ReturnFalse)
    {
        using ::testing::Return;

        m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);
        m_testMethod->PopulateTestMethodSlot<bool>(true, ConnectionType::Output, 1);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(0));
        m_behaviorParameter->m_typeId = azrtti_typeid<bool>();
        EXPECT_CALL(*m_behaviorMethodMock, GetResult()).Times(1).WillOnce(Return(m_behaviorParameter));
        bool actualResult = m_testMethod->IsOutOfDate();
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NodeAndMethodHaveDifferentOutputType_ReturnTrue)
    {
        using ::testing::Return;

        m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);
        m_testMethod->PopulateTestMethodSlot<int>(0, ConnectionType::Output, 1);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(0));
        m_behaviorParameter->m_typeId = azrtti_typeid<bool>();
        EXPECT_CALL(*m_behaviorMethodMock, GetResult()).Times(1).WillOnce(Return(m_behaviorParameter));
        bool actualResult = m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_MethodHasInputButNodeDoesNot_ReturnTrue)
    {
        using ::testing::Return;

        m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(1));
        bool actualResult = m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NodeHasInputButMethodDoesNot_ReturnTrue)
    {
        using ::testing::Return;

         m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);
         m_testMethod->PopulateTestMethodSlot<int>(0, ConnectionType::Input, 1);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(0));
        bool actualResult =  m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NodeAndMethodHaveSameInputType_ReturnFalse)
    {
        using ::testing::_;
        using ::testing::Return;

         m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);
         m_testMethod->PopulateTestMethodSlot<bool>(true, ConnectionType::Input, 1);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(1));
        m_behaviorParameter->m_typeId = azrtti_typeid<bool>();
        EXPECT_CALL(*m_behaviorMethodMock, GetArgument(_)).Times(1).WillOnce(Return(m_behaviorParameter));
        bool actualResult =  m_testMethod->IsOutOfDate();
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NodeAndMethodHaveDifferentInputType_ReturnTrue)
    {
        using ::testing::_;
        using ::testing::Return;

         m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);
         m_testMethod->PopulateTestMethodSlot<int>(0, ConnectionType::Input, 1);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(1));
        m_behaviorParameter->m_typeId = azrtti_typeid<bool>();
        EXPECT_CALL(*m_behaviorMethodMock, GetArgument(_)).Times(1).WillOnce(Return(m_behaviorParameter));
        bool actualResult =  m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_NodeHasMoreInputThanMethod_ReturnTrue)
    {
        using ::testing::_;
        using ::testing::Return;

         m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);
         m_testMethod->PopulateTestMethodSlot<bool>(true, ConnectionType::Input, 2);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(1));
        m_behaviorParameter->m_typeId = azrtti_typeid<bool>();
        EXPECT_CALL(*m_behaviorMethodMock, GetArgument(_)).Times(1).WillOnce(Return(m_behaviorParameter));
        bool actualResult =  m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasMethodUnitTestFixture, IsOutOfDate_MethodHasMoreInputThanNode_ReturnTrue)
    {
        using ::testing::_;
        using ::testing::Return;

         m_testMethod->PopulateTestMethodNode(*m_behaviorMethodMock);
         m_testMethod->PopulateTestMethodSlot<bool>(true, ConnectionType::Input, 1);

        EXPECT_CALL(*m_behaviorMethodMock, HasResult()).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*m_behaviorMethodMock, GetNumArguments()).Times(1).WillOnce(Return(2));
        m_behaviorParameter->m_typeId = azrtti_typeid<bool>();
        EXPECT_CALL(*m_behaviorMethodMock, GetArgument(_)).Times(1).WillOnce(Return(m_behaviorParameter));
        bool actualResult =  m_testMethod->IsOutOfDate();
        EXPECT_TRUE(actualResult);
    }
}
