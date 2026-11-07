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
