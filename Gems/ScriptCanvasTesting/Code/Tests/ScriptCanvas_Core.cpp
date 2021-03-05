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

#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>

using namespace ScriptCanvasTests;
using namespace TestNodes;

TEST_F(ScriptCanvasTestFixture, CoreNodeFunction_OwningGraphCheck)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* groupedNode = CreateConfigurableNode();
    
    EXPECT_EQ(graph, groupedNode->GetGraph());
}

TEST_F(ScriptCanvasTestFixture, AddRemoveSlot)
{
    

    RegisterComponentDescriptor<AddNodeWithRemoveSlot>();

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("RemoveSlotGraph");
    graphEntity->Init();
    Graph* graph{};
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::CreateGraphOnEntity, graphEntity.get());

    ScriptCanvasId graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId outID;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, outID);
    auto numberAddNode = CreateTestNode<AddNodeWithRemoveSlot>(graphUniqueId, outID);

    auto number1Node = CreateDataNode<Data::NumberType>(graphUniqueId, 1.0, outID);
    auto number2Node = CreateDataNode<Data::NumberType>(graphUniqueId, 2.0, outID);
    auto number3Node = CreateDataNode<Data::NumberType>(graphUniqueId, 4.0, outID);
    auto numberResultNode = CreateDataNode<Data::NumberType>(graphUniqueId, 0.0, outID);

    // data
    EXPECT_TRUE(Connect(*graph, number1Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "A"));
    EXPECT_TRUE(Connect(*graph, number2Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "B"));
    EXPECT_TRUE(Connect(*graph, number3Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "C"));
    EXPECT_TRUE(Connect(*graph, numberAddNode->GetEntityId(), "Result", numberResultNode->GetEntityId(), "Set"));

    // logic
    EXPECT_TRUE(Connect(*graph, startNode->GetEntityId(), "Out", numberAddNode->GetEntityId(), "In"));

    // execute pre remove
    graphEntity->Activate();
    ReportErrors(graph);    
    graphEntity->Deactivate();

    if (auto result = GetInput_UNIT_TEST<Data::NumberType>(numberResultNode, "Set"))
    {
        SC_EXPECT_DOUBLE_EQ(7.0, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    // Remove the second number slot from the AddNodeWithRemoveSlot node(the one that contains value = 2.0)
    numberAddNode->RemoveSlot(numberAddNode->GetSlotId("B"));
    EXPECT_FALSE(numberAddNode->GetSlotId("B").IsValid());

    // execute post-remove
    graphEntity->Activate();
    ReportErrors(graph);
    graphEntity->Deactivate();

    if (auto result = GetInput_UNIT_TEST<Data::NumberType>(numberResultNode, "Set"))
    {
        SC_EXPECT_DOUBLE_EQ(5.0, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    // Add two more slots and set there values to 8.0 and 16.0
    EXPECT_TRUE(numberAddNode->AddSlot("D").IsValid());
    EXPECT_TRUE(numberAddNode->AddSlot("B").IsValid());
    auto number4Node = CreateDataNode<Data::NumberType>(graphUniqueId, 8.0, outID);
    auto number5Node = CreateDataNode<Data::NumberType>(graphUniqueId, 16.0, outID);
    EXPECT_TRUE(Connect(*graph, number4Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "D"));
    EXPECT_TRUE(Connect(*graph, number5Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "B"));

    // execute post-add of slot "D" and re-add of slot "A"
    graphEntity->Activate();
    ReportErrors(graph);
    graphEntity->Deactivate();

    if (auto result = GetInput_UNIT_TEST<Data::NumberType>(numberResultNode, "Set"))
    {
        SC_EXPECT_DOUBLE_EQ(1.0 + 4.0 + 8.0 + 16.0, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    // Remove the first number slot from the AddNodeWithRemoveSlot node(the one that contains value = 1.0
    numberAddNode->RemoveSlot(numberAddNode->GetSlotId("A"));
    EXPECT_FALSE(numberAddNode->GetSlotId("A").IsValid());

    // execute post-remove of "A" slot
    graphEntity->Activate();
    ReportErrors(graph);
    graphEntity->Deactivate();

    if (auto result = GetInput_UNIT_TEST<Data::NumberType>(numberResultNode, "Set"))
    {
        SC_EXPECT_DOUBLE_EQ(4.0 + 8.0 + 16.0, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, AddRemoveSlotNotifications)
{
    RegisterComponentDescriptor<AddNodeWithRemoveSlot>();

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> numberAddEntity = AZStd::make_unique<AZ::Entity>("numberAddEntity");
    auto numberAddNode = numberAddEntity->CreateComponent<AddNodeWithRemoveSlot>();
    numberAddEntity->Init();

    ScriptUnitTestNodeNotificationHandler nodeNotifications(numberAddNode->GetEntityId());

    SlotId testSlotId = numberAddNode->AddSlot("test");
    EXPECT_EQ(1U, nodeNotifications.GetSlotsAdded());
    numberAddNode->RemoveSlot(testSlotId);
    EXPECT_EQ(1U, nodeNotifications.GetSlotsRemoved());

    testSlotId = numberAddNode->AddSlot("duplicate");
    EXPECT_EQ(2U, nodeNotifications.GetSlotsAdded());
    // This should not signal the NodeNotification::OnSlotAdded event as the slot already exist on the node
    SlotId duplicateSlotId = numberAddNode->AddSlot("duplicate");
    EXPECT_EQ(2U, nodeNotifications.GetSlotsAdded());
    EXPECT_EQ(testSlotId, duplicateSlotId);
    
    numberAddNode->RemoveSlot(testSlotId);
    EXPECT_EQ(2U, nodeNotifications.GetSlotsRemoved());
    // This should not signal the NodeNotification::OnSlotRemoved event as the slot no longer exist on the node
    numberAddNode->RemoveSlot(testSlotId); 
    EXPECT_EQ(2U, nodeNotifications.GetSlotsRemoved());

    numberAddEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, InsertSlot_Basic)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* basicNode = CreateConfigurableNode();

    SlotId firstSlotAdded;
    SlotId secondSlotAdded;
    SlotId thirdSlotAdded;

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "A";
        slotConfiguration.SetDefaultValue(0);
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        Slot* slot = basicNode->AddTestingSlot(slotConfiguration);

        firstSlotAdded = slot->GetId();
    }

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "C";
        slotConfiguration.SetDefaultValue(2);
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        Slot* slot = basicNode->AddTestingSlot(slotConfiguration);

        secondSlotAdded = slot->GetId();
    }

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "B";
        slotConfiguration.SetDefaultValue(1);
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        auto index = basicNode->FindSlotIndex(secondSlotAdded);
        EXPECT_EQ(index, 1);

        Slot* slot = basicNode->InsertTestingSlot(index, slotConfiguration);

        thirdSlotAdded = slot->GetId();
    }

    {
        AZStd::vector< const Slot* > slotList = basicNode->GetAllSlots();

        EXPECT_EQ(slotList.size(), 3);

        {
            const Slot* firstSlot = slotList[0];
            EXPECT_EQ(firstSlot->GetId(), firstSlotAdded);
            EXPECT_EQ(firstSlot->GetName(), "A");
            EXPECT_FLOAT_EQ(static_cast<float>((*firstSlot->FindDatum()->GetAs<Data::NumberType>())), 0.0f);
        }

        {
            const Slot* secondSlot = slotList[1];
            EXPECT_EQ(secondSlot->GetId(), thirdSlotAdded);
            EXPECT_EQ(secondSlot->GetName(), "B");
            EXPECT_FLOAT_EQ(static_cast<float>((*secondSlot->FindDatum()->GetAs<Data::NumberType>())), 1.0f);
        }

        {
            const Slot* thirdSlot = slotList[2];
            EXPECT_EQ(thirdSlot->GetId(), secondSlotAdded);
            EXPECT_EQ(thirdSlot->GetName(), "C");
            EXPECT_FLOAT_EQ(static_cast<float>((*thirdSlot->FindDatum()->GetAs<Data::NumberType>())), 2.0f);
        }
    }

    graph->Activate();

    {
        AZStd::vector< const Slot* > slotList = basicNode->GetAllSlots();

        EXPECT_EQ(slotList.size(), 3);

        {
            const Slot* firstSlot = slotList[0];
            EXPECT_EQ(firstSlot->GetId(), firstSlotAdded);
            EXPECT_EQ(firstSlot->GetName(), "A");
            EXPECT_FLOAT_EQ(static_cast<float>((*firstSlot->FindDatum()->GetAs<Data::NumberType>())), 0.0f);
        }

        {
            const Slot* secondSlot = slotList[1];
            EXPECT_EQ(secondSlot->GetId(), thirdSlotAdded);
            EXPECT_EQ(secondSlot->GetName(), "B");
            EXPECT_FLOAT_EQ(static_cast<float>((*secondSlot->FindDatum()->GetAs<Data::NumberType>())), 1.0f);
        }

        {
            const Slot* thirdSlot = slotList[2];
            EXPECT_EQ(thirdSlot->GetId(), secondSlotAdded);
            EXPECT_EQ(thirdSlot->GetName(), "C");
            EXPECT_FLOAT_EQ(static_cast<float>((*thirdSlot->FindDatum()->GetAs<Data::NumberType>())), 2.0f);
        }
    }

    graph->Deactivate();

    {
        AZStd::vector< const Slot* > slotList = basicNode->GetAllSlots();

        EXPECT_EQ(slotList.size(), 3);

        {
            const Slot* firstSlot = slotList[0];
            EXPECT_EQ(firstSlot->GetId(), firstSlotAdded);
            EXPECT_EQ(firstSlot->GetName(), "A");
            EXPECT_FLOAT_EQ(static_cast<float>((*firstSlot->FindDatum()->GetAs<Data::NumberType>())), 0.0f);
        }

        {
            const Slot* secondSlot = slotList[1];
            EXPECT_EQ(secondSlot->GetId(), thirdSlotAdded);
            EXPECT_EQ(secondSlot->GetName(), "B");
            EXPECT_FLOAT_EQ(static_cast<float>((*secondSlot->FindDatum()->GetAs<Data::NumberType>())), 1.0f);
        }

        {
            const Slot* thirdSlot = slotList[2];
            EXPECT_EQ(thirdSlot->GetId(), secondSlotAdded);
            EXPECT_EQ(thirdSlot->GetName(), "C");
            EXPECT_FLOAT_EQ(static_cast<float>((*thirdSlot->FindDatum()->GetAs<Data::NumberType>())), 2.0f);
        }
    }
}

TEST_F(ScriptCanvasTestFixture, InsertSlot_FrontPadded)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* basicNode = CreateConfigurableNode();

    basicNode->AddTestingSlot(ExecutionSlotConfiguration("Input", ConnectionType::Input));
    basicNode->AddTestingSlot(ExecutionSlotConfiguration("Input-1", ConnectionType::Input));
    basicNode->AddTestingSlot(ExecutionSlotConfiguration("Input-2", ConnectionType::Input));
    basicNode->AddTestingSlot(ExecutionSlotConfiguration("Input-3", ConnectionType::Input));

    SlotId firstSlotAdded;
    SlotId secondSlotAdded;
    SlotId thirdSlotAdded;

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "A";
        slotConfiguration.SetDefaultValue(0);
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        Slot* slot = basicNode->AddTestingSlot(slotConfiguration);

        firstSlotAdded = slot->GetId();
    }

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "C";
        slotConfiguration.SetDefaultValue(2);
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        Slot* slot = basicNode->AddTestingSlot(slotConfiguration);

        secondSlotAdded = slot->GetId();
    }

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "B";
        slotConfiguration.SetDefaultValue(1);
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        auto index = basicNode->FindSlotIndex(secondSlotAdded);

        Slot* slot = basicNode->InsertTestingSlot(index, slotConfiguration);

        thirdSlotAdded = slot->GetId();
    }

    {
        AZStd::vector< const Slot* > slotList = basicNode->FindSlotsByDescriptor(SlotDescriptors::DataIn());

        EXPECT_EQ(slotList.size(), 3);

        {
            const Slot* firstSlot = slotList[0];
            EXPECT_EQ(firstSlot->GetId(), firstSlotAdded);
            EXPECT_EQ(firstSlot->GetName(), "A");
            EXPECT_FLOAT_EQ(static_cast<float>((*firstSlot->FindDatum()->GetAs<Data::NumberType>())), 0.0f);
        }

        {
            const Slot* secondSlot = slotList[1];
            EXPECT_EQ(secondSlot->GetId(), thirdSlotAdded);
            EXPECT_EQ(secondSlot->GetName(), "B");
            EXPECT_FLOAT_EQ(static_cast<float>((*secondSlot->FindDatum()->GetAs<Data::NumberType>())), 1.0f);
        }

        {
            const Slot* thirdSlot = slotList[2];
            EXPECT_EQ(thirdSlot->GetId(), secondSlotAdded);
            EXPECT_EQ(thirdSlot->GetName(), "C");
            EXPECT_FLOAT_EQ(static_cast<float>((*thirdSlot->FindDatum()->GetAs<Data::NumberType>())), 2.0f);
        }
    }

    graph->Activate();

    {
        AZStd::vector< const Slot* > slotList = basicNode->FindSlotsByDescriptor(SlotDescriptors::DataIn());

        EXPECT_EQ(slotList.size(), 3);

        {
            const Slot* firstSlot = slotList[0];
            EXPECT_EQ(firstSlot->GetId(), firstSlotAdded);
            EXPECT_EQ(firstSlot->GetName(), "A");
            EXPECT_FLOAT_EQ(static_cast<float>((*firstSlot->FindDatum()->GetAs<Data::NumberType>())), 0.0f);
        }

        {
            const Slot* secondSlot = slotList[1];
            EXPECT_EQ(secondSlot->GetId(), thirdSlotAdded);
            EXPECT_EQ(secondSlot->GetName(), "B");
            EXPECT_FLOAT_EQ(static_cast<float>((*secondSlot->FindDatum()->GetAs<Data::NumberType>())), 1.0f);
        }

        {
            const Slot* thirdSlot = slotList[2];
            EXPECT_EQ(thirdSlot->GetId(), secondSlotAdded);
            EXPECT_EQ(thirdSlot->GetName(), "C");
            EXPECT_FLOAT_EQ(static_cast<float>((*thirdSlot->FindDatum()->GetAs<Data::NumberType>())), 2.0f);
        }
    }

    graph->Deactivate();

    {
        AZStd::vector< const Slot* > slotList = basicNode->FindSlotsByDescriptor(SlotDescriptors::DataIn());

        EXPECT_EQ(slotList.size(), 3);

        {
            const Slot* firstSlot = slotList[0];
            EXPECT_EQ(firstSlot->GetId(), firstSlotAdded);
            EXPECT_EQ(firstSlot->GetName(), "A");
            EXPECT_FLOAT_EQ(static_cast<float>((*firstSlot->FindDatum()->GetAs<Data::NumberType>())), 0.0f);
        }

        {
            const Slot* secondSlot = slotList[1];
            EXPECT_EQ(secondSlot->GetId(), thirdSlotAdded);
            EXPECT_EQ(secondSlot->GetName(), "B");
            EXPECT_FLOAT_EQ(static_cast<float>((*secondSlot->FindDatum()->GetAs<Data::NumberType>())), 1.0f);
        }

        {
            const Slot* thirdSlot = slotList[2];
            EXPECT_EQ(thirdSlot->GetId(), secondSlotAdded);
            EXPECT_EQ(thirdSlot->GetName(), "C");
            EXPECT_FLOAT_EQ(static_cast<float>((*thirdSlot->FindDatum()->GetAs<Data::NumberType>())), 2.0f);
        }
    }
}

