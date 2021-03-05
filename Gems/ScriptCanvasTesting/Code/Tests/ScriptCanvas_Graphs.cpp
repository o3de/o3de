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

#include "precompiled.h"

#include <Tests/ScriptCanvasTestFixture.h>

using namespace ScriptCanvasTests;

TEST_F(ScriptCanvasTestFixture, StringOperations)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();
    AZ::EntityId startNodeID{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNodeID, graphUniqueId, Nodes::Core::Start::TYPEINFO_Uuid());

    AZ::Uuid typeID = azrtti_typeid<AZStd::string>();
    const AZStd::string string0("abcd");
    AZ::EntityId string0NodeID;
    auto string0NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string0NodeID);
    string0NodePtr->SetInput_UNIT_TEST("Set", string0);

    // String 1
    const AZStd::string string1("ef");
    AZ::EntityId string1NodeID;
    auto string1NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string1NodeID);
    string1NodePtr->SetInput_UNIT_TEST("Set", string1);

    // String 2
    const AZStd::string string2("   abcd");
    AZ::EntityId string2NodeID;
    auto string2NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string2NodeID);
    string2NodePtr->SetInput_UNIT_TEST("Set", string2);

    // String 3
    const AZStd::string string3("abcd   ");
    AZ::EntityId string3NodeID;
    auto string3NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string3NodeID);
    string3NodePtr->SetInput_UNIT_TEST("Set", string3);

    // String 4
    const AZStd::string string4("abcd/ef/ghi");
    AZ::EntityId string4NodeID;
    auto string4NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string4NodeID);
    string4NodePtr->SetInput_UNIT_TEST("Set", string4);

    // String 5
    const AZStd::string string5("ab");
    AZ::EntityId string5NodeID;
    auto string5NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string5NodeID);
    string5NodePtr->SetInput_UNIT_TEST("Set", string5);

    // String 6
    const AZStd::string string6("ABCD");
    AZ::EntityId string6NodeID;
    auto string6NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string6NodeID);
    string6NodePtr->SetInput_UNIT_TEST("Set", string6);

    // String 7
    const AZStd::string string7("/");
    AZ::EntityId string7NodeID;
    auto string7NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string7NodeID);
    string7NodePtr->SetInput_UNIT_TEST("Set", string7);

    //Int 0
    AZ::EntityId int0NodeID;
    auto int0NodePtr = CreateTestNode<ScriptCanvas::Nodes::Math::Number>(graphUniqueId, int0NodeID);
    int0NodePtr->SetInput_UNIT_TEST("Set", 0);

    // Int 1
    AZ::EntityId int1NodeID;
    auto int1NodePtr = CreateTestNode<ScriptCanvas::Nodes::Math::Number>(graphUniqueId, int1NodeID);
    int1NodePtr->SetInput_UNIT_TEST("Set", 2);

    // Vector 0
    const AZStd::vector<AZStd::string> vector0({ AZStd::string("abcd"), AZStd::string("ef") });
    AZ::EntityId vector0NodeID;
    auto vector0NodePtr = CreateTestObjectNode(graphUniqueId, vector0NodeID, azrtti_typeid<AZStd::vector<AZStd::string>>());
    vector0NodePtr->SetInput_UNIT_TEST("Set", vector0);

    // LengthResult
    AZ::EntityId lengthResultNodeID;
    auto lengthResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Math::Number>(graphUniqueId, lengthResultNodeID);

    // FindResult
    AZ::EntityId findResultNodeID;
    auto findResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Math::Number>(graphUniqueId, findResultNodeID);

    // SubstringResult
    AZ::EntityId substringResultNodeID;
    auto* substringResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, substringResultNodeID);

    // ReplaceResult
    AZ::EntityId replaceResultNodeID;
    auto* replaceResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, replaceResultNodeID);

    // ReplaceByIndexResult
    AZ::EntityId replaceByIndexResultNodeID;
    auto* replaceByIndexResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, replaceByIndexResultNodeID);

    // AddResult
    AZ::EntityId addResultNodeID;
    auto* addResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, addResultNodeID);

    // TrimLeftResult
    AZ::EntityId trimLeftResultNodeID;
    auto* trimLeftResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, trimLeftResultNodeID);

    // TrimRightResult
    AZ::EntityId trimRightResultNodeID;
    auto* trimRightResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, trimRightResultNodeID);

    // ToLowerResult
    AZ::EntityId toLowerResultNodeID;
    auto* toLowerResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, toLowerResultNodeID);

    // ToUpperResult
    AZ::EntityId toUpperResultNodeID;
    auto* toUpperResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, toUpperResultNodeID);

    // JoinResult
    AZ::EntityId joinResultNodeID;
    auto* joinResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, joinResultNodeID);

    // SplitResult
    AZ::EntityId splitResultNodeID;
    auto* splitResultNodePtr = CreateTestObjectNode(graphUniqueId, splitResultNodeID, azrtti_typeid<AZStd::vector<AZStd::string>>());

    const AZStd::string stringClassName("AZStd::basic_string<char, AZStd::char_traits<char>, allocator>");

    // Length
    AZ::EntityId lengthNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Length"));

    //Find
    AZ::EntityId findNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Find"));

    //Substring
    AZ::EntityId substringNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Substring"));

    //Replace
    AZ::EntityId replaceNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Replace"));

    //ReplaceByIndex
    AZ::EntityId replaceByIndexNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("ReplaceByIndex"));

    //Add
    AZ::EntityId addNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Add"));

    //TrimLeft
    AZ::EntityId trimLeftNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("TrimLeft"));

    //TrimRight
    AZ::EntityId trimRightNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("TrimRight"));

    //ToLower
    AZ::EntityId toLowerNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("ToLower"));

    //ToUpper
    AZ::EntityId toUpperNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("ToUpper"));

    //Join
    AZ::EntityId joinNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Join"));

    //Split
    AZ::EntityId splitNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Split"));

    // Flow
    ScriptCanvas::SlotId firstSlotId;
    ScriptCanvas::SlotId secondSlotId;

    NodeRequestBus::EventResult(firstSlotId, startNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, lengthNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(startNodeID, firstSlotId, lengthNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, lengthNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, findNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(lengthNodeID, firstSlotId, findNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, substringNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, substringNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, addNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, addNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, addNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, trimLeftNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(addNodeID, firstSlotId, trimLeftNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, trimLeftNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, trimRightNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimLeftNodeID, firstSlotId, trimRightNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, trimRightNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, toLowerNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimRightNodeID, firstSlotId, toLowerNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, toLowerNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, toUpperNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toLowerNodeID, firstSlotId, toUpperNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, toUpperNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, joinNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toUpperNodeID, firstSlotId, joinNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, joinNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, splitNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(joinNodeID, firstSlotId, splitNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, splitNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, replaceNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(splitNodeID, firstSlotId, replaceNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, replaceByIndexNodeID, secondSlotId));

    // Length
    NodeRequestBus::EventResult(firstSlotId, lengthNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(lengthNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, lengthNodeID, &NodeRequests::GetSlotId, "Result: Number");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, lengthResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(lengthNodeID, firstSlotId, lengthResultNodeID, secondSlotId));

    // Find
    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string5NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, string5NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "Number: 2");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, int0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "Result: Number");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, findResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, findResultNodeID, secondSlotId));

    // Substring
    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "Number: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, int0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "Number: 2");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int1NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, int1NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, substringResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, substringResultNodeID, secondSlotId));

    // Replace
    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string5NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, string5NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "String: 2");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string1NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, string1NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, replaceResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, replaceResultNodeID, secondSlotId));

    //ReplaceByIndex
    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "Number: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, int0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "Number: 2");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int1NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, int1NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "String: 3");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string5NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, string5NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, replaceByIndexResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, replaceByIndexResultNodeID, secondSlotId));

    // Add
    NodeRequestBus::EventResult(firstSlotId, addNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(addNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, addNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string1NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(addNodeID, firstSlotId, string1NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, addNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, addResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(addNodeID, firstSlotId, addResultNodeID, secondSlotId));

    //TrimLeft
    NodeRequestBus::EventResult(firstSlotId, trimLeftNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string2NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimLeftNodeID, firstSlotId, string2NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, trimLeftNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, trimLeftResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimLeftNodeID, firstSlotId, trimLeftResultNodeID, secondSlotId));

    // TrimRight
    NodeRequestBus::EventResult(firstSlotId, trimRightNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string3NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimRightNodeID, firstSlotId, string3NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, trimRightNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, trimRightResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimRightNodeID, firstSlotId, trimRightResultNodeID, secondSlotId));

    // ToLower
    NodeRequestBus::EventResult(firstSlotId, toLowerNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string6NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toLowerNodeID, firstSlotId, string6NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, toLowerNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, toLowerResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toLowerNodeID, firstSlotId, toLowerResultNodeID, secondSlotId));

    // ToUpper
    NodeRequestBus::EventResult(firstSlotId, toUpperNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toUpperNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, toUpperNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, toUpperResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toUpperNodeID, firstSlotId, toUpperResultNodeID, secondSlotId));

    // Join
    NodeRequestBus::EventResult(firstSlotId, joinNodeID, &NodeRequests::GetSlotId, "AZStd::vector<AZStd::basic_string<char, AZStd::char_traits<char>, allocator>, allocator>: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, vector0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(joinNodeID, firstSlotId, vector0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, joinNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string7NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(joinNodeID, firstSlotId, string7NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, joinNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, joinResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(joinNodeID, firstSlotId, joinResultNodeID, secondSlotId));

    // Split
    NodeRequestBus::EventResult(firstSlotId, splitNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string4NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(splitNodeID, firstSlotId, string4NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, splitNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string7NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(splitNodeID, firstSlotId, string7NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, splitNodeID, &NodeRequests::GetSlotId, "Result: AZStd::vector<AZStd::basic_string<char, AZStd::char_traits<char>, allocator>, allocator>");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, splitResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(splitNodeID, firstSlotId, splitResultNodeID, secondSlotId));

    graph->SetStartNode(startNodeID);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    if (lengthResultNodePtr)
    {
        if (auto result = lengthResultNodePtr->GetInput_UNIT_TEST<int>("Set"))
        {
            EXPECT_EQ(4, *result);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (findResultNodePtr)
    {
        if (auto result = findResultNodePtr->GetInput_UNIT_TEST<int>("Set"))
        {
            EXPECT_EQ(0, *result);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (substringResultNodePtr)
    {
        auto resultValue = substringResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("ab", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (replaceResultNodePtr)
    {
        auto resultValue = replaceResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("efcd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (replaceByIndexResultNodePtr)
    {
        auto resultValue = replaceByIndexResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (addResultNodePtr)
    {
        auto resultValue = addResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcdef", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (trimLeftResultNodePtr)
    {
        auto resultValue = trimLeftResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (trimRightResultNodePtr)
    {
        auto resultValue = trimRightResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (toLowerResultNodePtr)
    {
        auto resultValue = toLowerResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (toUpperResultNodePtr)
    {
        auto resultValue = toUpperResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("ABCD", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (joinResultNodePtr)
    {
        auto resultValue = joinResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcd/ef", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (splitResultNodePtr)
    {
        auto result = splitResultNodePtr->GetInput_UNIT_TEST<AZStd::vector<AZStd::string>>("Set");
        if (result)
        {
            AZStd::vector<AZStd::string> ans = { "abcd", "ef", "ghi" };
            EXPECT_EQ(ans, *result);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    graph->GetEntity()->Deactivate();
    delete graph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST_F(ScriptCanvasTestFixture, SimplestNotTrue)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;

    const bool s_value = true;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Nodes
    AZ::EntityId startNodeId;
    Nodes::Core::Start* startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);
    ASSERT_TRUE(startNode != nullptr);

    AZ::EntityId booleanNodeId;
    Nodes::Logic::Boolean* booleanNode = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, booleanNodeId);
    ASSERT_TRUE(booleanNode != nullptr);
    booleanNode->SetInput_UNIT_TEST("Set", s_value);

    AZ::EntityId printNodeId;
    Print* printNode = CreateTestNode<Print>(graphUniqueId, printNodeId);
    ASSERT_TRUE(printNode != nullptr);

    AZ::EntityId notNodeId;
    Nodes::Logic::Not* notNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNodeId);
    ASSERT_TRUE(notNode != nullptr);

    AZ::EntityId notNotNodeId;
    Nodes::Logic::Not* notNotNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNotNodeId);
    ASSERT_TRUE(notNotNode != nullptr);

    AZ::EntityId printNode2Id;
    Print* printNode2 = CreateTestNode<Print>(graphUniqueId, printNode2Id);
    ASSERT_TRUE(printNode2 != nullptr);

    // Start  -> Not (false) => Print (true) -> Not (true) => Print
    //          /    \_________________________/
    // Boolean /
    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", notNodeId, Nodes::UnaryOperator::k_evaluateName));

    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_valueName, booleanNodeId, PureData::k_getThis));

    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onTrue, printNodeId, "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onFalse, printNodeId, "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_resultName, printNodeId, "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeId, "Out", notNotNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_valueName, notNodeId, Nodes::UnaryOperator::k_resultName));

    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onTrue, printNode2Id, "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onFalse, printNode2Id, "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_resultName, printNode2Id, "Value"));

    graph->SetStartNode(startNodeId);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // Validate results
    {
        const bool* result = printNode->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, !s_value);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    {
        const bool* result = printNode2->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, s_value);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    graph->GetEntity()->Deactivate();

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, SimplestNotFalse)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;

    const bool s_value = false;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Nodes
    AZ::EntityId startNodeId;
    Nodes::Core::Start* startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);
    ASSERT_TRUE(startNode != nullptr);

    AZ::EntityId booleanNodeId;
    Nodes::Logic::Boolean* booleanNode = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, booleanNodeId);
    ASSERT_TRUE(booleanNode != nullptr);
    booleanNode->SetInput_UNIT_TEST("Set", s_value);

    AZ::EntityId printNodeId;
    Print* printNode = CreateTestNode<Print>(graphUniqueId, printNodeId);
    ASSERT_TRUE(printNode != nullptr);

    AZ::EntityId notNodeId;
    Nodes::Logic::Not* notNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNodeId);
    ASSERT_TRUE(notNode != nullptr);

    AZ::EntityId notNotNodeId;
    Nodes::Logic::Not* notNotNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNotNodeId);
    ASSERT_TRUE(notNotNode != nullptr);

    AZ::EntityId printNode2Id;
    Print* printNode2 = CreateTestNode<Print>(graphUniqueId, printNode2Id);
    ASSERT_TRUE(printNode2 != nullptr);

    // Start  -> Not (false) => Print (true) -> Not (true) => Print
    //          /    \_________________________/
    // Boolean /
    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", notNodeId, Nodes::UnaryOperator::k_evaluateName));

    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_valueName, booleanNodeId, PureData::k_getThis));

    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onTrue, printNodeId, "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onFalse, printNodeId, "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_resultName, printNodeId, "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeId, "Out", notNotNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_valueName, notNodeId, Nodes::UnaryOperator::k_resultName));

    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onTrue, printNode2Id, "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onFalse, printNode2Id, "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_resultName, printNode2Id, "Value"));

    graph->SetStartNode(startNodeId);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // Validate results
    {
        const bool* result = printNode->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, !s_value);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    {
        const bool* result = printNode2->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, s_value);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    graph->GetEntity()->Deactivate();

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, LogicTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Nodes
    AZ::EntityId startNodeId;
    Nodes::Core::Start* startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);

    enum BooleanNodes
    {
        False,
        True
    };

    AZ::EntityId booleanNodeIds[2];
    Nodes::Logic::Boolean* booleanNodes[2];

    booleanNodes[BooleanNodes::False] = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, booleanNodeIds[BooleanNodes::False]);
    ASSERT_TRUE(booleanNodes[BooleanNodes::False] != nullptr);
    booleanNodes[BooleanNodes::False]->SetInput_UNIT_TEST("Set", false);
    ASSERT_FALSE(*booleanNodes[BooleanNodes::False]->GetInput_UNIT_TEST<bool>("Set"));

    booleanNodes[BooleanNodes::True] = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, booleanNodeIds[BooleanNodes::True]);
    ASSERT_TRUE(booleanNodes[BooleanNodes::True] != nullptr);
    booleanNodes[BooleanNodes::True]->SetInput_UNIT_TEST("Set", true);
    ASSERT_TRUE(*booleanNodes[BooleanNodes::True]->GetInput_UNIT_TEST<bool>("Set"));

    enum PrintNodes
    {
        Or,
        And,
        Not,
        NotNot,
        Count
    };

    AZ::EntityId printNodeIds[PrintNodes::Count];
    Print* printNodes[PrintNodes::Count];

    printNodes[PrintNodes::Or] = CreateTestNode<Print>(graphUniqueId, printNodeIds[Or]);
    ASSERT_TRUE(printNodes[PrintNodes::Or] != nullptr);

    printNodes[PrintNodes::And] = CreateTestNode<Print>(graphUniqueId, printNodeIds[And]);
    ASSERT_TRUE(printNodes[PrintNodes::And] != nullptr);

    printNodes[PrintNodes::Not] = CreateTestNode<Print>(graphUniqueId, printNodeIds[Not]);
    ASSERT_TRUE(printNodes[PrintNodes::Not] != nullptr);

    printNodes[PrintNodes::NotNot] = CreateTestNode<Print>(graphUniqueId, printNodeIds[NotNot]);
    ASSERT_TRUE(printNodes[PrintNodes::NotNot] != nullptr);

    AZ::EntityId orNodeId;
    Nodes::Logic::Or* orNode = CreateTestNode<Nodes::Logic::Or>(graphUniqueId, orNodeId);

    AZ::EntityId andNodeId;
    Nodes::Logic::And* andNode = CreateTestNode<Nodes::Logic::And>(graphUniqueId, andNodeId);

    AZ::EntityId notNodeId;
    Nodes::Logic::Not* notNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNodeId);

    AZ::EntityId notNotNodeId;
    Nodes::Logic::Not* notNotNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNotNodeId);

    // Start ---------------->  And --> Print -->  Or -> Print -> Not -> Print -> NotNot -> Print
    // Boolean (true) _________/_/_____true______/_ /___ true ____/  \__ false ___/    \____true
    // Boolean (false) ____________________________/

    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", andNodeId, "In"));

    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_onTrue, printNodeIds[And], "In"));
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_onFalse, printNodeIds[And], "In"));
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_lhsName, booleanNodeIds[True], PureData::k_getThis));
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_rhsName, booleanNodeIds[True], PureData::k_getThis));
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_resultName, printNodeIds[And], "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeIds[And], "Out", orNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_onTrue, printNodeIds[Or], "In"));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_onFalse, printNodeIds[Or], "In"));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_lhsName, booleanNodeIds[True], PureData::k_getThis));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_rhsName, booleanNodeIds[False], PureData::k_getThis));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_resultName, printNodeIds[Or], "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeIds[Or], "Out", notNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onTrue, printNodeIds[Not], "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onFalse, printNodeIds[Not], "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_valueName, orNodeId, Nodes::BinaryOperator::k_resultName));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_resultName, printNodeIds[Not], "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeIds[Not], "Out", notNotNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onTrue, printNodeIds[NotNot], "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onFalse, printNodeIds[NotNot], "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_valueName, notNotNodeId, Nodes::BinaryOperator::k_resultName));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_resultName, printNodeIds[NotNot], "Value"));

    graph->SetStartNode(startNodeId);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // Validate results

    if (printNodes[And])
    {
        const bool* result = printNodes[And]->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, true);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (printNodes[Or])
    {
        const bool* result = printNodes[And]->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, true);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (printNodes[Not])
    {
        const bool* result = printNodes[Not]->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, false);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (printNodes[NotNot])
    {
        const bool* result = printNodes[NotNot]->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, true);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    graph->GetEntity()->Deactivate();

    delete graph->GetEntity();
}