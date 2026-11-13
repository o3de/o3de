/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <AzCore/std/algorithm.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Libraries/Core/Reroute.h>
#include <QApplication>

namespace
{
    class SplicingModel
        : public GraphCanvas::GraphModelRequestBus::Handler
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        void Connect(const GraphCanvas::GraphId& scene)
        {
            GraphCanvas::GraphModelRequestBus::Handler::BusConnect(scene);
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(scene);
        }

        void Disconnect()
        {
            GraphCanvas::GraphModelRequestBus::Handler::BusDisconnect();
            GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        }

        void OnConnectionRemoved(const AZ::EntityId& id) override
        {
            m_connections.erase(id);
        }

        void RequestUndoPoint() override
        {
        }

        void RequestPushPreventUndoStateUpdate() override
        {
            ++m_undoDepth;
        }

        void RequestPopPreventUndoStateUpdate() override
        {
            --m_undoDepth;
        }

        void TriggerUndo() override
        {
        }

        void TriggerRedo() override
        {
        }

        void DisconnectConnection(const GraphCanvas::ConnectionId& id) override
        {
            m_connections.erase(id);
        }

        bool CreateConnection(
            const GraphCanvas::ConnectionId& id,
            const GraphCanvas::Endpoint& source, const GraphCanvas::Endpoint& target) override
        {
            if (!IsValidConnection(source, target))
            {
                return false;
            }

            m_connections[id] = {source, target};
            return true;
        }

        bool IsValidConnection(const GraphCanvas::Endpoint& source, const GraphCanvas::Endpoint& target) const override
        {
            return source.GetSlotId() != m_rejectedSlot && target.GetSlotId() != m_rejectedSlot;
        }

        AZStd::string GetDataTypeString(const AZ::Uuid&) override { return {}; }

        void OnSaveDataDirtied(const AZ::EntityId&) override
        {
        }

        void OnRemoveUnusedNodes() override
        {
        }

        void OnRemoveUnusedElements() override
        {
        }

        void ResetSlotToDefaultValue(const GraphCanvas::Endpoint&) override
        {
        }

        GraphCanvas::SlotId m_rejectedSlot;
        AZStd::unordered_map<GraphCanvas::ConnectionId, GraphCanvas::ConnectionEndpoints> m_connections;
        int m_undoDepth = 0;
    };

    struct SplicingPorts
    {
        GraphCanvas::NodeId m_node;
        GraphCanvas::Endpoint m_input;
        GraphCanvas::Endpoint m_output;
    };

    class PropertyDisplayState : public GraphCanvas::NodePropertyRequestBus::Handler
    {
    public:
        explicit PropertyDisplayState(const GraphCanvas::SlotId& slot)
        {
            GraphCanvas::NodePropertyRequestBus::EventResult(
                m_display, slot, &GraphCanvas::NodePropertyRequests::GetNodePropertyDisplay);
            BusConnect(slot);
        }

        void SetNodePropertyDisplay(GraphCanvas::NodePropertyDisplay* display) override { m_display = display; }
        GraphCanvas::NodePropertyDisplay* GetNodePropertyDisplay() const override { return m_display; }
        void SetDisabled(bool disabled) override { m_disabled = disabled; }

        GraphCanvas::NodePropertyDisplay* m_display = nullptr;
        bool m_disabled = true;
    };

    GraphCanvas::Endpoint MapEndpoint(const ScriptCanvas::Endpoint& endpoint)
    {
        using namespace ScriptCanvasEditor;
        GraphCanvas::NodeId node;
        SceneMemberMappingRequestBus::EventResult(
            node, endpoint.GetNodeId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);
        GraphCanvas::SlotId slot;
        SlotMappingRequestBus::EventResult(slot, endpoint.GetNodeId(), &SlotMappingRequests::MapToGraphCanvasId, endpoint.GetSlotId());
        return {node, slot};
    }
}

