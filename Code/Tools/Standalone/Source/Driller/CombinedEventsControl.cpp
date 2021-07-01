/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StandaloneTools_precompiled.h"

#include <QtWidgets/QGridLayout>

#include "CombinedEventsControl.hxx"
#include <Source/Driller/moc_CombinedEventsControl.cpp>

#include "Annotations/AnnotationsHeaderView_Events.hxx"
#include "Axis.hxx"
#include "CollapsiblePanel.hxx"
#include "ChannelDataView.hxx"
#include "DrillerAggregator.hxx"
#include "DrillerEvent.h"
#include "DrillerMainWindowMessages.h"

#include <QEvent>

using namespace Racetrack;

namespace Driller
{
    static const int k_raceTrackMinSize = 50;
    static const int k_eventTrackSize = 20;

    CombinedEventsControl::CombinedEventsControl(QWidget* parent, Qt::WindowFlags flags)
        : QDockWidget(parent, flags)
    {
        m_ScrubberIndex = 0;

        m_FirstIndex = 0;
        m_LastIndex = 0;

        QWidget* nullBar = new QWidget();
        setTitleBarWidget(nullBar);

        setFeatures(QDockWidget::NoDockWidgetFeatures);
        setAllowedAreas(Qt::BottomDockWidgetArea);

        m_collapsiblePanel = new CollapsiblePanel(this);
        this->setWidget(m_collapsiblePanel);

        m_Contents = new QWidget(this);
        m_Contents->setGeometry(0, 22, 542, 34);
        m_Contents->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        QVBoxLayout* layout = new QVBoxLayout();

        layout->setMargin(0);
        layout->setSpacing(2);

        m_Contents->setLayout(layout);
        m_collapsiblePanel->SetContent(m_Contents);
        m_collapsiblePanel->SetTitle("Detailed Event View");

        m_EventTrack = aznew CEQDataTrack(this);
        m_EventTrack->SetupAxis("", 0.0f, 1.0f, false);
        m_EventTrack->SetMarkerColor(Qt::darkMagenta);
        m_EventTrack->setMinimumHeight(k_raceTrackMinSize);

        m_annotationHeaderView = aznew AnnotationHeaderView_Events(this);

        m_Contents->layout()->addWidget(m_annotationHeaderView);
        m_Contents->layout()->addWidget(m_EventTrack);

        m_EventTrack->installEventFilter(this);

        connect(m_EventTrack, SIGNAL(EventRequestEventFocus(Driller::EventNumberType)), this, SLOT(OnEventTrackRequestEventFocus(Driller::EventNumberType)));
    }

    void CombinedEventsControl::SetIdentity(int identity)
    {
        m_identity = identity;
        DrillerEventWindowMessages::Handler::BusConnect(m_identity);
    }

