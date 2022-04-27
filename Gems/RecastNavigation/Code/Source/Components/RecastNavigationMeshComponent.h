/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "RecastNavigationDebugDraw.h"
#include "RecastNavigationMeshConfig.h"

#include <DetourNavMesh.h>
#include <Recast.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Components/RecastHelpers.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

namespace RecastNavigation
{
    //! Calculates a navigation mesh with the triangle data provided by @RecastNavigationSurveyorComponent.
    //! Provides APIs to find a path between two entities or two world positions.
    class RecastNavigationMeshComponent final
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public RecastNavigationMeshRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationMeshComponent, "{a281f314-a525-4c05-876d-17eb632f14b4}");

        RecastNavigationMeshComponent();
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // RecastNavigationRequestBus interface implementation
        void UpdateNavigationMesh() override;
        AZStd::vector<AZ::Vector3> FindPathToEntity(AZ::EntityId fromEntity, AZ::EntityId toEntity) override;
        AZStd::vector<AZ::Vector3> FindPathToPosition(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition) override;
        dtNavMesh* GetNativeNavigationMap() const override { return m_navMesh.get(); }
        dtNavMeshQuery* GetNativeNavigationQuery() const override { return m_navQuery.get(); }

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        AZ::Event<AZStd::shared_ptr<BoundedGeometry>>::Handler m_geometryCollectedEventHandler;
        void OnGeometryCollected(AZStd::shared_ptr<BoundedGeometry> boundedGeometry);

        AZStd::deque<AZStd::shared_ptr<BoundedGeometry>> m_pendingTiles;

        bool m_showNavigationMesh = true;

        static RecastVector3 GetPolyCenter(const dtNavMesh* navMesh, dtPolyRef ref);

        AZ::ScheduledEvent m_updateEvent;
        void OnNavigationMeshUpdated();

        AZStd::unique_ptr<AZ::TaskGraphEvent> m_taskGraphEvent;
        AZ::TaskGraph m_taskGraph;
        AZ::TaskDescriptor m_taskDescriptor{ "UpdateNavigationMesh", "RecastNavigation" };

        //! Does not require locking
        static NavigationTileData CreateNavigationTile(BoundedGeometry* geom,
            const RecastNavigationMeshConfig& meshConfig, rcContext* context);

        void StartProcessingTiles();
        void ProcessPendingTiles();

        //! Requires a lock when updating navigation mesh and query objects
        bool CreateNavigationMesh();
        bool AttachNavigationTileToMesh(NavigationTileData& navigationTileData);

        AZ::Aabb m_worldVolume = AZ::Aabb::CreateNull();

        AZStd::mutex m_navMeshMutex;
        AZStd::atomic<bool> m_waitingOnNavMeshRebuild = false;
        
        RecastNavigationMeshConfig m_meshConfig;
        RecastNavigationDebugDraw m_customDebugDraw;
        
        AZStd::unique_ptr<rcContext> m_context;
        RecastPointer<dtNavMesh> m_navMesh;
        RecastPointer<dtNavMeshQuery> m_navQuery;
    };

} // namespace RecastNavigation
