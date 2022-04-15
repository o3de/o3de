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
#include <AzCore/Math/Aabb.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Components/RecastHelpers.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

namespace RecastNavigation
{
    class RecastNavigationMeshComponent final
        : public AZ::Component
        , protected RecastNavigationMeshRequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationMeshComponent, "{a281f314-a525-4c05-876d-17eb632f14b4}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // RecastNavigationRequestBus interface implementation
        bool UpdateNavigationMesh() override;
        AZStd::vector<AZ::Vector3> FindPathToEntity(AZ::EntityId fromEntity, AZ::EntityId toEntity) override;
        AZStd::vector<AZ::Vector3> FindPathToPosition(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition) override;
        
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:

        static RecastVector3 GetPolyCenter(const dtNavMesh* navMesh, dtPolyRef ref);

        bool UpdateNavigationMesh_JobThread(const AZ::Aabb& aabb);
        AZStd::atomic<bool> m_navMeshReady = false;
        bool m_waitingOnNavMeshRebuild = false;
        
        Geometry m_geom;

        AZStd::unique_ptr<rcContext> m_context;

        RecastNavigationMeshConfig m_meshConfig;

        RecastNavigationDebugDraw m_customDebugDraw;
        
        rcConfig m_config = {};
        AZStd::vector<AZ::u8> m_trianglesAreas;
        RecastPointer<rcHeightfield> m_solid;
        RecastPointer<rcCompactHeightfield> m_chf;
        RecastPointer<rcContourSet> m_contourSet;
        RecastPointer<rcPolyMesh> m_pmesh;
        RecastPointer<rcPolyMeshDetail> m_detailMesh;
        RecastPointer<dtNavMesh> m_navMesh;
        RecastPointer<dtNavMeshQuery> m_navQuery;
    };

} // namespace RecastNavigation