class RerouteSplicingTest : public ScriptCanvasTests::ScriptCanvasTestFixture
{
protected:
    static void SetUpTestCase()
    {
        if (!QApplication::instance())
        {
            s_qt = AZStd::make_unique<QApplication>(s_argc, s_argv);
        }

        ScriptCanvasTestFixture::SetUpTestCase();
    }

    static void TearDownTestCase()
    {
        ScriptCanvasTestFixture::TearDownTestCase();
        s_qt.reset();
    }

    void SetUp() override
    {
        ScriptCanvasTestFixture::SetUp();

        AZ::Entity* scene = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(scene, &GraphCanvas::GraphCanvasRequests::CreateSceneAndActivate);
        ASSERT_TRUE(scene);
        m_scene.reset(scene);
        m_model.Connect(scene->GetId());
    }

    void TearDown() override
    {
        m_scene.reset();
        m_model.Disconnect();
        ScriptCanvasTestFixture::TearDown();
    }

    SplicingPorts AddNode(bool input, bool output)
    {
        AZ::Entity* node = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(node, &GraphCanvas::GraphCanvasRequests::CreateGeneralNodeAndActivate, "");
        EXPECT_TRUE(node);

        SplicingPorts ports;
        ports.m_node = node->GetId();
        for (auto direction : {GraphCanvas::ConnectionType::CT_Input, GraphCanvas::ConnectionType::CT_Output})
        {
            if ((direction == GraphCanvas::ConnectionType::CT_Input && !input)
                || (direction == GraphCanvas::ConnectionType::CT_Output && !output))
            {
                continue;
            }

            GraphCanvas::ExecutionSlotConfiguration config;
            config.m_name = "Slot";
            config.m_connectionType = direction;
            config.m_slotGroup = GraphCanvas::SlotGroups::ExecutionGroup;

            AZ::Entity* slot = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slot, &GraphCanvas::GraphCanvasRequests::CreateSlot, ports.m_node, config);
            EXPECT_TRUE(slot);
            slot->Init();
            slot->Activate();
            GraphCanvas::NodeRequestBus::Event(ports.m_node, &GraphCanvas::NodeRequests::AddSlot, slot->GetId());

            if (direction == GraphCanvas::ConnectionType::CT_Input)
            {
                ports.m_input = {ports.m_node, slot->GetId()};
            }
            else
            {
                ports.m_output = {ports.m_node, slot->GetId()};
            }
        }

        GraphCanvas::SceneRequestBus::Event(m_scene->GetId(), &GraphCanvas::SceneRequests::AddNode, ports.m_node, AZ::Vector2::CreateZero(), false);

        return ports;
    }

    GraphCanvas::ConnectionId Connect(const GraphCanvas::Endpoint& source, const GraphCanvas::Endpoint& target)
    {
        GraphCanvas::ConnectionId id;
        GraphCanvas::SceneRequestBus::EventResult(id, m_scene->GetId(), &GraphCanvas::SceneRequests::CreateConnectionBetween, source, target);
        return id;
    }

    bool HasConnection(const GraphCanvas::Endpoint& source, const GraphCanvas::Endpoint& target) const
    {
        return AZStd::any_of(m_model.m_connections.begin(), m_model.m_connections.end(), [&](const auto& item)
        {
            return item.second.m_sourceEndpoint == source && item.second.m_targetEndpoint == target;
        });
    }

    inline static int s_argc = 1;
    inline static char s_name[] = "RerouteSplicingTest";
    inline static char* s_argv[] = {s_name, nullptr};
    inline static AZStd::unique_ptr<QApplication> s_qt;

    AZStd::unique_ptr<AZ::Entity> m_scene;
    SplicingModel m_model;
};

