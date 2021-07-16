/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cmath>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <Components/GridVisualComponent.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Utils/QtDrawingUtils.h>

namespace GraphCanvas
{
    ////////////////////////
    // GridVisualComponent
    ////////////////////////

    void GridVisualComponent::Reflect(AZ::ReflectContext * context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GridVisualComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    GridVisualComponent::GridVisualComponent()
    {
    }

    void GridVisualComponent::Init()
    {
        m_gridVisualUi = AZStd::make_unique <GridGraphicsItem>(*this);
        m_gridVisualUi->Init();
    }

    void GridVisualComponent::Activate()
    {
        GridRequestBus::EventResult(m_majorPitch, GetEntityId(), &GridRequests::GetMajorPitch);
        GridRequestBus::EventResult(m_minorPitch, GetEntityId(), &GridRequests::GetMinorPitch);

        VisualRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberUIRequestBus::Handler::BusConnect(GetEntityId());
        GridNotificationBus::Handler::BusConnect(GetEntityId());

        m_gridVisualUi->Activate();
    }

    void GridVisualComponent::Deactivate()
    {
        VisualRequestBus::Handler::BusDisconnect();
        SceneMemberUIRequestBus::Handler::BusDisconnect();
        GridNotificationBus::Handler::BusDisconnect();

        m_gridVisualUi->Deactivate();
    }

    QGraphicsItem* GridVisualComponent::AsGraphicsItem()
    {
        return m_gridVisualUi.get();
    }

    bool GridVisualComponent::Contains(const AZ::Vector2 &) const
    {
        return false;
    }

    void GridVisualComponent::SetVisible(bool visible)
    {
        if (m_gridVisualUi)
        {
            m_gridVisualUi->setVisible(visible);
        }        
    }

    bool GridVisualComponent::IsVisible() const
    {
        if (m_gridVisualUi)
        {
            return m_gridVisualUi->isVisible();
        }

        return false;
    }

    QGraphicsItem* GridVisualComponent::GetRootGraphicsItem()
    {
        return m_gridVisualUi.get();
    }

    QGraphicsLayoutItem* GridVisualComponent::GetRootGraphicsLayoutItem()
    {
        return nullptr;
    }

    void GridVisualComponent::SetSelected(bool)
    {
    }

    bool GridVisualComponent::IsSelected() const
    {
        return false;
    }

    QPainterPath GridVisualComponent::GetOutline() const
    {
        return QPainterPath();
    }

    void GridVisualComponent::SetZValue(qreal)
    {
    }

    qreal GridVisualComponent::GetZValue() const
    {
        if (m_gridVisualUi)
        {
            return aznumeric_cast<int>(m_gridVisualUi->zValue());
        }

        return -10000;
    }

    void GridVisualComponent::OnMajorPitchChanged(const AZ::Vector2& pitch)
    {
        if (!pitch.IsClose(m_majorPitch))
        {
            m_majorPitch = pitch;

            // Clamp it to 1.0f
            if (m_majorPitch.GetX() < 1.0f)
            {
                m_majorPitch.SetX(1.0f);
            }

            if (m_majorPitch.GetY() < 1.0f)
            {
                m_majorPitch.SetY(1.0f);
            }

            if (m_gridVisualUi)
            {
                m_gridVisualUi->update();
            }
        }
    }

    void GridVisualComponent::OnMinorPitchChanged(const AZ::Vector2& pitch)
    {
        if (!pitch.IsClose(m_minorPitch))
        {
            m_minorPitch = pitch;

            if (m_minorPitch.GetX() < 1.0f)
            {
                m_minorPitch.SetX(1.0f);
            }

            if (m_minorPitch.GetY() < 1.0f)
            {
                m_minorPitch.SetY(1.0f);
            }

            if (m_gridVisualUi)
            {
                m_gridVisualUi->update();
            }
        }
    }

    /////////////////////
    // GridGraphicsItem
    /////////////////////

    const int GridGraphicsItem::k_stencilScaleFactor = 2;

    GridGraphicsItem::GridGraphicsItem(GridVisualComponent& gridVisual)
        : RootGraphicsItem(gridVisual.GetEntityId())
        , m_gridVisual(gridVisual)
    {
        setFlags(QGraphicsItem::ItemUsesExtendedStyleOption);
        setZValue(-10000);
        setAcceptHoverEvents(false);
        setData(GraphicsItemName, QStringLiteral("DefaultGridVisual/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));
        
        m_levelOfDetails.resize(m_levelOfDetails.capacity());
        for (int i = 0; i < m_levelOfDetails.size(); ++i)
        {
            m_levelOfDetails[i] = nullptr;
        }
    }

    void GridGraphicsItem::Init()
    {
    }

    void GridGraphicsItem::Activate()
    {
        StyleNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void GridGraphicsItem::Deactivate()
    {
        StyleNotificationBus::Handler::BusDisconnect();
    }
    
    QRectF GridGraphicsItem::GetBoundingRect() const
    {
        return boundingRect();
    }

    void GridGraphicsItem::OnStyleChanged()
    {
        m_style.SetStyle(GetEntityId());
        CacheStencils();
        update();
    }

    QRectF GridGraphicsItem::boundingRect(void) const
    {
        return{ QPointF{ -100000.0, -100000.0 }, QPointF{ 100000.0, 100000.0 } };
    }

    void GridGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* /*widget*/)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        int lod = 0;

        qreal width = option->exposedRect.width();
        qreal height = option->exposedRect.height();

        int numDraws = aznumeric_cast<int>(AZStd::max(ceil(height /static_cast<qreal>(m_gridVisual.m_majorPitch.GetY())) , ceil(width / static_cast<qreal>(m_gridVisual.m_majorPitch.GetX()))));
                
        while (numDraws > 10 && lod < (m_levelOfDetails.size() - 1))
        {
            lod++;
            numDraws /= k_stencilScaleFactor;
        }

        QPixmap* pixmap = m_levelOfDetails[lod];

        if (pixmap == nullptr)
        {
            return;
        }

        int gridStartX = aznumeric_cast<int>(option->exposedRect.left());
        gridStartX -= gridStartX % static_cast<int>(m_gridVisual.m_majorPitch.GetX());

        // Offset by one major step to give a buffer when dealing with negative values to avoid making this super complicated
        gridStartX = aznumeric_cast<int>(gridStartX - m_gridVisual.m_majorPitch.GetX());



        int gridStartY = aznumeric_cast<int>(option->exposedRect.top());
        gridStartY -= gridStartY % static_cast<int>(m_gridVisual.m_majorPitch.GetY());

        // Offset by one major step to give a buffer when dealing with negative values to avoid making this super complicated
        gridStartY = aznumeric_cast<int>(gridStartY - m_gridVisual.m_majorPitch.GetY());

        int terminalY = aznumeric_cast<int>(option->exposedRect.bottom() + m_gridVisual.m_majorPitch.GetY());

        QRectF patternFillRect(QPointF(gridStartX, gridStartY), QPointF(option->exposedRect.right(), terminalY));

        PatternFillConfiguration patternFillConfiguration;
        patternFillConfiguration.m_minimumTileRepetitions = 1;
        patternFillConfiguration.m_evenRowOffsetPercent = 0.0f;
        patternFillConfiguration.m_oddRowOffsetPercent = 0.0f;

        QtDrawingUtils::PatternFillArea((*painter), patternFillRect, (*pixmap), patternFillConfiguration);
    }

    void GridGraphicsItem::CacheStencils()
    {
        int stencilSize = 1;

        int majorX = aznumeric_cast<int>(m_gridVisual.m_majorPitch.GetX());
        int majorY = aznumeric_cast<int>(m_gridVisual.m_majorPitch.GetY());

        int minorX = aznumeric_cast<int>(m_gridVisual.m_minorPitch.GetX());
        int minorY = aznumeric_cast<int>(m_gridVisual.m_minorPitch.GetY());

        int totalSizeX = majorX * stencilSize;
        int totalSizeY = majorY * stencilSize;

        delete m_levelOfDetails[0];
        QPixmap* stencil = new QPixmap(totalSizeX, totalSizeY);
        m_levelOfDetails[0] = stencil;

        QPainter painter(stencil);
        QColor backgroundColor = m_style.GetColor(Styling::Attribute::BackgroundColor);

        painter.fillRect(0, 0, majorX, majorY, backgroundColor);

        QPen majorPen = m_style.GetPen(Styling::Attribute::GridMajorWidth, Styling::Attribute::GridMajorStyle, Styling::Attribute::GridMajorColor, Styling::Attribute::CapStyle, true);
        QPen minorPen = m_style.GetPen(Styling::Attribute::GridMinorWidth, Styling::Attribute::GridMinorStyle, Styling::Attribute::GridMinorColor, Styling::Attribute::CapStyle, true);

        painter.setPen(majorPen);

        for (int i = 0; i <= totalSizeX; i += majorX)
        {
            painter.drawLine(QPoint(i, 0), QPoint(i, majorY));
        }

        for (int i = 0; i <= totalSizeY; i += majorY)
        {
            painter.drawLine(QPoint(0, i), QPoint(majorX, i));
        }

        painter.setPen(minorPen);

        for (int i = minorX; i < totalSizeX; i += minorX)
        {
            if (i % majorX == 0)
            {
                continue;
            }

            painter.drawLine(QPoint(i, 0), QPoint(i, majorY));
        }

        for (int i = minorY; i < totalSizeY; i += minorY)
        {
            if (i % majorY == 0)
            {
                continue;
            }

            painter.drawLine(QPoint(0, i), QPoint(majorX, i));
        }

        PatternFillConfiguration patternFillConfiguration;
        patternFillConfiguration.m_minimumTileRepetitions = 1;
        patternFillConfiguration.m_evenRowOffsetPercent = 0.0f;
        patternFillConfiguration.m_oddRowOffsetPercent = 0.0f;

        for (int i = 1; i < m_levelOfDetails.size(); ++i)
        {
            QPixmap* drawStencil = m_levelOfDetails[i - 1];

            delete m_levelOfDetails[i];
            QPixmap* paintStencil = new QPixmap(drawStencil->width() * k_stencilScaleFactor, drawStencil->height() * k_stencilScaleFactor);
            m_levelOfDetails[i] = paintStencil;

            QPainter localPainter(paintStencil);

            QtDrawingUtils::PatternFillArea(localPainter, paintStencil->rect(), (*drawStencil), patternFillConfiguration);
        }
    }
}
