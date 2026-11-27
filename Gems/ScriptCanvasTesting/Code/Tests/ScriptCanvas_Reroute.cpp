/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/StringFunc/StringFunc.h>
#include <Editor/Framework/ScriptCanvasGraphUtilities.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/DisallowReentrantExecutionContract.h>
#include <ScriptCanvas/Core/Contracts/RestrictedNodeContract.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Grammar/RerouteFreeGraph.h>
#include <ScriptCanvas/Libraries/Core/Reroute.h>
#include <ScriptCanvas/Translation/Translation.h>
#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>

using namespace ScriptCanvasTests;

class RerouteTestFixture : public ScriptCanvasTestFixture
{
protected:
    ScriptCanvas::Nodes::Core::Reroute* CreateReroute(ScriptCanvas::Nodes::Core::Reroute::Mode mode)
    {
        auto* entity = aznew AZ::Entity("Reroute");
        auto* reroute = entity->CreateComponent<ScriptCanvas::Nodes::Core::Reroute>();
        entity->Init();
        EXPECT_TRUE(m_graph->AddNode(reroute->GetEntityId()));
        reroute->ConfigureMode(mode);

        return reroute;
    }
};

TEST_F(RerouteTestFixture, DataReroutePinsCannotBecomeReferences)
{
    using namespace ScriptCanvas;

    CreateGraph();
    auto* source = CreateConfigurableNode();
    const Slot* output = source->AddTestingSlot(DataSlotConfiguration(Data::Type::Number(), "Out", ConnectionType::Output));
    auto* reroute = CreateReroute(Nodes::Core::Reroute::Mode::Data);

    for (const char* name : {"In", "Out"})
    {
        Slot* slot = reroute->GetSlot(reroute->GetSlotId(name));
        EXPECT_FALSE(slot->CanConvertTypes());
        EXPECT_FALSE(slot->CanConvertToReference());
        EXPECT_FALSE(slot->CanConvertToReference(true));
        EXPECT_FALSE(slot->ConvertToReference());
        EXPECT_FALSE(slot->IsVariableReference());
    }

    ASSERT_TRUE(m_graph->ConnectByEndpoint(output->GetEndpoint(), {reroute->GetEntityId(), reroute->GetSlotId("In")}));
    Slot* rerouteOutput = reroute->GetSlot(reroute->GetSlotId("Out"));
    EXPECT_EQ(rerouteOutput->GetDataType(), Data::Type::Number());
    EXPECT_FALSE(rerouteOutput->CanConvertToReference());
    EXPECT_FALSE(rerouteOutput->ConvertToReference());
}

TEST_F(RerouteTestFixture, ExecutionRerouteResolvesFanout)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    auto* source = CreateConfigurableNode();
    auto* first = CreateConfigurableNode();
    auto* second = CreateConfigurableNode();

    const SlotId output = source->AddTestingSlot(ExecutionSlotConfiguration("Out", ConnectionType::Output))->GetId();
    const SlotId firstInput = first->AddTestingSlot(ExecutionSlotConfiguration("In", ConnectionType::Input))->GetId();
    const SlotId secondInput = second->AddTestingSlot(ExecutionSlotConfiguration("In", ConnectionType::Input))->GetId();
    auto* reroute = CreateReroute(Nodes::Core::Reroute::Mode::Execution);

    ASSERT_TRUE(graph->ConnectByEndpoint({source->GetEntityId(), output}, {reroute->GetEntityId(), reroute->GetSlotId("In")}));
    ASSERT_TRUE(graph->ConnectByEndpoint({reroute->GetEntityId(), reroute->GetSlotId("Out")}, {first->GetEntityId(), firstInput}));
    ASSERT_TRUE(graph->ConnectByEndpoint({reroute->GetEntityId(), reroute->GetSlotId("Out")}, {second->GetEntityId(), secondInput}));

    const auto resolved = source->GetConnectedNodesForTranslation(*source->GetSlot(output));
    ASSERT_EQ(resolved.size(), 2);
    EXPECT_TRUE(AZStd::any_of(resolved.begin(), resolved.end(), [first](const auto& endpoint) { return endpoint.first == first; }));
    EXPECT_TRUE(AZStd::any_of(resolved.begin(), resolved.end(), [second](const auto& endpoint) { return endpoint.first == second; }));
}

