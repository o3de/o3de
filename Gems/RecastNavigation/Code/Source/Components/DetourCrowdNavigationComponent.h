/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <DetourCrowd.h>
#include <RecastNavigation/DetourCrowdNavigationBus.h>
#include <RecastNavigation/DetourObstacleAvoidanceParams.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>
#include <RecastNavigation/RecastSmartPointer.h>

namespace RecastNavigation
{
    class DetourCrowdNavigationComponent final
        : public AZ::Component
        , private AZ::TickBus::Handler
        , public DetourCrowdNavigationRequestBus::Handler
        , private RecastNavigationMeshNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(DetourCrowdNavigationComponent, "{FECA2921-AF33-4968-9E3C-802965F0E7A2}");

        DetourCrowdNavigationComponent();

        static void Reflect(AZ::ReflectContext* context);

        // DetourCrowdNavigationRequestBus overrides
        AZ::Outcome<void, AZStd::string> AddAgent(
            AZ::EntityId agentEntityId, const AZ::Vector3& worldPosition, const DetourCrowdAgentParams& agentParams) override;
        AZ::Outcome<void, AZStd::string> RemoveAgent(AZ::EntityId agentEntityId) override;
        AZ::Outcome<void, AZStd::string> SetAgentMoveTarget(AZ::EntityId agentEntityId, const AZ::Vector3& targetPosition) override;
        AZ::Outcome<void, AZStd::string> ResetAgentMoveTarget(AZ::EntityId agentEntityId) override;

        void Activate() override;
        void Deactivate() override;

    private:
        bool InitializeCrowd();
        AZ::Crc32 GetAdvancedObstacleAvoidanceParamsVisibility() const;
        static AZStd::vector<DetourObstacleAvoidanceParams> CreateDefaultObstacleAvoidanceParams();

        AZ::Outcome<AZ::u32, AZStd::string> GetCrowdIndexForAgent(AZ::EntityId agentEntityId) const;

        // AZ::TickBus::Handler overrides.
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // RecastNavigationMeshNotificationBus overrides.
        void OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity) override;
        void OnNavigationMeshBeganRecalculating(AZ::EntityId navigationMeshEntity) override;

        AZ::EntityId m_navQueryEntityId;
        int m_maxAgents = 100;
        float m_maxAgentRadius = 0.6f;
        bool m_useAdvancedObstacleAvoidanceParams = false;
        AZStd::vector<DetourObstacleAvoidanceParams> m_obstacleAvoidanceParams;

        bool m_crowdInitialized = false;
        AZStd::unordered_map<AZ::EntityId, AZ::u32> m_agentEntityToCrowdIndex;

        RecastPointer<dtCrowd> m_crowd = nullptr;
    };
} // namespace RecastNavigation
