/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Components/DetourCrowdNavigationComponent.h>
#include <DetourCrowd.h>
#include <RecastNavigation/DetourCrowdAgentBus.h>
#include <RecastNavigation/NavMeshQuery.h>
#include <RecastNavigation/RecastHelpers.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>
#include <RecastNavigation/RecastSmartPointer.h>

namespace RecastNavigation
{
    AZStd::vector<DetourObstacleAvoidanceParams> DetourCrowdNavigationComponent::CreateDefaultObstacleAvoidanceParams()
    {
        return { DetourObstacleAvoidanceParams::CreateLowQuality(),
                 DetourObstacleAvoidanceParams::CreateMediumQuality(),
                 DetourObstacleAvoidanceParams::CreateGoodQuality(),
                 DetourObstacleAvoidanceParams::CreateHighQuality() };
    }

    DetourCrowdNavigationComponent::DetourCrowdNavigationComponent() = default;

    bool DetourCrowdNavigationComponent::InitializeCrowd()
    {
        if (!m_crowd)
        {
            m_crowd = RecastPointer<dtCrowd>(dtAllocCrowd());
            if (!m_crowd)
            {
                AZ_Error("DetourCrowdNavigationComponent", false, "Failed to allocate dtCrowd object.");
                return false;
            }
        }

        AZStd::shared_ptr<NavMeshQuery> navMeshQuery;
        RecastNavigationMeshRequestBus::EventResult(navMeshQuery, m_navQueryEntityId, &RecastNavigationMeshRequests::GetNavigationObject);
        if (!navMeshQuery)
        {
            AZ_Warning(
                "DetourCrowdNavigationComponent",
                false,
                "Navigation object is not available yet for entity '%s'. Crowd initialization postponed.",
                m_navQueryEntityId.ToString().c_str());
            return false;
        }

        NavMeshQuery::LockGuard lock(*navMeshQuery);
        if (!lock.GetNavQuery())
        {
            AZ_Warning(
                "DetourCrowdNavigationComponent",
                false,
                "Navigation query is not available yet for entity '%s'. Crowd initialization postponed.",
                m_navQueryEntityId.ToString().c_str());
            return false;
        }

        if (!m_crowd->init(m_maxAgents, m_maxAgentRadius, lock.GetNavMesh()))
        {
            AZ_Error("DetourCrowdNavigationComponent", false, "Failed to initialize dtCrowd.");
            return false;
        }

        m_agentEntityToCrowdIndex.clear();

        // Choose parameter set based on mode
        const AZStd::vector<DetourObstacleAvoidanceParams> simpleObstacleAvoidanceParams = CreateDefaultObstacleAvoidanceParams();
        const AZStd::vector<DetourObstacleAvoidanceParams>& obstacleAvoidanceParams =
            m_useAdvancedObstacleAvoidanceParams ? m_obstacleAvoidanceParams : simpleObstacleAvoidanceParams;

        const AZ::u8 maxAvoidanceProfiles = DT_CROWD_MAX_OBSTAVOIDANCE_PARAMS;
        if (obstacleAvoidanceParams.size() > static_cast<size_t>(maxAvoidanceProfiles))
        {
            AZ_Warning(
                "DetourCrowdNavigationComponent",
                false,
                "Obstacle avoidance profile count (%zu) exceeds Detour max (%u). Extra profiles will be ignored.",
                obstacleAvoidanceParams.size(),
                maxAvoidanceProfiles);
        }

        const size_t profilesToApply = AZStd::min(static_cast<size_t>(maxAvoidanceProfiles), obstacleAvoidanceParams.size());
        for (size_t profileIndex = 0; profileIndex < profilesToApply; ++profileIndex)
        {
            const dtObstacleAvoidanceParams detourParams = obstacleAvoidanceParams[profileIndex].ToDetourObstacleAvoidanceParams();
            m_crowd->setObstacleAvoidanceParams(static_cast<int>(profileIndex), &detourParams);
        }

        m_crowdInitialized = true;
        return true;
    }