TEST_F(RerouteTestFixture, ExecutionRerouteCycleResolvesEachEndpointOnce)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    auto* source = CreateConfigurableNode();
    auto* target = CreateConfigurableNode();
    const Slot* output = source->AddTestingSlot(ExecutionSlotConfiguration("Out", ConnectionType::Output));
    const Slot* input = target->AddTestingSlot(ExecutionSlotConfiguration("In", ConnectionType::Input));
    auto* first = CreateReroute(Nodes::Core::Reroute::Mode::Execution);
    auto* second = CreateReroute(Nodes::Core::Reroute::Mode::Execution);
    const Endpoint firstInput(first->GetEntityId(), first->GetSlotId("In"));
    const Endpoint firstOutput(first->GetEntityId(), first->GetSlotId("Out"));
    const Endpoint secondInput(second->GetEntityId(), second->GetSlotId("In"));
    const Endpoint secondOutput(second->GetEntityId(), second->GetSlotId("Out"));

    ASSERT_TRUE(graph->ConnectByEndpoint(output->GetEndpoint(), firstInput));
    ASSERT_TRUE(graph->ConnectByEndpoint(firstOutput, secondInput));
    ASSERT_TRUE(graph->ConnectByEndpoint(secondOutput, firstInput));
    ASSERT_TRUE(graph->ConnectByEndpoint(firstOutput, input->GetEndpoint()));
    ASSERT_TRUE(graph->ConnectByEndpoint(secondOutput, input->GetEndpoint()));

    const auto resolved = source->GetConnectedNodesForTranslation(*output);
    ASSERT_EQ(resolved.size(), 1);
    EXPECT_EQ(resolved.front().second, input);

    second->SetNodeDisabledFlag(NodeDisabledFlag::User);
    EXPECT_EQ(source->GetConnectedNodesForTranslation(*output).size(), 1);

    first->SetNodeDisabledFlag(NodeDisabledFlag::User);
    EXPECT_TRUE(source->GetConnectedNodesForTranslation(*output).empty());
}

class RestrictedRerouteTest
    : public RerouteTestFixture
    , public ::testing::WithParamInterface<ScriptCanvas::Nodes::Core::Reroute::Mode>
{
protected:
    ScriptCanvas::Slot* AddSlot(TestNodes::ConfigurableUnitTestNode* node, ScriptCanvas::ConnectionType direction)
    {
        using namespace ScriptCanvas;

        if (GetParam() == Nodes::Core::Reroute::Mode::Execution)
        {
            return node->AddTestingSlot(ExecutionSlotConfiguration("Slot", direction));
        }

        return node->AddTestingSlot(DataSlotConfiguration(Data::Type::Number(), "Slot", direction));
    }
};

TEST_P(RestrictedRerouteTest, RestrictedSourceCannotBeReplacedThroughChain)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    auto* permitted = CreateConfigurableNode();
    auto* other = CreateConfigurableNode();
    auto* handler = CreateConfigurableNode();
    const Endpoint permittedEndpoint = AddSlot(permitted, ConnectionType::Output)->GetEndpoint();
    const Endpoint otherEndpoint = AddSlot(other, ConnectionType::Output)->GetEndpoint();

    Slot* input = AddSlot(handler, ConnectionType::Input);
    input->AddContract({[id = permitted->GetEntityId()]() -> Contract*
    {
        return aznew RestrictedNodeContract(id);
    }});
    input->AddContract({[]() -> Contract*
    {
        return aznew ConnectionLimitContract(1);
    }});

    const Endpoint handlerEndpoint = input->GetEndpoint();
    ASSERT_FALSE(graph->CanCreateConnectionBetween(otherEndpoint, handlerEndpoint).IsSuccess());

    auto* reroute = CreateReroute(GetParam());
    auto* second = CreateReroute(GetParam());
    const Endpoint rerouteInput(reroute->GetEntityId(), reroute->GetSlotId("In"));
    const Endpoint rerouteOutput(reroute->GetEntityId(), reroute->GetSlotId("Out"));

    ASSERT_TRUE(graph->ConnectByEndpoint(permittedEndpoint, rerouteInput));
    ASSERT_TRUE(graph->ConnectByEndpoint(rerouteOutput, {second->GetEntityId(), second->GetSlotId("In")}));
    ASSERT_TRUE(graph->ConnectByEndpoint({second->GetEntityId(), second->GetSlotId("Out")}, handlerEndpoint));

    ASSERT_TRUE(graph->DisconnectByEndpoint(permittedEndpoint, rerouteInput));
    EXPECT_FALSE(graph->CanCreateConnectionBetween(otherEndpoint, rerouteInput).IsSuccess());
    EXPECT_FALSE(graph->CanCreateConnectionBetween(rerouteInput, otherEndpoint).IsSuccess());
    EXPECT_TRUE(graph->CanCreateConnectionBetween(permittedEndpoint, rerouteInput).IsSuccess());
    EXPECT_TRUE(graph->CanCreateConnectionBetween(rerouteInput, permittedEndpoint).IsSuccess());

    reroute->SetNodeDisabledFlag(NodeDisabledFlag::User);
    handler->SetNodeDisabledFlag(NodeDisabledFlag::User);
    EXPECT_FALSE(graph->CanCreateConnectionBetween(otherEndpoint, rerouteInput).IsSuccess());
    EXPECT_TRUE(graph->CanCreateConnectionBetween(permittedEndpoint, rerouteInput).IsSuccess());
}

