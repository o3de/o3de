/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Woodpecker_precompiled.h"

#include "StreamerDataAggregator.hxx"
#include <Woodpecker/Driller/IO/moc_StreamerDataAggregator.cpp>

#include "StreamerDrillerDialog.hxx"
#include "StreamerEvents.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include "Woodpecker/Driller/Workspaces/Workspace.h"

#include <QMenu>
#include <QAction>

namespace Driller
{
    class StreamerDataAggregatorSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(StreamerDataAggregatorSavedState, "{0174A3EE-C555-482F-9E7B-7D67D9B4B0A7}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(StreamerDataAggregatorSavedState, AZ::SystemAllocator, 0);
        StreamerDataAggregatorSavedState() {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<StreamerDataAggregatorSavedState>()
                    ->Version(1)
                    ;
            }
        }
    };

    // WORKSPACES are files loaded and stored independent of the global application
    // designed to be used for DRL data specific view settings and to pass around
    class StreamerDataAggregatorWorkspace
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(StreamerDataAggregatorWorkspace, "{D35E8CCA-6FA7-47F6-8A24-8E12EF237E40}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(StreamerDataAggregatorWorkspace, AZ::SystemAllocator, 0);

        int m_activeViewCount;
        AZStd::vector<int> m_activeViewTypes;

        StreamerDataAggregatorWorkspace()
            : m_activeViewCount(0)
        {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<StreamerDataAggregatorWorkspace>()
                    ->Field("m_activeViewCount", &StreamerDataAggregatorWorkspace::m_activeViewCount)
                    ->Field("m_activeViewTypes", &StreamerDataAggregatorWorkspace::m_activeViewTypes)
                    ->Version(1);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    StreamerDataAggregator::StreamerDataAggregator(int identity)
        : Aggregator(identity)
        , m_activeViewCount(0)
        , m_highwaterFrame(-1)
    {
        m_parser.SetAggregator(this);
        ResetTrackingInfo();
    }

    StreamerDataAggregator::~StreamerDataAggregator()
    {
        KillAllViews();
    }

    // gross generalization of activity based on total number of all events this frame
    float StreamerDataAggregator::ValueAtFrame(FrameNumberType frame)
    {
        const float maxEventsPerFrame = 10.0f; // just a scale number
        float numEventsPerFrame = static_cast<float>(NumOfEventsAtFrame(frame));
        return AZStd::GetMin(numEventsPerFrame / maxEventsPerFrame, 1.0f) * 2.0f - 1.0f;
    }

    QColor StreamerDataAggregator::GetColor() const
    {
        return QColor(0, 255, 255);
    }

    QString StreamerDataAggregator::GetName() const
    {
        return "Streamer";
    }

    QString StreamerDataAggregator::GetChannelName() const
    {
        return ChannelName();
    }

    QString StreamerDataAggregator::GetDescription() const
    {
        return "Streamer events driller";
    }

    QString StreamerDataAggregator::GetToolTip() const
    {
        return "Streamer Events";
    }

    AZ::Uuid StreamerDataAggregator::GetID() const
    {
        return AZ::Uuid("{9A2854C8-8106-4075-9287-3E047821D934}");
    }

    QWidget* StreamerDataAggregator::DrillDownRequest(FrameNumberType frame)
    {
        StreamerDrillerDialog* dialog = NULL;

        AZ::u32 availableIdx = 0;
        bool foundASpot = true;
        do
        {
            foundASpot = true;
            for (DataViewMap::iterator iter = m_dataViews.begin(); iter != m_dataViews.end(); ++iter)
            {
                if (iter->second == availableIdx)
                {
                    foundASpot = false;
                    ++availableIdx;
                    break;
                }
            }
        } while (!foundASpot);

        dialog = aznew StreamerDrillerDialog(this, frame, (1024 * GetIdentity()) + availableIdx);
        if (dialog)
        {
            m_dataViews[dialog] = availableIdx;
            connect(dialog, SIGNAL(destroyed(QObject*)), this, SLOT(OnDataViewDestroyed(QObject*)));
            ++m_activeViewCount;
        }

        return dialog;
    }

    QWidget* StreamerDataAggregator::DrillDownRequest(FrameNumberType frame, int type)
    {
        StreamerDrillerDialog* view = static_cast<StreamerDrillerDialog*>(DrillDownRequest(frame));
        if (view)
        {
            view->SetChartType(type);
        }

        return view;
    }

    void StreamerDataAggregator::OptionsRequest()
    {
        QMenu* popupMenu = new QMenu();
        QMenu* cachedHitsTypeMenu = new QMenu(tr("Cached Hits"));
        cachedHitsTypeMenu->addAction(tr("Do Not Report Cache Hits"));
        cachedHitsTypeMenu->addAction(tr("Report Cache Hits"));
        popupMenu->addMenu(cachedHitsTypeMenu);
        QAction* act = cachedHitsTypeMenu->exec(QCursor::pos());
        if (act)
        {
            if (act->text() == tr("Report Cache Hits"))
            {
                m_parser.AllowCacheHitsInReportedStream(true);
            }
            if (act->text() == tr("Do Not Report Cache Hits"))
            {
                m_parser.AllowCacheHitsInReportedStream(false);
            }
        }
    }

    void StreamerDataAggregator::OnDataViewDestroyed(QObject* dataView)
    {
        m_dataViews.erase(dataView);
        --m_activeViewCount;
    }

    void StreamerDataAggregator::KillAllViews()
    {
        do
        {
            DataViewMap::iterator iter = m_dataViews.begin();
            if (iter != m_dataViews.end())
            {
                delete iter->first;
                continue;
            }
            break;
        } while (1);
    }

    void StreamerDataAggregator::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* provider)
    {
        StreamerDataAggregatorWorkspace* workspace = provider->FindSetting<StreamerDataAggregatorWorkspace>(AZ_CRC("STREAMER DATA AGGREGATOR WORKSPACE", 0x105be192));
        if (workspace)
        {
            m_activeViewCount = workspace->m_activeViewCount;
        }
    }
    void StreamerDataAggregator::ActivateWorkspaceSettings(WorkspaceSettingsProvider* provider)
    {
        StreamerDataAggregatorWorkspace* workspace = provider->FindSetting<StreamerDataAggregatorWorkspace>(AZ_CRC("STREAMER DATA AGGREGATOR WORKSPACE", 0x105be192));
        if (workspace)
        {
            // kill all existing data view windows in preparation of opening the workspace specified ones
            KillAllViews();

            // the internal count should be 0 from the above house cleaning
            // and incremented back up from the workspace instantiations
            m_activeViewCount = 0;
            for (int i = 0; i < workspace->m_activeViewCount; ++i)
            {
                //// start this check to default
                int discoveredType = 0;
                if (workspace->m_activeViewTypes.size() > i)
                {
                    discoveredType = workspace->m_activeViewTypes[i];
                }

                Driller::StreamerDrillerDialog* dataView = qobject_cast<Driller::StreamerDrillerDialog*>(DrillDownRequest(1, discoveredType));
                if (dataView)
                {
                    // apply will overlay the workspace settings on top of the local user settings
                    dataView->ApplySettingsFromWorkspace(provider);
                    // activate will do the heavy lifting
                    dataView->ActivateWorkspaceSettings(provider);
                }
            }
        }
    }
    void StreamerDataAggregator::SaveSettingsToWorkspace(WorkspaceSettingsProvider* provider)
    {
        StreamerDataAggregatorWorkspace* workspace = provider->CreateSetting<StreamerDataAggregatorWorkspace>(AZ_CRC("STREAMER DATA AGGREGATOR WORKSPACE", 0x105be192));
        if (workspace)
        {
            workspace->m_activeViewTypes.clear();
            workspace->m_activeViewCount = m_activeViewCount;

            for (DataViewMap::iterator iter = m_dataViews.begin(); iter != m_dataViews.end(); ++iter)
            {
                Driller::StreamerDrillerDialog* dataView = qobject_cast<Driller::StreamerDrillerDialog* >(iter->first);
                if (dataView)
                {
                    workspace->m_activeViewTypes.push_back(dataView->GetViewType());
                    dataView->SaveSettingsToWorkspace(provider);
                }
            }
        }
    }

    //=========================================================================
    // Reset
    // [7/10/2013]
    //=========================================================================
    void StreamerDataAggregator::Reset()
    {
        m_devices.clear();
        m_streams.clear();
        m_requests.clear();
        m_seeksInfo.clear();
        ResetTrackingInfo();
    }

    void StreamerDataAggregator::ResetTrackingInfo()
    {
        m_seekTracking.clear();
        m_highwaterFrame = -1;

        // default device with ID := 0
        SeekTrackingInfo seekTrackingInfo;
        seekTrackingInfo.m_currentStreamId = 0;
        seekTrackingInfo.m_offset = 0;
        m_seekTracking.insert(AZStd::make_pair(0, seekTrackingInfo));
    }

    void StreamerDataAggregator::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            StreamerDataAggregatorSavedState::Reflect(context);
            StreamerDataAggregatorWorkspace::Reflect(context);
            StreamerDrillerDialog::Reflect(context);

            serialize->Class<StreamerDataAggregator>()
                ->Version(1)
                ;
        }
    }

    void StreamerDataAggregator::AdvanceToFrame(FrameNumberType frame)
    {
        while (m_highwaterFrame < frame)
        {
            ++m_highwaterFrame;
            FrameChanged(m_highwaterFrame);

            m_frameInfo.push_back();
            m_frameInfo.back().m_computedSeeksCount = 0;
            m_frameInfo.back().m_computedThroughput = 0;

            AZ::s64 numEvents = NumOfEventsAtFrame(m_highwaterFrame);
            if (numEvents)
            {
                AZ::s64 firstIndex = GetFirstIndexAtFrame(m_highwaterFrame);

                for (AZ::s64 idx = 0; idx < numEvents; ++idx)
                {
                    DrillerEvent* dep = m_events[ idx + firstIndex ];
                    AZ::u64 geid = dep->GetGlobalEventId();

                    switch (dep->GetEventType())
                    {
                    case Driller::Streamer::SET_DEVICE_MOUNTED:
                    {
                        SeekTrackingInfo seekTrackingInfo;
                        seekTrackingInfo.m_currentStreamId = 0;
                        seekTrackingInfo.m_offset = 0;
                        m_seekTracking.insert(AZStd::make_pair(static_cast<StreamerMountDeviceEvent*>(dep)->m_deviceData.m_id, seekTrackingInfo));
                        break;
                    }
                    case Driller::Streamer::SET_DEVICE_UNMOUNTED:
                    {
                        m_seekTracking.erase(static_cast<StreamerUnmountDeviceEvent*>(dep)->m_unmountedDeviceData->m_id);
                        break;
                    }
                    case Driller::Streamer::SET_ADD_REQUEST:
                    {
                        break;
                    }
                    case Driller::Streamer::SET_CANCEL_REQUEST:
                    case Driller::Streamer::SET_RESCHEDULE_REQUEST:
                    case Driller::Streamer::SET_COMPLETE_REQUEST:
                    {
                        break;
                    }
                    case Driller::Streamer::SET_REGISTER_STREAM:
                    case Driller::Streamer::SET_UNREGISTER_STREAM:
                    {
                        break;
                    }
                    // m_stream := possibly NULL in these two cases
                    case Driller::Streamer::SET_OPERATION_START:
                    {
                        auto depEvt = static_cast<StreamerOperationStartEvent*>(dep);
                        if (depEvt->m_stream)
                        {
                            auto trackedDevice = m_seekTracking.find(depEvt->m_stream->m_deviceId);
                            if (trackedDevice == m_seekTracking.end())
                            {
                                SeekTrackingInfo seekTrackingInfo;
                                seekTrackingInfo.m_currentStreamId = 0;
                                seekTrackingInfo.m_offset = 0;

                                m_seekTracking.insert(AZStd::make_pair(depEvt->m_stream->m_deviceId, seekTrackingInfo));
                                trackedDevice = m_seekTracking.find(depEvt->m_stream->m_deviceId);
                            }
                            // reasons a device might SEEK
                            if (trackedDevice->second.m_currentStreamId != depEvt->m_streamId)
                            {
                                if (
                                    (depEvt->m_stream->m_isCompressed && depEvt->m_operation.m_type == Streamer::SOP_COMPRESSOR_READ)
                                    ||
                                    (!depEvt->m_stream->m_isCompressed)
                                    )
                                {
                                    m_frameInfo.back().m_seekInfo.push_back();
                                    m_frameInfo.back().m_seekInfo.back().m_eventId = geid;
                                    m_frameInfo.back().m_seekInfo.back().m_eventReason = SEEK_EVENT_SWITCH;
                                    trackedDevice->second.m_currentStreamId = depEvt->m_streamId;
                                    trackedDevice->second.m_offset = depEvt->m_operation.m_offset;
                                    ++m_frameInfo.back().m_computedSeeksCount;
                                    m_seeksInfo.insert(AZStd::make_pair(geid, SEEK_EVENT_SWITCH));
                                }
                            }
                            else if (trackedDevice->second.m_offset != depEvt->m_operation.m_offset)
                            {
                                if (
                                    (depEvt->m_stream->m_isCompressed && depEvt->m_operation.m_type == Streamer::SOP_COMPRESSOR_READ)
                                    ||
                                    (!depEvt->m_stream->m_isCompressed)
                                    )
                                {
                                    m_frameInfo.back().m_seekInfo.push_back();
                                    m_frameInfo.back().m_seekInfo.back().m_eventId = geid;
                                    m_frameInfo.back().m_seekInfo.back().m_eventReason = SEEK_EVENT_SKIP;
                                    trackedDevice->second.m_offset = depEvt->m_operation.m_offset;
                                    ++m_frameInfo.back().m_computedSeeksCount;
                                    m_seeksInfo.insert(AZStd::make_pair(geid, SEEK_EVENT_SKIP));
                                }
                            }
                        }
                        break;
                    }
                    case Driller::Streamer::SET_OPERATION_COMPLETE:
                    {
                        auto depEvt = azdynamic_cast<StreamerOperationCompleteEvent*>(dep);

                        if (depEvt->m_stream)
                        {
                            m_frameInfo.back().m_transferInfo.push_back();
                            m_frameInfo.back().m_transferInfo.back().m_eventId = geid;
                            m_frameInfo.back().m_transferInfo.back().m_eventReason = (TransferEventType)(depEvt->m_type);

                            auto trackedDevice = m_seekTracking.find(depEvt->m_stream->m_deviceId);
                            if (trackedDevice == m_seekTracking.end())
                            {
                                SeekTrackingInfo seekTrackingInfo;
                                seekTrackingInfo.m_currentStreamId = 0;
                                seekTrackingInfo.m_offset = 0;

                                m_seekTracking.insert(AZStd::make_pair(depEvt->m_stream->m_deviceId, seekTrackingInfo));
                                trackedDevice = m_seekTracking.find(depEvt->m_stream->m_deviceId);
                            }

                            if (depEvt->m_stream->m_isCompressed)
                            {
                                if (static_cast<TransferEventType>(depEvt->m_type) == TRANSFER_EVENT_COMPRESSOR_READ || static_cast<TransferEventType>(depEvt->m_type) == TRANSFER_EVENT_COMPRESSOR_WRITE)
                                {
                                    m_frameInfo.back().m_transferInfo.back().m_byteCount = depEvt->m_bytesTransferred;
                                    m_frameInfo.back().m_computedThroughput += depEvt->m_bytesTransferred;
                                    trackedDevice->second.m_offset += depEvt->m_bytesTransferred;
                                }
                            }
                            else
                            {
                                m_frameInfo.back().m_transferInfo.back().m_byteCount = depEvt->m_bytesTransferred;
                                m_frameInfo.back().m_computedThroughput += depEvt->m_bytesTransferred;
                                trackedDevice->second.m_offset += depEvt->m_bytesTransferred;
                            }
                        }
                        break;
                    }
                    }
                }
            }
        }
    }

    const char* StreamerDataAggregator::GetFilenameFromStreamId(unsigned int globalEventId, AZ::u64 streamId)
    {
        // starting at eventId, skip backwards through the events until a register stream is found
        while (1)
        {
            if (globalEventId > m_events.size())
            {
                break;
            }
            auto event = m_events[globalEventId];
            auto registerEvent = azdynamic_cast<StreamerRegisterStreamEvent*>(event);
            if (registerEvent && registerEvent->m_streamData.m_id == streamId)
            {
                return registerEvent->m_streamData.m_name;
            }

            if (globalEventId == 0)
            {
                break;
            }

            --globalEventId;
        }

        return "";
    }

    const char* StreamerDataAggregator::GetDebugNameFromStreamId(unsigned int globalEventId, AZ::u64 streamId)
    {
        // starting at eventId, skip backwards through the events until a request added (which has debug info) is found
        while (1)
        {
            if (globalEventId > m_events.size())
            {
                break;
            }
            auto event = m_events[globalEventId];
            auto requestEvent = azdynamic_cast<StreamerAddRequestEvent*>(event);
            if (requestEvent && requestEvent->m_requestData.m_streamId == streamId)
            {
                if (requestEvent->m_requestData.m_debugName)
                {
                    return requestEvent->m_requestData.m_debugName;
                }
                else
                {
                    return "";
                }
            }

            if (globalEventId == 0)
            {
                break;
            }

            --globalEventId;
        }

        return "";
    }


    const AZ::u64 StreamerDataAggregator::GetOffsetFromStreamId(unsigned int globalEventId, AZ::u64 streamId)
    {
        // starting at eventId, skip backwards through the events until the start operation related to this ID is found
        while (1)
        {
            if (globalEventId > m_events.size())
            {
                break;
            }
            auto event = m_events[globalEventId];
            auto startEvent = azdynamic_cast<StreamerOperationStartEvent*>(event);
            if (startEvent && startEvent->m_streamId == streamId)
            {
                return startEvent->m_operation.m_offset;
            }

            if (globalEventId == 0)
            {
                break;
            }

            --globalEventId;
        }

        return 0;
    }

    float StreamerDataAggregator::ThroughputAtFrame(FrameNumberType frame)
    {
        if (frame >= 0)
        {
            AdvanceToFrame(frame);
            return (float)m_frameInfo[frame].m_computedThroughput;
        }

        return 0.0f;
    }
    float StreamerDataAggregator::SeeksAtFrame(FrameNumberType frame)
    {
        if (frame >= 0)
        {
            AdvanceToFrame(frame);
            return (float)m_frameInfo[frame].m_computedSeeksCount;
        }

        return 0.0f;
    }
    Driller::StreamerDataAggregator::TransferBreakoutType& StreamerDataAggregator::ThroughputAtFrameBreakout(FrameNumberType frame)
    {
        AdvanceToFrame(frame);
        return m_frameInfo[frame].m_transferInfo;
    }
    Driller::StreamerDataAggregator::SeeksBreakoutType& StreamerDataAggregator::SeeksAtFrameBreakout(FrameNumberType frame)
    {
        AdvanceToFrame(frame);
        return m_frameInfo[frame].m_seekInfo;
    }
    StreamerDataAggregator::SeekEventType StreamerDataAggregator::GetSeekType(AZ::s64 id)
    {
        auto iter = m_seeksInfo.find(id);
        if (iter != m_seeksInfo.end())
        {
            return iter->second;
        }
        return SEEK_EVENT_NONE;
    }
} // namespace Driller
