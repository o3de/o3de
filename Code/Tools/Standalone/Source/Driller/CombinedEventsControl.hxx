/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef COMBINEDEVENTS_CONTROL_H
#define COMBINEDEVENTS_CONTROL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QWidget>
#include <QDockWidget>

#include "DrillerNetworkMessages.h"
#include "DrillerMainWindowMessages.h"
#include "RacetrackChart.hxx"
#include "Annotations/AnnotationsHeaderView_Events.hxx"
#endif

namespace Charts
{
    class Axis;
}

namespace Driller
{
    class Aggregator;
    class CEQDataTrack;
    class CollapsiblePanel;
    class AnnotationsProvider;    

    /*
    Channel Control intermediates between one data Aggregator, the application's main window, and the renderer.
    This maintains state used by the renderer and passes changes both up and down via signal/slot.
    */

    class CombinedEventsControl
        : public QDockWidget
        , public Driller::DrillerEventWindowMessages::Bus::Handler
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(CombinedEventsControl,AZ::SystemAllocator,0);
        CombinedEventsControl( QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~CombinedEventsControl(void);

        void SetIdentity(int identity);

        void ClearAggregatorList();
        void AddAggregatorList(DrillerNetworkMessages::AggregatorList &theList);
        void AddAggregator(Aggregator &theAggregator);

        void SetAnnotationsProvider(AnnotationsProvider* ptrAnnotations);

        AZStd::list<Aggregator *>m_Aggregators;

        int m_identity;
        EventNumberType m_ScrubberIndex;
        void SanitizeScrubberIndex();

        QWidget *m_Contents;

        CollapsiblePanel* m_collapsiblePanel;

        AnnotationHeaderView_Events*    m_annotationHeaderView;
        CEQDataTrack*                   m_EventTrack;

        int m_FirstIndex;
        int m_LastIndex;
        int m_IndexCount;

        void SetActive( int active );
        void SetEndFrame( FrameNumberType frame );
        void SetSliderOffset( FrameNumberType frame );

        Charts::Axis* GetAxis() const;

        virtual void EventFocusChanged(EventNumberType eventIdx);

        protected:
            bool eventFilter(QObject *obj, QEvent *event);

        public slots:
            void MouseClickInformed( int newValue );
            void MouseMoveInformed( int newValue );
            void OnEventScrubberboxChanged( int newValue );
            void SetScrubberFrame( FrameNumberType frame );
            void OnEventTrackRequestEventFocus(Driller::EventNumberType);

        signals:
            void InformOfMouseClick( int newValue, int range, int modifiers );
            void InformOfMouseMove( int newValue, int range, int modifiers );
            void InfoAreaGeometryChanged ( QSize newSize);
            void EventRequestEventFocus(EventNumberType);
    };

    class CEQDataTrack
        : public Racetrack::DataRacetrack
    {
        Q_OBJECT;
    public:

        AZ_CLASS_ALLOCATOR(CEQDataTrack,AZ::SystemAllocator,0);
        CEQDataTrack( QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~CEQDataTrack(void);
    };

}


#endif // COMBINEDEVENTS_CONTROL_H
