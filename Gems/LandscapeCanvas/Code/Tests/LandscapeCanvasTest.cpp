/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzTest/AzTest.h>
#include <Editor/Core/ConnectionReconciliation.h>
#include <Editor/Core/GraphContext.h>
#include <Editor/Nodes/GradientModifiers/InvertGradientModifierNode.h>
#include <Editor/Nodes/Gradients/ConstantGradientNode.h>
#include <Editor/Nodes/RerouteNode.h>
#include <Editor/Nodes/Shapes/BoxShapeNode.h>
#include <GraphModel/Integration/GraphCanvasMetadata.h>
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Slot.h>
#include <Source/LandscapeCanvasSystemComponent.h>

class LandscapeCanvasTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup a system allocator

        AZ::ComponentApplication::Descriptor appDesc;

        m_application.Create(appDesc);
        m_application.RegisterComponentDescriptor(LandscapeCanvas::LandscapeCanvasSystemComponent::CreateDescriptor());
    }

    void TearDown() override
    {
        m_application.Destroy();
    }

    AZ::ComponentApplication m_application;
};

TEST_F(LandscapeCanvasTest, LandscapeCanvasSystemComponentCreatesAndDestroysSuccessfully)
{
    LandscapeCanvas::LandscapeCanvasSystemComponent test;
}

TEST_F(LandscapeCanvasTest, LandscapeCanvasSystemComponentActivatesAndDeactivatesSuccessfully)
{
    auto entity = AZStd::make_unique<AZ::Entity>();
    entity->CreateComponent<LandscapeCanvas::LandscapeCanvasSystemComponent>();

    entity->Init();
    EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

    entity->Activate();
    EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());

    entity->Deactivate();
    EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

class LandscapeCanvasRoutingTest : public LandscapeCanvasTest
{
protected:
    void SetUp() override
    {
        LandscapeCanvasTest::SetUp();
        auto* context = m_application.GetSerializeContext();
        GraphModel::DataType::Reflect(context);
        GraphModel::GraphContext::Reflect(context);
        GraphModel::GraphElement::Reflect(context);
        GraphModel::SlotId::Reflect(context);
        GraphModel::Slot::Reflect(context);
        GraphModel::Node::Reflect(context);
        GraphModel::Connection::Reflect(context);
        GraphModel::Graph::Reflect(context);
        GraphModelIntegration::GraphCanvasMetadata::Reflect(context);

        m_context = AZStd::make_shared<LandscapeCanvas::GraphContext>();
        m_graph = AZStd::make_shared<GraphModel::Graph>(m_context);
    }

    void TearDown() override
    {
        m_graph.reset();
        m_context.reset();
        LandscapeCanvasTest::TearDown();
    }

    template<class Node>
    AZStd::shared_ptr<Node> AddNode()
    {
        auto node = AZStd::make_shared<Node>(m_graph);
        m_graph->AddNode(node);

        return node;
    }

    GraphModel::GraphPtr Reload()
    {
        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&buffer);
        EXPECT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_XML, m_graph.get(), m_application.GetSerializeContext()));

        GraphModel::GraphPtr loaded(AZ::Utils::LoadObjectFromBuffer<GraphModel::Graph>(buffer.data(), buffer.size(), m_application.GetSerializeContext()));
        if (loaded)
        {
            loaded->PostLoadSetup(m_context);
        }

        return loaded;
    }

    AZStd::shared_ptr<LandscapeCanvas::GraphContext> m_context;
    GraphModel::GraphPtr m_graph;
};