TEST_F(ScriptCanvasTestFixture, InsertSlot)
{
    RegisterComponentDescriptor<InsertSlotConcatNode>();

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("RemoveSlotGraph");
    graphEntity->Init();
    
    SystemRequestBus::Broadcast(&SystemRequests::CreateEngineComponentsOnEntity, graphEntity.get());
    Graph* graph = AZ::EntityUtils::FindFirstDerivedComponent<Graph>(graphEntity.get());    
    
    ScriptCanvasId graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId outId;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, outId);
    auto insertSlotConcatNode = CreateTestNode<InsertSlotConcatNode>(graphUniqueId, outId);
    auto setVariableNode = CreateTestNode<Nodes::Core::SetVariableNode>(graphUniqueId, outId);
    VariableId resultVarId = CreateVariable(graphUniqueId, Data::StringType(), "result");
    setVariableNode->SetId(resultVarId);

    EXPECT_TRUE(setVariableNode->GetDataInSlotId().IsValid());

    //logic connections
    EXPECT_TRUE(Connect(*graph, startNode->GetEntityId(), "Out", insertSlotConcatNode->GetEntityId(), "In"));
    EXPECT_TRUE(Connect(*graph, insertSlotConcatNode->GetEntityId(), "Out", setVariableNode->GetEntityId(), "In"));

    // insert slots
    auto concatSlotId = insertSlotConcatNode->InsertSlot(-1, "A");
    auto slotAId = concatSlotId;
    SetInput_UNIT_TEST(insertSlotConcatNode, concatSlotId, Data::StringType("Hello"));
    concatSlotId = insertSlotConcatNode->InsertSlot(-1, "B");
    auto slotBId = concatSlotId;
    SetInput_UNIT_TEST(insertSlotConcatNode, concatSlotId, Data::StringType(" World"));
    
    // data connections
    EXPECT_TRUE(graph->Connect(insertSlotConcatNode->GetEntityId(), insertSlotConcatNode->GetSlotId("Result"), setVariableNode->GetEntityId(), setVariableNode->GetDataInSlotId()));

    graphEntity->Activate();
    ReportErrors(graph);
    graphEntity->Deactivate();

    const GraphVariable* resultDatum{};
    VariableRequestBus::EventResult(resultDatum, GraphScopedVariableId(graphUniqueId, resultVarId), &VariableRequests::GetVariableConst);
    ASSERT_NE(nullptr, resultDatum);
    EXPECT_EQ("Hello World", resultDatum->GetDatum()->ToString());

    // insert additional slot between A and B
    auto slotIndex = insertSlotConcatNode->FindSlotIndex(slotBId);
    EXPECT_GE(slotIndex, 0);
    concatSlotId = insertSlotConcatNode->InsertSlot(slotIndex, "Alpha");
    SetInput_UNIT_TEST(insertSlotConcatNode, concatSlotId, Data::StringType(" Ice Cream"));

    // re-execute the graph
    graphEntity->Activate();
    ReportErrors(graph);
    graphEntity->Deactivate();

    // the new concatenated string should be in the middle
    EXPECT_EQ("Hello Ice Cream World", resultDatum->GetDatum()->ToString());

    graphEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, NativeProperties)
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
    auto vector3NodeA = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdA);
    auto vector3NodeB = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdB);
    auto vector3NodeC = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdC);

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
        EXPECT_EQ(AZ::Vector3(5, 7, 9), *result);
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

