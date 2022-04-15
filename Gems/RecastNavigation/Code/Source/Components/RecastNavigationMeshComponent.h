/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DebugDraw.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Components/RecastHelpers.h>
#include <Components/RecastSmartPointer.h>
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

        // RecastNavigationRequestBus interface implementation
        bool UpdateNavigationMesh() override;
        void SetWorldBounds(const AZ::Aabb& worldBounds) override;
        AZStd::vector<AZ::Vector3> FindPathToEntity(AZ::EntityId fromEntity, AZ::EntityId toEntity) override;
        AZStd::vector<AZ::Vector3> FindPathToPosition(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition) override;
        
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:

        bool UpdateNavigationMesh_JobThread();
        AZStd::atomic<bool> m_navMeshReady = false;
        bool m_waitingOnNavMeshRebuild = false;

        struct Geometry
        {
            AZStd::vector<RecastVector3> m_verts;
            AZStd::vector<AZ::s32> m_indices;

            void clear()
            {
                m_verts.clear();
                m_indices.clear();
            }
        };

        Geometry GetColliderGeometry(const AZ::Aabb& aabb, const AzPhysics::SceneQueryHits& overlapHits);

        AZ::Aabb m_worldBounds = AZ::Aabb::CreateNull();

        RecastNavigationMeshComponent::Geometry m_geom;

        class CustomContext final : public rcContext
        {
        public:
            void doLog(const rcLogCategory, const char* message, const int) override
            {
                AZ_Printf("NavMesh", "%s", message);
            }
        };

        AZStd::unique_ptr<rcContext> m_context;

        rcConfig m_config = {};

        float m_cellSize = 0.3F;
        float m_cellHeight = 0.2F;

        float m_agentMaxSlope = 45;
        float m_agentHeight = 2.0F;
        float m_agentRadius = 0.75F;
        float m_agentMaxClimb = 0.9F;

        float m_edgeMaxError = 1.3F;
        float m_edgeMaxLen = 12.0F;

        float m_maxVertsPerPoly = 6;
        float m_detailSampleDist = 6.0F;
        float m_detailSampleMaxError = 1.0F;

        float m_regionMinSize = 8;
        float m_regionMergeSize = 20;

        RecastPointer<rcHeightfield> m_solid;

        AZStd::vector<AZ::u8> m_trianglesAreas;

        bool m_keepInterResults = false;

        bool m_filterLowHangingObstacles = true;
        bool m_filterLedgeSpans = true;
        bool m_filterWalkableLowHeightSpans = true;

        RecastPointer<rcCompactHeightfield> m_chf;

        RecastPointer<rcContourSet> m_contourSet;

        RecastPointer<rcPolyMesh> m_pmesh;

        RecastPointer<rcPolyMeshDetail> m_detailMesh;

        RecastPointer<dtNavMesh> m_navMesh;
        RecastPointer<dtNavMeshQuery> m_navQuery;

        class CustomDebugDraw final : public duDebugDraw
        {
        public:
            ~CustomDebugDraw() override = default;

            void depthMask([[maybe_unused]] bool state) override {}

            void texture([[maybe_unused]] bool state) override {}

            void begin(duDebugDrawPrimitives prim, float size = 1.0f) override;

            void vertex(const float* pos, unsigned int color) override { AddVertex(pos[0], pos[1], pos[2], color); }

            void vertex(const float x, const float y, const float z, unsigned int color) override { AddVertex(x, y, z, color); }

            void vertex(const float* pos, unsigned int color, [[maybe_unused]] const float* uv) override { AddVertex(pos[0], pos[1], pos[2], color); }

            void vertex(const float x, const float y, const float z, unsigned int color, [[maybe_unused]] const float u, [[maybe_unused]] const float v) override { AddVertex(x, y, z, color); }

            void end() override;

            void SetColor(const AZ::Color& color) { m_currentColor = color; }

        protected:
            void AddVertex(float x, float y, float z, unsigned int color);

            AZ::Color m_currentColor{ 1.F, 1, 1, 1 };

            duDebugDrawPrimitives m_currentPrim = DU_DRAW_POINTS;

            AZStd::vector<AZStd::pair<AZ::Vector3, AZ::u32>> m_verticesToDraw;
        };

        CustomDebugDraw m_customDebugDraw;

        static RecastVector3 GetPolyCenter(const dtNavMesh* navMesh, dtPolyRef ref);
    };

} // namespace RecastNavigation
