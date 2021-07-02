/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QPainter>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Components/BookmarkAnchor/BookmarkAnchorVisualComponent.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

namespace GraphCanvas
{
    ///////////////////////////////////////
    // BookmarkAnchorVisualGraphicsWidget
    ///////////////////////////////////////

    const int k_penWidth = 2;
    const float k_beaconMaxScale = 2.0f;

    BookmarkAnchorVisualGraphicsWidget::BookmarkAnchorVisualGraphicsWidget(const AZ::EntityId& busId)
        : RootGraphicsItem(busId)
        , m_animationDuration(1.0f)
    {
        setFlags(ItemIsSelectable | ItemIsFocusable | ItemIsMovable | ItemSendsScenePositionChanges);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setData(GraphicsItemName, QStringLiteral("BookmarkVisualGraphicsWidget/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));

        setMinimumSize(15, 15);
        setMaximumSize(15, 15);

        GeometryNotificationBus::Handler::BusConnect(busId);
        StyleNotificationBus::Handler::BusConnect(busId);
        SceneMemberNotificationBus::Handler::BusConnect(busId);
        BookmarkNotificationBus::Handler::BusConnect(busId);
    }

    void BookmarkAnchorVisualGraphicsWidget::SetColor(const QColor& drawColor)
    {
        m_drawColor = drawColor;
        update();
    }

    QPainterPath BookmarkAnchorVisualGraphicsWidget::GetOutline() const
    {
        return m_outline;
    }

    QRectF BookmarkAnchorVisualGraphicsWidget::GetBoundingRect() const
    {
        return boundingRect();
    }

    void BookmarkAnchorVisualGraphicsWidget::OnBookmarkTriggered()
    {
        qreal spacing = m_style.GetAttribute(Styling::Attribute::Spacing, 0.0f);
        spacing += k_penWidth;

        QRectF drawRect = sceneBoundingRect();
        drawRect.adjust(spacing, spacing, -spacing, -spacing);

        QPointF center = drawRect.center();

        AnimatedPulseConfiguration pulseConfiguration;
        pulseConfiguration.m_enableGradient = true;
        pulseConfiguration.m_drawColor = m_drawColor;
        pulseConfiguration.m_durationSec = m_animationDuration;
        pulseConfiguration.m_zValue = zValue() - 1;

        {
            QPointF leftPoint(drawRect.left(), center.y());
            QPointF maxLeftPoint = center;
            maxLeftPoint.setX(leftPoint.x() - drawRect.width() * k_beaconMaxScale);

            pulseConfiguration.m_controlPoints.emplace_back(leftPoint, maxLeftPoint);
        }

        {
            QPointF topPoint(center.x(), drawRect.top());
            QPointF maxTopPoint = drawRect.center();
            maxTopPoint.setY(topPoint.y() - drawRect.height() * k_beaconMaxScale);

            pulseConfiguration.m_controlPoints.emplace_back(topPoint, maxTopPoint);
        }

        {
            QPointF rightPoint(drawRect.right(), center.y());
            QPointF maxRightPoint = drawRect.center();
            maxRightPoint.setX(rightPoint.x() + drawRect.width() * k_beaconMaxScale);

            pulseConfiguration.m_controlPoints.emplace_back(rightPoint, maxRightPoint);
        }

        {
            QPointF bottomPoint(drawRect.left() + drawRect.width() * 0.5f, drawRect.bottom());
            QPointF maxBottomPoint = drawRect.center();
            maxBottomPoint.setY(bottomPoint.y() + drawRect.height() * k_beaconMaxScale);

            pulseConfiguration.m_controlPoints.emplace_back(bottomPoint, maxBottomPoint);
        }

        AZ::EntityId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        SceneRequestBus::Event(graphId, &SceneRequests::CreatePulse, pulseConfiguration);
    }

    void BookmarkAnchorVisualGraphicsWidget::OnBookmarkNameChanged()
    {
        AZStd::string name;
        BookmarkRequestBus::EventResult(name, GetEntityId(), &BookmarkRequests::GetBookmarkName);

        int shortcut = k_unusedShortcut;
        BookmarkRequestBus::EventResult(shortcut, GetEntityId(), &BookmarkRequests::GetShortcut);

        if (shortcut == k_unusedShortcut)
        {
            setToolTip(name.c_str());
        }
        else
        {
            QString tooltip = QString("%1 - Shortcut %2").arg(name.c_str()).arg(shortcut);
            setToolTip(tooltip);
        }
    }

    void BookmarkAnchorVisualGraphicsWidget::OnStyleChanged()
    {
        m_style.SetStyle(GetEntityId());

        update();
    }

    void BookmarkAnchorVisualGraphicsWidget::OnPositionChanged([[maybe_unused]] const AZ::EntityId& entityId, const AZ::Vector2& position)
    {
        setPos(ConversionUtils::AZToQPoint(position));
    }

    void BookmarkAnchorVisualGraphicsWidget::OnSceneSet(const AZ::EntityId& sceneId)
    {
        AZ::EntityId grid;
        SceneRequestBus::EventResult(grid, sceneId, &SceneRequests::GetGrid);

        AZ::Vector2 gridSize;
        GridRequestBus::EventResult(gridSize, grid, &GridRequests::GetMinorPitch);

        setMinimumSize(QSizeF(gridSize.GetX(), gridSize.GetY()));
        setMaximumSize(QSizeF(gridSize.GetX(), gridSize.GetY()));

        SetGridSize(gridSize);
        SetSnapToGridEnabled(true);
        SetResizeToGridEnabled(true);
        SetAnchorPoint(AZ::Vector2(0.5, 0.5));

        OnStyleChanged();

        OnBookmarkNameChanged();
    }

    void BookmarkAnchorVisualGraphicsWidget::paint(QPainter* painter, [[maybe_unused]] const QStyleOptionGraphicsItem* option, [[maybe_unused]] QWidget* widget)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QRectF drawRect = boundingRect();

        qreal spacing = m_style.GetAttribute(Styling::Attribute::Spacing, 0.0f);

        QPen pen;
        pen.setColor(m_drawColor);
        pen.setWidth(k_penWidth);
        pen.setJoinStyle(Qt::PenJoinStyle::MiterJoin);

        spacing += pen.width();

        drawRect.adjust(spacing, spacing, -spacing, -spacing);

        painter->save();

        QPen border = m_style.GetBorder();

        if (border.width() > 0)
        {
            painter->setPen(border);
            painter->drawRect(boundingRect());
        }

        painter->setPen(pen);

        QPointF leftPoint(drawRect.left(), drawRect.top() + drawRect.height() * 0.5f);
        QPointF rightPoint(drawRect.right(), drawRect.top() + drawRect.height() * 0.5f);

        QPointF topPoint(drawRect.left() + drawRect.width() * 0.5f, drawRect.top());
        QPointF bottomPoint(drawRect.left() + drawRect.width() * 0.5f, drawRect.bottom());

        {
            m_outline = QPainterPath();
            m_outline.moveTo(leftPoint);
            m_outline.lineTo(topPoint);
            m_outline.lineTo(rightPoint);
            m_outline.lineTo(bottomPoint);
            m_outline.lineTo(leftPoint);
            m_outline.closeSubpath();

            painter->drawPath(m_outline);
        }

        leftPoint = drawRect.center();
        leftPoint.setX(leftPoint.x() - 4);

        rightPoint = drawRect.center();
        rightPoint.setX(rightPoint.x() + 4);

        topPoint = drawRect.center();
        topPoint.setY(topPoint.y() - 4);

        bottomPoint = drawRect.center();
        bottomPoint.setY(bottomPoint.y() + 4);

        {
            QPainterPath path;
            path.moveTo(leftPoint);
            path.lineTo(topPoint);
            path.lineTo(rightPoint);
            path.lineTo(bottomPoint);
            path.lineTo(leftPoint);
            path.closeSubpath();

            painter->fillPath(path, m_drawColor);
        }

        painter->restore();
    }

    //////////////////////////////////
    // BookmarkAnchorVisualComponent
    //////////////////////////////////

    void BookmarkAnchorVisualComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<BookmarkAnchorVisualComponentSaveData>()
                ->Version(2)
                ;

            serializeContext->Class<BookmarkAnchorVisualComponent, GraphCanvasPropertyComponent>()
                ->Version(2)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<BookmarkAnchorVisualComponent>("BookmarkAnchorVisualComponent", "Component that handles the visualization of BookmarkAnchorVisuals")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    BookmarkAnchorVisualComponent::BookmarkAnchorVisualComponent()
        : m_graphicsWidget(nullptr)
    {
    }

    void BookmarkAnchorVisualComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();

        m_graphicsWidget = AZStd::make_unique<BookmarkAnchorVisualGraphicsWidget>(GetEntityId());
    }