INSTANTIATE_TEST_SUITE_P(DataAndExecution, RestrictedRerouteTest,
    ::testing::Values(ScriptCanvas::Nodes::Core::Reroute::Mode::Data, ScriptCanvas::Nodes::Core::Reroute::Mode::Execution));

TEST_F(RerouteTestFixture, RestrictedInputRejectsMixedSourcesThroughReroute)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    auto* permitted = CreateConfigurableNode();
    auto* other = CreateConfigurableNode();
    auto* handler = CreateConfigurableNode();
    const Slot* permittedOutput = permitted->AddTestingSlot(ExecutionSlotConfiguration("Out", ConnectionType::Output));
    const Slot* otherOutput = other->AddTestingSlot(ExecutionSlotConfiguration("Out", ConnectionType::Output));

    ExecutionSlotConfiguration restrictedInput("Connect", ConnectionType::Input);
    restrictedInput.m_contractDescs.emplace_back([id = permitted->GetEntityId()]() -> Contract*
    {
        return aznew RestrictedNodeContract(id);
    });
    const Slot* input = handler->AddTestingSlot(restrictedInput);
    auto* reroute = CreateReroute(Nodes::Core::Reroute::Mode::Execution);
    const Endpoint rerouteInput(reroute->GetEntityId(), reroute->GetSlotId("In"));
    const Endpoint rerouteOutput(reroute->GetEntityId(), reroute->GetSlotId("Out"));

    ASSERT_TRUE(graph->ConnectByEndpoint(permittedOutput->GetEndpoint(), rerouteInput));
    ASSERT_TRUE(graph->ConnectByEndpoint(otherOutput->GetEndpoint(), rerouteInput));
    EXPECT_FALSE(graph->CanCreateConnectionBetween(rerouteOutput, input->GetEndpoint()).IsSuccess());
}

TEST_F(RerouteTestFixture, DataChainPreservesTypeChecksAndResolvesBothDirections)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    auto* source = CreateConfigurableNode();
    auto* target = CreateConfigurableNode();
    const Slot* output = source->AddTestingSlot(DataSlotConfiguration(Data::Type::Number(), "Out", ConnectionType::Output));
    const Slot* input = target->AddTestingSlot(DataSlotConfiguration(Data::Type::Number(), "In", ConnectionType::Input));
    const Slot* wrongType = target->AddTestingSlot(DataSlotConfiguration(Data::Type::String(), "String", ConnectionType::Input));
    auto* first = CreateReroute(Nodes::Core::Reroute::Mode::Data);
    auto* second = CreateReroute(Nodes::Core::Reroute::Mode::Data);
    const Endpoint secondOutput(second->GetEntityId(), second->GetSlotId("Out"));

    ASSERT_TRUE(graph->ConnectByEndpoint(output->GetEndpoint(), {first->GetEntityId(), first->GetSlotId("In")}));
    ASSERT_TRUE(graph->ConnectByEndpoint(
        {first->GetEntityId(), first->GetSlotId("Out")}, {second->GetEntityId(), second->GetSlotId("In")}));
    EXPECT_FALSE(graph->CanCreateConnectionBetween(secondOutput, wrongType->GetEndpoint()).IsSuccess());
    ASSERT_TRUE(graph->ConnectByEndpoint(secondOutput, input->GetEndpoint()));

    const auto downstream = source->GetConnectedNodesForTranslation(*output);
    const auto upstream = target->GetConnectedNodesForTranslation(*input);
    ASSERT_EQ(downstream.size(), 1);
    ASSERT_EQ(upstream.size(), 1);
    EXPECT_EQ(downstream.front().second, input);
    EXPECT_EQ(upstream.front().second, output);
}

