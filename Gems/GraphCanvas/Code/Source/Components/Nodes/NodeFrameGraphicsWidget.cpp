/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QPainter>
#include <QGraphicsLayout>
#include <QGraphicsSceneEvent>

#include <Components/Nodes/NodeFrameGraphicsWidget.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    ////////////////////////////
    // NodeFrameGraphicsWidget
    ////////////////////////////

    NodeFrameGraphicsWidget::NodeFrameGraphicsWidget(const AZ::EntityId& entityKey)
        : RootGraphicsItem(entityKey)
        , m_displayState(NodeFrameDisplayState::None)
    {
        setFlags(ItemIsSelectable | ItemIsFocusable | ItemIsMovable | ItemSendsScenePositionChanges);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        setData(GraphicsItemName, QStringLiteral("DefaultNodeVisual/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));

        setCacheMode(QGraphicsItem::CacheMode::DeviceCoordinateCache);
    }

    void NodeFrameGraphicsWidget::Activate()
    {
        SceneMemberUIRequestBus::Handler::BusConnect(GetEntityId());
        GeometryNotificationBus::Handler::BusConnect(GetEntityId());
        StyleNotificationBus::Handler::BusConnect(GetEntityId());
        NodeNotificationBus::Handler::BusConnect(GetEntityId());
        NodeUIRequestBus::Handler::BusConnect(GetEntityId());
        VisualRequestBus::Handler::BusConnect(GetEntityId());

        OnActivated();
    }

    void NodeFrameGraphicsWidget::Deactivate()
    {
        StyleNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        NodeUIRequestBus::Handler::BusDisconnect();
        VisualRequestBus::Handler::BusDisconnect();
        GeometryNotificationBus::Handler::BusDisconnect();
        SceneMemberUIRequestBus::Handler::BusDisconnect();
    }

    QRectF NodeFrameGraphicsWidget::GetBoundingRect() const
    {
        return boundingRect();
    }

    QGraphicsItem* NodeFrameGraphicsWidget::GetRootGraphicsItem()
    {
        return this;
    }

    QGraphicsLayoutItem* NodeFrameGraphicsWidget::GetRootGraphicsLayoutItem()
    {
        return this;
    }

    void NodeFrameGraphicsWidget::SetSelected(bool selected)
    {
        setSelected(selected);
    }

    bool NodeFrameGraphicsWidget::IsSelected() const
    {
        return isSelected();
    }

    void NodeFrameGraphicsWidget::SetZValue(qreal zValue)
    {
        setZValue(zValue);
    }

    qreal NodeFrameGraphicsWidget::GetZValue() const
    {
        return aznumeric_cast<int>(zValue());
    }

    void NodeFrameGraphicsWidget::OnPositionChanged(const AZ::EntityId& /*entityId*/, const AZ::Vector2& position)
    {
        setPos(QPointF(position.GetX(), position.GetY()));
    }

    void NodeFrameGraphicsWidget::OnStyleChanged()
    {
        m_style.SetStyle(GetEntityId());

        setOpacity(m_style.GetAttribute(Styling::Attribute::Opacity, 1.0f));

        OnRefreshStyle();
        update();
    }

    QSizeF NodeFrameGraphicsWidget::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
    {
        QSizeF retVal = QGraphicsWidget::sizeHint(which, constraint);

        if (IsResizedToGrid())
        {
            qreal borderWidth = 2 * GetBorderWidth();

            int width = static_cast<int>(retVal.width() - borderWidth);
            int height = static_cast<int>(retVal.height() - borderWidth);

            width = GrowToNextStep(width, GetGridXStep(), StepAxis::Width);
            height = GrowToNextStep(height, GetGridYStep(), StepAxis::Height);

            retVal = QSizeF(width, height);
        }

        return retVal;
    }

    void NodeFrameGraphicsWidget::resizeEvent(QGraphicsSceneResizeEvent* resizeEvent)
    {
        QGraphicsWidget::resizeEvent(resizeEvent);

        // For some reason when you first begin to drag a node widget, it resizes itself from old size to 0. Causing it to resize the group it's in.
        //
        // Kind of a quick patch to avoid that happening since there's nothing obvious in a callstack where the faulty resize is coming from.
        if (!resizeEvent->newSize().isEmpty())
        {
            GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SignalBoundsChanged);
        }
    }
    
    void NodeFrameGraphicsWidget::OnDeleteItem()
    {
        AZ::EntityId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        SceneRequestBus::Event(graphId, &SceneRequests::DeleteNodeAndStitchConnections, GetEntityId());
    }

    QGraphicsItem* NodeFrameGraphicsWidget::AsGraphicsItem()
    {
        return this;
    }

    bool NodeFrameGraphicsWidget::Contains(const AZ::Vector2& position) const
    {
        auto local = mapFromScene(QPointF(position.GetX(), position.GetY()));
        return boundingRect().contains(local);
    }

    void NodeFrameGraphicsWidget::SetVisible(bool visible)
    {
        setVisible(visible);
    }

    bool NodeFrameGraphicsWidget::IsVisible() const
    {
        return isVisible();
    }

    void NodeFrameGraphicsWidget::OnNodeActivated()
    {
    }

    void NodeFrameGraphicsWidget::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        AZStd::string tooltip;
        NodeRequestBus::EventResult(tooltip, GetEntityId(), &NodeRequests::GetTooltip);
        setToolTip(Tools::qStringFromUtf8(tooltip));
        //TODO setEnabled(node->IsEnabled());

        AZ::Vector2 position;
        GeometryRequestBus::EventResult(position, GetEntityId(), &GeometryRequests::GetPosition);
        OnPositionChanged(GetEntityId(), position);

        SceneRequestBus::EventResult(m_editorId, sceneId, &SceneRequests::GetEditorId);
    }

    void NodeFrameGraphicsWidget::OnNodeWrapped(const AZ::EntityId& wrappingNode)
    {
        GeometryNotificationBus::Handler::BusDisconnect();
        setFlag(QGraphicsItem::ItemIsMovable, false);

        SetSnapToGridEnabled(false);
        SetResizeToGridEnabled(false);
        SetSteppedSizingEnabled(false);

        m_wrapperNode = wrappingNode;

        m_isWrapped = true;
    }

    void NodeFrameGraphicsWidget::AdjustSize()
    {
        QRectF originalSize = boundingRect();

        if (!m_isWrapped)
        {
            adjustSize();
        }
        else
        {
            NodeUIRequestBus::Event(m_wrapperNode, &NodeUIRequests::AdjustSize);
        }

        QRectF newSize = boundingRect();

        if (originalSize != newSize)
        {
            GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SignalBoundsChanged);
        }
    }

    void NodeFrameGraphicsWidget::SetSnapToGrid(bool snapToGrid)
    {
        SetSnapToGridEnabled(snapToGrid);
    }

    void NodeFrameGraphicsWidget::SetResizeToGrid(bool resizeToGrid)
    {
        SetResizeToGridEnabled(resizeToGrid);
    }

    void NodeFrameGraphicsWidget::SetGrid(AZ::EntityId gridId)
    {
        AZ::Vector2 gridSize;
        GridRequestBus::EventResult(gridSize, gridId, &GridRequests::GetMinorPitch);

        SetGridSize(gridSize);
    }

    qreal NodeFrameGraphicsWidget::GetCornerRadius() const
    {
        return m_style.GetAttribute(Styling::Attribute::BorderRadius, 5.0);
    }

    qreal NodeFrameGraphicsWidget::GetBorderWidth() const
    {
        return m_style.GetAttribute(Styling::Attribute::BorderWidth, 1.0);
    }

    void NodeFrameGraphicsWidget::SetSteppedSizingEnabled(bool enabled)
    {
        if (enabled != m_enabledSteppedSizing)
        {
            m_enabledSteppedSizing = enabled;
        }
    }

    int NodeFrameGraphicsWidget::GrowToNextStep(int value, int step, StepAxis stepAxis) const
    {
        int finalSize = value;
        int delta = value % step;

        if (delta != 0)
        {
            finalSize = value + (step - delta);
        }

        int gridSteps = finalSize / step;

        if (m_enabledSteppedSizing)
        {
            if (stepAxis == StepAxis::Width)
            {
                StyleManagerRequestBus::EventResult(gridSteps, m_editorId, &StyleManagerRequests::GetSteppedWidth, gridSteps);
            }
            else if (stepAxis == StepAxis::Height)
            {
                StyleManagerRequestBus::EventResult(gridSteps, m_editorId, &StyleManagerRequests::GetSteppedHeight, gridSteps);
            }
        }

        return gridSteps * step;
    }

    int NodeFrameGraphicsWidget::RoundToClosestStep(int value, int step) const
    {
        if (step == 1)
        {
            return value;
        }

        int halfStep = step / 2;

        value += halfStep;
        return ShrinkToPreviousStep(value, step);
    }

    int NodeFrameGraphicsWidget::ShrinkToPreviousStep(int value, int step) const
    {
        int absValue = (value%step);

        if (absValue < 0)
        {
            absValue = step + absValue;
        }

        return value - absValue;
    }

    void NodeFrameGraphicsWidget::OnActivated()
    {
    }

    void NodeFrameGraphicsWidget::OnDeactivated()
    {
    }

    void NodeFrameGraphicsWidget::OnRefreshStyle()
    {
    }
}
