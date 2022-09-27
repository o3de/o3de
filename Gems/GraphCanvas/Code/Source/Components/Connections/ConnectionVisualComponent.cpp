/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qevent.h>
#include <QtMath>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QStyle>
#include <QStyleOption>
#include <qglobal.h>

#include <Components/Connections/ConnectionVisualComponent.h>

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/definitions.h>

namespace GraphCanvas
{
    //////////////////////////////
    // ConnectionVisualComponent
    //////////////////////////////
    void ConnectionVisualComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ConnectionVisualComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    ConnectionVisualComponent::ConnectionVisualComponent()
    {
    }

    void ConnectionVisualComponent::Init()
    {
        CreateConnectionVisual();
    }

    void ConnectionVisualComponent::Activate()
    {
        if (m_connectionGraphicsItem)
        {
            m_connectionGraphicsItem->Activate();
        }

        VisualRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberUIRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ConnectionVisualComponent::Deactivate()
    {
        VisualRequestBus::Handler::BusDisconnect();
        SceneMemberUIRequestBus::Handler::BusDisconnect();

        if (m_connectionGraphicsItem)
        {
            m_connectionGraphicsItem->Deactivate();
        }
    }

    QGraphicsItem* ConnectionVisualComponent::AsGraphicsItem()
    {
        return m_connectionGraphicsItem.get();
    }

    bool ConnectionVisualComponent::Contains(const AZ::Vector2&) const
    {
        return false;
    }

    void ConnectionVisualComponent::SetVisible(bool visible)
    {
        m_connectionGraphicsItem->setVisible(visible);
    }

    bool ConnectionVisualComponent::IsVisible() const
    {
        return m_connectionGraphicsItem->isVisible();
    }

    QGraphicsItem* ConnectionVisualComponent::GetRootGraphicsItem()
    {
        return m_connectionGraphicsItem.get();
    }

    QGraphicsLayoutItem* ConnectionVisualComponent::GetRootGraphicsLayoutItem()
    {
        return nullptr;
    }

    void ConnectionVisualComponent::SetSelected(bool selected)
    {
        m_connectionGraphicsItem->setSelected(selected);
    }

    bool ConnectionVisualComponent::IsSelected() const
    {
        return m_connectionGraphicsItem->isSelected();
    }

    QPainterPath ConnectionVisualComponent::GetOutline() const
    {
        return m_connectionGraphicsItem->path();
    }

    void ConnectionVisualComponent::SetZValue(qreal zValue)
    {
        m_connectionGraphicsItem->setZValue(zValue);
    }

    qreal ConnectionVisualComponent::GetZValue() const
    {
        return aznumeric_cast<int>(m_connectionGraphicsItem->zValue());
    }

    void ConnectionVisualComponent::CreateConnectionVisual()
    {
        m_connectionGraphicsItem = AZStd::make_unique<ConnectionGraphicsItem>(GetEntityId());
    }

    ////////////////////////////
    // ConnectionGraphicsItem
    ////////////////////////////

    qreal ConnectionGraphicsItem::VectorLength(QPointF vectorPoint)
    {
        return qSqrt(vectorPoint.x() * vectorPoint.x() + vectorPoint.y() * vectorPoint.y());
    }

    ConnectionGraphicsItem::ConnectionGraphicsItem(const AZ::EntityId& connectionEntityId)
        : RootGraphicsItem(connectionEntityId)
        , m_trackMove(false)
        , m_moveSource(false)
        , m_curveType(Styling::ConnectionCurveType::Straight)
        , m_offset(0.0)
        , m_connectionEntityId(connectionEntityId)
    {
        setFlags(ItemIsSelectable | ItemIsFocusable);
        setData(GraphicsItemName, QStringLiteral("DefaultConnectionVisual/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));
    }

    void ConnectionGraphicsItem::Activate()
    {
        AZStd::string tooltip;
        ConnectionRequestBus::EventResult(tooltip, GetEntityId(), &ConnectionRequests::GetTooltip);
        setToolTip(Tools::qStringFromUtf8(tooltip));

        ConnectionNotificationBus::Handler::BusConnect(GetEntityId());
        StyleNotificationBus::Handler::BusConnect(GetEntityId());

        AZ::EntityId sourceId;
        ConnectionRequestBus::EventResult(sourceId, GetConnectionEntityId(), &ConnectionRequests::GetSourceSlotId);
        OnSourceSlotIdChanged(AZ::EntityId(), sourceId);

        AZ::EntityId targetId;
        ConnectionRequestBus::EventResult(targetId, GetConnectionEntityId(), &ConnectionRequests::GetTargetSlotId);
        OnTargetSlotIdChanged(AZ::EntityId(), targetId);

        ConnectionUIRequestBus::Handler::BusConnect(GetConnectionEntityId());

        SceneMemberNotificationBus::Handler::BusConnect(GetConnectionEntityId());

        OnStyleChanged();
        UpdateConnectionPath();

        OnActivate();
    }

    void ConnectionGraphicsItem::Deactivate()
    {
        SceneMemberNotificationBus::Handler::BusDisconnect();
        ConnectionUIRequestBus::Handler::BusDisconnect();
        ConnectionNotificationBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();

        VisualNotificationBus::MultiHandler::BusDisconnect();
    }

    void ConnectionGraphicsItem::RefreshStyle()
    {
        m_style.SetStyle(GetEntityId());
        UpdatePen();
    }

    const Styling::StyleHelper& ConnectionGraphicsItem::GetStyle() const
    {
        return m_style;
    }

    void ConnectionGraphicsItem::UpdateOffset()
    {
        auto currentTime = AZStd::chrono::steady_clock::now();
        auto currentDuration = currentTime.time_since_epoch();
        AZStd::chrono::milliseconds currentUpdate = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(currentDuration);

        float delta = (currentUpdate - m_lastUpdate).count() * 0.001f;

        m_lastUpdate = currentUpdate;

        // This works for all default dash/dot patterns, for now
        static const double offsetReset = 24.0;

        // TODO: maybe calculate this based on an "animation-speed" attribute?
        //       For now. 1.35 resets per second?
        m_offset += offsetReset * 1.35f * delta;

        if (m_offset >= offsetReset)
        {
            m_offset -= offsetReset;
        }

        QPen currentPen = pen();
        currentPen.setDashOffset(-m_offset);
        setPen(currentPen);
    }

    QRectF ConnectionGraphicsItem::GetBoundingRect() const
    {
        return boundingRect();
    }

    void ConnectionGraphicsItem::OnSourceSlotIdChanged(const AZ::EntityId& oldSlotId, const AZ::EntityId& newSlotId)
    {
        if (oldSlotId.IsValid())
        {
            VisualNotificationBus::MultiHandler::BusDisconnect(oldSlotId);
        }

        if (newSlotId.IsValid())
        {
            VisualNotificationBus::MultiHandler::BusConnect(newSlotId);
        }

        UpdateConnectionPath();
    }

    void ConnectionGraphicsItem::OnTargetSlotIdChanged(const AZ::EntityId& oldSlotId, const AZ::EntityId& newSlotId)
    {
        if (oldSlotId.IsValid())
        {
            VisualNotificationBus::MultiHandler::BusDisconnect(oldSlotId);
        }

        if (newSlotId.IsValid())
        {
            VisualNotificationBus::MultiHandler::BusConnect(newSlotId);
        }

        UpdateConnectionPath();
    }

    void ConnectionGraphicsItem::OnTooltipChanged(const AZStd::string& tooltip)
    {
        setToolTip(Tools::qStringFromUtf8(tooltip));
    }

    void ConnectionGraphicsItem::OnStyleChanged()
    {
        RefreshStyle();

        bool animate = m_style.GetAttribute(Styling::Attribute::LineStyle, Qt::SolidLine) != Qt::SolidLine;

        if (animate)
        {
            if (!AZ::SystemTickBus::Handler::BusIsConnected())
            {
                auto currentTime = AZStd::chrono::steady_clock::now();
                auto currentDuration = currentTime.time_since_epoch();
                m_lastUpdate = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(currentDuration);

                AZ::SystemTickBus::Handler::BusConnect();
            }
        }
        else if (AZ::SystemTickBus::Handler::BusIsConnected())
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
        }

        setOpacity(m_style.GetAttribute(Styling::Attribute::Opacity, 1.0f));

        UpdateConnectionPath();
    }

    void ConnectionGraphicsItem::OnSystemTick()
    {
        UpdateOffset();
    }

    void ConnectionGraphicsItem::OnItemChange(const AZ::EntityId& /*entityId*/, QGraphicsItem::GraphicsItemChange change, const QVariant& /*value*/)
    {
        switch (change)
        {
        case QGraphicsItem::ItemScenePositionHasChanged:
        {
            UpdateConnectionPath();
            break;
        }
        }
    }

    void ConnectionGraphicsItem::UpdateConnectionPath()
    {
        AZ::EntityId sourceId;
        ConnectionRequestBus::EventResult(sourceId, GetConnectionEntityId(), &ConnectionRequests::GetSourceSlotId);

        AZ::EntityId targetId;
        ConnectionRequestBus::EventResult(targetId, GetConnectionEntityId(), &ConnectionRequests::GetTargetSlotId);

        QPointF start;
        ConnectionRequestBus::EventResult(start, GetEntityId(), &ConnectionRequests::GetSourcePosition);

        QPointF startJutDirection;
        SlotUIRequestBus::EventResult(startJutDirection, sourceId, &SlotUIRequests::GetJutDirection);

        QPointF end;
        ConnectionRequestBus::EventResult(end, GetEntityId(), &ConnectionRequests::GetTargetPosition);

        QPointF endJutDirection;
        SlotUIRequestBus::EventResult(endJutDirection, targetId, &SlotUIRequests::GetJutDirection);

        if (!sourceId.IsValid())
        {
            startJutDirection = -endJutDirection;
        }
        else if (!targetId.IsValid())
        {
            endJutDirection = -startJutDirection;
        }

        bool loopback = false;
        float nodeHeight = 0;
        AZ::Vector2 nodePos(0, 0);

        if (end.isNull())
        {
            end = start;
        }
        else
        {
            // Determine if this connection is from and to the same node (self-connection)
            AZ::EntityId sourceNode;
            SlotRequestBus::EventResult(sourceNode, sourceId, &SlotRequestBus::Events::GetNode);

            AZ::EntityId targetNode;
            SlotRequestBus::EventResult(targetNode, targetId, &SlotRequestBus::Events::GetNode);

            loopback = sourceNode == targetNode;

            if (loopback)
            {
                QGraphicsItem* rootVisual = nullptr;
                SceneMemberUIRequestBus::EventResult(rootVisual, sourceNode, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (rootVisual)
                {
                    nodeHeight = aznumeric_cast<float>(rootVisual->boundingRect().height());
                }

                GeometryRequestBus::EventResult(nodePos, sourceNode, &GeometryRequestBus::Events::GetPosition);
            }
        }

        ConnectionType connectionType = ConnectionType::CT_Invalid;
        SlotRequestBus::EventResult(connectionType, sourceId, &SlotRequests::GetConnectionType);

        QPainterPath path = QPainterPath(start);

        if (m_curveType == Styling::ConnectionCurveType::Curved)
        {
            // Scale the control points based on the length of the line to make sure the curve looks pretty
            QPointF offset = (end - start);
            QPointF midVector = (start + end) / 2.0 - start;

            // Mathemagic to make the curvature look nice
            qreal magnitude = 0;

            if (offset.x() < 0)
            {
                magnitude = AZ::GetMax(qSqrt(VectorLength(offset)) * 5, qFabs(offset.x()) * 0.25f);
            }
            else
            {
                magnitude = AZ::GetMax(qSqrt(VectorLength(offset)) * 5, offset.x() * 0.5f);
            }
            magnitude = AZ::GetClamp(magnitude, (qreal) 10.0f, qMax(VectorLength(midVector), (qreal) 10.0f));

            // Makes the line come out horizontally from the start and end points
            QPointF offsetStart = start + startJutDirection * magnitude;
            QPointF offsetEnd = end + endJutDirection * magnitude;

            if (!loopback)
            {
                path.cubicTo(offsetStart, offsetEnd, end);
            }
            else
            {
                // Make the connection wrap around the node,
                // leaving some space between the connection and the node
                qreal heightOffset = (qreal)nodePos.GetY() + nodeHeight + 20.0f;

                path.cubicTo(offsetStart, { offsetStart.x(), heightOffset }, { start.x(), heightOffset });
                path.lineTo({ end.x(), heightOffset });
                path.cubicTo({ offsetEnd.x(), heightOffset }, offsetEnd, end);
            }
        }
        else
        {
            float connectionJut = m_style.GetAttribute(Styling::Attribute::ConnectionJut, 0.0f);

            QPointF startOffset = start + startJutDirection * connectionJut;
            QPointF endOffset = end + endJutDirection * connectionJut;
            path.lineTo(startOffset);
            path.lineTo(endOffset);
            path.lineTo(end);
        }

        setPath(path);
        update();

        OnPathChanged();
        ConnectionVisualNotificationBus::Event(GetEntityId(), &ConnectionVisualNotifications::OnConnectionPathUpdated);
    }

    void ConnectionGraphicsItem::SetAltDeletionEnabled(bool enabled)
    {
        SetAllowQuickDeletion(enabled);
    }

    void ConnectionGraphicsItem::SetGraphicsItemFlags(QGraphicsItem::GraphicsItemFlags flags)
    {
        setFlags(flags);
    }

    void ConnectionGraphicsItem::OnSceneMemberHidden()
    {
        AZ::EntityId sourceId;
        ConnectionRequestBus::EventResult(sourceId, GetConnectionEntityId(), &ConnectionRequests::GetSourceSlotId);
        VisualNotificationBus::MultiHandler::BusDisconnect(sourceId);

        AZ::EntityId targetId;
        ConnectionRequestBus::EventResult(targetId, GetConnectionEntityId(), &ConnectionRequests::GetTargetSlotId);
        VisualNotificationBus::MultiHandler::BusDisconnect(targetId);
    }

    void ConnectionGraphicsItem::OnSceneMemberShown()
    {
        AZ::EntityId sourceId;
        ConnectionRequestBus::EventResult(sourceId, GetConnectionEntityId(), &ConnectionRequests::GetSourceSlotId);
        VisualNotificationBus::MultiHandler::BusConnect(sourceId);

        AZ::EntityId targetId;
        ConnectionRequestBus::EventResult(targetId, GetConnectionEntityId(), &ConnectionRequests::GetTargetSlotId);
        VisualNotificationBus::MultiHandler::BusConnect(targetId);

        UpdateConnectionPath();
    }

    void ConnectionGraphicsItem::OnSceneSet(const GraphId& graphId)
    {
        m_editorId = EditorId();
        SceneRequestBus::EventResult(m_editorId, graphId, &SceneRequests::GetEditorId);

        AssetEditorSettingsNotificationBus::Handler::BusDisconnect();
        AssetEditorSettingsNotificationBus::Handler::BusConnect(m_editorId);

        UpdateCurveStyle();
    }

    void ConnectionGraphicsItem::OnSettingsChanged()
    {
        UpdateCurveStyle();
    }

    const AZ::EntityId& ConnectionGraphicsItem::GetConnectionEntityId() const
    {
        return m_connectionEntityId;
    }

    AZ::EntityId ConnectionGraphicsItem::GetSourceSlotEntityId() const
    {
        AZ::EntityId sourceId;
        ConnectionRequestBus::EventResult(sourceId, GetConnectionEntityId(), &ConnectionRequests::GetSourceSlotId);

        return sourceId;
    }

    AZ::EntityId ConnectionGraphicsItem::GetTargetSlotEntityId() const
    {
        AZ::EntityId targetId;
        ConnectionRequestBus::EventResult(targetId, GetConnectionEntityId(), &ConnectionRequests::GetTargetSlotId);

        return targetId;
    }

    EditorId ConnectionGraphicsItem::GetEditorId() const
    {
        return m_editorId;
    }

    void ConnectionGraphicsItem::UpdateCurveStyle()
    {
        Styling::ConnectionCurveType oldType = m_curveType;

        m_curveType = GetCurveStyle();

        if (m_curveType != oldType)
        {
            UpdateConnectionPath();
        }
    }

    Styling::ConnectionCurveType ConnectionGraphicsItem::GetCurveStyle() const
    {
        Styling::ConnectionCurveType curveStyle = Styling::ConnectionCurveType::Straight;
        AssetEditorSettingsRequestBus::EventResult(curveStyle, GetEditorId(), &AssetEditorSettingsRequests::GetConnectionCurveType);
        return curveStyle;
    }

    void ConnectionGraphicsItem::UpdatePen()
    {
        QPen pen = m_style.GetPen(Styling::Attribute::LineWidth, Styling::Attribute::LineStyle, Styling::Attribute::LineColor, Styling::Attribute::CapStyle);

        setPen(pen);
    }

    void ConnectionGraphicsItem::OnActivate()
    {
    }

    void ConnectionGraphicsItem::OnDeactivate()
    {
    }

    void ConnectionGraphicsItem::OnPathChanged()
    {
    }

    QPainterPath ConnectionGraphicsItem::shape() const
    {
        // Creates a "selectable area" around the connection
        QPainterPathStroker stroker;
        qreal padding = m_style.GetAttribute(Styling::Attribute::LineSelectionPadding, 0);
        stroker.setWidth(padding);
        return stroker.createStroke(path());
    }

    void ConnectionGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (mouseEvent->button() == Qt::MouseButton::LeftButton)
        {
            m_trackMove = false;

            if (GetDisplayState() == RootGraphicsItemDisplayState::Inspection)
            {
                const QPainterPath painterPath = path();

                QPointF clickPoint = mouseEvent->scenePos();
                QPointF startPoint = painterPath.pointAtPercent(0);
                QPointF endPoint = painterPath.pointAtPercent(1);

                float distanceToSource = aznumeric_cast<float>((clickPoint - startPoint).manhattanLength());
                float distanceToTarget = aznumeric_cast<float>((clickPoint - endPoint).manhattanLength());

                float maxDistance = m_style.GetAttribute(Styling::Attribute::ConnectionDragMaximumDistance, 100.0f);
                float dragPercentage = m_style.GetAttribute(Styling::Attribute::ConnectionDragPercent, 0.1f);

                float acceptanceDistance = AZStd::GetMin(maxDistance, aznumeric_cast<float>(painterPath.length() * dragPercentage));

                if (distanceToSource < acceptanceDistance)
                {
                    m_trackMove = true;
                    m_moveSource = true;
                    m_initialPoint = mouseEvent->scenePos();
                }
                else if (distanceToTarget < acceptanceDistance)
                {
                    m_trackMove = true;
                    m_moveSource = false;
                    m_initialPoint = mouseEvent->scenePos();
                }
            }
            else
            {
                RootGraphicsItem<QGraphicsPathItem>::mousePressEvent(mouseEvent);
            }
        }
        else
        {
            RootGraphicsItem<QGraphicsPathItem>::mousePressEvent(mouseEvent);
        }
    }

    void ConnectionGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (m_trackMove)
        {
            float maxDistance = m_style.GetAttribute(Styling::Attribute::ConnectionDragMoveBuffer, 0.0f);
            float distanceFromInitial = aznumeric_cast<float>((m_initialPoint - mouseEvent->scenePos()).manhattanLength());

            if (distanceFromInitial > maxDistance)
            {
                m_trackMove = false;

                if (m_moveSource)
                {
                    ConnectionRequestBus::Event(GetEntityId(), &ConnectionRequests::StartSourceMove);
                }
                else
                {
                    ConnectionRequestBus::Event(GetEntityId(), &ConnectionRequests::StartTargetMove);
                }
            }
        }
        else
        {
            RootGraphicsItem<QGraphicsPathItem>::mouseMoveEvent(mouseEvent);
        }
    }

    void ConnectionGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        // Handle the case where we clicked but didn't drag.
        // So we want to select ourselves.
        if (mouseEvent->button() == Qt::MouseButton::LeftButton && shape().contains(mouseEvent->scenePos()))
        {
            if ((mouseEvent->modifiers() & Qt::KeyboardModifier::ControlModifier) == 0)
            {
                AZ::EntityId sceneId;
                SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);
                SceneRequestBus::Event(sceneId, &SceneRequests::ClearSelection);

                setSelected(true);
            }
            else
            {
                setSelected(!isSelected());
            }

            m_trackMove = false;
        }
        else
        {
            RootGraphicsItem<QGraphicsPathItem>::mouseReleaseEvent(mouseEvent);
        }
    }

    void ConnectionGraphicsItem::focusOutEvent(QFocusEvent* focusEvent)
    {
        if (m_trackMove)
        {
            m_trackMove = false;
        }

        RootGraphicsItem<QGraphicsPathItem>::focusOutEvent(focusEvent);
    }

    void ConnectionGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /*= nullptr*/)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool showDefaultSelector = m_style.GetAttribute(Styling::Attribute::ConnectionDefaultMarquee, false);
        if (!showDefaultSelector)
        {
            // Remove the selected state to get rid of the marquee outline
            QStyleOptionGraphicsItem myoption = (*option);
            myoption.state &= !QStyle::State_Selected;
            QGraphicsPathItem::paint(painter, &myoption, widget);
        }
        else
        {
            QGraphicsPathItem::paint(painter, option, widget);
        }
    }

}