    AZ::Crc32 DetourCrowdNavigationComponent::GetAdvancedObstacleAvoidanceParamsVisibility() const
    {
        return m_useAdvancedObstacleAvoidanceParams ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void DetourCrowdNavigationComponent::OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity)
    {
        // Only init if the crowd hasn't been set up yet. The dtNavMesh is updated in-place during recalculation.
        if (navigationMeshEntity == m_navQueryEntityId && !m_crowdInitialized)
        {
            InitializeCrowd();
        }
    }

    void DetourCrowdNavigationComponent::OnNavigationMeshBeganRecalculating([[maybe_unused]] AZ::EntityId navigationMeshEntity)
    {
    }

    void DetourCrowdNavigationComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_crowd || deltaTime <= 0.0f)
        {
            return;
        }

        m_crowd->update(deltaTime, nullptr);

        AZStd::vector<AZStd::pair<AZ::EntityId, AZ::u32>> trackedAgents;
        trackedAgents.reserve(m_agentEntityToCrowdIndex.size());
        for (const auto& [agentEntityId, crowdIndex] : m_agentEntityToCrowdIndex)
        {
            trackedAgents.emplace_back(agentEntityId, crowdIndex);
        }

        for (const auto& [agentEntityId, crowdIndex] : trackedAgents)
        {
            const dtCrowdAgent* crowdAgent = m_crowd->getAgent(static_cast<int>(crowdIndex));
            if (!crowdAgent || !crowdAgent->active)
            {
                continue;
            }

            const AZ::Vector3 worldPosition = RecastVector3::CreateFromFloatValuesWithoutAxisSwapping(crowdAgent->npos).AsVector3WithZup();
            const AZ::Vector3 worldVelocity = RecastVector3::CreateFromFloatValuesWithoutAxisSwapping(crowdAgent->vel).AsVector3WithZup();
            DetourCrowdAgentNotificationBus::Event(
                agentEntityId, &DetourCrowdAgentNotifications::OnAgentPositionUpdated, worldPosition, worldVelocity);
        }
    }

    AZ::Outcome<void, AZStd::string> DetourCrowdNavigationComponent::AddAgent(
        AZ::EntityId agentEntityId, const AZ::Vector3& worldPosition, const DetourCrowdAgentParams& agentParams)
    {
        if (!agentEntityId.IsValid())
        {
            return AZ::Failure(AZStd::string("Cannot add agent: invalid entity id."));
        }

        if (!m_crowd)
        {
            return AZ::Failure(AZStd::string("Cannot add agent: crowd is not initialized."));
        }

        if (m_agentEntityToCrowdIndex.find(agentEntityId) != m_agentEntityToCrowdIndex.end())
        {
            return AZ::Failure(
                AZStd::string::format("Cannot add agent: entity '%s' is already registered.", agentEntityId.ToString().c_str()));
        }

        const int maxAgents = m_crowd->getAgentCount();
        if (maxAgents <= 0)
        {
            return AZ::Failure(AZStd::string("Cannot add agent: crowd has invalid max agent count."));
        }

        if (m_agentEntityToCrowdIndex.size() >= static_cast<size_t>(maxAgents))
        {
            return AZ::Failure(AZStd::string::format("Cannot add agent: crowd is full (%d agents).", maxAgents));
        }

        dtCrowdAgentParams detourParams = agentParams.ToDetourCrowdAgentParams();

        // Convert O3DE Z-up to Recast Y-up.
        RecastVector3 recastPos = RecastVector3::CreateFromVector3SwapYZ(worldPosition);

        const int crowdIndex = m_crowd->addAgent(recastPos.GetData(), &detourParams);
        if (crowdIndex < 0)
        {
            return AZ::Failure(AZStd::string("Cannot add agent: dtCrowd::addAgent failed."));
        }

        m_agentEntityToCrowdIndex.emplace(agentEntityId, static_cast<AZ::u32>(crowdIndex));
        return AZ::Success();
    }

    AZ::Outcome<AZ::u32, AZStd::string> DetourCrowdNavigationComponent::GetCrowdIndexForAgent(AZ::EntityId agentEntityId) const
    {
        if (!agentEntityId.IsValid())
        {
            return AZ::Failure(AZStd::string("Invalid entity id."));
        }

        if (!m_crowd)
        {
            return AZ::Failure(AZStd::string("Crowd is not initialized."));
        }

        const auto it = m_agentEntityToCrowdIndex.find(agentEntityId);
        if (it == m_agentEntityToCrowdIndex.end())
        {
            return AZ::Failure(AZStd::string::format("Entity '%s' is not registered.", agentEntityId.ToString().c_str()));
        }

        return AZ::Success(it->second);
    }

    AZ::Outcome<void, AZStd::string> DetourCrowdNavigationComponent::RemoveAgent(AZ::EntityId agentEntityId)
    {
        const AZ::Outcome<AZ::u32, AZStd::string> crowdIndexOutcome = GetCrowdIndexForAgent(agentEntityId);
        if (!crowdIndexOutcome.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format("Cannot remove agent: %s", crowdIndexOutcome.GetError().c_str()));
        }

        const AZ::u32 crowdIndex = crowdIndexOutcome.GetValue();
        m_agentEntityToCrowdIndex.erase(agentEntityId);
        m_crowd->removeAgent(crowdIndex);

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> DetourCrowdNavigationComponent::SetAgentMoveTarget(
        AZ::EntityId agentEntityId, const AZ::Vector3& targetPosition)
    {
        const AZ::Outcome<AZ::u32, AZStd::string> crowdIndexOutcome = GetCrowdIndexForAgent(agentEntityId);
        if (!crowdIndexOutcome.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format("Cannot set move target: %s", crowdIndexOutcome.GetError().c_str()));
        }

        const int crowdIndex = static_cast<int>(crowdIndexOutcome.GetValue());
        const dtCrowdAgent* agent = m_crowd->getAgent(crowdIndex);
        if (!agent || !agent->active)
        {
            return AZ::Failure(AZStd::string::format("Cannot set move target: agent at crowd index %d is not active.", crowdIndex));
        }

        // Get the navigation mesh query to find the target polygon reference
        AZStd::shared_ptr<NavMeshQuery> navMeshQuery;
        RecastNavigationMeshRequestBus::EventResult(navMeshQuery, m_navQueryEntityId, &RecastNavigationMeshRequests::GetNavigationObject);
        if (!navMeshQuery)
        {
            return AZ::Failure(AZStd::string("Cannot set move target: navigation mesh query is not available."));
        }

        NavMeshQuery::LockGuard lock(*navMeshQuery);
        dtNavMeshQuery* query = lock.GetNavQuery();
        if (!query)
        {
            return AZ::Failure(AZStd::string("Cannot set move target: failed to get navigation query."));
        }

        // Convert O3DE Z-up to Recast Y-up for target position
        RecastVector3 recastTarget = RecastVector3::CreateFromVector3SwapYZ(targetPosition);
        const float halfExtents[3] = { m_maxAgentRadius * 2.0f, m_maxAgentRadius * 4.0f, m_maxAgentRadius * 2.0f };

        // Find the nearest polygon reference for the target position
        dtPolyRef targetPoly = 0;
        RecastVector3 nearestPoint;
        const dtQueryFilter filter;
        const dtStatus findPolyStatus =
            query->findNearestPoly(recastTarget.GetData(), halfExtents, &filter, &targetPoly, nearestPoint.GetData());
        if (dtStatusFailed(findPolyStatus) || targetPoly == 0)
        {
            return AZ::Failure(AZStd::string::format("Cannot set move target: could not find valid polygon for target position."));
        }

        // Request the crowd to move the agent to the target
        if (!m_crowd->requestMoveTarget(crowdIndex, targetPoly, recastTarget.GetData()))
        {
            return AZ::Failure(
                AZStd::string::format("Cannot set move target: dtCrowd::requestMoveTarget failed for agent %d.", crowdIndex));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> DetourCrowdNavigationComponent::ResetAgentMoveTarget(AZ::EntityId agentEntityId)
    {
        const AZ::Outcome<AZ::u32, AZStd::string> crowdIndexOutcome = GetCrowdIndexForAgent(agentEntityId);
        if (!crowdIndexOutcome.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format("Cannot reset move target: %s", crowdIndexOutcome.GetError().c_str()));
        }

        const int crowdIndex = static_cast<int>(crowdIndexOutcome.GetValue());
        const dtCrowdAgent* agent = m_crowd->getAgent(crowdIndex);
        if (!agent || !agent->active)
        {
            return AZ::Failure(AZStd::string::format("Cannot reset move target: agent at crowd index %d is not active.", crowdIndex));
        }

        if (!m_crowd->resetMoveTarget(crowdIndex))
        {
            return AZ::Failure(
                AZStd::string::format("Cannot reset move target: dtCrowd::resetMoveTarget failed for agent %d.", crowdIndex));
        }

        return AZ::Success();
    }

    void DetourCrowdNavigationComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DetourCrowdNavigationComponent, AZ::Component>()
                ->Version(2)
                ->Field("NavQueryEntityId", &DetourCrowdNavigationComponent::m_navQueryEntityId)
                ->Field("MaxAgents", &DetourCrowdNavigationComponent::m_maxAgents)
                ->Field("MaxAgentRadius", &DetourCrowdNavigationComponent::m_maxAgentRadius)
                ->Field("UseAdvancedObstacleAvoidanceParams", &DetourCrowdNavigationComponent::m_useAdvancedObstacleAvoidanceParams)
                ->Field("ObstacleAvoidanceParams", &DetourCrowdNavigationComponent::m_obstacleAvoidanceParams);

            if (auto editContext = serialize->GetEditContext())
            {
                editContext
                    ->Class<DetourCrowdNavigationComponent>(
                        "Detour Crowd Navigation Component", "Provides crowd navigation features using DetourCrowd.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourCrowdNavigationComponent::m_navQueryEntityId,
                        "Navigation Mesh",
                        "Entity with Recast Navigation Mesh component")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourCrowdNavigationComponent::m_maxAgents,
                        "Max Agents",
                        "Maximum number of agents that can be managed by the crowd.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourCrowdNavigationComponent::m_maxAgentRadius,
                        "Max Agent Radius",
                        "Maximum radius of any agent that will be added to the crowd.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox,
                        &DetourCrowdNavigationComponent::m_useAdvancedObstacleAvoidanceParams,
                        "Manual Obstacle Avoidance Presets",
                        "Direct control over avoidance parameters. Disable for preset profiles.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourCrowdNavigationComponent::m_obstacleAvoidanceParams,
                        "Obstacle Avoidance Params",
                        "Obstacle avoidance parameter profiles. Indices map to Detour quality levels.")
                    ->Attribute(
                        AZ::Edit::Attributes::Visibility, &DetourCrowdNavigationComponent::GetAdvancedObstacleAvoidanceParamsVisibility);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<DetourCrowdNavigationRequestBus>("DetourCrowdNavigationRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Recast Navigation")
                ->Event("AddAgent", &DetourCrowdNavigationRequests::AddAgent)
                ->Event("RemoveAgent", &DetourCrowdNavigationRequests::RemoveAgent)
                ->Event("SetAgentMoveTarget", &DetourCrowdNavigationRequests::SetAgentMoveTarget)
                ->Event("ResetAgentMoveTarget", &DetourCrowdNavigationRequests::ResetAgentMoveTarget);

            behaviorContext->Class<DetourCrowdNavigationComponent>()->RequestBus("DetourCrowdNavigationRequestBus");

            behaviorContext->EBus<DetourCrowdAgentNotificationBus>("DetourCrowdAgentNotificationBus")
                ->Handler<DetourCrowdAgentNotificationHandler>();
        }
    }

    void DetourCrowdNavigationComponent::Activate()
    {
        if (!m_navQueryEntityId.IsValid())
        {
            AZ_Error("DetourCrowdNavigationComponent", false, "Cannot activate: NavQueryEntityId is invalid.");
            return;
        }

        DetourCrowdNavigationRequestBus::Handler::BusConnect(GetEntityId());
        RecastNavigationMeshNotificationBus::Handler::BusConnect(m_navQueryEntityId);
        AZ::TickBus::Handler::BusConnect();

        InitializeCrowd();
    }

    void DetourCrowdNavigationComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        RecastNavigationMeshNotificationBus::Handler::BusDisconnect();
        DetourCrowdNavigationRequestBus::Handler::BusDisconnect();
        m_agentEntityToCrowdIndex.clear();
        m_crowd = nullptr;
        m_crowdInitialized = false;
    }
} // namespace RecastNavigation
