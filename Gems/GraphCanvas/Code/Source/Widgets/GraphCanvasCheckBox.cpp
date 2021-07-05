/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QCoreApplication>
#include <QFont>
#include <QGraphicsItem>
#include <qgraphicssceneevent.h>
#include <QPainter>

#include <AzCore/Serialization/EditContext.h>

#include <Widgets/GraphCanvasCheckBox.h>

#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/tools.h>

namespace GraphCanvas
{
    ////////////////////////
    // GraphCanvasCheckBox
    ////////////////////////

    GraphCanvasCheckBox::GraphCanvasCheckBox(QGraphicsItem* parent)
        : QGraphicsWidget(parent)
        , m_checked(false)
        , m_pressed(false)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        setFlag(ItemIsMovable, false);
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::MouseButton::LeftButton);
    }
    
    void GraphCanvasCheckBox::SetStyle(const AZ::EntityId& entityId, const char* styleElement)
    {
        prepareGeometryChange();
        m_styleHelper.SetStyle(entityId, styleElement);
        updateGeometry();     
        update();
    }

    void GraphCanvasCheckBox::SetSceneStyle(const AZ::EntityId& sceneId, const char* style)
    {
        prepareGeometryChange();
        m_styleHelper.SetScene(sceneId);
        m_styleHelper.SetStyle(style);
        updateGeometry();
        update();
    }
    
    void GraphCanvasCheckBox::SetChecked(bool checked)
    {
        if (m_checked != checked)
        {
            m_checked = checked;
            GraphCanvasCheckBoxNotificationBus::Event(this, &GraphCanvasCheckBoxNotifications::OnValueChanged, m_checked);

            update();
        }
    }
    
    bool GraphCanvasCheckBox::IsChecked() const
    {
        return m_checked;
    }

    void GraphCanvasCheckBox::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/ /*= nullptr*/)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        painter->save();

        // Background
        qreal borderRadius = m_styleHelper.GetAttribute(Styling::Attribute::BorderRadius, 0);
        qreal halfBorder = m_styleHelper.GetAttribute(Styling::Attribute::BorderWidth, 0.0f) * 0.5f;

        QRectF drawRectangle = boundingRect();
        QSizeF size = m_styleHelper.GetSize(minimumSize());

        qreal halfWidthDiff = (drawRectangle.width() - size.width()) * 0.5f;
        qreal halfHeightDiff = (drawRectangle.height() - size.height()) * 0.5f;
        
        drawRectangle.setX(halfWidthDiff);
        drawRectangle.setY(halfHeightDiff);

        drawRectangle.setWidth(size.width());
        drawRectangle.setHeight(size.height());

        drawRectangle.adjust(halfBorder, halfBorder, -halfBorder, -halfBorder);

        QPainterPath borderPath;

        borderPath.addRoundedRect(drawRectangle, borderRadius, borderRadius);
        painter->fillPath(borderPath, m_styleHelper.GetBrush(Styling::Attribute::BackgroundColor));

        if (m_styleHelper.HasAttribute(Styling::Attribute::BorderWidth))
        {
            QPen restorePen = painter->pen();
            painter->setPen(m_styleHelper.GetBorder());
            painter->drawPath(borderPath);
            painter->setPen(restorePen);
        }

        if (m_checked)
        {
            qreal spacing = m_styleHelper.GetAttribute(Styling::Attribute::Spacing, 2.0);

            drawRectangle.adjust(spacing, spacing, -spacing, -spacing);

            painter->setBrush(m_styleHelper.GetBrush(Styling::Attribute::Color));
            painter->drawRoundedRect(drawRectangle, borderRadius, borderRadius);

            qreal margin = m_styleHelper.GetAttribute(Styling::Attribute::Margin, 0.0f);
            drawRectangle.adjust(margin, margin, -margin, -margin);

            QPen checkPen;
            checkPen.setColor(m_styleHelper.GetAttribute(Styling::Attribute::LineColor, QColor(0,0,0)));
            checkPen.setWidth(m_styleHelper.GetAttribute(Styling::Attribute::LineWidth, 2));

            painter->setPen(checkPen);

            // Check mark drawing
            QPointF firstPoint = drawRectangle.topLeft();
            firstPoint.setY(firstPoint.y() + drawRectangle.height() * 0.65f);

            QPointF secondPoint = drawRectangle.center();
            secondPoint.setX(secondPoint.x() - drawRectangle.width() * 0.15f);
            secondPoint.setY(drawRectangle.bottom());

            QPointF thirdPoint = drawRectangle.topRight();

            painter->drawLine(firstPoint, secondPoint);
            painter->drawLine(secondPoint, thirdPoint);
        }

        painter->restore();
    }

    QSizeF GraphCanvasCheckBox::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
    {
        QSizeF size = m_styleHelper.GetSize(QSizeF());
        switch (which)
        {
        case Qt::PreferredSize:
            return size;
        case Qt::MinimumSize:
            return size;
        case Qt::MaximumSize:
        {
            QSizeF maximumSize = m_styleHelper.GetMaximumSize();

            if (maximumSize.width() > size.width())
            {
                maximumSize.setWidth(size.width());
            }

            return maximumSize;
        }
        default:
            break;
        }

        return QGraphicsWidget::sizeHint(which, constraint);
    }

    void GraphCanvasCheckBox::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        QGraphicsWidget::mousePressEvent(mouseEvent);

        mouseEvent->accept();

        m_pressed = true;
        m_styleHelper.AddSelector(Styling::States::Pressed);
        update();
    }

    void GraphCanvasCheckBox::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        QGraphicsWidget::mouseMoveEvent(mouseEvent);

        if (m_pressed)
        {
            if (!mapRectToScene(boundingRect()).contains(mouseEvent->scenePos()))
            {
                m_styleHelper.RemoveSelector(Styling::States::Pressed);
                m_pressed = false;
                update();
            }
        }
        else if (!m_pressed)
        {
            if (mapRectToScene(boundingRect()).contains(mouseEvent->scenePos()))
            {
                m_styleHelper.AddSelector(Styling::States::Pressed);
                m_pressed = true;
                update();
            }
        }
    }

    void GraphCanvasCheckBox::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        QGraphicsWidget::mouseReleaseEvent(mouseEvent);

        if (m_pressed)
        {
            m_styleHelper.RemoveSelector(Styling::States::Pressed);
            GraphCanvas::GraphCanvasCheckBoxNotificationBus::Event(this, &GraphCanvasCheckBoxNotifications::OnClicked);
            
            SetChecked(!IsChecked());            
        }

        m_pressed = false;
    }

    void GraphCanvasCheckBox::hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        QGraphicsWidget::hoverEnterEvent(hoverEvent);

        hoverEvent->accept();

        m_styleHelper.AddSelector(Styling::States::Hovered);
        update();
    }

    void GraphCanvasCheckBox::hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        QGraphicsWidget::hoverLeaveEvent(hoverEvent);
        
        m_styleHelper.RemoveSelector(Styling::States::Hovered);
        update();
    }
}
