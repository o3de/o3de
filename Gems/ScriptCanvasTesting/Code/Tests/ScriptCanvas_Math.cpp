/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>

#include <AzCore/Math/Vector3.h>
#include <ScriptCanvas/Libraries/Math/Math.h>
#include <ScriptCanvas/Data/Data.h>

using namespace ScriptCanvasTests;
using namespace ScriptCanvas;

template<typename t_Node>
AZStd::vector<ScriptCanvas::Datum> TestMathFunction(std::initializer_list<AZStd::string_view> inputNamesList, std::initializer_list<ScriptCanvas::Datum> inputList, std::initializer_list<AZStd::string_view> outputNamesList, std::initializer_list<ScriptCanvas::Datum> outputList)
{
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
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startID);
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
        node->SetInput_UNIT_TEST(PureData::k_setThis, input[i]);
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

    for (int i = 0; i < output.size(); ++i)
    {
        output[i] = *outputNodes[i]->GetInput_UNIT_TEST(PureData::k_setThis);
    }

    graph->GetEntity()->Deactivate();
    delete graph;
    return output;
}

TEST_F(ScriptCanvasTestFixture, MathOperations_Graph)
{
    RunUnitTestGraph("LY_SC_UnitTest_MathOperations", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, MathCustom_Graph)
{
    RunUnitTestGraph("LY_SC_UnitTest_UnitTest_MathCustom", ExecutionMode::Interpreted);
}
