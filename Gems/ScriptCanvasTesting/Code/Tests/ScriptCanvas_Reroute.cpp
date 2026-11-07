/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Framework/ScriptCanvasGraphUtilities.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/DisallowReentrantExecutionContract.h>
#include <ScriptCanvas/Core/Contracts/RestrictedNodeContract.h>
#include <ScriptCanvas/Libraries/Core/Reroute.h>
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
