/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZ_STRIPCHART_H
#define AZ_STRIPCHART_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <QWidget>

#include <Source/Driller/ChartTypes.hxx>
#endif

namespace Charts
{
    class Axis;
}

namespace StripChart
{
    enum TransformResult
    {
        OUTSIDE_LEFT    = -1,
        INSIDE_RANGE    = 0,
        OUTSIDE_RIGHT   = 1,
        INVALID_RANGE   = 2
    }; 

    struct Channel
    {
        AZ_CLASS_ALLOCATOR(Channel,AZ::SystemAllocator,0);
        Channel() : m_Color(QColor(255,255,0,255)), m_Style(STYLE_POINT), m_channelID(0), m_highlighted(false), m_highlightSample(false), m_highlightedSampleID(0) {}

        enum ChannelStyle
        {
            STYLE_POINT = 0,
            STYLE_CONNECTED_LINE,
            STYLE_VERTICAL_LINE,
            STYLE_BAR,
            STYLE_PLUSMINUS
        };

        void SetName(QString name) { m_Name = name; }
        void SetColor(QColor &color) { m_Color = color; }
        void SetStyle(ChannelStyle style) { m_Style = style; }
        void SetHighlight(bool highlight) { m_highlighted = highlight; }
        void SetHighlightedSample(bool highlight, AZ::u64 sampleID) { m_highlightedSampleID = sampleID; m_highlightSample = highlight; }
        void SetID(int id) { m_channelID = id; }

        struct Sample
        {
            float m_domainValue;
            float m_dependentValue;
            AZ::u64 m_sampleID;

            Sample() : m_domainValue(0.0f), m_dependentValue(0.0f), m_sampleID(0) {}
            Sample(AZ::u64 sampleID, float domain, float dependent) : 
                m_sampleID(sampleID), 
                m_domainValue(domain),
                m_dependentValue(dependent) {}
        };

        int m_channelID;
        QString m_Name;
        AZStd::vector< Sample > m_Data;
        QColor m_Color;
        ChannelStyle m_Style;
        bool m_highlighted;
        AZ::u64 m_highlightedSampleID;
        bool m_highlightSample;
    };

