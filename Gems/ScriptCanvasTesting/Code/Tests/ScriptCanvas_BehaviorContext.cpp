/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

TEST_F(ScriptCanvasTestFixture, BehaviorContextProperties)
{
    
    using namespace ScriptCanvas;
    using namespace Nodes;

    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    
    AZ::EntityId addId;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addId);
    
    AZ::EntityId vector3IdA, vector3IdB, vector3IdC;
    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<AZ::Vector3>());

    Core::BehaviorContextObjectNode* vector3NodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdB);
    vector3NodeB->InitializeObject(azrtti_typeid<AZ::Vector3>());

    Core::BehaviorContextObjectNode* vector3NodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdC);
    vector3NodeC->InitializeObject(azrtti_typeid<AZ::Vector3>());

    AZ::EntityId number1Id, number2Id, number3Id, number4Id, number5Id, number6Id, number7Id, number8Id, number9Id;
    Node* number1Node = CreateDataNode<Data::NumberType>(graphUniqueId, 1, number1Id);
    Node* number2Node = CreateDataNode<Data::NumberType>(graphUniqueId, 2, number2Id);
    Node* number3Node = CreateDataNode<Data::NumberType>(graphUniqueId, 3, number3Id);
    Node* number4Node = CreateDataNode<Data::NumberType>(graphUniqueId, 4, number4Id);
    Node* number5Node = CreateDataNode<Data::NumberType>(graphUniqueId, 5, number5Id);
    Node* number6Node = CreateDataNode<Data::NumberType>(graphUniqueId, 6, number6Id);
    Node* number7Node = CreateDataNode<Data::NumberType>(graphUniqueId, 7, number7Id);
    Node* number8Node = CreateDataNode<Data::NumberType>(graphUniqueId, 8, number8Id);
    Node* number9Node = CreateDataNode<Data::NumberType>(graphUniqueId, 0, number9Id);

    auto number1 = GetInput_UNIT_TEST<Data::NumberType>(number1Node, "Set");
    EXPECT_DOUBLE_EQ(1.0, (*number1));

    auto number4 = GetInput_UNIT_TEST<Data::NumberType>(number4Node, "Set");
    EXPECT_DOUBLE_EQ(4.0, (*number4));

    // data
    EXPECT_TRUE(Connect(*graph, number1Id, "Get", vector3IdA, "Number: x"));
    EXPECT_TRUE(Connect(*graph, number2Id, "Get", vector3IdA, "Number: y"));
    EXPECT_TRUE(Connect(*graph, number3Id, "Get", vector3IdA, "Number: z"));

    EXPECT_TRUE(Connect(*graph, number4Id, "Get", vector3IdB, "Number: x"));
    EXPECT_TRUE(Connect(*graph, number5Id, "Get", vector3IdB, "Number: y"));
    EXPECT_TRUE(Connect(*graph, number6Id, "Get", vector3IdB, "Number: z"));

    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", addId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", addId, "Vector3: B"));

    EXPECT_TRUE(Connect(*graph, addId, "Result: Vector3", vector3IdC, "Set"));    
    
    EXPECT_TRUE(Connect(*graph, vector3IdC, "x: Number", number7Id, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "y: Number", number8Id, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "z: Number", number9Id, "Set"));    

    // code
    EXPECT_TRUE(Connect(*graph, startID, "Out", addId, "In"));

    graphEntity->Activate();

    ReportErrors(graph);    

    if (auto result = GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeC, "Set"))
    {
        EXPECT_EQ(AZ::Vector3(5, 7, 9), (*result));
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = GetInput_UNIT_TEST<Data::NumberType>(number7Node, "Set"))
    {
        EXPECT_EQ(5, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = GetInput_UNIT_TEST<Data::NumberType>(number8Node, "Set"))
    {
        EXPECT_EQ(7, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = GetInput_UNIT_TEST<Data::NumberType>(number9Node, "Set"))
    {
        EXPECT_EQ(9, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;
}


TEST_F(ScriptCanvasTestFixture, BehaviorContextObjectGenericConstructor)
{
    

    using namespace ScriptCanvas;
    using namespace Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    
    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();
    
    //Validates the GenericConstructorOverride attribute is being used to construct types that are normally not initialized in C++
    AZ::EntityId vector3IdA;
    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    
    ReportErrors(graph);
    
    if (auto result = GetInput_UNIT_TEST<TestBehaviorContextObject>(vector3NodeA, "Set"))
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
    Graph* graph = AZ::EntityUtils::FindFirstDerivedComponent<Graph>(graphEntity);    
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId uuidEventHandlerId;
    auto uuidEventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, uuidEventHandlerId);
    uuidEventHandler->InitializeBus("TemplateEventTestHandler<AZ::Uuid >");
    AZ::Uuid uuidBusId = AZ::Uuid::CreateName("TemplateEventBus");
    SetInput_UNIT_TEST(uuidEventHandler, Nodes::Core::EBusEventHandler::c_busIdName, uuidBusId); //Set Uuid bus id
    
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

TEST_F(ScriptCanvasTestFixture, BehaviorContext_BusHandlerNonEntityIdBusId)
{
    
    using namespace ScriptCanvas;

    RegisterComponentDescriptor<StringView>();

    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_behaviorContext);
    TemplateEventTestHandler<AZStd::string>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZStd::string>::Reflect(m_behaviorContext);

    auto graphEntity = aznew AZ::Entity;
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    Graph* graph = AZ::EntityUtils::FindFirstDerivedComponent<Graph>(graphEntity);
    EXPECT_NE(nullptr, graph);
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId uuidEventHandlerId;
    auto uuidEventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, uuidEventHandlerId);
    uuidEventHandler->InitializeBus("TemplateEventTestHandler<AZ::Uuid >");
    AZ::Uuid uuidBusId = AZ::Uuid::CreateName("TemplateEventBus");
    SetInput_UNIT_TEST(uuidEventHandler, Nodes::Core::EBusEventHandler::c_busIdName, uuidBusId); //Set Uuid bus id
    
    AZ::EntityId stringEventHandlerId;
    auto stringEventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, stringEventHandlerId);
    stringEventHandler->InitializeBus("TemplateEventTestHandler<AZStd::basic_string<char, AZStd::char_traits<char>, allocator> >");
    AZStd::string stringBusId = "TemplateEventBus";
    SetInput_UNIT_TEST(stringEventHandler, Nodes::Core::EBusEventHandler::c_busIdName, stringBusId); // Set String bus id    

                                                                                                     // print (for generic event)
    AZ::EntityId printId;
    auto print = CreateTestNode<TestResult>(graphUniqueId, printId);
    print->SetText("Generic Event executed");

    // print (for string event)
    AZ::EntityId print2Id;
    auto print2 = CreateTestNode<TestResult>(graphUniqueId, print2Id);
    print2->SetText("print2 was un-initialized");

    // print (for Vector3 event)
    AZ::EntityId print3Id;
    auto print3 = CreateTestNode<TestResult>(graphUniqueId, print3Id);
    print3->SetText("print3 was un-initialized");

    // result string
    AZ::EntityId stringId;
    auto stringNode = CreateTestNode<StringView>(graphUniqueId, stringId);

    // result vector
    AZ::EntityId vector3Id;
    auto vector3Node = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3Id);
    SetInput_UNIT_TEST(vector3Node, "Set", AZ::Vector3(0.0f, 0.0f, 0.0f));

    // Ebus Handlers connected by Uuid
    // EBus Handlers connected by String
    for (auto&& genericUuidHandler : { uuidEventHandler, stringEventHandler })
    {
        // Generic Event
        auto eventEntry = genericUuidHandler->FindEvent("GenericEvent");
        EXPECT_NE(nullptr, eventEntry);
        EXPECT_TRUE(graph->Connect(genericUuidHandler->GetEntityId(), eventEntry->m_eventSlotId, printId, print->GetSlotId("In")));

        eventEntry = genericUuidHandler->FindEvent("UpdateNameEvent");
        EXPECT_NE(nullptr, eventEntry);
        EXPECT_FALSE(eventEntry->m_parameterSlotIds.empty());
        EXPECT_TRUE(graph->Connect(genericUuidHandler->GetEntityId(), eventEntry->m_eventSlotId, print2Id, print2->GetSlotId("In")));
        EXPECT_TRUE(graph->Connect(genericUuidHandler->GetEntityId(), eventEntry->m_parameterSlotIds[0], print2Id, print2->GetSlotId("Value")));
        EXPECT_TRUE(graph->Connect(genericUuidHandler->GetEntityId(), eventEntry->m_eventSlotId, stringId, stringNode->GetSlotId("In")));
        EXPECT_TRUE(graph->Connect(genericUuidHandler->GetEntityId(), eventEntry->m_parameterSlotIds[0], stringId, stringNode->GetSlotId("View")));
        EXPECT_TRUE(graph->Connect(stringId, stringNode->GetSlotId("Result"), genericUuidHandler->GetEntityId(), eventEntry->m_resultSlotId));        

        eventEntry = genericUuidHandler->FindEvent("VectorCreatedEvent");
        EXPECT_NE(nullptr, eventEntry);
        EXPECT_FALSE(eventEntry->m_parameterSlotIds.empty());
        EXPECT_TRUE(graph->Connect(genericUuidHandler->GetEntityId(), eventEntry->m_eventSlotId, print3Id, print3->GetSlotId("In")));
        EXPECT_TRUE(graph->Connect(genericUuidHandler->GetEntityId(), eventEntry->m_parameterSlotIds[0], print3Id, print3->GetSlotId("Value")));
        EXPECT_TRUE(graph->Connect(genericUuidHandler->GetEntityId(), eventEntry->m_parameterSlotIds[0], vector3Id, vector3Node->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(vector3Id, vector3Node->GetSlotId("Get"), genericUuidHandler->GetEntityId(), eventEntry->m_resultSlotId));

        const Datum* resultDatum = genericUuidHandler->FindDatum(eventEntry->m_resultSlotId);
        EXPECT_NE(resultDatum, nullptr);
    }

    for (auto&& genericUuidHandler : { uuidEventHandler, stringEventHandler })
    {
        auto eventEntry = genericUuidHandler->FindEvent("VectorCreatedEvent");
        const Datum* resultDatum = genericUuidHandler->FindDatum(eventEntry->m_resultSlotId);
        EXPECT_NE(resultDatum, nullptr);
    }

    graphEntity->Activate();

    for (auto&& genericUuidHandler : { uuidEventHandler, stringEventHandler })
    {
        auto eventEntry = genericUuidHandler->FindEvent("VectorCreatedEvent");
        const Datum* resultDatum = genericUuidHandler->FindDatum(eventEntry->m_resultSlotId);
        EXPECT_NE(resultDatum, nullptr);
    }


    ReportErrors(graph);    

    for (auto&& genericUuidHandler : { uuidEventHandler, stringEventHandler })
    {
        auto eventEntry = genericUuidHandler->FindEvent("VectorCreatedEvent");
        const Datum* resultDatum = genericUuidHandler->FindDatum(eventEntry->m_resultSlotId);
        EXPECT_NE(resultDatum, nullptr);
    }

    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
    TemplateEventTestBus<AZ::Uuid>::Event(uuidBusId, &TemplateEventTest<AZ::Uuid>::GenericEvent);
    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
    ReportErrors(graph);    

    for (auto&& genericUuidHandler : { uuidEventHandler, stringEventHandler })
    {
        auto eventEntry = genericUuidHandler->FindEvent("VectorCreatedEvent");
        const Datum* resultDatum = genericUuidHandler->FindDatum(eventEntry->m_resultSlotId);
        EXPECT_NE(resultDatum, nullptr);
    }


    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
    TemplateEventTestBus<AZStd::string>::Event(stringBusId, &TemplateEventTest<AZStd::string>::GenericEvent);
    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
    ReportErrors(graph);

    // string
    AZStd::string stringResultByUuid;
    AZStd::string stringResultByString;
    AZStd::string hello("Hello!");
    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
    TemplateEventTestBus<AZ::Uuid>::EventResult(stringResultByUuid, uuidBusId, &TemplateEventTest<AZ::Uuid>::UpdateNameEvent, hello);
    TemplateEventTestBus<AZStd::string>::EventResult(stringResultByString, stringBusId, &TemplateEventTest<AZStd::string>::UpdateNameEvent, hello);
    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
    ReportErrors(graph);

    EXPECT_EQ(hello, stringResultByUuid);
    EXPECT_EQ(hello, stringResultByString);
    EXPECT_EQ(hello, print2->GetText());

    for (auto&& genericUuidHandler : { uuidEventHandler, stringEventHandler })
    {
        auto eventEntry = genericUuidHandler->FindEvent("VectorCreatedEvent");
        const Datum* resultDatum = genericUuidHandler->FindDatum(eventEntry->m_resultSlotId);
        EXPECT_NE(resultDatum, nullptr);
    }

    // vector
    AZ::Vector3 vectorResultByUuid = AZ::Vector3::CreateZero();
    AZ::Vector3 vectorResultByString = AZ::Vector3::CreateZero();
    AZ::Vector3 sevenAteNine(7.0f, 8.0f, 9.0f);
    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
    TemplateEventTestBus<AZ::Uuid>::EventResult(vectorResultByUuid, uuidBusId, &TemplateEventTest<AZ::Uuid>::VectorCreatedEvent, sevenAteNine);
    TemplateEventTestBus<AZStd::string>::EventResult(vectorResultByString, stringBusId, &TemplateEventTest<AZStd::string>::VectorCreatedEvent, sevenAteNine);
    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
    ReportErrors(graph);

    AZStd::string vectorString = Datum(Data::Vector3Type(7, 8, 9)).ToString();
    EXPECT_EQ(sevenAteNine, vectorResultByUuid);
    EXPECT_EQ(sevenAteNine, vectorResultByString);
    EXPECT_EQ(vectorString, print3->GetText());

    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    StringView::Reflect(m_serializeContext);
    StringView::Reflect(m_behaviorContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_behaviorContext);
    TemplateEventTestHandler<AZStd::string>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZStd::string>::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
};

