/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>

#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Libraries/Core/BehaviorContextObjectNode.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>

using namespace ScriptCanvasTests;
using namespace ScriptCanvasEditor;
using namespace TestNodes;

TEST_F(ScriptCanvasTestFixture, BehaviorContextObjectGenericConstructor)
{
    using namespace ScriptCanvas;
    
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    
    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<ScriptCanvas::Graph>();
    EXPECT_NE(nullptr, graph);

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();
    
    //Validates the GenericConstructorOverride attribute is being used to construct types that are normally not initialized in C++
    AZ::EntityId vector3IdA;
    ScriptCanvas::Nodes::Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    
    ReportErrors(graph);

    if (auto result = vector3NodeA->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set"))
    {
        EXPECT_EQ(0, result->GetValue());
    }
    else
    {
        ADD_FAILURE();
    }

    delete graphEntity;
    
    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

// Tests the basic footprint of the ebus node both before and after graph activation to make sure all internal bookeeping is correct.
TEST_F(ScriptCanvasTestFixture, BehaviorContext_BusHandlerNodeFootPrint)
{
    using namespace ScriptCanvas;

    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_behaviorContext);

    auto graphEntity = aznew AZ::Entity;
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    ScriptCanvas::Graph* graph = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(graphEntity);
    graphEntity->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId uuidEventHandlerId;
    auto uuidEventHandler = CreateTestNode<ScriptCanvas::Nodes::Core::EBusEventHandler>(graphUniqueId, uuidEventHandlerId);
    uuidEventHandler->InitializeBus("TemplateEventTestHandler<AZ::Uuid >");
    AZ::Uuid uuidBusId = AZ::Uuid::CreateName("TemplateEventBus");
    uuidEventHandler->SetInput_UNIT_TEST(ScriptCanvas::Nodes::Core::EBusEventHandler::c_busIdName, uuidBusId); //Set Uuid bus id
    
    {
        auto eventEntry = uuidEventHandler->FindEvent("VectorCreatedEvent");
        EXPECT_NE(nullptr, eventEntry);
        EXPECT_EQ(eventEntry->m_parameterSlotIds.size(), 1);
        EXPECT_TRUE(eventEntry->m_resultSlotId.IsValid());

        {
            const Slot* outputSlot = uuidEventHandler->GetSlot(eventEntry->m_eventSlotId);
            EXPECT_EQ(outputSlot->GetDescriptor(), SlotDescriptors::ExecutionOut());
        }

        {
            const Slot* dataSlot = uuidEventHandler->GetSlot(eventEntry->m_parameterSlotIds[0]);

            EXPECT_EQ(dataSlot->GetDescriptor(), SlotDescriptors::DataOut());
            EXPECT_EQ(dataSlot->GetDataType(), Data::Type::Vector3());

            const Datum* datum = uuidEventHandler->FindDatum(eventEntry->m_parameterSlotIds[0]);
            EXPECT_EQ(nullptr, datum);

            ModifiableDatumView datumView;
            uuidEventHandler->FindModifiableDatumView(eventEntry->m_parameterSlotIds[0], datumView);

            EXPECT_FALSE(datumView.IsValid());
        }

        {
            const Slot* resultSlot = uuidEventHandler->GetSlot(eventEntry->m_resultSlotId);

            EXPECT_EQ(resultSlot->GetDescriptor(), SlotDescriptors::DataIn());
            EXPECT_EQ(resultSlot->GetDataType(), Data::Type::Vector3());

            const Datum* datum = uuidEventHandler->FindDatum(eventEntry->m_resultSlotId);
            EXPECT_NE(nullptr, datum);

            if (datum)
            {
                EXPECT_TRUE(datum->IS_A<Data::Vector3Type>());
            }

            ModifiableDatumView datumView;
            uuidEventHandler->FindModifiableDatumView(eventEntry->m_resultSlotId, datumView);

            EXPECT_TRUE(datumView.IsValid());

            if (datumView.IsValid())
            {                
                const Datum* datum2 = datumView.GetDatum();
                EXPECT_TRUE(datum2->IS_A<Data::Vector3Type>());
            }
        }
    }

    graphEntity->Activate();

    {
        auto eventEntry = uuidEventHandler->FindEvent("VectorCreatedEvent");
        EXPECT_NE(nullptr, eventEntry);
        EXPECT_EQ(eventEntry->m_parameterSlotIds.size(), 1);
        EXPECT_TRUE(eventEntry->m_resultSlotId.IsValid());

        {
            const Slot* outputSlot = uuidEventHandler->GetSlot(eventEntry->m_eventSlotId);
            EXPECT_EQ(outputSlot->GetDescriptor(), SlotDescriptors::ExecutionOut());
        }

        {
            const Slot* dataSlot = uuidEventHandler->GetSlot(eventEntry->m_parameterSlotIds[0]);

            EXPECT_EQ(dataSlot->GetDescriptor(), SlotDescriptors::DataOut());
            EXPECT_EQ(dataSlot->GetDataType(), Data::Type::Vector3());

            const Datum* datum = uuidEventHandler->FindDatum(eventEntry->m_parameterSlotIds[0]);
            EXPECT_EQ(nullptr, datum);

            ModifiableDatumView datumView;
            uuidEventHandler->FindModifiableDatumView(eventEntry->m_parameterSlotIds[0], datumView);

            EXPECT_FALSE(datumView.IsValid());
        }

        {
            const Slot* resultSlot = uuidEventHandler->GetSlot(eventEntry->m_resultSlotId);

            EXPECT_EQ(resultSlot->GetDescriptor(), SlotDescriptors::DataIn());
            EXPECT_EQ(resultSlot->GetDataType(), Data::Type::Vector3());

            const Datum* datum = uuidEventHandler->FindDatum(eventEntry->m_resultSlotId);
            EXPECT_NE(nullptr, datum);

            if (datum)
            {
                EXPECT_TRUE(datum->IS_A<Data::Vector3Type>());
            }

            ModifiableDatumView datumView;
            uuidEventHandler->FindModifiableDatumView(eventEntry->m_resultSlotId, datumView);

            EXPECT_TRUE(datumView.IsValid());

            if (datumView.IsValid())
            {
                const Datum* datum2 = datumView.GetDatum();
                EXPECT_TRUE(datum2->IS_A<Data::Vector3Type>());
            }
        }
    }

    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

void ReflectSignCorrectly()
{
    AZ::BehaviorContext* behaviorContext(nullptr);
    AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
    AZ_Assert(behaviorContext, "A behavior context is required!");
    behaviorContext->Method("Sign", AZ::GetSign);
}