TEST_F(RerouteSplicingTest, UnfinishedRerouteChainShowsFallbackAcrossDisconnectAndReconnect)
{
    using namespace ScriptCanvas;

    auto data = ScriptCanvasEditor::EditorGraph::Create();
    ASSERT_TRUE(data);
    auto* graph = data->m_scriptCanvasEntity->FindComponent<ScriptCanvasEditor::EditorGraph>();
    ASSERT_NE(graph, nullptr);
    auto addNode = [&]()
    {
        auto* entity = aznew AZ::Entity("ConfigurableNode");
        auto* node = entity->CreateComponent<TestNodes::ConfigurableUnitTestNode>();
        entity->Init();
        EXPECT_TRUE(graph->AddNode(node->GetEntityId()));
        return node;
    };
    auto* source = addNode();
    const Slot* output = source->AddTestingSlot(DataSlotConfiguration(Data::Type::Number(), "Out", ConnectionType::Output));
    AZStd::vector<Slot*> inputs;
    for (int i = 0; i < 2; ++i)
    {
        auto* target = addNode();
        inputs.push_back(target->AddTestingSlot(DataSlotConfiguration(Data::Type::Number(), "In", ConnectionType::Input)));
        ModifiableDatumView datum;
        ASSERT_TRUE(inputs.back()->FindModifiableDatumView(datum));
        datum.SetAs<Data::NumberType>(42.0);
    }

    AZStd::vector<Nodes::Core::Reroute*> reroutes;
    for (int i = 0; i < 3; ++i)
    {
        auto* entity = aznew AZ::Entity("Reroute");
        auto* reroute = entity->CreateComponent<Nodes::Core::Reroute>();
        entity->Init();
        ASSERT_TRUE(graph->AddNode(reroute->GetEntityId()));
        reroutes.push_back(reroute);
    }

    const Endpoint firstInput(reroutes[0]->GetEntityId(), reroutes[0]->GetSlotId("In"));
    const Endpoint firstOutput(reroutes[0]->GetEntityId(), reroutes[0]->GetSlotId("Out"));
    const Endpoint secondInput(reroutes[1]->GetEntityId(), reroutes[1]->GetSlotId("In"));
    const Endpoint secondOutput(reroutes[1]->GetEntityId(), reroutes[1]->GetSlotId("Out"));
    ASSERT_TRUE(graph->ConnectByEndpoint(firstOutput, secondInput));
    for (const Slot* input : inputs)
    {
        ASSERT_TRUE(graph->ConnectByEndpoint(secondOutput, input->GetEndpoint()));
    }
    ASSERT_TRUE(graph->ConnectByEndpoint(output->GetEndpoint(), {reroutes[2]->GetEntityId(), reroutes[2]->GetSlotId("In")}));

    graph->CreateGraphCanvasScene();
    const auto scene = graph->GetGraphCanvasGraphId();
    ASSERT_TRUE(scene.IsValid());
    QApplication::processEvents();

    PropertyDisplayState firstState(MapEndpoint(inputs[0]->GetEndpoint()).GetSlotId());
    PropertyDisplayState secondState(MapEndpoint(inputs[1]->GetEndpoint()).GetSlotId());
    ASSERT_NE(firstState.m_display, nullptr);
    ASSERT_NE(secondState.m_display, nullptr);

    auto expectFallback = [&](bool visible)
    {
        for (Slot* input : inputs)
        {
            const auto canvasEndpoint = MapEndpoint(input->GetEndpoint());
            EXPECT_EQ(graph->IsSlotConnected(canvasEndpoint), !visible);
            EXPECT_EQ(input->GetNode()->GetConnectedNodesForTranslation(*input).empty(), visible);
            ModifiableDatumView datum;
            ASSERT_TRUE(input->FindModifiableDatumView(datum));
            EXPECT_EQ(*datum.GetAs<Data::NumberType>(), 42.0);
            AZ::Crc32 expectedVisibility = AZ::Edit::PropertyVisibility::Hide;
            if (visible)
            {
                expectedVisibility = AZ::Edit::PropertyVisibility::ShowChildrenOnly;
            }
            EXPECT_EQ(datum.GetVisibility(), expectedVisibility);
        }
        EXPECT_EQ(firstState.m_disabled, !visible);
        EXPECT_EQ(secondState.m_disabled, !visible);
    };

    auto connect = [&](const Endpoint& from, const Endpoint& to)
    {
        GraphCanvas::ConnectionId id;
        GraphCanvas::SceneRequestBus::EventResult(
            id, scene, &GraphCanvas::SceneRequests::CreateConnectionBetween, MapEndpoint(from), MapEndpoint(to));
        return id;
    };

    for (Slot* input : inputs)
    {
        input->UpdateDatumVisibility();
        GraphCanvas::DataSlotRequestBus::Event(
            MapEndpoint(input->GetEndpoint()).GetSlotId(), &GraphCanvas::DataSlotRequests::UpdatePropertyDisplayState);
    }
    expectFallback(true);
    GraphCanvas::ConnectionId upstream = connect(output->GetEndpoint(), firstInput);
    ASSERT_TRUE(upstream.IsValid());
    expectFallback(false);

    GraphCanvas::SceneRequestBus::Event(scene, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{upstream});
    expectFallback(true);

    upstream = connect(output->GetEndpoint(), firstInput);
    ASSERT_TRUE(upstream.IsValid());
    expectFallback(false);

    const auto canvasOutput = MapEndpoint({reroutes[2]->GetEntityId(), reroutes[2]->GetSlotId("Out")});
    bool canConvert = true;
    GraphCanvas::DataSlotRequestBus::EventResult(canConvert, canvasOutput.GetSlotId(), &GraphCanvas::DataSlotRequests::CanConvertToReference, false);
    EXPECT_FALSE(canConvert);
    EXPECT_FALSE(graph->CanPromoteToVariable(canvasOutput));
    EXPECT_FALSE(graph->PromoteToVariableAction(canvasOutput, false));
}

