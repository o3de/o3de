/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef RACETRACKCHART_H
#define RACETRACKCHART_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <QtWidgets/QWidget>

#include "Source/Driller/DrillerDataTypes.h"
#endif

namespace Charts
{
    class Axis;
}


namespace Racetrack
{
    typedef enum
    {
        OUTSIDE_LEFT    = -1,
        INSIDE_RANGE    = 0,
        OUTSIDE_RIGHT   = 1,
        INVALID_RANGE   = 2
    } TransformResult;

    struct Channel
    {
        AZ_CLASS_ALLOCATOR(Channel,AZ::SystemAllocator,0);
        Channel() : m_Color(QColor(255,255,0,255)) {}

        void SetName( QString name ) { m_Name = name; }
        void SetColor( QColor &color ) { m_Color = color; }

        QString m_Name;
        AZStd::vector< AZStd::pair<float,float> > m_Data;
        QColor m_Color;
    };


    class DataRacetrack
        : public QWidget
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(DataRacetrack,AZ::SystemAllocator,0);
        DataRacetrack( QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~DataRacetrack(void);

        void SetupAxis(QString label, float minimum, float maximum, bool locked = true );

        int AddChannel( QString name );
        void AddData( int channelID, float h, float v = 0.0f );
        void Clear();
        void ClearData( int channelID );
        void SetChannelColor( int channelID, QColor color );
        void SetZoomLimit( float limit );
        void SetZeroBasedAxisNumbering( bool tf );

        void SetMarkerColor(QColor qc);
        void SetMarkerPosition(float qposn);
        
        Charts::Axis *GetAxis() const;
        public slots:

    protected:
        virtual void wheelEvent ( QWheelEvent * event );
        virtual void mouseMoveEvent ( QMouseEvent * event );
        virtual void mousePressEvent ( QMouseEvent * event );
        virtual void mouseReleaseEvent ( QMouseEvent * event );
        virtual void resizeEvent( QResizeEvent * event );
        virtual void leaveEvent(QEvent *);

    protected:
        int m_InsetL;
        int m_InsetR;
        int m_InsetT;
        int m_InsetB;
        QRect m_Inset;
        float m_ZoomLimit;

        typedef AZStd::vector<Channel> Channels;
        Channels m_Channels;
        QPoint m_DragTracker;

        bool m_IsDragging;
        bool m_IsLeftDragging;

        Charts::Axis *m_Axis;
        // first submission = horizontal

        QColor m_MarkerColor;
        float m_MarkerPosition;
        bool m_ZeroBasedAxisDisplay;
        int m_iChannelHighlight;

        // internal ops
        virtual void paintEvent(QPaintEvent *event);
        void DrawRotatedText(QString text, QPainter *painter, float degrees, int x, int y, float scale = 1.0f);
        void RecurseVert(QPainter *painter, float step, float ratio );
        void RenderHorizCallouts( QPainter *painter );
        void RecalculateInset();
        void Zoom( QPoint pt, int steps );
        void Drag( int deltaX);
        QPoint Transform( float h );
        TransformResult Transform( float h, QPoint &outQPoint );

        public slots:
            void OnAxisInvalidated();

signals:
            void EventRequestEventFocus(Driller::EventNumberType);
    };

}


#endif //RACETRACKCHART_H
