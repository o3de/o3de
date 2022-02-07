/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <ScriptCanvas/Libraries/Core/EventHandlerTranslationUtility.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    class ScriptCanvasEventHandlerTranslationUtilityUnitTestFixture
        : public ScriptCanvasUnitTestFixture
    {
    protected:
        Nodes::Core::ReceiveScriptEvent* m_receiveScriptEvent;
        Nodes::Core::Internal::ScriptEventEntry* m_eventEntry;

        void SetUp() override
        {
            ScriptCanvasUnitTestFixture::SetUp();

            m_receiveScriptEvent = new Nodes::Core::ReceiveScriptEvent();
            m_eventEntry = new Nodes::Core::Internal::ScriptEventEntry();
        };

        void TearDown() override
        {
            delete m_eventEntry;
            delete m_receiveScriptEvent;

            ScriptCanvasUnitTestFixture::TearDown();
        };
    };

    TEST_F(ScriptCanvasEventHandlerTranslationUtilityUnitTestFixture, GetSlotsInExecutionThreadByType_GetEmptyOutcome_WhenEventEntryHasNoResult)
    {
        // event without result
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("OnEvent", ConnectionType::Output));
        m_eventEntry->m_eventSlotId = m_receiveScriptEvent->GetSlotByName("OnEvent")->GetId();
        m_receiveScriptEvent->m_eventMap.emplace(AZ::Crc32("OnEvent"), *m_eventEntry);

        auto outcome = Nodes::Core::EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*m_receiveScriptEvent, *m_receiveScriptEvent->GetSlotByName("OnEvent"), CombinedSlotType::DataIn);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(0, outcome.GetValue().size());
    }

    TEST_F(ScriptCanvasEventHandlerTranslationUtilityUnitTestFixture, GetSlotsInExecutionThreadByType_GetSuccessOutcome_WhenEventEntryHasOneResult)
    {
        // event with one result
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("OnEvent", ConnectionType::Output));
        m_receiveScriptEvent->AddSlot(DataSlotConfiguration(Data::Type::Boolean(), "Result:Boolean", ConnectionType::Input));
        m_eventEntry->m_eventSlotId = m_receiveScriptEvent->GetSlotByName("OnEvent")->GetId();
        m_eventEntry->m_resultSlotId = m_receiveScriptEvent->GetSlotByName("Result:Boolean")->GetId();
        m_receiveScriptEvent->m_eventMap.emplace(AZ::Crc32("OnEvent"), *m_eventEntry);

        auto outcome = Nodes::Core::EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*m_receiveScriptEvent, *m_receiveScriptEvent->GetSlotByName("OnEvent"), CombinedSlotType::DataIn);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(1, outcome.GetValue().size());
    }

    TEST_F(ScriptCanvasEventHandlerTranslationUtilityUnitTestFixture, GetSlotsInExecutionThreadByType_GetEmptyOutcome_WhenEventEntryHasNoParameter)
    {
        // event without parameter
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("OnEvent", ConnectionType::Output));
        m_eventEntry->m_eventSlotId = m_receiveScriptEvent->GetSlotByName("OnEvent")->GetId();
        m_receiveScriptEvent->m_eventMap.emplace(AZ::Crc32("OnEvent"), *m_eventEntry);

        auto outcome = Nodes::Core::EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*m_receiveScriptEvent, *m_receiveScriptEvent->GetSlotByName("OnEvent"), CombinedSlotType::DataOut);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(0, outcome.GetValue().size());
    }

    TEST_F(ScriptCanvasEventHandlerTranslationUtilityUnitTestFixture, GetSlotsInExecutionThreadByType_GetSuccessOutcome_WhenEventEntryHasOneParameter)
    {
        // event with one parameter
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("OnEvent", ConnectionType::Output));
        m_receiveScriptEvent->AddSlot(DataSlotConfiguration(Data::Type::Boolean(), "BooleanParameter", ConnectionType::Output));
        m_eventEntry->m_eventSlotId = m_receiveScriptEvent->GetSlotByName("OnEvent")->GetId();
        m_eventEntry->m_parameterSlotIds.emplace_back(m_receiveScriptEvent->GetSlotByName("BooleanParameter")->GetId());
        m_receiveScriptEvent->m_eventMap.emplace(AZ::Crc32("OnEvent"), *m_eventEntry);

        auto outcome = Nodes::Core::EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*m_receiveScriptEvent, *m_receiveScriptEvent->GetSlotByName("OnEvent"), CombinedSlotType::DataOut);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(1, outcome.GetValue().size());
    }

    TEST_F(ScriptCanvasEventHandlerTranslationUtilityUnitTestFixture, GetSlotsInExecutionThreadByType_GetEmptyOutcome_WhenExecutionInSlotIsNotAddressed)
    {
        // execution in slot (default not addressed)
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("In", ConnectionType::Input));

        auto outcome = Nodes::Core::EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*m_receiveScriptEvent, *m_receiveScriptEvent->GetSlotByName("In"), CombinedSlotType::DataIn);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(0, outcome.GetValue().size());
    }

    TEST_F(ScriptCanvasEventHandlerTranslationUtilityUnitTestFixture, GetSlotsInExecutionThreadByType_GetSuccessOutcome_WhenConnectSlotMapsToOnConnectedSlot)
    {
        // Connect slot -> OnConnected slot
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("Connect", ConnectionType::Input));
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("OnConnected", ConnectionType::Output));

        auto outcome = Nodes::Core::EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*m_receiveScriptEvent, *m_receiveScriptEvent->GetSlotByName("Connect"), CombinedSlotType::ExecutionOut);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(1, outcome.GetValue().size());
    }

    TEST_F(ScriptCanvasEventHandlerTranslationUtilityUnitTestFixture, GetSlotsInExecutionThreadByType_GetSuccessOutcome_WhenDisconnectSlotMapsToOnDisconnectedSlot)
    {
        // Disconnect slot -> OnDisconnected slot
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("Disconnect", ConnectionType::Input));
        m_receiveScriptEvent->AddSlot(ExecutionSlotConfiguration("OnDisconnected", ConnectionType::Output));

        auto outcome = Nodes::Core::EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*m_receiveScriptEvent, *m_receiveScriptEvent->GetSlotByName("Disconnect"), CombinedSlotType::ExecutionOut);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(1, outcome.GetValue().size());
    }
}