TEST_F(RerouteSplicingTest, PartialInsertionRestoresOriginalConnection)
{
    const auto source = AddNode(false, true);
    const auto target = AddNode(true, false);
    const auto reroute = AddNode(true, true);
    const auto original = Connect(source.m_output, target.m_input);
    ASSERT_TRUE(original.IsValid());

    m_model.m_rejectedSlot = reroute.m_input.GetSlotId();
    GraphCanvas::ConnectionSpliceConfig config;
    EXPECT_FALSE(GraphCanvas::GraphUtils::SpliceNodeOntoConnection(reroute.m_node, original, config));
    EXPECT_EQ(m_model.m_connections.size(), 1);
    EXPECT_TRUE(HasConnection(source.m_output, target.m_input));
    EXPECT_FALSE(config.m_splicedSourceEndpoint.IsValid());
    EXPECT_FALSE(config.m_splicedTargetEndpoint.IsValid());
    EXPECT_EQ(m_model.m_undoDepth, 0);
}

TEST_F(RerouteSplicingTest, FanoutInsertionAndDissolvePreserveEndpoints)
{
    const auto source = AddNode(false, true);
    const auto first = AddNode(true, false);
    const auto second = AddNode(true, false);
    const auto reroute = AddNode(true, true);
    const auto firstWire = Connect(source.m_output, first.m_input);
    const auto secondWire = Connect(source.m_output, second.m_input);
    ASSERT_TRUE(firstWire.IsValid());
    ASSERT_TRUE(secondWire.IsValid());

    GraphCanvas::ConnectionSpliceConfig config;
    ASSERT_TRUE(GraphCanvas::GraphUtils::SpliceNodeOntoConnection(reroute.m_node, firstWire, config));
    GraphCanvas::SceneRequestBus::Event(m_scene->GetId(), &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{secondWire});
    ASSERT_TRUE(Connect(config.m_splicedSourceEndpoint, second.m_input).IsValid());
    ASSERT_EQ(m_model.m_connections.size(), 3);

    GraphCanvas::GraphUtils::DetachNodeAndStitchConnections(GraphCanvas::NodeDetachConfig(reroute.m_node));
    EXPECT_EQ(m_model.m_connections.size(), 2);
    EXPECT_TRUE(HasConnection(source.m_output, first.m_input));
    EXPECT_TRUE(HasConnection(source.m_output, second.m_input));
    EXPECT_EQ(m_model.m_undoDepth, 0);
}
