/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationMeshComponent.h"

#include <DetourDebugDraw.h>
#include <DetourNavMeshBuilder.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

AZ_CVAR(
    bool, cl_navmesh_debug, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draw debug visual information about Navigation Mesh");

#pragma optimize("", off)

namespace RecastNavigation
{
    void RecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        RecastNavigationMeshConfig::Reflect(context);

        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationMeshComponent, AZ::Component>()
                ->Field("Config", &RecastNavigationMeshComponent::m_meshConfig)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<RecastNavigationMeshComponent>("Recast Navigation Mesh", "[Calculates the walkable navigation mesh]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RecastNavigationMeshRequestBus>("RecastNavigationMeshRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Navigation")
                ->Event("UpdateNavigationMesh", &RecastNavigationMeshRequests::UpdateNavigationMesh)
                ;

            behaviorContext->Class<RecastNavigationMeshComponent>()->RequestBus("RecastNavigationMeshRequestBus");
        }
    }

    void RecastNavigationMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void RecastNavigationMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void RecastNavigationMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    RecastVector3 RecastNavigationMeshComponent::GetPolyCenter(const dtNavMesh* navMesh, dtPolyRef ref)
    {
        RecastVector3 centerRecast;

        float center[3];
        center[0] = 0;
        center[1] = 0;
        center[2] = 0;

        const dtMeshTile* tile = nullptr;
        const dtPoly* poly = nullptr;
        const dtStatus status = navMesh->getTileAndPolyByRef(ref, &tile, &poly);
        if (dtStatusFailed(status))
        {
            return {};
        }

        if (poly->vertCount == 0)
        {
            return {};
        }

        for (int i = 0; i < aznumeric_cast<int>(poly->vertCount); ++i)
        {
            const float* v = &tile->verts[poly->verts[i] * 3];
            center[0] += v[0];
            center[1] += v[1];
            center[2] += v[2];
        }
        const float s = 1.0f / aznumeric_cast<float>(poly->vertCount);
        center[0] *= s;
        center[1] *= s;
        center[2] *= s;

        centerRecast.m_x = center[0];
        centerRecast.m_y = center[1];
        centerRecast.m_z = center[2];

        return centerRecast;
    }

    bool RecastNavigationMeshComponent::UpdateNavigationMesh()
    {
        if (m_waitingOnNavMeshRebuild)
        {
            return false;
        }

        m_navMeshReady = false;
        m_waitingOnNavMeshRebuild = true;

        m_geom.clear();
        RecastNavigationSurveyorRequestBus::Event(GetEntityId(), &RecastNavigationSurveyorRequestBus::Events::CollectGeometry, m_geom);

        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        RecastNavigationSurveyorRequestBus::EventResult(worldBounds, GetEntityId(), &RecastNavigationSurveyorRequestBus::Events::GetWorldBounds);

        auto* job = AZ::CreateJobFunction([this, worldBounds]()
            {
                if (UpdateNavigationMesh_JobThread(worldBounds))
                {
                    m_navMeshReady = true;
                    RecastNavigationMeshNotificationBus::Broadcast(&RecastNavigationMeshNotificationBus::Events::OnNavigationMeshUpdated);
                }
            }
        , true);

        job->Start();

        return true;
    }

    bool RecastNavigationMeshComponent::UpdateNavigationMesh_JobThread(const AZ::Aabb& aabb)
    {
        if (m_geom.m_verts.empty())
        {
            return true;
        }

        const float* vertices = &m_geom.m_verts.front().m_x;
        const int vertexCount = static_cast<int>(m_geom.m_verts.size());
        const int* triangleData = &m_geom.m_indices[0];
        const int triangleCount = static_cast<int>(m_geom.m_indices.size()) / 3;

        //
        // Step 1. Initialize build config.
        //

        // Init build configuration from GUI
        memset(&m_config, 0, sizeof(m_config));
        m_config.cs = m_meshConfig.m_cellSize;
        m_config.ch = m_meshConfig.m_cellHeight;
        m_config.walkableSlopeAngle = m_meshConfig.m_agentMaxSlope;
        m_config.walkableHeight = static_cast<int>(ceilf(m_meshConfig.m_agentHeight / m_config.ch));
        m_config.walkableClimb = static_cast<int>(floorf(m_meshConfig.m_agentMaxClimb / m_config.ch));
        m_config.walkableRadius = static_cast<int>(ceilf(m_meshConfig.m_agentRadius / m_config.cs));
        m_config.maxEdgeLen = static_cast<int>(m_meshConfig.m_edgeMaxLen / m_meshConfig.m_cellSize);
        m_config.maxSimplificationError = m_meshConfig.m_edgeMaxError;
        m_config.minRegionArea = static_cast<int>(rcSqr(m_meshConfig.m_regionMinSize));		// Note: area = size*size
        m_config.mergeRegionArea = static_cast<int>(rcSqr(m_meshConfig.m_regionMergeSize));	// Note: area = size*size
        m_config.maxVertsPerPoly = static_cast<int>(m_meshConfig.m_maxVertsPerPoly);
        m_config.detailSampleDist = m_meshConfig.m_detailSampleDist < 0.9f ? 0 : m_meshConfig.m_cellSize * m_meshConfig.m_detailSampleDist;
        m_config.detailSampleMaxError = m_meshConfig.m_cellHeight * m_meshConfig.m_detailSampleMaxError;

        // Set the area where the navigation will be build.
        // Here the bounds of the input mesh are used, but the
        // area could be specified by an user defined box, etc.

        const RecastVector3 worldMin(aabb.GetMin());
        const RecastVector3 worldMax(aabb.GetMax());

        rcVcopy(m_config.bmin, &worldMin.m_x);
        rcVcopy(m_config.bmax, &worldMax.m_x);
        rcCalcGridSize(m_config.bmin, m_config.bmax, m_config.cs, &m_config.width, &m_config.height);

        // Reset build times gathering.
        m_context->resetTimers();

        // Start the build process.
        m_context->startTimer(RC_TIMER_TOTAL);

        m_context->log(RC_LOG_PROGRESS, "Building navigation:");
        m_context->log(RC_LOG_PROGRESS, " - %d x %d cells", m_config.width, m_config.height);
        m_context->log(RC_LOG_PROGRESS, " - %d verts, %d triangles", vertexCount, triangleCount);

        //
        // Step 2. Rasterize input polygon soup.
        //

        // Allocate voxel height field where we rasterize our input data to.
        m_solid.reset(rcAllocHeightfield());
        if (!m_solid)
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
            return false;
        }
        if (!rcCreateHeightfield(m_context.get(), *m_solid, m_config.width, m_config.height,
            m_config.bmin, m_config.bmax, m_config.cs, m_config.ch))
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Could not create solid height field.");
            return false;
        }

        // Allocate array that can hold triangle area types.
        // If you have multiple meshes you need to process, allocate
        // and array which can hold the max number of triangles you need to process.
        m_trianglesAreas.resize(triangleCount, 0);

        // Find triangles which are walkable based on their slope and rasterize them.
        // If your input data is multiple meshes, you can transform them here, calculate
        // the are type for each of the meshes and rasterize them.
        rcMarkWalkableTriangles(m_context.get(), m_config.walkableSlopeAngle, vertices,
            vertexCount, triangleData, triangleCount, m_trianglesAreas.data());
        if (!rcRasterizeTriangles(m_context.get(), vertices, vertexCount, triangleData,
            m_trianglesAreas.data(), triangleCount, *m_solid /*, m_config.walkableClimb*/))
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Could not rasterize triangles.");
            return false;
        }

        m_trianglesAreas.clear();

        //
        // Step 3. Filter walkable surfaces.
        //

        // Once all geometry is rasterized, we do initial pass of filtering to
        // remove unwanted overhangs caused by the conservative rasterization
        // as well as filter spans where the character cannot possibly stand.
        if (m_meshConfig.m_filterLowHangingObstacles)
            rcFilterLowHangingWalkableObstacles(m_context.get(), m_config.walkableClimb, *m_solid);
        if (m_meshConfig.m_filterLedgeSpans)
            rcFilterLedgeSpans(m_context.get(), m_config.walkableHeight, m_config.walkableClimb, *m_solid);
        if (m_meshConfig.m_filterWalkableLowHeightSpans)
            rcFilterWalkableLowHeightSpans(m_context.get(), m_config.walkableHeight, *m_solid);

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(1.F, 0, 0, 0.1F));
        //    duDebugDrawHeightfieldSolid(&m_customDebugDraw, *m_solid);
        //}

        //
        // Step 4. Partition walkable surface to simple regions.
        //

        // Compact the height field so that it is faster to handle from now on.
        // This will result more cache coherent data as well as the neighbors
        // between walkable cells will be calculated.
        m_chf.reset(rcAllocCompactHeightfield());
        if (!m_chf)
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
            return false;
        }
        if (!rcBuildCompactHeightfield(m_context.get(), m_config.walkableHeight, m_config.walkableClimb, *m_solid, *m_chf))
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
            return false;
        }

        m_solid.reset();

        // Erode the walkable area by agent radius.
        if (!rcErodeWalkableArea(m_context.get(), m_config.walkableRadius, *m_chf))
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
            return false;
        }

        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distance field.
        if (!rcBuildRegionsMonotone(m_context.get(), *m_chf, 0, m_config.minRegionArea, m_config.mergeRegionArea))
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
            return false;
        }

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(0.F, 1, 0, 1));
        //    duDebugDrawCompactHeightfieldSolid(&m_customDebugDraw, *m_chf);
        //}

        //
        // Step 5. Trace and simplify region contours.
        //

        // Create contours.
        m_contourSet.reset(rcAllocContourSet());
        if (!m_contourSet)
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Out of memory while allocating contours.");
            return false;
        }
        if (!rcBuildContours(m_context.get(), *m_chf, m_config.maxSimplificationError,
            m_config.maxEdgeLen, *m_contourSet))
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
            return false;
        }

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(0.F, 0, 1, 1));
        //    duDebugDrawContours(&m_customDebugDraw, *m_contourSet);
        //}

        //
        // Step 6. Build polygons mesh from contours.
        //

        // Build polygon navmesh from the contours.
        m_pmesh.reset(rcAllocPolyMesh());
        if (!m_pmesh)
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
            return false;
        }
        if (!rcBuildPolyMesh(m_context.get(), *m_contourSet, m_config.maxVertsPerPoly, *m_pmesh))
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
            return false;
        }

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(0.F, 1, 1, 1));
        //    duDebugDrawPolyMesh(&m_customDebugDraw, *m_pmesh);
        //}

        //
        // Step 7. Create detail mesh which allows to access approximate height on each polygon.
        //

        m_detailMesh.reset(rcAllocPolyMeshDetail());
        if (!m_detailMesh)
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Out of memory while allocating detail mesh.");
            return false;
        }

        if (!rcBuildPolyMeshDetail(m_context.get(), *m_pmesh, *m_chf, m_config.detailSampleDist, m_config.detailSampleMaxError, *m_detailMesh))
        {
            m_context->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
            return false;
        }

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(0.5F, 1, 1, 1));
        //    duDebugDrawPolyMeshDetail(&m_customDebugDraw, *m_detailMesh);
        //}

        m_chf = nullptr;
        m_contourSet = nullptr;

        // At this point the navigation mesh data is ready, you can access it from m_pmesh.
        // See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

        //
        // (Optional) Step 8. Create Detour data from Recast poly mesh.
        //

        // The GUI may allow more max points per polygon than Detour can handle.
        // Only build the detour navmesh if we do not exceed the limit.
        if (m_config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
        {
            unsigned char* navData = nullptr;
            int navDataSize = 0;

            // Update poly flags from areas.
            for (int i = 0; i < m_pmesh->npolys; ++i)
            {
                if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
                {
                    m_pmesh->flags[i] = RC_WALKABLE_AREA;
                }
            }

            dtNavMeshCreateParams params = {};
            params.verts = m_pmesh->verts;
            params.vertCount = m_pmesh->nverts;
            params.polys = m_pmesh->polys;
            params.polyAreas = m_pmesh->areas;
            params.polyFlags = m_pmesh->flags;
            params.polyCount = m_pmesh->npolys;
            params.nvp = m_pmesh->nvp;
            params.detailMeshes = m_detailMesh->meshes;
            params.detailVerts = m_detailMesh->verts;
            params.detailVertsCount = m_detailMesh->nverts;
            params.detailTris = m_detailMesh->tris;
            params.detailTriCount = m_detailMesh->ntris;

            params.offMeshConVerts = nullptr;
            params.offMeshConRad = nullptr;
            params.offMeshConDir = nullptr;
            params.offMeshConAreas = nullptr;
            params.offMeshConFlags = nullptr;
            params.offMeshConUserID = nullptr;
            params.offMeshConCount = 0;

            params.walkableHeight = m_meshConfig.m_agentHeight;
            params.walkableRadius = m_meshConfig.m_agentRadius;
            params.walkableClimb = m_meshConfig.m_agentMaxClimb;
            rcVcopy(params.bmin, m_pmesh->bmin);
            rcVcopy(params.bmax, m_pmesh->bmax);
            params.cs = m_config.cs;
            params.ch = m_config.ch;
            params.buildBvTree = true;

            if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
            {
                m_context->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
                return false;
            }

            m_navMesh.reset(dtAllocNavMesh());
            if (!m_navMesh)
            {
                dtFree(navData);
                m_context->log(RC_LOG_ERROR, "Could not create Detour navmesh");
                return false;
            }

            dtStatus status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
            if (dtStatusFailed(status))
            {
                dtFree(navData);
                m_context->log(RC_LOG_ERROR, "Could not init Detour navmesh");
                return false;
            }

            if (cl_navmesh_debug)
            {
                m_customDebugDraw.SetColor(AZ::Color(0.F, 0.9f, 0, 1));
                duDebugDrawNavMesh(&m_customDebugDraw, *m_navMesh, DU_DRAWNAVMESH_COLOR_TILES);
            }

            m_navQuery.reset(dtAllocNavMeshQuery());

            status = m_navQuery->init(m_navMesh.get(), 2048);
            if (dtStatusFailed(status))
            {
                m_context->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
                return false;
            }

            m_context->stopTimer(RC_TIMER_TOTAL);
        }

        return true;
    }

    AZStd::vector<AZ::Vector3> RecastNavigationMeshComponent::FindPathToEntity(AZ::EntityId fromEntity, AZ::EntityId toEntity)
    {
        if (m_navMeshReady)
        {
            if (fromEntity.IsValid() && toEntity.IsValid())
            {
                AZ::Vector3 start = AZ::Vector3::CreateZero(), end = AZ::Vector3::CreateZero();
                AZ::TransformBus::EventResult(start, fromEntity, &AZ::TransformBus::Events::GetWorldTranslation);
                AZ::TransformBus::EventResult(end, toEntity, &AZ::TransformBus::Events::GetWorldTranslation);

                return FindPathToPosition(start, end);
            }
        }

        return {};
    }

    AZStd::vector<AZ::Vector3> RecastNavigationMeshComponent::FindPathToPosition(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition)
    {
        if (m_navMeshReady == false)
        {
            return {};
        }

        AZStd::vector<AZ::Vector3> pathPoints;

        {
            RecastVector3 startRecast{ fromWorldPosition }, endRecast{ targetWorldPosition };
            constexpr float halfExtents[3] = { 1.F, 1., 1 };

            dtPolyRef startPoly = 0, endPoly = 0;

            RecastVector3 nearestStartPoint, nearestEndPoint;

            const dtQueryFilter filter;

            dtStatus result = m_navQuery->findNearestPoly(startRecast.data(), halfExtents, &filter, &startPoly, nearestStartPoint.data());
            if (result != DT_SUCCESS)
            {
                return {};
            }

            result = m_navQuery->findNearestPoly(endRecast.data(), halfExtents, &filter, &endPoly, nearestEndPoint.data());
            if (result != DT_SUCCESS)
            {
                return {};
            }

            constexpr int maxPathLength = 100;
            dtPolyRef path[maxPathLength] = {};
            int pathLength = 0;

            // find an approximate path
            m_navQuery->findPath(startPoly, endPoly, nearestStartPoint.data(), nearestEndPoint.data(), &filter, path, &pathLength, maxPathLength);

            //if (cl_navmesh_debug)
            //{
            //    AZ_Printf("NavMesh", "from %d to %d, findPath = %d", startPoly, endPoly, pathLength);
            //}

            AZStd::vector<RecastVector3> approximatePath;
            approximatePath.resize(pathLength);

            for (int pathIndex = 0; pathIndex < pathLength; ++pathIndex)
            {
                RecastVector3 center = GetPolyCenter(m_navMesh.get(), path[pathIndex]);
                approximatePath.push_back(center);

                //if (cl_navmesh_debug)
                //{
                //    AZ_Printf("NavMesh", "path %d = %s", pathIndex, AZ::ToString(center.AsVector3()).c_str());
                //}
            }

            constexpr int maxDetailedPathLength = 100;
            RecastVector3 detailedPath[maxDetailedPathLength] = {};
            AZ::u8 detailedPathFlags[maxDetailedPathLength] = {};
            dtPolyRef detailedPolyPathRefs[maxDetailedPathLength] = {};
            int detailedPathCount = 0;

            m_navQuery->findStraightPath(startRecast.data(), endRecast.data(), path, pathLength, detailedPath[0].data(), detailedPathFlags, detailedPolyPathRefs,
                &detailedPathCount, maxDetailedPathLength);

            if (cl_navmesh_debug)
            {
                //for (size_t pathIndex = 1; pathIndex < approximatePath.size(); ++pathIndex)
                //{
                //    DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequestBus::Events::DrawLineLocationToLocation,
                //        approximatePath[pathIndex - 1].AsVector3(), approximatePath[pathIndex].AsVector3(), AZ::Color(1.F, 0, 0, 1), 30.F);
                //}

                constexpr AZ::Crc32 ViewportId = AzFramework::g_defaultSceneEntityDebugDisplayId;
                AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
                AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, ViewportId);
                AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
                if (debugDisplay)
                {
                    for (size_t detailedIndex = 1; detailedIndex < approximatePath.size(); ++detailedIndex)
                    {
                        if (detailedPath[detailedIndex].AsVector3().IsZero())
                            break;

                        const AZ::Vector4 colorVector4 = AZ::Color(0.F, 1, 0, 1).GetAsVector4();

                        debugDisplay->DrawLine(
                            detailedPath[detailedIndex - 1].AsVector3(),
                            detailedPath[detailedIndex].AsVector3(), colorVector4, colorVector4);
                    }
                }
            }

            for (int i = 0; i < detailedPathCount; ++i)
            {
                pathPoints.push_back(detailedPath[i].AsVector3());
            }
        }

        return pathPoints;
    }

    void RecastNavigationMeshComponent::Activate()
    {
        m_context = AZStd::make_unique<RecastCustomContext>();

        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        RecastNavigationMeshRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();
    }

    void RecastNavigationMeshComponent::Deactivate()
    {
        m_navQuery = {};
        m_navMesh = {};

        RecastNavigationMeshRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void RecastNavigationMeshComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_waitingOnNavMeshRebuild)
        {
            if (m_navMeshReady)
            {
                m_waitingOnNavMeshRebuild = false;
                RecastNavigationMeshNotificationBus::Event(GetEntityId(), &RecastNavigationMeshNotificationBus::Events::OnNavigationMeshUpdated);
            }
        }
        else if (m_navMeshReady && cl_navmesh_debug)
        {
            m_customDebugDraw.SetColor(AZ::Color(0.F, 0.9f, 0, 1));
            duDebugDrawNavMesh(&m_customDebugDraw, *m_navMesh, DU_DRAWNAVMESH_COLOR_TILES);
        }
    }
} // namespace RecastNavigation

#pragma optimize("", on)
