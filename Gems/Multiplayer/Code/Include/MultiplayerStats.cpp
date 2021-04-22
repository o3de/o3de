/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Include/MultiplayerStats.h>

namespace Multiplayer
{
    void MultiplayerStats::ReserveComponentStats(uint16_t netComponentId, uint16_t propertyCount, uint16_t rpcCount)
    {
        if (m_componentStats.size() <= netComponentId)
        {
            m_componentStats.resize(netComponentId + 1);
        }
        m_componentStats[netComponentId].m_propertyUpdatesSent.resize(propertyCount);
        m_componentStats[netComponentId].m_propertyUpdatesRecv.resize(propertyCount);
        m_componentStats[netComponentId].m_rpcsSent.resize(rpcCount);
        m_componentStats[netComponentId].m_rpcsRecv.resize(rpcCount);
    }

    void MultiplayerStats::RecordPropertySent(uint16_t netComponentId, uint16_t propertyId, uint32_t totalBytes)
    {
        m_componentStats[netComponentId].m_propertyUpdatesSent[propertyId].m_totalCalls++;
        m_componentStats[netComponentId].m_propertyUpdatesSent[propertyId].m_totalBytes += totalBytes;
        m_componentStats[netComponentId].m_propertyUpdatesSent[propertyId].m_callHistory[m_recordMetricIndex]++;
        m_componentStats[netComponentId].m_propertyUpdatesSent[propertyId].m_byteHistory[m_recordMetricIndex] += totalBytes;
    }

    void MultiplayerStats::RecordPropertyReceived(uint16_t netComponentId, uint16_t propertyId, uint32_t totalBytes)
    {
        m_componentStats[netComponentId].m_propertyUpdatesRecv[propertyId].m_totalCalls++;
        m_componentStats[netComponentId].m_propertyUpdatesRecv[propertyId].m_totalBytes += totalBytes;
        m_componentStats[netComponentId].m_propertyUpdatesRecv[propertyId].m_callHistory[m_recordMetricIndex]++;
        m_componentStats[netComponentId].m_propertyUpdatesRecv[propertyId].m_byteHistory[m_recordMetricIndex] += totalBytes;
    }

    void MultiplayerStats::RecordRpcSent(uint16_t netComponentId, uint16_t rpcId, uint32_t totalBytes)
    {
        m_componentStats[netComponentId].m_rpcsSent[rpcId].m_totalCalls++;
        m_componentStats[netComponentId].m_rpcsSent[rpcId].m_totalBytes += totalBytes;
        m_componentStats[netComponentId].m_rpcsSent[rpcId].m_callHistory[m_recordMetricIndex]++;
        m_componentStats[netComponentId].m_rpcsSent[rpcId].m_byteHistory[m_recordMetricIndex] += totalBytes;
    }

    void MultiplayerStats::RecordRpcReceived(uint16_t netComponentId, uint16_t rpcId, uint32_t totalBytes)
    {
        m_componentStats[netComponentId].m_rpcsRecv[rpcId].m_totalCalls++;
        m_componentStats[netComponentId].m_rpcsRecv[rpcId].m_totalBytes += totalBytes;
        m_componentStats[netComponentId].m_rpcsRecv[rpcId].m_callHistory[m_recordMetricIndex]++;
        m_componentStats[netComponentId].m_rpcsRecv[rpcId].m_byteHistory[m_recordMetricIndex] += totalBytes;
    }

    void MultiplayerStats::TickStats(AZ::TimeMs metricFrameTimeMs)
    {
        m_totalHistoryTimeMs = metricFrameTimeMs * static_cast<AZ::TimeMs>(RingbufferSamples);
        m_recordMetricIndex = ++m_recordMetricIndex % RingbufferSamples;
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

    MultiplayerStats::Metric MultiplayerStats::CalculateComponentPropertyUpdateSentMetrics(uint16_t netComponentId) const
    {
        return SumMetricVector(m_componentStats[netComponentId].m_propertyUpdatesSent);
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateComponentPropertyUpdateRecvMetrics(uint16_t netComponentId) const
    {
        return SumMetricVector(m_componentStats[netComponentId].m_propertyUpdatesRecv);
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateComponentRpcsSentMetrics(uint16_t netComponentId) const
    {
        return SumMetricVector(m_componentStats[netComponentId].m_rpcsSent);
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateComponentRpcsRecvMetrics(uint16_t netComponentId) const
    {
        return SumMetricVector(m_componentStats[netComponentId].m_rpcsRecv);
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateTotalPropertyUpdateSentMetrics() const
    {
        Metric result;
        for (AZStd::size_t index = 0; index < m_componentStats.size(); ++index)
        {
            CombineMetrics(result, CalculateComponentPropertyUpdateSentMetrics(index));
        }
        return result;
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateTotalPropertyUpdateRecvMetrics() const
    {
        Metric result;
        for (AZStd::size_t index = 0; index < m_componentStats.size(); ++index)
        {
            CombineMetrics(result, CalculateComponentPropertyUpdateRecvMetrics(index));
        }
        return result;
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateTotalRpcsSentMetrics() const
    {
        Metric result;
        for (AZStd::size_t index = 0; index < m_componentStats.size(); ++index)
        {
            CombineMetrics(result, CalculateComponentRpcsSentMetrics(index));
        }
        return result;
    }

    MultiplayerStats::Metric MultiplayerStats::CalculateTotalRpcsRecvMetrics() const
    {
        Metric result;
        for (AZStd::size_t index = 0; index < m_componentStats.size(); ++index)
        {
            CombineMetrics(result, CalculateComponentRpcsRecvMetrics(index));
        }
        return result;
    }
}
