/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "DrillerAggregator.hxx"
#include <Source/Driller/moc_DrillerAggregator.cpp>

#include "DrillerEvent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/IO/SystemFile.h>

#include "Source/Driller/CSVExportSettings.h"

#include <QMessageBox>

namespace Driller
{
    class AggregatorSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(AggregatorSavedState, "{9AAB69CE-8061-4CB6-8387-DB60FD8DBB75}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(AggregatorSavedState, AZ::SystemAllocator, 0);
        AggregatorSavedState() {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AggregatorSavedState>()
                    ->Version(1)
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    Aggregator::Aggregator(int identity)
        : QObject()
        , m_identity(identity)
        , m_currentEvent(Driller::kInvalidEventIndex)
        , m_isCaptureEnabled(true)
    {
        DrillerMainWindowMessages::Handler::BusConnect(m_identity);
        DrillerWorkspaceWindowMessages::Handler::BusConnect(m_identity);

        // subclassed aggregators should work with settingsDocument at this point
        // to retrieve state
    }
    Aggregator::~Aggregator()
    {
        // subclassed aggregators should work with settingsDocument at this point
        // to store the current state so it can be retrieved when constructed again

        DrillerWorkspaceWindowMessages::Handler::BusDisconnect(m_identity);
        DrillerMainWindowMessages::Handler::BusDisconnect(m_identity);

        for (EventListType::iterator it = m_events.begin(); it != m_events.end(); ++it)
        {
            delete *it;
        }
    }

    void Aggregator::Reset()
    {
        for (EventListType::iterator it = m_events.begin(); it != m_events.end(); ++it)
        {
            delete *it;
        }

        m_events.clear();
        m_frameToEventIndex.clear();
        m_currentEvent = kInvalidEventIndex;
    }
    bool Aggregator::IsValid()
    {
        return m_events.size() > 0;
    }

    void Aggregator::AddNewFrame()
    {
        m_frameToEventIndex.push_back(m_events.size());
    }

    bool Aggregator::DataAtFrame(FrameNumberType frame)
    {
        return NumOfEventsAtFrame(frame) > 0;
    }

    void Aggregator::ExportToCSVRequest(const char* filename, CSVExportSettings* exportSettings)
    {
        AZ::IO::SystemFile exportFile;

        if (exportFile.Open(filename, AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            if (exportSettings == nullptr || exportSettings->ShouldExportColumnDescriptors())
            {
                ExportColumnDescriptorToCSV(exportFile, exportSettings);
            }

            for (DrillerEvent* drillerEvent : m_events)
            {
                ExportEventToCSV(exportFile, drillerEvent, exportSettings);
            }

            exportFile.Close();
        }
        else
        {
            QMessageBox::critical(nullptr, "Error Opening File", QString("Could not open file %1").arg(filename), QMessageBox::Ok);
        }
    }

    size_t Aggregator::NumOfEventsAtFrame(FrameNumberType frame) const
    {
        size_t numFrames = m_frameToEventIndex.size();
        if (numFrames == 1)
        {
            return m_events.size();
        }

        if (frame == numFrames - 1)
        {
            return m_events.size() - m_frameToEventIndex[frame]; // last frame
        }

        if (numFrames >= 2)
        {
            EventNumberType ftei1 = m_frameToEventIndex[frame + 1];
            EventNumberType ftei0 = m_frameToEventIndex[frame];
            size_t fte = (ftei1 - ftei0);
            return fte;
        }

        return 0;
    }

    QString Aggregator::GetDialogTitle() const
    {
        return QString("%1 - %2").arg(GetName()).arg(GetInspectionFileName());
    }

    void Aggregator::FrameChanged(FrameNumberType frame)
    {
        size_t numFrames = m_frameToEventIndex.size();
        if (numFrames)
        {
            EventNumberType targetEventIndex;

            if (frame == numFrames - 1)
            {
                targetEventIndex = m_events.size() - 1;
            }
            else
            {
                targetEventIndex = m_frameToEventIndex[frame + 1];

                // the current targetEventIndex belongs to the next frame "frame+1" minus 1
                --targetEventIndex;
            }

            EventChanged(targetEventIndex);
        }
    }

    void Aggregator::EventChanged(EventNumberType eventIndex)
    {
        if (eventIndex == m_currentEvent)
        {
            return;
        }

        // TODO: If we are at the end and we click at the start, we can start from the START as it's a known state
        if (eventIndex > m_currentEvent)
        {
            // forward (m_currentEvent has already been executed so start currentEvent+1)
            for (EventNumberType i = m_currentEvent + 1; i <= eventIndex; ++i)
            {
                m_events[i]->StepForward(this);
            }
        }
        else if (eventIndex < m_currentEvent)
        {
            // backward (current event has executed so revert it
            for (EventNumberType i = m_currentEvent; i > eventIndex; --i)
            {
                m_events[i]->StepBackward(this);
            }
        }

        m_currentEvent = eventIndex;
        emit OnDataCurrentEventChanged();
    }

    void Aggregator::ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings)
    {
        (void)file;
        (void)exportSettings;
    }

    void Aggregator::ExportEventToCSV(AZ::IO::SystemFile& file, const DrillerEvent* drillerEvent, CSVExportSettings* exportSettings)
    {
        (void)file;
        (void)drillerEvent;
        (void)exportSettings;
    }

    void Aggregator::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            AggregatorSavedState::Reflect(context);
        }
    }
}
