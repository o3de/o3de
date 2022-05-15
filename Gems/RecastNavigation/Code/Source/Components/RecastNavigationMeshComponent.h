/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DetourNavMesh.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <Components/RecastNavigationMeshCommon.h>
#include <Components/RecastNavigationMeshConfig.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

namespace RecastNavigation
{
    //! Calculates a navigation mesh with the triangle data provided by @RecastNavigationSurveyorComponent.
    //! Provides APIs to find a path between two entities or two world positions.
    class RecastNavigationMeshComponent final
        : public AZ::Component
        , public RecastNavigationMeshCommon
        , public RecastNavigationMeshRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationMeshComponent, "{a281f314-a525-4c05-876d-17eb632f14b4}");
        RecastNavigationMeshComponent() = default;
        RecastNavigationMeshComponent(const RecastNavigationMeshConfig& config, bool drawDebug);

        static void Reflect(AZ::ReflectContext* context);

        // RecastNavigationRequestBus interface implementation
        void UpdateNavigationMesh() override;
        AZStd::vector<AZ::Vector3> FindPathToEntity(AZ::EntityId fromEntity, AZ::EntityId toEntity) override;
        AZStd::vector<AZ::Vector3> FindPathToPosition(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition) override;
        dtNavMesh* GetNativeNavigationMap() const override { return m_navMesh.get(); }
        dtNavMeshQuery* GetNativeNavigationQuery() const override { return m_navQuery.get(); }

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

    private:
        AZ::ScheduledEvent m_tickEvent{ [this]() { OnTick(); }, AZ::Name("RecastNavigationDebugViewTick")};
        void OnTick();

        RecastNavigationMeshConfig m_meshConfig;
        bool m_showNavigationMesh = true;
    };

} // namespace RecastNavigation
