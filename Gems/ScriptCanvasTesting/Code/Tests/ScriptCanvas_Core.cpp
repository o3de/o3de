/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>

#include <AzCore/Serialization/IdUtils.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

using namespace ScriptCanvasTests;
using namespace TestNodes;

TEST_F(ScriptCanvasTestFixture, CoreNodeFunction_OwningGraphCheck)
{
    using namespace ScriptCanvas;

    ScriptCanvas::Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* groupedNode = CreateConfigurableNode();
    
    EXPECT_EQ(graph, groupedNode->GetGraph());
}

TEST_F(ScriptCanvasTestFixture, AddRemoveSlotNotifications)
{
    RegisterComponentDescriptor<AddNodeWithRemoveSlot>();

    using namespace ScriptCanvas;

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
    numberAddNode->RemoveSlot(testSlotId, false); 
    EXPECT_EQ(2U, nodeNotifications.GetSlotsRemoved());

    numberAddEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, InsertSlot_Basic)
{
    using namespace ScriptCanvas;

    ScriptCanvas::Graph* graph = CreateGraph();
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
        const AZStd::vector< const Slot* > slotList = basicNode->GetAllSlots();

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

    ScriptCanvas::Graph* graph = CreateGraph();
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
    /*int numberHexValue =*/ *numberHex.GetAs<int>();

    Datum numberPi(Datum(3.14f));
    float numberPiValue = *numberPi.GetAs<float>();

    Datum numberSigned(Datum(-100));
    /*int numberSignedValue =*/ *numberSigned.GetAs<int>();

    Datum numberUnsigned(Datum(100u));
    /*unsigned int numberUnsignedValue =*/ *numberUnsigned.GetAs<unsigned int>();

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

TEST_F(ScriptCanvasTestFixture, Contracts)
{    
    using namespace ScriptCanvas;
    RegisterComponentDescriptor<ContractNode>();

    //////////////////////////////////////////////////////////////////////////
    // Make the graph.
    ScriptCanvas::Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity("Start") };
    startEntity->Init();
    AZ::EntityId startNode{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNode, graphUniqueId, ScriptCanvas::Nodes::Core::Start::TYPEINFO_Uuid());

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
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Get String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set Number")));
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

const int k_executionCount = 998;

TEST_F(ScriptCanvasTestFixture, While)
{
    RunUnitTestGraph("LY_SC_UnitTest_While", ScriptCanvas::ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, WhileBreak)
{
    RunUnitTestGraph("LY_SC_UnitTest_WhileBreak", ScriptCanvas::ExecutionMode::Interpreted);
}
