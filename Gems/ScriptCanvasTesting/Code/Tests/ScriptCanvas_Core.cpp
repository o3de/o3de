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


namespace
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    bool GraphHasVectorWithValue(const ScriptCanvas::Graph& graph, const AZ::Vector3& vector)
    {
        for (auto nodeId : graph.GetNodes())
        {
            AZ::Entity* entity{};
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);

            if (entity)
            {
                if (auto node = AZ::EntityUtils::FindFirstDerivedComponent<Core::BehaviorContextObjectNode>(entity))
                {
                    if (auto candidate = node->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
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
// 
// TEST_F(ScriptCanvasTestFixture, BinaryOperationTest)
// {
//     
//     using namespace ScriptCanvas::Nodes;
//     using namespace ScriptCanvas::Nodes::Comparison;
// 
//     TestBehaviorContextObject::Reflect(m_serializeContext);
//     TestBehaviorContextObject::Reflect(m_behaviorContext);
// 
//     BinaryOpTest<EqualTo>(1, 1, true);
//     BinaryOpTest<EqualTo>(1, 0, false);
//     BinaryOpTest<EqualTo>(0, 1, false);
//     BinaryOpTest<EqualTo>(0, 0, true);
// 
//     BinaryOpTest<NotEqualTo>(1, 1, false);
//     BinaryOpTest<NotEqualTo>(1, 0, true);
//     BinaryOpTest<NotEqualTo>(0, 1, true);
//     BinaryOpTest<NotEqualTo>(0, 0, false);
// 
//     BinaryOpTest<Greater>(1, 1, false);
//     BinaryOpTest<Greater>(1, 0, true);
//     BinaryOpTest<Greater>(0, 1, false);
//     BinaryOpTest<Greater>(0, 0, false);
// 
//     BinaryOpTest<GreaterEqual>(1, 1, true);
//     BinaryOpTest<GreaterEqual>(1, 0, true);
//     BinaryOpTest<GreaterEqual>(0, 1, false);
//     BinaryOpTest<GreaterEqual>(0, 0, true);
// 
//     BinaryOpTest<Less>(1, 1, false);
//     BinaryOpTest<Less>(1, 0, false);
//     BinaryOpTest<Less>(0, 1, true);
//     BinaryOpTest<Less>(0, 0, false);
// 
//     BinaryOpTest<LessEqual>(1, 1, true);
//     BinaryOpTest<LessEqual>(1, 0, false);
//     BinaryOpTest<LessEqual>(0, 1, true);
//     BinaryOpTest<LessEqual>(0, 0, true);
// 
//     BinaryOpTest<EqualTo>(true, true, true);
//     BinaryOpTest<EqualTo>(true, false, false);
//     BinaryOpTest<EqualTo>(false, true, false);
//     BinaryOpTest<EqualTo>(false, false, true);
// 
//     BinaryOpTest<NotEqualTo>(true, true, false);
//     BinaryOpTest<NotEqualTo>(true, false, true);
//     BinaryOpTest<NotEqualTo>(false, true, true);
//     BinaryOpTest<NotEqualTo>(false, false, false);
// 
//     AZStd::string string0("a string");
//     AZStd::string string1("b string");
// 
//     BinaryOpTest<EqualTo>(string1, string1, true);
//     BinaryOpTest<EqualTo>(string1, string0, false);
//     BinaryOpTest<EqualTo>(string0, string1, false);
//     BinaryOpTest<EqualTo>(string0, string0, true);
// 
//     BinaryOpTest<NotEqualTo>(string1, string1, false);
//     BinaryOpTest<NotEqualTo>(string1, string0, true);
//     BinaryOpTest<NotEqualTo>(string0, string1, true);
//     BinaryOpTest<NotEqualTo>(string0, string0, false);
// 
//     BinaryOpTest<Greater>(string1, string1, false);
//     BinaryOpTest<Greater>(string1, string0, true);
//     BinaryOpTest<Greater>(string0, string1, false);
//     BinaryOpTest<Greater>(string0, string0, false);
// 
//     BinaryOpTest<GreaterEqual>(string1, string1, true);
//     BinaryOpTest<GreaterEqual>(string1, string0, true);
//     BinaryOpTest<GreaterEqual>(string0, string1, false);
//     BinaryOpTest<GreaterEqual>(string0, string0, true);
// 
//     BinaryOpTest<Less>(string1, string1, false);
//     BinaryOpTest<Less>(string1, string0, false);
//     BinaryOpTest<Less>(string0, string1, true);
//     BinaryOpTest<Less>(string0, string0, false);
// 
//     BinaryOpTest<LessEqual>(string1, string1, true);
//     BinaryOpTest<LessEqual>(string1, string0, false);
//     BinaryOpTest<LessEqual>(string0, string1, true);
//     BinaryOpTest<LessEqual>(string0, string0, true);
// 
//     const AZ::Vector3 vectorOne(1.0f, 1.0f, 1.0f);
//     const AZ::Vector3 vectorZero(0.0f, 0.0f, 0.0f);
// 
//     BinaryOpTest<EqualTo>(vectorOne, vectorOne, true);
//     BinaryOpTest<EqualTo>(vectorOne, vectorZero, false);
//     BinaryOpTest<EqualTo>(vectorZero, vectorOne, false);
//     BinaryOpTest<EqualTo>(vectorZero, vectorZero, true);
// 
//     BinaryOpTest<NotEqualTo>(vectorOne, vectorOne, false);
//     BinaryOpTest<NotEqualTo>(vectorOne, vectorZero, true);
//     BinaryOpTest<NotEqualTo>(vectorZero, vectorOne, true);
//     BinaryOpTest<NotEqualTo>(vectorZero, vectorZero, false);
// 
//     const TestBehaviorContextObject zero(0);
//     const TestBehaviorContextObject one(1);
//     const TestBehaviorContextObject otherOne(1);
// 
//     BinaryOpTest<EqualTo>(one, one, true);
//     BinaryOpTest<EqualTo>(one, zero, false);
//     BinaryOpTest<EqualTo>(zero, one, false);
//     BinaryOpTest<EqualTo>(zero, zero, true);
// 
//     BinaryOpTest<NotEqualTo>(one, one, false);
//     BinaryOpTest<NotEqualTo>(one, zero, true);
//     BinaryOpTest<NotEqualTo>(zero, one, true);
//     BinaryOpTest<NotEqualTo>(zero, zero, false);
// 
//     BinaryOpTest<Greater>(one, one, false);
//     BinaryOpTest<Greater>(one, zero, true);
//     BinaryOpTest<Greater>(zero, one, false);
//     BinaryOpTest<Greater>(zero, zero, false);
// 
//     BinaryOpTest<GreaterEqual>(one, one, true);
//     BinaryOpTest<GreaterEqual>(one, zero, true);
//     BinaryOpTest<GreaterEqual>(zero, one, false);
//     BinaryOpTest<GreaterEqual>(zero, zero, true);
// 
//     BinaryOpTest<Less>(one, one, false);
//     BinaryOpTest<Less>(one, zero, false);
//     BinaryOpTest<Less>(zero, one, true);
//     BinaryOpTest<Less>(zero, zero, false);
// 
//     BinaryOpTest<LessEqual>(one, one, true);
//     BinaryOpTest<LessEqual>(one, zero, false);
//     BinaryOpTest<LessEqual>(zero, one, true);
//     BinaryOpTest<LessEqual>(zero, zero, true);
// 
//     BinaryOpTest<EqualTo>(one, otherOne, true);
//     BinaryOpTest<EqualTo>(otherOne, one, true);
// 
//     BinaryOpTest<NotEqualTo>(one, otherOne, false);
//     BinaryOpTest<NotEqualTo>(otherOne, one, false);
// 
//     BinaryOpTest<Greater>(one, otherOne, false);
//     BinaryOpTest<Greater>(otherOne, one, false);
// 
//     BinaryOpTest<GreaterEqual>(one, otherOne, true);
//     BinaryOpTest<GreaterEqual>(otherOne, one, true);
// 
//     BinaryOpTest<Less>(one, otherOne, false);
//     BinaryOpTest<Less>(otherOne, one, false);
// 
//     BinaryOpTest<LessEqual>(one, otherOne, true);
//     BinaryOpTest<LessEqual>(otherOne, one, true);
// 
//     // force failures, to verify crash proofiness
//     BinaryOpTest<EqualTo>(one, zero, false, true);
//     BinaryOpTest<NotEqualTo>(one, zero, true, true);
//     BinaryOpTest<Greater>(one, zero, true, true);
//     BinaryOpTest<GreaterEqual>(one, zero, true, true);
//     BinaryOpTest<Less>(one, zero, false, true);
//     BinaryOpTest<LessEqual>(one, zero, false, true);
// 
//     m_serializeContext->EnableRemoveReflection();
//     m_behaviorContext->EnableRemoveReflection();
//     TestBehaviorContextObject::Reflect(m_serializeContext);
//     TestBehaviorContextObject::Reflect(m_behaviorContext);
//     m_serializeContext->DisableRemoveReflection();
//     m_behaviorContext->DisableRemoveReflection();
// }

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

    // Script Canvas ScriptCanvas::Graph
    ScriptCanvas::Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    m_entityContext.AddEntity(first.GetId());
    m_entityContext.AddEntity(second.GetId());
    m_entityContext.AddEntity(graph->GetEntityId());

    graph->GetEntity()->SetName("ScriptCanvas::Graph Owner Entity");

    graph->GetEntity()->CreateComponent<ScriptCanvasTests::TestComponent>();
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    // EntityRef node to some specific entity #1
    AZ::EntityId selfEntityIdNode;
    auto selfEntityRef = CreateTestNode<ScriptCanvas::Nodes::Entity::EntityRef>(graphUniqueId, selfEntityIdNode);
    selfEntityRef->SetEntityRef(first.GetId());

    // EntityRef node to some specific entity #2
    AZ::EntityId otherEntityIdNode;
    auto otherEntityRef = CreateTestNode<ScriptCanvas::Nodes::Entity::EntityRef>(graphUniqueId, otherEntityIdNode);
    otherEntityRef->SetEntityRef(second.GetId());

    // Explicitly set an EntityRef node with this graph's entity Id.
    AZ::EntityId graphEntityIdNode;
    auto graphEntityRef = CreateTestNode<ScriptCanvas::Nodes::Entity::EntityRef>(graphUniqueId, graphEntityIdNode);
    graphEntityRef->SetEntityRef(graph->GetEntity()->GetId());

    // First test: directly set an entity Id on the EntityID: 0 slot
    AZ::EntityId eventAid;
    ScriptCanvas::Nodes::Core::Method* nodeA = CreateTestNode<ScriptCanvas::Nodes::Core::Method>(graphUniqueId, eventAid);
    nodeA->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");
    if (auto entityId = nodeA->ModInput_UNIT_TEST<AZ::EntityId>("EntityID: 0"))
    {
        *entityId = first.GetId();
    }

    // Second test: Will connect the slot to an EntityRef node
    AZ::EntityId eventBid;
    ScriptCanvas::Nodes::Core::Method* nodeB = CreateTestNode<ScriptCanvas::Nodes::Core::Method>(graphUniqueId, eventBid);
    nodeB->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");

    // Third test: Set the slot's EntityId: 0 to SelfReferenceId, this should result in the same Id as graph->GetEntityId()
    AZ::EntityId eventCid;
    ScriptCanvas::Nodes::Core::Method* nodeC = CreateTestNode<ScriptCanvas::Nodes::Core::Method>(graphUniqueId, eventCid);
    nodeC->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");
    if (auto entityId = nodeC->ModInput_UNIT_TEST<AZ::EntityId>("EntityID: 0"))
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
    //                   -O EntityId: 0  (not connected, slot is set to SelfReferenceId)
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

    ScriptCanvas::Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId previousID = startID;

    for (int i = 0; i < k_executionCount; ++i)
    {
        AZ::EntityId printNodeID;
        TestResult* printNode = CreateTestNode<TestResult>(graphUniqueId, printNodeID);
        printNode->SetInput_UNIT_TEST<int>("Value", i);
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

TEST_F(ScriptCanvasTestFixture, While)
{
    RunUnitTestGraph("LY_SC_UnitTest_While", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, WhileBreak)
{
    RunUnitTestGraph("LY_SC_UnitTest_WhileBreak", ExecutionMode::Interpreted);
}
