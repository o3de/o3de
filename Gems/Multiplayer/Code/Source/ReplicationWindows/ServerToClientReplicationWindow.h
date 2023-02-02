/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <Multiplayer/ReplicationWindows/IReplicationWindow.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace Multiplayer
{
    class NetSystemComponent;
    class NetworkHierarchyRootComponent;

    class ServerToClientReplicationWindow
        : public IReplicationWindow
    {
    public:

        struct PrioritizedReplicationCandidate
        {
            PrioritizedReplicationCandidate() = default;
            PrioritizedReplicationCandidate(const ConstNetworkEntityHandle& entityHandle, float priority);
            bool operator <(const PrioritizedReplicationCandidate& rhs) const;
            bool operator >(const PrioritizedReplicationCandidate& rhs) const;
            ConstNetworkEntityHandle m_entityHandle;
            float m_priority;
        };
        // we sort lowest priority first, so that we can easily keep the biggest N priorities
        using ReplicationCandidateQueue = AZStd::priority_queue<PrioritizedReplicationCandidate>;

        ServerToClientReplicationWindow(NetworkEntityHandle controlledEntity, AzNetworking::IConnection* connection);

        //! IReplicationWindow interface
        //! @{
        bool ReplicationSetUpdateReady() override;
        const ReplicationSet& GetReplicationSet() const override;
        uint32_t GetMaxProxyEntityReplicatorSendCount() const override;
        bool IsInWindow(const ConstNetworkEntityHandle& entityPtr, NetEntityRole& outNetworkRole) const override;
        bool AddEntity(AZ::Entity* entity) override;
        void RemoveEntity(AZ::Entity* entity) override;
        void UpdateWindow() override;
        AzNetworking::PacketId SendEntityUpdateMessages(NetworkEntityUpdateVector& entityUpdateVector) override;
        void SendEntityRpcs(NetworkEntityRpcVector& entityRpcVector, bool reliable) override;
        void SendEntityResets(const NetEntityIdSet& resetIds) override;
        void DebugDraw() const override;
        //! @}

    private:

        void UpdateHierarchyReplicationSet(ReplicationSet& replicationSet, NetworkHierarchyRootComponent& hierarchyComponent);

        void EvaluateConnection();
        void AddEntityToReplicationSet(ConstNetworkEntityHandle& entityHandle, float priority, float distanceSquared);

        ServerToClientReplicationWindow& operator=(const ServerToClientReplicationWindow&) = delete;

        // sorted in reverse, lowest priority is the top()
        ReplicationCandidateQueue m_candidateQueue;
        ReplicationSet m_replicationSet;

        NetworkEntityHandle m_controlledEntity;
        AZ::TransformInterface* m_controlledEntityTransform = nullptr;

        AZ::EntityActivatedEvent::Handler m_entityActivatedEventHandler;
        AZ::EntityDeactivatedEvent::Handler m_entityDeactivatedEventHandler;

        AzNetworking::IConnection* m_connection = nullptr;

        // Cached values to detect a poor network connection
        uint32_t m_lastCheckedSentPackets = 0;
        uint32_t m_lastCheckedLostPackets = 0;
        bool     m_isPoorConnection = true;
    };
}
