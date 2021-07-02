/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StandaloneTools_precompiled.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>

#include <Source/Driller/Axis.hxx>

#include <Source/Driller/AreaChart.hxx>
#include <Source/Driller/moc_AreaChart.cpp>

#include <QPainter>
#include <QPen>
#include <QMouseEvent>

namespace AreaChart
{
    ///////////////
    // LineSeries
    ///////////////
    
    LineSeries::LineSeries(AreaChart* owner, size_t seriesId, const QString& name, const QColor& color, size_t seriesSize)
        : m_owner(owner)
        , m_seriesId(seriesId)
        , m_name(name)
        , m_color(color)
        , m_highlighted(false)
        , m_enabled(true)
        , m_hasData(false)
    {
        if (seriesSize > 0)
        {
            m_linePoints.reserve(seriesSize);            
        }        
    }
    
    LineSeries::~LineSeries()
    {        
    } 
    
    size_t LineSeries::GetSeriesId() const
    {
        return m_seriesId;
    }
    
    void LineSeries::AddPoint(const LinePoint& linePoint)
    {
        // Handle simple case first
        if (m_linePoints.empty() || m_linePoints.back().m_position < linePoint.m_position)
        {
            m_hasData |= linePoint.m_value > 0;
            m_linePoints.push_back(linePoint);            
        }
        else
        {
            // TODO: Handle the case of out of order insertion
            AZ_Error("LineSeries",false,"Trying to add series point out of order. Unsupported behavior");
        }        
    }
    
    void LineSeries::Reset()
    {
        m_linePoints.clear();
    }

    bool LineSeries::IsHighlighted() const
    {
        return m_highlighted;
    }

    bool LineSeries::IsEnabled() const
    {
        return m_enabled && m_hasData;
    }

    const QColor& LineSeries::GetColor() const
    {
        return m_color;
    }

    void LineSeries::ResetPainterPath()
    {
        // Kind of silly that QPainterPath doesn't have a clear.
        m_painterPath = QPainterPath();
    }

    QPainterPath& LineSeries::GetPainterPath()
    {
        return m_painterPath;
    }

    const QPainterPath& LineSeries::GetPainterPath() const
    {
        return m_painterPath;
    }
    
    //////////////
    // AreaChart
    //////////////    

    const size_t AreaChart::k_invalidSeriesId = static_cast<size_t>(-1);
    
    AreaChart::AreaChart(QWidget* parent)
        : QWidget(parent)
        , m_inspectionSeries(k_invalidSeriesId)
        , m_sizingDirty(true)
        , m_regenGraph(true)        
        , m_axisMin(0)
        , m_horizontalAxis(nullptr)
        , m_verticalAxis(nullptr)
        , m_insetTop(16)
        , m_insetBottom(24)
        , m_insetLeft(56)
        , m_insetRight(16)
        , m_widgetBackground(32,32,32,255)
        , m_graphBackground(Qt::black)
    {
        setStyleSheet(QString("QToolTip {   border: 1px solid white; padding: 1px; background: black; color: white; }"));

        m_axisMax = m_axisMin;        
    }
    
    AreaChart::~AreaChart()
    {
        delete m_horizontalAxis;
        delete m_verticalAxis;
    }

    bool AreaChart::IsMouseInspectionEnabled() const
    {
        return hasMouseTracking();
    }

    void AreaChart::EnableMouseInspection(bool enabled)
    {
        setMouseTracking(enabled);
    }

    void AreaChart::SetMinimumValueRange(unsigned int value)
    {
        m_axisMin = value;

        m_axisMax = m_axisMin;

        for (auto& sizingPair : m_maxSizing)
        {
            if (sizingPair.second > m_axisMax)
            {
                m_axisMax = sizingPair.second;
            }
        }

        m_regenGraph = true;
        update();
    }
    
    void AreaChart::ResetChart()
    {
        m_axisMax = m_axisMin;
        m_maxSizing.clear();
        m_lineSeries.clear();
        m_markers.clear();

        m_sizingDirty = true;
        m_regenGraph = true;        
        
        update();
    }

    void AreaChart::ConfigureVerticalAxis(QString label, unsigned int minimumHeight)
    {
        if (minimumHeight >= 0)
        {
            SetMinimumValueRange(minimumHeight);
        }

        if (m_verticalAxis == nullptr)
        {
            m_verticalAxis = aznew Charts::Axis();
        }

        if (m_verticalAxis)
        {
            m_verticalAxis->SetLabel(label);
            m_verticalAxis->SetAxisRange(0.0f, static_cast<float>(m_axisMax));
        }
    }

