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

#include <AzCore/Math/Vector3.h>
#include <ScriptCanvas/Libraries/Math/Math.h>
#include <ScriptCanvas/Data/Data.h>

using namespace ScriptCanvasTests;

template<typename t_Node>
AZStd::vector<ScriptCanvas::Datum> TestMathFunction(std::initializer_list<AZStd::string_view> inputNamesList, std::initializer_list<ScriptCanvas::Datum> inputList, std::initializer_list<AZStd::string_view> outputNamesList, std::initializer_list<ScriptCanvas::Datum> outputList)
{
    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::vector<AZStd::string_view> inputNames(inputNamesList);
    AZStd::vector<Datum> input(inputList);
    AZStd::vector<AZStd::string_view> outputNames(outputNamesList);
    AZStd::vector<Datum> output(outputList);

    AZ_Assert(inputNames.size() == input.size(), "Size mismatch");
    AZ_Assert(outputNames.size() == output.size(), "Size mismatch");

    AZ::Entity graphEntity("Graph");
    graphEntity.Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, &graphEntity);
    auto graph = graphEntity.FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId functionID;
    CreateTestNode<t_Node>(graphUniqueId, functionID);
    EXPECT_TRUE(Connect(*graph, startID, "Out", functionID, "In"));

    AZStd::vector<Node*> inputNodes;
    AZStd::vector<AZ::EntityId> inputNodeIDs;
    AZStd::vector<Node*> outputNodes;
    AZStd::vector<AZ::EntityId> outputNodeIDs;

    for (int i = 0; i < input.size(); ++i)
    {
        AZ::EntityId inputNodeID;
        auto node = CreateDataNodeByType(graphUniqueId, input[i].GetType(), inputNodeID);
        inputNodeIDs.push_back(inputNodeID);
        inputNodes.push_back(node);
        NodeAccessor::SetInput_UNIT_TEST(node, PureData::k_setThis, input[i]);
    }

    for (int i = 0; i < output.size(); ++i)
    {
        AZ::EntityId outputNodeID;
        auto node = CreateDataNodeByType(graphUniqueId, output[i].GetType(), outputNodeID);
        outputNodeIDs.push_back(outputNodeID);
        outputNodes.push_back(node);
    }

    for (int i = 0; i < inputNames.size(); ++i)
    {
        EXPECT_TRUE(Connect(*graph, inputNodeIDs[i], "Get", functionID, inputNames[i].data()));
    }

    for (int i = 0; i < output.size(); ++i)
    {
        EXPECT_TRUE(Connect(*graph, functionID, outputNames[i].data(), outputNodeIDs[i], PureData::k_setThis));
    }

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    for (int i = 0; i < output.size(); ++i)
    {
        output[i] = *NodeAccessor::GetInput_UNIT_TEST(outputNodes[i], PureData::k_setThis);
    }

    graph->GetEntity()->Deactivate();
    delete graph;
    return output;
}


TEST_F(ScriptCanvasTestFixture, MathCustom)
{
    
    using namespace ScriptCanvas;

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::Vector3 allOne(1, 1, 1);

    AZ::EntityId justCompile;
    CreateTestNode<Nodes::Math::AABB>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Color>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::CRC>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Matrix3x3>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Matrix4x4>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::OBB>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Plane>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Vector2>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Vector4>(graphUniqueId, justCompile);

    AZ::EntityId addVectorId;
    Node* addVector = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne, addVectorId);
    AZ::EntityId subtractVectorId;
    Node* subtractVector = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne, subtractVectorId);
    AZ::EntityId normalizeVectorId;
    Node* normalizeVector = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne, normalizeVectorId);

    AZ::EntityId addId;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addId);
    AZ::EntityId subtractId;
    CreateTestNode<Vector3Nodes::SubtractNode>(graphUniqueId, subtractId);
    AZ::EntityId normalizeId;
    CreateTestNode<Vector3Nodes::NormalizeNode>(graphUniqueId, normalizeId);

    EXPECT_TRUE(Connect(*graph, addVectorId, "Get", addId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, addVectorId, "Get", addId, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, addVectorId, "Set", addId, "Result: Vector3"));

    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Get", subtractId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Get", subtractId, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Set", subtractId, "Result: Vector3"));

    EXPECT_TRUE(Connect(*graph, normalizeVectorId, "Get", normalizeId, "Vector3: Source"));
    EXPECT_TRUE(Connect(*graph, normalizeVectorId, "Set", normalizeId, "Result: Vector3"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", addId, "In"));
    EXPECT_TRUE(Connect(*graph, addId, "Out", subtractId, "In"));
    EXPECT_TRUE(Connect(*graph, subtractId, "Out", normalizeId, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(addVector, "Set"), AZ::Vector3(2, 2, 2));
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(subtractVector, "Set"), AZ::Vector3::CreateZero());
    EXPECT_TRUE(GetInput_UNIT_TEST<AZ::Vector3>(normalizeVector, "Set")->IsNormalized());

    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, MathMixed1)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

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

    const AZ::Vector3 allOne4(1, 1, 1);
    const AZ::Vector3 allTwo4(2, 2, 2);
    const AZ::Vector3 allFour4(4, 4, 4);

    const AZ::Vector3 allOne3(allOne4);
    const AZ::Vector3 allTwo3(allTwo4);
    const AZ::Vector3 allFour3(allFour4);

    AZ::EntityId vectorAId, vectorBId, vectorCId, vectorDId;
    Node* vectorANode = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorAId);
    Node* vectorBNode = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorBId);
    Node* vectorCNode = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorCId);
    Node* vectorDNode = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorDId);

    AZ::EntityId vector3AId, vector3BId, vector3CId, vector3DId;

    Core::BehaviorContextObjectNode* vector3ANode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3AId);
    vector3ANode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3ANode, "Set") = allOne3;

    Core::BehaviorContextObjectNode* vector3BNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3BId);
    vector3BNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3BNode, "Set") = allOne3;

    Core::BehaviorContextObjectNode* vector3CNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3CId);
    vector3CNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3CNode, "Set") = allOne3;

    Core::BehaviorContextObjectNode* vector3DNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3DId);
    vector3DNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3DNode, "Set") = allOne3;

    AZ::EntityId addId;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addId);
    
    AZ::EntityId add3Id;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, add3Id);

    // data connections
    EXPECT_TRUE(Connect(*graph, vectorAId, "Get", addId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, vector3AId, "Get", addId, "Vector3: B"));

    EXPECT_TRUE(Connect(*graph, addId, "Result: Vector3", vector3BId, "Set"));
    EXPECT_TRUE(Connect(*graph, addId, "Result: Vector3", vectorBId, "Set"));

    EXPECT_TRUE(Connect(*graph, vectorBId, "Get", vector3CId, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3CId, "Get", vectorCId, "Set"));

    EXPECT_TRUE(Connect(*graph, vector3BId, "Get", add3Id, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, vector3CId, "Get", add3Id, "Vector3: B"));

    EXPECT_TRUE(Connect(*graph, add3Id, "Result: Vector3", vectorDId, "Set"));
    EXPECT_TRUE(Connect(*graph, add3Id, "Result: Vector3", vector3DId, "Set"));

    // execution connections
    EXPECT_TRUE(Connect(*graph, startID, "Out", addId, "In"));
    EXPECT_TRUE(Connect(*graph, addId, "Out", add3Id, "In"));

    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorANode, "Set"), allOne4);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorBNode, "Set"), allOne4);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorCNode, "Set"), allOne4);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorDNode, "Set"), allOne4);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vector3ANode, "Set"), allOne3);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vector3BNode, "Set"), allOne3);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vector3CNode, "Set"), allOne3);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vector3DNode, "Set"), allOne3);

    auto vector3Aptr = *GetInput_UNIT_TEST<AZ::Vector3>(vector3ANode, "Set");
    auto vector3Bptr = *GetInput_UNIT_TEST<AZ::Vector3>(vector3BNode, "Set");
    auto vector3Cptr = *GetInput_UNIT_TEST<AZ::Vector3>(vector3CNode, "Set");
    auto vector3Dptr = *GetInput_UNIT_TEST<AZ::Vector3>(vector3DNode, "Set");

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    auto vectorA = *GetInput_UNIT_TEST<AZ::Vector3>(vectorANode, "Set");
    auto vectorB = *GetInput_UNIT_TEST<AZ::Vector3>(vectorBNode, "Set");
    auto vectorC = *GetInput_UNIT_TEST<AZ::Vector3>(vectorCNode, "Set");
    auto vectorD = *GetInput_UNIT_TEST<AZ::Vector3>(vectorDNode, "Set");

    auto vector3A = *GetInput_UNIT_TEST<AZ::Vector3>(vector3ANode, "Set");
    auto vector3B = *GetInput_UNIT_TEST<AZ::Vector3>(vector3BNode, "Set");
    auto vector3C = *GetInput_UNIT_TEST<AZ::Vector3>(vector3CNode, "Set");
    auto vector3D = *GetInput_UNIT_TEST<AZ::Vector3>(vector3DNode, "Set");

    EXPECT_EQ(vectorA, allOne4);
    EXPECT_EQ(vectorB, allTwo4);
    EXPECT_EQ(vectorC, allTwo4);
    EXPECT_EQ(vectorD, allFour4);

    EXPECT_EQ(vector3A, allOne3);
    EXPECT_EQ(vector3B, allTwo3);
    EXPECT_EQ(vector3C, allTwo3);
    EXPECT_EQ(vector3D, allFour3);

    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, MathMixed2)
{
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

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

    const AZ::Vector3 allOne4(1, 1, 1);
    const AZ::Vector3 allEight4(8, 8, 8);

    const AZ::Vector3 allOne3(allOne4);
    const AZ::Vector3 allEight3(allEight4);

    AZ::EntityId vectorIdA, vectorIdB, vectorIdC, vectorIdD;
    Node* vectorNodeA = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorIdA);
    Node* vectorNodeB = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorIdB);
    Node* vectorNodeC = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorIdC);
    Node* vectorNodeD = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorIdD);

    AZ::EntityId vector3IdA, vector3IdB, vector3IdC, vector3IdD;
    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3NodeA, "Set") = allOne3;
    Core::BehaviorContextObjectNode* vector3NodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdB);
    vector3NodeB->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3NodeB, "Set") = allOne3;
    Core::BehaviorContextObjectNode* vector3NodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdC);
    vector3NodeC->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3NodeC, "Set") = allOne3;
    Core::BehaviorContextObjectNode* vector3NodeD = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdD);
    vector3NodeD->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *ModInput_UNIT_TEST<AZ::Vector3>(vector3NodeD, "Set") = allOne3;

    AZ::EntityId addIdA;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addIdA);
    AZ::EntityId addIdB;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addIdB);
    AZ::EntityId addIdC;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addIdC);

    AZ::EntityId add3IdA;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, add3IdA);
    AZ::EntityId add3IdB;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, add3IdB);
    AZ::EntityId add3IdC;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, add3IdC);

    AZ::EntityId vectorNormalizeId;
    CreateTestNode<Vector3Nodes::NormalizeNode>(graphUniqueId, vectorNormalizeId);

    const AZ::Vector3 x10(10, 0, 0);
    AZ::EntityId vectorNormalizedInId, vectorNormalizedOutId, numberLengthId;
    Node* vectorNormalizedInNode = CreateDataNode<AZ::Vector3>(graphUniqueId, x10, vectorNormalizedInId);
    Node* vectorNormalizedOutNode = CreateDataNode<AZ::Vector3>(graphUniqueId, x10, vectorNormalizedOutId);

    // data connections

    EXPECT_TRUE(Connect(*graph, vectorIdA, "Get", addIdA, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, vectorIdA, "Get", add3IdA, "Vector3: A"));

    EXPECT_TRUE(Connect(*graph, addIdA, "Result: Vector3", addIdB, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, addIdA, "Result: Vector3", add3IdB, "Vector3: A"));

    EXPECT_TRUE(Connect(*graph, addIdB, "Result: Vector3", addIdC, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, addIdB, "Result: Vector3", add3IdC, "Vector3: A"));

    EXPECT_TRUE(Connect(*graph, addIdC, "Result: Vector3", vectorIdB, "Set"));
    EXPECT_TRUE(Connect(*graph, addIdC, "Result: Vector3", vector3IdB, "Set"));

    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", vectorIdC, "Set"));

    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", addIdA, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", add3IdA, "Vector3: B"));

    EXPECT_TRUE(Connect(*graph, add3IdA, "Result: Vector3", addIdB, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, add3IdA, "Result: Vector3", add3IdB, "Vector3: B"));

    EXPECT_TRUE(Connect(*graph, add3IdB, "Result: Vector3", addIdC, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, add3IdB, "Result: Vector3", add3IdC, "Vector3: B"));

    EXPECT_TRUE(Connect(*graph, add3IdC, "Result: Vector3", vectorIdD, "Set"));
    EXPECT_TRUE(Connect(*graph, add3IdC, "Result: Vector3", vector3IdD, "Set"));

    EXPECT_TRUE(Connect(*graph, vectorIdD, "Get", vector3IdC, "Set"));

    EXPECT_TRUE(Connect(*graph, vectorNormalizedInId, "Get", vectorNormalizeId, "Vector3: Source"));
    EXPECT_TRUE(Connect(*graph, vectorNormalizeId, "Result: Vector3", vectorNormalizedOutId, "Set"));

    // execution connections
    EXPECT_TRUE(Connect(*graph, startID, "Out", addIdA, "In"));
    EXPECT_TRUE(Connect(*graph, addIdA, "Out", add3IdA, "In"));
    EXPECT_TRUE(Connect(*graph, add3IdA, "Out", addIdB, "In"));
    EXPECT_TRUE(Connect(*graph, addIdB, "Out", add3IdB, "In"));
    EXPECT_TRUE(Connect(*graph, add3IdB, "Out", addIdC, "In"));
    EXPECT_TRUE(Connect(*graph, addIdC, "Out", add3IdC, "In"));

    EXPECT_TRUE(Connect(*graph, add3IdC, "Out", vectorNormalizeId, "In"));

    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorNodeA, "Set"), allOne4);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorNodeB, "Set"), allOne4);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorNodeC, "Set"), allOne4);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorNodeD, "Set"), allOne4);

    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorNormalizedInNode, "Set"), x10);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vectorNormalizedOutNode, "Set"), x10);

    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeA, "Set"), allOne3);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeB, "Set"), allOne3);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeC, "Set"), allOne3);
    EXPECT_EQ(*GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeD, "Set"), allOne3);

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    auto vectorA = *GetInput_UNIT_TEST<AZ::Vector3>(vectorNodeA, "Set");
    auto vectorB = *GetInput_UNIT_TEST<AZ::Vector3>(vectorNodeB, "Set");
    auto vectorC = *GetInput_UNIT_TEST<AZ::Vector3>(vectorNodeC, "Set");
    auto vectorD = *GetInput_UNIT_TEST<AZ::Vector3>(vectorNodeD, "Set");

    auto vector3A = *GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeA, "Set");
    auto vector3Aptr = GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeA, "Set");

    auto vector3B = *GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeB, "Set");
    auto vector3C = *GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeC, "Set");
    auto vector3D = *GetInput_UNIT_TEST<AZ::Vector3>(vector3NodeD, "Set");

    EXPECT_EQ(vectorA, allOne4);
    EXPECT_EQ(vectorB, allEight4);
    EXPECT_EQ(vectorC, allEight4);
    EXPECT_EQ(vectorD, allEight4);

    EXPECT_EQ(vector3A, allOne3);
    EXPECT_EQ(vector3B, allEight3);
    EXPECT_EQ(vector3C, allEight3);
    EXPECT_EQ(vector3D, allEight3);

    auto normalizedIn = *GetInput_UNIT_TEST<AZ::Vector3>(vectorNormalizedInNode, "Set");
    auto normalizedOut = *GetInput_UNIT_TEST<AZ::Vector3>(vectorNormalizedOutNode, "Set");
    EXPECT_EQ(x10, normalizedIn);
    EXPECT_EQ(AZ::Vector3(1, 0, 0), normalizedOut);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, MathOperations)
{
    

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes::Math;

    BinaryOpTest<Multiply>(3, 4, 3 * 4);
    BinaryOpTest<Multiply>(3.5f, 2.0f, 3.5f*2.0f);
    BinaryOpTest<Sum>(3, 4, 3 + 4);
    BinaryOpTest<Sum>(3.f, 4.f, 3.0f + 4.0f);
    BinaryOpTest<Subtract>(3, 4, 3 - 4);
    BinaryOpTest<Subtract>(3.f, 4.f, 3.f - 4.f);
    BinaryOpTest<Divide>(3, 4, 3 / 4);
    BinaryOpTest<Divide>(3.f, 4.f, 3.f / 4.f);
}

