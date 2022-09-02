/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/Profiler.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <DebugDraw/DebugDrawBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Misc/RecastNavigationPhysXProviderComponentController.h>

AZ_CVAR(
    bool, cl_navmesh_showInputData, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draws triangle mesh input data that was used for the navigation mesh calculation");
AZ_CVAR(
    float, cl_navmesh_showInputDataSeconds, 30.f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, keeps the debug triangle mesh input for the specified number of seconds");
AZ_CVAR(
    AZ::u32, bg_navmesh_tileThreads, 4, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Number of threads to use to process tiles for each RecastNavigationPhysXProvider");

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    void RecastNavigationPhysXProviderComponentController::Reflect(AZ::ReflectContext* context)
    {
        RecastNavigationPhysXProviderConfig::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationPhysXProviderComponentController>()
                ->Field("Config", &RecastNavigationPhysXProviderComponentController::m_config)
                ->Version(1);
        }
    }

    void RecastNavigationPhysXProviderComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        // This can be used to depend on this specific component.
        provided.push_back(AZ_CRC_CE("RecastNavigationPhysXProviderComponentController"));
        // Or be able to satisfy requirements of @RecastNavigationMeshComponent, as one of geometry data providers for the navigation mesh.
        provided.push_back(AZ_CRC_CE("RecastNavigationProviderService"));
    }

    void RecastNavigationPhysXProviderComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationPhysXProviderComponentController"));
        incompatible.push_back(AZ_CRC_CE("RecastNavigationProviderService"));
    }

    void RecastNavigationPhysXProviderComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    RecastNavigationPhysXProviderComponentController::RecastNavigationPhysXProviderComponentController()
        : m_taskExecutor(bg_navmesh_tileThreads)
    {
    }

    RecastNavigationPhysXProviderComponentController::RecastNavigationPhysXProviderComponentController(
        const RecastNavigationPhysXProviderConfig& config)
        : m_config(config)
        , m_taskExecutor(bg_navmesh_tileThreads)
    {
    }

    void RecastNavigationPhysXProviderComponentController::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;
        m_shouldProcessTiles = true;
        m_updateInProgress = false;
        OnConfigurationChanged();
        RecastNavigationProviderRequestBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());
    }

    void RecastNavigationPhysXProviderComponentController::SetConfiguration(const RecastNavigationPhysXProviderConfig& config)
    {
        m_config = config;
    }

    const RecastNavigationPhysXProviderConfig& RecastNavigationPhysXProviderComponentController::GetConfiguration() const
    {
        return m_config;
    }

    void RecastNavigationPhysXProviderComponentController::Deactivate()
    {
        if (m_updateInProgress)
        {
            m_shouldProcessTiles = false;
            if (m_taskGraphEvent && m_taskGraphEvent->IsSignaled() == false)
            {
                // If the tasks are still in progress, wait until the task graph is finished.
                m_taskGraphEvent->Wait();
            }
        }

        m_updateInProgress = false;
        RecastNavigationProviderRequestBus::Handler::BusDisconnect();
        // The event is used to detect if tasks are already in progress.
        m_taskGraphEvent.reset();
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> RecastNavigationPhysXProviderComponentController::CollectGeometry(
        float tileSize, float borderSize)
    {
        // Blocking call.
        return CollectGeometryImpl(tileSize, borderSize, GetWorldBounds());
    }

    bool RecastNavigationPhysXProviderComponentController::CollectGeometryAsync(
        float tileSize,
        float borderSize,
        AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> tileCallback)
    {
        return CollectGeometryAsyncImpl(tileSize, borderSize, GetWorldBounds(), AZStd::move(tileCallback));
    }

    AZ::Aabb RecastNavigationPhysXProviderComponentController::GetWorldBounds() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, m_entityComponentIdPair.GetEntityId(),
            &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return worldBounds;
    }

    int RecastNavigationPhysXProviderComponentController::GetNumberOfTiles(float tileSize) const
    {
        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        const int tilesAlongX = aznumeric_cast<int>(AZStd::ceil(extents.GetX() / tileSize));
        const int tilesAlongY = aznumeric_cast<int>(AZStd::ceil(extents.GetY() / tileSize));

        return tilesAlongX * tilesAlongY;
    }

    const char* RecastNavigationPhysXProviderComponentController::GetSceneName() const
    {
        return m_config.m_useEditorScene ? AzPhysics::EditorPhysicsSceneName : AzPhysics::DefaultPhysicsSceneName;
    }

    void RecastNavigationPhysXProviderComponentController::OnConfigurationChanged()
    {
        m_collisionGroup = GetCollisionGroupById(m_config.m_collisionGroupId);
    }

    void RecastNavigationPhysXProviderComponentController::CollectCollidersWithinVolume(const AZ::Aabb& volume, QueryHits& overlapHits)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometryWithinVolume");

        AZ::Vector3 dimension = volume.GetExtents();
        AZ::Transform pose = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), volume.GetCenter());

        Physics::BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = dimension;

        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(dimension, pose, nullptr);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Static; // only looking for static PhysX colliders
        request.m_collisionGroup = m_collisionGroup;

        AzPhysics::SceneQuery::UnboundedOverlapHitCallback unboundedOverlapHitCallback =
            [&overlapHits](AZStd::optional<AzPhysics::SceneQueryHit>&& hit)
        {
            if (hit && ((hit->m_resultFlags & AzPhysics::SceneQuery::EntityId) != 0))
            {
                const AzPhysics::SceneQueryHit& sceneQueryHit = *hit;
                overlapHits.push_back(sceneQueryHit);
            }

            return true;
        };

        //! We need to use unbounded callback, otherwise the results will be limited to 32 or so objects.
        request.m_unboundedOverlapHitCallback = unboundedOverlapHitCallback;

        if (auto sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(GetSceneName());

            // Note: blocking call
            sceneInterface->QueryScene(sceneHandle, &request);
            // results are in overlapHits
        }
    }

    void RecastNavigationPhysXProviderComponentController::AppendColliderGeometry(
        TileGeometry& geometry,
        const QueryHits& overlapHits)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: AppendColliderGeometry");

        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;
        AZStd::size_t indicesCount = geometry.m_indices.size();

        AzPhysics::SceneInterface* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(GetSceneName());

        for (const auto& overlapHit : overlapHits)
        {
            AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, overlapHit.m_bodyHandle);
            if (!body)
            {
                continue;
            }

            // Create an AABB for the Recast tile in local space and pass it in to GetGeometry so that large geometry sets
            // (like heightfields) can just return the subset of geometry that overlaps the AABB.
            auto pose = overlapHit.m_shape->GetLocalPose();
            AZ::Aabb localScanBounds = geometry.m_scanBounds.GetTranslated(-pose.first);
            overlapHit.m_shape->GetGeometry(vertices, indices, &localScanBounds);

            // Note: returned geometry data is also in local space
            AZ::Transform tBody = AZ::Transform::CreateFromQuaternionAndTranslation(body->GetOrientation(), body->GetPosition());
            AZ::Transform t = tBody * AZ::Transform::CreateTranslation(pose.first);

            if (!vertices.empty())
            {
                if (indices.empty())
                {
                    // Some PhysX colliders (convex shapes) return geometry without indices. Build indices now.
                    AZStd::vector<AZ::Vector3> transformed;

                    int currentLocalIndex = 0;
                    for (const AZ::Vector3& vertex : vertices)
                    {
                        const AZ::Vector3 translated = t.TransformPoint(vertex);
                        geometry.m_vertices.push_back(RecastVector3::CreateFromVector3SwapYZ(translated));

                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + currentLocalIndex));
                        currentLocalIndex++;
                    }
                }
                else
                {
                    AZStd::vector<AZ::Vector3> transformed;

                    for (const AZ::Vector3& vertex : vertices)
                    {
                        const AZ::Vector3 translated = t.TransformPoint(vertex);
                        geometry.m_vertices.push_back(RecastVector3::CreateFromVector3SwapYZ(translated));
                    }

                    for (size_t i = 2; i < indices.size(); i += 3)
                    {
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i]));
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i - 1]));
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i - 2]));
                    }
                }

                indicesCount += vertices.size();
                vertices.clear();
                indices.clear();
            }
        }
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> RecastNavigationPhysXProviderComponentController::CollectGeometryImpl(
        float tileSize, float borderSize, const AZ::Aabb& worldVolume)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometry");

        bool notInProgress = false;
        if (!m_updateInProgress.compare_exchange_strong(notInProgress, true))
        {
            return {};
        }

        if (tileSize <= 0.f)
        {
            return {};
        }

        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        const AZ::Vector3 extents = worldVolume.GetExtents();
        int tilesAlongX = aznumeric_cast<int>(AZStd::ceil(extents.GetX() / tileSize));
        int tilesAlongY = aznumeric_cast<int>(AZStd::ceil(extents.GetY() / tileSize));

        const AZ::Vector3& worldMin = worldVolume.GetMin();
        const AZ::Vector3& worldMax = worldVolume.GetMax();

        const AZ::Vector3 border = AZ::Vector3::CreateOne() * borderSize;

        // Find all geometry one tile at a time.
        for (int y = 0; y < tilesAlongY; ++y)
        {
            for (int x = 0; x < tilesAlongX; ++x)
            {
                const AZ::Vector3 tileMin{
                    worldMin.GetX() + aznumeric_cast<float>(x) * tileSize,
                    worldMin.GetY() + aznumeric_cast<float>(y) * tileSize,
                    worldMin.GetZ()
                };

                const AZ::Vector3 tileMax{
                    worldMin.GetX() + aznumeric_cast<float>(x + 1) * tileSize,
                    worldMin.GetY() + aznumeric_cast<float>(y + 1) * tileSize,
                    worldMax.GetZ()
                };

                // Recast wants extra triangle data around each tile, so that each tile can connect to each other.
                AZ::Aabb tileVolume = AZ::Aabb::CreateFromMinMax(tileMin, tileMax);
                AZ::Aabb scanVolume = AZ::Aabb::CreateFromMinMax(tileMin - border, tileMax + border);

                QueryHits results;
                CollectCollidersWithinVolume(scanVolume, results);

                AZStd::shared_ptr<TileGeometry> geometryData = AZStd::make_unique<TileGeometry>();
                geometryData->m_worldBounds = tileVolume;
                geometryData->m_scanBounds = scanVolume;
                AppendColliderGeometry(*geometryData, results);

                geometryData->m_tileX = x;
                geometryData->m_tileY = y;
                tiles.push_back(geometryData);
            }
        }

        m_updateInProgress = false;
        return tiles;
    }

    // Adjust the origin, so that any tile over-extension is even across all sides.
    // Note, navigation mesh is made up of square tiles. Recast does not support uneven tiles,
    // so the best we can do is even them out. Additionally, users can set their own tile size on @RecastNavigationMeshComponent.
    AZ::Vector3 GetAdjustedOriginBasedOnTileSize(const AZ::Aabb& worldVolume, float tileSize)
    {
        if (tileSize <= 0.f)
        {
            AZ_Warning("Recast Navigation", true, "Tile size is invalid. It should be a positive number.");
            return AZ::Vector3::CreateZero();
        }

        AZ::Vector3 origin = worldVolume.GetMin();
        const AZ::Vector3& extents = worldVolume.GetExtents();

        const float tileOverExtensionOnX = AZStd::ceil(extents.GetX() / tileSize) * tileSize - extents.GetX();
        origin.SetX(origin.GetX() - tileOverExtensionOnX / 2.f);

        const float tileOverExtensionOnY = AZStd::ceil(extents.GetY() / tileSize) * tileSize - extents.GetY();
        origin.SetY(origin.GetY() - tileOverExtensionOnY / 2.f);

        return origin;
    }

    bool RecastNavigationPhysXProviderComponentController::CollectGeometryAsyncImpl(
        float tileSize,
        float borderSize,
        const AZ::Aabb& worldVolume,
        AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> tileCallback)
    {
        bool notInProgress = false;
        if (!m_updateInProgress.compare_exchange_strong(notInProgress, true))
        {
            return false;
        }

        if (!m_taskGraphEvent || m_taskGraphEvent->IsSignaled())
        {
            AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometryAsync");

            m_taskGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>("RecastNavigation PhysX Wait");
            m_taskGraph.Reset();

            AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

            const AZ::Vector3 extents = worldVolume.GetExtents();
            int tilesAlongX = aznumeric_cast<int>(AZStd::ceil(extents.GetX() / tileSize));
            int tilesAlongY = aznumeric_cast<int>(AZStd::ceil(extents.GetY() / tileSize));
            
            const AZ::Vector3 worldOrigin = GetAdjustedOriginBasedOnTileSize(worldVolume, tileSize);
            const AZ::Vector3 worldMax(
                worldOrigin.GetX() + tileSize * aznumeric_cast<float>(tilesAlongX),
                worldOrigin.GetY() + tileSize * aznumeric_cast<float>(tilesAlongY),
                worldVolume.GetMax().GetZ());

            const AZ::Vector3 border = AZ::Vector3::CreateOne() * borderSize;

            AZStd::vector<AZ::TaskToken> tileTaskTokens;

            // Create tasks for each tile and a finish task.
            for (int y = 0; y < tilesAlongY; ++y)
            {
                for (int x = 0; x < tilesAlongX; ++x)
                {
                    const AZ::Vector3 tileMin{
                        worldOrigin.GetX() + aznumeric_cast<float>(x) * tileSize,
                        worldOrigin.GetY() + aznumeric_cast<float>(y) * tileSize,
                        worldOrigin.GetZ()
                    };

                    const AZ::Vector3 tileMax{
                        worldOrigin.GetX() + aznumeric_cast<float>(x + 1) * tileSize,
                        worldOrigin.GetY() + aznumeric_cast<float>(y + 1) * tileSize,
                        worldMax.GetZ()
                    };

                    AZ::Aabb tileVolume = AZ::Aabb::CreateFromMinMax(tileMin, tileMax);
                    AZ::Aabb scanVolume = AZ::Aabb::CreateFromMinMax(tileMin - border, tileMax + border);
                    AZStd::shared_ptr<TileGeometry> geometryData = AZStd::make_unique<TileGeometry>();
                    geometryData->m_tileCallback = tileCallback;
                    geometryData->m_worldBounds = tileVolume;
                    geometryData->m_scanBounds = scanVolume;
                    geometryData->m_tileX = x;
                    geometryData->m_tileY = y;

                    AZ::TaskToken token = m_taskGraph.AddTask(
                        m_taskDescriptor, [this, geometryData]()
                        {
                            if (m_shouldProcessTiles)
                            {
                                AZ_PROFILE_SCOPE(Navigation, "Navigation: collecting geometry for a tile");
                                QueryHits results;
                                CollectCollidersWithinVolume(geometryData->m_scanBounds, results);
                                AppendColliderGeometry(*geometryData, results);
                                geometryData->m_tileCallback(geometryData);
                            }
                        });

                    tileTaskTokens.push_back(AZStd::move(token));
                }
            }

            AZ::TaskToken finishToken = m_taskGraph.AddTask(
                m_taskDescriptor, [this, tileCallback]()
                {
                    tileCallback({}); // Notifies the caller that the operation is done.
                    m_updateInProgress = false;
                });

            for (AZ::TaskToken& task : tileTaskTokens)
            {
                task.Precedes(finishToken);
            }

            AZ_Assert(m_taskGraphEvent->IsSignaled() == false, "RecastNavigationPhysXProviderComponentController might be runtime two async gather operations, which is not supported.");
            m_taskGraph.SubmitOnExecutor(m_taskExecutor, m_taskGraphEvent.get());
            return true;
        }

        return false;
    }
} // namespace RecastNavigation
