/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/MultiplayerMetrics.h>
#include <Multiplayer/MultiplayerPerformanceStats.h>
#include <Multiplayer/MultiplayerStats.h>

namespace Multiplayer
{
    MultiplayerStats::Metric::Metric()
    {
        AZStd::uninitialized_fill_n(m_callHistory.data(), RingbufferSamples, 0);
        AZStd::uninitialized_fill_n(m_byteHistory.data(), RingbufferSamples, 0);
    }

    void MultiplayerStats::ReserveComponentStats(NetComponentId netComponentId, uint16_t propertyCount, uint16_t rpcCount)
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        if (m_componentStats.size() <= netComponentIndex)
        {
            m_componentStats.resize(netComponentIndex + 1);
        }
        m_componentStats[netComponentIndex].m_propertyUpdatesSent.resize(propertyCount);
        m_componentStats[netComponentIndex].m_propertyUpdatesRecv.resize(propertyCount);
        m_componentStats[netComponentIndex].m_rpcsSent.resize(rpcCount);
        m_componentStats[netComponentIndex].m_rpcsRecv.resize(rpcCount);
    }

    void MultiplayerStats::RecordEntitySerializeStart(AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName)
    {
        m_events.m_entitySerializeStart.Signal(mode, entityId, entityName);
    }

    void MultiplayerStats::RecordComponentSerializeEnd(AzNetworking::SerializerMode mode, NetComponentId netComponentId)
    {
        m_events.m_componentSerializeEnd.Signal(mode, netComponentId);
    }

    void MultiplayerStats::RecordEntitySerializeStop(AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName)
    {
        m_events.m_entitySerializeStop.Signal(mode, entityId, entityName);
    }

    void MultiplayerStats::RecordPropertySent(NetComponentId netComponentId, PropertyIndex propertyId, uint32_t totalBytes)
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        const uint16_t propertyIndex = aznumeric_cast<uint16_t>(propertyId);
        if (m_componentStats[netComponentIndex].m_propertyUpdatesSent.size() > propertyIndex)
        {
            m_componentStats[netComponentIndex].m_propertyUpdatesSent[propertyIndex].m_totalCalls++;
            m_componentStats[netComponentIndex].m_propertyUpdatesSent[propertyIndex].m_totalBytes += totalBytes;
            m_componentStats[netComponentIndex].m_propertyUpdatesSent[propertyIndex].m_callHistory[m_recordMetricIndex]++;
            m_componentStats[netComponentIndex].m_propertyUpdatesSent[propertyIndex].m_byteHistory[m_recordMetricIndex] += totalBytes;
        }
        else
        {
            AZ_Warning("MultiplayerStats", false,
                "Component ID %u has fewer than %u sent propertyIndex. Mismatch by caller suspected.", netComponentIndex, propertyIndex);
        }

        m_events.m_propertySent.Signal(netComponentId, propertyId, totalBytes);
    }

    void MultiplayerStats::RecordPropertyReceived(NetComponentId netComponentId, PropertyIndex propertyId, uint32_t totalBytes)
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        const uint16_t propertyIndex = aznumeric_cast<uint16_t>(propertyId);
        if (m_componentStats[netComponentIndex].m_propertyUpdatesRecv.size() > propertyIndex)
        {
            m_componentStats[netComponentIndex].m_propertyUpdatesRecv[propertyIndex].m_totalCalls++;
            m_componentStats[netComponentIndex].m_propertyUpdatesRecv[propertyIndex].m_totalBytes += totalBytes;
            m_componentStats[netComponentIndex].m_propertyUpdatesRecv[propertyIndex].m_callHistory[m_recordMetricIndex]++;
            m_componentStats[netComponentIndex].m_propertyUpdatesRecv[propertyIndex].m_byteHistory[m_recordMetricIndex] += totalBytes;
        }
        else
        {
            AZ_Warning("MultiplayerStats", false,
                "Component ID %u has fewer than %u receive propertyIndex. Mismatch by caller suspected.", netComponentIndex, propertyIndex);
        }

        m_events.m_propertyReceived.Signal(netComponentId, propertyId, totalBytes);
    }

    void MultiplayerStats::RecordRpcSent(AZ::EntityId entityId, const char* entityName, NetComponentId netComponentId, RpcIndex rpcId, uint32_t totalBytes)
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        const uint16_t rpcIndex = aznumeric_cast<uint16_t>(rpcId);

        if (m_componentStats[netComponentIndex].m_rpcsSent.size() > rpcIndex)
        {
            m_componentStats[netComponentIndex].m_rpcsSent[rpcIndex].m_totalCalls++;
            m_componentStats[netComponentIndex].m_rpcsSent[rpcIndex].m_totalBytes += totalBytes;
            m_componentStats[netComponentIndex].m_rpcsSent[rpcIndex].m_callHistory[m_recordMetricIndex]++;
            m_componentStats[netComponentIndex].m_rpcsSent[rpcIndex].m_byteHistory[m_recordMetricIndex] += totalBytes;
        }
        else
        {
            AZ_Warning("MultiplayerStats", false,
                "Component ID %u has fewer than %u sent rpcIndex. Mismatch by caller suspected.", netComponentIndex, rpcIndex);
        }

        m_events.m_rpcSent.Signal(entityId, entityName, netComponentId, rpcId, totalBytes);
    }

    void MultiplayerStats::RecordRpcReceived(AZ::EntityId entityId, const char* entityName, NetComponentId netComponentId, RpcIndex rpcId, uint32_t totalBytes)
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        const uint16_t rpcIndex = aznumeric_cast<uint16_t>(rpcId);
        if (m_componentStats[netComponentIndex].m_rpcsRecv.size() > rpcIndex)
        {
            m_componentStats[netComponentIndex].m_rpcsRecv[rpcIndex].m_totalCalls++;
            m_componentStats[netComponentIndex].m_rpcsRecv[rpcIndex].m_totalBytes += totalBytes;
            m_componentStats[netComponentIndex].m_rpcsRecv[rpcIndex].m_callHistory[m_recordMetricIndex]++;
            m_componentStats[netComponentIndex].m_rpcsRecv[rpcIndex].m_byteHistory[m_recordMetricIndex] += totalBytes;
        }
        else
        {
            AZ_Warning("MultiplayerStats", false,
                "Component ID %u has fewer than %u receive rpcIndex. Mismatch by caller suspected.", netComponentIndex, rpcIndex);
        }

        m_events.m_rpcReceived.Signal(entityId, entityName, netComponentId, rpcId, totalBytes);
    }

    void MultiplayerStats::TickStats(AZ::TimeMs metricFrameTimeMs)
    {
        SET_PERFORMANCE_STAT(MultiplayerStat_EntityCount, m_entityCount);
        SET_PERFORMANCE_STAT(MultiplayerStat_ClientConnectionCount, m_clientConnectionCount);

        m_totalHistoryTimeMs = metricFrameTimeMs * static_cast<AZ::TimeMs>(RingbufferSamples);
        m_recordMetricIndex = ++m_recordMetricIndex % RingbufferSamples;
        for (ComponentStats& componentStats : m_componentStats)
        {
            for (Metric& metric : componentStats.m_propertyUpdatesSent)
            {
                metric.m_callHistory[m_recordMetricIndex] = 0;
                metric.m_byteHistory[m_recordMetricIndex] = 0;
            }
            for (Metric& metric : componentStats.m_propertyUpdatesRecv)
            {
                metric.m_callHistory[m_recordMetricIndex] = 0;
                metric.m_byteHistory[m_recordMetricIndex] = 0;
            }
            for (Metric& metric : componentStats.m_rpcsSent)
            {
                metric.m_callHistory[m_recordMetricIndex] = 0;
                metric.m_byteHistory[m_recordMetricIndex] = 0;
            }
            for (Metric& metric : componentStats.m_rpcsRecv)
            {
                metric.m_callHistory[m_recordMetricIndex] = 0;
                metric.m_byteHistory[m_recordMetricIndex] = 0;
            }
        }
    }

    static void CombineMetrics(MultiplayerStats::Metric& outArg1, const MultiplayerStats::Metric& arg2)
    {
        outArg1.m_totalCalls += arg2.m_totalCalls;
        outArg1.m_totalBytes += arg2.m_totalBytes;
        for (uint32_t index = 0; index < MultiplayerStats::RingbufferSamples; ++index)
        {
            outArg1.m_callHistory[index] += arg2.m_callHistory[index];
            outArg1.m_byteHistory[index] += arg2.m_byteHistory[index];
        }
    }

    static MultiplayerStats::Metric SumMetricVector(const AZStd::vector<MultiplayerStats::Metric>& metricVector)
    {
        MultiplayerStats::Metric result;
        for (AZStd::size_t index = 0; index < metricVector.size(); ++index)
        {
            CombineMetrics(result, metricVector[index]);
        }
        return result;
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateComponentPropertyUpdateSentMetrics(NetComponentId netComponentId) const
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        return SumMetricVector(m_componentStats[netComponentIndex].m_propertyUpdatesSent);
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateComponentPropertyUpdateRecvMetrics(NetComponentId netComponentId) const
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        return SumMetricVector(m_componentStats[netComponentIndex].m_propertyUpdatesRecv);
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateComponentRpcsSentMetrics(NetComponentId netComponentId) const
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        return SumMetricVector(m_componentStats[netComponentIndex].m_rpcsSent);
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateComponentRpcsRecvMetrics(NetComponentId netComponentId) const
    {
        const uint16_t netComponentIndex = aznumeric_cast<uint16_t>(netComponentId);
        return SumMetricVector(m_componentStats[netComponentIndex].m_rpcsRecv);
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateTotalPropertyUpdateSentMetrics() const
    {
        Metric result;
        for (AZStd::size_t index = 0; index < m_componentStats.size(); ++index)
        {
            const NetComponentId netComponentId = aznumeric_cast<NetComponentId>(index);
            CombineMetrics(result, CalculateComponentPropertyUpdateSentMetrics(netComponentId));
        }
        return result;
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateTotalPropertyUpdateRecvMetrics() const
    {
        Metric result;
        for (AZStd::size_t index = 0; index < m_componentStats.size(); ++index)
        {
            const NetComponentId netComponentId = aznumeric_cast<NetComponentId>(index);
            CombineMetrics(result, CalculateComponentPropertyUpdateRecvMetrics(netComponentId));
        }
        return result;
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateTotalRpcsSentMetrics() const
    {
        Metric result;
        for (AZStd::size_t index = 0; index < m_componentStats.size(); ++index)
        {
            const NetComponentId netComponentId = aznumeric_cast<NetComponentId>(index);
            CombineMetrics(result, CalculateComponentRpcsSentMetrics(netComponentId));
        }
        return result;
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateTotalRpcsRecvMetrics() const
    {
        Metric result;
        for (AZStd::size_t index = 0; index < m_componentStats.size(); ++index)
        {
            const NetComponentId netComponentId = aznumeric_cast<NetComponentId>(index);
            CombineMetrics(result, CalculateComponentRpcsRecvMetrics(netComponentId));
        }
        return result;
    }

    void MultiplayerStats::ConnectHandlers(EventHandlers& handlers)
    {
        handlers.m_entitySerializeStart.Connect(m_events.m_entitySerializeStart);
        handlers.m_componentSerializeEnd.Connect(m_events.m_componentSerializeEnd);
        handlers.m_entitySerializeStop.Connect(m_events.m_entitySerializeStop);
        handlers.m_propertySent.Connect(m_events.m_propertySent);
        handlers.m_propertyReceived.Connect(m_events.m_propertyReceived);
        handlers.m_rpcSent.Connect(m_events.m_rpcSent);
        handlers.m_rpcReceived.Connect(m_events.m_rpcReceived);
    }

    void MultiplayerStats::RecordFrameTime(AZ::TimeUs networkFrameTime)
    {
        SET_PERFORMANCE_STAT(MultiplayerStat_FrameTimeUs, networkFrameTime);
    }
} // namespace Multiplayer