class ReentrantRerouteTest
    : public RerouteTestFixture
    , public ::testing::WithParamInterface<bool>
{
};

TEST_P(ReentrantRerouteTest, ReroutePreservesSelfConnectionProhibition)
{
    using namespace ScriptCanvas;

    Graph* graph = CreateGraph();
    auto* node = CreateConfigurableNode();
    const Slot* output = node->AddTestingSlot(ExecutionSlotConfiguration("Out", ConnectionType::Output));

    ExecutionSlotConfiguration inputConfig("In", ConnectionType::Input);
    inputConfig.m_contractDescs.emplace_back([]() -> Contract*
    {
        return aznew DisallowReentrantExecutionContract();
    });

    const Slot* input = node->AddTestingSlot(inputConfig);
    const Endpoint source = output->GetEndpoint();
    const Endpoint target = input->GetEndpoint();
    ASSERT_FALSE(graph->CanCreateConnectionBetween(source, target).IsSuccess());

    auto* reroute = CreateReroute(Nodes::Core::Reroute::Mode::Execution);
    const Endpoint rerouteInput(reroute->GetEntityId(), reroute->GetSlotId("In"));
    const Endpoint rerouteOutput(reroute->GetEntityId(), reroute->GetSlotId("Out"));
    Endpoint proposedSource;
    Endpoint proposedTarget;
    if (GetParam())
    {
        ASSERT_TRUE(graph->ConnectByEndpoint(source, rerouteInput));
        proposedSource = rerouteOutput;
        proposedTarget = target;
    }
    else
    {
        ASSERT_TRUE(graph->ConnectByEndpoint(rerouteOutput, target));
        proposedSource = source;
        proposedTarget = rerouteInput;
    }

    const auto result = graph->CanCreateConnectionBetween(proposedSource, proposedTarget);
    EXPECT_FALSE(result.IsSuccess()) << "Reroute accepted a self-connection prohibited by the concrete input contract";
    EXPECT_FALSE(graph->CanCreateConnectionBetween(proposedTarget, proposedSource).IsSuccess());
}

INSTANTIATE_TEST_SUITE_P(BothWiringOrders, ReentrantRerouteTest, ::testing::Values(false, true));

TEST_F(ScriptCanvasTestFixture, TranslatedDataAndExecutionReroutesPreserveResults)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvasEditor;

    const AZStd::string path = AZStd::string::format("%s/LY_SC_UnitTest_OperatorAdd.scriptcanvas", GetUnitTestDirPathRelative());
    RunGraphSpec spec;
    spec.graphPath = path;
    spec.dirPath = GetUnitTestDirPathRelative();

    Reporter baseline;
    baseline.SetFilePath(spec.graphPath);
    baseline.SetExecutionMode(ExecutionMode::Interpreted);
    baseline.SetExecutionConfiguration(ExecutionConfiguration::Release);

    RunGraphImplementation(spec, baseline);
    VerifyReporter(baseline);
    ASSERT_TRUE(baseline.IsComplete());
    ASSERT_FALSE(baseline.GetCheckpoints().empty());

    Reporter result;
    result.SetFilePath(spec.graphPath);
    result.SetExecutionMode(ExecutionMode::Interpreted);
    result.SetExecutionConfiguration(ExecutionConfiguration::Release);
    {
        auto routed = LoadTestGraph(path);
        ASSERT_TRUE(routed.m_editorAsset.Mod());
        Graph* graph = routed.m_editorAsset.Mod();
        ASSERT_TRUE(graph);

        const auto connections = graph->GetGraphData()->m_connections;
        size_t dataReroutes = 0;
        size_t executionReroutes = 0;
        for (const AZ::Entity* entity : connections)
        {
            const auto* connection = entity->FindComponent<Connection>();
            ASSERT_TRUE(connection);

            const Endpoint source = connection->GetSourceEndpoint();
            const Endpoint target = connection->GetTargetEndpoint();
            const Slot* slot = graph->FindNode(source.GetNodeId())->GetSlot(source.GetSlotId());
            ASSERT_TRUE(slot);

            Nodes::Core::Reroute::Mode mode;
            if (slot->IsData())
            {
                if (dataReroutes > 0)
                {
                    continue;
                }

                mode = Nodes::Core::Reroute::Mode::Data;
                ++dataReroutes;
            }
            else
            {
                if (executionReroutes > 0)
                {
                    continue;
                }

                mode = Nodes::Core::Reroute::Mode::Execution;
                ++executionReroutes;
            }

            auto* rerouteEntity = aznew AZ::Entity("Reroute");
            auto* reroute = rerouteEntity->CreateComponent<Nodes::Core::Reroute>();
            rerouteEntity->Init();
            ASSERT_TRUE(graph->AddNode(reroute->GetEntityId()));
            reroute->ConfigureMode(mode);

            ASSERT_TRUE(graph->DisconnectByEndpoint(source, target));
            ASSERT_TRUE(graph->ConnectByEndpoint(source, {reroute->GetEntityId(), reroute->GetSlotId("In")}));
            ASSERT_TRUE(graph->ConnectByEndpoint({reroute->GetEntityId(), reroute->GetSlotId("Out")}, target));

            if (dataReroutes > 0 && executionReroutes > 0)
            {
                break;
            }
        }

        ASSERT_EQ(dataReroutes, 1);
        ASSERT_EQ(executionReroutes, 1);
        RunGraphImplementation(spec, routed, result);
    }

    VerifyReporter(result);
    EXPECT_TRUE(result.IsComplete());
    EXPECT_EQ(result.GetSuccess(), baseline.GetSuccess());
    EXPECT_EQ(result.GetCheckpoints(), baseline.GetCheckpoints());
}

