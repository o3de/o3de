/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/ITime.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>
#include <Multiplayer/MultiplayerTypes.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    struct MultiplayerStats
    {
        uint64_t m_entityCount = 0;
        uint64_t m_clientConnectionCount = 0;
        uint64_t m_serverConnectionCount = 0;

        uint64_t m_recordMetricIndex = 0;
        AZ::TimeMs m_totalHistoryTimeMs = AZ::TimeMs{ 0 };

        static const uint32_t RingbufferSamples = 32;
        using MetricRingbuffer = AZStd::array<uint64_t, RingbufferSamples>;
        struct Metric
        {
            Metric();
            uint64_t m_totalCalls = 0;
            uint64_t m_totalBytes = 0;
            MetricRingbuffer m_callHistory;
            MetricRingbuffer m_byteHistory;
        };

        struct ComponentStats
        {
            AZStd::vector<Metric> m_propertyUpdatesSent;
            AZStd::vector<Metric> m_propertyUpdatesRecv;
            AZStd::vector<Metric> m_rpcsSent;
            AZStd::vector<Metric> m_rpcsRecv;
        };
        AZStd::vector<ComponentStats> m_componentStats;

        void ReserveComponentStats(NetComponentId netComponentId, uint16_t propertyCount, uint16_t rpcCount);
        void RecordPropertySent(NetComponentId netComponentId, PropertyIndex propertyId, uint32_t totalBytes);
        void RecordPropertyReceived(NetComponentId netComponentId, PropertyIndex propertyId, uint32_t totalBytes);
        void RecordRpcSent(NetComponentId netComponentId, RpcIndex rpcId, uint32_t totalBytes);
        void RecordRpcReceived(NetComponentId netComponentId, RpcIndex rpcId, uint32_t totalBytes);
        void TickStats(AZ::TimeMs metricFrameTimeMs);

        Metric CalculateComponentPropertyUpdateSentMetrics(NetComponentId netComponentId) const;
        Metric CalculateComponentPropertyUpdateRecvMetrics(NetComponentId netComponentId) const;
        Metric CalculateComponentRpcsSentMetrics(NetComponentId netComponentId) const;
        Metric CalculateComponentRpcsRecvMetrics(NetComponentId netComponentId) const;
        Metric CalculateTotalPropertyUpdateSentMetrics() const;
        Metric CalculateTotalPropertyUpdateRecvMetrics() const;
        Metric CalculateTotalRpcsSentMetrics() const;
        Metric CalculateTotalRpcsRecvMetrics() const;
    };
}
