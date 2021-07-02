/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "MemoryDataAggregator.hxx"
#include <Source/Driller/Memory/moc_MemoryDataAggregator.cpp>
#include "MemoryDataView.hxx"

#include "MemoryEvents.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include "Source/Driller/Workspaces/Workspace.h"

namespace Driller
{
    class MemoryDataAggregatorSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(MemoryDataAggregatorSavedState, "{9A117AF1-842B-43C4-8E98-F08E8080579A}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(MemoryDataAggregatorSavedState, AZ::SystemAllocator, 0);
        MemoryDataAggregatorSavedState()
            : m_activeViewCount(0)
        {}

        int m_activeViewCount;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<MemoryDataAggregatorSavedState>()
                    ->Field("m_activeViewCount", &MemoryDataAggregatorSavedState::m_activeViewCount)
                    ->Version(2);
            }
        }
    };

    class MemoryDataAggregatorWorkspace
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(MemoryDataAggregatorWorkspace, "{4CBE496B-1CC3-4219-A0E2-D88850F6BCFD}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(MemoryDataAggregatorWorkspace, AZ::SystemAllocator, 0);

        int m_activeViewCount;

        MemoryDataAggregatorWorkspace()
            : m_activeViewCount(0)
        {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<MemoryDataAggregatorWorkspace, AZ::UserSettings>()
                    ->Field("m_activeViewCount", &MemoryDataAggregatorWorkspace::m_activeViewCount)
                    ->Version(2);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    MemoryDataAggregator::MemoryDataAggregator(int identity)
        : Aggregator(identity)
    {
        m_parser.SetAggregator(this);

        m_persistentState = AZ::UserSettings::CreateFind<MemoryDataAggregatorSavedState>(AZ_CRC("MEMORY DATA AGGREGATOR SAVED STATE", 0x672155eb), AZ::UserSettings::CT_GLOBAL);
        AZ_Assert(m_persistentState, "Persistent State is NULL?");
    }

    MemoryDataAggregator::~MemoryDataAggregator()
    {
        KillAllViews();
    }

    float MemoryDataAggregator::ValueAtFrame(FrameNumberType frame)
    {
        const float maxEventsPerFrame = 1000.0f; // just a scale number
        float numEventsPerFrame = static_cast<float>(NumOfEventsAtFrame(frame));
        return AZStd::GetMin(numEventsPerFrame / maxEventsPerFrame, 1.0f) * 2.0f - 1.0f;
    }

    QColor MemoryDataAggregator::GetColor() const
    {
        return QColor(255, 0, 0);
    }

    QString MemoryDataAggregator::GetName() const
    {
        return "Memory";
    }

    QString MemoryDataAggregator::GetChannelName() const
    {
        return ChannelName();
    }

    QString MemoryDataAggregator::GetDescription() const
    {
        return "Memory allocations driller";
    }

    QString MemoryDataAggregator::GetToolTip() const
    {
        return "Information about Memory allocations";
    }

    AZ::Uuid MemoryDataAggregator::GetID() const
    {
        return AZ::Uuid("{D97E63EC-D85C-4DBB-B7CD-B092E2AB3A63}");
    }

    QWidget* MemoryDataAggregator::DrillDownRequest(FrameNumberType frame)
    {
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

        Driller::MemoryDataView* dv = NULL;
        if (m_events.size())
        {
            dv = aznew Driller::MemoryDataView(this, frame, (1024 * GetIdentity()) + availableIdx);
            if (dv)
            {
                m_dataViews[dv] = availableIdx;
                connect(dv, SIGNAL(destroyed(QObject*)), this, SLOT(OnDataViewDestroyed(QObject*)));
                ++m_persistentState->m_activeViewCount;
            }
        }

        return dv;
    }
    void MemoryDataAggregator::OptionsRequest()
    {
    }
    void MemoryDataAggregator::OnDataViewDestroyed(QObject* dataView)
    {
        m_dataViews.erase(static_cast<Driller::MemoryDataView*>(dataView));
        --m_persistentState->m_activeViewCount;
    }

    MemoryDataAggregator::AllocatorInfoArrayType::iterator MemoryDataAggregator::FindAllocatorById(AZ::u64 id)
    {
        for (MemoryDataAggregator::AllocatorInfoArrayType::iterator alIt = m_allocators.begin(); alIt != m_allocators.end(); ++alIt)
        {
            if ((*alIt)->m_id == id)
            {
                return alIt;
            }
        }
        return m_allocators.end();
    }

    MemoryDataAggregator::AllocatorInfoArrayType::iterator MemoryDataAggregator::FindAllocatorByRecordsId(AZ::u64 recordsId)
    {
        for (MemoryDataAggregator::AllocatorInfoArrayType::iterator alIt = m_allocators.begin(); alIt != m_allocators.end(); ++alIt)
        {
            if ((*alIt)->m_recordsId == recordsId)
            {
                return alIt;
            }
        }
        return m_allocators.end();
    }

    void MemoryDataAggregator::KillAllViews()
    {
        do
        {
            DataViewMap::iterator iter = m_dataViews.begin();
            if (iter != m_dataViews.end())
            {
                iter->first->hide();
                delete iter->first;
                continue;
            }
            break;
        } while (1);
    }

    void MemoryDataAggregator::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* provider)
    {
        MemoryDataAggregatorWorkspace* workspace = provider->FindSetting<MemoryDataAggregatorWorkspace>(AZ_CRC("MEMORY DATA AGGREGATOR WORKSPACE", 0x41ee95bc));
        if (workspace)
        {
            m_persistentState->m_activeViewCount = workspace->m_activeViewCount;
        }
    }
    void MemoryDataAggregator::ActivateWorkspaceSettings(WorkspaceSettingsProvider* provider)
    {
        MemoryDataAggregatorWorkspace* workspace = provider->FindSetting<MemoryDataAggregatorWorkspace>(AZ_CRC("MEMORY DATA AGGREGATOR WORKSPACE", 0x41ee95bc));
        if (workspace)
        {
            // kill all existing data view windows in preparation of opening the workspace specified ones
            KillAllViews();

            // the internal count should be 0 from the above house cleaning
            // and incremented back up from the workspace instantiations
            m_persistentState->m_activeViewCount = 0;
            for (int i = 0; i < workspace->m_activeViewCount; ++i)
            {
                // driller must be created at (frame > 0) for it to have a valid tree to display
                Driller::MemoryDataView* dataView = qobject_cast<Driller::MemoryDataView*>(DrillDownRequest(1));
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
    void MemoryDataAggregator::SaveSettingsToWorkspace(WorkspaceSettingsProvider* provider)
    {
        MemoryDataAggregatorWorkspace* workspace = provider->CreateSetting<MemoryDataAggregatorWorkspace>(AZ_CRC("MEMORY DATA AGGREGATOR WORKSPACE", 0x41ee95bc));
        if (workspace)
        {
            workspace->m_activeViewCount = m_persistentState->m_activeViewCount;

            for (DataViewMap::iterator iter = m_dataViews.begin(); iter != m_dataViews.end(); ++iter)
            {
                iter->first->SaveSettingsToWorkspace(provider);
            }
        }
    }

    //=========================================================================
    // Reset
    // [7/10/2013]
    //=========================================================================
    void MemoryDataAggregator::Reset()
    {
        m_allocators.clear();
    }

    void MemoryDataAggregator::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            MemoryDataAggregatorSavedState::Reflect(context);
            MemoryDataAggregatorWorkspace::Reflect(context);
            MemoryDataView::Reflect(context);

            serialize->Class<MemoryDataAggregator>()
                ->Version(1)
                ->SerializeWithNoData();
        }
    }
} // namespace Driller
