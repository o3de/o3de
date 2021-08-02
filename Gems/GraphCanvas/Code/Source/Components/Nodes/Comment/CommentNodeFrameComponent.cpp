/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/Nodes/Comment/CommentNodeFrameComponent.h>

#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>

namespace GraphCanvas
{
    //////////////////////////////
    // CommentNodeFrameComponent
    //////////////////////////////

    void CommentNodeFrameComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CommentNodeFrameComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    CommentNodeFrameComponent::CommentNodeFrameComponent()
        : m_frameWidget(nullptr)
    {
    }

    void CommentNodeFrameComponent::Init()
    {
        m_frameWidget = AZStd::make_unique<CommentNodeFrameGraphicsWidget>(GetEntityId());
    }

    void CommentNodeFrameComponent::Activate()
    {
        NodeNotificationBus::Handler::BusConnect(GetEntityId());

        m_frameWidget->Activate();
    }

    void CommentNodeFrameComponent::Deactivate()
    {
        m_frameWidget->Deactivate();

        NodeNotificationBus::Handler::BusDisconnect();
    }

    void CommentNodeFrameComponent::OnNodeActivated()
    {
        QGraphicsLayout* layout = nullptr;
        NodeLayoutRequestBus::EventResult(layout, GetEntityId(), &NodeLayoutRequests::GetLayout);
        m_frameWidget->setLayout(layout);
    }

    ///////////////////////////////////
    // CommentNodeFrameGraphicsWidget
    ///////////////////////////////////

    CommentNodeFrameGraphicsWidget::CommentNodeFrameGraphicsWidget(const AZ::EntityId& entityKey)
        : GeneralNodeFrameGraphicsWidget(entityKey)
    {
        CommentNotificationBus::Handler::BusConnect(entityKey);
    }

    void CommentNodeFrameGraphicsWidget::OnBackgroundColorChanged(const AZ::Color& color)
    {
        QColor convertedColor = ConversionUtils::AZToQColor(color);
        m_style.AddAttributeOverride(Styling::Attribute::BackgroundColor, convertedColor);
        update();
    }

    void CommentNodeFrameGraphicsWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* /*mouseEvent*/)
    {
        CommentUIRequestBus::Event(GetEntityId(), &CommentUIRequests::SetEditable, true);
    }
}