    void AreaChart::ConfigureHorizontalAxis(QString label, int minimum, int maximum)
    {
        if (m_horizontalAxis == nullptr)
        {
            m_horizontalAxis = aznew Charts::Axis();
        }

        if (m_horizontalAxis)        
        {
            m_horizontalAxis->SetLabel(label);
            m_horizontalAxis->SetAxisRange(static_cast<float>(minimum), static_cast<float>(maximum));
        }
    }
    
    void AreaChart::ResetSeries(AZ::u32 seriesId)
    {
        if (!IsValidSeriesId(seriesId))
        {
            return;
        }

        m_lineSeries[seriesId].Reset();
    }
    
    size_t AreaChart::CreateSeries(const QString& name, const QColor& color, size_t size)
    {
        size_t seriesKey = m_lineSeries.size();
        AZ_Error("AreaChart", seriesKey != k_invalidSeriesId,"Trying to use invalid key for series Id. Too many Area Series created.");

        if (size <= 0)
        {
            size = m_maxSizing.size();
        }

        m_lineSeries.emplace_back(this, seriesKey, name, color, size);
        
        return seriesKey;
    }
    
    void AreaChart::AddPoint(size_t seriesId, int position, unsigned int value)
    {
        AZ_PROFILE_TIMER("Standalone Tools", __FUNCTION__);
        LinePoint linePoint(position,value);
        AddPoint(seriesId,linePoint);
    }
    
    void AreaChart::AddPoint(size_t seriesId, const LinePoint& linePoint)
    {
        AZ_PROFILE_TIMER("Standalone Tools", __FUNCTION__);
        if (!IsValidSeriesId(seriesId))
        {
            AZ_Error("AreaChart", false, "Invalid SeriesId given.");
            return;
        }        
        
        LineSeries& lineSeries = m_lineSeries[seriesId];
        
        lineSeries.AddPoint(linePoint);

        auto sizingIter = m_maxSizing.find(linePoint.m_position);
            
        if (sizingIter != m_maxSizing.end())
        {
            sizingIter->second += linePoint.m_value;

            if (sizingIter->second > m_axisMax)
            {
                m_axisMax = sizingIter->second;
            }
        }
        else
        {
            m_maxSizing[linePoint.m_position] = linePoint.m_value;

            if (linePoint.m_value > m_axisMax)
            {
                m_axisMax = linePoint.m_value;
            }
        }

        m_regenGraph = true;
        update();
    }

    void AreaChart::SetSeriesHighlight(size_t seriesId, bool highlighted)
    {
        if (IsValidSeriesId(seriesId))
        {
            LineSeries& lineSeries = m_lineSeries[seriesId];

            lineSeries.m_highlighted = highlighted;

            update();
        }
    }

    void AreaChart::SetSeriesEnabled(size_t seriesId, bool enabled)
    {
        if (IsValidSeriesId(seriesId))
        {
            LineSeries& lineSeries = m_lineSeries[seriesId];

            lineSeries.m_enabled = enabled;

            // Need to regen our graph data here, since we've removed one from the listing
            m_regenGraph = true;
            update();
        }
    }

    void AreaChart::AddMarker(Charts::AxisType axis, int position, const QColor& color)
    {
        m_markers.emplace_back(axis, position, color);
    }

    void AreaChart::mouseMoveEvent(QMouseEvent* mouseEvent)
    {        
        if (IsMouseInspectionEnabled())
        {
            QPoint mousePos = mouseEvent->pos();
            size_t hoveredArea = k_invalidSeriesId;

            if (m_graphRect.contains(mousePos) && m_hitAreas.size() > 0)
            {
                int offset = mousePos.x() - m_graphRect.left();

                size_t counter = static_cast<size_t>(static_cast<float>(offset) / (static_cast<float>(m_graphRect.width())/m_hitAreas.size()));

                bool escape = false;

                // Need to handle the areas right at the edge of the polygons
                for (int i = -1; i <= 1; ++i)
                {
                    if ((counter+i) < 0 || (counter + i) >= m_hitAreas.size())
                    {
                        continue;
                    }

                    const AZStd::vector<HitArea>& hitAreas = m_hitAreas[counter + i];

                    for (const HitArea& hitArea : hitAreas)
                    {
                        QPolygon polygon = hitArea.m_polygon;

                        if (hitArea.m_polygon.containsPoint(mousePos,Qt::OddEvenFill))
                        {
                            hoveredArea = hitArea.m_seriesId;                    
                            escape = true;
                            break;
                        }
                    }

                    if (escape)
                    {
                        break;
                    }
                }
            }        

            if (hoveredArea != m_inspectionSeries)
            {
                m_inspectionSeries = hoveredArea;
                update();

                // Signal out which series we are inspecting
                emit InspectedSeries(m_inspectionSeries);
            }
        }
    }

