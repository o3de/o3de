/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ReplicationWindows/ServerToClientReplicationWindow.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/std/sort.h>

namespace Multiplayer
{
    AZ_CVAR(bool, sv_ReplicateServerProxies, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Enable sending of ServerProxy entities to clients");
    AZ_CVAR(uint32_t, sv_MaxEntitiesToTrackReplication, 512, nullptr, AZ::ConsoleFunctorFlags::Null, "The default max number of entities to track for replication");
    AZ_CVAR(uint32_t, sv_MinEntitiesToReplicate, 128, nullptr, AZ::ConsoleFunctorFlags::Null, "The default min number of entities to replicate to a client connection");
    AZ_CVAR(uint32_t, sv_MaxEntitiesToReplicate, 256, nullptr, AZ::ConsoleFunctorFlags::Null, "The default max number of entities to replicate to a client connection");
    AZ_CVAR(uint32_t, sv_PacketsToIntegrateQos, 1000, nullptr, AZ::ConsoleFunctorFlags::Null, "The number of packets to accumulate before updating connection quality of service metrics");
    AZ_CVAR(float, sv_BadConnectionThreshold, 0.25f, nullptr, AZ::ConsoleFunctorFlags::Null, "The loss percentage beyond which we consider our network bad");
    AZ_CVAR(AZ::TimeMs, sv_ClientReplicationWindowUpdateMs, AZ::TimeMs{ 300 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Rate for replication window updates.");
    AZ_CVAR(float, sv_ClientAwarenessRadius, 500.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "The maximum distance entities can be from the client and still be relevant");

    const char* GetConnectionStateString(bool isPoor)
    {
        return isPoor ? "poor" : "ideal";
    }

    ServerToClientReplicationWindow::PrioritizedReplicationCandidate::PrioritizedReplicationCandidate
    (
        const ConstNetworkEntityHandle& entityHandle,
        float priority
    )
        : m_entityHandle(entityHandle)
        , m_priority(priority)
    {
        ;
    }

    bool ServerToClientReplicationWindow::PrioritizedReplicationCandidate::operator <(const PrioritizedReplicationCandidate& rhs) const
    {
        return m_priority > rhs.m_priority; // sort lowest priority front
    }

    bool ServerToClientReplicationWindow::PrioritizedReplicationCandidate::operator >(const PrioritizedReplicationCandidate& rhs) const
    {
        return m_priority < rhs.m_priority;
    }

    ServerToClientReplicationWindow::ServerToClientReplicationWindow(NetworkEntityHandle controlledEntity, const AzNetworking::IConnection* connection)
        : m_controlledEntity(controlledEntity)
        , m_entityActivatedEventHandler([this](AZ::Entity* entity) { OnEntityActivated(entity); })
        , m_entityDeactivatedEventHandler([this](AZ::Entity* entity) { OnEntityDeactivated(entity); })
        , m_connection(connection)
        , m_lastCheckedSentPackets(connection->GetMetrics().m_packetsSent)
        , m_lastCheckedLostPackets(connection->GetMetrics().m_packetsLost)
        , m_updateWindowEvent([this]() { UpdateWindow(); }, AZ::Name("Server to client replication window update event"))
    {
        AZ::Entity* entity = m_controlledEntity.GetEntity();
        AZ_Assert(entity, "Invalid controlled entity provided to replication window");
        m_controlledEntityTransform = entity ? entity->GetTransform() : nullptr;
        AZ_Assert(m_controlledEntityTransform, "Controlled player entity must have a transform");

        //// this one is optional
        //mp_ControlledFilteredEntityComponent = m_controlledEntity->FindController<FilteredEntityComponent::Authority>();
        //if (mp_ControlledFilteredEntityComponent)
        //{
        //    mp_ControlledFilteredEntityComponent->AddFilteredEntityEventHandle(m_FilteredEntityAddedEventHandle);
        //}

        m_updateWindowEvent.Enqueue(sv_ClientReplicationWindowUpdateMs, true);

        AZ::Interface<AZ::ComponentApplicationRequests>::Get()->RegisterEntityActivatedEventHandler(m_entityActivatedEventHandler);
        AZ::Interface<AZ::ComponentApplicationRequests>::Get()->RegisterEntityDeactivatedEventHandler(m_entityDeactivatedEventHandler);
    }

    bool ServerToClientReplicationWindow::ReplicationSetUpdateReady()
    {
        // if we don't have a controlled entity anymore, don't send updates (validate this)
        if (!m_controlledEntity.Exists())
        {
            m_replicationSet.clear();
        }
        return true;
    }

    const ReplicationSet& ServerToClientReplicationWindow::GetReplicationSet() const
    {
        return m_replicationSet;
    }

    uint32_t ServerToClientReplicationWindow::GetMaxEntityReplicatorSendCount() const
    {
        return m_isPoorConnection ? sv_MinEntitiesToReplicate : sv_MaxEntitiesToReplicate;
    }

    bool ServerToClientReplicationWindow::IsInWindow(const ConstNetworkEntityHandle& entityHandle, NetEntityRole& outNetworkRole) const
    {
        // TODO: Clean up this interface, this function is used for server->server migrations, and probably shouldn't be exposed in it's current setup
        AZ_Assert(false, "IsInWindow should not be called on the ServerToClientReplicationWindow");
        outNetworkRole = NetEntityRole::InvalidRole;
        auto iter = m_replicationSet.find(entityHandle);
        if (iter != m_replicationSet.end())
        {
            outNetworkRole = iter->second.m_netEntityRole;
            return true;
        }
        return false;
    }

    void ServerToClientReplicationWindow::UpdateWindow()
    {
        // clear the candidate queue, we're going to rebuild it
        ReplicationCandidateQueue clearQueue;
        clearQueue.get_container().reserve(sv_MaxEntitiesToTrackReplication);
        m_candidateQueue.swap(clearQueue);
        m_replicationSet.clear();

        NetBindComponent* netBindComponent = m_controlledEntity.GetNetBindComponent();
        if (!netBindComponent || !netBindComponent->HasController())
        {
            // if we don't have a controlled entity, or we no longer have control of the entity, don't run the update
            return;
        }

        EvaluateConnection();

        AZ::TransformInterface* transformInterface = m_controlledEntity.GetEntity()->GetTransform();
        const AZ::Vector3 controlledEntityPosition = transformInterface->GetWorldTranslation();

        AZStd::vector<AzFramework::VisibilityEntry*> gatheredEntries;
        AZ::Sphere awarenessSphere = AZ::Sphere(controlledEntityPosition, sv_ClientAwarenessRadius);
        AZ::Interface<AzFramework::IVisibilitySystem>::Get()->GetDefaultVisibilityScene()->Enumerate(awarenessSphere, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData)
            {
                gatheredEntries.reserve(gatheredEntries.size() + nodeData.m_entries.size());
                for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
                {
                    if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_Entity)
                    {
                        gatheredEntries.push_back(visEntry);
                    }
                }
            }
        );

        NetworkEntityTracker* networkEntityTracker = GetNetworkEntityTracker();

        // Add all the neighbors
        for (AzFramework::VisibilityEntry* visEntry : gatheredEntries)
        {
            //if (mp_ControlledFilteredEntityComponent && mp_ControlledFilteredEntityComponent->IsEntityFiltered(iterator.Get()))
            //{
            //    continue;
            //}

            // We want to find the closest extent to the player and prioritize using that distance
            AZ::Entity* entity = static_cast<AZ::Entity*>(visEntry->m_userData);
            NetBindComponent* entryNetBindComponent = entity->template FindComponent<NetBindComponent>();
            if (entryNetBindComponent != nullptr)
            {
                const AZ::Vector3 supportNormal = controlledEntityPosition - visEntry->m_boundingVolume.GetCenter();
                const AZ::Vector3 closestPosition = visEntry->m_boundingVolume.GetSupport(supportNormal);
                const float gatherDistanceSquared = controlledEntityPosition.GetDistanceSq(closestPosition);
                const float priority = (gatherDistanceSquared > 0.0f) ? 1.0f / gatherDistanceSquared : 0.0f;

                NetworkEntityHandle entityHandle(entryNetBindComponent, networkEntityTracker);
                AddEntityToReplicationSet(entityHandle, priority, gatherDistanceSquared);
            }
        }

        // Add in Autonomous Entities
        // Note: Do not add any Client entities after this point, otherwise you stomp over the Autonomous mode
        m_replicationSet[m_controlledEntity] = { NetEntityRole::Autonomous, 1.0f };  // Always replicate autonomous entities

        //auto hierarchyController = FindController<EntityHierarchyComponent::Authority>(m_ControlledEntity);
        //if (hierarchyController != nullptr)
        //{
        //    CollectControlledEntitiesRecursive(m_replicationSet, *hierarchyController);
        //}
    }

    void ServerToClientReplicationWindow::DebugDraw() const
    {
        static const float   BoundaryStripeHeight = 1.0f;
        static const float   BoundaryStripeSpacing = 0.5f;
        static const int32_t BoundaryStripeCount = 10;

        //if (auto localEnt = m_ControlledEntity.lock())
        //{
        //    auto* octreeNodeComponent = localEnt->FindComponent<OctreeNodeComponent::Server>();
        //    AZ_Assert(octreeNodeComponent != nullptr, "Controlled entity does not have an octree node component");
        //
        //    const float radius = octreeNodeComponent->GetAwarenessRadius();
        //    Vec3 currPos = octreeNodeComponent->GetPosition();
        //    currPos.z += BoundaryStripeHeight;
        //
        //    for (int i = 0; i < BoundaryStripeCount; ++i)
        //    {
        //        draw.Circle(currPos, radius, DebugColors::green);
        //        currPos.z +=BoundaryStripeSpacing;
        //    }
        //}
    }

    void ServerToClientReplicationWindow::OnEntityActivated(AZ::Entity* entity)
    {
        ConstNetworkEntityHandle entityHandle(entity, GetNetworkEntityTracker());
        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        if (netBindComponent != nullptr)
        {
            if (netBindComponent->HasController())
            {
                //if (mp_ControlledFilteredEntityComponent && mp_ControlledFilteredEntityComponent->IsEntityFiltered(newEntity))
                //{
                //    return;
                //}

                AZ::TransformInterface* transformInterface = entity->GetTransform();
                if (transformInterface != nullptr)
                {
                    const AZ::Vector3 clientPosition = m_controlledEntityTransform->GetWorldTranslation();
                    float distSq = clientPosition.GetDistanceSq(transformInterface->GetWorldTranslation());
                    float awarenessSq = sv_ClientAwarenessRadius * sv_ClientAwarenessRadius;
                    // Make sure we would be in the awareness radius
                    if (distSq < awarenessSq)
                    {
                        AddEntityToReplicationSet(entityHandle, 1.0f, distSq);
                    }
                }
            }
        }
    }

    void ServerToClientReplicationWindow::OnEntityDeactivated(AZ::Entity* entity)
    {
        ConstNetworkEntityHandle entityHandle(entity, GetNetworkEntityTracker());
        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        if (netBindComponent != nullptr)
        {
            m_replicationSet.erase(entityHandle);
        }
    }

    void ServerToClientReplicationWindow::EvaluateConnection()
    {
        const uint32_t newPacketsSent = m_connection->GetMetrics().m_packetsSent;
        const uint32_t packetSentDelta = newPacketsSent - m_lastCheckedSentPackets;

        if (packetSentDelta > sv_PacketsToIntegrateQos) // Just some threshold for having enough samples
        {
            const uint32_t newPacketsLost = m_connection->GetMetrics().m_packetsLost;
            const uint32_t packetLostDelta = newPacketsLost - m_lastCheckedLostPackets;
            const float packetLostPercent = float(packetLostDelta) / float(packetSentDelta);
            const bool isPoorConnection = (packetLostPercent > sv_BadConnectionThreshold);
            if (isPoorConnection != m_isPoorConnection)
            {
                m_isPoorConnection = isPoorConnection;
                AZLOG_INFO
                (
                    "Connection# %u with entity %u quality state changed status from %s to %s",
                    static_cast<uint32_t>(m_connection->GetConnectionId()),
                    static_cast<uint32_t>(m_controlledEntity.GetNetEntityId()),
                    GetConnectionStateString(!m_isPoorConnection),
                    GetConnectionStateString(m_isPoorConnection)
                );
            }

            m_lastCheckedSentPackets = newPacketsSent;
            m_lastCheckedLostPackets = newPacketsLost;
        }
    }

    void ServerToClientReplicationWindow::AddEntityToReplicationSet(ConstNetworkEntityHandle& entityHandle, float priority, [[maybe_unused]] float distanceSquared)
    {
        if (!sv_ReplicateServerProxies)
        {
            NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
            if ((netBindComponent != nullptr) && (netBindComponent->GetNetEntityRole() == NetEntityRole::Server))
            {
                // Proxy replication disabled
                return;
            }
        }

        const bool isQueueFull = (m_candidateQueue.size() >= sv_MaxEntitiesToTrackReplication);  // See if have the maximum number of entities in our set
        const bool isBetterChoice = !m_candidateQueue.empty() && (priority > m_candidateQueue.top().m_priority);  // Check if the new thing we are adding is better than the worst item in our set
        const bool isInReplicationSet = m_replicationSet.find(entityHandle) != m_replicationSet.end();
        if (!isInReplicationSet)
        {
            if (isQueueFull)  // if our set is full, then we need to remove the worst priority in our set
            {
                ConstNetworkEntityHandle removeEnt = m_candidateQueue.top().m_entityHandle;
                m_candidateQueue.pop();
                m_replicationSet.erase(removeEnt);
            }
            m_candidateQueue.push(PrioritizedReplicationCandidate(entityHandle, priority));
            m_replicationSet[entityHandle] = { NetEntityRole::Client, priority };
        }
    }

    //void ServerToClientReplicationWindow::CollectControlledEntitiesRecursive(ReplicationSet& replicationSet, EntityHierarchyComponent::Authority& hierarchyController)
    //{
    //    auto controlledEnts = hierarchyController.GetChildrenRelatedEntities();
    //    for (auto& controlledEnt : controlledEnts)
    //    {
    //        AZ_Assert(controlledEnt != nullptr, "We have lost a controlled entity unexpectedly");
    //        replicationSet[controlledEnt.GetConstEntity()] = EntityReplicationData(EntityNetworkRoleT::e_Autonomous, EntityPrioritySystem::k_MaxPriority); // Always replicate controlled entities
    //        auto hierarchyController = controlledEnt.FindController<EntityHierarchyComponent::Authority>();
    //        if (hierarchyController != nullptr)
    //        {
    //            CollectControlledEntitiesRecursive(replicationSet, *hierarchyController);
    //        }
    //    }
    //}

    //void ServerToClientReplicationWindow::OnAddFilteredEntity(NetEntityId filteredEntityId)
    //{
    //    ConstEntityPtr filteredEntity = gNovaGame->GetEntityManager().GetEntity(filteredEntityId);
    //    m_replicationSet.erase(filteredEntityId);
    //}
}