TEST_F(ScriptCanvasTestFixture, ExtractPropertiesNativeType)
{
    // TODO: Add Extract test for all Native Types
    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    Graph* graph{};
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::CreateGraphOnEntity, graphEntity.get());
    ASSERT_NE(nullptr, graph);

    AZ::EntityId propertyEntityId = graphEntity->GetId();
    ScriptCanvasId graphUniqueId = graph->GetScriptCanvasId();

    graphEntity->Init();

    // Create Extract Property Node
    AZ::EntityId outID;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, outID);
    auto extractPropertyNode = CreateTestNode<Nodes::Core::ExtractProperty>(graphUniqueId, outID);
    auto printNode = CreateTestNode<TestResult>(graphUniqueId, outID);

    // Test Native Vector3 Properties
    auto vector3Node = CreateDataNode(graphUniqueId, Data::Vector3Type(10.0f, 23.3f, 77.45f), outID);

    auto numberResultNode1 = CreateDataNode(graphUniqueId, 0.0f, outID);
    auto numberResultNode2 = CreateDataNode(graphUniqueId, 0.0f, outID);
    auto numberResultNode3 = CreateDataNode(graphUniqueId, 0.0f, outID);

    // data
    EXPECT_EQ(0, extractPropertyNode->GetPropertyFields().size());
    EXPECT_FALSE(extractPropertyNode->GetSourceSlotDataType().IsValid());

    EXPECT_TRUE(Connect(*graph, vector3Node->GetEntityId(), "Get", extractPropertyNode->GetEntityId(), "Source"));

    auto propertyFields = extractPropertyNode->GetPropertyFields();
    EXPECT_EQ(3, propertyFields.size());
    EXPECT_TRUE(extractPropertyNode->GetSourceSlotDataType().IS_A(Data::Type::Vector3()));

    // Vector3::x
    auto propIt = AZStd::find_if(propertyFields.begin(), propertyFields.end(), [](const AZStd::pair<AZStd::string_view, SlotId> propertyField)
    {
        return propertyField.first == "x";
    });

    ASSERT_NE(propIt, propertyFields.end());
    EXPECT_TRUE(graph->Connect(extractPropertyNode->GetEntityId(), propIt->second, numberResultNode1->GetEntityId(), numberResultNode1->GetSlotId("Set")));

    // Vector3::y
    propIt = AZStd::find_if(propertyFields.begin(), propertyFields.end(), [](const AZStd::pair<AZStd::string_view, SlotId> propertyField)
    {
        return propertyField.first == "y";
    });

    ASSERT_NE(propIt, propertyFields.end());
    EXPECT_TRUE(graph->Connect(extractPropertyNode->GetEntityId(), propIt->second, numberResultNode2->GetEntityId(), numberResultNode2->GetSlotId("Set")));

    // Vector3::z
    propIt = AZStd::find_if(propertyFields.begin(), propertyFields.end(), [](const AZStd::pair<AZStd::string_view, SlotId> propertyField)
    {
        return propertyField.first == "z";
    });

    ASSERT_NE(propIt, propertyFields.end());
    EXPECT_TRUE(graph->Connect(extractPropertyNode->GetEntityId(), propIt->second, numberResultNode3->GetEntityId(), numberResultNode3->GetSlotId("Set")));

    // Print the z value to console
    EXPECT_TRUE(graph->Connect(numberResultNode3->GetEntityId(), numberResultNode3->GetSlotId("Get"), printNode->GetEntityId(), printNode->GetSlotId("Value")));

    // logic
    EXPECT_TRUE(Connect(*graph, startNode->GetEntityId(), "Out", extractPropertyNode->GetEntityId(), "In"));
    EXPECT_TRUE(Connect(*graph, extractPropertyNode->GetEntityId(), "Out", printNode->GetEntityId(), "In"));

    // execute
    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();
    }
    ReportErrors(graph);
    graphEntity->Deactivate();

    AZ::Entity* connectionEntity{};
    EXPECT_TRUE(graph->FindConnection(connectionEntity, { vector3Node->GetEntityId(), vector3Node->GetSlotId("Get") }, { extractPropertyNode->GetEntityId(), extractPropertyNode->GetSlotId("Source") }));
    EXPECT_TRUE(graph->DisconnectById(connectionEntity->GetId()));

    const float tolerance = 0.001f;
    // x result
    auto vectorElementResult = GetInput_UNIT_TEST<Data::NumberType>(numberResultNode1, "Set");
    ASSERT_NE(nullptr, vectorElementResult);
    EXPECT_NEAR(10.0f, *vectorElementResult, tolerance);

    // y result
    vectorElementResult = GetInput_UNIT_TEST<Data::NumberType>(numberResultNode2, "Set");
    ASSERT_NE(nullptr, vectorElementResult);
    EXPECT_NEAR(23.3f, *vectorElementResult, tolerance);

    // z result
    vectorElementResult = GetInput_UNIT_TEST<Data::NumberType>(numberResultNode3, "Set");
    ASSERT_NE(nullptr, vectorElementResult);
    EXPECT_NEAR(77.45f, *vectorElementResult, tolerance);

    graphEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, ExtractPropertiesBehaviorType)
{
    TestBehaviorContextProperties::Reflect(m_serializeContext);
    TestBehaviorContextProperties::Reflect(m_behaviorContext);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    Graph* graph{};
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::CreateGraphOnEntity, graphEntity.get());
    ASSERT_NE(nullptr, graph);

    AZ::EntityId propertyEntityId = graphEntity->GetId();
    ScriptCanvasId graphUniqueId = graph->GetScriptCanvasId();

    graphEntity->Init();

    // Create Extract Property Node
    AZ::EntityId outID;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, outID);
    auto extractPropertyNode = CreateTestNode<Nodes::Core::ExtractProperty>(graphUniqueId, outID);
    auto printNode = CreateTestNode<TestResult>(graphUniqueId, outID);

    // Test BehaviorContext Properties

    TestBehaviorContextProperties behaviorContextProperties;
    behaviorContextProperties.m_booleanProp = true;
    behaviorContextProperties.m_numberProp = 11.90f;
    behaviorContextProperties.m_stringProp = "String Property";
    auto behaviorContextPropertyNode = CreateDataNode(graphUniqueId, behaviorContextProperties, outID);

    auto booleanResultNode = CreateDataNode(graphUniqueId, false, outID);
    auto numberResultNode = CreateDataNode(graphUniqueId, 0.0f, outID);
    auto stringResultNode = CreateDataNode(graphUniqueId, Data::StringType("Uninitialized"), outID);

    // data
    EXPECT_EQ(0, extractPropertyNode->GetPropertyFields().size());
    EXPECT_FALSE(extractPropertyNode->GetSourceSlotDataType().IsValid());

    EXPECT_TRUE(Connect(*graph, behaviorContextPropertyNode->GetEntityId(), "Get", extractPropertyNode->GetEntityId(), "Source"));

    auto propertyFields = extractPropertyNode->GetPropertyFields();
    EXPECT_EQ(4, propertyFields.size());
    EXPECT_TRUE(extractPropertyNode->GetSourceSlotDataType().IS_A(Data::FromAZType<TestBehaviorContextProperties>()));

    // TestBehaviorContextProperties::boolean
    auto propIt = AZStd::find_if(propertyFields.begin(), propertyFields.end(), [](const AZStd::pair<AZStd::string_view, SlotId> propertyField)
    {
        return propertyField.first == "boolean";
    });

    ASSERT_NE(propIt, propertyFields.end());
    EXPECT_TRUE(graph->Connect(extractPropertyNode->GetEntityId(), propIt->second, booleanResultNode->GetEntityId(), booleanResultNode->GetSlotId("Set")));

    // TestBehaviorContextProperties::number
    propIt = AZStd::find_if(propertyFields.begin(), propertyFields.end(), [](const AZStd::pair<AZStd::string_view, SlotId> propertyField)
    {
        return propertyField.first == "number";
    });

    ASSERT_NE(propIt, propertyFields.end());
    EXPECT_TRUE(graph->Connect(extractPropertyNode->GetEntityId(), propIt->second, numberResultNode->GetEntityId(), numberResultNode->GetSlotId("Set")));

    // TestBehaviorContextProperties::string
    propIt = AZStd::find_if(propertyFields.begin(), propertyFields.end(), [](const AZStd::pair<AZStd::string_view, SlotId> propertyField)
    {
        return propertyField.first == "string";
    });

    ASSERT_NE(propIt, propertyFields.end());
    EXPECT_TRUE(graph->Connect(extractPropertyNode->GetEntityId(), propIt->second, stringResultNode->GetEntityId(), stringResultNode->GetSlotId("Set")));

    // Print the string value to console
    EXPECT_TRUE(graph->Connect(stringResultNode->GetEntityId(), stringResultNode->GetSlotId("Get"), printNode->GetEntityId(), printNode->GetSlotId("Value")));

    // logic
    EXPECT_TRUE(Connect(*graph, startNode->GetEntityId(), "Out", extractPropertyNode->GetEntityId(), "In"));
    EXPECT_TRUE(Connect(*graph, extractPropertyNode->GetEntityId(), "Out", printNode->GetEntityId(), "In"));

    // execute
    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();
    }

    ReportErrors(graph);    
    graphEntity->Deactivate();

    AZ::Entity* connectionEntity{};
    EXPECT_TRUE(graph->FindConnection(connectionEntity, { behaviorContextPropertyNode->GetEntityId(), behaviorContextPropertyNode->GetSlotId("Get") }, { extractPropertyNode->GetEntityId(), extractPropertyNode->GetSlotId("Source") }));
    EXPECT_TRUE(graph->DisconnectById(connectionEntity->GetId()));

                                                                                                                                // boolean result
    auto booleanResult = GetInput_UNIT_TEST<Data::BooleanType>(booleanResultNode, "Set");
    ASSERT_NE(nullptr, booleanResult);
    EXPECT_TRUE(booleanResult);

    // number result
    const float tolerance = 0.001f;
    auto numberResult = GetInput_UNIT_TEST<Data::NumberType>(numberResultNode, "Set");
    ASSERT_NE(nullptr, numberResult);
    EXPECT_NEAR(11.90f, *numberResult, tolerance);

    // string result
    auto stringResult = GetInput_UNIT_TEST<Data::StringType>(stringResultNode, "Set");
    ASSERT_NE(nullptr, stringResult);
    EXPECT_EQ("String Property", *stringResult);

    graphEntity.reset();

    m_serializeContext->EnableRemoveReflection();
    TestBehaviorContextProperties::Reflect(m_serializeContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextProperties::Reflect(m_behaviorContext);
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, IsNullCheck)
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

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId isNullFalseID;
    CreateTestNode<Nodes::Logic::IsNull>(graphUniqueId, isNullFalseID);

    AZ::EntityId isNullTrueID;
    CreateTestNode<Nodes::Logic::IsNull>(graphUniqueId, isNullTrueID);

    AZ::EntityId isNullResultFalseID;
    auto isNullResultFalse = CreateDataNode(graphUniqueId, true, isNullResultFalseID);
    AZ::EntityId isNullResultTrueID;
    auto isNullResultTrue = CreateDataNode(graphUniqueId, false, isNullResultTrueID);

    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "TestBehaviorContextObject", "Normalize");

    AZ::EntityId vectorID;
    Nodes::Core::BehaviorContextObjectNode* vector = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, vectorID);
    const AZStd::string vector3("TestBehaviorContextObject");
    vector->InitializeObject(vector3);
    if (auto vector3_2 = ModInput_UNIT_TEST<TestBehaviorContextObject>(vector, "Set"))
    {
        EXPECT_FALSE(vector3_2->IsNormalized());
    }
    else
    {
        ADD_FAILURE();
    }

    // test the reference type contract
    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        EXPECT_FALSE(Connect(*graph, isNullResultFalseID, "Get", isNullTrueID, "Reference"));
    }

    // data
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", normalizeID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", isNullFalseID, "Reference"));
    EXPECT_TRUE(Connect(*graph, isNullTrueID, "Is Null", isNullResultTrueID, "Set"));
    EXPECT_TRUE(Connect(*graph, isNullFalseID, "Is Null", isNullResultFalseID, "Set"));
    // execution 
    EXPECT_TRUE(Connect(*graph, startID, "Out", isNullTrueID, "In"));
    EXPECT_TRUE(Connect(*graph, isNullTrueID, "True", isNullFalseID, "In"));
    EXPECT_TRUE(Connect(*graph, isNullFalseID, "False", normalizeID, "In"));
    
    AZ::Entity* graphEntity = graph->GetEntity();

    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();
    }

    ReportErrors(graph);

    if (auto vector3_2 = GetInput_UNIT_TEST<TestBehaviorContextObject>(vector, "Set"))
    {
        EXPECT_TRUE(vector3_2->IsNormalized());
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto shouldBeFalse = GetInput_UNIT_TEST<bool>(isNullResultFalse, "Set"))
    {
        EXPECT_FALSE(*shouldBeFalse);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto shouldBeTrue = GetInput_UNIT_TEST<bool>(isNullResultTrue, "Set"))
    {
        EXPECT_TRUE(*shouldBeTrue);
    }
    else
    {
        ADD_FAILURE();
    }

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

