/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QPainter>
#include <QStyleOption>
AZ_POP_DISABLE_WARNING

#include <Components/Nodes/General/GeneralNodeFrameComponent.h>

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>


namespace GraphCanvas
{
    //////////////////////////////
    // GeneralNodeFrameComponent
    //////////////////////////////

    void GeneralNodeFrameComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GeneralNodeFrameComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    GeneralNodeFrameComponent::GeneralNodeFrameComponent()
        : m_frameWidget(nullptr)
        , m_shouldDeleteFrame(true)
    {

    }

    GeneralNodeFrameComponent::~GeneralNodeFrameComponent()
    {
        if (m_shouldDeleteFrame)
        {
            delete m_frameWidget;
        }
    }

    void GeneralNodeFrameComponent::Init()
    {
        m_frameWidget = aznew GeneralNodeFrameGraphicsWidget(GetEntityId());
    }

    void GeneralNodeFrameComponent::Activate()
    {
        NodeNotificationBus::Handler::BusConnect(GetEntityId());

        m_frameWidget->Activate();
    }

    void GeneralNodeFrameComponent::Deactivate()
    {
        m_frameWidget->Deactivate();

        NodeNotificationBus::Handler::BusDisconnect();
    }

    void GeneralNodeFrameComponent::OnNodeActivated()
    {
        QGraphicsLayout* layout = nullptr;
        NodeLayoutRequestBus::EventResult(layout, GetEntityId(), &NodeLayoutRequests::GetLayout);
        m_frameWidget->setLayout(layout);
    }

    void GeneralNodeFrameComponent::OnNodeWrapped(const AZ::EntityId& /*wrappingNode*/)
    {
        // When wrapped, our NodeFrame widget is part of another objects layout, and will
        // be deleted when that object gets deleted.
        m_shouldDeleteFrame = false;
    }

    void GeneralNodeFrameComponent::OnNodeUnwrapped(const AZ::EntityId& /*wrappingNode*/)
    {
        m_shouldDeleteFrame = true;
    }

    ///////////////////////////////////
    // GeneralNodeFrameGraphicsWidget
    ///////////////////////////////////

    GeneralNodeFrameGraphicsWidget::GeneralNodeFrameGraphicsWidget(const AZ::EntityId& entityKey)
        : NodeFrameGraphicsWidget(entityKey)
    {
    }

    QPainterPath GeneralNodeFrameGraphicsWidget::GetOutline() const
    {
        QPainterPath path;

        QPen border = m_style.GetBorder();
        qreal cornerRadius = GetCornerRadius();

        qreal halfBorder = border.widthF() / 2.;
        QRectF adjusted = sceneBoundingRect().marginsRemoved(QMarginsF(halfBorder, halfBorder, halfBorder, halfBorder));

        if (cornerRadius >= 1.0)
        {
            path.addRoundedRect(adjusted, cornerRadius, cornerRadius);
        }
        else
        {
            path.addRect(adjusted);
        }

        return path;
    }

    void GeneralNodeFrameGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QPen border = m_style.GetBorder();
        QBrush background = m_style.GetBrush(Styling::Attribute::BackgroundColor);

        if (border.style() != Qt::NoPen || background.color().alpha() > 0)
        {
            qreal cornerRadius = GetCornerRadius();

            border.setJoinStyle(Qt::PenJoinStyle::MiterJoin); // sharp corners
            painter->setPen(border);
            painter->setBrush(background);

            qreal halfBorder = border.widthF() / 2.;
            QRectF adjusted = boundingRect().marginsRemoved(QMarginsF(halfBorder, halfBorder, halfBorder, halfBorder));

            if (cornerRadius >= 1.0)
            {
                painter->drawRoundedRect(adjusted, cornerRadius, cornerRadius);
            }
            else
            {
                painter->drawRect(adjusted);
            }
        }

        QGraphicsWidget::paint(painter, option, widget);
    }
}
