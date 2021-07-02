/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>

#pragma warning( push )
#pragma warning( disable : 5046) //'function' : Symbol involving type with internal linkage not defined

using namespace ScriptCanvasTests;

namespace 
{

    AZ_INLINE void ArgsNoReturn(float)
    {
        UnitTestEventsBus::Broadcast(&UnitTestEvents::SideEffect, "ArgsNoReturn SideFX");
    }

    AZ_INLINE std::tuple<AZStd::string, bool> ArgsReturnMulti(double input)
    {
        // compile test
        return input >= 0.0
            ? std::make_tuple("positive", true)
            : std::make_tuple("negative", false);
    }

    AZ_INLINE void NoArgsNoReturn()
    {
        UnitTestEventsBus::Broadcast(&UnitTestEvents::SideEffect, "NoArgsNoReturn SideFX");
    }

    AZ_INLINE float NoArgsReturn()
    {
        UnitTestEventsBus::Broadcast(&UnitTestEvents::SideEffect, "NoArgsReturn SideFX");
        return 0.0f;
    }

    AZ_INLINE std::tuple<AZStd::string, bool> NoArgsReturnMulti()
    {
        return std::make_tuple("no-args", false);
    }

    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ArgsNoReturn, "UnitTests", "{980E4400-288B-4DA2-8C5C-BBC5164CA2AB}", "", "One Arg");
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(ArgsReturnMulti, "UnitTests", "{D7475558-BD14-4588-BC3A-6B4BD1ACF3B4}", "", "input:One Arg", "output:string", "output:bool");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(NoArgsNoReturn, "UnitTests", "{18BC4E04-7D97-4379-8A36-877881633AA9}", "");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(NoArgsReturn, "UnitTests", "{08E6535A-FCE0-4953-BA3E-59CF5A10073B}", "");
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NoArgsReturnMulti, "UnitTests", "{A73262FA-2756-40D6-A25C-8B98A64348F2}", "", "output:string", "output:bool");

    // generic types that passed around by reference/pointer should behavior just like the value types
    int MaxReturnByValueInteger(int lhs, int rhs)
    {
        return lhs >= rhs ? lhs : rhs;
    }

    const int* MaxReturnByPointerInteger(const int* lhs, const int* rhs)
    {
        return (lhs && rhs && (*lhs) >= (*rhs)) ? lhs : rhs;
    }

    const int& MaxReturnByReferenceInteger(const int& lhs, const int& rhs)
    {
        return lhs >= rhs ? lhs : rhs;
    }

    std::tuple<TestBehaviorContextObject, int> MaxReturnByValueMulti(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs, int lhsInt, int rhsInt)
    {
        return std::make_tuple
        (lhs.GetValue() >= rhs.GetValue() ? lhs : rhs
            , lhsInt >= rhsInt ? lhsInt : rhsInt);
    }

    std::tuple<const TestBehaviorContextObject*, const int*> MaxReturnByPointerMulti(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs, const int* lhsInt, const int* rhsInt)
    {
        return std::make_tuple
        ((lhs && rhs && (lhs->GetValue() >= rhs->GetValue())) ? lhs : rhs
            , lhsInt && rhsInt && *lhsInt >= *rhsInt ? lhsInt : rhsInt);
    }

    std::tuple<const TestBehaviorContextObject&, const int&> MaxReturnByReferenceMulti(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs, const int& lhsInt, const int& rhsInt)
    {
        return std::forward_as_tuple
        (lhs.GetValue() >= rhs.GetValue() ? lhs : rhs
            , lhsInt >= rhsInt ? lhsInt : rhsInt);
    }

    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByValue, "UnitTests", "{60C054C6-8A07-4D41-A9E4-E3BB0D20F098}", "", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByPointer, "UnitTests", "{16AFDE59-31B5-4B49-999F-8B486FC91371}", "", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByReference, "UnitTests", "{0A1FD51A-1D53-46FC-9A2F-DF711F62FDE9}", "", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByValueInteger, "UnitTests", "{5165F1BA-248F-434F-9227-B6AC2102D4B5}", "", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByPointerInteger, "UnitTests", "{BE658D24-8AB0-463B-979D-C829985E96EF}", "", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByReferenceInteger, "UnitTests", "{8DE10FF6-9628-4015-A149-4276BF98D2AB}", "", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(MaxReturnByValueMulti, "UnitTests", "{5BE8F2C8-C036-4C82-A7C1-4DCBAC2FA6FC}", "", "0", "1", "0", "1", "Result", "Result");
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(MaxReturnByPointerMulti, "UnitTests", "{339BDAB0-BB80-4BFE-B377-12FD08278A8E}", "", "0", "1", "0", "1", "Result", "Result");
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(MaxReturnByReferenceMulti, "UnitTests", "{7FECD272-4348-463C-80CC-45D0C77378A6}", "", "0", "1", "0", "1", "Result", "Result");

} // namespace

