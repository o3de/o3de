/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ProfilerDataAggregator.hxx"
#include <Source/Driller/Profiler/moc_ProfilerDataAggregator.cpp>

#include "ProfilerDataView.hxx"
#include "ProfilerEvents.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include "Source/Driller/Workspaces/Workspace.h"

namespace Driller
{
    // used against m_roiVersion to silently clear and reinitialize on internal updates
    static const int dataAggregatorVersion = 2;

    // USER SETTINGS are local only, global settings to the application
    // designed to be used for window placement, global preferences, that kind of thing
    class ProfilerDataAggregatorSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ProfilerDataAggregatorSavedState, "{98494FFE-783F-48A7-A35F-714138425640}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ProfilerDataAggregatorSavedState, AZ::SystemAllocator, 0);

        struct RegisterOfInterest
        {
            AZ_RTTI(RegisterOfInterest, "{885335FD-79D1-4462-B637-177FC0FCF01C}");
            AZStd::string m_name;
            float m_dataScale;
            int m_usesDelta;
            int m_useSubValue;

            virtual ~RegisterOfInterest() {}

            static void Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<RegisterOfInterest>()
                        ->Field("m_name", &RegisterOfInterest::m_name)
                        ->Field("m_dataScale", &RegisterOfInterest::m_dataScale)
                        ->Field("m_usesDelta", &RegisterOfInterest::m_usesDelta)
                        ->Field("m_useSubValue", &RegisterOfInterest::m_useSubValue)
                        ->Version(3);
                }
            }
        };

        int m_activeViewCount;

        AZStd::vector<RegisterOfInterest> m_registersOfInterest;
        int m_roiVersion;

        ProfilerDataAggregatorSavedState()
            : m_activeViewCount(0)
            , m_roiVersion(1)
        {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                RegisterOfInterest::Reflect(context);

                serialize->Class<ProfilerDataAggregatorSavedState>()
                    ->Field("m_activeViewCount", &ProfilerDataAggregatorSavedState::m_activeViewCount)
                    ->Field("m_registersOfInterest", &ProfilerDataAggregatorSavedState::m_registersOfInterest)
                    ->Field("m_roiVersion", &ProfilerDataAggregatorSavedState::m_roiVersion)
                    ->Version(7);
            }
        }
    };

    // WORKSPACES are files loaded and stored independent of the global application
    // designed to be used for DRL data specific view settings and to pass around
    class ProfilerDataAggregatorWorkspace
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ProfilerDataAggregatorWorkspace, "{2C41A0B1-E200-448D-8727-5109DF877B0E}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ProfilerDataAggregatorWorkspace, AZ::SystemAllocator, 0);

        int m_activeViewCount;
        AZStd::vector<int> m_activeViewTypes;

        ProfilerDataAggregatorWorkspace()
            : m_activeViewCount(0)
        {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ProfilerDataAggregatorWorkspace>()
                    ->Field("m_activeViewCount", &ProfilerDataAggregatorWorkspace::m_activeViewCount)
                    ->Field("m_activeViewTypes", &ProfilerDataAggregatorWorkspace::m_activeViewTypes)
                    ->Version(3);
            }
        }
    };


    //////////////////////////////////////////////////////////////////////////
    ProfilerDataAggregator::ProfilerDataAggregator(int identity)
        : Aggregator(identity)
        , m_currentDisplayRegister(0)
        , m_dataView(nullptr)
    {
        m_parser.SetAggregator(this);

        // find state and restore it
        m_persistentState = AZ::UserSettings::CreateFind<ProfilerDataAggregatorSavedState>(AZ_CRC("PROFILER DATA AGGREGATOR SAVED STATE", 0x49c357f6), AZ::UserSettings::CT_GLOBAL);
        AZ_Assert(m_persistentState, "Persistent State is NULL?");
        // please see ::dataAggregatorVersion to control updates
        if (m_persistentState->m_registersOfInterest.empty() || m_persistentState->m_roiVersion != dataAggregatorVersion)
        {
            m_persistentState->m_registersOfInterest.clear();
            m_persistentState->m_registersOfInterest.push_back();
            m_persistentState->m_registersOfInterest[0].m_name = "Component application tick function";
            m_persistentState->m_registersOfInterest[0].m_dataScale = 1.0f / 64000.0f;
            m_persistentState->m_registersOfInterest[0].m_usesDelta = 1;    // this is a delta time calculated on the fly here
            m_persistentState->m_registersOfInterest[0].m_useSubValue = 0;  // user data 0 is m_time from the register union
        }

        m_allRegistersOfInterestInData.clear();
        if (!m_persistentState->m_registersOfInterest.empty())
        {
            m_allRegistersOfInterestInData.resize(m_persistentState->m_registersOfInterest.size(), NULL);
            m_allCorrespondingIdsForRegistersOfInterestInData.resize(m_persistentState->m_registersOfInterest.size(), 0);
        }
    }

    ProfilerDataAggregator::~ProfilerDataAggregator()
    {
        KillAllViews();
    }

    // this aggregator has to dive deeper into the source data
    // to synthesize a meaningful -1...+1 value for the main display
    float ProfilerDataAggregator::ValueAtFrame(FrameNumberType frame)
    {
        size_t numEvents = NumOfEventsAtFrame(frame);
        if (numEvents && frame > 0)
        {
            for (EventNumberType eventIndex = m_frameToEventIndex[frame]; eventIndex < static_cast<EventNumberType>(m_frameToEventIndex[frame] + numEvents); ++eventIndex)
            {
                DrillerEvent* drillerEvent = drillerEvent = GetEvents()[ eventIndex ];
                if (drillerEvent->GetEventType() == Driller::Profiler::PET_UPDATE_REGISTER)
                {
                    Driller::ProfilerDrillerUpdateRegisterEvent* reg = static_cast<Driller::ProfilerDrillerUpdateRegisterEvent*>(drillerEvent);
                    for (auto iter = m_allCorrespondingIdsForRegistersOfInterestInData.begin(); iter != m_allCorrespondingIdsForRegistersOfInterestInData.end(); ++iter)
                    {
                        if (reg->GetRegisterId() == *iter)
                        {
                            float t = 0.0f;
                            if (m_persistentState->m_registersOfInterest[m_currentDisplayRegister].m_usesDelta)
                            {
                                switch (m_persistentState->m_registersOfInterest[m_currentDisplayRegister].m_useSubValue)
                                {
                                case 0:
                                    t = (float)(reg->GetData().m_valueData.m_value1 - (reg->GetPreviousSample() == NULL ? 0 : reg->GetPreviousSample()->GetData().m_valueData.m_value1));
                                    break;
                                case 1:
                                    t = (float)(reg->GetData().m_valueData.m_value2 - (reg->GetPreviousSample() == NULL ? 0 : reg->GetPreviousSample()->GetData().m_valueData.m_value2));
                                    break;
                                case 2:
                                    t = (float)(reg->GetData().m_valueData.m_value3 - (reg->GetPreviousSample() == NULL ? 0 : reg->GetPreviousSample()->GetData().m_valueData.m_value3));
                                    break;
                                case 3:
                                    t = (float)(reg->GetData().m_valueData.m_value4 - (reg->GetPreviousSample() == NULL ? 0 : reg->GetPreviousSample()->GetData().m_valueData.m_value4));
                                    break;
                                }
                            }
                            else
                            {
                                switch (m_persistentState->m_registersOfInterest[m_currentDisplayRegister].m_useSubValue)
                                {
                                case 0:
                                    t = (float)reg->GetData().m_valueData.m_value1;
                                    break;
                                case 1:
                                    t = (float)reg->GetData().m_valueData.m_value2;
                                    break;
                                case 2:
                                    t = (float)reg->GetData().m_valueData.m_value3;
                                    break;
                                case 3:
                                    t = (float)reg->GetData().m_valueData.m_value4;
                                    break;
                                }
                            }

                            t *= m_persistentState->m_registersOfInterest[m_currentDisplayRegister].m_dataScale;
                            t = t * 2.0f - 1.0f;
                            t = t > 1.0f ? 1.0f : t;
                            t = t < -1.0f ? -1.0f : t;
                            return t;
                        }
                    }
                }
            }
        }

        return -1.0f;
    }

    QColor ProfilerDataAggregator::GetColor() const
    {
        return QColor(255, 127, 0);
    }

    QString ProfilerDataAggregator::GetName() const
    {
        return "CPU";
    }

    QString ProfilerDataAggregator::GetChannelName() const
    {
        return ChannelName();
    }

    QString ProfilerDataAggregator::GetDescription() const
    {
        return "Profiler Driller";
    }

    QString ProfilerDataAggregator::GetToolTip() const
    {
        return "Information about CPU usage time and function usage tracking)";
    }

    AZ::Uuid ProfilerDataAggregator::GetID() const
    {
        return AZ::Uuid("{A6DB5318-82BF-416B-BF3D-FFD187329845}");
    }

    QWidget* ProfilerDataAggregator::DrillDownRequest(FrameNumberType frame)
    {
        return DrillDownRequest(frame, Profiler::RegisterInfo::PRT_TIME);
    }

    QWidget* ProfilerDataAggregator::DrillDownRequest(FrameNumberType frame, int viewType)
    {
        Driller::ProfilerDataView* pdv = NULL;

        if (m_dataView)
        {
            KillAllViews();
        }

        pdv = aznew Driller::ProfilerDataView(this, frame, 0, viewType);
        if (pdv)
        {
            m_dataView = pdv;
            connect(pdv, SIGNAL(destroyed(QObject*)), this, SLOT(OnDataViewDestroyed(QObject*)));
            ++m_persistentState->m_activeViewCount;
        }

        return pdv;
    }

    void ProfilerDataAggregator::OptionsRequest()
    {
        char output[64];
        GetID().ToString(output, AZ_ARRAY_SIZE(output), true, true);
        AZ_TracePrintf("Driller", "Options Request for ProfilerDataAggregator %s\n", output);
    }

    void ProfilerDataAggregator::OnDataViewDestroyed(QObject* dataView)
    {
        if (dataView == m_dataView)
        {
            m_dataView = nullptr;
            --m_persistentState->m_activeViewCount;
        }
    }

    void ProfilerDataAggregator::KillAllViews()
    {
        if (m_dataView)
        {
            QObject* object = m_dataView;
            OnDataViewDestroyed(m_dataView);
            m_dataView = nullptr;
            delete object;
        }
    }

    void ProfilerDataAggregator::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* provider)
    {
        ProfilerDataAggregatorWorkspace* workspace = provider->FindSetting<ProfilerDataAggregatorWorkspace>(AZ_CRC("PROFILER DATA AGGREGATOR WORKSPACE", 0xfdb6cb89));
        if (workspace)
        {
            m_persistentState->m_activeViewCount = workspace->m_activeViewCount;
        }
    }
    void ProfilerDataAggregator::ActivateWorkspaceSettings(WorkspaceSettingsProvider* provider)
    {
        ProfilerDataAggregatorWorkspace* workspace = provider->FindSetting<ProfilerDataAggregatorWorkspace>(AZ_CRC("PROFILER DATA AGGREGATOR WORKSPACE", 0xfdb6cb89));
        if (workspace)
        {
            // kill all existing data view windows in preparation of opening the workspace specified ones
            KillAllViews();

            // the internal count should be 0 from the above house cleaning
            // and incremented back up from the workspace instantiations
            m_persistentState->m_activeViewCount = 0;
            for (int i = 0; i < workspace->m_activeViewCount; ++i)
            {
                // older workspaces will not have any active view types
                // therefore this check to default PRT_TIME
                int discoveredType = Profiler::RegisterInfo::PRT_TIME;
                if (workspace->m_activeViewTypes.size() > i)
                {
                    discoveredType = workspace->m_activeViewTypes[i];
                }

                Driller::ProfilerDataView* dataView = qobject_cast<Driller::ProfilerDataView*>(DrillDownRequest(1, discoveredType));
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
    void ProfilerDataAggregator::SaveSettingsToWorkspace(WorkspaceSettingsProvider* provider)
    {
        ProfilerDataAggregatorWorkspace* workspace = provider->CreateSetting<ProfilerDataAggregatorWorkspace>(AZ_CRC("PROFILER DATA AGGREGATOR WORKSPACE", 0xfdb6cb89));
        if (workspace)
        {
            workspace->m_activeViewTypes.clear();
            workspace->m_activeViewCount = m_persistentState->m_activeViewCount;

            if (m_dataView)
            {
                Driller::ProfilerDataView* dataView = qobject_cast<Driller::ProfilerDataView* >(m_dataView);

                if (dataView)
                {
                    workspace->m_activeViewTypes.push_back(dataView->GetViewType());
                    dataView->SaveSettingsToWorkspace(provider);
                }
            }
        }
    }

    //=========================================================================
    // OnEventLoaded
    // [7/10/2013]
    //=========================================================================
    void ProfilerDataAggregator::OnEventLoaded(DrillerEvent* event)
    {
        switch (event->GetEventType())
        {
        case Profiler::PET_NEW_REGISTER:
        {
            Driller::ProfilerDrillerNewRegisterEvent* reg = static_cast<Driller::ProfilerDrillerNewRegisterEvent*>(event);

            for (AZStd::size_t idx = 0; idx < m_persistentState->m_registersOfInterest.size(); ++idx)
            {
                AZStd::string registerName;
                if (reg->GetInfo().m_name == NULL)
                {
                    registerName = AZStd::string::format("%s(%d)"
                            , reg->GetInfo().m_function ? reg->GetInfo().m_function : "N/A"
                            , reg->GetInfo().m_line);
                }
                else
                {
                    registerName = reg->GetInfo().m_name;
                }

                if (!qstricmp(registerName.data(), m_persistentState->m_registersOfInterest[idx].m_name.data()))
                {
                    m_allRegistersOfInterestInData[idx] = reg;
                    m_allCorrespondingIdsForRegistersOfInterestInData[idx] = reg->GetInfo().m_id;
                }
            }

            if (m_lifeTimeThreads.find(reg->GetInfo().m_threadId) == m_lifeTimeThreads.end())
            {
                // this register belongs to a thread which was not AZStd::thread or was NOT reported to the
                // AZStd::ThreadEventBus. This is possible for middleware and so on. Although we should attempt
                // to report those threads too (the best we can, with some name at least)
                // as of now just add the id.
                // NB: threadId can be be defaulted at 0 if this is an older data set
                // in which case we do not add it to the threads
                if (reg->GetInfo().m_threadId != 0)
                {
                    if (m_lifeTimeThreads.find(reg->GetInfo().m_threadId) == m_lifeTimeThreads.end())
                    {
                        m_lifeTimeThreads.insert(AZStd::make_pair(reg->GetInfo().m_threadId, nullptr));
                    }
                }
            }
            break;
        }
        case Profiler::PET_UPDATE_REGISTER:
        {
            Driller::ProfilerDrillerUpdateRegisterEvent* reg = static_cast<Driller::ProfilerDrillerUpdateRegisterEvent*>(event);

            for (AZStd::size_t idx = 0; idx < m_allCorrespondingIdsForRegistersOfInterestInData.size(); ++idx)
            {
                if (reg->GetRegisterId() == m_allCorrespondingIdsForRegistersOfInterestInData[idx])
                {
                    reg->PreComputeForward(m_allRegistersOfInterestInData[idx]);
                }
            }
        }
        break;

        case Profiler::PET_ENTER_THREAD:
        {
            // make sure we have a valid list with all the threads in the world
            ProfilerDrillerEnterThreadEvent* newThread = static_cast<ProfilerDrillerEnterThreadEvent*>(event);
            if (m_lifeTimeThreads.find(newThread->m_threadId) == m_lifeTimeThreads.end())
            {
                m_lifeTimeThreads.insert(AZStd::make_pair(newThread->m_threadId, newThread));
            }
        } break;
        }
    }

    //=========================================================================
    // Reset()
    // [7/10/2013]
    //=========================================================================
    void ProfilerDataAggregator::Reset()
    {
        m_systems.clear();
        m_threads.clear();
        m_lifeTimeThreads.clear();
        m_registers.clear();

        KillAllViews();
    }

    void ProfilerDataAggregator::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            ProfilerDataAggregatorSavedState::Reflect(context);
            ProfilerDataAggregatorWorkspace::Reflect(context);
            ProfilerDataView::Reflect(context);

            serialize->Class<ProfilerDataAggregator>()
                ->Version(1)
                ->SerializeWithNoData();
        }
    }
} // namespace Driller