    void AreaChart::leaveEvent(QEvent* event)
    {
        (void)event;

        if (m_inspectionSeries != k_invalidSeriesId)
        {
            m_inspectionSeries = k_invalidSeriesId;
            update();

            // Signal out which series we are inspecting
            emit InspectedSeries(m_inspectionSeries);
        }

        if (m_clicked)
        {
            m_clicked = false;
        }
    }

    void AreaChart::mousePressEvent(QMouseEvent* mouseEvent)
    {
        if (IsMouseInspectionEnabled())
        {
            m_clicked = true;
            m_mouseDownPoint = mouseEvent->pos();
        }
    }

    void AreaChart::mouseReleaseEvent(QMouseEvent* mouseEvent)
    {
        if (IsMouseInspectionEnabled() && m_clicked)
        {
            QPoint upPoint = mouseEvent->pos();

            // Want it to be roughly the same spot.
            if ((m_mouseDownPoint - upPoint).manhattanLength() < 20)
            {
                int closestValue = 0;
                if (m_horizontalAxis && m_graphRect.width() > 0)
                {
                    float ratio = static_cast<float>(upPoint.x() - m_graphRect.left()) / static_cast<float>(m_graphRect.width());
                    closestValue = static_cast<int>(m_horizontalAxis->GetRangeMin()) + static_cast<int>((m_horizontalAxis->GetRange() * ratio) + 0.5f);
                }

                emit SelectedSeries(m_inspectionSeries, closestValue);
            }
        }
    }

    void AreaChart::resizeEvent(QResizeEvent* event)
    {
        (void)event;

        m_sizingDirty = true;
        update();
    }
    