namespace RerouteLuaEquivalence
{
    using namespace ScriptCanvas;

    //! Everything translation produces that could reach the runtime: validity, the Lua text and the validation events.
    struct Translated
    {
        bool m_modelValid = false;
        AZStd::string m_lua;
        AZStd::vector<AZStd::string> m_events;
    };

    bool IsConstantLocalDeclaration(AZStd::string_view line)
    {
        while (!line.empty() && (line.front() == '\t' || line.front() == ' '))
        {
            line.remove_prefix(1);
        }
        const size_t equals = line.find(" = ");
        if (!line.starts_with("local ") || equals == AZStd::string_view::npos)
        {
            return false;
        }
        const AZStd::string_view value = line.substr(equals + 3);
        if (value == "true" || value == "false" || value == "nil" || (value.starts_with("[[") && value.ends_with("]]")))
        {
            return true;
        }
        return !value.empty() && value.find_first_not_of("-+.0123456789e") == AZStd::string_view::npos;
    }

    //! The translator writes a function's constant-initialized locals from a pointer-keyed set, so their order
    //! changes from build to build with or without reroutes. Sorting each run of them leaves every other line in place.
    AZStd::string SortConstantLocalDeclarations(const AZStd::string& lua)
    {
        AZStd::vector<AZStd::string> lines;
        AZ::StringFunc::Tokenize(lua, lines, '\n', true, true);
        for (size_t begin = 0; begin < lines.size();)
        {
            size_t end = begin;
            while (end < lines.size() && IsConstantLocalDeclaration(lines[end]))
            {
                ++end;
            }
            AZStd::sort(lines.begin() + begin, lines.begin() + end);
            begin = end == begin ? begin + 1 : end;
        }
        AZStd::string sorted;
        AZ::StringFunc::Join(sorted, lines.begin(), lines.end(), "\n");
        return sorted;
    }

    Translated TranslateGraph(const Graph& graph)
    {
        Grammar::Request request;
        request.graph = &graph;
        request.name = "RerouteLuaEquivalence";
        const Translation::Result result = Translation::ToLua(request);

        Translated translated;
        translated.m_modelValid = result.IsModelValid();
        if (const auto lua = result.m_translations.find(Translation::TargetFlags::Lua); lua != result.m_translations.end())
        {
            translated.m_lua = SortConstantLocalDeclarations(lua->second.m_text);
        }
        if (result.m_model)
        {
            for (const auto& event : result.m_model->GetValidationEvents())
            {
                translated.m_events.push_back(AZStd::string::format(
                    "%s %.*s", event->GetIdentifier().c_str(), AZ_STRING_ARG(event->GetDescription())));
            }
        }
        return translated;
    }

    Nodes::Core::Reroute* AddReroute(Graph& graph, Nodes::Core::Reroute::Mode mode)
    {
        auto* entity = aznew AZ::Entity("Reroute");
        auto* reroute = entity->CreateComponent<Nodes::Core::Reroute>();
        entity->Init();
        EXPECT_TRUE(graph.AddNode(reroute->GetEntityId()));
        reroute->ConfigureMode(mode);
        return reroute;
    }

