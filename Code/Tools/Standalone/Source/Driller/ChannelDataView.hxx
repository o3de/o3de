/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CHANNEL_DATA_VIEW_H
#define CHANNEL_DATA_VIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>

#include <QtWidgets/QWidget>

#include "Source/Driller/DrillerDataTypes.h"
#endif

namespace Driller
{
    class AnnotationsProvider;
    class ChannelControl;
    class ChannelDataView;
    class ChannelProfilerWidget;
    class Aggregator;

    class AggregatorDataPoint
    {
    public:
        AggregatorDataPoint();
        AggregatorDataPoint(QRect rectangle,ChannelProfilerWidget* profiler);        
        
        void Draw(QPainter& painter, int leftEdge, float barWidth);
        bool ContainsPoint(const QPoint& point);

        bool IntersectsDataPoint(const AggregatorDataPoint& dataPoint);
        void AddAggregatorDataPoint(const AggregatorDataPoint& dataPoint);

        bool SetOverlayEnabled(bool enabled);

    private:
        
        bool   m_isActive;
        bool   m_isShowingOverlay;
        bool   m_shouldOutline;
        QColor m_drawColor;
        QRect  m_visualBlock;

        AZStd::unordered_set< ChannelProfilerWidget* > m_combinedProfilers;
    };

    class BudgetMarker
    {
    public:
        BudgetMarker(float value, QColor& drawColor);
        ~BudgetMarker();

        float GetValue() const;
        const QColor& GetColor() const;

    private:

        float m_value;
        QColor m_drawColor;
    };

    typedef unsigned int BudgetMarkerTicket;

    /*
    Channel Data View handles all rendering of the scrolling data graph.
    To do this it caches pointer access to its owning Channel
    and pulls state information directly from there.

    Mouse events are passed upwards to its owning Channel via signal/slot.
    */

    class ChannelDataView : public QWidget
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(ChannelDataView, AZ::SystemAllocator, 0);
        ChannelDataView( QWidget* parent = nullptr );

        virtual ~ChannelDataView();

        void RegisterToChannel(ChannelControl* channel, AnnotationsProvider* annotations);

        QPainter *m_Painter;
        ChannelControl *m_Channel;
        AnnotationsProvider *m_ptrAnnotations;

        int FrameToPosition(FrameNumberType frameNumber);
        FrameNumberType PositionToFrame( const QPoint &pt );
        FrameNumberType FramesPerPixel();

        float GetBarWidth();

        virtual void paintEvent( QPaintEvent *event );
        virtual void mouseMoveEvent( QMouseEvent *event );
        virtual void mousePressEvent( QMouseEvent *event );
        virtual void mouseReleaseEvent( QMouseEvent *event );        
        virtual void wheelEvent(QWheelEvent *event);
        virtual void leaveEvent( QEvent* event );

        void DirtyGraphData();
        void RefreshGraphData();

        BudgetMarkerTicket AddBudgetMarker(float value, QColor color);
        void RemoveBudgetMarker(BudgetMarkerTicket ticket);

signals:
        void InformOfMouseClick(Qt::MouseButton button, FrameNumberType frame, FrameNumberType range, int modifiers );
        void InformOfMouseMove( FrameNumberType frame, FrameNumberType range, int modifiers );
        void InformOfMouseRelease(Qt::MouseButton button, FrameNumberType frame, FrameNumberType range, int modifiers );
        void InformOfMouseWheel( FrameNumberType frame, int wheelAmount, FrameNumberType range, int modifiers );

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private:

        void RecalculateGraphedPoints();
        void RemoveHighlight();

        typedef AZStd::list< AggregatorDataPoint > DataPointList;
        typedef AZStd::unordered_map<FrameNumberType, DataPointList > FramePointMapping;
        
        typedef AZStd::unordered_map<BudgetMarkerTicket, BudgetMarker > BudgetMarkerMap;

        BudgetMarkerTicket m_budgetMarkerCounter;
        BudgetMarkerMap   m_budgetMarkers;

        FramePointMapping m_graphedPoints;
        AZStd::list< ChannelProfilerWidget* > m_profilerWidgets;

        FrameNumberType m_minFrame;
        FrameNumberType m_maxFrame;
        FrameNumberType m_highlightedFrame;

        FrameNumberType m_lastFrame;
        int  m_xOffset;

        bool m_initializeDrag;
        bool m_dragInitialized;

        bool m_shouldIgnorePoint;
        
        QPoint m_simulatedPoint;
        QPoint m_centerPoint;        
        
        bool m_mouseGrabbed;
        bool m_dirtyGraph;
    };

}


#endif // CHANNEL_DATA_VIEW_H
