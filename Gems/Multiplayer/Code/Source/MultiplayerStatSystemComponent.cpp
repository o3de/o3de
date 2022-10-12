/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/MultiplayerStatSystemComponent.h>

namespace Multiplayer
{
    // Metrics cvars
    void OnEnableNetworkingMetricsChanged(const bool& enabled);
    AZ_CVAR(bool, bg_enableNetworkingMetrics, true, &OnEnableNetworkingMetricsChanged, AZ::ConsoleFunctorFlags::DontReplicate, "Whether to capture networking metrics");
    AZ_CVAR(AZ::CVarFixedString, cl_metricsFile, "client_network_metrics.json", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "File of the client metrics file if enabled, placed under <ProjectFolder>/user/metrics");
    AZ_CVAR(AZ::CVarFixedString, sv_metricsFile, "server_network_metrics.json", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "File of the server metrics file if enabled, placed under <ProjectFolder>/user/metrics");

    void ConfigureEventLoggerHelper(const AZ::CVarFixedString& filename)
    {
        if (auto eventLoggerFactory = AZ::Interface<AZ::Metrics::IEventLoggerFactory>::Get())
        {
            const AZ::IO::FixedMaxPath metricsFilepath = AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Metrics" / filename;
            constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath;

            auto stream = AZStd::make_unique<AZ::IO::SystemFileStream>(metricsFilepath.c_str(), openMode);
            auto eventLogger = AZStd::make_unique<AZ::Metrics::JsonTraceEventLogger>(AZStd::move(stream));
            eventLoggerFactory->RegisterEventLogger(NetworkingMetricsId, AZStd::move(eventLogger));
        }
    }

    void UnregisterEventLoggerHelper()
    {
        if (auto* eventLoggerFactory = AZ::Interface<AZ::Metrics::IEventLoggerFactory>::Get())
        {
            eventLoggerFactory->UnregisterEventLogger(NetworkingMetricsId);
        }
    }

    void OnEnableNetworkingMetricsChanged(const bool& enabled)
    {
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())
        {
            if (enabled)
            {
                statSystem->Register();
            }
            else
            {
                statSystem->Unregister();
            }
        }
    }

    void MultiplayerStatSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerStatSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void MultiplayerStatSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerStatSystemComponent"));
    }

    void MultiplayerStatSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MultiplayerStatSystemComponent"));
    }

    MultiplayerStatSystemComponent::MultiplayerStatSystemComponent()
    {
        AZ::Interface<IMultiplayerStatSystem>::Register(this);
    }

    MultiplayerStatSystemComponent::~MultiplayerStatSystemComponent()
    {
        AZ::Interface<IMultiplayerStatSystem>::Unregister(this);
    }

    void MultiplayerStatSystemComponent::Activate()
    {
    }

    void MultiplayerStatSystemComponent::Deactivate()
    {
        Unregister();
    }

    void MultiplayerStatSystemComponent::Register()
    {
        UnregisterEventLoggerHelper();

        if (GetMultiplayer())
        {
            switch (GetMultiplayer()->GetAgentType())
            {
            case MultiplayerAgentType::DedicatedServer: // fallthrough
            case MultiplayerAgentType::ClientServer:
                ConfigureEventLoggerHelper(sv_metricsFile);
                break;
            case MultiplayerAgentType::Client:
                ConfigureEventLoggerHelper(cl_metricsFile);
                break;
            default:
                AZLOG_WARN("Unitialized or unsupported agent type for recording metrics.");
                break;
            }
        }
        
        m_metricsEvent.Enqueue(AZ::TimeMs{ 1000 }, true);
    }

    void MultiplayerStatSystemComponent::Unregister()
    {
        m_metricsEvent.RemoveFromQueue();
        if (bg_enableNetworkingMetrics)
        {
            UnregisterEventLoggerHelper();
        }
    }

    void MultiplayerStatSystemComponent::SetReportPeriod(AZ::TimeMs period)
    {
        m_metricsEvent.Requeue(period);
    }

    void MultiplayerStatSystemComponent::DeclareStatGroup(int uniqueGroupId, const char* groupName)
    {
        AZStd::lock_guard lock(m_access);
        m_groups[uniqueGroupId].m_name = groupName;
    }

    void MultiplayerStatSystemComponent::DeclareStatTypeIntU64(int uniqueGroupId, int uniqueStatId, const char* statName)
    {
        AZStd::lock_guard lock(m_access);
        const auto groupIterator = m_groups.find(uniqueGroupId);
        if (groupIterator != m_groups.end())
        {
            groupIterator->second.m_stats[uniqueStatId].m_name = statName;
            m_statToGroupId[uniqueStatId] = &groupIterator->second;
        }
        else
        {
            AZLOG_WARN("Stat group with id %d has not been declared using DECLARE_STAT_GROUP", uniqueGroupId);
        }
    }

    void MultiplayerStatSystemComponent::SetStatTypeIntU64(int uniqueStatId, AZ::u64 value)
    {
        AZStd::lock_guard lock(m_access);
        const auto groupIterator = m_statToGroupId.find(uniqueStatId);
        if (groupIterator != m_statToGroupId.end())
        {
            groupIterator->second->m_stats[uniqueStatId].m_value = value;
        }
        else
        {
            AZLOG_WARN("Stat with id %d has not been declared using DECLARE_STAT_UINT64", uniqueStatId);
        }
    }

    void MultiplayerStatSystemComponent::RecordMetrics()
    {
        if (const auto* eventLoggerFactory = AZ::Interface<AZ::Metrics::IEventLoggerFactory>::Get())
        {
            if (auto* eventLogger = eventLoggerFactory->FindEventLogger(NetworkingMetricsId))
            {
                AZStd::lock_guard lock(m_access);
                for (const AZStd::pair<int, Group>& group : m_groups)
                {
                    AZStd::vector<AZ::Metrics::EventField> argsContainer;
                    for (const auto& stats : group.second.m_stats)
                    {
                        argsContainer.emplace_back(stats.second.m_name, stats.second.m_value);
                    }

                    AZ::Metrics::CounterArgs counterArgs;
                    counterArgs.m_name = "Stats";
                    counterArgs.m_cat = group.second.m_name;
                    counterArgs.m_args = argsContainer;

                    eventLogger->RecordCounterEvent(counterArgs);
                }
            }
        }
    }
} // namespace Multiplayer
