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

    void CommentNodeFrameGraphicsWidget::mouseDoubleClickEvent([[maybe_unused]] QGraphicsSceneMouseEvent* mouseEvent)
    {
        CommentUIRequestBus::Event(GetEntityId(), &CommentUIRequests::SetEditable, true);
    }
}