TEST_F(ScriptCanvasTestFixture, NullThisPointerDoesNotCrash)
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

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "TestBehaviorContextObject", "Normalize");

    EXPECT_TRUE(Connect(*graph, startID, "Out", normalizeID, "In"));

    AZ::Entity* graphEntity = graph->GetEntity();
    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();
    }
    // just don't crash, but be in error
    const bool expectError = true;
    const bool expectIrrecoverableError = true;
    ReportErrors(graph, expectError, expectIrrecoverableError);

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

TEST_F(ScriptCanvasTestFixture, ValueTypes)
{    
    using namespace ScriptCanvas;

    Datum number0(Datum(0));
    int number0Value = *number0.GetAs<int>();

    Datum number1(Datum(1));
    int number1Value = *number1.GetAs<int>();

    Datum numberFloat(Datum(2.f));
    float numberFloatValue = *numberFloat.GetAs<float>();

    Datum numberDouble(Datum(3.0));
    double numberDoubleValue = *numberDouble.GetAs<double>();

    Datum numberHex(Datum(0xff));
    int numberHexValue = *numberHex.GetAs<int>();

    Datum numberPi(Datum(3.14f));
    float numberPiValue = *numberPi.GetAs<float>();

    Datum numberSigned(Datum(-100));
    int numberSignedValue = *numberSigned.GetAs<int>();

    Datum numberUnsigned(Datum(100u));
    unsigned int numberUnsignedValue = *numberUnsigned.GetAs<unsigned int>();

    Datum numberDoublePi(Datum(6.28));
    double numberDoublePiValue = *numberDoublePi.GetAs<double>();

    EXPECT_EQ(number0Value, 0);
    EXPECT_EQ(number1Value, 1);

    EXPECT_TRUE(AZ::IsClose(numberFloatValue, 2.f, std::numeric_limits<float>::epsilon()));
    EXPECT_TRUE(AZ::IsClose(numberDoubleValue, 3.0, std::numeric_limits<double>::epsilon()));

    EXPECT_NE(number0Value, number1Value);
    SC_EXPECT_FLOAT_EQ(numberPiValue, 3.14f);

    EXPECT_NE(number0Value, numberPiValue);
    EXPECT_NE(numberPiValue, numberDoublePiValue);

    Datum boolTrue(Datum(true));
    EXPECT_TRUE(*boolTrue.GetAs<bool>());
    Datum boolFalse(Datum(false));
    EXPECT_FALSE(*boolFalse.GetAs<bool>());
    boolFalse = boolTrue;
    EXPECT_TRUE(*boolFalse.GetAs<bool>());
    Datum boolFalse2(Datum(false));
    boolTrue = boolFalse2;
    EXPECT_FALSE(*boolTrue.GetAs<bool>());

    {
        const int k_count(10000);
        AZStd::vector<Datum> objects;
        for (int i = 0; i < k_count; ++i)
        {
            objects.push_back(Datum("Vector3", Datum::eOriginality::Original));
        }
    }

    GTEST_SUCCEED();
}