    bool CombinedEventsControl::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::Resize)
        {
            if (obj == m_EventTrack)
            {
                QRect eventTrackGeometry = m_EventTrack->geometry();
                QSize actualSize = QSize(eventTrackGeometry.x(), eventTrackGeometry.height());
                emit InfoAreaGeometryChanged(actualSize);
            }
        }

        // hand it to the owner
        return QObject::eventFilter(obj, event);
    }

    Charts::Axis* CombinedEventsControl::GetAxis() const
    {
        return m_EventTrack->GetAxis();
    }


    CombinedEventsControl::~CombinedEventsControl()
    {
        DrillerEventWindowMessages::Handler::BusDisconnect(m_identity);

        if (m_EventTrack)
        {
            delete m_EventTrack;
        }
    }

    void CombinedEventsControl::ClearAggregatorList()
    {
        m_Aggregators.clear();
        m_EventTrack->Clear();
        m_EventTrack->setMinimumHeight(k_raceTrackMinSize);

        m_ScrubberIndex = 0;
        m_FirstIndex = 0;
        m_LastIndex = 0;
    }

    void CombinedEventsControl::AddAggregatorList(DrillerNetworkMessages::AggregatorList& theList)
    {
        // m_EventStrip clear all existing channels and the channel axis
        m_EventTrack->Clear();
        for (DrillerNetworkMessages::AggregatorList::iterator iter = theList.begin(); iter != theList.end(); ++iter)
        {
            m_Aggregators.push_back(*iter);

            int channelID = m_EventTrack->AddChannel((*iter)->GetName());
            m_EventTrack->SetChannelColor(channelID, (*iter)->GetColor());
        }

        m_EventTrack->setMinimumHeight(k_raceTrackMinSize + static_cast<int>(m_Aggregators.size()) * k_eventTrackSize);
    }
    void CombinedEventsControl::AddAggregator(Aggregator& theAggregator)
    {
        m_Aggregators.push_back(&theAggregator);

        int channelID = m_EventTrack->AddChannel(theAggregator.GetName());
        m_EventTrack->SetChannelColor(channelID, theAggregator.GetColor());

        m_EventTrack->setMinimumHeight(k_raceTrackMinSize + static_cast<int>(m_Aggregators.size()) * k_eventTrackSize);
    }

    void CombinedEventsControl::SetAnnotationsProvider(AnnotationsProvider* ptrAnnotations)
    {
        m_annotationHeaderView->AttachToAxis(ptrAnnotations, GetAxis());
    }

    void CombinedEventsControl::SetEndFrame(FrameNumberType /*frame*/)
    {
        m_EventTrack->update();
    }

    void CombinedEventsControl::SetSliderOffset(FrameNumberType /*frame*/)
    {
    }

    void CombinedEventsControl::MouseClickInformed(int newValue)
    {
        emit InformOfMouseClick(newValue, 1, 0);
    }

    void CombinedEventsControl::MouseMoveInformed(int newValue)
    {
        emit InformOfMouseMove(newValue, 1, 0);
    }

    void CombinedEventsControl::OnEventScrubberboxChanged(int newValue)
    {
        emit EventRequestEventFocus(static_cast<EventNumberType>(newValue));
    }

    void CombinedEventsControl::SetScrubberFrame(FrameNumberType frame)
    {
        m_EventTrack->GetAxis()->Clear();

        int temp_m_FirstIndex = 0x7fffffff;
        int temp_m_LastIndex = 0;

        AZ::s64 highestCount = 0;
        int channelIdx = 0;
        for (AZStd::list<Aggregator*>::iterator iter = m_Aggregators.begin(); iter != m_Aggregators.end(); ++iter, ++channelIdx)
        {
            m_EventTrack->ClearData(channelIdx);

            size_t numEvents = (*iter)->NumOfEventsAtFrame(frame);

            if (numEvents)
            {
                highestCount += numEvents;
                EventNumberType eventIndexOffset = (*iter)->GetFirstIndexAtFrame(frame);

                for (EventNumberType eventIndex = 0; eventIndex < static_cast<EventNumberType>(numEvents); ++eventIndex)
                {
                    DrillerEvent* dep = (*iter)->GetEvents()[ eventIndex + eventIndexOffset ];
                    AZ::s64 geid = dep->GetGlobalEventId();

                    temp_m_FirstIndex = (int)geid < temp_m_FirstIndex ? (int)geid : temp_m_FirstIndex;
                    temp_m_LastIndex = (int)geid > temp_m_LastIndex ? (int)geid : temp_m_LastIndex;

                    //AZ_TracePrintf("LUA","First %d Last %d\n",temp_m_FirstIndex, temp_m_LastIndex);

                    m_EventTrack->AddData(channelIdx, (float)geid, (float)channelIdx);
                }
            }
        }

        m_FirstIndex = temp_m_FirstIndex;
        m_LastIndex  = temp_m_LastIndex;

        m_EventTrack->GetAxis()->SetAxisRange((float)m_FirstIndex, (float)m_LastIndex);
        m_EventTrack->GetAxis()->SetViewFull();
        SanitizeScrubberIndex();
        emit EventRequestEventFocus(m_ScrubberIndex);
    }

    void CombinedEventsControl::OnEventTrackRequestEventFocus(Driller::EventNumberType eventIndex)
    {
        emit EventRequestEventFocus(eventIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    // Event Window Messages
    void CombinedEventsControl::EventFocusChanged(EventNumberType eventIdx)
    {
        m_ScrubberIndex = eventIdx;
        SanitizeScrubberIndex();
        m_EventTrack->SetMarkerPosition((float)m_ScrubberIndex);
    }

    void CombinedEventsControl::SanitizeScrubberIndex()
    {
        m_ScrubberIndex = m_ScrubberIndex < m_FirstIndex ? m_FirstIndex : m_ScrubberIndex;
        m_ScrubberIndex = m_ScrubberIndex > m_LastIndex ? m_LastIndex : m_ScrubberIndex;
    }


    //////////////////////////////////////////////////////////////////////////
    CEQDataTrack::CEQDataTrack(QWidget* parent, Qt::WindowFlags flags)
        : DataRacetrack(parent, flags)
    {
        m_InsetT = 4;
        m_InsetB = 12;
        SetZeroBasedAxisNumbering(true);
        setAutoFillBackground(false);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
    }
    CEQDataTrack::~CEQDataTrack()
    {
    }
}