    Endpoint RerouteIn(const Nodes::Core::Reroute* reroute)
    {
        return { reroute->GetEntityId(), reroute->GetSlotId("In") };
    }

    Endpoint RerouteOut(const Nodes::Core::Reroute* reroute)
    {
        return { reroute->GetEntityId(), reroute->GetSlotId("Out") };
    }

    AZStd::vector<AZStd::pair<Endpoint, Endpoint>> GetConnections(const Graph& graph)
    {
        AZStd::vector<AZStd::pair<Endpoint, Endpoint>> connections;
        for (const AZ::Entity* entity : graph.GetGraphDataConst()->m_connections)
        {
            if (const auto* connection = entity ? entity->FindComponent<Connection>() : nullptr)
            {
                Endpoint source = connection->GetSourceEndpoint();
                Endpoint target = connection->GetTargetEndpoint();
                // Some older graphs store a connection from its input end.
                if (const Node* node = graph.FindNode(source.GetNodeId()); node && node->GetSlot(source.GetSlotId())->IsInput())
                {
                    AZStd::swap(source, target);
                }
                connections.emplace_back(source, target);
            }
        }
        return connections;
    }

    Nodes::Core::Reroute::Mode ModeOf(const Graph& graph, const Endpoint& source)
    {
        const Slot* slot = graph.FindNode(source.GetNodeId())->GetSlot(source.GetSlotId());
        return slot->IsData() ? Nodes::Core::Reroute::Mode::Data : Nodes::Core::Reroute::Mode::Execution;
    }

    bool EndpointLess(const Endpoint& lhs, const Endpoint& rhs)
    {
        return lhs.GetNodeId() < rhs.GetNodeId();
    }

    //! Routes every connection of a unit test graph through two reroutes, then wires it directly again,
    //! and expects the same Lua both ways.
    void ExpectRoutedGraphTranslatesLikeDirectGraph(const char* graphName)
    {
        auto loaded = ScriptCanvasEditor::LoadTestGraph(
            AZStd::string::format("%s/%s.scriptcanvas", GetUnitTestDirPathRelative(), graphName));
        ASSERT_TRUE(loaded.m_editorAsset.Mod());
        Graph& graph = *loaded.m_editorAsset.Mod();
        ASSERT_TRUE(TranslateGraph(graph).m_modelValid);

        auto connect = [&graph](const Endpoint& from, const Endpoint& to)
        {
            const auto canConnect = graph.CanCreateConnectionBetween(from, to);
            EXPECT_TRUE(canConnect.IsSuccess()) << canConnect.GetError().c_str();
            return canConnect.IsSuccess() && graph.ConnectByEndpoint(from, to);
        };

        // Every connection becomes source -> reroute -> reroute -> target.
        struct Chain
        {
            Endpoint m_source;
            Endpoint m_target;
            Nodes::Core::Reroute* m_first;
            Nodes::Core::Reroute* m_second;
        };
        AZStd::vector<Chain> chains;
        size_t routedExecution = 0;
        for (const auto& [source, target] : GetConnections(graph))
        {
            const Nodes::Core::Reroute::Mode mode = ModeOf(graph, source);
            Chain chain{ source, target, AddReroute(graph, mode), AddReroute(graph, mode) };
            ASSERT_TRUE(graph.DisconnectByEndpoint(source, target));
            ASSERT_TRUE(connect(source, RerouteIn(chain.m_first)));
            ASSERT_TRUE(connect(RerouteOut(chain.m_first), RerouteIn(chain.m_second)));
            ASSERT_TRUE(connect(RerouteOut(chain.m_second), target))
                << "routing " << graph.FindNode(source.GetNodeId())->GetNodeName().c_str() << " to "
                << graph.FindNode(target.GetNodeId())->GetNodeName().c_str();
            chains.push_back(chain);
            routedExecution += mode == Nodes::Core::Reroute::Mode::Execution ? 1 : 0;
        }
        ASSERT_GT(routedExecution, 0);

        const Translated routed = TranslateGraph(graph);
        EXPECT_TRUE(routed.m_modelValid);

        // Translating must leave the editor graph exactly as the user laid it out.
        for (const Chain& chain : chains)
        {
            for (const auto* reroute : { chain.m_first, chain.m_second })
            {
                EXPECT_TRUE(graph.GetGraphDataConst()->m_nodes.contains(reroute->GetEntity()));
                EXPECT_TRUE(graph.IsEndpointConnected(RerouteIn(reroute)));
                EXPECT_TRUE(graph.IsEndpointConnected(RerouteOut(reroute)));
            }
        }

        // Now remove the reroutes and wire each connection directly again: the Lua must not change.
        for (const Chain& chain : chains)
        {
            ASSERT_TRUE(graph.DisconnectByEndpoint(chain.m_source, RerouteIn(chain.m_first)));
            ASSERT_TRUE(graph.DisconnectByEndpoint(RerouteOut(chain.m_first), RerouteIn(chain.m_second)));
            ASSERT_TRUE(graph.DisconnectByEndpoint(RerouteOut(chain.m_second), chain.m_target));
            for (auto* reroute : { chain.m_first, chain.m_second })
            {
                AZ::Entity* entity = reroute->GetEntity();
                ASSERT_TRUE(graph.RemoveNode(entity->GetId()));
                delete entity;
            }
            ASSERT_TRUE(connect(chain.m_source, chain.m_target));
        }

        const Translated direct = TranslateGraph(graph);
        EXPECT_EQ(routed.m_modelValid, direct.m_modelValid);
        EXPECT_EQ(routed.m_lua, direct.m_lua);
        EXPECT_EQ(routed.m_events, direct.m_events);
    }
}