namespace
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    bool GraphHasVectorWithValue(const Graph& graph, const AZ::Vector3& vector)
    {
        for (auto nodeId : graph.GetNodes())
        {
            AZ::Entity* entity{};
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);

            if (entity)
            {
                if (auto node = AZ::EntityUtils::FindFirstDerivedComponent<Core::BehaviorContextObjectNode>(entity))
                {
                    if (auto candidate = NodeAccessor::GetInput_UNIT_TEST<AZ::Vector3>(node, "Set"))
                    {
                        if (*candidate == vector)
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }
}

TEST_F(ScriptCanvasTestFixture, SerializationSaveTest)
{

    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
    //////////////////////////////////////////////////////////////////////////

    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();
    AZ::EntityId startNode{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNode, graphUniqueId, Nodes::Core::Start::TYPEINFO_Uuid());

    // Constant 0
    AZ::Entity* constant0Entity{ aznew AZ::Entity };
    constant0Entity->Init();
    AZ::EntityId constant0Node{ constant0Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, constant0Node, graphUniqueId, Nodes::Math::Number::TYPEINFO_Uuid());

    // Get the node and set the value
    Nodes::Math::Number* constant0NodePtr = nullptr;
    SystemRequestBus::BroadcastResult(constant0NodePtr, &SystemRequests::GetNode<Nodes::Math::Number>, constant0Node);
    EXPECT_TRUE(constant0NodePtr != nullptr);
    if (constant0NodePtr)
    {
        SetInput_UNIT_TEST(constant0NodePtr, "Set", 4);
    }

    // Constant 1
    AZ::Entity* constant1Entity{ aznew AZ::Entity };
    constant1Entity->Init();
    AZ::EntityId constant1Node{ constant1Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, constant1Node, graphUniqueId, Nodes::Math::Number::TYPEINFO_Uuid());
    // Get the node and set the value
    Nodes::Math::Number* constant1NodePtr = nullptr;
    SystemRequestBus::BroadcastResult(constant1NodePtr, &SystemRequests::GetNode<Nodes::Math::Number>, constant1Node);
    EXPECT_TRUE(constant1NodePtr != nullptr);
    if (constant1NodePtr)
    {
        SetInput_UNIT_TEST(constant1NodePtr, "Set", 12);
    }

    // Sum
    AZ::Entity* sumEntity{ aznew AZ::Entity };
    sumEntity->Init();
    AZ::EntityId sumNode{ sumEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, sumNode, graphUniqueId, Nodes::Math::Sum::TYPEINFO_Uuid());

    AZ::EntityId printID;
    auto printNode = CreateTestNode<TestResult>(graphUniqueId, printID);

    // Connect, disconnect and reconnect to validate disconnection
    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId(BinaryOperator::k_lhsName), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    graph->Disconnect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId(BinaryOperator::k_lhsName), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId(BinaryOperator::k_lhsName), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    {
        auto* sumNodePtr = AZ::EntityUtils::FindFirstDerivedComponent<Nodes::Math::Sum>(sumEntity);
        EXPECT_NE(nullptr, sumNodePtr);
        EXPECT_EQ(1, graph->GetConnectedEndpoints({ sumNode, sumNodePtr->GetSlotId(BinaryOperator::k_lhsName) }).size());
    }

    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId(BinaryOperator::k_rhsName), constant1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant1Entity)->GetSlotId("Get"));

    EXPECT_TRUE(Connect(*graph, sumNode, "Result", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, startNode, "Out", sumNode, Nodes::BinaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, sumNode, Nodes::BinaryOperator::k_outName, printID, "In"));

    AZ::EntityId vector3IdA, vector3IdB, vector3IdC;
    const AZ::Vector3 allOne(1, 1, 1);
    const AZ::Vector3 allTwo(2, 2, 2);
    const AZ::Vector3 allThree(3, 3, 3);
    const AZ::Vector3 allFour(4, 4, 4);

    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3NodeA, "Set") = allOne;
    Core::BehaviorContextObjectNode* vector3NodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdB);
    vector3NodeB->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3NodeB, "Set") = allTwo;
    Core::BehaviorContextObjectNode* vector3NodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdC);
    vector3NodeC->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3NodeC, "Set") = allFour;

    AZ::EntityId add3IdA;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, add3IdA);

    EXPECT_TRUE(Connect(*graph, printID, "Out", add3IdA, "In"));
    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", add3IdA, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", add3IdA, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "Set", add3IdA, "Result: Vector3"));

    AZStd::unique_ptr<AZ::Entity> graphEntity{ graph->GetEntity() };

    EXPECT_EQ(allOne, *GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeA, "Set"));
    EXPECT_EQ(allTwo, *GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeB, "Set"));
    auto vector3C = *GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeC, "Set");
    EXPECT_EQ(allFour, vector3C);

    auto nodeIDs = graph->GetNodes();
    EXPECT_EQ(9, nodeIDs.size());

    ScriptCanvasEditor::TraceSuppressionBus::Broadcast(&ScriptCanvasEditor::TraceSuppressionRequests::SuppressPrintf, true);
    graphEntity->Activate();
    ScriptCanvasEditor::TraceSuppressionBus::Broadcast(&ScriptCanvasEditor::TraceSuppressionRequests::SuppressPrintf, false);
    ReportErrors(graph);    

    vector3C = *GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeC, "Set");
    EXPECT_EQ(allThree, vector3C);

    if (auto result = GetInput_UNIT_TEST<float>(printNode, "Value"))
    {
        SC_EXPECT_FLOAT_EQ(*result, 16.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();

    auto* fileIO = AZ::IO::LocalFileIO::GetInstance();
    EXPECT_TRUE(fileIO != nullptr);

    auto pathResult = fileIO->CreatePath(k_tempCoreAssetDir);;
    EXPECT_EQ(pathResult.GetResultCode(), AZ::IO::ResultCode::Success) << "Path failed to create";

    // Save it
    bool result = false;
    AZ::IO::FileIOStream outFileStream(k_tempCoreAssetPath, AZ::IO::OpenMode::ModeWrite);
    if (outFileStream.IsOpen())
    {
        RuntimeAssetHandler runtimeAssetHandler;
        AZ::Data::Asset<RuntimeAsset> runtimeAsset = CreateRuntimeAsset(graph);
        EXPECT_TRUE(runtimeAssetHandler.SaveAssetData(runtimeAsset, &outFileStream)) << "Asset failed to save";

        EXPECT_TRUE(fileIO->Exists(k_tempCoreAssetPath));
    }
    else
    {
        ADD_FAILURE() << "File stream not open";
    }

    outFileStream.Close();
}

TEST_F(ScriptCanvasTestFixture, SerializationLoadTest_Graph)
{
    using namespace ScriptCanvas;
    //////////////////////////////////////////////////////////////////////////

    // Make the graph.
    Graph* graph = nullptr;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("Loaded Graph");
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::CreateGraphOnEntity, graphEntity.get());

    auto* fileIO = AZ::IO::LocalFileIO::GetInstance();
    ASSERT_NE(nullptr, fileIO);
    EXPECT_TRUE(fileIO->Exists(k_tempCoreAssetPath));

    // Load it
    AZ::Data::Asset<RuntimeAsset> runtimeAsset;
    runtimeAsset.Create(AZ::Uuid::CreateRandom());
    AZ::u64 fileLength = 0;
    AZ::IO::FileIOBase::GetInstance()->Size(k_tempCoreAssetPath, fileLength);
    AZStd::shared_ptr<AZ::Data::AssetDataStream> stream = AZStd::make_shared<AZ::Data::AssetDataStream>();
    stream->Open(k_tempCoreAssetPath, 0, fileLength);
    stream->BlockUntilLoadComplete();

    // Serialize the graph entity back in.
    RuntimeAssetHandler runtimeAssetHandler;
    EXPECT_EQ(runtimeAssetHandler.LoadAssetData(runtimeAsset, stream, {}), AZ::Data::AssetHandler::LoadResult::LoadComplete);

    GraphData& assetGraphData = runtimeAsset.Get()->GetData().m_graphData;
    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> idRemap;
    AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&assetGraphData, idRemap, m_serializeContext);
    graph->AddGraphData(assetGraphData);
    // Clear GraphData on asset to prevent it from deleting the graph data, the Graph component takes care of cleaning it up
    assetGraphData.Clear();

    // Initialize the entity
    graphEntity->Init();
    ScriptCanvasEditor::TraceSuppressionBus::Broadcast(&ScriptCanvasEditor::TraceSuppressionRequests::SuppressPrintf, true);
    graphEntity->Activate();
    ScriptCanvasEditor::TraceSuppressionBus::Broadcast(&ScriptCanvasEditor::TraceSuppressionRequests::SuppressPrintf, false);

    // Run the graph component
    graph = graphEntity->FindComponent<Graph>();
    EXPECT_TRUE(graph != nullptr); // "Graph entity did not have the graph component.");
    ReportErrors(graph);

    const AZ::Vector3 allOne(1, 1, 1);
    const AZ::Vector3 allTwo(2, 2, 2);
    const AZ::Vector3 allThree(3, 3, 3);
    const AZ::Vector3 allFour(4, 4, 4);

    auto nodeIDs = graph->GetNodes();
    EXPECT_EQ(9, nodeIDs.size());

    // Graph result should be still 16
    // Serialized graph has new remapped entity ids
    auto nodes = graph->GetNodes();
    auto sumNodeIt = AZStd::find_if(nodes.begin(), nodes.end(), [](const AZ::EntityId& nodeId) -> bool
    {
        AZ::Entity* entity{};
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
        return entity ? AZ::EntityUtils::FindFirstDerivedComponent<TestResult>(entity) ? true : false : false;
    });
    EXPECT_NE(nodes.end(), sumNodeIt);

    TestResult* printNode = nullptr;
    SystemRequestBus::BroadcastResult(printNode, &SystemRequests::GetNode<TestResult>, *sumNodeIt);
    EXPECT_TRUE(printNode != nullptr);

    if (auto result = GetInput_UNIT_TEST<float>(printNode, "Value"))
    {
        SC_EXPECT_FLOAT_EQ(*result, 16.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allThree));
    EXPECT_FALSE(GraphHasVectorWithValue(*graph, allFour));
}

