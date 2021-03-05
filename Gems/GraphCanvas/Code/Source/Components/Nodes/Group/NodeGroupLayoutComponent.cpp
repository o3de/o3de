/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"

#include <functional>

#include <AzCore/RTTI/TypeInfo.h>
#include <QGraphicsLayoutItem>
#include <QGraphicsGridLayout>

#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>

#include <Components/Nodes/NodeComponent.h>
#include <Components/Nodes/Group/NodeGroupFrameComponent.h>
#include <Components/Nodes/Group/NodeGroupLayerControllerComponent.h>
#include <Components/Nodes/Group/NodeGroupLayoutComponent.h>
#include <Components/Nodes/Comment/CommentNodeTextComponent.h>

namespace GraphCanvas
{
    /////////////////////////////
    // NodeGroupLayoutComponent
    /////////////////////////////
    void NodeGroupLayoutComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NodeGroupLayoutComponent, NodeLayoutComponent>()
                ->Version(1)
                ;
        }
    }

    AZ::Entity* NodeGroupLayoutComponent::CreateNodeGroupEntity()
    {
        // Create this Node's entity.
        NodeConfiguration config;
        config.SetShowInOutliner(false);

        AZ::Entity* entity = NodeComponent::CreateCoreNodeEntity(config);
        entity->SetName("NodeGroup");

        entity->CreateComponent<StylingComponent>(Styling::Elements::BlockComment::BlockComment, AZ::EntityId());
        entity->CreateComponent<NodeGroupFrameComponent>();
        entity->CreateComponent<NodeGroupLayoutComponent>();
        entity->CreateComponent<CommentNodeTextComponent>("Untitled Group");
        entity->CreateComponent<NodeGroupLayerControllerComponent>();

        return entity;
    }

    NodeGroupLayoutComponent::NodeGroupLayoutComponent()
    {
    }

    NodeGroupLayoutComponent::~NodeGroupLayoutComponent()
    {
    }

    void NodeGroupLayoutComponent::OnStyleChanged()
    {
        m_style.SetStyle(GetEntityId());
        UpdateLayoutParameters();
    }

    void NodeGroupLayoutComponent::Init()
    {
        NodeLayoutComponent::Init();
        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        m_comment = new QGraphicsLinearLayout(Qt::Horizontal);
    }

    void NodeGroupLayoutComponent::Activate()
    {
        NodeLayoutComponent::Activate();
        NodeNotificationBus::Handler::BusConnect(GetEntityId());

        StyleNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void NodeGroupLayoutComponent::Deactivate()
    {
        NodeLayoutComponent::Deactivate();
        
        StyleNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
    }

    void NodeGroupLayoutComponent::OnNodeActivated()
    {
        QGraphicsLayoutItem* commentGraphicsItem = nullptr;
        CommentLayoutRequestBus::EventResult(commentGraphicsItem, GetEntityId(), &CommentLayoutRequestBus::Events::GetGraphicsLayoutItem);
        if (commentGraphicsItem)
        {
            m_comment->addItem(commentGraphicsItem);
        }

        GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_comment);
        UpdateLayoutParameters();
    }

    void NodeGroupLayoutComponent::UpdateLayoutParameters()
    {
        qreal border = m_style.GetAttribute(Styling::Attribute::BorderWidth, 0.);
        qreal spacing = m_style.GetAttribute(Styling::Attribute::Spacing, 4.);
        qreal margin = m_style.GetAttribute(Styling::Attribute::Margin, 4.);

        m_layout->setContentsMargins(border, border, border, border);
        for (QGraphicsLinearLayout* internalLayout : { m_comment })
        {
            internalLayout->setContentsMargins(margin, margin, margin, margin);
            internalLayout->setSpacing(spacing);
        }

        m_layout->invalidate();
    }
}