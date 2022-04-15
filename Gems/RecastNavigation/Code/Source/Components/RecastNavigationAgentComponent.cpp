/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationAgentComponent.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

#pragma optimize("", off)

namespace RecastNavigation
{
    void RecastNavigationAgentComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationAgentComponent, AZ::Component>()
                ->Version(1)
                ->Field("Navigation Mesh Entity", &RecastNavigationAgentComponent::m_navigationMeshEntityId)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<RecastNavigationAgentComponent>("Recast Navigation Agent", "[Queries Recast Navigation Mesh]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &RecastNavigationAgentComponent::m_navigationMeshEntityId, "Navigation Mesh Entity", "")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RecastNavigationAgentComponent>("RecastNavigationAgentComponent")
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Navigation")
                ->Method("PathToEntity", &RecastNavigationAgentComponent::PathToEntity)
                ->Method("PathToPosition", &RecastNavigationAgentComponent::PathToPosition)
                ->Method("CancelPath", &RecastNavigationAgentComponent::CancelPath)

                ->Method("GetPathFoundEvent", [](AZ::EntityId id) -> AZ::Event<const AZStd::vector<AZ::Vector3>&>*
                    {
                        AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                        if (!entity)
                        {
                            AZ_Warning("RecastNavigationAgentComponent", false, "GetPathFoundEvent failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                                return nullptr;
                        }

                        RecastNavigationAgentComponent* component = entity->FindComponent<RecastNavigationAgentComponent>();
                        if (!component)
                        {
                            AZ_Warning("RecastNavigationAgentComponent", false, "GetPathFoundEvent failed. Entity '%s' (id: %s) is missing MultiplayerSystemComponent, be sure to add MultiplayerSystemComponent to this entity.", entity->GetName().c_str(), id.ToString().c_str())
                                return nullptr;
                        }

                        return &component->m_pathFoundEvent;
                    })
                ->Attribute(
                    AZ::Script::Attributes::AzEventDescription,
                    AZ::BehaviorAzEventDescription{ "Path Found Event", {"Path Points"} })

                        ->Method("GetNextTraversalPointEvent", [](AZ::EntityId id) -> AZ::Event<const AZ::Vector3&, const AZ::Vector3&>*
                            {
                                AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                                if (!entity)
                                {
                                    AZ_Warning("RecastNavigationAgentComponent", false, "GetNextTraversalPointEvent failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                                        return nullptr;
                                }

                                RecastNavigationAgentComponent* component = entity->FindComponent<RecastNavigationAgentComponent>();
                                if (!component)
                                {
                                    AZ_Warning("RecastNavigationAgentComponent", false, "GetNextTraversalPointEvent failed. Entity '%s' (id: %s) is missing MultiplayerSystemComponent, be sure to add MultiplayerSystemComponent to this entity.", entity->GetName().c_str(), id.ToString().c_str())
                                        return nullptr;
                                }

                                return &component->m_nextTraversalPointEvent;
                            })
                        ->Attribute(
                            AZ::Script::Attributes::AzEventDescription,
                            AZ::BehaviorAzEventDescription{ "Next Traversal Point Event", {"Next Point", "After Next Point"} });
                            ;
        }
    }

    void RecastNavigationAgentComponent::GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationAgentComponent"));
    }

    void RecastNavigationAgentComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void RecastNavigationAgentComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void RecastNavigationAgentComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void RecastNavigationAgentComponent::Activate()
    {
        RecastNavigationAgentRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();
    }

    void RecastNavigationAgentComponent::Deactivate()
    {
        RecastNavigationAgentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    AZStd::vector<AZ::Vector3> RecastNavigationAgentComponent::PathToEntity(AZ::EntityId targetEntity)
    {
        AZ::Vector3 end = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(end, targetEntity, &AZ::TransformBus::Events::GetWorldTranslation);

        return PathToPosition(end);
    }

    AZStd::vector<AZ::Vector3> RecastNavigationAgentComponent::PathToPosition(const AZ::Vector3& targetWorldPosition)
    {
        m_pathPoints.clear();
        m_currentPathIndex = 0;

        AZ::Vector3 start = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(start, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        RecastNavigationMeshRequestBus::EventResult(m_pathPoints, m_navigationMeshEntityId, &RecastNavigationMeshRequestBus::Events::FindPathToPosition, start, targetWorldPosition);

        if (m_pathPoints.empty())
        {
            m_pathFoundEvent.Signal({});
        }
        else
        {
            m_pathFoundEvent.Signal(m_pathPoints);

            if (m_pathPoints.size() >= 3)
            {
                m_nextTraversalPointEvent.Signal(m_pathPoints[1], m_pathPoints[2]);
            }
            else if (m_pathPoints.size() == 2)
            {
                m_nextTraversalPointEvent.Signal(m_pathPoints[1], m_pathPoints[1]);
            }
            else
            {
                m_nextTraversalPointEvent.Signal(m_pathPoints[0], m_pathPoints[0]);
            }
        }
        return m_pathPoints;
    }

    void RecastNavigationAgentComponent::CancelPath()
    {
        m_pathPoints.clear();
        m_currentPathIndex = 0;
        m_pathFoundEvent.Signal({});
    }

    void RecastNavigationAgentComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // TODO is this needed?
    }

    void RecastNavigationAgentComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        if (m_pathPoints.empty() || m_currentPathIndex >= m_pathPoints.size())
        {
            return;
        }

        if (m_distanceForArrivingToPoint > m_pathPoints[m_currentPathIndex].GetDistance(world.GetTranslation()))
        {
            m_currentPathIndex++;

            if (m_currentPathIndex + 1 == m_pathPoints.size())
            {
                m_nextTraversalPointEvent.Signal(m_pathPoints[m_currentPathIndex], m_pathPoints[m_currentPathIndex]);
            }
            else if (m_currentPathIndex + 1 < m_pathPoints.size())
            {
                m_nextTraversalPointEvent.Signal(m_pathPoints[m_currentPathIndex], m_pathPoints[m_currentPathIndex + 1]);
            }
            else
            {
                m_pathPoints.clear();
                m_currentPathIndex = 0;
                m_pathFoundEvent.Signal({});
            }
        }
    }

    void RecastNavigationAgentComponent::SetPathFoundEvent(AZ::Event<const AZStd::vector<AZ::Vector3>&>::Handler handler)
    {
        handler.Connect(m_pathFoundEvent);
    }

    void RecastNavigationAgentComponent::SetNextTraversalPointEvent(AZ::Event<const AZ::Vector3&, const AZ::Vector3&>::Handler handler)
    {
        handler.Connect(m_nextTraversalPointEvent);
    }
} // namespace RecastNavigation

#pragma optimize("", on)
