/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkTime/NetworkTime.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>

namespace Multiplayer
{
    AZ_CVAR(float, sv_RewindVolumeExtrudeDistance, 50.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "The amount to increase rewind volume checks to account for fast moving entities");

    NetworkTime::NetworkTime()
    {
        AZ::Interface<INetworkTime>::Register(this);
    }

    NetworkTime::~NetworkTime()
    {
        AZ::Interface<INetworkTime>::Unregister(this);
    }

    bool NetworkTime::IsTimeRewound() const
    {
        return m_rewindingConnectionId != AzNetworking::InvalidConnectionId;
    }

    HostFrameId NetworkTime::GetHostFrameId() const
    {
        return m_hostFrameId;
    }

    HostFrameId NetworkTime::GetUnalteredHostFrameId() const
    {
        return m_unalteredFrameId;
    }

    void NetworkTime::IncrementHostFrameId()
    {
        AZ_Assert(!IsTimeRewound(), "Incrementing the global application frameId is unsupported under a rewound time scope");
        ++m_unalteredFrameId;
        m_hostFrameId = m_unalteredFrameId;
    }

    AZ::TimeMs NetworkTime::GetHostTimeMs() const
    {
        return m_hostTimeMs;
    }

    AzNetworking::ConnectionId NetworkTime::GetRewindingConnectionId() const
    {
        return m_rewindingConnectionId;
    }

    HostFrameId NetworkTime::GetHostFrameIdForRewindingConnection(AzNetworking::ConnectionId rewindConnectionId) const
    {
        return (IsTimeRewound() && (rewindConnectionId == m_rewindingConnectionId)) ? m_unalteredFrameId : m_hostFrameId;
    }

    void NetworkTime::AlterTime(HostFrameId frameId, AZ::TimeMs timeMs, AzNetworking::ConnectionId rewindConnectionId)
    {
        m_hostFrameId = frameId;
        m_hostTimeMs = timeMs;
        m_rewindingConnectionId = rewindConnectionId;
    }

    void NetworkTime::SyncEntitiesToRewindState(const AZ::Aabb& rewindVolume)
    {
        // Since the vis system doesn't support rewound queries, first query with an expanded volume to catch any fast moving entities
        const AZ::Aabb expandedVolume = rewindVolume.GetExpanded(AZ::Vector3(sv_RewindVolumeExtrudeDistance));

        AzFramework::IEntityBoundsUnion* entityBoundsUnion = AZ::Interface<AzFramework::IEntityBoundsUnion>::Get();
        AZStd::vector<NetBindComponent*> gatheredEntities;
        AZ::Interface<AzFramework::IVisibilitySystem>::Get()->GetDefaultVisibilityScene()->Enumerate(expandedVolume,
            [entityBoundsUnion, rewindVolume, &gatheredEntities](const AzFramework::IVisibilityScene::NodeData& nodeData)
        {
            gatheredEntities.reserve(gatheredEntities.size() + nodeData.m_entries.size());
            for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
            {
                if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_Entity)
                {
                    AZ::Entity* entity = static_cast<AZ::Entity*>(visEntry->m_userData);
                    const AZ::Aabb currentBounds = entityBoundsUnion->GetEntityLocalBoundsUnion(entity->GetId());
                    const AZ::Vector3 currentCenter = currentBounds.GetCenter();

                    NetworkTransformComponent* networkTransform = entity->template FindComponent<NetworkTransformComponent>();

                    if (networkTransform != nullptr)
                    {
                        const AZ::Vector3 rewindCenter = networkTransform->GetTranslation(); // Get the rewound position
                        const AZ::Vector3 rewindOffset = rewindCenter - currentCenter; // Compute offset between rewound and current positions
                        const AZ::Aabb rewoundAabb = currentBounds.GetTranslated(rewindOffset); // Apply offset to the entity aabb

                        if (AZ::ShapeIntersection::Overlaps(rewoundAabb, rewindVolume)) // Validate the rewound aabb intersects our rewind volume
                        {
                            // Due to component constraints, netBindComponent must exist if networkTransform exists
                            NetBindComponent* netBindComponent = entity->template FindComponent<NetBindComponent>();
                            gatheredEntities.push_back(netBindComponent);
                        }
                    }
                }
            }
        });

        NetworkEntityTracker* networkEntityTracker = GetNetworkEntityTracker();
        for (NetBindComponent* netBindComponent : gatheredEntities)
        {
            netBindComponent->NotifySyncRewindState();
            m_rewoundEntities.push_back(NetworkEntityHandle(netBindComponent, networkEntityTracker));
        }
    }

    void NetworkTime::ClearRewoundEntities()
    {
        AZ_Assert(!IsTimeRewound(), "Cannot clear rewound entity state while still within scoped rewind");

        for (NetworkEntityHandle entityHandle : m_rewoundEntities)
        {
            if (NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent())
            {
                netBindComponent->NotifySyncRewindState();
            }
        }
        m_rewoundEntities.clear();
    }
}