    class DataStrip
        : public QWidget
    {
        Q_OBJECT;
    public:
        static int s_invalidChannelId;

        AZ_CLASS_ALLOCATOR(DataStrip,AZ::SystemAllocator,0);
        DataStrip(QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~DataStrip(void);

        void Reset();

        void SetChannelHighlight(int channelID, bool highlight);
        void SetChannelSampleHighlight( int channelID, AZ::u64 sampleID, bool highlight);

        bool AddAxis(QString label, float minimum, float maximum, bool lockedZoom = true, bool lockedRange = false);

        void ZoomExtents(Charts::AxisType axis);
        void ZoomManual(Charts::AxisType axis, float minValue, float maxValue);

        int AddChannel(QString name);

        // Use AddData when just adding a random point to the graph. This will keep all of
        // the axis's in order.
        void AddData(int channelID, AZ::u64 sampleID, float h, float v = 0.0f);

        // Use AddDataBatch when adding lots of data all at once, this will ignore some calls.
        void StartBatchDataAdd();
        void AddBatchedData(int channelID, AZ::u64 sampleID, float h, float v = 0.0f);
        void EndBatchDataAdd();

        void ClearData(int channelID);
        void ClearAxisRange();
        void SetChannelColor(int channelID, QColor color);
        
        void SetChannelStyle(int channelID, Channel::ChannelStyle style);
        void SetViewFull();
        void SetLockRight(bool tf);
        void SetZoomLimit(float limit);

        void SetMarkerColor(QColor qc);
        void SetMarkerPosition(float qposn);

        bool GetAxisRange(Charts::AxisType whichAxis, float& minValue, float& maxValue);
        bool GetWindowRange(Charts::AxisType whichAxis, float &minValue, float &maxValue);
        // override for knowledgeable outsiders who want a different range than from the data provided via AddData(...)
        void SetWindowRange(Charts::AxisType whichAxis, float minValue, float maxValue);
        void AddWindowRange(Charts::AxisType whichAxis, float minValue, float maxValue);

        // you may set it to null to clear it.
        // the chart will automatically uninstall the text formatter if it is destroyed
        // it DOES NOT TAKE OWNERSHIP of the formatter.
        void SetAxisTextFormatter(Charts::QAbstractAxisFormatter* target);
        void AttachDataSourceWidget(QWidget *widget);

        void SetDataDirty();
        bool IsValidChannelId(int channelId) const;

signals:
        void onMouseOverDataPoint(int channelID, AZ::u64 sampleID, float primaryAxisValue, float dependentAxisValue);
        void onMouseOverNothing(float primaryAxisValue, float dependentAxisValue);
        void onMouseLeftDownDomainValue(float domainValue);
        void onMouseLeftDragDomainValue(float domainValue);
        void onMouseLeftUpDomainValue(float domainValue);
        void ProcureData(StripChart::DataStrip*);

    protected:
        virtual void wheelEvent(QWheelEvent * event);
        virtual void mouseMoveEvent(QMouseEvent * event);
        virtual void mousePressEvent(QMouseEvent * event);
        virtual void mouseReleaseEvent(QMouseEvent * event);
        virtual void resizeEvent(QResizeEvent * event);

    protected slots:
        void OnDestroyAxisFormatter(QObject *pDestroyed);

    protected:        

        int m_InsetL;
        int m_InsetR;
        int m_InsetT;
        int m_InsetB;
        QRect m_Inset;
        float m_ZoomLimit;
        bool m_bLeftDown;
        bool m_isDataDirty;

        typedef AZStd::vector<Channel> Channels;
        Channels m_Channels;
        QPoint m_DragTracker;
        bool m_MouseWasDragged;

        Charts::QAbstractAxisFormatter* m_ptrFormatter;

        bool m_IsDragging;
        bool m_InBatchMode;

        // axes dealt with internally as follows:
        Charts::Axis* m_Axis;
        Charts::Axis* m_DependentAxis; // vertical axis
        // first submission = horizontal, for binary state data points
        // second submission = vertical, for state with value data points
        // third submission = depth, for data grids with value

        QColor m_MarkerColor;
        float m_MarkerPosition;

        // internal ops
        virtual void paintEvent(QPaintEvent *event);
        void DrawRotatedText(QString text, QPainter *painter, float degrees, int x, int y, float scale = 1.0f);
        void RenderVertCallouts(QPainter *painter);
        void RenderHorizCallouts(QPainter *painter);
        void RecalculateInset();
        void Drag(Charts::Axis *axis, int deltaY);
        void Drag(Charts::Axis *axis, int deltaX, int deltaY);
        QPoint TransformVert(Charts::Axis *axis, float v);
        QPoint TransformHoriz(Charts::Axis *axis, float h);
        TransformResult Transform(Charts::Axis *axis, float h, float v, QPoint &outQPoint);

        class HitArea
        {
        public:
            AZ_CLASS_ALLOCATOR(HitArea, AZ::SystemAllocator, 0);
            Channel *m_ptrChannel;
            AZ::u64 m_sampleID;
            float m_primaryAxisValue;
            float m_dependentAxisValue;
            QPoint m_hitBoxCenter;
            HitArea() : m_ptrChannel(NULL), m_primaryAxisValue(0.0f), m_dependentAxisValue(0.0f) {}
            HitArea(float x, float y, QPoint hitBoxCenter, Channel* pChannel, AZ::u64 sampleID) :
            m_hitBoxCenter(hitBoxCenter), m_primaryAxisValue(x), m_dependentAxisValue(y), m_ptrChannel(pChannel), m_sampleID(sampleID) {}
        };

        typedef AZStd::vector<HitArea> HitAreaContainer;
        HitAreaContainer m_hitAreas;
    };

}

#endif //AZ_STRIPCHART_H
