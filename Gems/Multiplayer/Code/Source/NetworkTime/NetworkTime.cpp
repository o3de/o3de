/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace Multiplayer
{
    AZ_CVAR(float, sv_RewindVolumeExtrudeDistance, 50.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "The amount to increase rewind volume checks to account for fast moving entities");
    AZ_CVAR(bool, bg_RewindDebugDraw, false, nullptr, AZ::ConsoleFunctorFlags::Null, "If true enables debug draw of rewind operations");

    void NetworkTime::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<INetworkTimeRequestBus>("Network Time Requests")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Attribute(AZ::Script::Attributes::Category, "Multiplayer")
                ->Event("IsTimeRewound", &INetworkTimeRequestBus::Events::IsTimeRewound)
                ->Event("GetHostFrameId", &INetworkTimeRequestBus::Events::GetHostFrameId)
                ->Event("GetHostFrameId (Unaltered)", &INetworkTimeRequestBus::Events::GetUnalteredHostFrameId)
                ;
        }
    }

    NetworkTime::NetworkTime()
    {
        AZ::Interface<INetworkTime>::Register(this);
        INetworkTimeRequestBus::Handler::BusConnect();
    }

    NetworkTime::~NetworkTime()
    {
        INetworkTimeRequestBus::Handler::BusDisconnect();
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
        m_hostTimeMs = AZ::GetElapsedTimeMs();
    }

    AZ::TimeMs NetworkTime::GetHostTimeMs() const
    {
        return m_hostTimeMs;
    }

    float NetworkTime::GetHostBlendFactor() const
    {
        return m_hostBlendFactor;
    }

    AzNetworking::ConnectionId NetworkTime::GetRewindingConnectionId() const
    {
        return m_rewindingConnectionId;
    }

    void NetworkTime::ForceSetTime(HostFrameId frameId, AZ::TimeMs timeMs)
    {
        AZ_Assert(!IsTimeRewound(), "Forcibly setting network time is unsupported under a rewound time scope");
        m_unalteredFrameId = frameId;
        m_hostFrameId = frameId;
        m_hostTimeMs = timeMs;
        m_rewindingConnectionId = AzNetworking::InvalidConnectionId;
    }

    void NetworkTime::AlterTime(HostFrameId frameId, AZ::TimeMs timeMs, float blendFactor, AzNetworking::ConnectionId rewindConnectionId)
    {
        m_hostFrameId = frameId;
        m_hostTimeMs = timeMs;
        m_hostBlendFactor = blendFactor;
        m_rewindingConnectionId = rewindConnectionId;
    }

    void NetworkTime::SyncEntitiesToRewindState(const AZ::Aabb& rewindVolume)
    {
        if (!IsTimeRewound())
        {
            // If we're not inside a rewind scope then reset any rewound state and exit
            ClearRewoundEntities();
            return;
        }

        // Since the vis system doesn't support rewound queries, first query with an expanded volume to catch any fast moving entities
        const AZ::Aabb expandedVolume = rewindVolume.GetExpanded(AZ::Vector3(sv_RewindVolumeExtrudeDistance));

        AzFramework::DebugDisplayRequests* debugDisplay = nullptr;
        if (bg_RewindDebugDraw)
        {
            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
            debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
        }

        if (debugDisplay)
        {
            debugDisplay->SetColor(AZ::Colors::Red);
            debugDisplay->DrawWireBox(expandedVolume.GetMin(), expandedVolume.GetMax());
        }

        NetworkEntityTracker* networkEntityTracker = GetNetworkEntityTracker();
        AzFramework::IEntityBoundsUnion* entityBoundsUnion = AZ::Interface<AzFramework::IEntityBoundsUnion>::Get();
        AZ::Interface<AzFramework::IVisibilitySystem>::Get()->GetDefaultVisibilityScene()->Enumerate(expandedVolume,
            [this, debugDisplay, networkEntityTracker, entityBoundsUnion, rewindVolume](const AzFramework::IVisibilityScene::NodeData& nodeData)
        {
            m_rewoundEntities.reserve(m_rewoundEntities.size() + nodeData.m_entries.size());
            for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
            {
                if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_Entity)
                {
                    AZ::Entity* entity = static_cast<AZ::Entity*>(visEntry->m_userData);
                    NetworkEntityHandle entityHandle(entity, networkEntityTracker);
                    if (entityHandle.GetNetBindComponent() != nullptr)
                    {
                        const AZ::Aabb currentBounds = entityBoundsUnion->GetEntityWorldBoundsUnion(entity->GetId());
                        const AZ::Vector3 currentCenter = currentBounds.GetCenter();
                        NetworkTransformComponent* networkTransform = entity->template FindComponent<NetworkTransformComponent>();
                        if (debugDisplay)
                        {
                            debugDisplay->SetColor(AZ::Colors::White);
                            debugDisplay->DrawWireBox(currentBounds.GetMin(), currentBounds.GetMax());
                        }

                        if (networkTransform != nullptr)
                        {
                            // Get the rewound position for target host frame ID plus the one preceding it for potential lerp
                            AZ::Vector3 rewindCenter = networkTransform->GetTranslation();
                            const AZ::Vector3 rewindCenterPrevious = networkTransform->GetTranslationPrevious();
                            const float blendFactor = GetNetworkTime()->GetHostBlendFactor();
                            if (!AZ::IsClose(blendFactor, 1.0f) && !rewindCenter.IsClose(rewindCenterPrevious))
                            {
                                // If we have a blend factor, lerp the translation for accuracy
                                rewindCenter = rewindCenterPrevious.Lerp(rewindCenter, blendFactor);
                            }
                            const AZ::Vector3 rewindOffset = rewindCenter - currentCenter; // Compute offset between rewound and current positions
                            const AZ::Aabb rewoundAabb = currentBounds.GetTranslated(rewindOffset); // Apply offset to the entity aabb
                            if (debugDisplay)
                            {
                                debugDisplay->SetColor(AZ::Colors::Grey);
                                debugDisplay->DrawWireBox(rewoundAabb.GetMin(), rewoundAabb.GetMax());
                            }

                            if (AZ::ShapeIntersection::Overlaps(rewoundAabb, rewindVolume)) // Validate the rewound aabb intersects our rewind volume
                            {
                                m_rewoundEntities.push_back(entityHandle);
                                entityHandle.GetNetBindComponent()->NotifySyncRewindState();
                            }
                        }
                    }
                }
            }
        });
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
