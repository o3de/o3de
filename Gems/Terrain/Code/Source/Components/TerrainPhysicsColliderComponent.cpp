/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Components/TerrainPhysicsColliderComponent.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

AZ_DECLARE_BUDGET(Terrain);

namespace Terrain
{
    AZ_CVAR(int32_t, cl_terrainPhysicsColliderMaxJobs, AzFramework::Terrain::QueryAsyncParams::UseMaxJobs, nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The maximum number of jobs to use when updating a Terrain Physics Collider (-1 will use all available cores).");


    Physics::HeightfieldProviderNotifications::HeightfieldChangeMask TerrainToPhysicsHeightfieldChangeMask(AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask mask)
    {
        using AzFramework::Terrain::TerrainDataNotifications;
        using Physics::HeightfieldProviderNotifications;

        HeightfieldProviderNotifications::HeightfieldChangeMask result = HeightfieldProviderNotifications::HeightfieldChangeMask::None;

        if (mask & TerrainDataNotifications::Settings)
        {
            result |= HeightfieldProviderNotifications::HeightfieldChangeMask::Settings;
        }

        if (mask & TerrainDataNotifications::HeightData)
        {
            result |= HeightfieldProviderNotifications::HeightfieldChangeMask::HeightData;
        }

        if (mask & TerrainDataNotifications::SurfaceData)
        {
            result |= HeightfieldProviderNotifications::HeightfieldChangeMask::SurfaceData;
        }

        return result;
    }

    void TerrainPhysicsSurfaceMaterialMapping::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainPhysicsSurfaceMaterialMapping>()
                ->Version(3)
                ->Field("Surface", &TerrainPhysicsSurfaceMaterialMapping::m_surfaceTag)
                ->Field("MaterialAsset", &TerrainPhysicsSurfaceMaterialMapping::m_materialAsset)
                ->Field("Material", &TerrainPhysicsSurfaceMaterialMapping::m_legacyMaterialId)
            ;
        }
    }

    void TerrainPhysicsColliderConfig::Reflect(AZ::ReflectContext* context)
    {
        TerrainPhysicsSurfaceMaterialMapping::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainPhysicsColliderConfig>()
                ->Version(5)
                ->Field("DefaultMaterialAsset", &TerrainPhysicsColliderConfig::m_defaultMaterialAsset)
                ->Field("DefaultMaterial", &TerrainPhysicsColliderConfig::m_legacyDefaultMaterialSelection)
                ->Field("Mappings", &TerrainPhysicsColliderConfig::m_surfaceMaterialMappings)
            ;
        }
    }

    void TerrainPhysicsColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void TerrainPhysicsColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void TerrainPhysicsColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void TerrainPhysicsColliderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        // If any of the following appear on the same entity as this one, they should get activated first as their data will
        // affect this component.
        services.push_back(AZ_CRC_CE("TerrainAreaService"));
        services.push_back(AZ_CRC_CE("TerrainHeightProviderService"));
        services.push_back(AZ_CRC_CE("TerrainSurfaceProviderService"));
    }

    void TerrainPhysicsColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainPhysicsColliderConfig::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainPhysicsColliderComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainPhysicsColliderComponent::m_configuration)
            ;
        }
    }

    TerrainPhysicsColliderComponent::TerrainPhysicsColliderComponent(const TerrainPhysicsColliderConfig& configuration)
        : m_configuration(configuration)
    {
    }

    TerrainPhysicsColliderComponent::TerrainPhysicsColliderComponent()
    {

    }

    void TerrainPhysicsColliderComponent::Activate()
    {
        // Build a mapping of surface tags to material indices for quick lookups when building/refreshing the collider.
        BuildSurfaceTagToMaterialIndexLookup();

        const auto entityId = GetEntityId();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        Physics::HeightfieldProviderRequestsBus::Handler::BusConnect(entityId);
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
    }

    void TerrainPhysicsColliderComponent::Deactivate()
    {
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        Physics::HeightfieldProviderRequestsBus::Handler ::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
    }

    void TerrainPhysicsColliderComponent::NotifyListenersOfHeightfieldDataChange(
        const Physics::HeightfieldProviderNotifications::HeightfieldChangeMask heightfieldChangeMask,
        const AZ::Aabb& dirtyRegion)
    {
        AZ_PROFILE_FUNCTION(Terrain);

        CalculateHeightfieldRegion();

        AZ::Aabb colliderBounds = GetHeightfieldAabb();

        if (dirtyRegion.IsValid())
        {
            // If we have a dirty region, only update this collider if the dirty region overlaps the collider bounds.
            if (dirtyRegion.Overlaps(colliderBounds))
            {
                // Find the intersection of the dirty region and the collider, and only notify about that area as changing.
                AZ::Aabb dirtyBounds = colliderBounds.GetClamped(dirtyRegion);

                Physics::HeightfieldProviderNotificationBus::Event(
                    GetEntityId(),
                    &Physics::HeightfieldProviderNotificationBus::Events::OnHeightfieldDataChanged, dirtyBounds, heightfieldChangeMask);
            }
        }
        else
        {
            // No valid dirty region, so update the entire collider bounds.
            Physics::HeightfieldProviderNotificationBus::Event(
                GetEntityId(),
                &Physics::HeightfieldProviderNotificationBus::Events::OnHeightfieldDataChanged, colliderBounds, heightfieldChangeMask);
        }
    }

    void TerrainPhysicsColliderComponent::OnShapeChanged([[maybe_unused]] ShapeChangeReasons changeReason)
    {
        // This will notify us of both shape changes and transform changes.
        // It's important to use this event for transform changes instead of listening to OnTransformChanged, because we need to guarantee
        // the shape has received the transform change message and updated its internal state before passing it along to us.
        Physics::HeightfieldProviderNotifications::HeightfieldChangeMask changeMask =
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings |
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::HeightData;

        NotifyListenersOfHeightfieldDataChange(changeMask, AZ::Aabb::CreateNull());
    }

    void TerrainPhysicsColliderComponent::OnTerrainDataCreateEnd()
    {
        m_terrainDataActive = true;

        // The terrain system has finished creating itself, so we should now have data for creating a heightfield.
        // Notify this as a 'settings' change because the heightfield has changed activation status.
        NotifyListenersOfHeightfieldDataChange(
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings, AZ::Aabb::CreateNull());
    }

    void TerrainPhysicsColliderComponent::OnTerrainDataDestroyBegin()
    {
        m_terrainDataActive = false;

        // The terrain system is starting to destroy itself, so notify listeners of a change since the heightfield
        // will no longer have any valid data.
        // Notify this as a 'settings' change because the heightfield has changed activation status.
        NotifyListenersOfHeightfieldDataChange(
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings, AZ::Aabb::CreateNull());
    }

    void TerrainPhysicsColliderComponent::OnTerrainDataChanged(
        const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if (m_terrainDataActive)
        {
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask physicsMask =
                TerrainToPhysicsHeightfieldChangeMask(dataChangedMask);

            NotifyListenersOfHeightfieldDataChange(physicsMask, dirtyRegion);
        }
    }

    void TerrainPhysicsColliderComponent::CalculateHeightfieldRegion()
    {
        if (!m_terrainDataActive)
        {
            AZStd::unique_lock lock(m_stateMutex);
            m_heightfieldRegion = AzFramework::Terrain::TerrainQueryRegion();
            return;
        }

        AZ::Aabb heightfieldBox = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            heightfieldBox, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Vector2 constrictedAlignedStartPoint = (AZ::Vector2(heightfieldBox.GetMin()) / gridResolution).GetCeil() * gridResolution;
        AZ::Vector2 constrictedAlignedEndPoint = (AZ::Vector2(heightfieldBox.GetMax()) / gridResolution).GetFloor() * gridResolution;

        // The "+ 1.0" at the end is because we need to be sure to include the end points. (ex: start=1, end=4 should have 4 points)
        AZ::Vector2 numPoints = (constrictedAlignedEndPoint - constrictedAlignedStartPoint) / gridResolution + AZ::Vector2(1.0f);

        {
            AZStd::unique_lock lock(m_stateMutex);
            m_heightfieldRegion.m_startPoint =
                AZ::Vector3(constrictedAlignedStartPoint.GetX(), constrictedAlignedStartPoint.GetY(), heightfieldBox.GetMin().GetZ());
            m_heightfieldRegion.m_stepSize = gridResolution;
            m_heightfieldRegion.m_numPointsX = aznumeric_cast<size_t>(numPoints.GetX());
            m_heightfieldRegion.m_numPointsY = aznumeric_cast<size_t>(numPoints.GetY());
        }
    }

    AZ::Aabb TerrainPhysicsColliderComponent::GetHeightfieldAabb() const
    {
        if (!m_terrainDataActive)
        {
            return AZ::Aabb::CreateNull();
        }

        AZ::Aabb heightfieldBox = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            heightfieldBox, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        {
            AZStd::shared_lock lock(m_stateMutex);
            AZ::Vector3 endPoint = m_heightfieldRegion.m_startPoint +
                AZ::Vector3(m_heightfieldRegion.m_stepSize.GetX() * (m_heightfieldRegion.m_numPointsX - 1),
                            m_heightfieldRegion.m_stepSize.GetY() * (m_heightfieldRegion.m_numPointsY - 1), heightfieldBox.GetZExtent());
            return AZ::Aabb::CreateFromMinMax(m_heightfieldRegion.m_startPoint, endPoint);
        }
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldHeightBounds(float& minHeightBounds, float& maxHeightBounds) const
    {
        if (!m_terrainDataActive)
        {
            minHeightBounds = 0.0f;
            maxHeightBounds = 0.0f;
            return;
        }

        const AZ::Aabb heightfieldAabb = GetHeightfieldAabb();

        // Because our terrain heights are relative to the center of the bounding box, the min and max allowable heights are also
        // relative to the center.  They are also clamped to the size of the bounding box.
        maxHeightBounds = heightfieldAabb.GetZExtent() / 2.0f;
        minHeightBounds = -maxHeightBounds;
    }

    float TerrainPhysicsColliderComponent::GetHeightfieldMinHeight() const
    {
        float minHeightBounds{ 0.0f };
        float maxHeightBounds{ 0.0f };
        GetHeightfieldHeightBounds(minHeightBounds, maxHeightBounds);
        return minHeightBounds;
    }

    float TerrainPhysicsColliderComponent::GetHeightfieldMaxHeight() const
    {
        float minHeightBounds{ 0.0f };
        float maxHeightBounds{ 0.0f };
        GetHeightfieldHeightBounds(minHeightBounds, maxHeightBounds);
        return maxHeightBounds;
    }

    AZ::Transform TerrainPhysicsColliderComponent::GetHeightfieldTransform() const
    {
        // We currently don't support rotation of terrain heightfields.
        // We also need to adjust the center to account for the fact that the heightfield might be expanded unevenly from
        // the entity's center, depending on where the entity's shape lies relative to the terrain grid.
        return AZ::Transform::CreateTranslation(GetHeightfieldAabb().GetCenter());
    }

    void TerrainPhysicsColliderComponent::GenerateHeightsInBounds(AZStd::vector<float>& heights) const
    {
        AZ_PROFILE_FUNCTION(Terrain);

        AzFramework::Terrain::TerrainQueryRegion queryRegion;

        {
            AZStd::shared_lock lock(m_stateMutex);
            queryRegion = m_heightfieldRegion;
        }


        heights.clear();
        heights.reserve(queryRegion.m_numPointsX * queryRegion.m_numPointsY);

        AZ::Aabb worldSize = GetHeightfieldAabb();
        const float worldCenterZ = worldSize.GetCenter().GetZ();

        auto perPositionHeightCallback = [&heights, worldCenterZ]
            ([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            heights.emplace_back(surfacePoint.m_position.GetZ() - worldCenterZ);
        };

        // We can use the "EXACT" sampler here because our query points are guaranteed to be aligned with terrain grid points.
        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequests::QueryRegion, queryRegion,
            AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights,
            perPositionHeightCallback, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);
    }

    uint8_t TerrainPhysicsColliderComponent::GetMaterialIndex(
        const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset,
        const AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>>& materialList) const
    {
        const auto& materialIter = AZStd::find(materialList.begin(), materialList.end(), materialAsset);
        if (materialIter != materialList.end())
        {
            return static_cast<uint8_t>(materialIter - materialList.begin());
        }

        return DefaultMaterialIndex;
    }

    AZ::Data::Asset<Physics::MaterialAsset> TerrainPhysicsColliderComponent::FindMaterialAssetForSurfaceTag(const SurfaceData::SurfaceTag tag) const
    {
        AZStd::shared_lock lock(m_stateMutex);

        for (auto& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            if (mapping.m_surfaceTag == tag)
            {
                return mapping.m_materialAsset;
            }
        }

        // If this surface isn't mapped, use the default material.
        return m_configuration.m_defaultMaterialAsset;
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldIndicesFromRegion(
        const AZ::Aabb& regionIn, size_t& startColumn, size_t& startRow, size_t& numColumns, size_t& numRows) const
    {
        if (!m_terrainDataActive)
        {
            startRow = 0;
            startColumn = 0;
            numRows = 0;
            numColumns = 0;
            return;
        }

        AZ::Aabb region = regionIn;

        AZ::Aabb worldSize = GetHeightfieldAabb();
        if (!region.IsValid())
        {
            region = worldSize;
        }
        else
        {
            region.Clamp(worldSize);
        }

        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        size_t xOffset, yOffset;

        {
            AZStd::shared_lock lock(m_stateMutex);

            // Convert the heightfield start point from world scale (1 = 1 meter) to terrain grid scale (1 = 1 terrain square)
            AZ::Vector2 heightfieldStartGridPoint = AZ::Vector2(m_heightfieldRegion.m_startPoint) / m_heightfieldRegion.m_stepSize;

            AZ::Vector2 contractedAlignedStartGridPoint = (AZ::Vector2(region.GetMin()) / gridResolution).GetCeil();
            AZ::Vector2 contractedAlignedEndGridPoint = (AZ::Vector2(region.GetMax()) / gridResolution).GetFloor();

            xOffset = aznumeric_cast<size_t>(contractedAlignedStartGridPoint.GetX() - heightfieldStartGridPoint.GetX());
            yOffset = aznumeric_cast<size_t>(contractedAlignedStartGridPoint.GetY() - heightfieldStartGridPoint.GetY());

            // The "+ 1.0" at the end is because we need to be sure to include the end points. (ex: start=1, end=4 should have 4 points)
            AZ::Vector2 numPoints = contractedAlignedEndGridPoint - contractedAlignedStartGridPoint + AZ::Vector2(1.0f);
            const size_t numPointsX = AZStd::min(aznumeric_cast<size_t>(numPoints.GetX()), m_heightfieldRegion.m_numPointsX);
            const size_t numPointsY = AZStd::min(aznumeric_cast<size_t>(numPoints.GetY()), m_heightfieldRegion.m_numPointsY);

            startColumn = xOffset;
            startRow = yOffset;
            numColumns = numPointsX;
            numRows = numPointsY;
        }
    }

    //! Updates the list of heights and materials within the region.
    void TerrainPhysicsColliderComponent::UpdateHeightsAndMaterialsAsync(
        const Physics::UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback,
        const Physics::UpdateHeightfieldCompleteFunction& updateHeightsCompleteCallback,
        size_t startColumn,
        size_t startRow,
        size_t numColumns,
        size_t numRows) const
    {
        using namespace AzFramework::Terrain;

        AZ_PROFILE_FUNCTION(Terrain);

        // Early out if there's no terrain data, or we aren't trying to update any points.
        if ((!m_terrainDataActive) || (numColumns == 0) || (numRows == 0))
        {
            updateHeightsCompleteCallback();
            return;
        }


        AZ::Aabb worldSize = GetHeightfieldAabb();
        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Vector2 startPoint =
            AZ::Vector2(worldSize.GetMin())
            + (AZ::Vector2(aznumeric_cast<float>(startColumn), aznumeric_cast<float>(startRow)) * gridResolution);

        TerrainQueryRegion queryRegion = TerrainQueryRegion(startPoint, numColumns, numRows, gridResolution);

        const float worldCenterZ = worldSize.GetCenter().GetZ();
        const float worldHeightBoundsMin = worldSize.GetMin().GetZ();
        const float worldHeightBoundsMax = worldSize.GetMax().GetZ();

        // Grab a local copy of the surface tag to material lookup to ensure that modifications on other threads
        // don't affect us while we're in the middle of the query.
        AZStd::unordered_map<SurfaceData::SurfaceTag, uint8_t> surfaceTagToMaterialIndexLookup;
        {
            AZStd::shared_lock lock(m_stateMutex);
            surfaceTagToMaterialIndexLookup = m_surfaceTagToMaterialIndexLookup;
        }

        // Everything is copied by value into the lambda because this is an async callback, so anything referenced by it needs to
        // continue to exist after the outer function completes.
        auto perPositionCallback =
            [startColumn, startRow, updateHeightsMaterialsCallback, surfaceTagToMaterialIndexLookup, worldCenterZ,
            worldHeightBoundsMin, worldHeightBoundsMax]
            (size_t xIndex, size_t yIndex, const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
        {
            float height = surfacePoint.m_position.GetZ();

            // Any heights that fall outside the range of our bounding box will get turned into holes.
            if ((height < worldHeightBoundsMin) || (height > worldHeightBoundsMax))
            {
                height = worldHeightBoundsMin;
                terrainExists = false;
            }

            // Find the best surface tag at this point.
            // We want the MaxSurfaceWeight. The ProcessSurfacePoints callback has surface weights sorted.
            // So, we pick the value at the front of the list.
            AzFramework::SurfaceData::SurfaceTagWeight surfaceWeight;
            if (!surfacePoint.m_surfaceTags.empty())
            {
                surfaceWeight = *surfacePoint.m_surfaceTags.begin();
            }

            Physics::HeightMaterialPoint point;
            point.m_height = height - worldCenterZ;
            point.m_quadMeshType = terrainExists ? Physics::QuadMeshType::SubdivideUpperLeftToBottomRight : Physics::QuadMeshType::Hole;

            // Get the material index for the surface type. If we can't find it, use the default material.
            if (const auto& entry = surfaceTagToMaterialIndexLookup.find(surfaceWeight.m_surfaceType);
                entry != surfaceTagToMaterialIndexLookup.end())
            {
                point.m_materialIndex = entry->second;
            }
            else
            {
                point.m_materialIndex = DefaultMaterialIndex;
            }

            size_t column = startColumn + xIndex;
            size_t row = startRow + yIndex;
            updateHeightsMaterialsCallback(column, row, point);
        };

        // Create an async query to update all of the height and material data so that we can spread the computation across
        // multiple threads and then call back a completion method at the end.

        AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;

        auto params = AZStd::make_shared<AzFramework::Terrain::QueryAsyncParams>();
        params->m_desiredNumberOfJobs = cl_terrainPhysicsColliderMaxJobs;
        params->m_completionCallback =
            [updateHeightsCompleteCallback]([[maybe_unused]] AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> context)
        {
            updateHeightsCompleteCallback();
        };

        // We can use the "EXACT" sampler here because our query points are guaranteed to be aligned with terrain grid points.
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryRegionAsync, queryRegion,
            static_cast<TerrainDataRequests::TerrainDataMask>(
                TerrainDataRequests::TerrainDataMask::Heights | TerrainDataRequests::TerrainDataMask::SurfaceData),
            perPositionCallback, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, params);

        // If the call to UpdateHeightsAndMaterials was made on a thread, and the TerrainSystem is currently shutting down on a different
        // thread, it's possible that the TerrainDataRequest bus won't have a listener at the moment we call it, which is why we need
        // to validate that the jobContext was returned successfully. If it wasn't, just call the completion callback immediately.
        if (!jobContext)
        {
            updateHeightsCompleteCallback();
        }
    }

    //! Updates the list of heights and materials within the region.
    void TerrainPhysicsColliderComponent::UpdateHeightsAndMaterials(
        const Physics::UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback,
        size_t startColumn,
        size_t startRow,
        size_t numColumns,
        size_t numRows) const
    {
        AZ_PROFILE_FUNCTION(Terrain);

        AZStd::binary_semaphore wait;
        auto completionCallback = [&wait]()
        {
            wait.release();
        };

        UpdateHeightsAndMaterialsAsync(updateHeightsMaterialsCallback, completionCallback, startColumn, startRow, numColumns, numRows);

        // Wait for the query to complete.
        wait.acquire();
    }

    void TerrainPhysicsColliderComponent::BuildSurfaceTagToMaterialIndexLookup()
    {
        auto materialList = GetMaterialList();

        // Lock this *after* calling GetMaterialList() so that we don't have nested locks.
        AZStd::unique_lock lock(m_stateMutex);

        m_surfaceTagToMaterialIndexLookup.clear();

        for (const auto& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            for (uint8_t materialIndex = 0; materialIndex < materialList.size(); materialIndex++)
            {
                if (mapping.m_materialAsset == materialList[materialIndex])
                {
                    m_surfaceTagToMaterialIndexLookup.emplace(mapping.m_surfaceTag, materialIndex);
                    break;
                }
            }
        }
    }

    void TerrainPhysicsColliderComponent::UpdateConfiguration(const TerrainPhysicsColliderConfig& newConfiguration)
    {
        {
            AZStd::unique_lock lock(m_stateMutex);
            m_configuration = newConfiguration;
        }

        // Build a mapping of surface tags to material indices for quick lookups when building/refreshing the collider.
        BuildSurfaceTagToMaterialIndexLookup();

        NotifyListenersOfHeightfieldDataChange(
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::SurfaceMapping, AZ::Aabb::CreateNull());
    }

    AZ::Vector2 TerrainPhysicsColliderComponent::GetHeightfieldGridSpacing() const
    {
        if (!m_terrainDataActive)
        {
            return AZ::Vector2(0.0f);
        }

        float gridResolution = 1.0f;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            gridResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        return AZ::Vector2(gridResolution);
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldGridSize(size_t& numColumns, size_t& numRows) const
    {
        AZStd::shared_lock lock(m_stateMutex);
        numColumns = m_heightfieldRegion.m_numPointsX;
        numRows = m_heightfieldRegion.m_numPointsY;
    }

    size_t TerrainPhysicsColliderComponent::GetHeightfieldGridColumns() const
    {
        AZStd::shared_lock lock(m_stateMutex);
        return m_heightfieldRegion.m_numPointsX;
    }

    size_t TerrainPhysicsColliderComponent::GetHeightfieldGridRows() const
    {
        AZStd::shared_lock lock(m_stateMutex);
        return m_heightfieldRegion.m_numPointsY;
    }

    AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>> TerrainPhysicsColliderComponent::GetMaterialList() const
    {
        AZStd::shared_lock lock(m_stateMutex);
        
        AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>> materialList;
        materialList.reserve(m_configuration.m_surfaceMaterialMappings.size() + 1); // +1 for default material asset

        // Ensure the list contains the default material as the first entry.
        materialList.push_back(m_configuration.m_defaultMaterialAsset);

        for (auto& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            const auto& existingInstance = AZStd::find(materialList.begin(), materialList.end(), mapping.m_materialAsset);
            if (existingInstance == materialList.end()) // Avoid having the same asset more than once
            {
                materialList.push_back(mapping.m_materialAsset);
            }
        }

        return materialList;
    }

    AZStd::vector<float> TerrainPhysicsColliderComponent::GetHeights() const
    {
        AZStd::vector<float> heights; 
        GenerateHeightsInBounds(heights);

        return heights;
    }

    AZStd::vector<Physics::HeightMaterialPoint> TerrainPhysicsColliderComponent::GetHeightsAndMaterials() const
    {
        size_t gridWidth = 0, gridHeight = 0;
        GetHeightfieldGridSize(gridWidth, gridHeight);
        AZ_Assert(gridWidth * gridHeight != 0, "GetHeightsAndMaterials: Invalid grid size. Size cannot be zero.");

        AZStd::vector<Physics::HeightMaterialPoint> heightMaterials(gridWidth * gridHeight);
        UpdateHeightsAndMaterials(
            [&heightMaterials, gridWidth](size_t col, size_t row, const Physics::HeightMaterialPoint& point)
            {
                heightMaterials[col + row * gridWidth] = point;
            },
            0, 0, gridWidth, gridHeight);

        return heightMaterials;
    }
}
