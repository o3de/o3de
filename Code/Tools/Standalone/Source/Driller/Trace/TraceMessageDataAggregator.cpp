/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "TraceMessageDataAggregator.hxx"
#include <Source/Driller/Trace/moc_TraceMessageDataAggregator.cpp>

#include "TraceDrillerDialog.hxx"
#include "TraceMessageEvents.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <Source/Driller/Annotations/Annotations.hxx>
#include <Source/Driller/Workspaces/Workspace.h>

namespace Driller
{
    class TraceMessageDataAggregatorSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(TraceMessageDataAggregatorSavedState, "{48FADE93-10C0-48BE-96FA-44EFE49D8ED3}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(TraceMessageDataAggregatorSavedState, AZ::SystemAllocator, 0);
        TraceMessageDataAggregatorSavedState()
            : m_activeViewCount(0)
        {}

        int m_activeViewCount;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<TraceMessageDataAggregatorSavedState>()
                    ->Field("m_activeViewCount", &TraceMessageDataAggregatorSavedState::m_activeViewCount)
                    ->Version(2);
            }
        }
    };


    TraceMessageDataAggregator::TraceMessageDataAggregator(int identity)
        : Aggregator(identity)
        , m_dataView(nullptr)
    {
        m_parser.SetAggregator(this);

        m_persistentState = AZ::UserSettings::CreateFind<TraceMessageDataAggregatorSavedState>(AZ_CRC("TRACE MESSAGE DATA AGGREGATOR SAVED STATE", 0xa4996e1f), AZ::UserSettings::CT_GLOBAL);
        AZ_Assert(m_persistentState, "Persistent State is NULL?");
    }

    float TraceMessageDataAggregator::ValueAtFrame(FrameNumberType frame)
    {
        return NumOfEventsAtFrame(frame) > 0 ? 1.0f : -1.0f;
    }

    QColor TraceMessageDataAggregator::GetColor() const
    {
        return QColor(0, 255, 0);
    }

    QString TraceMessageDataAggregator::GetName() const
    {
        return "Trace messages";
    }

    QString TraceMessageDataAggregator::GetChannelName() const
    {
        return ChannelName();
    }

    QString TraceMessageDataAggregator::GetDescription() const
    {
        return "All trace messages";
    }

    QString TraceMessageDataAggregator::GetToolTip() const
    {
        return "Logged Messages from Application";
    }

    AZ::Uuid TraceMessageDataAggregator::GetID() const
    {
        return AZ::Uuid("{368D6FB2-9A92-4DFE-8DB4-4F106194BA6F}");
    }

    QWidget* TraceMessageDataAggregator::DrillDownRequest(FrameNumberType frame)
    {
        (void)frame;

        if (m_dataView)
        {
            m_dataView->deleteLater();
            OnDataViewDestroyed(m_dataView);
            m_dataView = nullptr;
        }

        TraceDrillerDialog* dv = aznew TraceDrillerDialog(this, (1024 * GetIdentity()) + 0);
        dv->show();
        m_dataView = dv;

        connect(dv, SIGNAL(destroyed(QObject*)), this, SLOT(OnDataViewDestroyed(QObject*)));
        ++m_persistentState->m_activeViewCount;

        return dv;
    }
    void TraceMessageDataAggregator::OptionsRequest()
    {
    }
    void TraceMessageDataAggregator::OnDataViewDestroyed(QObject* dataView)
    {
        if (dataView == m_dataView)
        {
            m_dataView = nullptr;
            --m_persistentState->m_activeViewCount;
        }
    }

    void TraceMessageDataAggregator::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* provider)
    {
        TraceMessageDataAggregatorSavedState* workspace = provider->FindSetting<TraceMessageDataAggregatorSavedState>(AZ_CRC("TRACE MESSAGE DATA AGGREGATOR WORKSPACE", 0xff055f40));
        if (workspace)
        {
            m_persistentState->m_activeViewCount = workspace->m_activeViewCount;
        }
    }
    void TraceMessageDataAggregator::ActivateWorkspaceSettings(WorkspaceSettingsProvider* provider)
    {
        TraceMessageDataAggregatorSavedState* workspace = provider->FindSetting<TraceMessageDataAggregatorSavedState>(AZ_CRC("TRACE MESSAGE DATA AGGREGATOR WORKSPACE", 0xff055f40));
        if (workspace)
        {
            // kill all existing data view windows in preparation of opening the workspace specified ones
            delete m_dataView;

            // the internal count should be 0 from the above house cleaning
            // and incremented back up from the workspace instantiations
            m_persistentState->m_activeViewCount = 0;
            for (int i = 0; i < workspace->m_activeViewCount; ++i)
            {
                // driller must be created at (frame > 0) for it to have a valid tree to display
                TraceDrillerDialog* dataView = qobject_cast<TraceDrillerDialog*>(DrillDownRequest(1));
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

    void TraceMessageDataAggregator::SaveSettingsToWorkspace(WorkspaceSettingsProvider* provider)
    {
        TraceMessageDataAggregatorSavedState* workspace = provider->CreateSetting<TraceMessageDataAggregatorSavedState>(AZ_CRC("TRACE MESSAGE DATA AGGREGATOR WORKSPACE", 0xff055f40));
        if (workspace)
        {
            workspace->m_activeViewCount = m_persistentState->m_activeViewCount;

            if (m_dataView)
            {
                qobject_cast<TraceDrillerDialog*>(m_dataView)->SaveSettingsToWorkspace(provider);
            }
        }
    }


    // emit all annotations that match the provider's filter, given the start and end frame:
    void TraceMessageDataAggregator::EmitAllAnnotationsForFrameRange(int startFrameInclusive, int endFrameInclusive, AnnotationsProvider* ptrProvider)
    {
        for (; startFrameInclusive <= endFrameInclusive; ++startFrameInclusive)
        {
            AZ::s64 startEvent = m_frameToEventIndex[startFrameInclusive];
            AZ::s64 endEvent = startEvent + NumOfEventsAtFrame(startFrameInclusive);

            for (AZ::s64 i = startEvent; i < endEvent; ++i)
            {
                TraceMessageEvent* event = static_cast<TraceMessageEvent*>(m_events[i]);
                if (ptrProvider->IsChannelEnabled(event->m_windowCRC))
                {
                    ptrProvider->AddAnnotation(Driller::Annotation(m_events[i]->GetGlobalEventId(), startFrameInclusive, event->m_message, event->m_window));
                }
            }
        }
    }

    // emit all channels that you are aware of existing within that frame range (You may emit duplicate channels, they will be ignored)
    void TraceMessageDataAggregator::EmitAnnotationChannelsForFrameRange(int startFrameInclusive, int endFrameInclusive, AnnotationsProvider* ptrProvider)
    {
        for (; startFrameInclusive <= endFrameInclusive; ++startFrameInclusive)
        {
            AZ::s64 startEvent = m_frameToEventIndex[startFrameInclusive];
            AZ::s64 endEvent = startEvent + NumOfEventsAtFrame(startFrameInclusive);

            for (AZ::s64 i = startEvent; i < endEvent; ++i)
            {
                TraceMessageEvent* event = static_cast<TraceMessageEvent*>(m_events[i]);
                ptrProvider->NotifyOfChannelExistence(event->m_window);
            }
        }
    }

    void TraceMessageDataAggregator::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            TraceMessageDataAggregatorSavedState::Reflect(context);
            TraceDrillerDialog::Reflect(context);

            serialize->Class<TraceMessageDataAggregator>()
                ->Version(1)
                ->SerializeWithNoData();
        }
    }
} // namespace Driller