namespace ScriptCanvas
{
    AZ_INLINE AZ::Vector3 NormalizeWithDefault(const AZ::Vector3& source, const Data::NumberType tolerance, [[maybe_unused]] const Data::BooleanType fakeValueForTestingDefault)
    {
        AZ_TracePrintf("SC", "The fake value for testing default is %s\n", fakeValueForTestingDefault ? "True" : "False");
        return source.GetNormalizedSafe(tolerance);
    }

    void NormalizeWithDefaultInputOverrides(Node& node) { SetDefaultValuesByIndex< 1, 2 >::_(node, 3.3, true); }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(NormalizeWithDefault, NormalizeWithDefaultInputOverrides, "Math/Vector3", "{1A56B08E-7E48-4240-878A-397A912519B6}", "description placeholder", "Vector", "Tolerance", "Fake Testing Default Value");
}

TEST_F(ScriptCanvasTestFixture, NodeGenerics)
{    
    using namespace ScriptCanvasEditor;

    RegisterComponentDescriptor<ArgsNoReturnNode>();
    RegisterComponentDescriptor<ArgsReturnMultiNode>();
    RegisterComponentDescriptor<NoArgsNoReturnNode>();
    RegisterComponentDescriptor<NoArgsReturnNode>();
    RegisterComponentDescriptor<NoArgsReturnMultiNode>();
    RegisterComponentDescriptor<NormalizeWithDefaultNode>();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    ScriptCanvas::Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId noArgsNoReturnNodeID;
    CreateTestNode<NoArgsNoReturnNode>(graphUniqueId, noArgsNoReturnNodeID);
    AZ::EntityId argsNoReturnNodeID;
    CreateTestNode<ArgsNoReturnNode>(graphUniqueId, argsNoReturnNodeID);
    AZ::EntityId noArgsReturnNodeID;
    CreateTestNode<NoArgsReturnNode>(graphUniqueId, noArgsReturnNodeID);

    AZ::EntityId normalizeWithDefaultNodeID;
    CreateTestNode<NormalizeWithDefaultNode>(graphUniqueId, normalizeWithDefaultNodeID);

    AZ::EntityId unused0, unused1;
    CreateTestNode<ArgsReturnMultiNode>(graphUniqueId, unused0);
    CreateTestNode<NoArgsReturnMultiNode>(graphUniqueId, unused1);

    EXPECT_TRUE(Connect(*graph, startID, "Out", noArgsNoReturnNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, noArgsNoReturnNodeID, "Out", argsNoReturnNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, argsNoReturnNodeID, "Out", noArgsReturnNodeID, "In"));

    EXPECT_TRUE(Connect(*graph, noArgsReturnNodeID, "Out", normalizeWithDefaultNodeID, "In"));

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByValue)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByValueNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByValueNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByPointer)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByPointerNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByPointerNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByReference)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByReferenceNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByReferenceNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByValueInteger)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByValueIntegerNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);    

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId graphEntityId = graph->GetEntityId();
    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByValueIntegerNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Node* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto value1 = valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID5, "Set"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByPointerInteger)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByPointerIntegerNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);    

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByPointerIntegerNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Node* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto value1 = valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID5, "Set"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByReferenceInteger)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByReferenceIntegerNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByReferenceIntegerNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Node* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto value1 = valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID5, "Set"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);    
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByValueMulti)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByValueMultiNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByValueMultiNode>(graphUniqueId, maxByValueID);
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    AZ::EntityId valueIntegerID1, valueIntegerID2, valueIntegerID3, valueIntegerID4, valueIntegerID5;

    Node* valueIntegerNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueIntegerID1);
    EXPECT_EQ(1, *valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueIntegerID2);
    EXPECT_EQ(2, *valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueIntegerID3);
    EXPECT_EQ(3, *valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueIntegerID4);
    EXPECT_EQ(4, *valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueIntegerID5);
    EXPECT_EQ(5, *valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto valueInteger1 = valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger2 = valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(valueInteger1, valueInteger2);
    EXPECT_NE(valueInteger1, valueInteger3);
    EXPECT_NE(valueInteger1, valueInteger4);
    EXPECT_NE(valueInteger1, valueInteger5);
    EXPECT_NE(valueInteger2, valueInteger3);
    EXPECT_NE(valueInteger2, valueInteger4);
    EXPECT_NE(valueInteger2, valueInteger5);
    EXPECT_NE(valueInteger3, valueInteger4);
    EXPECT_NE(valueInteger3, valueInteger5);
    EXPECT_NE(valueInteger4, valueInteger5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    EXPECT_TRUE(Connect(*graph, valueIntegerID1, "Get", maxByValueID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID2, "Get", maxByValueID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueIntegerID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID3, "Get", valueIntegerID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueIntegerID5, "Set"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByReferenceMulti)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByReferenceMultiNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByReferenceID;
    CreateTestNode<MaxReturnByReferenceMultiNode>(graphUniqueId, maxByReferenceID);
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    AZ::EntityId valueIntegerID1, valueIntegerID2, valueIntegerID3, valueIntegerID4, valueIntegerID5;

    Node* valueIntegerNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueIntegerID1);
    EXPECT_EQ(1, *valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueIntegerID2);
    EXPECT_EQ(2, *valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueIntegerID3);
    EXPECT_EQ(3, *valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueIntegerID4);
    EXPECT_EQ(4, *valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueIntegerID5);
    EXPECT_EQ(5, *valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto valueInteger1 = valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger2 = valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(valueInteger1, valueInteger2);
    EXPECT_NE(valueInteger1, valueInteger3);
    EXPECT_NE(valueInteger1, valueInteger4);
    EXPECT_NE(valueInteger1, valueInteger5);
    EXPECT_NE(valueInteger2, valueInteger3);
    EXPECT_NE(valueInteger2, valueInteger4);
    EXPECT_NE(valueInteger2, valueInteger5);
    EXPECT_NE(valueInteger3, valueInteger4);
    EXPECT_NE(valueInteger3, valueInteger5);
    EXPECT_NE(valueInteger4, valueInteger5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByReferenceID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByReferenceID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    EXPECT_TRUE(Connect(*graph, valueIntegerID1, "Get", maxByReferenceID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID2, "Get", maxByReferenceID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: Number", valueIntegerID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID3, "Get", valueIntegerID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: Number", valueIntegerID5, "Set"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByReferenceID, "In"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByPointerMulti)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    RegisterComponentDescriptor<MaxReturnByPointerMultiNode>();

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startID;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId maxByPointerID;
    CreateTestNode<MaxReturnByPointerMultiNode>(graphUniqueId, maxByPointerID);
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    AZ::EntityId valueIntegerID1, valueIntegerID2, valueIntegerID3, valueIntegerID4, valueIntegerID5;

    Node* valueIntegerNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueIntegerID1);
    EXPECT_EQ(1, *valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueIntegerID2);
    EXPECT_EQ(2, *valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueIntegerID3);
    EXPECT_EQ(3, *valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueIntegerID4);
    EXPECT_EQ(4, *valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueIntegerID5);
    EXPECT_EQ(5, *valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto valueInteger1 = valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger2 = valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(valueInteger1, valueInteger2);
    EXPECT_NE(valueInteger1, valueInteger3);
    EXPECT_NE(valueInteger1, valueInteger4);
    EXPECT_NE(valueInteger1, valueInteger5);
    EXPECT_NE(valueInteger2, valueInteger3);
    EXPECT_NE(valueInteger2, valueInteger4);
    EXPECT_NE(valueInteger2, valueInteger5);
    EXPECT_NE(valueInteger3, valueInteger4);
    EXPECT_NE(valueInteger3, valueInteger5);
    EXPECT_NE(valueInteger4, valueInteger5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByPointerID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByPointerID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    EXPECT_TRUE(Connect(*graph, valueIntegerID1, "Get", maxByPointerID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID2, "Get", maxByPointerID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueIntegerID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID3, "Get", valueIntegerID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueIntegerID5, "Set"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByPointerID, "In"));

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

#pragma warning( pop )
