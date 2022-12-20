/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Metrics/IEventLoggerFactory.h>
#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/Utils/Utils.h>
#include <Multiplayer/IMultiplayer.h>
#include <Source/MultiplayerStatSystemComponent.h>

namespace Multiplayer
{
    // Metrics cvars
    void OnEnableNetworkingMetricsChanged(const bool& enabled);
    AZ_CVAR(
        bool,
        bg_enableNetworkingMetrics,
        true,
        &OnEnableNetworkingMetricsChanged,
        AZ::ConsoleFunctorFlags::DontReplicate,
        "Whether to capture networking metrics");
    AZ_CVAR(
        AZ::TimeMs,
        bg_networkingMetricCollectionPeriod,
        AZ::TimeMs{ 1000 },
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate,
        "How often to capture metrics by default.");
    AZ_CVAR(
        AZ::CVarFixedString,
        cl_metricsFile,
        "client_network_metrics.json",
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate,
        "File of the client metrics file if enabled, placed under <ProjectFolder>/user/metrics");
    AZ_CVAR(
        AZ::CVarFixedString,
        sv_metricsFile,
        "server_network_metrics.json",
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate,
        "File of the server metrics file if enabled, placed under <ProjectFolder>/user/metrics");

    void ConfigureEventLoggerHelper(const AZ::CVarFixedString& filename)
    {
        if (auto eventLoggerFactory = AZ::Interface<AZ::Metrics::IEventLoggerFactory>::Get())
        {
            const AZ::IO::FixedMaxPath metricsFilepath = AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Metrics" / filename;
            constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath;

            auto stream = AZStd::make_unique<AZ::IO::SystemFileStream>(metricsFilepath.c_str(), openMode);
            AZ::Metrics::JsonTraceEventLoggerConfig config{ "Multiplayer" };
            auto eventLogger = AZStd::make_unique<AZ::Metrics::JsonTraceEventLogger>(AZStd::move(stream), config);
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
            case MultiplayerAgentType::Uninitialized:
                AZLOG_WARN("Unitialized agent type isn't supported for networking metrics.");
                break;
            }
        }

        m_metricsEvent.Enqueue(bg_networkingMetricCollectionPeriod, true);
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
        StatGroup* newGroup = m_statGroups.AddNew(uniqueGroupId);
        newGroup->m_name = groupName;
    }

    void MultiplayerStatSystemComponent::DeclareStat(int uniqueGroupId, int uniqueStatId, const char* statName)
    {
        AZStd::lock_guard lock(m_access);
        if (StatGroup* group = m_statGroups.Find(uniqueGroupId))
        {
            auto* newStat = group->m_stats.AddNew(uniqueStatId);
            newStat->m_name = statName;

            const auto statIterator = m_statIdToGroupId.find(uniqueStatId);
            if (statIterator == m_statIdToGroupId.end())
            {
                m_statIdToGroupId.insert(AZStd::make_pair(uniqueStatId, uniqueGroupId));
            }
            else
            {
                AZLOG_WARN("A stat has already been declared using DECLARE_PERFORMANCE_STAT with id %d", uniqueStatId);
            }
        }
        else
        {
            AZLOG_WARN("Stat group with id %d has not been declared using DECLARE_PERFORMANCE_STAT_GROUP", uniqueGroupId);
        }
    }

    void MultiplayerStatSystemComponent::SetStat(int uniqueStatId, double value)
    {
        AZStd::lock_guard lock(m_access);
        const auto statIterator = m_statIdToGroupId.find(uniqueStatId);
        if (statIterator != m_statIdToGroupId.end())
        {
            if (const auto group = m_statGroups.Find(statIterator->second))
            {
                if (CumulativeAverage* stat = group->m_stats.Find(uniqueStatId))
                {
                    stat->m_lastValue = value;
                    stat->m_average.PushEntry(value);
                    return;
                }
            }
        }

        AZLOG_WARN("Stat with id %d has not been declared using DECLARE_PERFORMANCE_STAT", uniqueStatId);
    }

    void MultiplayerStatSystemComponent::IncrementStat(int uniqueStatId)
    {
        AZStd::lock_guard lock(m_access);
        const auto statIterator = m_statIdToGroupId.find(uniqueStatId);
        if (statIterator != m_statIdToGroupId.end())
        {
            if (const auto group = m_statGroups.Find(statIterator->second))
            {
                if (CumulativeAverage* stat = group->m_stats.Find(uniqueStatId))
                {
                    stat->m_counterValue++;
                    return;
                }
            }
        }

        AZLOG_WARN("Stat with id %d has not been declared using DECLARE_PERFORMANCE_STAT", uniqueStatId);
    }

    void MultiplayerStatSystemComponent::RecordMetrics()
    {
        if (const auto* eventLoggerFactory = AZ::Interface<AZ::Metrics::IEventLoggerFactory>::Get())
        {
            if (auto* eventLogger = eventLoggerFactory->FindEventLogger(NetworkingMetricsId))
            {
                AZStd::lock_guard lock(m_access);
                for (StatGroup& group : m_statGroups.m_items)
                {
                    AZStd::vector<AZ::Metrics::EventField> argsContainer;

                    for (auto& stat : group.m_stats.m_items)
                    {
                        if (stat.m_average.GetNumRecorded() > 0)
                        {
                            // If there are new entries, update the average.
                            argsContainer.emplace_back(stat.m_name.c_str(), stat.m_average.CalculateAverage());
                        }
                        else if (stat.m_counterValue > 0)
                        {
                            // counter metric
                            argsContainer.emplace_back(stat.m_name.c_str(), stat.m_counterValue);
                            stat.m_counterValue = 0;
                            stat.m_lastValue = 0;
                        }
                        else
                        {
                            // If there were no entries within the last collection period, report the last value received.
                            argsContainer.emplace_back(stat.m_name.c_str(), stat.m_lastValue);
                        }

                        // Reset average in order to measure average over the save period.
                        stat.m_average = CumulativeAverage::AverageWindowType{};
                    }

                    AZ::Metrics::CounterArgs counterArgs;
                    counterArgs.m_name = "Stats";
                    counterArgs.m_cat = group.m_name;
                    counterArgs.m_args = argsContainer;

                    eventLogger->RecordCounterEvent(counterArgs);
                }
            }
        }
    }
} // namespace Multiplayer