TEST_F(ScriptCanvasTestFixture, SerializationLoadTest_RuntimeComponent)
{
    
    using namespace ScriptCanvas;
    //////////////////////////////////////////////////////////////////////////

    // Make the graph.
    AZStd::unique_ptr<AZ::Entity> executionEntity = AZStd::make_unique<AZ::Entity>("Loaded Graph");
    auto executionComponent = executionEntity->CreateComponent<ScriptCanvas::RuntimeComponent>();

    auto* fileIO = AZ::IO::LocalFileIO::GetInstance();
    EXPECT_NE(nullptr, fileIO);
    EXPECT_TRUE(fileIO->Exists(k_tempCoreAssetPath));

    // Load it
    AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
    runtimeAsset.Create(AZ::Uuid::CreateRandom());
    AZ::u64 fileLength = 0;
    AZ::IO::FileIOBase::GetInstance()->Size(k_tempCoreAssetPath, fileLength);
    AZStd::shared_ptr<AZ::Data::AssetDataStream> stream = AZStd::make_shared<AZ::Data::AssetDataStream>();
    stream->Open(k_tempCoreAssetPath, 0, fileLength);
    stream->BlockUntilLoadComplete();

    // Serialize the runtime asset back in.
    RuntimeAssetHandler runtimeAssetHandler;
    EXPECT_EQ(runtimeAssetHandler.LoadAssetData(runtimeAsset, stream, {}), AZ::Data::AssetHandler::LoadResult::LoadComplete);
    runtimeAssetHandler.InitAsset(runtimeAsset, true, false);

    // Initialize the entity
    executionComponent->SetRuntimeAsset(runtimeAsset);
    executionEntity->Init();

    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;            
        executionEntity->Activate();
        // Dispatch the AssetBus events to force onAssetReady event
        AZ::Data::AssetManager::Instance().DispatchEvents();            
    }

    EXPECT_FALSE(executionComponent->IsInErrorState());

    const AZ::Vector3 allOne(1, 1, 1);
    const AZ::Vector3 allTwo(2, 2, 2);
    const AZ::Vector3 allThree(3, 3, 3);
    const AZ::Vector3 allFour(4, 4, 4);

    auto nodeIds = executionComponent->GetNodes();
    EXPECT_EQ(9, nodeIds.size());

    // Graph result should be still 16
    // Serialized graph has new remapped entity ids

    auto sumNodeIt = AZStd::find_if(nodeIds.begin(), nodeIds.end(), [](const AZ::EntityId& nodeId) -> bool
    {
        AZ::Entity* entity{};
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
        return entity ? AZ::EntityUtils::FindFirstDerivedComponent<TestResult>(entity) ? true : false : false;
    });
    EXPECT_NE(nodeIds.end(), sumNodeIt);

    TestResult* printNode = nullptr;
    SystemRequestBus::BroadcastResult(printNode, &SystemRequests::GetNode<TestResult>, *sumNodeIt);
    EXPECT_TRUE(printNode != nullptr);

    if (auto result = GetInput_UNIT_TEST<float>(printNode, "Value"))
    {
        SC_EXPECT_FLOAT_EQ(*result, 16.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    AZStd::vector<AZ::Vector3> vector3Results;
    for (auto nodeId : nodeIds)
    {
        AZ::Entity* entity{};
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
        auto behaviorContextNode = entity ? AZ::EntityUtils::FindFirstDerivedComponent<Core::BehaviorContextObjectNode>(entity) : nullptr;
            auto rawVector3 = behaviorContextNode ? GetInput_UNIT_TEST<AZ::Vector3>(behaviorContextNode, PureData::k_setThis) : nullptr;
        if (rawVector3)
        {
            vector3Results.push_back(*rawVector3);
        }
    }

    auto allOneFound = AZStd::any_of(vector3Results.begin(), vector3Results.end(), [allOne](const AZ::Vector3& vector3Result)
    {
        return allOne == vector3Result;
    });
    auto allTwoFound = AZStd::any_of(vector3Results.begin(), vector3Results.end(), [allTwo](const AZ::Vector3& vector3Result)
    {
        return allTwo == vector3Result;
    });
    auto allThreeFound = AZStd::any_of(vector3Results.begin(), vector3Results.end(), [allThree](const AZ::Vector3& vector3Result)
    {
        return allThree == vector3Result;
    });
    auto allFourFound = AZStd::any_of(vector3Results.begin(), vector3Results.end(), [allFour](const AZ::Vector3& vector3Result)
    {
        return allFour == vector3Result;
    });

    EXPECT_TRUE(allOneFound);
    EXPECT_TRUE(allTwoFound);
    EXPECT_TRUE(allThreeFound);
    EXPECT_FALSE(allFourFound);
}

TEST_F(ScriptCanvasTestFixture, Contracts)
{    
    using namespace ScriptCanvas;
    RegisterComponentDescriptor<ContractNode>();

    //////////////////////////////////////////////////////////////////////////
    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity("Start") };
    startEntity->Init();
    AZ::EntityId startNode{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNode, graphUniqueId, Nodes::Core::Start::TYPEINFO_Uuid());

    // ContractNode 0
    AZ::Entity* contract0Entity{ aznew AZ::Entity("Contract 0") };
    contract0Entity->Init();
    AZ::EntityId contract0Node{ contract0Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, contract0Node, graphUniqueId, ContractNode::TYPEINFO_Uuid());

    // ContractNode 1
    AZ::Entity* contract1Entity{ aznew AZ::Entity("Contract 1") };
    contract1Entity->Init();
    AZ::EntityId contract1Node{ contract1Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, contract1Node, graphUniqueId, ContractNode::TYPEINFO_Uuid());

    // invalid
    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Out")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Get String")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Get String")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String")));
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set String")));
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set Number")));
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get Number")));
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set String")));

        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));
    }

    // valid
    EXPECT_TRUE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get String")));
    EXPECT_TRUE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Out")));
    EXPECT_TRUE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set Number"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get Number")));

    // Execute it
    graph->GetEntity()->Activate();
    ReportErrors(graph);
    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

