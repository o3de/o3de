/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        AZ::TimeMs m_totalHistoryTimeMs = AZ::Time::ZeroTimeMs;

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
        void RecordEntitySerializeStart(AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName);
        void RecordComponentSerializeEnd(AzNetworking::SerializerMode mode, NetComponentId netComponentId);
        void RecordEntitySerializeStop(AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName);
        void RecordPropertySent(NetComponentId netComponentId, PropertyIndex propertyId, uint32_t totalBytes);
        void RecordPropertyReceived(NetComponentId netComponentId, PropertyIndex propertyId, uint32_t totalBytes);
        void RecordRpcSent(AZ::EntityId entityId, const char* entityName, NetComponentId netComponentId, RpcIndex rpcId, uint32_t totalBytes);
        void RecordRpcReceived(AZ::EntityId entityId, const char* entityName, NetComponentId netComponentId, RpcIndex rpcId, uint32_t totalBytes);
        void TickStats(AZ::TimeMs metricFrameTimeMs);

        Metric CalculateComponentPropertyUpdateSentMetrics(NetComponentId netComponentId) const;
        Metric CalculateComponentPropertyUpdateRecvMetrics(NetComponentId netComponentId) const;
        Metric CalculateComponentRpcsSentMetrics(NetComponentId netComponentId) const;
        Metric CalculateComponentRpcsRecvMetrics(NetComponentId netComponentId) const;
        Metric CalculateTotalPropertyUpdateSentMetrics() const;
        Metric CalculateTotalPropertyUpdateRecvMetrics() const;
        Metric CalculateTotalRpcsSentMetrics() const;
        Metric CalculateTotalRpcsRecvMetrics() const;

        struct Events
        {
            AZ::Event<AzNetworking::SerializerMode, AZ::EntityId, const char*> m_entitySerializeStart;
            AZ::Event<AzNetworking::SerializerMode, NetComponentId> m_componentSerializeEnd;
            AZ::Event<AzNetworking::SerializerMode, AZ::EntityId, const char*> m_entitySerializeStop;
            AZ::Event<NetComponentId, PropertyIndex, uint32_t> m_propertySent;
            AZ::Event<NetComponentId, PropertyIndex, uint32_t> m_propertyReceived;
            AZ::Event<AZ::EntityId, const char*, NetComponentId, RpcIndex, uint32_t> m_rpcSent;
            AZ::Event<AZ::EntityId, const char*, NetComponentId, RpcIndex, uint32_t> m_rpcReceived;
        };

        Events m_events;

        struct EventHandlers
        {
            AZ::Event<AzNetworking::SerializerMode, AZ::EntityId, const char*>::Handler m_entitySerializeStart;
            AZ::Event<AzNetworking::SerializerMode, NetComponentId>::Handler m_componentSerializeEnd;
            AZ::Event<AzNetworking::SerializerMode, AZ::EntityId, const char*>::Handler m_entitySerializeStop;
            AZ::Event<NetComponentId, PropertyIndex, uint32_t>::Handler m_propertySent;
            AZ::Event<NetComponentId, PropertyIndex, uint32_t>::Handler m_propertyReceived;
            AZ::Event<AZ::EntityId, const char*, NetComponentId, RpcIndex, uint32_t>::Handler m_rpcSent;
            AZ::Event<AZ::EntityId, const char*, NetComponentId, RpcIndex, uint32_t>::Handler m_rpcReceived;
        };

        void ConnectHandlers(EventHandlers& handlers);
    };
}