// One test in ScriptCanvasTestFixture rather than a suite of its own: like the fixture's other translating tests,
// it relies on the fixture's suite-level ScriptCanvas environment.
TEST_F(ScriptCanvasTestFixture, RoutingEveryConnectionLeavesTheGeneratedLuaUnchanged)
{
    for (const char* graphName :
         {
             "LY_SC_UnitTest_AndBranch",
             "LY_SC_UnitTest_CompactNodeChain",
             "LY_SC_UnitTest_ForEachNode",
             "LY_SC_UnitTest_DisabledNode",
             "LY_SC_UnitTest_MultipleOut",
             "LY_SC_UnitTest_MultipleOutFromRoot",
             "LY_SC_UnitTest_MultipleOutFromIf",
             "LY_SC_UnitTest_MultipleOutDataFlow",
             "LY_SC_UnitTest_IfBranch",
             "LY_SC_UnitTest_Switch",
             "LY_SC_UnitTest_While",
             "LY_SC_UnitTest_WhileBreak",
             "LY_SC_UnitTest_Once",
             "LY_SC_UnitTest_OrderedSequencer",
             "LY_SC_UnitTest_NodeableDelay",
             "LY_SC_UnitTest_NodeableRepeater",
             "LY_SC_UnitTest_VariableChangeEvent",
             "LY_SC_UnitTest_VariableReferences",
             "LY_SC_UnitTest_MathExpression",
             "LY_SC_UnitTest_StringFormat",
             "LY_SC_UnitTest_PathologicalFlowOfControl",
             "LY_SC_UnitTest_EventHandlingLocal",
             "LY_SC_UnitTest_PropertiesByConnection",
             "LY_SC_UnitTest_RandomSignal",
             "LY_SC_UnitTest_MultipleReturnResults"
         })
    {
        SCOPED_TRACE(graphName);
        RerouteLuaEquivalence::ExpectRoutedGraphTranslatesLikeDirectGraph(graphName);
    }
}