TEST_F(ScriptCanvasTestFixture, ColorNodes)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::ColorNodes;
    using namespace ScriptCanvas::Data;
    using namespace ScriptCanvas::Nodes;

    const ColorType a(Vector4Type(0.5f, 0.25f, 0.75f, 1.0f));
    const ColorType b(Vector4Type(0.66f, 0.33f, 0.5f, 0.5f));
    const NumberType number(0.314);

    { // Add
        auto result = a + b;
        auto output = TestMathFunction<AddNode>({ "Color: A", "Color: B" }, { Datum(a), Datum(b) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // DivideByColor
        auto result = a / b;
        auto output = TestMathFunction<DivideByColorNode>({ "Color: A", "Color: B" }, { Datum(a), Datum(b) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }
#endif

    { // DivideByNumber
        auto result = a / number;
        auto output = TestMathFunction<DivideByNumberNode>({ "Color: Source", "Number: Divisor" }, { Datum(a), Datum(number) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // Dot
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Color: A", "Color: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(NumberType()) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // Dot3
        auto result = a.Dot3(b);
        auto output = TestMathFunction<Dot3Node>({ "Color: A", "Color: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(NumberType()) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // FromValues
        auto result = ColorType(0.2f, 0.4f, 0.6f, 0.8f);
        auto output = TestMathFunction<FromValuesNode>({ "Number: R", "Number: G", "Number: B", "Number: A" }, { Datum(0.2f), Datum(0.4f), Datum(0.6f), Datum(0.8f) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // FromVector3
        auto source = Vector3Type(0.2f, 0.4f, 0.6f);
        auto result = ColorType::CreateFromVector3(source);
        auto output = TestMathFunction<FromVector3Node>({ "Vector3: RGB" }, { Datum(source) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // FromVector3AndNumber
        auto vector3 = Vector3Type(0.1, 0.2, 0.3);
        auto number2 = 0.4f;
        auto result = ColorType::CreateFromVector3AndFloat(vector3, number2);
        auto output = TestMathFunction<FromVector3AndNumberNode>({ "Vector3: RGB", "Number: A" }, { Datum(vector3), Datum(number2) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // FromVector4
        auto vector4 = Vector4Type(0.1, 0.2, 0.3, 0.4);
        auto result = ColorType(vector4);
        auto output = TestMathFunction<FromVector4Node>({ "Vector4: RGBA" }, { Datum(vector4) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }
#endif

    { // GammaToLinear
        auto result = a.GammaToLinear();
        auto output = TestMathFunction<GammaToLinearNode>({ "Color: Source" }, { Datum(a) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // IsClose
        {
            auto result = a.IsClose(a);
            auto output = TestMathFunction<IsCloseNode>({ "Color: A", "Color: B", "Number: Tolerance" }, { Datum(a), Datum(a), Datum(0.1f) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_TRUE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }

        {
            auto result = b.IsClose(a);
            auto output = TestMathFunction<IsCloseNode>({ "Color: A", "Color: B", "Number: Tolerance" }, { Datum(b), Datum(a), Datum(0.1f) }, { "Result: Boolean" }, { Datum(true) });
            EXPECT_FALSE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }
    }

    { // IsZero
        {
            auto zero = ColorType::CreateZero();
            auto result = zero.IsZero();
            auto output = TestMathFunction<IsZeroNode>({ "Color: Source",  "Number: Tolerance" }, { Datum(zero), Datum(0.1f) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_TRUE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }

        {
            auto result = a.IsZero();
            auto output = TestMathFunction<IsZeroNode>({ "Color: Source", "Number: Tolerance" }, { Datum(a), Datum(0.1f) }, { "Result: Boolean" }, { Datum(true) });
            EXPECT_FALSE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }
    }

    { // LinearToGamma
        auto result = a.LinearToGamma();
        auto output = TestMathFunction<LinearToGammaNode>({ "Color: Source" }, { Datum(a) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // MultiplyByColor
        auto result = a * b;
        auto output = TestMathFunction<MultiplyByColorNode>({ "Color: A", "Color: B" }, { Datum(a), Datum(b) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // MultiplyByNumber
        auto result = a * number;
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Color: Source", "Number: Multiplier" }, { Datum(a), Datum(number) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // Negate
        auto result = -a;
        auto output = TestMathFunction<NegateNode>({ "Color: Source" }, { Datum(a) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // One
        auto source = ColorType();
        auto result = ColorType::CreateOne();
        auto output = TestMathFunction<OneNode>({}, {}, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // Subtract
        auto result = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Color: A", "Color: B" }, { Datum(a), Datum(b) }, { "Result: Color" }, { Datum(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    // Missing:
    // ModRNode
    // ModGNode
    // ModBNode
    // ModANode
}

TEST_F(ScriptCanvasTestFixture, CrcNodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::CrcNodes;

    { // FromString
        Data::StringType value = "Test";
        auto result = Data::CRCType(value.data());
        auto output = TestMathFunction<FromStringNode>({ "String: Value" }, { Datum(value) },
        { "Result: CRC" }, { Datum(Data::CRCType()) });
        EXPECT_EQ(result, *output[0].GetAs<Data::CRCType>());
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // FromNumber
        AZ::u32 value = 0x784dd132;
        auto result = Data::CRCType(value);
        auto output = TestMathFunction<FromNumberNode>({ "Number: Value" }, { Datum(static_cast<AZ::u64>(value)) },
        { "Result: CRC" }, { Datum(Data::CRCType()) });
        EXPECT_EQ(result, *output[0].GetAs<Data::CRCType>());
    }

    { // GetNumber
        auto source = Data::CRCType("Test");
        auto result = source.operator AZ::u32();
        auto output = TestMathFunction<GetNumberNode>({ "CRC: Value", }, { Datum(source) }, { "Result: Number" }, { Datum(Data::NumberType()) });
        EXPECT_EQ(result, *output[0].GetAs<Data::NumberType>());
    }
#endif
}

TEST_F(ScriptCanvasTestFixture, AABBNodes)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
    using namespace ScriptCanvas::AABBNodes;
    using namespace ScriptCanvas::Data;

    const Vector3Type outsideMax(222);
    const Vector3Type min3(-111, -111, -111);
    const Vector3Type max3(111, 111, 111);

    const Vector3Type minHalf3(min3 * 0.5f);
    const Vector3Type maxHalf3(max3 * 0.5f);

    auto IsClose = [](const AABBType& lhs, const AABBType& rhs)->bool { return lhs.GetMin().IsClose(rhs.GetMin()) && lhs.GetMax().IsClose(rhs.GetMax()); };

    { // AddAabb
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<AddAABBNode>({ "AABB: A", "AABB: B" }, { Datum(source), Datum(source) }, { "Result: AABB" }, { Datum(Data::AABBType()) });
        source.AddAabb(source);
        EXPECT_TRUE(IsClose(source, *output[0].GetAs<AABBType>()));
    }

    { // AddPoint
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<AddPointNode>({ "AABB: Source", "Vector3: Point" }, { Datum(source), Datum(outsideMax) }, { "Result: AABB" }, { Datum(Data::AABBType()) });
        source.AddPoint(outsideMax);
        EXPECT_TRUE(IsClose(source, *output[0].GetAs<AABBType>()));
    }

    { // ApplyTransform
        auto transform = TransformType::CreateIdentity();
        transform.SetFromEulerDegrees(Vector3Type(10.0f, 20.0f, 30.0f));
        transform.SetTranslation(Vector3Type(4.0f, 4.0f, 4.0f));
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<ApplyTransformNode>({ "AABB: Source", "Transform: Transform" }, { Datum(source), Datum(transform) }, { "Result: AABB" }, { Datum(Data::AABBType()) });
        source.ApplyTransform(transform);
        EXPECT_TRUE(IsClose(source, *output[0].GetAs<AABBType>()));
    }

    { // Center
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<CenterNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        auto result = source.GetCenter();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // Clamp
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto sourceHalf = AABBType::CreateFromMinMax(minHalf3, maxHalf3);
        auto output = TestMathFunction<ClampNode>({ "AABB: Source", "AABB: Clamp" }, { Datum(source), Datum(sourceHalf) }, { "Result: AABB" }, { Datum(AABBType()) });
        auto result = source.GetClamped(sourceHalf);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // ContainsAABB
        auto bigger = AABBType::CreateFromMinMax(min3, max3);
        auto smaller = AABBType::CreateFromMinMax(minHalf3, maxHalf3);
        auto output = TestMathFunction<ContainsAABBNode>({ "AABB: Source", "AABB: Candidate" }, { Datum(bigger), Datum(smaller) }, { "Result: Boolean" }, { Datum(BooleanType()) });
        auto result = bigger.Contains(smaller);
        EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        EXPECT_TRUE(result);
    }

    { // ContainsVector3Node
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto contained = Vector3Type::CreateZero();
        auto NOTcontained = Vector3Type(-10000, -10000, -10000);

        {
            auto output = TestMathFunction<ContainsVector3Node>({ "AABB: Source", "Vector3: Candidate" }, { Datum(source), Datum(contained) }, { "Result: Boolean" }, { Datum(false) });
            auto result = source.Contains(contained);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
            EXPECT_TRUE(result);
        }
        {
            auto output = TestMathFunction<ContainsVector3Node>({ "AABB: Source", "Vector3: Candidate" }, { Datum(source), Datum(NOTcontained) }, { "Result: Boolean" }, { Datum(true) });
            auto result = source.Contains(NOTcontained);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
            EXPECT_FALSE(result);
        }
    }

    { // ZExtentNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<ZExtentNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(NumberType()) });
        float result = source.GetZExtent();
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // DistanceNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto point = Vector3Type(100, 3000, 15879);
        auto output = TestMathFunction<DistanceNode>({ "AABB: Source", "Vector3: Point" }, { Datum(source), Datum(point) }, { "Result: Number" }, { Datum(NumberType()) });
        float result = source.GetDistance(point);
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // ExpandNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto delta = Vector3Type(10, 20, 30);
        auto output = TestMathFunction<ExpandNode>({ "AABB: Source", "Vector3: Delta" }, { Datum(source), Datum(delta) }, { "Result: AABB" }, { Datum(AABBType()) });
        source.Expand(delta);
        EXPECT_TRUE(IsClose(source, *output[0].GetAs<AABBType>()));
    }

    { // ExtentsNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<ExtentsNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        auto result = source.GetExtents();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // FromCenterHalfExtentsNode
        auto center = Vector3Type(1, 2, 3);
        auto extents = Vector3Type(9, 8, 7);
        auto output = TestMathFunction<FromCenterHalfExtentsNode>({ "Vector3: Center", "Vector3: HalfExtents" }, { Datum(center), Datum(extents) }, { "Result: AABB" }, { Datum(AABBType()) });
        auto result = AABBType::CreateCenterHalfExtents(center, extents);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // FromCenterRadiusNode
        auto center = Vector3Type(1, 2, 3);
        auto radius = 33.0f;
        auto output = TestMathFunction<FromCenterRadiusNode>({ "Vector3: Center", "Number: Radius" }, { Datum(center), Datum(radius) }, { "Result: AABB" }, { Datum(AABBType()) });
        auto result = AABBType::CreateCenterRadius(center, radius);
        auto output0 = *output[0].GetAs<AABBType>();
        EXPECT_TRUE(IsClose(result, output0));
    }

    { // FromMinMaxNode
        auto result = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<FromMinMaxNode>({ "Vector3: Min", "Vector3: Max" }, { Datum(min3), Datum(max3) }, { "Result: AABB" }, { Datum(AABBType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // FromOBBNode
        auto source = OBBType::CreateFromAabb(AABBType::CreateFromMinMax(min3, max3));
        auto output = TestMathFunction<FromOBBNode>({ "OBB: Source" }, { Datum(source) }, { "Result: AABB" }, { Datum(AABBType()) });
        auto result = AABBType::CreateFromObb(source);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // FromPointNode
        auto source = Vector3Type(3, 2, 1);
        auto output = TestMathFunction<FromPointNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: AABB" }, { Datum(AABBType()) });
        auto result = AABBType::CreateFromPoint(source);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // IsFiniteNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<IsFiniteNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Boolean" }, { Datum(BooleanType()) });
        auto result = source.IsFinite();
        EXPECT_TRUE(result);
        EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
    }

    { // IsValidNode
        {
            auto source = AABBType::CreateFromMinMax(min3, max3);
            auto output = TestMathFunction<IsValidNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Boolean" }, { Datum(BooleanType()) });
            auto result = source.IsValid();
            EXPECT_TRUE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }
    }

    { // YExtentNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<YExtentNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(NumberType()) });
        float result = source.GetYExtent();
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // NullNode
        auto output = TestMathFunction<NullNode>({}, {}, { "Result: AABB" }, { Datum(AABBType()) });
        auto result = AABBType::CreateNull();
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // OverlapsNode
        {
            auto a = AABBType::CreateFromMinMax(min3, max3);
            auto b = AABBType::CreateFromMinMax(min3 + Vector3Type(5, 5, 5), max3 + Vector3Type(5, 5, 5));
            auto output = TestMathFunction<OverlapsNode>({ "AABB: A", "AABB: B" }, { Datum(a), Datum(b) }, { "Result: Boolean" }, { Datum(false) });
            auto result = a.Overlaps(b);
            EXPECT_TRUE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }

        {
            auto a = AABBType::CreateFromMinMax(min3, max3);
            auto b = AABBType::CreateFromMinMax(min3 + Vector3Type(300, 300, 300), max3 + Vector3Type(300, 300, 300));
            auto output = TestMathFunction<OverlapsNode>({ "AABB: A", "AABB: B" }, { Datum(a), Datum(b) }, { "Result: Boolean" }, { Datum(false) });
            auto result = a.Overlaps(b);
            EXPECT_FALSE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }
    }

    { // SurfaceAreaNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<SurfaceAreaNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(NumberType()) });
        float result = source.GetSurfaceArea();
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // GetMaxNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<GetMaxNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        auto result = source.GetMax();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // GetMinNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<GetMinNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        auto result = source.GetMin();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // ToSphereNode
        auto source = AABBType::CreateFromMinMax(min3 + Vector3Type(5, 5, 5), max3 + Vector3Type(5, 5, 5));
        auto output = TestMathFunction<ToSphereNode>({ "AABB: Source" }, { Datum(source) }, { "Center: Vector3", "Radius: Number" }, { Datum(Vector3Type()), Datum(NumberType()) });
        Vector3Type center;
        float radiusVF;
        source.GetAsSphere(center, radiusVF);
        EXPECT_TRUE(center.IsClose(*output[0].GetAs<Vector3Type>()));
        float radius = radiusVF;
        SC_EXPECT_FLOAT_EQ(radius, aznumeric_caster(*output[1].GetAs<NumberType>()));
    }

    { // TranslateNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto translation = Vector3Type(25, 30, -40);
        auto output = TestMathFunction<TranslateNode>({ "AABB: Source", "Vector3: Translation" }, { Datum(source), Datum(translation) }, { "Result: AABB" }, { Datum(AABBType()) });
        auto result = source.GetTranslated(translation);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // XExtentNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<XExtentNode>({ "AABB: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(NumberType()) });
        float result = source.GetXExtent();
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }
}

TEST_F(ScriptCanvasTestFixture, Matrix3x3Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Matrix3x3Nodes;

    {   // Add
        auto a(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne()));
        auto b(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne()));
        auto c = a + b;
        auto output = TestMathFunction<AddNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum(a), Datum(b) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    {   // DivideByNumber
        auto a(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type(3.0, 3.0, 3.0), Data::Vector3Type(3.0, 3.0, 3.0), Data::Vector3Type(3.0, 3.0, 3.0)));
        Data::NumberType b(3.0);
        auto result = a / static_cast<float>(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Matrix3x3: Source", "Number: Divisor" }, { Datum(a), Datum(b) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromColumns
        auto col1 = Data::Vector3Type(1.0, 2.0, 3.0);
        auto col2 = Data::Vector3Type(4.0, 5.0, 6.0);
        auto col3 = Data::Vector3Type(7.0, 8.0, 9.0);
        auto result(Data::Matrix3x3Type::CreateFromColumns(col1, col2, col3));
        auto output = TestMathFunction<FromColumnsNode>({ "Vector3: Column1", "Vector3: Column2", "Vector3: Column3" }, { Datum(col1), Datum(col2), Datum(col3) },
        { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromCrossProduct
        auto source = Data::Vector3Type(1.0, -1.0, 0.0);
        auto result(Data::Matrix3x3Type::CreateCrossProduct(source));
        auto output = TestMathFunction<FromCrossProductNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromDiagonal
        auto source = Data::Vector3Type(1.0, 1.0, 1.0);
        auto result(Data::Matrix3x3Type::CreateDiagonal(source));
        auto output = TestMathFunction<FromDiagonalNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromMatrix4x4
        auto row1 = Data::Vector4Type(1.0, 2.0, 3.0, 10.0);
        auto row2 = Data::Vector4Type(4.0, 5.0, 6.0, 20.0);
        auto row3 = Data::Vector4Type(7.0, 8.0, 9.0, -30.0);
        auto row4 = Data::Vector4Type(-75.454, 2.5419, -102343435.72, 5587981.54);
        auto source = Data::Matrix4x4Type::CreateFromRows(row1, row2, row3, row4);
        auto result(Data::Matrix3x3Type::CreateFromMatrix4x4(source));
        auto output = TestMathFunction<FromMatrix4x4Node>({ "Matrix4x4: Source" }, { Datum(source) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromQuaternion
        auto rotation = Data::QuaternionType(1.0, 2.0, 3.0, 4.0);
        auto result(Data::Matrix3x3Type::CreateFromQuaternion(rotation));
        auto output = TestMathFunction<FromQuaternionNode>({ "Quaternion: Source" }, { Datum(rotation) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromRotationXDegrees
      // FromRotationXRadians
        auto degrees = 45.0f;
        auto resultDegrees(Data::Matrix3x3Type::CreateRotationX(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationXDegreesNode>({ "Number: Degrees" }, { Datum(degrees) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromRotationYDegrees
        auto degrees = 30.0f;
        auto resultDegrees(Data::Matrix3x3Type::CreateRotationY(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationYDegreesNode>({ "Number: Degrees" }, { Datum(degrees) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromRotationZDegrees
        auto degrees = 60.0f;
        auto resultDegrees(Data::Matrix3x3Type::CreateRotationZ(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationZDegreesNode>({ "Number: Degrees" }, { Datum(degrees) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromRows
        auto row1 = Data::Vector3Type(1.0, 2.0, 3.0);
        auto row2 = Data::Vector3Type(4.0, 5.0, 6.0);
        auto row3 = Data::Vector3Type(7.0, 8.0, 9.0);
        auto result(Data::Matrix3x3Type::CreateFromRows(row1, row2, row3));
        auto output = TestMathFunction<FromRowsNode>({ "Vector3: Row1", "Vector3: Row2", "Vector3: Row3" }, { Datum(row1), Datum(row2), Datum(row3) },
        { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromScale
        auto scale = Data::Vector3Type(2.0, 2.0, 2.0);
        auto result(Data::Matrix3x3Type::CreateScale(scale));
        auto output = TestMathFunction<FromScaleNode>({ "Vector3: Scale" }, { Datum(scale) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromTransform
        auto transform = Data::TransformType::CreateIdentity();
        auto result(Data::Matrix3x3Type::CreateFromTransform(transform));
        auto output = TestMathFunction<FromTransformNode>({ "Transform: Transform" }, { Datum(transform) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // Invert
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 1.0, 26.0),
            Data::Vector3Type(7.0, -8.0, 1.0));
        auto result(source.GetInverseFull());
        auto output = TestMathFunction<InvertNode>({ "Matrix3x3: Source", }, { Datum(source) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // IsCloseNode
        auto a(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne()));
        auto b(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type(2, 2, 2), Data::Vector3Type(2, 2, 2), Data::Vector3Type(2, 2, 2)));
        auto c(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type(1.0001, 1.0001, 1.0001), Data::Vector3Type(1.0001, 1.0001, 1.0001), Data::Vector3Type(1.0001, 1.0001, 1.0001)));

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum(a), Datum(b) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, 2.1f);
        output = TestMathFunction<IsCloseNode>({ "Matrix3x3: A", "Matrix3x3: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(2.1) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum(a), Datum(c) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, 0.9f);
        output = TestMathFunction<IsCloseNode>({ "Matrix3x3: A", "Matrix3x3: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(0.9) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, std::numeric_limits<float>::infinity(), 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        auto result = source.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Matrix3x3: Source" }, { Datum(source) }, { "Result: Boolean", }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsOrthogonal
        auto source(Data::Matrix3x3Type::CreateDiagonal(Data::Vector3Type(2.0, 2.0, 2.0)));
        auto result = source.IsOrthogonal();
        auto output = TestMathFunction<IsOrthogonalNode>({ "Matrix3x3: Source" }, { Datum(source) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    {   // MultiplyByNumber
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        Data::NumberType scalar(3.0);
        auto result = source * static_cast<float>(scalar);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Matrix3x3: Source", "Number: Multiplier" }, { Datum(source), Datum(scalar) }, { "Result: Matrix3x3" },
        { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    {  // MultiplyByMatrix
        auto a(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        auto b(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(-1.0, -2.0, 13.0),
            Data::Vector3Type(14.0, 15.0, -6.0),
            Data::Vector3Type(17.0, -8.0, 19.0)));
        auto result = a * b;
        auto output = TestMathFunction<MultiplyByMatrixNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum(a), Datum(b) },
        { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    {  // MultiplyByVector
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        Data::Vector3Type vector3(2.0);
        auto result = source * vector3;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Matrix3x3: Source", "Vector3: Vector" }, { Datum(source), Datum(vector3) },
        { "Result: Vector3" }, { Datum(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Orthogonalize
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        auto result = source.GetOrthogonalized();
        auto output = TestMathFunction<OrthogonalizeNode>({ "Matrix3x3: Source" }, { Datum(source) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    {   // Subtract
        auto a(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type(2.0, -8.5, -6.12), Data::Vector3Type(0.0, 0.0, 2.3), Data::Vector3Type(17.2, 4.533, 16.33492)));
        auto b(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne()));
        auto c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum(a), Datum(b) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // ToAdjugate
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 0.0, 0.0),
            Data::Vector3Type(0.0, 1.0, 0.0),
            Data::Vector3Type(0.0, 0.0, 1.0));
        auto result(source.GetAdjugate());
        auto output = TestMathFunction<ToAdjugateNode>({ "Matrix3x3: Source" }, { Datum(source) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // GetColumnNode
        auto source = Data::Matrix3x3Type::CreateFromColumns(
            Data::Vector3Type(1.0, 2.0, 4.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0));
        auto result(source.GetColumn(1));
        auto output = TestMathFunction<GetColumnNode>({ "Matrix3x3: Source", "Number: Column" }, { Datum(source), Datum(1) },
        { "Result: Vector3" }, { Datum(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // GetColumns
        const Data::Vector3Type col1(1.0, 2.0, 3.0);
        const Data::Vector3Type col2(5.0, 16.5, 21.2);
        const Data::Vector3Type col3(-44.4, -72.1, 72.4);
        auto source = Data::Matrix3x3Type::CreateFromColumns(col1, col2, col3);
        auto output = TestMathFunction<GetColumnsNode>(
        { "Matrix3x3: Source" },
        { Datum(source) },
        { "Column1: Vector3", "Column2: Vector3", "Column3: Vector3" },
        { Datum(Data::Vector3Type::CreateZero()), Datum(Data::Vector3Type::CreateZero()), Datum(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(col1.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(col2.IsClose(*output[1].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(col3.IsClose(*output[2].GetAs<Data::Vector3Type>()));
    }

    { // ToDeterminant
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 0.0, 0.0),
            Data::Vector3Type(0.0, 1.0, 0.0),
            Data::Vector3Type(0.0, 0.0, 1.0));
        auto result(source.GetDeterminant());
        // Test naming single output slots: The GetDeterminant function names it's result slot "Determinant"
        auto output = TestMathFunction<ToDeterminantNode>({ "Matrix3x3: Source" }, { Datum(source) }, { "Determinant: Number" }, { Datum(Data::NumberType()) });
        EXPECT_TRUE(AZ::IsClose(result, static_cast<float>(*output[0].GetAs<Data::NumberType>())));
    }

    { // GetDiagonal
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0));
        auto result(source.GetDiagonal());
        auto output = TestMathFunction<GetDiagonalNode>({ "Matrix3x3: Source", }, { Datum(source) }, { "Result: Vector3" }, { Datum(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // GetElement
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 4.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0));
        auto result(source.GetElement(2, 1));
        auto output = TestMathFunction<GetElementNode>({ "Matrix3x3: Source", "Number: Row", "Number: Column" }, { Datum(source), Datum(2), Datum(1) },
        { "Result: Number" }, { Datum(Data::NumberType()) });
        EXPECT_TRUE(AZ::IsClose(result, static_cast<float>(*output[0].GetAs<Data::NumberType>())));
    }

    { // GetRow
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 4.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0));
        auto result(source.GetRow(0));
        auto output = TestMathFunction<GetRowNode>({ "Matrix3x3: Source", "Number: Row" }, { Datum(source), Datum(0) },
        { "Result: Vector3" }, { Datum(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // GetRows
        const Data::Vector3Type row1(1.0, 2.0, 3.0);
        const Data::Vector3Type row2(5.0, 16.5, 21.2);
        const Data::Vector3Type row3(-44.4, -72.1, 72.4);
        auto source = Data::Matrix3x3Type::CreateFromRows(row1, row2, row3);
        auto output = TestMathFunction<GetRowsNode>(
        { "Matrix3x3: Source" },
        { Datum(source) },
        { "Row1: Vector3", "Row2: Vector3", "Row3: Vector3" },
        { Datum(Data::Vector3Type::CreateZero()), Datum(Data::Vector3Type::CreateZero()), Datum(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(row1.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(row2.IsClose(*output[1].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(row3.IsClose(*output[2].GetAs<Data::Vector3Type>()));
    }

    { // ToScale
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 1.0, -6.0),
            Data::Vector3Type(7.0, -8.0, 1.0));
        auto result(source.RetrieveScale());
        auto output = TestMathFunction<ToScaleNode>({ "Matrix3x3: Source", }, { Datum(source) }, { "Result: Vector3" }, { Datum(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Transpose
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        auto result = source.GetTranspose();
        auto output = TestMathFunction<TransposeNode>({ "Matrix3x3: Source" }, { Datum(source) }, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // Zero
        auto result = Data::Matrix3x3Type::CreateZero();
        auto output = TestMathFunction<ZeroNode>({}, {}, { "Result: Matrix3x3" }, { Datum(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }
}

TEST_F(ScriptCanvasTestFixture, Matrix4x4Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Matrix4x4Nodes;

    { // FromColumns
        auto col1 = Data::Vector4Type(1.0, 2.0, 3.0, 4.0);
        auto col2 = Data::Vector4Type(5.0, 6.0, 7.0, 8.0);
        auto col3 = Data::Vector4Type(9.0, 10.0, 11.0, 12.0);
        auto col4 = Data::Vector4Type(13.0, 15.0, 15.0, 16.0);
        auto result(Data::Matrix4x4Type::CreateFromColumns(col1, col2, col3, col4));
        auto output = TestMathFunction<FromColumnsNode>({ "Vector4: Column1", "Vector4: Column2", "Vector4: Column3", "Vector4: Column4" },
        { Datum(col1), Datum(col2), Datum(col3), Datum(col4) },
        { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromDiagonal
        auto source = Data::Vector4Type(1.0, 1.0, 1.0, 1.0);
        auto result(Data::Matrix4x4Type::CreateDiagonal(source));
        auto output = TestMathFunction<FromDiagonalNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromMatrix3x3
        auto row1 = Data::Vector3Type(1.0, 2.0, 3.0);
        auto row2 = Data::Vector3Type(4.0, 5.0, 6.0);
        auto row3 = Data::Vector3Type(7.0, 8.0, 9.0);
        auto source = Data::Matrix3x3Type::CreateFromRows(row1, row2, row3);
        auto result(Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type::CreateFromVector3(source.GetRow(0)),
            Data::Vector4Type::CreateFromVector3(source.GetRow(1)),
            Data::Vector4Type::CreateFromVector3(source.GetRow(2)),
            Data::Vector4Type(0.0, 0.0, 0.0, 1.0)));
        auto output = TestMathFunction<FromMatrix3x3Node>({ "Matrix3x3: Source" }, { Datum(source) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromQuaternion
        auto rotation = Data::QuaternionType(1.0, 2.0, 3.0, 4.0);
        auto result(Data::Matrix4x4Type::CreateFromQuaternion(rotation));
        auto output = TestMathFunction<FromQuaternionNode>({ "Quaternion: Source" }, { Datum(rotation) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromQuaternionAndTranslation
        auto rotation = Data::QuaternionType(1.0, 2.0, 3.0, 4.0);
        auto translation = Data::Vector3Type(10.0, -4.5, -16.10);
        auto result = Data::Matrix4x4Type::CreateFromQuaternionAndTranslation(rotation, translation);
        auto output = TestMathFunction<FromQuaternionAndTranslationNode>({ "Quaternion: Rotation", "Vector3: Translation" }, { Datum(rotation), Datum(translation) },
        { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // FromProjection
        const auto verticalFov = Data::NumberType(4.0 / 3.0 * AZ::Constants::HalfPi);
        const auto aspectRatio = Data::NumberType(16.0 / 9.0);
        const auto nearDist = Data::NumberType(0.1);
        const auto farDist = Data::NumberType(10.0);
        auto result = Data::Matrix4x4Type::CreateProjection(static_cast<float>(verticalFov), static_cast<float>(aspectRatio), static_cast<float>(nearDist), static_cast<float>(farDist));
        auto output = TestMathFunction<FromProjectionNode>({ "Number: Vertical FOV", "Number: Aspect Ratio", "Number: Near", "Number: Far" },
        { Datum(verticalFov), Datum(aspectRatio), Datum(nearDist), Datum(farDist) },
        { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromProjectionFov
        const auto verticalFov = Data::NumberType(4.0 / 3.0 * AZ::Constants::HalfPi);
        const auto horizontalFov = Data::NumberType(5.0 / 6.0 * AZ::Constants::HalfPi);
        const auto nearDist = Data::NumberType(0.1);
        const auto farDist = Data::NumberType(10.0);
        auto result = Data::Matrix4x4Type::CreateProjectionFov(static_cast<float>(verticalFov), static_cast<float>(horizontalFov), static_cast<float>(nearDist), static_cast<float>(farDist));
        auto output = TestMathFunction<FromProjectionFovNode>({ "Number: Vertical FOV", "Number: Horizontal FOV", "Number: Near", "Number: Far" },
        { Datum(verticalFov), Datum(horizontalFov), Datum(nearDist), Datum(farDist) },
        { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromProjectionVolume
        const auto left = Data::NumberType(-10.0);
        const auto right = Data::NumberType(10.0);
        const auto bottom = Data::NumberType(0.0);
        const auto top = Data::NumberType(12.0);
        const auto nearDist = Data::NumberType(0.1);
        const auto farDist = Data::NumberType(10.0);
        auto result = Data::Matrix4x4Type::CreateProjectionOffset(static_cast<float>(left), static_cast<float>(right), static_cast<float>(bottom), static_cast<float>(top), static_cast<float>(nearDist), static_cast<float>(farDist));
        auto output = TestMathFunction<FromProjectionVolumeNode>({ "Number: Left", "Number: Right", "Number: Bottom", "Number: Top", "Number: Near", "Number: Far" },
        { Datum(left), Datum(right), Datum(bottom), Datum(top),  Datum(nearDist), Datum(farDist) },
        { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }
#endif

    { // FromRotationXDegrees
        auto degrees = 45.0f;
        auto resultDegrees(Data::Matrix4x4Type::CreateRotationX(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationXDegreesNode>({ "Number: Degrees" }, { Datum(degrees) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromRotationYDegrees
        auto degrees = 30.0f;
        auto resultDegrees(Data::Matrix4x4Type::CreateRotationY(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationYDegreesNode>({ "Number: Degrees" }, { Datum(degrees) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromRotationZDegrees
        auto degrees = 60.0f;
        auto resultDegrees(Data::Matrix4x4Type::CreateRotationZ(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationZDegreesNode>({ "Number: Degrees" }, { Datum(degrees) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromRows
        auto row1 = Data::Vector4Type(1.0, 2.0, 3.0, 4.0);
        auto row2 = Data::Vector4Type(5.0, 6.0, 7.0, 8.0);
        auto row3 = Data::Vector4Type(9.0, 10.0, 11.0, 12.0);
        auto row4 = Data::Vector4Type(13.0, 15.0, 15.0, 16.0);
        auto result(Data::Matrix4x4Type::CreateFromRows(row1, row2, row3, row4));
        auto output = TestMathFunction<FromRowsNode>({ "Vector4: Row1", "Vector4: Row2", "Vector4: Row3", "Vector4: Row4" },
        { Datum(row1), Datum(row2), Datum(row3), Datum(row4) },
        { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromScale
        auto scale = Data::Vector3Type(2.0, 2.0, 2.0);
        auto result(Data::Matrix4x4Type::CreateScale(scale));
        auto output = TestMathFunction<FromScaleNode>({ "Vector3: Scale" }, { Datum(scale) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromTransform
        auto transform = Data::TransformType::CreateIdentity();
        auto result = Data::Matrix4x4Type::CreateFromTransform(transform);
        auto output = TestMathFunction<FromTransformNode>({ "Transform: Transform" }, { Datum(transform) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromTranslation
        auto source = Data::Vector3Type(1.0, -1.0, 15.0);
        auto result(Data::Matrix4x4Type::CreateTranslation(source));
        auto output = TestMathFunction<FromTranslationNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // Invert
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 21.2),
            Data::Vector4Type(4.0, 1.0, 26.0, -45.8),
            Data::Vector4Type(7.0, -8.0, 1.0, 73.3),
            Data::Vector4Type(27.5, 36.8, 0.8, 14.0));
        auto result(source.GetInverseFull());
        auto output = TestMathFunction<InvertNode>({ "Matrix4x4: Source", }, { Datum(source) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // IsCloseNode
        auto a(Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type::CreateOne(), Data::Vector4Type::CreateOne(), Data::Vector4Type::CreateOne(), Data::Vector4Type::CreateOne()));
        auto b(Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type(2.0, 2.0, 2.0, 2.0), Data::Vector4Type(2.0, 2.0, 2.0, 2.0), Data::Vector4Type(2.0, 2.0, 2.0, 2.0), Data::Vector4Type(2.0, 2.0, 2.0, 2.0)));
        auto c(Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type(1.0001, 1.0001, 1.0001, 1.0001), Data::Vector4Type(1.0001, 1.0001, 1.0001, 1.0001), Data::Vector4Type(1.0001, 1.0001, 1.0001, 1.0001), Data::Vector4Type(1.0001, 1.0001, 1.0001, 1.0001)));

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Matrix4x4: A", "Matrix4x4: B" }, { Datum(a), Datum(b) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, 2.1f);
        output = TestMathFunction<IsCloseNode>({ "Matrix4x4: A", "Matrix4x4: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(2.1) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Matrix4x4: A", "Matrix4x4: B" }, { Datum(a), Datum(c) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, 0.9f);
        output = TestMathFunction<IsCloseNode>({ "Matrix4x4: A", "Matrix4x4: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(0.9) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 21.2),
            Data::Vector4Type(4.0, 1.0, 26.0, -45.8),
            Data::Vector4Type(7.0, -8.0, std::numeric_limits<float>::infinity(), 73.3),
            Data::Vector4Type(27.5, 36.8, 0.8, 14.0));
        auto result = source.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Matrix4x4: Source" }, { Datum(source) }, { "Result: Boolean", }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    {  // MultiplyByMatrix
        auto a(Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 17.98)));
        auto b(Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(-1.0, -2.0, 13.0, -0.0),
            Data::Vector4Type(14.0, 15.0, -6.0, +0.0),
            Data::Vector4Type(17.0, -8.0, 19.0, 1.0),
            Data::Vector4Type(4.1, -7.6, -11.3, 1.0)));
        auto result = a * b;
        auto output = TestMathFunction<MultiplyByMatrixNode>({ "Matrix4x4: A", "Matrix4x4: B" }, { Datum(a), Datum(b) },
        { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    {  // MultiplyByVector
        auto source(Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 17.98)));
        Data::Vector4Type vector4(3.0);
        auto result = source * vector4;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Matrix4x4: Source", "Vector4: Vector" }, { Datum(source), Datum(vector4) },
        { "Result: Vector4" }, { Datum(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetColumn
        auto source = Data::Matrix4x4Type::CreateFromColumns(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 40.0));
        auto result(source.GetColumn(3));
        auto output = TestMathFunction<GetColumnNode>({ "Matrix4x4: Source", "Number: Column" }, { Datum(source), Datum(3) },
        { "Result: Vector4" }, { Datum(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetColumns
        const Data::Vector4Type col1(1.0, 2.0, 3.0, 5.0);
        const Data::Vector4Type col2(5.0, 16.5, 21.2, -1.2);
        const Data::Vector4Type col3(-44.4, -72.1, 72.4, 6.5);
        const Data::Vector4Type col4(2.7, 17.65, 2.3, 13.3);
        auto source = Data::Matrix4x4Type::CreateFromColumns(col1, col2, col3, col4);
        auto output = TestMathFunction<GetColumnsNode>(
        { "Matrix4x4: Source" },
        { Datum(source) },
        { "Column1: Vector4", "Column2: Vector4", "Column3: Vector4", "Column4: Vector4" },
        { Datum(Data::Vector4Type::CreateZero()), Datum(Data::Vector4Type::CreateZero()), Datum(Data::Vector4Type::CreateZero()), Datum(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(col1.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(col2.IsClose(*output[1].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(col3.IsClose(*output[2].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(col4.IsClose(*output[3].GetAs<Data::Vector4Type>()));
    }

    { // GetDiagonal
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(10.0, 1.0, 2.0, 3.0),
            Data::Vector4Type(24.0, 4.0, 5.0, 6.0),
            Data::Vector4Type(40.0, 7.0, 8.0, 9.0),
            Data::Vector4Type(30.0, 10.0, 11.0, 12.0));
        auto result(source.GetDiagonal());
        auto output = TestMathFunction<GetDiagonalNode>({ "Matrix4x4: Source", }, { Datum(source) }, { "Result: Vector4" }, { Datum(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetElement
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 40.0));
        auto result(source.GetElement(1, 3));
        auto outputSuccess = TestMathFunction<GetElementNode>({ "Matrix4x4: Source", "Number: Row", "Number: Column" }, { Datum(source), Datum(1), Datum(3) },
        { "Result: Number" }, { Datum(Data::NumberType()) });
        auto outputFailure = TestMathFunction<GetElementNode>({ "Matrix4x4: Source", "Number: Row", "Number: Column" }, { Datum(source), Datum(2), Datum(2) },
        { "Result: Number" }, { Datum(Data::NumberType()) });
        EXPECT_TRUE(AZ::IsClose(result, static_cast<float>(*outputSuccess[0].GetAs<Data::NumberType>())));
        EXPECT_FALSE(AZ::IsClose(result, static_cast<float>(*outputFailure[0].GetAs<Data::NumberType>())));
    }

    { // GetRow
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 40.0));
        auto result(source.GetRow(0));
        auto output = TestMathFunction<GetRowNode>({ "Matrix4x4: Source", "Number: Row" }, { Datum(source), Datum(0) },
        { "Result: Vector4" }, { Datum(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetRows
        const Data::Vector4Type row1(1.0, 2.0, 3.0, 5.0);
        const Data::Vector4Type row2(5.0, 16.5, 21.2, -1.2);
        const Data::Vector4Type row3(-44.4, -72.1, 72.4, 6.5);
        const Data::Vector4Type row4(2.7, 17.65, 2.3, 13.3);
        auto source = Data::Matrix4x4Type::CreateFromRows(row1, row2, row3, row4);
        auto output = TestMathFunction<GetRowsNode>(
        { "Matrix4x4: Source" },
        { Datum(source) },
        { "Row1: Vector4", "Row2: Vector4", "Row3: Vector4", "Row4: Vector4" },
        { Datum(Data::Vector4Type::CreateZero()), Datum(Data::Vector4Type::CreateZero()), Datum(Data::Vector4Type::CreateZero()), Datum(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(row1.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(row2.IsClose(*output[1].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(row3.IsClose(*output[2].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(row4.IsClose(*output[3].GetAs<Data::Vector4Type>()));
    }

    { // ToScale
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 4.5),
            Data::Vector4Type(4.0, 1.0, -6.0, -1.0),
            Data::Vector4Type(7.0, -8.0, 1.0, 1.0),
            Data::Vector4Type(0.0, 0.0, 0.0, 1.0));
        auto result(source.RetrieveScale());
        auto output = TestMathFunction<ToScaleNode>({ "Matrix4x4: Source", }, { Datum(source) }, { "Result: Vector3" }, { Datum(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Transpose
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 0.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 6.0),
            Data::Vector4Type(7.0, 8.0, 9.0, -10.0),
            Data::Vector4Type(5.0, 8.0, 3.0, 1.0));
        auto result = source.GetTranspose();
        auto output = TestMathFunction<TransposeNode>({ "Matrix4x4: Source" }, { Datum(source) }, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // Zero
        auto result = Data::Matrix4x4Type::CreateZero();
        auto output = TestMathFunction<ZeroNode>({}, {}, { "Result: Matrix4x4" }, { Datum(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }
}

TEST_F(ScriptCanvasTestFixture, OBBNodes)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
    using namespace ScriptCanvas::OBBNodes;
    using namespace ScriptCanvas::Data;

    const Vector3Type outsideMax(222);
    const Vector3Type min3(-111, -111, -111);
    const Vector3Type max3(111, 111, 111);

    const Vector3Type minHalf3(min3 * 0.5f);
    const Vector3Type maxHalf3(max3 * 0.5f);

    auto obbRotated = OBBType::CreateFromPositionRotationAndHalfLengths(Vector3Type(10, 20, 30), QuaternionType::CreateIdentity(), Vector3Type(10, 30, 20));
    auto transform = TransformType::CreateFromQuaternion(QuaternionType::CreateFromAxisAngle(Vector3Type(1, 1, 1).GetNormalized(), AZ::DegToRad(30)));
    obbRotated = transform * obbRotated;

    auto IsClose = [](const OBBType& lhs, const OBBType& rhs)->bool
    {
        return lhs.GetAxisX().IsClose(rhs.GetAxisX())
            && lhs.GetAxisY().IsClose(rhs.GetAxisY())
            && lhs.GetAxisZ().IsClose(rhs.GetAxisZ())
            && lhs.GetPosition().IsClose(rhs.GetPosition());
    };

    {// FromAabbNode
        auto source = AABBType::CreateFromMinMax(Vector3Type(-1, -2, -3), Vector3Type(1, 2, 3));
        auto output = TestMathFunction<FromAabbNode>({ "AABB: Source" }, { Datum(source) }, { "Result: OBB" }, { Datum(OBBType()) });
        auto result = OBBType::CreateFromAabb(source);
        EXPECT_TRUE(IsClose(result, (*output[0].GetAs<OBBType>())));
    }

    {// FromPositionAndAxesNode
        auto output = TestMathFunction<FromPositionRotationAndHalfLengthsNode>
            ({ "Vector3: Position", "Quaternion: Rotation", "Vector3: HalfLengths" }
                , { Datum(obbRotated.GetPosition()), Datum(obbRotated.GetRotation()), Datum(obbRotated.GetHalfLengths()) }
                , { "Result: OBB" }
        , { Datum(OBBType()) });
        auto result = obbRotated;
        EXPECT_TRUE(IsClose(result, (*output[0].GetAs<OBBType>())));
    }

    {// GetAxisXNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetAxisXNode>({ "OBB: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        auto result = source.GetAxisX();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    {// GetAxisYNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetAxisYNode>({ "OBB: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        auto result = source.GetAxisY();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    {// GetAxisZNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetAxisZNode>({ "OBB: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        auto result = source.GetAxisZ();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {// GetHalfLengthXNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetHalfLengthXNode>({ "OBB: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(NumberType()) });
        float result = source.GetHalfLengthX();
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    {// GetHalfLengthYNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetHalfLengthYNode>({ "OBB: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(NumberType()) });
        float result = source.GetHalfLengthY();
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    {// GetHalfLengthZNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetHalfLengthZNode>({ "OBB: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(NumberType()) });
        float result = source.GetHalfLengthZ();
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }
#endif

    {// GetPositionNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetPositionNode>({ "OBB: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        auto result = obbRotated.GetPosition();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    {// IsFiniteNode
        auto source = obbRotated;
        auto output = TestMathFunction<IsFiniteNode>({ "OBB: Source" }, { Datum(source) }, { "Result: Boolean" }, { Datum(BooleanType()) });
        auto result = source.IsFinite();
        EXPECT_TRUE(result);
        EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
    }

}

TEST_F(ScriptCanvasTestFixture, PlaneNodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::PlaneNodes;
    using namespace ScriptCanvas::Data;

    const PlaneType source = PlaneType::CreateFromNormalAndDistance(Vector3Type::CreateOne().GetNormalized(), 12.0);
    const Vector4Type sourceVec4 = source.GetPlaneEquationCoefficients();

    const Vector3Type point(33, 66, 99);
    const Vector3Type normal(Vector3Type(1, -2, 3).GetNormalized());
    const float A = normal.GetX();
    const float B = normal.GetY();
    const float C = normal.GetZ();
    const float D = 13.0f;

    auto IsClose = [](PlaneType lhs, PlaneType rhs) { return lhs.GetPlaneEquationCoefficients().IsClose(rhs.GetPlaneEquationCoefficients()); };

    { // CreateFromCoefficientsNode
        auto result = PlaneType::CreateFromCoefficients(A, B, C, D);
        auto output = TestMathFunction<FromCoefficientsNode>({ "Number: A", "Number: B", "Number: C", "Number: D" }, { Datum(A),Datum(B),Datum(C),Datum(D) }, { "Result: Plane" }, { Datum(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }

    { // CreateFromNormalAndDistanceNode
        auto result = PlaneType::CreateFromNormalAndDistance(normal, D);
        auto output = TestMathFunction<FromNormalAndDistanceNode>({ "Vector3: Normal", "Number: Distance" }, { Datum(normal), Datum(D) }, { "Result: Plane" }, { Datum(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }

    { // CreateFromNormalAndPointNode
        auto result = PlaneType::CreateFromNormalAndPoint(normal, point);
        auto output = TestMathFunction<FromNormalAndPointNode>({ "Vector3: Normal", "Vector3: Point" }, { Datum(normal), Datum(point) }, { "Result: Plane" }, { Datum(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }

    { // DistanceToPointNode
        auto result = source.GetPointDist(point);
        auto output = TestMathFunction<DistanceToPointNode>({ "Plane: Source", "Vector3: Point" }, { Datum(source), Datum(point) }, { "Result: Number" }, { Datum(-1.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // IsFiniteNode
        auto result = source.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Plane: Source" }, { Datum(source) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_TRUE(result);
        EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // ModDistanceNode
        auto result = source;
        result.SetDistance(D);
        auto output = TestMathFunction<ModDistanceNode>({ "Plane: Source", "Number: Distance" }, { Datum(source), Datum(D) }, { "Result: Plane" }, { Datum(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }

    { // ModNormalNode
        auto result = source;
        result.SetNormal(normal);
        auto output = TestMathFunction<ModNormalNode>({ "Plane: Source", "Vector3: Normal" }, { Datum(source), Datum(normal) }, { "Result: Plane" }, { Datum(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }
#endif

    { // GetDistanceNode
        auto result = source.GetDistance();
        auto output = TestMathFunction<GetDistanceNode>({ "Plane: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(-1) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // GetNormalNode
        auto result = source.GetNormal();
        auto output = TestMathFunction<GetNormalNode>({ "Plane: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(Vector3Type(0, 0, 0)) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // GetPlaneEquationCoefficientsNode
        auto result = source.GetPlaneEquationCoefficients();
        auto output = TestMathFunction<GetPlaneEquationCoefficientsNode>({ "Plane: Source" }, { Datum(source) }, { "A: Number", "B: Number", "C: Number", "D: Number" }, { Datum(0),Datum(0),Datum(0),Datum(0) });

        for (int i = 0; i < 4; ++i)
        {
            SC_EXPECT_FLOAT_EQ(result.GetElement(i), aznumeric_caster(*output[i].GetAs<Data::NumberType>()));
        }
    }

    { // GetProjectedNode
        auto result = source.GetProjected(point);
        auto output = TestMathFunction<ProjectNode>({ "Plane: Source", "Vector3: Point" }, { Datum(source), Datum(point) }, { "Result: Vector3" }, { Datum(Vector3Type()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // TransformNode
        TransformType transform = TransformType::CreateFromQuaternionAndTranslation(QuaternionType::CreateFromAxisAngle(Vector3Type(1, 1, 1).GetNormalized(), AZ::DegToRad(30)), Vector3Type(-100, 50, -25));
        auto result = source.GetTransform(transform);
        auto output = TestMathFunction<TransformNode>({ "Plane: Source", "Transform: Transform" }, { Datum(source), Datum(transform) }, { "Result: Plane" }, { Datum(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }
}

TEST_F(ScriptCanvasTestFixture, TransformNodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::TransformNodes;
    using namespace ScriptCanvas::Data;

    const Vector3Type vector3Zero(0, 0, 0);
    const Vector3Type zero3(0, 0, 0);
    const Vector3Type vector3One(1, 1, 1);
    const Vector3Type xPos(1, 0, 0);
    const Vector3Type yPos(0, 1, 0);
    const Vector3Type zPos(0, 0, 1);
    const Vector3Type position(-10.1, 0.1, 10.1);
    const Vector3Type scale(-.66, .33, .66);

    const Vector4Type r0(1, 0, 0, -.5);
    const Vector4Type r1(0, 1, 0, 0.0);
    const Vector4Type r2(0, 0, 1, 0.5);
    const Vector4Type zero4(0, 0, 0, 0);

    const Matrix3x3Type matrix3x3One(Matrix3x3Type::CreateFromValue(1));

    const QuaternionType rotationOne(QuaternionType::CreateRotationZ(1));

    const TransformType identity(TransformType::CreateIdentity());
    const TransformType invertable(TransformType::CreateFromQuaternionAndTranslation(rotationOne, position));
    const TransformType notOrthogonal(TransformType::CreateScale(Vector3Type(3, 4, 5)));

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // ExtractScale
        auto source = TransformType::CreateScale(Vector3Type(-0.5f, .5f, 1.5f));
        auto extracted = source;
        auto scale = extracted.ExtractScale();
        auto output = TestMathFunction<ExtractScaleNode>({ "Transform: Source" }, { Datum(source) }, { "Scale: Vector3", "Extracted: Transform" }, { Datum(vector3Zero), Datum(identity) });

        auto scaleOutput = *output[0].GetAs<Vector3Type>();
        auto extractedOutput = *output[1].GetAs<TransformType>();
        EXPECT_TRUE(scale.IsClose(scaleOutput));
        EXPECT_TRUE(extracted.IsClose(extractedOutput));
    }
#endif

    { // FromMatrix3x3Node
        auto output = TestMathFunction<FromMatrix3x3Node>({ "Matrix3x3: Source" }, { Datum(matrix3x3One) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateFromMatrix3x3(matrix3x3One);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromMatrix3x3AndTranslationNode
        auto output = TestMathFunction<FromMatrix3x3AndTranslationNode>({ "Matrix3x3: Matrix", "Vector3: Translation" }, { Datum(matrix3x3One), Datum(position) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateFromMatrix3x3AndTranslation(matrix3x3One, position);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromRotationNode
        auto output = TestMathFunction<FromRotationNode>({ "Quaternion: Source" }, { Datum(rotationOne) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateFromQuaternion(rotationOne);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromRotationAndTranslationNode
        auto output = TestMathFunction<FromRotationAndTranslationNode>({ "Quaternion: Rotation", "Vector3: Translation" }, { Datum(rotationOne), Datum(position) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateFromQuaternionAndTranslation(rotationOne, position);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromScaleNode
        auto output = TestMathFunction<FromScaleNode>({ "Vector3: Scale" }, { Datum(scale) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateScale(scale);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromTranslationNode
        auto output = TestMathFunction<FromTranslationNode>({ "Vector3: Translation" }, { Datum(position) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateTranslation(position);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // InvertOrthogonalNode
        auto output = TestMathFunction<InvertOrthogonalNode>({ "Transform: Source" }, { Datum(invertable) }, { "Result: Transform" }, { Datum(identity) });
        auto result = invertable.GetInverse();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }
#endif

    { // InvertSlowNode
        auto output = TestMathFunction<InvertSlowNode>({ "Transform: Source" }, { Datum(invertable) }, { "Result: Transform" }, { Datum(identity) });
        auto result = invertable.GetInverse();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // IsCloseNode
        auto outputFalse = TestMathFunction<IsCloseNode>({ "Transform: A", "Transform: B" }, { Datum(invertable), Datum(identity) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(invertable.IsClose(identity), *outputFalse[0].GetAs<BooleanType>());
        EXPECT_FALSE(invertable.IsClose(identity));
        auto outputTrue = TestMathFunction<IsCloseNode>({ "Transform: A", "Transform: B" }, { Datum(invertable), Datum(invertable) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(invertable.IsClose(invertable), *outputTrue[0].GetAs<BooleanType>());
        EXPECT_TRUE(invertable.IsClose(invertable));
    }

    { // IsFiniteNode
        auto output = TestMathFunction<IsFiniteNode>({ "Transform: Source" }, { Datum(identity) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(identity.IsFinite(), *output[0].GetAs<BooleanType>());
        EXPECT_TRUE(identity.IsFinite());
    }

    { // IsOrthogonalNode
        auto outputTrue = TestMathFunction<IsOrthogonalNode>({ "Transform: Source" }, { Datum(identity) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(identity.IsOrthogonal(), *outputTrue[0].GetAs<BooleanType>());
        EXPECT_TRUE(identity.IsOrthogonal());
        auto outputFalse = TestMathFunction<IsOrthogonalNode>({ "Transform: Source" }, { Datum(notOrthogonal) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(notOrthogonal.IsOrthogonal(), *outputFalse[0].GetAs<BooleanType>());
        EXPECT_FALSE(notOrthogonal.IsOrthogonal());
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // Multiply3x3ByVector3Node
        auto output = TestMathFunction<Multiply3x3ByVector3Node>({ "Transform: Source", "Vector3: Multiplier" }, { Datum(notOrthogonal), Datum(scale) }, { "Result: Vector3" }, { Datum(zero3) });
        auto result = notOrthogonal.Multiply3x3(scale);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }
#endif

    { // MultiplyByScaleNode
        auto output = TestMathFunction<MultiplyByScaleNode>({ "Transform: Source", "Vector3: Scale" }, { Datum(notOrthogonal), Datum(scale) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType(notOrthogonal);
        result.MultiplyByScale(scale);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // MultiplyByTransformNode
        auto output = TestMathFunction<MultiplyByTransformNode>({ "Transform: A", "Transform: B" }, { Datum(notOrthogonal), Datum(notOrthogonal) }, { "Result: Transform" }, { Datum(identity) });
        auto result = notOrthogonal * notOrthogonal;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // MultiplyByVector3Node
        auto output = TestMathFunction<MultiplyByVector3Node>({ "Transform: Source", "Vector3: Multiplier" }, { Datum(notOrthogonal), Datum(scale) }, { "Result: Vector3" }, { Datum(zero3) });
        auto result = notOrthogonal.TransformPoint(scale);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // MultiplyByVector4Node
        const Vector4Type multiplier(4, 3, 2, 7);
        auto output = TestMathFunction<MultiplyByVector4Node>({ "Transform: Source", "Vector4: Multiplier" }, { Datum(notOrthogonal), Datum(multiplier) }, { "Result: Vector4" }, { Datum(zero4) });
        auto result = notOrthogonal.TransformPoint(multiplier);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector4Type>()));
    }

    { // OrthogonalizeNode
        TransformType nearlyOrthogonal = TransformType::CreateScale(Vector3Type(1.0f, 1.0f, 1.01f));

        auto orthogonalResult = nearlyOrthogonal.GetOrthogonalized();
        auto orthogonalOutput = *TestMathFunction<OrthogonalizeNode>({ "Transform: Source" }, { Datum(nearlyOrthogonal) }, { "Result: Transform" }, { Datum(notOrthogonal) })[0].GetAs<TransformType>();
        EXPECT_TRUE(orthogonalResult.IsClose(orthogonalOutput));
        EXPECT_TRUE(orthogonalResult.IsOrthogonal());
        EXPECT_TRUE(orthogonalOutput.IsOrthogonal());
    }

    { // RotationXDegreesNode
        auto output = TestMathFunction<RotationXDegreesNode>({ "Number: Degrees" }, { Datum(30) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateRotationX(AZ::DegToRad(30));
        auto outputTM = *output[0].GetAs<TransformType>();
        EXPECT_TRUE(result.IsClose(outputTM));
    }

    { // RotationYDegreesNode
        auto output = TestMathFunction<RotationYDegreesNode>({ "Number: Degrees" }, { Datum(30) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateRotationY(AZ::DegToRad(30));
        auto outputTM = *output[0].GetAs<TransformType>();
        EXPECT_TRUE(result.IsClose(outputTM));
    }

    { // RotationZDegreesNode
        auto output = TestMathFunction<RotationZDegreesNode>({ "Number: Degrees" }, { Datum(30) }, { "Result: Transform" }, { Datum(identity) });
        auto result = TransformType::CreateRotationZ(AZ::DegToRad(30));
        auto outputTM = *output[0].GetAs<TransformType>();
        EXPECT_TRUE(result.IsClose(outputTM));
    }

    { // ToScaleNode
        auto output = TestMathFunction<ToScaleNode>({ "Transform: Source" }, { Datum(notOrthogonal) }, { "Result: Vector3" }, { Datum(zero3) });
        auto result = notOrthogonal.GetScale();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // GetTranslationNode
        auto output = TestMathFunction<GetTranslationNode>({ "Transform: Source" }, { Datum(notOrthogonal) }, { "Result: Vector3" }, { Datum(zero3) });
        auto result = notOrthogonal.GetTranslation();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }
} // Transform Test

TEST_F(ScriptCanvasTestFixture, Vector2Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Vector2Nodes;

    Data::Vector2Type zero(0, 0);
    Data::Vector2Type one(1.f, 1.f);
    Data::Vector2Type negativeOne(-1.f, -1.f);

    {   // Absolute
        Data::Vector2Type source(-1, -1);
        Data::Vector2Type absolute = source.GetAbs();
        auto output = TestMathFunction<AbsoluteNode>({ "Vector2: Source" }, { Datum(source) }, { "Result: Vector2" }, { Datum(source) });
        EXPECT_TRUE(absolute.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // Add
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(1, 1);
        Data::Vector2Type c = a + b;
        auto output = TestMathFunction<AddNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // Clamp
        Data::Vector2Type source(-10, 20);
        Data::Vector2Type min(1, 1);
        Data::Vector2Type max(1, 1);
        auto result = source.GetClamp(min, max);
        auto output = TestMathFunction<ClampNode>({ "Vector2: Source", "Vector2: Min", "Vector2: Max" }, { Datum(source), Datum(min), Datum(max), }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // Distance
        Data::Vector2Type a(1, 2);
        Data::Vector2Type b(-3, -2);
        auto result = a.GetDistance(b);
        auto output = TestMathFunction<DistanceNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // DistanceSqr
        Data::Vector2Type a(1, 2);
        Data::Vector2Type b(-3, -2);
        auto result = a.GetDistanceSq(b);
        auto output = TestMathFunction<DistanceSquaredNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    {   // DivideByNumber
        Data::Vector2Type a(1, 2);
        Data::NumberType b(3.0);
        auto result = a / static_cast<float>(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Vector2: Source", "Number: Divisor" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {  // DivideByVector
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(2, 2);
        Data::Vector2Type c = a / b;
        auto output = TestMathFunction<DivideByVectorNode>({ "Vector2: Numerator", "Vector2: Divisor" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Dot
        Data::Vector2Type a(1, 2);
        Data::Vector2Type b(-3, -2);
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // From Element
        const Data::Vector2Type source(1, 2);

        for (int index = 0; index < 2; ++index)
        {
            auto result = source;
            result.SetElement(index, 4.0f);
            auto output = TestMathFunction<FromElementNode>({ "Vector2: Source", "Number: Index", "Number: Value" }, { Datum(source), Datum(index), Datum(4.0) }, { "Result: Vector2" }, { Datum(zero) });
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
        }
    }    

    { // FromLength
        Data::Vector2Type source(1, 2);
        Data::NumberType length(10);
        auto result = source;
        result.SetLength(static_cast<float>(length));
        auto output = TestMathFunction<FromLengthNode>({ "Vector2: Source", "Number: Length" }, { Datum(source), Datum(length) }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }
#endif

    { // FromValues
        Data::NumberType x(1);
        Data::NumberType y(2);
        auto result = Data::Vector2Type(static_cast<float>(x), static_cast<float>(y));
        auto output = TestMathFunction<FromValuesNode>({ "Number: X", "Number: Y" }, { Datum(x), Datum(y) }, { "Result: Vector2", }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // GetElements
        auto source = Vector2Type(1, 2);
        auto output = TestMathFunction<GetElementsNode>({ "Vector2: Source" }, { Datum(source) }, { "X: Number", "Y: Number" }, { Datum(0), Datum(0) });
        SC_EXPECT_FLOAT_EQ(source.GetX(), aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
        SC_EXPECT_FLOAT_EQ(source.GetY(), aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // IsClose
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(2, 2);
        Data::Vector2Type c(1.09, 1.09);

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, 2.1f);
        output = TestMathFunction<IsCloseNode>({ "Vector2: A", "Vector2: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(2.1) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(c) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, 0.9f);
        output = TestMathFunction<IsCloseNode>({ "Vector2: A", "Vector2: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(0.9) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        Data::Vector2Type sourceFinite(1, 1);
        auto result = sourceFinite.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Vector2: Source" }, { Datum(sourceFinite) }, { "Result: Boolean", }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsNormal
        Data::Vector2Type normal(1.f, 0.f);
        Data::Vector2Type nearlyNormal(1.0001f, 0.0f);
        Data::Vector2Type notNormal(10.f, 0.f);

        {
            auto result = normal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source" }, { Datum(normal) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = nearlyNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source" }, { Datum(nearlyNormal) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = nearlyNormal.IsNormalized(2.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source", "Number: Tolerance" }, { Datum(nearlyNormal), Datum(2.0f) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = notNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source" }, { Datum(notNormal) }, { "Result: Boolean" }, { Datum(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = notNormal.IsNormalized(12.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source", "Number: Tolerance" }, { Datum(notNormal), Datum(12.0f) }, { "Result: Boolean" }, { Datum(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }
    }

    { // IsZero
        auto result = zero.IsZero();
        auto output = TestMathFunction<IsZeroNode>({ "Vector2: Source" }, { Datum(zero) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = one.IsZero();
        output = TestMathFunction<IsZeroNode>({ "Vector2: Source" }, { Datum(one) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // Length
        Data::Vector2Type source(1, 2);
        auto result = source.GetLength();
        auto output = TestMathFunction<LengthNode>({ "Vector2: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthSquared
        Data::Vector2Type source(1, 2);
        auto result = source.GetLengthSq();
        auto output = TestMathFunction<LengthSquaredNode>({ "Vector2: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // Lerp
        Data::Vector2Type from(0.f, 0.f);
        Data::Vector2Type to(1.f, 1.f);
        Data::NumberType t(0.5);

        auto result = from.Lerp(to, static_cast<float>(t));
        auto output = TestMathFunction<LerpNode>({ "Vector2: From", "Vector2: To", "Number: T" }, { Datum(from), Datum(to), Datum(t) }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Max
        Data::Vector2Type a(-1, -1);
        Data::Vector2Type b(1, 1);
        auto result = a.GetMax(b);
        auto output = TestMathFunction<MaxNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Min
        Data::Vector2Type a(-1, -1);
        Data::Vector2Type b(1, 1);
        auto result = a.GetMin(b);
        auto output = TestMathFunction<MinNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // ModXNode
        Data::Vector2Type a(1, 2);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetXNode>({ "Vector2: Source", "Number: X" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(zero) });
        auto result = a;
        result.SetX(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModYNode
        Data::Vector2Type a(1, 2);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetYNode>({ "Vector2: Source", "Number: Y" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(zero) });
        auto result = a;
        result.SetY(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // MultiplyAdd
        Data::Vector2Type a(2, 2);
        Data::Vector2Type b(3, 3);
        Data::Vector2Type c(4, 4);
        auto result = a.GetMadd(b, c);
        auto output = TestMathFunction<MultiplyAddNode>({ "Vector2: A", "Vector2: B", "Vector2: C" }, { Datum(a), Datum(b), Datum(c) }, { "Result: Vector2" }, { Datum(a) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }
#endif

    {   // MultiplyByNumber
        Data::Vector2Type a(1, 2);
        Data::NumberType b(3.0);
        auto result = a * static_cast<float>(b);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Vector2: Source", "Number: Multiplier" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {  // MultiplyByVector
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(2, 2);
        Data::Vector2Type c = a * b;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Vector2: Source", "Vector2: Multiplier" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // Negate
        Data::Vector2Type source = one;
        auto output = TestMathFunction<NegateNode>({ "Vector2: Source" }, { Datum(source) }, { "Result: Vector2" }, { Datum(zero) });
        auto result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));

        source = negativeOne;
        output = TestMathFunction<NegateNode>({ "Vector2: Source" }, { Datum(source) }, { "Result: Vector2" }, { Datum(zero) });
        result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Normalize
        Data::Vector2Type source = one;
        auto output = TestMathFunction<NormalizeNode>({ "Vector2: Source" }, { Datum(source) }, { "Result: Vector2" }, { Datum(zero) });
        auto result = source.GetNormalizedSafe();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // NormalizeWithLength
        Data::Vector2Type source = one;
        auto output = TestMathFunction<NormalizeWithLengthNode>({ "Vector2: Source" }, { Datum(source) }, { "Normalized: Vector2", "Length: Number" }, { Datum(zero), Datum(0) });
        auto result = source.NormalizeSafeWithLength();
        EXPECT_TRUE(source.IsClose(*output[0].GetAs<Data::Vector2Type>()));
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // Project
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(-2, -2);
        auto output = TestMathFunction<ProjectNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(a) });
        a.Project(b);
        EXPECT_TRUE(a.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Slerp
        Data::Vector2Type from(1.f, 0.f);
        Data::Vector2Type to(0.f, 1.f);
        Data::NumberType t(0.5);

        auto slerp = from.Slerp(to, static_cast<float>(t));
        auto outputSlerp = TestMathFunction<SlerpNode>({ "Vector2: From", "Vector2: To", "Number: T" }, { Datum(from), Datum(to), Datum(t) }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(slerp.IsClose(*outputSlerp[0].GetAs<Data::Vector2Type>()));

        auto lerp = from.Lerp(to, static_cast<float>(t));
        auto outputLerp = TestMathFunction<LerpNode>({ "Vector2: From", "Vector2: To", "Number: T" }, { Datum(from), Datum(to), Datum(t) }, { "Result: Vector2" }, { Datum(zero) });
        EXPECT_TRUE(lerp.IsClose(*outputLerp[0].GetAs<Data::Vector2Type>()));

        EXPECT_FALSE(lerp.IsClose(slerp));
        EXPECT_FALSE((*outputLerp[0].GetAs<Data::Vector2Type>()).IsClose(*outputSlerp[0].GetAs<Data::Vector2Type>()));
    }

    {   // Subtract
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(2, 2);
        Data::Vector2Type c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Vector2: A", "Vector2: B" }, { Datum(a), Datum(b) }, { "Result: Vector2" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // GetElement
        const Data::Vector2Type source(1, 2);

        for (int index = 0; index < 2; ++index)
        {
            float result = aznumeric_caster(source.GetElement(index));
            auto output = TestMathFunction<GetElementNode>({ "Vector2: Source", "Number: Index" }, { Datum(source), Datum(index) }, { "Result: Number" }, { Datum(0) });
            float outputNumber = aznumeric_caster(*output[0].GetAs<Data::NumberType>());
            SC_EXPECT_FLOAT_EQ(result, outputNumber);
        }
    }

    {   // ToPerpendicular
        Data::Vector2Type source(3, -1);
        auto output = TestMathFunction<ToPerpendicularNode>({ "Vector2: Source" }, { Datum(source) }, { "Result: Vector2" }, { Datum(zero) });
        auto result = source.GetPerpendicular();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

} // Test Vector2Node

TEST_F(ScriptCanvasTestFixture, Vector3Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Vector3Nodes;
    Data::Vector3Type zero(0, 0, 0);
    Data::Vector3Type one(1.f, 1.f, 1.f);
    Data::Vector3Type negativeOne(-1.f, -1.f, -1.f);

    {   // Absolute
        Data::Vector3Type source(-1, -1, -1);
        Data::Vector3Type absolute = source.GetAbs();
        auto output = TestMathFunction<AbsoluteNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(source) });
        EXPECT_TRUE(absolute.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // Add
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(1, 1, 1);
        Data::Vector3Type c = a + b;
        auto output = TestMathFunction<AddNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {   // AngleMod
        Data::Vector3Type source(1, 1, 1);
        Data::Vector3Type result = source.GetAngleMod();
        auto output = TestMathFunction<AngleModNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

    {   // BuildTangentBasis
        Data::Vector3Type source, tangent, bitangent;
        source = Data::Vector3Type(1, 1, 1);
        source.NormalizeSafe();
        source.BuildTangentBasis(tangent, bitangent);
        auto output = TestMathFunction<BuildTangentBasisNode>({ "Vector3: Normal" }, { Datum(source) }, { "Tangent: Vector3", "Bitangent: Vector3" }, { Datum(zero), Datum(zero) });
        EXPECT_TRUE(tangent.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(bitangent.IsClose(*output[1].GetAs<Data::Vector3Type>()));
    }

    {   // Clamp
        Data::Vector3Type source(-10, 20, 1);
        Data::Vector3Type min(1, 1, 1);
        Data::Vector3Type max(1, 1, 1);
        auto result = source.GetClamp(min, max);
        auto output = TestMathFunction<ClampNode>({ "Vector3: Source", "Vector3: Min", "Vector3: Max" }, { Datum(source), Datum(min), Datum(max), }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {   // Cosine
        Data::Vector3Type source(1.5, 1.5, 1.5);
        auto result = source.GetCos();
        auto output = TestMathFunction<CosineNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // CrossXAxis
        Data::Vector3Type source(1, 1, 1);
        auto result = source.CrossXAxis();
        auto output = TestMathFunction<CrossXAxisNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // CrossYAxis
        Data::Vector3Type source(1, 1, 1);
        auto result = source.CrossYAxis();
        auto output = TestMathFunction<CrossYAxisNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // CrossZAxis
        Data::Vector3Type source(1, 1, 1);
        auto result = source.CrossZAxis();
        auto output = TestMathFunction<CrossZAxisNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

    {   // Distance
        Data::Vector3Type a(1, 2, 3);
        Data::Vector3Type b(-3, -2, -1);
        auto result = a.GetDistance(b);
        auto output = TestMathFunction<DistanceNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // DistanceSqr
        Data::Vector3Type a(1, 2, 3);
        Data::Vector3Type b(-3, -2, -1);
        auto result = a.GetDistanceSq(b);
        auto output = TestMathFunction<DistanceSquaredNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    {   // DivideByNumber
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(3.0);
        auto result = a / static_cast<float>(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Vector3: Source", "Number: Divisor" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {  // DivideByVector
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(2, 2, 2);
        Data::Vector3Type c = a / b;
        auto output = TestMathFunction<DivideByVectorNode>({ "Vector3: Numerator", "Vector3: Divisor" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Dot
        Data::Vector3Type a(1, 2, 3);
        Data::Vector3Type b(-3, -2, -1);
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // From Element
        const Data::Vector3Type source(1, 2, 3);

        for (int index = 0; index < 3; ++index)
        {
            auto result = source;
            result.SetElement(index, 4.0f);
            auto output = TestMathFunction<FromElementNode>({ "Vector3: Source", "Number: Index", "Number: Value" }, { Datum(source), Datum(index), Datum(4.0) }, { "Result: Vector3" }, { Datum(zero) });
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        }
    }

    { // FromLength
        Data::Vector3Type source(1, 2, 3);
        Data::NumberType length(10);
        auto result = source;
        result.SetLength(static_cast<float>(length));
        auto output = TestMathFunction<FromLengthNode>({ "Vector3: Source", "Number: Length" }, { Datum(source), Datum(length) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

    { // FromValues
        Data::NumberType x(1);
        Data::NumberType y(2);
        Data::NumberType z(3);
        auto result = Data::Vector3Type(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        auto output = TestMathFunction<FromValuesNode>({ "Number: X", "Number: Y", "Number: Z" }, { Datum(x), Datum(y), Datum(z) }, { "Result: Vector3", }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // IsCloseNode
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(2, 2, 2);
        Data::Vector3Type c(1.0001, 1.0001, 1.0001);

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, 2.1f);
        output = TestMathFunction<IsCloseNode>({ "Vector3: A", "Vector3: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(2.1) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(c) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, 0.9f);
        output = TestMathFunction<IsCloseNode>({ "Vector3: A", "Vector3: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(0.9) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        Data::Vector3Type sourceFinite(1, 1, 1);
        auto result = sourceFinite.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Vector3: Source" }, { Datum(sourceFinite) }, { "Result: Boolean", }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsNormal
        Data::Vector3Type normal(1.f, 0.f, 0.f);
        Data::Vector3Type nearlyNormal(1.0001f, 0.0f, 0.0f);
        Data::Vector3Type notNormal(10.f, 0.f, 0.f);

        {
            auto result = normal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source" }, { Datum(normal) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = nearlyNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source" }, { Datum(nearlyNormal) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = nearlyNormal.IsNormalized(2.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source", "Number: Tolerance" }, { Datum(nearlyNormal), Datum(2.0f) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = notNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source" }, { Datum(notNormal) }, { "Result: Boolean" }, { Datum(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = notNormal.IsNormalized(12.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source", "Number: Tolerance" }, { Datum(notNormal), Datum(12.0f) }, { "Result: Boolean" }, { Datum(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }
    }

    { // IsPerpendicular
        Data::Vector3Type x(1.f, 0.f, 0.f);
        Data::Vector3Type y(0.f, 1.f, 0.f);

        auto result = x.IsPerpendicular(x);
        auto output = TestMathFunction<IsPerpendicularNode>({ "Vector3: A", "Vector3: B" }, { Datum(x), Datum(x) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = x.IsPerpendicular(y);
        output = TestMathFunction<IsPerpendicularNode>({ "Vector3: A", "Vector3: B" }, { Datum(x), Datum(y) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsZero
        auto result = zero.IsZero();
        auto output = TestMathFunction<IsZeroNode>({ "Vector3: Source" }, { Datum(zero) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = one.IsZero();
        output = TestMathFunction<IsZeroNode>({ "Vector3: Source" }, { Datum(one) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // Length
        Data::Vector3Type source(1, 2, 3);
        auto result = source.GetLength();
        auto output = TestMathFunction<LengthNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthReciprocal
        Data::Vector3Type source(1, 2, 3);
        auto result = source.GetLengthReciprocal();
        auto output = TestMathFunction<LengthReciprocalNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthSquared
        Data::Vector3Type source(1, 2, 3);
        auto result = source.GetLengthSq();
        auto output = TestMathFunction<LengthSquaredNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // Lerp
        Data::Vector3Type from(0.f, 0.f, 0.f);
        Data::Vector3Type to(1.f, 1.f, 1.f);
        Data::NumberType t(0.5);

        auto result = from.Lerp(to, static_cast<float>(t));
        auto output = TestMathFunction<LerpNode>({ "Vector3: From", "Vector3: To", "Number: T" }, { Datum(from), Datum(to), Datum(t) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Max
        Data::Vector3Type a(-1, -1, -1);
        Data::Vector3Type b(1, 1, 1);
        auto result = a.GetMax(b);
        auto output = TestMathFunction<MaxNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Min
        Data::Vector3Type a(-1, -1, -1);
        Data::Vector3Type b(1, 1, 1);
        auto result = a.GetMin(b);
        auto output = TestMathFunction<MinNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // ModXNode
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetXNode>({ "Vector3: Source", "Number: X" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(zero) });
        auto result = a;
        result.SetX(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModYNode
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetYNode>({ "Vector3: Source", "Number: Y" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(zero) });
        auto result = a;
        result.SetY(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModZNode
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetZNode>({ "Vector3: Source", "Number: Z" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(zero) });
        auto result = a;
        result.SetZ(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }    

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // MultiplyAdd
        Data::Vector3Type a(2, 2, 2);
        Data::Vector3Type b(3, 3, 3);
        Data::Vector3Type c(4, 4, 4);
        auto result = a.GetMadd(b, c);
        auto output = TestMathFunction<MultiplyAddNode>({ "Vector3: A", "Vector3: B", "Vector3: C" }, { Datum(a), Datum(b), Datum(c) }, { "Result: Vector3" }, { Datum(a) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

    {   // MultiplyByNumber
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(3.0);
        auto result = a * static_cast<float>(b);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Vector3: Source", "Number: Multiplier" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {  // MultiplyByVector
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(2, 2, 2);
        Data::Vector3Type c = a * b;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Vector3: Source", "Vector3: Multiplier" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // Negate
        Data::Vector3Type source = one;
        auto output = TestMathFunction<NegateNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(zero) });
        auto result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));

        source = negativeOne;
        output = TestMathFunction<NegateNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(zero) });
        result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Normalize
        Data::Vector3Type source = one;
        auto output = TestMathFunction<NormalizeNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(zero) });
        auto result = source.GetNormalizedSafe();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // NormalizeWithLength
        Data::Vector3Type source = one;
        auto output = TestMathFunction<NormalizeWithLengthNode>({ "Vector3: Source" }, { Datum(source) }, { "Normalized: Vector3", "Length: Number" }, { Datum(zero), Datum(0) });
        auto result = source.NormalizeSafeWithLength();
        EXPECT_TRUE(source.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // Project
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(-2, -2, -2);
        auto output = TestMathFunction<ProjectNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(a) });
        a.Project(b);
        EXPECT_TRUE(a.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Reciprocal
        Data::Vector3Type source(2, 2, 2);
        Data::Vector3Type result = source.GetReciprocal();
        auto output = TestMathFunction<ReciprocalNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // Sine
        Data::Vector3Type source(.75, .75, .75);
        Data::Vector3Type result = source.GetSin();
        auto output = TestMathFunction<SineNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }    

    { // SineCosine
        Data::Vector3Type source(.75, .75, .75);
        Data::Vector3Type sine, cosine;
        source.GetSinCos(sine, cosine);
        auto output = TestMathFunction<SineCosineNode>({ "Vector3: Source" }, { Datum(source) }, { "Sine: Vector3", "Cosine: Vector3" }, { Datum(source), Datum(source) });
        EXPECT_TRUE(sine.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(cosine.IsClose(*output[1].GetAs<Data::Vector3Type>()));
    }
#endif

    { // Slerp
        Data::Vector3Type from(0.f, 0.f, 0.f);
        Data::Vector3Type to(1.f, 1.f, 1.f);
        Data::NumberType t(0.5);

        auto slerp = from.Slerp(to, static_cast<float>(t));
        auto outputSlerp = TestMathFunction<SlerpNode>({ "Vector3: From", "Vector3: To", "Number: T" }, { Datum(from), Datum(to), Datum(t) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(slerp.IsClose(*outputSlerp[0].GetAs<Data::Vector3Type>()));

        auto lerp = from.Lerp(to, static_cast<float>(t));
        auto outputLerp = TestMathFunction<LerpNode>({ "Vector3: From", "Vector3: To", "Number: T" }, { Datum(from), Datum(to), Datum(t) }, { "Result: Vector3" }, { Datum(zero) });
        EXPECT_TRUE(lerp.IsClose(*outputLerp[0].GetAs<Data::Vector3Type>()));

        EXPECT_NE(lerp, slerp);
        EXPECT_NE(*outputLerp[0].GetAs<Data::Vector3Type>(), *outputSlerp[0].GetAs<Data::Vector3Type>());
    }

    {   // Subtract
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(2, 2, 2);
        Data::Vector3Type c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Vector3: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Vector3" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // GetElement
        const Data::Vector3Type source(1, 2, 3);

        for (int index = 0; index < 3; ++index)
        {
            float result = aznumeric_caster(source.GetElement(index));
            auto output = TestMathFunction<GetElementNode>({ "Vector3: Source", "Number: Index" }, { Datum(source), Datum(index) }, { "Result: Number" }, { Datum(0) });
            float outputNumber = aznumeric_caster(*output[0].GetAs<Data::NumberType>());
            SC_EXPECT_FLOAT_EQ(result, outputNumber);
        }
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // XAxisCross
        Data::Vector3Type source(.75, .75, .75);
        auto result = source.XAxisCross();
        auto output = TestMathFunction<XAxisCrossNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // YAxisCross
        Data::Vector3Type source(.75, .75, .75);
        auto result = source.YAxisCross();
        auto output = TestMathFunction<YAxisCrossNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // ZAxisCross
        Data::Vector3Type source(.75, .75, .75);
        auto result = source.ZAxisCross();
        auto output = TestMathFunction<ZAxisCrossNode>({ "Vector3: Source" }, { Datum(source) }, { "Result: Vector3" }, { Datum(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

} // Test Vector3Node

TEST_F(ScriptCanvasTestFixture, Vector4Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Vector4Nodes;
    Data::Vector4Type zero(0, 0, 0, 0);
    Data::Vector4Type one(1.f, 1.f, 1.f, 1.f);
    Data::Vector4Type negativeOne(-1.f, -1.f, -1.f, -1.f);

    {   // Absolute
        Data::Vector4Type source(-1, -1, -1., -1.);
        Data::Vector4Type absolute = source.GetAbs();
        auto output = TestMathFunction<AbsoluteNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Vector4" }, { Datum(source) });
        EXPECT_TRUE(absolute.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {   // Add
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(1, 1, 1, 1);
        Data::Vector4Type c = a + b;
        auto output = TestMathFunction<AddNode>({ "Vector4: A", "Vector4: B" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {   // DivideByNumber
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(3.0);
        auto result = a / static_cast<float>(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Vector4: Source", "Number: Divisor" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {  // DivideByVector
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(2, 2, 2, 2);
        Data::Vector4Type c = a / b;
        auto output = TestMathFunction<DivideByVectorNode>({ "Vector4: Numerator", "Vector4: Divisor" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // Dot
        Data::Vector4Type a(1, 2, 3, 4);
        Data::Vector4Type b(-4, -3, -2, -1);
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Vector4: A", "Vector4: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // Dot3
        Data::Vector4Type a(1, 2, 3, 4);
        Data::Vector3Type b(-4, -3, -2);
        auto result = a.Dot3(b);
        auto output = TestMathFunction<Dot3Node>({ "Vector4: A", "Vector3: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // From Element
        const Data::Vector4Type source(1, 2, 3, 4);

        for (int index = 0; index < 4; ++index)
        {
            auto result = source;
            result.SetElement(index, 5.0f);
            auto output = TestMathFunction<FromElementNode>({ "Vector4: Source", "Number: Index", "Number: Value" }, { Datum(source), Datum(index), Datum(5.0) }, { "Result: Vector4" }, { Datum(zero) });
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        }
    }
#endif

    { // FromValues
        Data::NumberType x(1);
        Data::NumberType y(2);
        Data::NumberType z(3);
        Data::NumberType w(4);
        auto result = Data::Vector4Type(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(w));
        auto output = TestMathFunction<FromValuesNode>({ "Number: X", "Number: Y", "Number: Z", "Number: W" }, { Datum(x), Datum(y), Datum(z), Datum(w) }, { "Result: Vector4", }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // GetElements
        auto source = Vector4Type(1, 2, 3, 4);
        auto output = TestMathFunction<GetElementsNode>({ "Vector4: Source" }, { Datum(source) }, { "X: Number", "Y: Number", "Z: Number", "W: Number" }, { Datum(0), Datum(0), Datum(0), Datum(0) });
        SC_EXPECT_FLOAT_EQ(source.GetX(), aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
        SC_EXPECT_FLOAT_EQ(source.GetY(), aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
        SC_EXPECT_FLOAT_EQ(source.GetZ(), aznumeric_caster(*output[2].GetAs<Data::NumberType>()));
        SC_EXPECT_FLOAT_EQ(source.GetW(), aznumeric_caster(*output[3].GetAs<Data::NumberType>()));
    }
#endif

    { // IsCloseNode
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(2, 2, 2, 2);
        Data::Vector4Type c(1.0001, 1.0001, 1.0001, 1.0001);

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Vector4: A", "Vector4: B" }, { Datum(a), Datum(b) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, 2.1f);
        output = TestMathFunction<IsCloseNode>({ "Vector4: A", "Vector4: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(2.1) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Vector4: A", "Vector4: B" }, { Datum(a), Datum(c) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, 0.9f);
        output = TestMathFunction<IsCloseNode>({ "Vector4: A", "Vector4: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(0.9) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        Data::Vector4Type sourceFinite(1, 1, 1, 1);
        auto result = sourceFinite.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Vector4: Source" }, { Datum(sourceFinite) }, { "Result: Boolean", }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsNormal
        Data::Vector4Type normal(1.f, 0.f, 0.f, 0.f);
        Data::Vector4Type nearlyNormal(1.0001f, 0.0f, 0.0f, 0.f);
        Data::Vector4Type notNormal(10.f, 0.f, 0.f, 0.f);

        {
            auto result = normal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source" }, { Datum(normal) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = nearlyNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source" }, { Datum(nearlyNormal) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = nearlyNormal.IsNormalized(2.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source", "Number: Tolerance" }, { Datum(nearlyNormal), Datum(2.0f) }, { "Result: Boolean" }, { Datum(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = notNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source" }, { Datum(notNormal) }, { "Result: Boolean" }, { Datum(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = notNormal.IsNormalized(12.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source", "Number: Tolerance" }, { Datum(notNormal), Datum(12.0f) }, { "Result: Boolean" }, { Datum(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }
    }

    { // IsZero
        auto result = zero.IsZero();
        auto output = TestMathFunction<IsZeroNode>({ "Vector4: Source" }, { Datum(zero) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = one.IsZero();
        output = TestMathFunction<IsZeroNode>({ "Vector4: Source" }, { Datum(one) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // Length
        Data::Vector4Type source(1, 2, 3, 4);
        auto result = source.GetLength();
        auto output = TestMathFunction<LengthNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthReciprocal
        Data::Vector4Type source(1, 2, 3, 4);
        auto result = source.GetLengthReciprocal();
        auto output = TestMathFunction<LengthReciprocalNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthSquared
        Data::Vector4Type source(1, 2, 3, 4);
        auto result = source.GetLengthSq();
        auto output = TestMathFunction<LengthSquaredNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    {   // ModXNode
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetXNode>({ "Vector4: Source", "Number: X" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(zero) });
        auto result = a;
        result.SetX(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModYNode
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetYNode>({ "Vector4: Source", "Number: Y" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(zero) });
        auto result = a;
        result.SetY(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModZNode
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetZNode>({ "Vector4: Source", "Number: Z" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(zero) });
        auto result = a;
        result.SetZ(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModWNode
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(0);
        auto output = TestMathFunction<SetWNode>({ "Vector4: Source", "Number: W" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(zero) });
        auto result = a;
        result.SetW(static_cast<float>(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // MultiplyByNumber
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(3.0);
        auto result = a * static_cast<float>(b);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Vector4: Source", "Number: Multiplier" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {  // MultiplyByVector
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(2, 2, 2, 2);
        Data::Vector4Type c = a * b;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Vector4: Source", "Vector4: Multiplier" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {   // Negate
        Data::Vector4Type source = one;
        auto output = TestMathFunction<NegateNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Vector4" }, { Datum(zero) });
        auto result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));

        source = negativeOne;
        output = TestMathFunction<NegateNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Vector4" }, { Datum(zero) });
        result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // Normalize
        Data::Vector4Type source = one;
        auto output = TestMathFunction<NormalizeNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Vector4" }, { Datum(zero) });
        auto result = source.GetNormalizedSafe();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // NormalizeWithLength
        Data::Vector4Type source = one;
        auto output = TestMathFunction<NormalizeWithLengthNode>({ "Vector4: Source" }, { Datum(source) }, { "Normalized: Vector4", "Length: Number" }, { Datum(zero), Datum(0) });
        auto result = source.NormalizeSafeWithLength();
        EXPECT_TRUE(source.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // Reciprocal
        Data::Vector4Type source(2, 2, 2, 2);
        Data::Vector4Type result = source.GetReciprocal();
        auto output = TestMathFunction<ReciprocalNode>({ "Vector4: Source" }, { Datum(source) }, { "Result: Vector4" }, { Datum(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {   // Subtract
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(2, 2, 2, 2);
        Data::Vector4Type c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Vector4: A", "Vector4: B" }, { Datum(a), Datum(b) }, { "Result: Vector4" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetElement
        const Data::Vector4Type source(1, 2, 3, 4);

        for (int index = 0; index < 4; ++index)
        {
            float result = aznumeric_caster(source.GetElement(index));
            auto output = TestMathFunction<GetElementNode>({ "Vector4: Source", "Number: Index" }, { Datum(source), Datum(index) }, { "Result: Number" }, { Datum(0) });
            float outputNumber = aznumeric_caster(*output[0].GetAs<Data::NumberType>());
            SC_EXPECT_FLOAT_EQ(result, outputNumber);
        }
    }

    // Missing Nodes:
    // HomogenizeNode
    // FromVector3AndNumberNode

} // Test Vector4Node

TEST_F(ScriptCanvasTestFixture, QuaternionNodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::QuaternionNodes;
    const Data::QuaternionType esclates(0.125, 0.25, 0.50, 0.75);
    const Data::QuaternionType negativeOne(-1.f, -1.f, -1.f, -1.f);
    const Data::QuaternionType one(1.f, 1.f, 1.f, 1.f);
    const Data::QuaternionType zero(0, 0, 0, 0);
    const Data::QuaternionType identity(Data::QuaternionType::CreateIdentity());

    {   // Add
        Data::QuaternionType a(1, 1, 1, 1);
        Data::QuaternionType b(1, 1, 1, 1);
        Data::QuaternionType c = a + b;
        auto output = TestMathFunction<AddNode>({ "Quaternion: A", "Quaternion: B" }, { Datum(a), Datum(b) }, { "Result: Quaternion" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    {   // Conjugate
        Data::QuaternionType source = esclates;
        auto output = TestMathFunction<ConjugateNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Quaternion" }, { Datum(zero) });
        auto result = esclates.GetConjugate();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    {   // DivideByNumber
        Data::QuaternionType a(1, 2, 3, 4);
        Data::NumberType b(3.0);
        auto result = a / static_cast<float>(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Quaternion: Numerator", "Number: Divisor" }, { Datum(a), Datum(b) }, { "Result: Quaternion" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // Dot
        Data::QuaternionType a(1, 2, 3, 4);
        Data::QuaternionType b(-4, -3, -2, -1);
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Quaternion: A", "Quaternion: B" }, { Datum(a), Datum(b) }, { "Result: Number" }, { Datum(0.0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // From AxisAngleDegrees
        const Vector3Type axis = Vector3Type(1, 1, 1).GetNormalized();
        const float angle(2.0);
        auto result = QuaternionType::CreateFromAxisAngle(axis, AZ::DegToRad(angle));
        auto output = TestMathFunction<FromAxisAngleDegreesNode>({ "Vector3: Axis", "Number: Degrees" }, { Datum(axis), Datum(angle) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // From Element
        const Data::QuaternionType source(1, 2, 3, 4);

        for (int index = 0; index < 4; ++index)
        {
            auto result = source;
            result.SetElement(index, 5.0f);
            auto output = TestMathFunction<FromElementNode>({ "Quaternion: Source", "Number: Index", "Number: Value" }, { Datum(source), Datum(index), Datum(5.0) }, { "Result: Quaternion" }, { Datum(zero) });
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
        }
    }
#endif

    { // FromMatrix3x3
        const Matrix3x3Type matrix = Matrix3x3Type::CreateFromValue(3.0);
        auto result = QuaternionType::CreateFromMatrix3x3(matrix);
        auto output = TestMathFunction<FromMatrix3x3Node>({ "Matrix3x3: Source" }, { Datum(matrix) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // FromMatrix4x4
        const Matrix4x4Type matrix = Matrix4x4Type::CreateFromValue(3.0);
        auto result = QuaternionType::CreateFromMatrix4x4(matrix);
        auto output = TestMathFunction<FromMatrix4x4Node>({ "Matrix4x4: Source" }, { Datum(matrix) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // FromTransform
        const TransformType transform = TransformType::CreateRotationX(1.0f);
        auto result = transform.GetRotation();
        auto output = TestMathFunction<FromTransformNode>({ "Transform: Source" }, { Datum(transform) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // FromVector3
        const Vector3Type vector3 = Vector3Type(3, 3, 3);
        auto result = QuaternionType::CreateFromVector3(vector3);
        auto output = TestMathFunction<FromVector3Node>({ "Vector3: Source" }, { Datum(vector3) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    // Note: This is just raw setting the elements to the value, it is not creating from axis and angle
    //       There is another method which does create from axis angle.

    { // FromVector3AndValueNode
        const Vector3Type axis = Vector3Type(1, 1, 1).GetNormalized();
        const float angle(2.0);
        auto result = QuaternionType::CreateFromVector3AndValue(axis, angle);
        auto output = TestMathFunction<FromVector3AndValueNode>({ "Vector3: Imaginary", "Number: Real" }, { Datum(axis), Datum(angle) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // GetElements
        auto source = QuaternionType(1, 2, 3, 4);
        auto output = TestMathFunction<GetElementsNode>({ "Quaternion: Source" }, { Datum(source) }, { "X: Number", "Y: Number", "Z: Number", "W: Number" }, { Datum(0), Datum(0), Datum(0), Datum(0) });
        SC_EXPECT_FLOAT_EQ(source.GetX(), aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
        SC_EXPECT_FLOAT_EQ(source.GetY(), aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
        SC_EXPECT_FLOAT_EQ(source.GetZ(), aznumeric_caster(*output[2].GetAs<Data::NumberType>()));
        SC_EXPECT_FLOAT_EQ(source.GetW(), aznumeric_caster(*output[3].GetAs<Data::NumberType>()));
    }
#endif

    { // InvertFullNode
        const QuaternionType source = QuaternionType::CreateFromVector3AndValue(AZ::Vector3(1, 1, 1), 3);
        auto result = source.GetInverseFull();
        auto output = TestMathFunction<InvertFullNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // IsCloseNode
        Data::QuaternionType a(1, 1, 1, 1);
        Data::QuaternionType b(2, 2, 2, 2);
        Data::QuaternionType c(1.0001, 1.0001, 1.0001, 1.0001);

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Quaternion: A", "Quaternion: B" }, { Datum(a), Datum(b) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, 2.1f);
        output = TestMathFunction<IsCloseNode>({ "Quaternion: A", "Quaternion: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(2.1) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Quaternion: A", "Quaternion: B" }, { Datum(a), Datum(c) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, 0.9f);
        output = TestMathFunction<IsCloseNode>({ "Quaternion: A", "Quaternion: B", "Number: Tolerance" }, { Datum(a), Datum(b), Datum(0.9) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        Data::QuaternionType sourceFinite(1, 1, 1, 1);
        auto result = sourceFinite.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Quaternion: Source" }, { Datum(sourceFinite) }, { "Result: Boolean", }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsIdentityNode
        auto result = identity.IsIdentity();
        auto output = TestMathFunction<IsIdentityNode>({ "Quaternion: Source" }, { Datum(identity) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = zero.IsIdentity();
        output = TestMathFunction<IsIdentityNode>({ "Quaternion: Source" }, { Datum(zero) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsZero
        auto result = zero.IsZero();
        auto output = TestMathFunction<IsZeroNode>({ "Quaternion: Source" }, { Datum(zero) }, { "Result: Boolean" }, { Datum(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = one.IsZero();
        output = TestMathFunction<IsZeroNode>({ "Quaternion: Source" }, { Datum(one) }, { "Result: Boolean" }, { Datum(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // Length
        Data::QuaternionType source(1, 2, 3, 4);
        auto result = source.GetLength();
        auto output = TestMathFunction<LengthNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthReciprocal
        Data::QuaternionType source(1, 2, 3, 4);
        auto result = source.GetLengthReciprocal();
        auto output = TestMathFunction<LengthReciprocalNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthSquared
        Data::QuaternionType source(1, 2, 3, 4);
        auto result = source.GetLengthSq();
        auto output = TestMathFunction<LengthSquaredNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // Lerp
        Data::QuaternionType from(zero);
        Data::QuaternionType to(Data::QuaternionType::CreateFromAxisAngle(AZ::Vector3(1, 1, 1).GetNormalized(), 0.75));
        Data::NumberType t(0.5);

        auto result = from.Lerp(to, static_cast<float>(t));
        auto output = TestMathFunction<LerpNode>({ "Quaternion: From", "Quaternion: To", "Number: T" }, { Datum(from), Datum(to), Datum(t) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    {   // MultiplyByNumber
        Data::QuaternionType a(1, 2, 3, 4);
        Data::NumberType b(3.0);
        auto result = a * static_cast<float>(b);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Quaternion: Source", "Number: Multiplier" }, { Datum(a), Datum(b) }, { "Result: Quaternion" }, { Datum(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    {  // MultiplyByRotation
        Data::QuaternionType a(1, 1, 1, 1);
        Data::QuaternionType b(2, 2, 2, 2);
        Data::QuaternionType c = a * b;
        auto output = TestMathFunction<MultiplyByRotationNode>({ "Quaternion: A", "Quaternion: B" }, { Datum(a), Datum(b) }, { "Result: Quaternion" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    {   // Negate
        Data::QuaternionType source = one;
        auto output = TestMathFunction<NegateNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Quaternion" }, { Datum(zero) });
        auto result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));

        source = negativeOne;
        output = TestMathFunction<NegateNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Quaternion" }, { Datum(zero) });
        result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // Normalize
        Data::QuaternionType source = one;
        auto output = TestMathFunction<NormalizeNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Quaternion" }, { Datum(zero) });
        auto result = source.GetNormalized();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // NormalizeWithLength
        Data::QuaternionType source = one;
        auto output = TestMathFunction<NormalizeWithLengthNode>({ "Quaternion: Source" }, { Datum(source) }, { "Normalized: Quaternion", "Length: Number" }, { Datum(zero), Datum(0) });
        auto result = source.NormalizeWithLength();
        EXPECT_TRUE(source.IsClose(*output[0].GetAs<Data::QuaternionType>()));
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // RotationXDegreesNode
        auto output = TestMathFunction<RotationXDegreesNode>({ "Number: Degrees" }, { Datum(30) }, { "Result: Quaternion" }, { Datum(identity) });
        auto result = QuaternionType::CreateRotationX(AZ::DegToRad(30));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<QuaternionType>()));
    }

    { // RotationYDegreesNode
        auto output = TestMathFunction<RotationYDegreesNode>({ "Number: Degrees" }, { Datum(30) }, { "Result: Quaternion" }, { Datum(identity) });
        auto result = QuaternionType::CreateRotationY(AZ::DegToRad(30));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<QuaternionType>()));
    }

    { // RotationZDegreesNode
        auto output = TestMathFunction<RotationZDegreesNode>({ "Number: Degrees" }, { Datum(30) }, { "Result: Quaternion" }, { Datum(identity) });
        auto result = QuaternionType::CreateRotationZ(AZ::DegToRad(30));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<QuaternionType>()));
    }

    { // ShortestArcNode
        Data::Vector3Type from(-1, 0, 0);
        Data::Vector3Type to(1, 1, 1);

        auto result = QuaternionType::CreateShortestArc(from, to);
        auto output = TestMathFunction<ShortestArcNode>({ "Vector3: From", "Vector3: To" }, { Datum(from), Datum(to) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // Slerp
        Data::QuaternionType from(zero);
        Data::QuaternionType to(Data::QuaternionType::CreateFromAxisAngle(AZ::Vector3(1, 1, 1).GetNormalized(), 0.75));
        Data::NumberType t(0.5);

        auto result = from.Slerp(to, static_cast<float>(t));
        auto output = TestMathFunction<SlerpNode>({ "Quaternion: From", "Quaternion: To", "Number: T" }, { Datum(from), Datum(to), Datum(t) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // SquadNode
        Data::QuaternionType from(Data::QuaternionType::CreateFromAxisAngle(Vector3Type(-1, 0, 0).GetNormalized(), 0.75));
        Data::QuaternionType to(Data::QuaternionType::CreateFromAxisAngle(Vector3Type(1, 1, 1).GetNormalized(), 0.75));
        Data::QuaternionType out(Data::QuaternionType::CreateFromAxisAngle(Vector3Type(1, 0, 1).GetNormalized(), 0.75));
        Data::QuaternionType in(Data::QuaternionType::CreateFromAxisAngle(Vector3Type(1, 1, 0).GetNormalized(), 0.75));
        Data::NumberType t(0.5);

        auto result = from.Squad(to, in, out, static_cast<float>(t));
        auto output = TestMathFunction<SquadNode>({ "Quaternion: From", "Quaternion: To", "Quaternion: In", "Quaternion: Out", "Number: T" }, { Datum(from), Datum(to), Datum(in), Datum(out), Datum(t) }, { "Result: Quaternion" }, { Datum(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    {   // Subtract
        Data::QuaternionType a(1, 1, 1, 1);
        Data::QuaternionType b(2, 2, 2, 2);
        Data::QuaternionType c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Quaternion: A", "Quaternion: B" }, { Datum(a), Datum(b) }, { "Result: Quaternion" }, { Datum(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

    { // ToAngleDegreesNode
        Data::QuaternionType source(Data::QuaternionType::CreateFromAxisAngle(Vector3Type(-1, 0, 0).GetNormalized(), 0.75));
        auto output = TestMathFunction<ToAngleDegreesNode>({ "Quaternion: Source" }, { Datum(source) }, { "Result: Number" }, { Datum(0) });
        float result = AZ::RadToDeg(source.GetAngle());
        SC_EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // CreateFromEulerAnglesNode
        Data::QuaternionType baseValue = AZ::ConvertEulerDegreesToQuaternion(AZ::Vector3(1.0f,2.0f,3.0f));
        auto output = TestMathFunction<CreateFromEulerAnglesNode>({ "Number: Pitch", "Number: Roll", "Number: Yaw" }, { Datum(1.0f), Datum(2.0f), Datum(3.0f) }, { "Result: Quaternion" }, { Datum(AZ::Quaternion()) });
        EXPECT_TRUE(baseValue.IsClose(*output[0].GetAs<Data::QuaternionType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // GetElement
        const Data::QuaternionType source(1, 2, 3, 4);

        for (int index = 0; index < 4; ++index)
        {
            float result = aznumeric_caster(source.GetElement(index));
            auto output = TestMathFunction<GetElementNode>({ "Quaternion: Source", "Number: Index" }, { Datum(source), Datum(index) }, { "Result: Number" }, { Datum(0) });
            float outputNumber = aznumeric_caster(*output[0].GetAs<Data::NumberType>());
            SC_EXPECT_FLOAT_EQ(result, outputNumber);
        }
    }
#endif

    // Missing Units Tests:
    // ModXNode
    // ModYNode
    // ModZNode
    // ModWNode
    // ToImaginary

} // Test QuaternionNodes

TEST_F(ScriptCanvasTestFixture, RandomNodes)
{
    using namespace ScriptCanvas;

    const int testIterations = 10;

    auto transform = Data::TransformType::CreateIdentity();

    { // RandomColor
        const Data::ColorType min(0.1f, 0.3f, 0.5f, 0.7f);
        const Data::ColorType max(0.2f, 0.4f, 0.6f, 0.8f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomColorNode>({ "Color: Min", "Color: Max" }, { Datum(min), Datum(max) }, { "Result: Color" }, { Datum(Data::ColorType()) });
            auto value = *output[0].GetAs<Data::ColorType>();
            EXPECT_TRUE(value.GetR() >= 0.1f && value.GetR() <= 0.2f);
            EXPECT_TRUE(value.GetG() >= 0.3f && value.GetG() <= 0.4f);
            EXPECT_TRUE(value.GetB() >= 0.5f && value.GetB() <= 0.6f);
            EXPECT_TRUE(value.GetA() >= 0.7f && value.GetA() <= 0.8f);
        }
    }

    { // RandomGrayscale
        const float minColor = 96.f;
        const float maxColor = 192.f;

        const float minColorNormalized = minColor / 255.f;
        const float maxColorNormalized = maxColor / 255.f;
            
        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomGrayscaleNode>({ "Number: Min", "Number: Max" }, { Datum(minColor), Datum(maxColor) }, { "Result: Color" }, { Datum(Data::ColorType()) });
            auto value = *output[0].GetAs<Data::ColorType>();
            EXPECT_TRUE((value.GetR() == value.GetG()) && value.GetR() == value.GetB());
            EXPECT_TRUE(value.GetR() >= minColorNormalized && value.GetR() <= maxColorNormalized);
        }
    }

    { // RandomInteger
        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomIntegerNode>({ "Number: Min", "Number: Max" }, { Datum(9), Datum(99) }, { "Result: Number" }, { Datum(Data::NumberType()) });
            auto value = *output[0].GetAs<Data::NumberType>();
            EXPECT_TRUE(value >= 9);
            EXPECT_TRUE(value <= 99);
        }
    }

    { // RandomNumber
        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomNumberNode>({ "Number: Min", "Number: Max" }, { Datum(9.f), Datum(99.f) }, { "Result: Number" }, { Datum(Data::NumberType()) });
            auto value = *output[0].GetAs<Data::NumberType>();
            EXPECT_TRUE(value >= 9);
            EXPECT_TRUE(value <= 99);
        }
    }

    { // RandomPointInBox
        Data::Vector3Type dimensions(10.f, 100.f, 1000.f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomPointInBoxNode>({ "Vector3: Dimensions" }, { Datum(dimensions) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            EXPECT_TRUE((value.GetX() >= -5.f) && (value.GetX() <= 5.f));
            EXPECT_TRUE((value.GetY() >= -50.f) && (value.GetY() <= 50.f));
            EXPECT_TRUE((value.GetZ() >= -500.f) && (value.GetZ() <= 500.f));
        }
    }

    { // RandomPointOnCircle
        Data::NumberType radius(10.f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomPointOnCircleNode>({ "Number: Radius" }, { Datum(radius) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            float lenSq = value.GetLengthSq();
            SC_EXPECT_FLOAT_EQ(lenSq, aznumeric_cast<float>(radius * radius));
        }
    }

    { // RandomPointInCone
        Data::NumberType radius(10.f);
        Data::NumberType angleInDegrees(45.f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomPointInConeNode>({ "Number: Radius", "Number: Angle" }, { Datum(radius), Datum(angleInDegrees) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            float dot = value.Dot(Data::Vector3Type(0.f, 0.f, 1.f));
            EXPECT_TRUE(dot >= 0.f);
            EXPECT_TRUE(dot <= radius);
        }
    }

    { // RandomPointInCylinder
        Data::NumberType radius(10.f);
        Data::NumberType height(100.f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomPointInCylinderNode>({ "Number: Radius", "Number: Height" }, { Datum(radius), Datum(height) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            float lenXYsq = Data::Vector2Type(value.GetX(), value.GetY()).GetLengthSq();
            EXPECT_TRUE(lenXYsq <= (radius * radius));
            float dotZ = value.Dot(Data::Vector3Type(0.f, 0.f, 1.f));
            EXPECT_TRUE(dotZ >= -(0.5f * height));
            EXPECT_TRUE(dotZ <= (0.5f * height));
        }
    }

    { // RandomPointInCircle
        Data::NumberType radius(10.f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomPointInCircleNode>({ "Number: Radius" }, { Datum(radius) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            float dot = value.Dot(Data::Vector3Type(0.f, 0.f, 1.f));
            float lenSq = value.GetLengthSq();
            EXPECT_TRUE(dot == 0.f);
            EXPECT_TRUE(lenSq <= (radius * radius));
        }
    }

    { // RandomPointInSphere
        Data::NumberType radius(10.f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomPointInSphereNode>({ "Number: Radius" }, { Datum(radius) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            float lenSq = value.GetLengthSq();
            EXPECT_TRUE(lenSq <= (radius * radius));
        }
    }

    { // RandomPointInSquare
        Data::Vector2Type dimensions(10.f, 100.f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomPointInSquareNode>({ "Vector2: Dimensions" }, { Datum(dimensions) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            float dot = value.Dot(Data::Vector3Type(0.f, 0.f, 1.f));
            EXPECT_TRUE(dot == 0.f);
            EXPECT_TRUE((value.GetX() >= -(0.5f * dimensions.GetX())) && (value.GetX() <= (0.5f * dimensions.GetX())));
            EXPECT_TRUE((value.GetY() >= -(0.5f * dimensions.GetY())) && (value.GetY() <= (0.5f * dimensions.GetY())));
        }
    }

    { // RandomPointOnSphere
        Data::NumberType radius(10.f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomPointOnSphereNode>({ "Number: Radius" }, { Datum(radius) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            float lenSq = value.GetLengthSq();
            SC_EXPECT_FLOAT_EQ(lenSq, aznumeric_cast<float>(radius * radius));
        }
    }

    { // RandomUnitVector2
        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomUnitVector2Node>({}, {}, { "Result: Vector2" }, { Datum(Data::Vector2Type()) });
            auto value = *output[0].GetAs<Data::Vector2Type>();
            float lenSq = value.GetLengthSq();
            SC_EXPECT_FLOAT_EQ(lenSq, 1.f);
        }
    }

    { // RandomUnitVector3
        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomUnitVector3Node>({}, {}, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            float lenSq = value.GetLengthSq();
            SC_EXPECT_FLOAT_EQ(lenSq, 1.f);
        }
    }

    { // RandomVector2
        const Data::Vector2Type min(0.1f, 0.3f);
        const Data::Vector2Type max(0.2f, 0.4f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomVector2Node>({ "Vector2: Min", "Vector2: Max" }, { Datum(min), Datum(max) }, { "Result: Vector2" }, { Datum(Data::Vector2Type()) });
            auto value = *output[0].GetAs<Data::Vector2Type>();
            EXPECT_TRUE(value.GetX() >= min.GetX() && value.GetX() <= max.GetX());
            EXPECT_TRUE(value.GetY() >= min.GetY() && value.GetY() <= max.GetY());
        }
    }

    { // RandomVector3
        const Data::Vector3Type min(0.1f, 0.3f, 0.5f);
        const Data::Vector3Type max(0.2f, 0.4f, 0.6f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomVector3Node>({ "Vector3: Min", "Vector3: Max" }, { Datum(min), Datum(max) }, { "Result: Vector3" }, { Datum(Data::Vector3Type()) });
            auto value = *output[0].GetAs<Data::Vector3Type>();
            EXPECT_TRUE(value.GetX() >= min.GetX() && value.GetX() <= max.GetX());
            EXPECT_TRUE(value.GetY() >= min.GetY() && value.GetY() <= max.GetY());
            EXPECT_TRUE(value.GetZ() >= min.GetZ() && value.GetZ() <= max.GetZ());
        }
    }

    { // RandomVector4
        const Data::Vector4Type min(0.1f, 0.3f, 0.5f, 0.7f);
        const Data::Vector4Type max(0.2f, 0.4f, 0.6f, 0.8f);

        for (int i = 0; i < testIterations; ++i)
        {
            auto output = TestMathFunction<RandomNodes::RandomVector4Node>({ "Vector4: Min", "Vector4: Max" }, { Datum(min), Datum(max) }, { "Result: Vector4" }, { Datum(Data::Vector4Type()) });
            auto value = *output[0].GetAs<Data::Vector4Type>();
            EXPECT_TRUE(value.GetX() >= min.GetX() && value.GetX() <= max.GetX());
            EXPECT_TRUE(value.GetY() >= min.GetY() && value.GetY() <= max.GetY());
            EXPECT_TRUE(value.GetZ() >= min.GetZ() && value.GetZ() <= max.GetZ());
            EXPECT_TRUE(value.GetW() >= min.GetW() && value.GetW() <= max.GetW());
        }
    }
} // Test RandomNodes

TEST_F(ScriptCanvasTestFixture, MathOperations_Graph)
{
    RunUnitTestGraph("LY_SC_UnitTest_MathOperations");
}

TEST_F(ScriptCanvasTestFixture, MathCustom_Graph)
{
    RunUnitTestGraph("LY_SC_UnitTest_UnitTest_MathCustom");
}