void ReflectSignCorrectly()
{
    AZ::BehaviorContext* behaviorContext(nullptr);
    AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
    AZ_Assert(behaviorContext, "A behavior context is required!");
    behaviorContext->Method("Sign", AZ::GetSign);
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ClassExposition)
{
    
    using namespace ScriptCanvas;

    ReflectSignCorrectly();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();
    AZ::EntityId startNodeID{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNodeID, graphUniqueId, azrtti_typeid<Nodes::Core::Start>());
    Nodes::Core::Start* startNode(nullptr);
    SystemRequestBus::BroadcastResult(startNode, &SystemRequests::GetNode<Nodes::Core::Start>, startNodeID);
    EXPECT_TRUE(startNode != nullptr);

    const TestBehaviorContextObject allOne(1);
    const TestBehaviorContextObject* allOnePtr = &allOne;

    AZ::Entity* vectorAEntity{ aznew AZ::Entity };
    vectorAEntity->Init();
    AZ::EntityId vectorANodeID{ vectorAEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, vectorANodeID, graphUniqueId, azrtti_typeid<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode>());
    ScriptCanvas::Nodes::Core::BehaviorContextObjectNode* vectorANode(nullptr);
    SystemRequestBus::BroadcastResult(vectorANode, &SystemRequests::GetNode<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode>, vectorANodeID);
    EXPECT_TRUE(vectorANode != nullptr);
    vectorANode->InitializeObject(AZStd::string("TestBehaviorContextObject"));
    SetInput_UNIT_TEST(vectorANode, "Set", allOne);

    const TestBehaviorContextObject* vectorANodeVector = GetInput_UNIT_TEST<TestBehaviorContextObject>(vectorANode, "Set");

    EXPECT_EQ(vectorANodeVector, allOnePtr);

    ScriptCanvas::Namespaces emptyNamespaces;

    AZ::Entity* vectorBEntity{ aznew AZ::Entity };
    vectorBEntity->Init();
    AZ::EntityId vectorBNodeID{ vectorBEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, vectorBNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* vectorBNode(nullptr);
    SystemRequestBus::BroadcastResult(vectorBNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, vectorBNodeID);
    EXPECT_TRUE(vectorBNode != nullptr);
    vectorBNode->InitializeObject(AZStd::string("TestBehaviorContextObject"));

    AZ::Entity* vectorCEntity{ aznew AZ::Entity };
    vectorCEntity->Init();
    AZ::EntityId vectorCNodeID{ vectorCEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, vectorCNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* vectorCNode(nullptr);
    SystemRequestBus::BroadcastResult(vectorCNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, vectorCNodeID);
    EXPECT_TRUE(vectorCNode != nullptr);
    vectorCNode->InitializeObject(AZStd::string("TestBehaviorContextObject"));

    AZ::Entity* normalizeEntity{ aznew AZ::Entity };
    normalizeEntity->Init();
    AZ::EntityId normalizeNodeID{ normalizeEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, normalizeNodeID, graphUniqueId, Nodes::Core::Method::RTTI_Type());
    Nodes::Core::Method* normalizeNode(nullptr);
    SystemRequestBus::BroadcastResult(normalizeNode, &SystemRequests::GetNode<Nodes::Core::Method>, normalizeNodeID);
    EXPECT_TRUE(normalizeNode != nullptr);
    normalizeNode->InitializeClass(emptyNamespaces, AZStd::string("TestBehaviorContextObject"), AZStd::string("Normalize"));

    AZ::Entity* getSignEntity{ aznew AZ::Entity };
    getSignEntity->Init();
    AZ::EntityId getSignNodeID{ getSignEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, getSignNodeID, graphUniqueId, Nodes::Core::Method::RTTI_Type());
    Nodes::Core::Method* getSignNode(nullptr);
    SystemRequestBus::BroadcastResult(getSignNode, &SystemRequests::GetNode<Nodes::Core::Method>, getSignNodeID);
    EXPECT_TRUE(getSignNode != nullptr);
    getSignNode->InitializeFree(emptyNamespaces, AZStd::string("Sign"));

    AZ::Entity* getSignEntity2{ aznew AZ::Entity };
    getSignEntity2->Init();
    AZ::EntityId getSignNodeID2{ getSignEntity2->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, getSignNodeID2, graphUniqueId, Nodes::Core::Method::RTTI_Type());
    Nodes::Core::Method* getSignNode2(nullptr);
    SystemRequestBus::BroadcastResult(getSignNode2, &SystemRequests::GetNode<Nodes::Core::Method>, getSignNodeID2);
    EXPECT_TRUE(getSignNode2 != nullptr);
    getSignNode2->InitializeFree(emptyNamespaces, AZStd::string("Sign"));

    AZ::Entity* floatPosEntity{ aznew AZ::Entity };
    floatPosEntity->Init();
    AZ::EntityId floatPosNodeID{ floatPosEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, floatPosNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* floatPosNode(nullptr);
    SystemRequestBus::BroadcastResult(floatPosNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, floatPosNodeID);
    EXPECT_TRUE(floatPosNode != nullptr);
    {
        ScopedOutputSuppression suppressOutput;
        floatPosNode->InitializeObject(azrtti_typeid<float>());
    }
    SetInput_UNIT_TEST(floatPosNode, "Set", 333.0f);

    AZ::Entity* signPosEntity{ aznew AZ::Entity };
    signPosEntity->Init();
    AZ::EntityId signPosNodeID{ signPosEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, signPosNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* signPosNode(nullptr);
    SystemRequestBus::BroadcastResult(signPosNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, signPosNodeID);
    EXPECT_TRUE(signPosNode != nullptr);
    {
        ScopedOutputSuppression suppressOutput;
        signPosNode->InitializeObject(azrtti_typeid<float>());
    }
    SetInput_UNIT_TEST(signPosNode, "Set", -333.0f);

    AZ::Entity* floatNegEntity{ aznew AZ::Entity };
    floatNegEntity->Init();
    AZ::EntityId floatNegNodeID{ floatNegEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, floatNegNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* floatNegNode(nullptr);
    SystemRequestBus::BroadcastResult(floatNegNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, floatNegNodeID);
    EXPECT_TRUE(floatNegNode != nullptr);
    {
        ScopedOutputSuppression suppressOutput;
        floatNegNode->InitializeObject(azrtti_typeid<float>());
    }
    SetInput_UNIT_TEST(floatNegNode, "Set", -333.0f);

    AZ::Entity* signNegEntity{ aznew AZ::Entity };
    signNegEntity->Init();
    AZ::EntityId signNegNodeID{ signNegEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, signNegNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* signNegNode(nullptr);
    SystemRequestBus::BroadcastResult(signNegNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, signNegNodeID);
    EXPECT_TRUE(signNegNode != nullptr);
    {
        ScopedOutputSuppression suppressOutput;
        signNegNode->InitializeObject(azrtti_typeid<float>());
    }
    SetInput_UNIT_TEST(signNegNode, "Set", 333.0f);


    EXPECT_TRUE(normalizeNode->GetSlotId("TestBehaviorContextObject: 0").IsValid());
    EXPECT_TRUE(vectorANode->GetSlotId("Get").IsValid());

    auto vector30 = normalizeNode->GetSlotId("TestBehaviorContextObject: 0");
    auto getID = vectorANode->GetSlotId("Get");

    EXPECT_TRUE(graph->Connect(startNodeID, startNode->GetSlotId("Out"), normalizeNodeID, normalizeNode->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(normalizeNodeID, normalizeNode->GetSlotId("TestBehaviorContextObject: 0"), vectorANodeID, vectorANode->GetSlotId("Get")));

    EXPECT_TRUE(graph->Connect(normalizeNodeID, normalizeNode->GetSlotId("Out"), getSignNodeID, getSignNode->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(getSignNodeID, getSignNode->GetSlotId("Number: 0"), floatPosNodeID, floatPosNode->GetSlotId("Get")));
    EXPECT_TRUE(graph->Connect(getSignNodeID, getSignNode->GetSlotId("Result: Number"), signPosNodeID, signPosNode->GetSlotId("Set")));

    EXPECT_TRUE(graph->Connect(getSignNodeID, getSignNode->GetSlotId("Out"), getSignNodeID2, getSignNode2->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(getSignNodeID2, getSignNode2->GetSlotId("Number: 0"), floatNegNodeID, floatNegNode->GetSlotId("Get")));
    EXPECT_TRUE(graph->Connect(getSignNodeID2, getSignNode2->GetSlotId("Result: Number"), signNegNodeID, signNegNode->GetSlotId("Set")));

    AZ::Entity entity("ScriptCanvasHandlerTest");
    entity.Init();
    entity.Activate();
    AZ::EntityId entityID(entity.GetId());

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Activate();
    ReportErrors(graph);

    if (vectorANodeVector)
    {
        EXPECT_TRUE(vectorANodeVector->IsNormalized()) << "TestBehaviorContextObject not normalized";
    }
    else
    {
        ADD_FAILURE() << "TestBehaviorContextObject type not found!";
    }

    if (auto positive = GetInput_UNIT_TEST<Data::NumberType>(signPosNode, "Set"))
    {
        EXPECT_EQ(*positive, 1.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto negative = GetInput_UNIT_TEST<Data::NumberType>(signNegNode, "Set"))
    {
        EXPECT_EQ(*negative, -1.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
};

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ObjectTrackingByValue)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByValueID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByValue");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode1, "Set")->SetValue(1);
    EXPECT_EQ(1, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode1, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode2, "Set")->SetValue(2);
    EXPECT_EQ(2, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode2, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set")->SetValue(3);
    EXPECT_EQ(3, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set")->SetValue(4);
    EXPECT_EQ(4, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set")->SetValue(5);
    EXPECT_EQ(5, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set")->GetValue());

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    ReportErrors(graph);

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set");
    auto value4 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set");
    auto value5 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ObjectTrackingByPointer)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByPointerID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByPointer");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode1, "Set")->SetValue(1);
    EXPECT_EQ(1, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode1, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode2, "Set")->SetValue(2);
    EXPECT_EQ(2, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode2, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set")->SetValue(3);
    EXPECT_EQ(3, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set")->SetValue(4);
    EXPECT_EQ(4, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set")->SetValue(5);
    EXPECT_EQ(5, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set")->GetValue());

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByPointerID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByPointerID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByPointerID, "In"));

    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    ReportErrors(graph);

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set");
    auto value4 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set");
    auto value5 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ObjectTrackingByReference)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByReferenceID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByReference");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode1, "Set")->SetValue(1);
    EXPECT_EQ(1, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode1, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode2, "Set")->SetValue(2);
    EXPECT_EQ(2, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode2, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set")->SetValue(3);
    EXPECT_EQ(3, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set")->SetValue(4);
    EXPECT_EQ(4, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set")->SetValue(5);
    EXPECT_EQ(5, GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set")->GetValue());

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByReferenceID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByReferenceID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByReferenceID, "In"));

    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    ReportErrors(graph);

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode3, "Set");
    auto value4 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode4, "Set");
    auto value5 = GetInput_UNIT_TEST<TestBehaviorContextObject>(valueNode5, "Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_InvalidInputByReference)
{    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startNodeId;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);

    AZ::EntityId maxReferenceNodeId = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByReference");

    AZ::EntityId valueObjectNodeId;

    Core::BehaviorContextObjectNode* valueNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueObjectNodeId);
    valueNode->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode, "Set")->SetValue(1);

    EXPECT_TRUE(Connect(*graph, valueObjectNodeId, "Get", maxReferenceNodeId, "TestBehaviorContextObject: 0"));

    // execution
    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", maxReferenceNodeId, "In"));

    {
        // MaxReturnByReferenceNode is missing a an input. So this should error.
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    const bool expectErrors = true;
    const bool expectIrrecoverableErrors = true;
    ReportErrors(graph, expectErrors, expectIrrecoverableErrors);

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_InvalidInputByValue)
{    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startNodeId;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);

    AZ::EntityId maxValueNodeId = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByValue");

    AZ::EntityId valueObjectNodeId;

    Core::BehaviorContextObjectNode* valueNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueObjectNodeId);
    valueNode->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    ModInput_UNIT_TEST<TestBehaviorContextObject>(valueNode, "Set")->SetValue(1);

    EXPECT_TRUE(Connect(*graph, valueObjectNodeId, "Get", maxValueNodeId, "TestBehaviorContextObject: 0"));

    // execution
    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", maxValueNodeId, "In"));

    {
        // MaxReturnByValue is missing a an input. So this should error.
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    const bool expectErrors = true;
    const bool expectIrrecoverableErrors = true;
    ReportErrors(graph, expectErrors, expectIrrecoverableErrors);

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ScriptCanvasValueDataTypesByValue)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByPointerID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByValueInteger");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    auto* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *GetInput_UNIT_TEST<Data::NumberType>(valueNode1, "Set"));

    auto* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *GetInput_UNIT_TEST<Data::NumberType>(valueNode2, "Set"));

    auto* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *GetInput_UNIT_TEST<Data::NumberType>(valueNode3, "Set"));

    auto* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *GetInput_UNIT_TEST<Data::NumberType>(valueNode4, "Set"));

    auto* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *GetInput_UNIT_TEST<Data::NumberType>(valueNode5, "Set"));

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByPointerID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByPointerID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByPointerID, "In"));

    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    ReportErrors(graph);    

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = GetInput_UNIT_TEST<Data::NumberType>(valueNode3, "Set");
    auto value4 = GetInput_UNIT_TEST<Data::NumberType>(valueNode4, "Set");
    auto value5 = GetInput_UNIT_TEST<Data::NumberType>(valueNode5, "Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
        EXPECT_NE(value4, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ScriptCanvasValueDataTypesByPointer)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByPointerID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByPointerInteger");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    auto* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *GetInput_UNIT_TEST<Data::NumberType>(valueNode1, "Set"));

    auto* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *GetInput_UNIT_TEST<Data::NumberType>(valueNode2, "Set"));

    auto* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *GetInput_UNIT_TEST<Data::NumberType>(valueNode3, "Set"));

    auto* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *GetInput_UNIT_TEST<Data::NumberType>(valueNode4, "Set"));

    auto* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *GetInput_UNIT_TEST<Data::NumberType>(valueNode5, "Set"));

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByPointerID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByPointerID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByPointerID, "In"));

    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    ReportErrors(graph);

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = GetInput_UNIT_TEST<Data::NumberType>(valueNode3, "Set");
    auto value4 = GetInput_UNIT_TEST<Data::NumberType>(valueNode4, "Set");
    auto value5 = GetInput_UNIT_TEST<Data::NumberType>(valueNode5, "Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
        EXPECT_NE(value4, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ScriptCanvasValueDataTypesByReference)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByReferenceID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByReferenceInteger");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    auto* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *GetInput_UNIT_TEST<Data::NumberType>(valueNode1, "Set"));

    auto* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *GetInput_UNIT_TEST<Data::NumberType>(valueNode2, "Set"));

    auto* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *GetInput_UNIT_TEST<Data::NumberType>(valueNode3, "Set"));

    auto* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *GetInput_UNIT_TEST<Data::NumberType>(valueNode4, "Set"));

    auto* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *GetInput_UNIT_TEST<Data::NumberType>(valueNode5, "Set"));

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByReferenceID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByReferenceID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByReferenceID, "In"));

    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    ReportErrors(graph);

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = GetInput_UNIT_TEST<Data::NumberType>(valueNode3, "Set");
    auto value4 = GetInput_UNIT_TEST<Data::NumberType>(valueNode4, "Set");
    auto value5 = GetInput_UNIT_TEST<Data::NumberType>(valueNode5, "Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
        EXPECT_NE(value4, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ScriptCanvasStringToNonAZStdString)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    ConvertibleToString::Reflect(m_serializeContext);
    ConvertibleToString::Reflect(m_behaviorContext);

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_NE(nullptr, graph);
    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startId;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startId);

    AZ::EntityId constCharPtrToStringId = CreateClassFunctionNode(graphUniqueId, "ConvertibleToString", "ConstCharPtrToString");
    AZ::EntityId stringViewToStringId = CreateClassFunctionNode(graphUniqueId, "ConvertibleToString", "StringViewToString");

    AZ::EntityId stringId1, stringId2;
    AZ::EntityId resultValueId1, resultValueId2;

    auto* stringNode1 = CreateDataNode<Data::StringType>(graphUniqueId, "Hello", stringId1);
    EXPECT_EQ("Hello", *GetInput_UNIT_TEST<Data::StringType>(stringNode1, "Set"));

    auto* stringNode2 = CreateDataNode<Data::StringType>(graphUniqueId, "World", stringId2);
    EXPECT_EQ("World", *GetInput_UNIT_TEST<Data::StringType>(stringNode2, "Set"));

    auto* resultValueNode1 = CreateDataNode<Data::StringType>(graphUniqueId, "", resultValueId1);
    auto* resultValueNode2 = CreateDataNode<Data::StringType>(graphUniqueId, "", resultValueId2);

    // data
    EXPECT_TRUE(Connect(*graph, stringId1, "Get", constCharPtrToStringId, "String: 0"));
    EXPECT_TRUE(Connect(*graph, stringId2, "Get", stringViewToStringId, "String: 0"));
    EXPECT_TRUE(Connect(*graph, constCharPtrToStringId, "Result: String", resultValueId1, "Set"));
    EXPECT_TRUE(Connect(*graph, stringViewToStringId, "Result: String", resultValueId2, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startId, "Out", constCharPtrToStringId, "In"));
    EXPECT_TRUE(Connect(*graph, startId, "Out", stringViewToStringId, "In"));

    {
        //ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();
    }

    ReportErrors(graph);    

    auto constCharToStringValue = GetInput_UNIT_TEST<Data::StringType>(resultValueNode1, "Set");
    auto stringViewToStringValue = GetInput_UNIT_TEST<Data::StringType>(resultValueNode2, "Set");

    EXPECT_NE(nullptr, constCharToStringValue);
    EXPECT_NE(nullptr, stringViewToStringValue);
    if (constCharToStringValue && stringViewToStringValue)
    {
        EXPECT_EQ("Hello", *constCharToStringValue);
        EXPECT_EQ("World", *stringViewToStringValue);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    ConvertibleToString::Reflect(m_serializeContext);
    ConvertibleToString::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_BusHandler)
{
    
    using namespace ScriptCanvas;

    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId eventHandlerID;
    Nodes::Core::EBusEventHandler* eventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, eventHandlerID);
    eventHandler->InitializeBus("EventTestHandler");

    // number event
    AZ::EntityId numberHandlerID;
    Nodes::Core::EBusEventHandler* numberHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, numberHandlerID);
    numberHandler->InitializeBus("EventTestHandler");

    int threeInt(3);
    AZ::EntityId threeID;
    CreateDataNode(graphUniqueId, threeInt, threeID);
    AZ::EntityId sumID;
    auto sumNode = CreateTestNode<Nodes::Math::Sum>(graphUniqueId, sumID);

    // string event
    AZ::EntityId stringHandlerID;
    Nodes::Core::EBusEventHandler* stringHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, stringHandlerID);
    stringHandler->InitializeBus("EventTestHandler");

    AZ::EntityId stringID;
    Nodes::Core::String* string = CreateTestNode<Nodes::Core::String>(graphUniqueId, stringID);
    AZStd::string goodBye("good bye!");
    SetInput_UNIT_TEST(string, "Set", goodBye);

    // objects event
    AZ::EntityId vectorHandlerID;
    Nodes::Core::EBusEventHandler* vectorHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, vectorHandlerID);
    vectorHandler->InitializeBus("EventTestHandler");

    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "TestBehaviorContextObject", "Normalize");

    AZ::EntityId objectNodeId;
    Nodes::Core::BehaviorContextObjectNode* objectNodePtr = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, objectNodeId);
    const AZStd::string objectClassName("TestBehaviorContextObject");
    objectNodePtr->InitializeObject(objectClassName);

    // print (for strings, and generic event)
    AZ::EntityId printID;
    TestResult* print = CreateTestNode<TestResult>(graphUniqueId, printID);
    print->SetText("print was un-initialized");    

    AZ::EntityId print2ID;
    TestResult* print2 = CreateTestNode<TestResult>(graphUniqueId, print2ID);
    print2->SetText("print was un-initialized");

    AZ::EntityId print3ID;
    TestResult* print3 = CreateTestNode<TestResult>(graphUniqueId, print3ID);
    print3->SetText("print was un-initialized");

    auto eventEntry = eventHandler->FindEvent("Event");
    EXPECT_NE(nullptr, eventEntry);
    EXPECT_TRUE(graph->Connect(eventHandlerID, eventEntry->m_eventSlotId, printID, print->GetSlotId("In")));
    
    auto stringEventEntry = stringHandler->FindEvent("String");
    EXPECT_NE(nullptr, stringEventEntry);
    EXPECT_FALSE(stringEventEntry->m_parameterSlotIds.empty());
    EXPECT_TRUE(graph->Connect(stringHandlerID, stringEventEntry->m_eventSlotId, printID, print->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(stringHandlerID, stringEventEntry->m_parameterSlotIds[0], printID, print->GetSlotId("Value")));
    EXPECT_TRUE(graph->Connect(stringHandlerID, stringEventEntry->m_parameterSlotIds[0], stringID, string->GetSlotId("Set")));
    EXPECT_TRUE(graph->Connect(stringHandlerID, stringEventEntry->m_resultSlotId, stringID, string->GetSlotId("Get")));

    auto numberEventEntry = numberHandler->FindEvent("Number");
    EXPECT_NE(nullptr, numberEventEntry);
    EXPECT_FALSE(numberEventEntry->m_parameterSlotIds.empty());
    EXPECT_TRUE(graph->Connect(numberHandlerID, numberEventEntry->m_eventSlotId, sumID, sumNode->GetSlotId(Nodes::BinaryOperator::k_evaluateName)));
    EXPECT_TRUE(Connect(*graph, sumID, Nodes::BinaryOperator::k_outName, print3ID, "In"));
    EXPECT_TRUE(graph->Connect(numberHandlerID, numberEventEntry->m_parameterSlotIds[0], print3ID, print3->GetSlotId("Value")));
    EXPECT_TRUE(graph->Connect(numberHandlerID, numberEventEntry->m_parameterSlotIds[0], sumID, sumNode->GetSlotId(Nodes::BinaryOperator::k_lhsName)));
    EXPECT_TRUE(graph->Connect(numberHandlerID, numberEventEntry->m_resultSlotId, sumID, sumNode->GetSlotId(Nodes::BinaryOperator::k_resultName)));
    EXPECT_TRUE(Connect(*graph, threeID, "Get", sumID, Nodes::BinaryOperator::k_rhsName));    

    auto objectEventEntry = vectorHandler->FindEvent("Object");
    EXPECT_NE(nullptr, objectEventEntry);
    EXPECT_FALSE(objectEventEntry->m_parameterSlotIds.empty());
    EXPECT_TRUE(graph->Connect(vectorHandlerID, objectEventEntry->m_eventSlotId, print2ID, print2->GetSlotId("In")));
    EXPECT_TRUE(Connect(*graph, print2ID, "Out", normalizeID, "In"));

    EXPECT_TRUE(graph->Connect(vectorHandlerID, objectEventEntry->m_parameterSlotIds[0], objectNodeId, objectNodePtr->GetSlotId("Set")));
    EXPECT_TRUE(graph->Connect(vectorHandlerID, objectEventEntry->m_resultSlotId, objectNodeId, objectNodePtr->GetSlotId("Get")));
    EXPECT_TRUE(Connect(*graph, objectNodeId, "Get", normalizeID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, objectNodeId, "Get", print2ID, "Value"));

    AZ::Entity* graphEntity = graph->GetEntity();
    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
    graphEntity->Activate();
    ReportErrors(graph);

    // just don't crash
    EventTestBus::Event(graphEntityID, &EventTest::Event);
    ReportErrors(graph);

    // string
    AZ::EBusAggregateResults<AZStd::string> stringResults;
    AZStd::string hello("Hello!");
    EventTestBus::EventResult(stringResults, graphEntityID, &EventTest::String, hello);
    auto iterString = AZStd::find(stringResults.values.begin(), stringResults.values.end(), hello);
    EXPECT_NE(iterString, stringResults.values.end());
    EXPECT_EQ(print->GetText(), hello);
    ReportErrors(graph);

    // number
    AZ::EBusAggregateResults<int> numberResults;
    int four(4);
    EventTestBus::EventResult(numberResults, graphEntityID, &EventTest::Number, four);
    int seven(7);
    auto interNumber = AZStd::find(numberResults.values.begin(), numberResults.values.end(), seven);
    EXPECT_NE(interNumber, numberResults.values.end());
    AZStd::string sevenString = AZStd::string("4.000000");
    EXPECT_EQ(print3->GetText(), sevenString);
    ReportErrors(graph);

    // vector
    AZ::EBusAggregateResults<TestBehaviorContextObject> vectorResults;
    TestBehaviorContextObject allSeven;
    void* allSevenVoidPtr = reinterpret_cast<void*>(&allSeven);
    EventTestBus::EventResult(vectorResults, graphEntityID, &EventTest::Object, allSeven);
    allSeven.Normalize();
    auto iterVector = AZStd::find(vectorResults.values.begin(), vectorResults.values.end(), allSeven);
    EXPECT_NE(iterVector, vectorResults.values.end());
    ReportErrors(graph);
    TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, GetterSetterProperties)
{
    

    using namespace ScriptCanvas;
    using namespace Nodes;

    TestBehaviorContextProperties::Reflect(m_serializeContext);
    TestBehaviorContextProperties::Reflect(m_behaviorContext);

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("Graph");
    Graph* graph{};
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::CreateGraphOnEntity, graphEntity.get());
    ASSERT_NE(nullptr, graph);
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    // Logic Nodes
    AZ::EntityId outID;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, outID);
    auto printNode = CreateTestNode<TestResult>(graphUniqueId, outID);
    auto printNode2 = CreateTestNode<TestResult>(graphUniqueId, outID);
    auto printNode3 = CreateTestNode<TestResult>(graphUniqueId, outID);

    // Logic
    EXPECT_TRUE(graph->Connect(startNode->GetEntityId(), startNode->GetSlotId("Out"), printNode->GetEntityId(), printNode->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(printNode->GetEntityId(), printNode->GetSlotId("Out"), printNode2->GetEntityId(), printNode2->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(printNode2->GetEntityId(), printNode2->GetSlotId("Out"), printNode3->GetEntityId(), printNode3->GetSlotId("In")));

    // Data Nodes
    TestBehaviorContextProperties behaviorContextProperties;
    behaviorContextProperties.m_booleanProp = true;
    behaviorContextProperties.m_numberProp = 11.90f;
    behaviorContextProperties.m_stringProp = "NULL";
    behaviorContextProperties.m_getterOnlyProp = 10.0;
    behaviorContextProperties.m_setterOnlyProp = 5.0;
    auto behaviorContextPropertyNode = CreateDataNode(graphUniqueId, behaviorContextProperties, outID);

    auto vector4Node = CreateDataNode(graphUniqueId, Data::Vector4Type(1.0f, 2.0f, 3.0f, 4.0f), outID);

    auto matrix4x4Node = CreateDataNode(graphUniqueId, Data::Matrix4x4Type::CreateFromColumns(
        Data::Vector4Type(1.0f, 2.0f, 3.0f, 4.0f),
        Data::Vector4Type(5.0f, 2.0f, 3.0f, 4.0f),
        Data::Vector4Type(9.0f, 2.0f, 3.0f, 4.0f),
        Data::Vector4Type(13.0f, 2.0f, 3.0f, 4.0f)
    ), outID);

    {
        // Input Data
        auto sourceNumberNode = CreateDataNode(graphUniqueId, 20.0, outID);
        auto sourceStringNode = CreateDataNode(graphUniqueId, Data::StringType("Set Value"), outID);

        // Output Data
        auto resultNumberNode = CreateDataNode(graphUniqueId, 0.0, outID);
        auto resultStringNode = CreateDataNode(graphUniqueId, Data::StringType(), outID);

        // Data connections for Behavior Context Properties
        // This should fail getterOnlyNumber is a property without a setter
        {
            ScopedOutputSuppression suppressOutput;
            EXPECT_TRUE(!graph->Connect(sourceNumberNode->GetEntityId(), sourceNumberNode->GetSlotId("Get"), behaviorContextPropertyNode->GetEntityId(), behaviorContextPropertyNode->GetSlotId("Number: getterOnlyNumber")));
        }

        EXPECT_TRUE(graph->Connect(sourceNumberNode->GetEntityId(), sourceNumberNode->GetSlotId("Get"), behaviorContextPropertyNode->GetEntityId(), behaviorContextPropertyNode->GetSlotId("Number: number")));
        EXPECT_TRUE(graph->Connect(sourceNumberNode->GetEntityId(), sourceNumberNode->GetSlotId("Get"), behaviorContextPropertyNode->GetEntityId(), behaviorContextPropertyNode->GetSlotId("Number: setterOnlyNumber")));
        EXPECT_TRUE(graph->Connect(sourceStringNode->GetEntityId(), sourceStringNode->GetSlotId("Get"), behaviorContextPropertyNode->GetEntityId(), behaviorContextPropertyNode->GetSlotId("String: string")));

        // This should fail setterOnlyNumber is a property without a getter
        {
            ScopedOutputSuppression suppressOutput;
            EXPECT_TRUE(!graph->Connect(behaviorContextPropertyNode->GetEntityId(), behaviorContextPropertyNode->GetSlotId("setterOnlyNumber: Number"), resultNumberNode->GetEntityId(), resultNumberNode->GetSlotId("Set")));
        }

        EXPECT_TRUE(graph->Connect(behaviorContextPropertyNode->GetEntityId(), behaviorContextPropertyNode->GetSlotId("getterOnlyNumber: Number"), resultNumberNode->GetEntityId(), resultNumberNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(behaviorContextPropertyNode->GetEntityId(), behaviorContextPropertyNode->GetSlotId("string: String"), resultStringNode->GetEntityId(), resultStringNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(resultStringNode->GetEntityId(), resultStringNode->GetSlotId("Get"), printNode3->GetEntityId(), printNode3->GetSlotId("Value")));

        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
        graphEntity->Activate();
        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
        ReportErrors(graph);
        graphEntity->Deactivate();

        const float tolerance = 0.00001f;
        auto bcPropertyTestObj = GetInput_UNIT_TEST<TestBehaviorContextProperties>(behaviorContextPropertyNode, "Set");
        ASSERT_NE(nullptr, bcPropertyTestObj);
        EXPECT_TRUE(bcPropertyTestObj->m_booleanProp);
        EXPECT_NEAR(20.0, bcPropertyTestObj->m_numberProp, tolerance);
        EXPECT_EQ("Set Value", bcPropertyTestObj->m_stringProp);
        EXPECT_NEAR(10.0, bcPropertyTestObj->m_getterOnlyProp, tolerance);
        EXPECT_NEAR(20.0, bcPropertyTestObj->m_setterOnlyProp, tolerance);

        auto resultString = GetInput_UNIT_TEST<Data::StringType>(resultStringNode, "Set");
        ASSERT_NE(nullptr, resultString);
        EXPECT_EQ("Set Value", *resultString);

        auto resultNumber = GetInput_UNIT_TEST<Data::NumberType>(resultNumberNode, "Set");
        ASSERT_NE(nullptr, resultNumber);
        EXPECT_NEAR(10.0, *resultNumber, tolerance);
    }

    // Data connections for Vector Properties
    {
        auto testNumberNode1 = CreateDataNode(graphUniqueId, 9000.1, outID);
        auto testNumberNode2 = CreateDataNode(graphUniqueId, 31337.0, outID);
        auto resultXNode = CreateDataNode(graphUniqueId, 0.0, outID);
        auto resultYNode = CreateDataNode(graphUniqueId, 0.0, outID);
        auto resultZNode = CreateDataNode(graphUniqueId, 0.0, outID);
        auto resultWNode = CreateDataNode(graphUniqueId, 0.0, outID);
        EXPECT_TRUE(graph->Connect(testNumberNode1->GetEntityId(), testNumberNode1->GetSlotId("Get"), vector4Node->GetEntityId(), vector4Node->GetSlotId("Number: x")));
        EXPECT_TRUE(graph->Connect(testNumberNode2->GetEntityId(), testNumberNode2->GetSlotId("Get"), vector4Node->GetEntityId(), vector4Node->GetSlotId("Number: z")));
        EXPECT_TRUE(graph->Connect(vector4Node->GetEntityId(), vector4Node->GetSlotId("x: Number"), resultXNode->GetEntityId(), resultXNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(vector4Node->GetEntityId(), vector4Node->GetSlotId("y: Number"), resultYNode->GetEntityId(), resultYNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(vector4Node->GetEntityId(), vector4Node->GetSlotId("z: Number"), resultZNode->GetEntityId(), resultZNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(vector4Node->GetEntityId(), vector4Node->GetSlotId("w: Number"), resultWNode->GetEntityId(), resultWNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(vector4Node->GetEntityId(), vector4Node->GetSlotId("Get"), printNode->GetEntityId(), printNode->GetSlotId("Value")));

        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
        graphEntity->Activate();
        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
        ReportErrors(graph);
        graphEntity->Deactivate();

        const float tolerance = 0.01f;
        auto vector4Value = GetInput_UNIT_TEST<Data::Vector4Type>(vector4Node, "Set");
        ASSERT_NE(nullptr, vector4Value);
        EXPECT_NEAR(9000.1, vector4Value->GetX(), tolerance);
        EXPECT_NEAR(2.0, vector4Value->GetY(), tolerance);
        EXPECT_NEAR(31337.0, vector4Value->GetZ(), tolerance);
        EXPECT_NEAR(4.0, vector4Value->GetW(), tolerance);

        auto resultValue = GetInput_UNIT_TEST<Data::NumberType>(resultXNode, "Set");
        ASSERT_NE(nullptr, resultValue);
        EXPECT_NEAR(9000.1, *resultValue, tolerance);

        resultValue = GetInput_UNIT_TEST<Data::NumberType>(resultYNode, "Set");
        ASSERT_NE(nullptr, resultValue);
        EXPECT_NEAR(2.0, *resultValue, tolerance);

        resultValue = GetInput_UNIT_TEST<Data::NumberType>(resultZNode, "Set");
        ASSERT_NE(nullptr, resultValue);
        EXPECT_NEAR(31337.0, *resultValue, tolerance);

        resultValue = GetInput_UNIT_TEST<Data::NumberType>(resultWNode, "Set");
        ASSERT_NE(nullptr, resultValue);
        EXPECT_NEAR(4.0, *resultValue, tolerance);
    }

    // Data connections for Matrix Properties
    {
        auto testYBasisNode = CreateDataNode(graphUniqueId, Data::Vector4Type(15.0, 20.0, -18.0, 17.0), outID);
        auto testPositionNode = CreateDataNode(graphUniqueId, Data::Vector3Type(0.0, 45.6, 0.0), outID);

        auto resultXBasisNode = CreateDataNode(graphUniqueId, Data::Vector4Type::CreateZero(), outID);
        auto resultYBasisNode = CreateDataNode(graphUniqueId, Data::Vector4Type::CreateZero(), outID);
        auto resultZBasisNode = CreateDataNode(graphUniqueId, Data::Vector4Type::CreateZero(), outID);
        auto resultPositionNode = CreateDataNode(graphUniqueId, Data::Vector3Type::CreateZero(), outID);
        EXPECT_TRUE(graph->Connect(testYBasisNode->GetEntityId(), testYBasisNode->GetSlotId("Get"), matrix4x4Node->GetEntityId(), matrix4x4Node->GetSlotId("Vector4: basisY")));
        EXPECT_TRUE(graph->Connect(testPositionNode->GetEntityId(), testPositionNode->GetSlotId("Get"), matrix4x4Node->GetEntityId(), matrix4x4Node->GetSlotId("Vector3: translation")));
        EXPECT_TRUE(graph->Connect(matrix4x4Node->GetEntityId(), matrix4x4Node->GetSlotId("basisX: Vector4"), resultXBasisNode->GetEntityId(), resultXBasisNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(matrix4x4Node->GetEntityId(), matrix4x4Node->GetSlotId("basisY: Vector4"), resultYBasisNode->GetEntityId(), resultYBasisNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(matrix4x4Node->GetEntityId(), matrix4x4Node->GetSlotId("basisZ: Vector4"), resultZBasisNode->GetEntityId(), resultZBasisNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(matrix4x4Node->GetEntityId(), matrix4x4Node->GetSlotId("translation: Vector3"), resultPositionNode->GetEntityId(), resultPositionNode->GetSlotId("Set")));
        EXPECT_TRUE(graph->Connect(matrix4x4Node->GetEntityId(), matrix4x4Node->GetSlotId("Get"), printNode2->GetEntityId(), printNode2->GetSlotId("Value")));

        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
        graphEntity->Activate();
        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
        ReportErrors(graph);
        graphEntity->Deactivate();

        auto testYBasisValue = GetInput_UNIT_TEST<Data::Vector4Type>(testYBasisNode, "Set");
        ASSERT_NE(nullptr, testYBasisValue);
        auto testPositionValue = GetInput_UNIT_TEST<Data::Vector3Type>(testPositionNode, "Set");
        ASSERT_NE(nullptr, testPositionValue);

        const float tolerance = 0.00001f;
        auto matrix4x4Value = GetInput_UNIT_TEST<Data::Matrix4x4Type>(matrix4x4Node, "Set");
        ASSERT_NE(nullptr, matrix4x4Value);
        EXPECT_EQ(Data::Vector4Type(1.0f, 2.0f, 3.0f, 4.0f), matrix4x4Value->GetBasisX());
        EXPECT_EQ(*testYBasisValue, matrix4x4Value->GetBasisY());
        EXPECT_EQ(Data::Vector4Type(9.0f, 2.0f, 3.0f, 4.0f), matrix4x4Value->GetBasisZ());
        EXPECT_EQ(*testPositionValue, matrix4x4Value->GetTranslation());

        auto resultBasisValue = GetInput_UNIT_TEST<Data::Vector4Type>(resultXBasisNode, "Set");
        ASSERT_NE(nullptr, resultBasisValue);
        EXPECT_EQ(Data::Vector4Type(1.0f, 2.0f, 3.0f, 4.0f), *resultBasisValue);

        resultBasisValue = GetInput_UNIT_TEST<Data::Vector4Type>(resultYBasisNode, "Set");
        ASSERT_NE(nullptr, resultBasisValue);
        EXPECT_EQ(*testYBasisValue, *resultBasisValue);

        resultBasisValue = GetInput_UNIT_TEST<Data::Vector4Type>(resultZBasisNode, "Set");
        ASSERT_NE(nullptr, resultBasisValue);
        EXPECT_EQ(Data::Vector4Type(9.0f, 2.0f, 3.0f, 4.0f), *resultBasisValue);

        auto resultPositionValue = GetInput_UNIT_TEST<Data::Vector3Type>(resultPositionNode, "Set");
        ASSERT_NE(nullptr, resultPositionValue);
        EXPECT_EQ(*testPositionValue, *resultPositionValue);
    }

    graphEntity.reset();

    m_serializeContext->EnableRemoveReflection();
    TestBehaviorContextProperties::Reflect(m_serializeContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextProperties::Reflect(m_behaviorContext);
    m_behaviorContext->DisableRemoveReflection();
}