#if defined (SCRIPTCANVAS_ERRORS_ENABLED)

TEST_F(ScriptCanvasTestFixture, Error)
{
    using namespace ScriptCanvas;

    RegisterComponentDescriptor<UnitTestErrorNode>();
    RegisterComponentDescriptor<InfiniteLoopNode>();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZStd::string description = "Unit test error!";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZ::EntityId errorNodeID;
    CreateTestNode<UnitTestErrorNode>(graphUniqueId, errorNodeID);

    AZ::EntityId sideEffectID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, startID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, errorNodeID, "Out", sideEffectID, "In"));

    // test to make sure the graph received an error
    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    const bool expectErrors = true;
    const bool expectIrrecoverableErrors = true;
    ReportErrors(graph, expectErrors, expectIrrecoverableErrors);    

    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 0);

    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, ErrorNode)
{
    
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    // Start
    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZStd::string description = "Test Error Report";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZ::EntityId errorNodeID;
    CreateTestNode<Nodes::Core::Error>(graphUniqueId, errorNodeID);

    AZ::EntityId sideEffectID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, startID, "Out", sideEffectID, "In"));
    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, sideEffectID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, stringNodeID, "Get", errorNodeID, "Description"));

    // test to make sure the graph received an error
    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    const bool expectErrors = true;
    const bool expectIrrecoverableErrors = true;
    ReportErrors(graph, expectErrors, expectIrrecoverableErrors);

    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 1);

    graph->GetEntity()->Deactivate();

    // test to make sure the graph did to retry execution
    graph->GetEntity()->Activate();
    ReportErrors(graph, expectErrors, expectIrrecoverableErrors);

    // if the graph was allowed to execute, the side effect counter should be higher
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 1);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, InfiniteLoopDetected)
{
    using namespace ScriptCanvas;

    RegisterComponentDescriptor<InfiniteLoopNode>();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZStd::string sideFXpass = "Side FX Infinite Loop Error Handled";
    AZ::EntityId sideFXpassNodeID;
    CreateDataNode(graphUniqueId, sideFXpass, sideFXpassNodeID);

    AZStd::string sideFXfail = "Side FX Infinite Loop Node Failed";
    AZ::EntityId sideFXfailNodeID;
    CreateDataNode(graphUniqueId, sideFXfail, sideFXfailNodeID);

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId infiniteLoopNodeID;
    CreateTestNode<InfiniteLoopNode>(graphUniqueId, infiniteLoopNodeID);
    AZ::EntityId errorHandlerID;
    CreateTestNode<Nodes::Core::ErrorHandler>(graphUniqueId, errorHandlerID);

    AZ::EntityId sideEffectPassID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");
    AZ::EntityId sideEffectFailID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, sideFXpassNodeID, "Get", sideEffectPassID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, sideFXfailNodeID, "Get", sideEffectFailID, "String: 0"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", infiniteLoopNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, infiniteLoopNodeID, "In", infiniteLoopNodeID, "Before Infinity"));

    EXPECT_TRUE(Connect(*graph, infiniteLoopNodeID, "After Infinity", sideEffectFailID, "In"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Out", sideEffectPassID, "In"));

    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    const bool expectError = true;
    const bool expectIrrecoverableError = true;
    ReportErrors(graph, expectError, expectIrrecoverableError);    
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXpass));
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXfail));

    graph->GetEntity()->Deactivate();
    graph->GetEntity()->Activate();

    ReportErrors(graph, expectError, expectIrrecoverableError);
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXpass));
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXfail));

    delete graph->GetEntity();
}

#endif // SCRIPTCANAS_ERRORS_ENABLED


