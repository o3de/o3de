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

#pragma once

#include <Source/ReplicationWindows/IReplicationWindow.h>
#include <Source/NetworkEntity/NetworkEntityHandle.h>
#include <Source/NetworkEntity/INetworkEntityManager.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace Multiplayer
{
    class NetSystemComponent;

    class ServerToClientReplicationWindow
        : public IReplicationWindow
        , public AZ::EntitySystemBus::Handler
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

        ServerToClientReplicationWindow(NetworkEntityHandle controlledEntity, const AzNetworking::IConnection* connection);

        //! IReplicationWindow interface
        //! @{
        bool ReplicationSetUpdateReady() override;
        const ReplicationSet& GetReplicationSet() const override;
        uint32_t GetMaxEntityReplicatorSendCount() const override;
        bool IsInWindow(const ConstNetworkEntityHandle& entityPtr, NetEntityRole& outNetworkRole) const override;
        void UpdateWindow() override;
        void DebugDraw() const override;
        //! @}

    private:
        //! EntitySystemBus interface
        //! @{
        void OnEntityActivated(const AZ::EntityId&) override;
        void OnEntityDeactivated(const AZ::EntityId&) override;
        //! @}

        //void CollectControlledEntitiesRecursive(ReplicationSet& replicationSet, EntityHierarchyComponent::Authority& hierarchyController);
        //void OnAddFilteredEntity(NetEntityId filteredEntityId);

        void EvaluateConnection();
        void AddEntityToReplicationSet(ConstNetworkEntityHandle& entityHandle, float priority, float distanceSquared);

        ServerToClientReplicationWindow& operator=(const ServerToClientReplicationWindow&) = delete;

        // sorted in reverse, lowest priority is the top()
        ReplicationCandidateQueue m_candidateQueue;
        ReplicationSet m_replicationSet;

        AZ::ScheduledEvent m_updateWindowEvent;

        NetworkEntityHandle m_controlledEntity;
        AZ::TransformInterface* m_controlledEntityTransform = nullptr;

        //FilteredEntityComponent::Authority* m_controlledFilteredEntityComponent = nullptr;
        //NetBindComponent* m_controlledNetBindComponent = nullptr;

        const AzNetworking::IConnection* m_connection = nullptr;
        float m_minPriorityReplicated = 0.0f; ///< Lowest replicated entity priority in last update

        // Cached values to detect a poor network connection
        uint32_t m_lastCheckedSentPackets = 0;
        uint32_t m_lastCheckedLostPackets = 0;
        bool     m_isPoorConnection = true;
    };
}
