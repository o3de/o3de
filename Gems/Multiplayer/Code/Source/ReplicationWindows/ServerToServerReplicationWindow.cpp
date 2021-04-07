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

#include <Source/ReplicationWindows/ServerToServerReplicationWindow.h>
#include <Source/Components/NetBindComponent.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    AZ_CVAR(float, sv_ReplicationWindowWidth, 100.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "This is the additional area around the non-overlapping map region over which the server should replicate entities");
    AZ_CVAR(AZ::TimeMs, sv_ServerReplicationWindowUpdateMs, AZ::TimeMs{ 300 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Rate for replication window updates.");

    ServerToServerReplicationWindow::ServerToServerReplicationWindow(const AZ::Aabb& aabb)
        : m_aabb(aabb)
        , m_updateWindowEvent([this]() { UpdateWindow(); }, AZ::Name("Server to server replication window update event"))
        , m_controllersActivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersActivated(entityHandle, entityIsMigrating); })
        , m_controllersDeactivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersDeactivated(entityHandle, entityIsMigrating); })
    {
        m_aabb.Expand(AZ::Vector3(sv_ReplicationWindowWidth, sv_ReplicationWindowWidth, sv_ReplicationWindowWidth));
        m_updateWindowEvent.Enqueue(sv_ServerReplicationWindowUpdateMs, true);
        GetNetworkEntityManager()->AddControllersActivatedHandler(m_controllersActivatedHandler);
        GetNetworkEntityManager()->AddControllersDeactivatedHandler(m_controllersDeactivatedHandler);
    }

    bool ServerToServerReplicationWindow::ReplicationSetUpdateReady()
    {
        return m_initialGatherComplete;
    }

    const ReplicationSet& ServerToServerReplicationWindow::GetReplicationSet() const
    {
        return m_replicationSet;
    }

    uint32_t ServerToServerReplicationWindow::GetMaxEntityReplicatorSendCount() const
    {
        return AZStd::numeric_limits<uint32_t>::max();
    }

    bool ServerToServerReplicationWindow::IsInWindow(const ConstNetworkEntityHandle& entityHandle, NetEntityRole& outNetworkRole) const
    {
        outNetworkRole = NetEntityRole::InvalidRole;
        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        if (netBindComponent != nullptr)
        {
            NetEntityRole networkRole = netBindComponent->GetNetEntityRole();
            if (networkRole == NetEntityRole::Authority)
            {
                const AZ::Entity* entity = entityHandle.GetEntity();
                AZ::TransformInterface* transformInterface = entity->GetTransform();
                AZ::Vector3 entityPosition = transformInterface->GetWorldTranslation();
                if (m_aabb.Contains(entityPosition))
                {
                    outNetworkRole = Multiplayer::NetEntityRole::Server;
                    return true;
                }
            }
        }

        return false;
    }

    void ServerToServerReplicationWindow::UpdateWindow()
    {
        m_initialGatherComplete = true; // Auto updates bootstrapped

        m_replicationSet.clear();

        AZ::Interface<AzFramework::IVisibilitySystem>::Get()->Enumerate(m_aabb, [this](const AzFramework::IVisibilitySystem::NodeData& nodeData)
            {
                for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
                {
                    if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_NetEntity)
                    {
                        ConstNetworkEntityHandle entityHandle = ConstNetworkEntityHandle(static_cast<AZ::Entity*>(visEntry->m_userData), GetNetworkEntityTracker());
                        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
                        if (netBindComponent != nullptr)
                        {
                            NetEntityRole networkRole = netBindComponent->GetNetEntityRole();
                            if (networkRole == NetEntityRole::Authority)
                            {
                                m_replicationSet[entityHandle] = { NetEntityRole::Server, 0.0f }; // Note, server replication does not use priority
                            }
                        }
                    }
                }
            }
        );
    }

    void ServerToServerReplicationWindow::DebugDraw() const
    {
        static const float BoundaryStripeHeight = 1.0f;
        static const float BoundaryStripeSpacing = 0.5f;
        static const int32_t BoundaryStripeCount = 10;

        //auto* loc = draw.GetOwnerConst()->FindComponent<LocationComponent::Server>();
        //if (loc == nullptr)
        //{
        //    return;
        //}
        //
        //Vec3 dmnMin = m_ReplicationWindowParams.GetMin();
        //Vec3 dmnMax = m_ReplicationWindowParams.GetMax();
        //
        //dmnMin.z = loc->GetPosition().z;
        //dmnMax.z = dmnMin.z + k_BoundaryStripeHeight;
        //
        //for (int i = 0; i < k_BoundaryStripeCount; ++i)
        //{
        //    draw.AABB(dmnMin, dmnMax, DebugColors::green);
        //    dmnMin.z += k_BoundaryStripeSpacing;
        //    dmnMax.z += k_BoundaryStripeSpacing;
        //}
    }

    void ServerToServerReplicationWindow::OnControllersActivated(const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        NetEntityRole networkRole = NetEntityRole::InvalidRole;
        if (IsInWindow(entityHandle, networkRole))
        {
            m_replicationSet[entityHandle] = { NetEntityRole::Server, 0.0f };
        }
    }

    void ServerToServerReplicationWindow::OnControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        m_replicationSet.erase(entityHandle);
    }
}