TEST_F(ScriptCanvasTestFixture, BinaryOperationTest)
{
    
    using namespace ScriptCanvas::Nodes;
    using namespace ScriptCanvas::Nodes::Comparison;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    BinaryOpTest<EqualTo>(1, 1, true);
    BinaryOpTest<EqualTo>(1, 0, false);
    BinaryOpTest<EqualTo>(0, 1, false);
    BinaryOpTest<EqualTo>(0, 0, true);

    BinaryOpTest<NotEqualTo>(1, 1, false);
    BinaryOpTest<NotEqualTo>(1, 0, true);
    BinaryOpTest<NotEqualTo>(0, 1, true);
    BinaryOpTest<NotEqualTo>(0, 0, false);

    BinaryOpTest<Greater>(1, 1, false);
    BinaryOpTest<Greater>(1, 0, true);
    BinaryOpTest<Greater>(0, 1, false);
    BinaryOpTest<Greater>(0, 0, false);

    BinaryOpTest<GreaterEqual>(1, 1, true);
    BinaryOpTest<GreaterEqual>(1, 0, true);
    BinaryOpTest<GreaterEqual>(0, 1, false);
    BinaryOpTest<GreaterEqual>(0, 0, true);

    BinaryOpTest<Less>(1, 1, false);
    BinaryOpTest<Less>(1, 0, false);
    BinaryOpTest<Less>(0, 1, true);
    BinaryOpTest<Less>(0, 0, false);

    BinaryOpTest<LessEqual>(1, 1, true);
    BinaryOpTest<LessEqual>(1, 0, false);
    BinaryOpTest<LessEqual>(0, 1, true);
    BinaryOpTest<LessEqual>(0, 0, true);

    BinaryOpTest<EqualTo>(true, true, true);
    BinaryOpTest<EqualTo>(true, false, false);
    BinaryOpTest<EqualTo>(false, true, false);
    BinaryOpTest<EqualTo>(false, false, true);

    BinaryOpTest<NotEqualTo>(true, true, false);
    BinaryOpTest<NotEqualTo>(true, false, true);
    BinaryOpTest<NotEqualTo>(false, true, true);
    BinaryOpTest<NotEqualTo>(false, false, false);

    AZStd::string string0("a string");
    AZStd::string string1("b string");

    BinaryOpTest<EqualTo>(string1, string1, true);
    BinaryOpTest<EqualTo>(string1, string0, false);
    BinaryOpTest<EqualTo>(string0, string1, false);
    BinaryOpTest<EqualTo>(string0, string0, true);

    BinaryOpTest<NotEqualTo>(string1, string1, false);
    BinaryOpTest<NotEqualTo>(string1, string0, true);
    BinaryOpTest<NotEqualTo>(string0, string1, true);
    BinaryOpTest<NotEqualTo>(string0, string0, false);

    BinaryOpTest<Greater>(string1, string1, false);
    BinaryOpTest<Greater>(string1, string0, true);
    BinaryOpTest<Greater>(string0, string1, false);
    BinaryOpTest<Greater>(string0, string0, false);

    BinaryOpTest<GreaterEqual>(string1, string1, true);
    BinaryOpTest<GreaterEqual>(string1, string0, true);
    BinaryOpTest<GreaterEqual>(string0, string1, false);
    BinaryOpTest<GreaterEqual>(string0, string0, true);

    BinaryOpTest<Less>(string1, string1, false);
    BinaryOpTest<Less>(string1, string0, false);
    BinaryOpTest<Less>(string0, string1, true);
    BinaryOpTest<Less>(string0, string0, false);

    BinaryOpTest<LessEqual>(string1, string1, true);
    BinaryOpTest<LessEqual>(string1, string0, false);
    BinaryOpTest<LessEqual>(string0, string1, true);
    BinaryOpTest<LessEqual>(string0, string0, true);

    const AZ::Vector3 vectorOne(1.0f, 1.0f, 1.0f);
    const AZ::Vector3 vectorZero(0.0f, 0.0f, 0.0f);

    BinaryOpTest<EqualTo>(vectorOne, vectorOne, true);
    BinaryOpTest<EqualTo>(vectorOne, vectorZero, false);
    BinaryOpTest<EqualTo>(vectorZero, vectorOne, false);
    BinaryOpTest<EqualTo>(vectorZero, vectorZero, true);

    BinaryOpTest<NotEqualTo>(vectorOne, vectorOne, false);
    BinaryOpTest<NotEqualTo>(vectorOne, vectorZero, true);
    BinaryOpTest<NotEqualTo>(vectorZero, vectorOne, true);
    BinaryOpTest<NotEqualTo>(vectorZero, vectorZero, false);

    const TestBehaviorContextObject zero(0);
    const TestBehaviorContextObject one(1);
    const TestBehaviorContextObject otherOne(1);

    BinaryOpTest<EqualTo>(one, one, true);
    BinaryOpTest<EqualTo>(one, zero, false);
    BinaryOpTest<EqualTo>(zero, one, false);
    BinaryOpTest<EqualTo>(zero, zero, true);

    BinaryOpTest<NotEqualTo>(one, one, false);
    BinaryOpTest<NotEqualTo>(one, zero, true);
    BinaryOpTest<NotEqualTo>(zero, one, true);
    BinaryOpTest<NotEqualTo>(zero, zero, false);

    BinaryOpTest<Greater>(one, one, false);
    BinaryOpTest<Greater>(one, zero, true);
    BinaryOpTest<Greater>(zero, one, false);
    BinaryOpTest<Greater>(zero, zero, false);

    BinaryOpTest<GreaterEqual>(one, one, true);
    BinaryOpTest<GreaterEqual>(one, zero, true);
    BinaryOpTest<GreaterEqual>(zero, one, false);
    BinaryOpTest<GreaterEqual>(zero, zero, true);

    BinaryOpTest<Less>(one, one, false);
    BinaryOpTest<Less>(one, zero, false);
    BinaryOpTest<Less>(zero, one, true);
    BinaryOpTest<Less>(zero, zero, false);

    BinaryOpTest<LessEqual>(one, one, true);
    BinaryOpTest<LessEqual>(one, zero, false);
    BinaryOpTest<LessEqual>(zero, one, true);
    BinaryOpTest<LessEqual>(zero, zero, true);

    BinaryOpTest<EqualTo>(one, otherOne, true);
    BinaryOpTest<EqualTo>(otherOne, one, true);

    BinaryOpTest<NotEqualTo>(one, otherOne, false);
    BinaryOpTest<NotEqualTo>(otherOne, one, false);

    BinaryOpTest<Greater>(one, otherOne, false);
    BinaryOpTest<Greater>(otherOne, one, false);

    BinaryOpTest<GreaterEqual>(one, otherOne, true);
    BinaryOpTest<GreaterEqual>(otherOne, one, true);

    BinaryOpTest<Less>(one, otherOne, false);
    BinaryOpTest<Less>(otherOne, one, false);

    BinaryOpTest<LessEqual>(one, otherOne, true);
    BinaryOpTest<LessEqual>(otherOne, one, true);

    // force failures, to verify crash proofiness
    BinaryOpTest<EqualTo>(one, zero, false, true);
    BinaryOpTest<NotEqualTo>(one, zero, true, true);
    BinaryOpTest<Greater>(one, zero, true, true);
    BinaryOpTest<GreaterEqual>(one, zero, true, true);
    BinaryOpTest<Less>(one, zero, false, true);
    BinaryOpTest<LessEqual>(one, zero, false, true);

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, EntityRefTest)
{
    

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    // Fake "world" entities
    AZ::Entity first("First");
    first.CreateComponent<ScriptCanvasTests::TestComponent>();
    first.Init();
    first.Activate();

    AZ::Entity second("Second");
    second.CreateComponent<ScriptCanvasTests::TestComponent>();
    second.Init();
    second.Activate();

    // Script Canvas Graph
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    m_entityContext.AddEntity(first.GetId());
    m_entityContext.AddEntity(second.GetId());
    m_entityContext.AddEntity(graph->GetEntityId());

    graph->GetEntity()->SetName("Graph Owner Entity");

    graph->GetEntity()->CreateComponent<ScriptCanvasTests::TestComponent>();
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    // EntityRef node to some specific entity #1
    AZ::EntityId selfEntityIdNode;
    auto selfEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, selfEntityIdNode);
    selfEntityRef->SetEntityRef(first.GetId());

    // EntityRef node to some specific entity #2
    AZ::EntityId otherEntityIdNode;
    auto otherEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, otherEntityIdNode);
    otherEntityRef->SetEntityRef(second.GetId());

    // Explicitly set an EntityRef node with this graph's entity Id.
    AZ::EntityId graphEntityIdNode;
    auto graphEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, graphEntityIdNode);
    graphEntityRef->SetEntityRef(graph->GetEntity()->GetId());

    // First test: directly set an entity Id on the EntityID: 0 slot
    AZ::EntityId eventAid;
    Nodes::Core::Method* nodeA = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventAid);
    nodeA->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");
    if (auto entityId = ModInput_UNIT_TEST<AZ::EntityId>(nodeA, "EntityID: 0"))
    {
        *entityId = first.GetId();
    }

    // Second test: Will connect the slot to an EntityRef node
    AZ::EntityId eventBid;
    Nodes::Core::Method* nodeB = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventBid);
    nodeB->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");

    // Third test: Set the slot's EntityId: 0 to GraphOwnerId, this should result in the same Id as graph->GetEntityId()
    AZ::EntityId eventCid;
    Nodes::Core::Method* nodeC = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventCid);
    nodeC->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");
    if (auto entityId = ModInput_UNIT_TEST<AZ::EntityId>(nodeC, "EntityID: 0"))
    {
        *entityId = ScriptCanvas::GraphOwnerId;
    }

    // True 
    AZ::EntityId trueNodeId;
    CreateDataNode<Data::BooleanType>(graphUniqueId, true, trueNodeId);

    // False
    AZ::EntityId falseNodeId;
    CreateDataNode<Data::BooleanType>(graphUniqueId, false, falseNodeId);

    // Start            -> TestEvent
    //                   O EntityId: 0    (not connected, it was set directly on the slot)
    // EntityRef: first -O EntityId: 1
    // False            -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventAid, "In"));
    EXPECT_TRUE(Connect(*graph, eventAid, "EntityID: 1", selfEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventAid, "Boolean: 2", falseNodeId, "Get"));

    // Start            -> TestEvent
    // EntityRef: second -O EntityId: 0 
    // EntityRef: second -O EntityId: 1
    // False             -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventBid, "In"));
    EXPECT_TRUE(Connect(*graph, eventBid, "EntityID: 0", otherEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventBid, "EntityID: 1", otherEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventBid, "Boolean: 2", falseNodeId, "Get"));

    // Start            -> TestEvent
    //                   -O EntityId: 0  (not connected, slot is set to GraphOwnerId)
    // graphEntityIdNode -O EntityId: 1
    // True              -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventCid, "In"));
    EXPECT_TRUE(Connect(*graph, eventCid, "EntityID: 1", graphEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventCid, "Boolean: 2", trueNodeId, "Get"));

    // Execute the graph

    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    ReportErrors(graph);
    delete graph->GetEntity();

}

const int k_executionCount = 998;

TEST_F(ScriptCanvasTestFixture, ExecutionLength)
{
    
    using namespace ScriptCanvas;

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId previousID = startID;

    for (int i = 0; i < k_executionCount; ++i)
    {
        AZ::EntityId printNodeID;
        TestResult* printNode = CreateTestNode<TestResult>(graphUniqueId, printNodeID);
        SetInput_UNIT_TEST<int>(printNode, "Value", i);
        EXPECT_TRUE(Connect(*graph, previousID, "Out", printNodeID, "In"));
        previousID = printNodeID;
    }

    AZ::Entity* graphEntity = graph->GetEntity();
    {
        ScriptCanvasEditor::ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();        
    }

    ReportErrors(graph);

    graphEntity->Deactivate();
    delete graphEntity;
}

TEST_F(ScriptCanvasTestFixture, AnyNode)
{
    RunUnitTestGraph("LY_SC_UnitTest_Any");
}