TEST_F(LandscapeCanvasRoutingTest, DisconnectedChainAndFanoutSurviveReloadAndReconciliation)
{
    using namespace LandscapeCanvas;
    using namespace LandscapeCanvasEditor;

    auto source = AddNode<BoxShapeNode>();
    auto first = AddNode<RerouteNode>();
    auto second = AddNode<RerouteNode>();
    auto target = AddNode<ConstantGradientNode>();
    auto otherTarget = AddNode<ConstantGradientNode>();

    auto upstream = m_graph->AddConnection(source->GetSlot(BaseShapeNode::BOUNDS_SLOT_ID), first->GetSlot(RerouteNode::IN_SLOT_ID));
    ASSERT_TRUE(upstream);
    ASSERT_TRUE(m_graph->AddConnection(first->GetSlot(RerouteNode::OUT_SLOT_ID), second->GetSlot(RerouteNode::IN_SLOT_ID)));
    ASSERT_TRUE(m_graph->AddConnection(second->GetSlot(RerouteNode::OUT_SLOT_ID), target->GetSlot(PREVIEW_BOUNDS_SLOT_ID)));
    ASSERT_TRUE(m_graph->AddConnection(second->GetSlot(RerouteNode::OUT_SLOT_ID), otherTarget->GetSlot(PREVIEW_BOUNDS_SLOT_ID)));

    ConnectionsList expected{{
        {source, source->GetSlot(BaseShapeNode::BOUNDS_SLOT_ID)},
        {target, target->GetSlot(PREVIEW_BOUNDS_SLOT_ID)},
    }};
    EXPECT_TRUE(ReconcileConnections(target, expected).empty());
    EXPECT_TRUE(expected.empty());

    ASSERT_TRUE(m_graph->RemoveConnection(upstream));
    auto loaded = Reload();
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->GetConnectionCount(), 3);

    for (auto id : {target->GetId(), otherTarget->GetId()})
    {
        ConnectionsList noComponentSource;
        EXPECT_TRUE(ReconcileConnections(loaded->GetNode(id), noComponentSource).empty());
    }
}

TEST_F(LandscapeCanvasRoutingTest, ExplicitComponentSourceReplacesUnresolvedRoute)
{
    using namespace LandscapeCanvas;
    using namespace LandscapeCanvasEditor;

    auto source = AddNode<BoxShapeNode>();
    auto reroute = AddNode<RerouteNode>();
    auto target = AddNode<ConstantGradientNode>();
    auto wire = m_graph->AddConnection(reroute->GetSlot(RerouteNode::OUT_SLOT_ID), target->GetSlot(PREVIEW_BOUNDS_SLOT_ID));
    ASSERT_TRUE(wire);

    ConnectionsList expected{{
        {source, source->GetSlot(BaseShapeNode::BOUNDS_SLOT_ID)},
        {target, target->GetSlot(PREVIEW_BOUNDS_SLOT_ID)},
    }};
    const auto obsolete = ReconcileConnections(target, expected);
    ASSERT_EQ(obsolete.size(), 1);
    EXPECT_EQ(obsolete.front(), wire);
    ASSERT_EQ(expected.size(), 1);

    ASSERT_TRUE(m_graph->RemoveConnection(obsolete.front()));
    ASSERT_TRUE(m_graph->AddConnection(expected.front().first.second, expected.front().second.second));
    EXPECT_TRUE(ReconcileConnections(target, expected).empty());
    EXPECT_TRUE(expected.empty());
}

TEST_F(LandscapeCanvasRoutingTest, ClearingComponentSourceStillRemovesDirectConnection)
{
    using namespace LandscapeCanvas;

    auto source = AddNode<BoxShapeNode>();
    auto target = AddNode<ConstantGradientNode>();
    const auto wire = m_graph->AddConnection(source->GetSlot(BaseShapeNode::BOUNDS_SLOT_ID), target->GetSlot(PREVIEW_BOUNDS_SLOT_ID));
    ASSERT_TRUE(wire);

    LandscapeCanvasEditor::ConnectionsList expected;
    const auto obsolete = LandscapeCanvasEditor::ReconcileConnections(target, expected);
    ASSERT_EQ(obsolete.size(), 1);
    EXPECT_EQ(obsolete.front(), wire);
}

TEST_F(LandscapeCanvasRoutingTest, AnotherExpectedInputDoesNotDiscardUnresolvedRoute)
{
    using namespace LandscapeCanvas;
    using namespace LandscapeCanvasEditor;

    auto source = AddNode<ConstantGradientNode>();
    auto reroute = AddNode<RerouteNode>();
    auto target = AddNode<InvertGradientModifierNode>();
    ASSERT_TRUE(m_graph->AddConnection(reroute->GetSlot(RerouteNode::OUT_SLOT_ID), target->GetSlot(PREVIEW_BOUNDS_SLOT_ID)));

    ConnectionsList expected{{
        {source, source->GetSlot(OUTBOUND_GRADIENT_SLOT_ID)},
        {target, target->GetSlot(INBOUND_GRADIENT_SLOT_ID)},
    }};
    EXPECT_TRUE(ReconcileConnections(target, expected).empty());
    EXPECT_EQ(expected.size(), 1);
}
