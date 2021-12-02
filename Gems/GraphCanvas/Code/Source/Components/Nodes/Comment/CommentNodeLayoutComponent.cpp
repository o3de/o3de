/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <functional>

#include <QGraphicsLayoutItem>
#include <QGraphicsGridLayout>

#include <AzCore/RTTI/TypeInfo.h>

#include <Components/Nodes/Comment/CommentNodeLayoutComponent.h>

#include <Components/Nodes/Comment/CommentLayerControllerComponent.h>
#include <Components/Nodes/Comment/CommentNodeFrameComponent.h>
#include <Components/Nodes/Comment/CommentNodeTextComponent.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/NodeComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // CommentNodeLayoutComponent
    ///////////////////////////////

    void CommentNodeLayoutComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CommentNodeLayoutComponent, NodeLayoutComponent>()
                ->Version(1)
                ;
        }
    }

    AZ::Entity* CommentNodeLayoutComponent::CreateCommentNodeEntity()
    {
        // Create this Node's entity.
        NodeConfiguration config;
        config.SetShowInOutliner(false);

        AZ::Entity* entity = NodeComponent::CreateCoreNodeEntity(config);
        entity->SetName("Comment");

        entity->CreateComponent<StylingComponent>(Styling::Elements::Comment, AZ::EntityId());
        entity->CreateComponent<CommentNodeFrameComponent>();
        entity->CreateComponent<CommentNodeLayoutComponent>();
        entity->CreateComponent<CommentNodeTextComponent>();
        entity->CreateComponent<CommentLayerControllerComponent>();

        return entity;
    }

    CommentNodeLayoutComponent::CommentNodeLayoutComponent()
    {
    }

    CommentNodeLayoutComponent::~CommentNodeLayoutComponent()
    {
    }

    void CommentNodeLayoutComponent::Init()
    {
        NodeLayoutComponent::Init();

        AZ::EntityBus::Handler::BusConnect(GetEntityId());

        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        m_comment = new QGraphicsLinearLayout(Qt::Horizontal);
    }

    void CommentNodeLayoutComponent::Activate()
    {
        NodeLayoutComponent::Activate();
        NodeNotificationBus::Handler::BusConnect(GetEntityId());

        StyleNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void CommentNodeLayoutComponent::Deactivate()
    {
        NodeLayoutComponent::Deactivate();

        StyleNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
    }

    void CommentNodeLayoutComponent::OnEntityExists(const AZ::EntityId& /*entityId*/)
    {
        AZ::Entity* entity = GetEntity();

        CommentNodeFrameComponent* commentFrameComponent = entity->FindComponent<CommentNodeFrameComponent>();

        if (!commentFrameComponent)
        {
            GeneralNodeFrameComponent* generalNodeFrameComponent = entity->FindComponent<GeneralNodeFrameComponent>();
            entity->RemoveComponent(generalNodeFrameComponent);

            delete generalNodeFrameComponent;

            entity->CreateComponent<CommentNodeFrameComponent>();
        }

        AZ::EntityBus::Handler::BusDisconnect(GetEntityId());
    }

    void CommentNodeLayoutComponent::OnStyleChanged()
    {
        m_style.SetStyle(GetEntityId());
        UpdateLayoutParameters();
    }

    void CommentNodeLayoutComponent::OnNodeActivated()
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

    void CommentNodeLayoutComponent::UpdateLayoutParameters()
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