    void AreaChart::paintEvent(QPaintEvent* event)
    {
        AZ_PROFILE_TIMER("Standalone Tools", __FUNCTION__);
        (void)event;
        
        if (m_sizingDirty)
        {            
            m_sizingDirty = false;
            m_regenGraph = true;
            
            QPoint topLeft(rect().left() + m_insetLeft, rect().top() + m_insetTop);
            QPoint bottomRight(rect().right() - m_insetRight, rect().bottom() - m_insetBottom);
            
            m_graphRect = QRect(topLeft,bottomRight);            
        }        

        if (m_regenGraph)
        {
            AZ_PROFILE_TIMER("Standalone Tools", "Generating Graph Data");
            m_regenGraph = false;

            if (m_verticalAxis)
            {
                m_verticalAxis->SetAxisRange(0.0f, static_cast<float>(m_axisMax));
            }

            m_hitAreas.clear();
            m_hitAreas.reserve(m_maxSizing.size());
            m_hitAreas.resize(m_maxSizing.size());

            // Running tally of samples that we need to keep track of to manipulate our way through        
            AZStd::vector<unsigned int> runningTotal(m_maxSizing.size(), 0);

            for (LineSeries& lineSeries : m_lineSeries)
            {
                unsigned int counter = 0;

                // Would have to special case out the single data point sample
                if (lineSeries.IsEnabled() && lineSeries.m_linePoints.size() > 1)
                {
                    unsigned int currentValue = lineSeries.m_linePoints[counter].m_value;

                    unsigned int bottomLeft = runningTotal[counter];
                    unsigned int topLeft = bottomLeft + currentValue;

                    runningTotal[counter] = topLeft;
                    ++counter;

                    lineSeries.ResetPainterPath();
                    QPainterPath& painterPath = lineSeries.GetPainterPath();

                    AZ_Assert(runningTotal.size() == lineSeries.m_linePoints.size(), "Mismatched/missing sample values given to AreaChart");
                    for (; counter < lineSeries.m_linePoints.size(); ++counter)
                    {
                        currentValue = lineSeries.m_linePoints[counter].m_value;

                        unsigned int bottomRight = runningTotal[counter];
                        unsigned int topRight = bottomRight + currentValue;

                        runningTotal[counter] = topRight;                       

                        QPolygon polygon;
                        polygon.append(ConvertToGraphPoint(counter - 1, bottomLeft));
                        polygon.append(ConvertToGraphPoint(counter - 1, topLeft));
                        polygon.append(ConvertToGraphPoint(counter, topRight));
                        polygon.append(ConvertToGraphPoint(counter, bottomRight));

                        painterPath.addPolygon(polygon);

                        m_hitAreas[counter].emplace_back(polygon, lineSeries.GetSeriesId());

                        bottomLeft = bottomRight;
                        topLeft = topRight;
                    }
                }
            }
        }
        
        {
            QPen pen;        
            QBrush brush;        
            QPainter p(this);            
        
            p.fillRect(rect(),m_widgetBackground);
            p.fillRect(m_graphRect, m_graphBackground);        
            
            QRect widgetBounds = rect();

            if (m_horizontalAxis)
            {                
                m_horizontalAxis->PaintAxis(Charts::AxisType::Horizontal, &p, widgetBounds, m_graphRect, nullptr);                
            }

            if (m_verticalAxis)
            {
                m_verticalAxis->PaintAxis(Charts::AxisType::Vertical, &p, widgetBounds, m_graphRect, nullptr);                
            }
            
            p.setClipRect(m_graphRect.left(), m_graphRect.top() - 1, m_graphRect.width() + 2, m_graphRect.height() + 2);            

            brush.setStyle(Qt::SolidPattern);

            pen.setStyle(Qt::SolidLine);
            pen.setWidth(2);

            for (LineSeries& lineSeries : m_lineSeries)
            {
                if (!lineSeries.IsEnabled())
                {
                    continue;
                }

                brush.setColor(lineSeries.GetColor());
                p.fillPath(lineSeries.GetPainterPath(), brush);

                if (lineSeries.IsHighlighted()
                    || lineSeries.GetSeriesId() == m_inspectionSeries)
                {
                    // Then highlight it
                    pen.setColor(Qt::white);
                    p.setPen(pen);

                    p.drawPath(lineSeries.GetPainterPath());
                }                
            }

            brush.setStyle(Qt::SolidPattern);

            pen.setStyle(Qt::SolidLine);
            pen.setColor( m_graphBackground );
            pen.setWidth(2);                        

            p.setPen(pen);

            for (GraphMarker& marker : m_markers)
            {
                brush.setColor(marker.m_color);
                switch (marker.m_axis)
                {
                case Charts::AxisType::Horizontal:
                {
                    static const int k_barWidth = 4;
                    static const int k_halfWidth = k_barWidth / 2;

                    if (!AZ::IsClose(m_horizontalAxis->GetRange(),0.0f,0.01f) )
                    {
                        float minRange = m_horizontalAxis->GetRangeMin();                    

                        float ratio = (marker.m_position - minRange) / m_horizontalAxis->GetRange();
                        ratio = AZStd::GetMin(1.0f, ratio);

                        QPoint startPoint;
                        startPoint.setX(m_graphRect.left() + static_cast<int>(m_graphRect.width() * ratio) - k_halfWidth);
                        startPoint.setY(m_graphRect.top());                        

                        p.fillRect(startPoint.x(), startPoint.y(), k_barWidth, m_graphRect.height(), brush);
                        p.drawRect(startPoint.x(), startPoint.y() + 1, k_barWidth, m_graphRect.height() - 1);
                    }
                    break;
                }
                case Charts::AxisType::Vertical:
                {
                    QPoint startPoint = ConvertToGraphPoint(0, static_cast<unsigned int>(marker.m_position));
                    startPoint.setX(m_graphRect.right());

                    p.fillRect(startPoint.x(), startPoint.y(), m_graphRect.width(), 2, brush);
                    p.drawRect(startPoint.x(), startPoint.y(), m_graphRect.width(), 2);
                    break;
                }
                default:
                    AZ_Error("Standalone Tools", false, "Unknown axis type given to marker.");
                };
            }
        }
    }

    Charts::Axis* AreaChart::GetAxis(Charts::AxisType axisType)
    {
        switch (axisType)
        {
            case Charts::AxisType::Horizontal:
                return m_horizontalAxis;
            case Charts::AxisType::Vertical:
                return m_verticalAxis;
            default:
                AZ_Error("AreaChart", false, "Unknown AxisType.");
                return nullptr;
        }
    }

    bool AreaChart::IsValidSeriesId(size_t seriesId) const
    {
        return seriesId < m_lineSeries.size();
    }

    QPoint AreaChart::ConvertToGraphPoint(int index, unsigned int value)
    {
        QPoint graphPoint(m_graphRect.bottomLeft());

        int maxSizes = static_cast<int>(m_maxSizing.size());

        if (m_horizontalAxis)
        {
            maxSizes = AZStd::GetMax(maxSizes, static_cast<int>(m_horizontalAxis->GetRange()));
        }

        if (maxSizes >= 2)
        {
            // -1, since the index is 0 based.
            graphPoint.setX(m_graphRect.left() + static_cast<int>(m_graphRect.width() * (static_cast<float>(index) / static_cast<float>(maxSizes - 1))));
        }

        if (m_axisMax > 0)
        {
            graphPoint.setY(m_graphRect.bottom() - static_cast<int>(m_graphRect.height() * (static_cast<float>(value) / static_cast<float>(m_axisMax))));
        }

        return graphPoint;
    }
}