    void BookmarkAnchorVisualComponent::Activate()
    {
        GraphCanvasPropertyComponent::Activate();

        SceneMemberUIRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());

        BookmarkNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void BookmarkAnchorVisualComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        SceneMemberUIRequestBus::Handler::BusDisconnect();
    }

    QGraphicsItem* BookmarkAnchorVisualComponent::GetRootGraphicsItem()
    {
        return m_graphicsWidget.get();
    }

    QGraphicsLayoutItem* BookmarkAnchorVisualComponent::GetRootGraphicsLayoutItem()
    {
        return m_graphicsWidget.get();
    }

    void BookmarkAnchorVisualComponent::SetSelected(bool selected)
    {
        m_graphicsWidget->setSelected(selected);
    }

    bool BookmarkAnchorVisualComponent::IsSelected() const
    {
        return m_graphicsWidget->isSelected();
    }

    QPainterPath BookmarkAnchorVisualComponent::GetOutline() const
    {
        return m_graphicsWidget->GetOutline();
    }

    void BookmarkAnchorVisualComponent::SetZValue(qreal zValue)
    {
        m_graphicsWidget->setZValue(zValue);
    }

    qreal BookmarkAnchorVisualComponent::GetZValue() const
    {
        return aznumeric_cast<int>(m_graphicsWidget->zValue());
    }

    void BookmarkAnchorVisualComponent::OnSceneSet([[maybe_unused]] const AZ::EntityId& graphId)
    {
        AZ::Vector2 position;
        GeometryRequestBus::EventResult(position, GetEntityId(), &GeometryRequests::GetPosition);

        m_graphicsWidget->setPos(QPointF(position.GetX(), position.GetY()));

        OnBookmarkColorChanged();
    }

    void BookmarkAnchorVisualComponent::OnBookmarkColorChanged()
    {
        if (m_graphicsWidget)
        {
            QColor bookmarkColor;
            BookmarkRequestBus::EventResult(bookmarkColor, GetEntityId(), &BookmarkRequests::GetBookmarkColor);

            m_graphicsWidget->SetColor(bookmarkColor);
        }
    }
}