TEST_F(ScriptCanvasTestFixture, DanglingReroutesTranslateLikeUnconnectedPorts)
{
    using namespace ScriptCanvas;
    using namespace RerouteLuaEquivalence;

    auto loaded = ScriptCanvasEditor::LoadTestGraph(
        AZStd::string::format("%s/LY_SC_UnitTest_OperatorAdd.scriptcanvas", GetUnitTestDirPathRelative()));
    ASSERT_TRUE(loaded.m_editorAsset.Mod());
    Graph& graph = *loaded.m_editorAsset.Mod();

    // Cut one data and one execution connection.
    AZStd::vector<AZStd::pair<Endpoint, Endpoint>> cut;
    bool cutData = false;
    bool cutExecution = false;
    for (const auto& [source, target] : GetConnections(graph))
    {
        bool& alreadyCut = ModeOf(graph, source) == Nodes::Core::Reroute::Mode::Data ? cutData : cutExecution;
        if (!alreadyCut)
        {
            alreadyCut = true;
            cut.emplace_back(source, target);
            ASSERT_TRUE(graph.DisconnectByEndpoint(source, target));
        }
    }
    ASSERT_TRUE(cutData && cutExecution);

    const Translated unconnected = TranslateGraph(graph);

    // Hang an unfinished reroute chain off both ends of each cut: source -> reroute, and reroute -> reroute -> target.
    for (const auto& [source, target] : cut)
    {
        const Nodes::Core::Reroute::Mode mode = ModeOf(graph, source);
        auto* fromSource = AddReroute(graph, mode);
        auto* unfed = AddReroute(graph, mode);
        auto* toTarget = AddReroute(graph, mode);
        ASSERT_TRUE(graph.ConnectByEndpoint(source, RerouteIn(fromSource)));
        ASSERT_TRUE(graph.ConnectByEndpoint(RerouteOut(unfed), RerouteIn(toTarget)));
        ASSERT_TRUE(graph.ConnectByEndpoint(RerouteOut(toTarget), target));
    }
    // A reroute connected to nothing at all.
    AddReroute(graph, Nodes::Core::Reroute::Mode::Data);

    const Translated dangling = TranslateGraph(graph);
    EXPECT_EQ(dangling.m_modelValid, unconnected.m_modelValid);
    EXPECT_EQ(dangling.m_lua, unconnected.m_lua);
    EXPECT_EQ(dangling.m_events, unconnected.m_events);
}

TEST_F(RerouteTestFixture, RerouteFreeEndpointMapSplicesChainsAndDropsDanglingEnds)
{
    using namespace ScriptCanvas;
    using namespace RerouteLuaEquivalence;

    Graph* graph = CreateGraph();
    auto* source = CreateConfigurableNode();
    auto* targetA = CreateConfigurableNode();
    auto* targetB = CreateConfigurableNode();
    const Endpoint out = source->AddTestingSlot(ExecutionSlotConfiguration("Out", ConnectionType::Output))->GetEndpoint();
    const Endpoint inA = targetA->AddTestingSlot(ExecutionSlotConfiguration("In", ConnectionType::Input))->GetEndpoint();
    const Endpoint inB = targetB->AddTestingSlot(ExecutionSlotConfiguration("In", ConnectionType::Input))->GetEndpoint();

    // out -> trunk -> {inA, inB}, a second route out -> other -> inA, and a dangling reroute off out.
    auto* trunk = CreateReroute(Nodes::Core::Reroute::Mode::Execution);
    auto* other = CreateReroute(Nodes::Core::Reroute::Mode::Execution);
    auto* dangling = CreateReroute(Nodes::Core::Reroute::Mode::Execution);
    ASSERT_TRUE(graph->ConnectByEndpoint(out, RerouteIn(trunk)));
    ASSERT_TRUE(graph->ConnectByEndpoint(RerouteOut(trunk), inA));
    ASSERT_TRUE(graph->ConnectByEndpoint(RerouteOut(trunk), inB));
    ASSERT_TRUE(graph->ConnectByEndpoint(out, RerouteIn(other)));
    ASSERT_TRUE(graph->ConnectByEndpoint(RerouteOut(other), inA));
    ASSERT_TRUE(graph->ConnectByEndpoint(out, RerouteIn(dangling)));

    const auto map = Grammar::MakeRerouteFreeEndpointMap(*graph->GetGraphDataConst());
    auto partners = [&map](const Endpoint& endpoint)
    {
        AZStd::vector<Endpoint> found;
        const auto [begin, end] = map.equal_range(endpoint);
        for (auto iter = begin; iter != end; ++iter)
        {
            found.push_back(iter->second);
        }
        AZStd::sort(found.begin(), found.end(), &EndpointLess);
        return found;
    };

    AZStd::vector<Endpoint> expectedOut{ inA, inB };
    AZStd::sort(expectedOut.begin(), expectedOut.end(), &EndpointLess);
    EXPECT_EQ(partners(out), expectedOut);
    EXPECT_EQ(partners(inA), AZStd::vector<Endpoint>{ out });
    EXPECT_EQ(partners(inB), AZStd::vector<Endpoint>{ out });
    EXPECT_EQ(map.size(), 4);
    for (const auto* reroute : { trunk, other, dangling })
    {
        EXPECT_EQ(map.count(RerouteIn(reroute)), 0);
        EXPECT_EQ(map.count(RerouteOut(reroute)), 0);
    }
}
