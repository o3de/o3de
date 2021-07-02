/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef PROFILER_AREACHART_H
#define PROFILER_AREACHART_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <QWidget>
#include <QPainterPath>

#include <Source/Driller/ChartTypes.hxx>
#endif

namespace Charts
{
    class Axis;
}

namespace AreaChart
{
    class AreaChart;
    
    struct LinePoint
    {        
    public:
        LinePoint(int position, unsigned int value)
            : m_position(position)
            , m_value(value)
        {
        }
        
        int m_position;
        unsigned int m_value;
    };
    
    class LineSeries
    {
    private:
        friend class AreaChart;
        
        typedef AZStd::vector< LinePoint > LinePoints;
        
    public:
        LineSeries(AreaChart* owner, size_t seriesId, const QString& name, const QColor& color, size_t seriesSize = 0);
        ~LineSeries();        

        size_t GetSeriesId() const;
        void AddPoint(const LinePoint& linePoint);
        void Reset();

        bool IsHighlighted() const;
        bool IsEnabled() const;

        const QColor& GetColor() const;

        void ResetPainterPath();

        QPainterPath& GetPainterPath();
        const QPainterPath& GetPainterPath() const;
        
    private:
    
        AreaChart* m_owner;
        LinePoints m_linePoints;
    
        size_t m_seriesId;
        QString m_name;
        QColor  m_color;

        QPainterPath m_painterPath;
        bool         m_highlighted;
        bool         m_enabled;
        bool         m_hasData;
    };
    
    class AreaChart
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(int insetTop MEMBER m_insetTop)
        Q_PROPERTY(int insetBottom MEMBER m_insetBottom)
        Q_PROPERTY(int insetLeft MEMBER m_insetLeft)
        Q_PROPERTY(int insetRight MEMBER m_insetRight)
        Q_PROPERTY(QColor widgetBackground MEMBER m_widgetBackground)
        Q_PROPERTY(QColor graphBackground MEMBER m_graphBackground)

        struct HitArea
        {
            HitArea(const QPolygon& polygon, size_t seriesId)
                : m_polygon(polygon)
                , m_seriesId(seriesId)
            {            
            }

            QPolygon    m_polygon;
            size_t      m_seriesId;
        };

        struct GraphMarker
        {
            GraphMarker(Charts::AxisType axis, int position, const QColor& color)
                : m_axis(axis)                
                , m_position(position)
                , m_color(color)
            {            
            }

            Charts::AxisType m_axis;
            int              m_position;
            QColor           m_color;
        };

    public:        
        static const size_t k_invalidSeriesId;

        AreaChart(QWidget* parent = nullptr);
        ~AreaChart();

        bool IsMouseInspectionEnabled() const;
        void EnableMouseInspection(bool enabled);        
        
        void ResetChart();

        void ConfigureVerticalAxis(QString label, unsigned int minimumHeight = -1);
        void ConfigureHorizontalAxis(QString label, int minimum, int maximum);

        size_t CreateSeries(const QString& name, const QColor& color, size_t size = 0);        
        void ResetSeries(AZ::u32);
        
        // Methods of adding points
        void AddPoint(size_t seriesId, int position, unsigned int value);
        void AddPoint(size_t seriesId, const LinePoint& linePoint);        

        // Methods of manipulating series
        void SetSeriesHighlight(size_t seriesId, bool highlighted);
        void SetSeriesEnabled(size_t seriesId, bool enabled);

        // Methods of adding markers
        void AddMarker(Charts::AxisType axis, int position, const QColor& color);

    public slots:

    signals:
        void InspectedSeries(size_t seriesId);
        void SelectedSeries(size_t seriesId, int position);
        
    protected:

        // Mouse Inspection
        void mouseMoveEvent(QMouseEvent* mouseEvent) override;
        void leaveEvent(QEvent* mouseEvent) override;

        // Mouse clicks
        void mousePressEvent(QMouseEvent* mouseEvent) override;
        void mouseReleaseEvent(QMouseEvent* mouseEvent) override;        

        void resizeEvent(QResizeEvent* resizeEvent) override;
        void paintEvent(QPaintEvent* paintEvent) override;
        
    private:

        void SetMinimumValueRange(unsigned int value);
        Charts::Axis* GetAxis(Charts::AxisType axisType);

        bool IsValidSeriesId(size_t seriesId) const;

        QPoint ConvertToGraphPoint(int index, unsigned int value);
        
        AZStd::vector< GraphMarker > m_markers;
        AZStd::vector< LineSeries > m_lineSeries;

        AZStd::unordered_map<int, unsigned int> m_maxSizing;

        size_t                                  m_inspectionSeries;
        
        size_t                                  m_mouseOverArea;
        AZStd::vector< AZStd::vector<HitArea> > m_hitAreas;

        bool        m_clicked;
        QPoint      m_mouseDownPoint;        
        
        QRect       m_graphRect;
        bool        m_sizingDirty;
        bool        m_regenGraph;

        unsigned int m_axisMin;
        unsigned int m_axisMax;

        Charts::Axis* m_horizontalAxis;
        Charts::Axis* m_verticalAxis;
        
        // Styling
        int    m_insetTop;
        int    m_insetBottom;
        int    m_insetLeft;
        int    m_insetRight;
        QColor m_widgetBackground;
        QColor m_graphBackground;
    };
    
}

#endif
