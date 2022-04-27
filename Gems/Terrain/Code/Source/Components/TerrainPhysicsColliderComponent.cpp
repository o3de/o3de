/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Components/TerrainPhysicsColliderComponent.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

AZ_DECLARE_BUDGET(Terrain);

namespace Terrain
{
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
                ->Version(1)
                ->Field("Surface", &TerrainPhysicsSurfaceMaterialMapping::m_surfaceTag)
                ->Field("Material", &TerrainPhysicsSurfaceMaterialMapping::m_materialId);
        }
    }


    AZ::Data::AssetId TerrainPhysicsSurfaceMaterialMapping::GetMaterialLibraryId()
    {
        if (const auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (const auto* physicsConfiguration = physicsSystem->GetConfiguration())
            {
                return physicsConfiguration->m_materialLibraryAsset.GetId();
            }
        }
        return {};
    }

    void TerrainPhysicsColliderConfig::Reflect(AZ::ReflectContext* context)
    {
        TerrainPhysicsSurfaceMaterialMapping::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainPhysicsColliderConfig, AZ::ComponentConfig>()
                ->Version(3)
                ->Field("DefaultMaterial", &TerrainPhysicsColliderConfig::m_defaultMaterialSelection)
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

    bool TerrainPhysicsColliderComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainPhysicsColliderConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainPhysicsColliderComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainPhysicsColliderConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainPhysicsColliderComponent::NotifyListenersOfHeightfieldDataChange(
        const Physics::HeightfieldProviderNotifications::HeightfieldChangeMask heightfieldChangeMask,
        const AZ::Aabb& dirtyRegion)
    {
        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        if (dirtyRegion.IsValid())
        {
            worldSize = dirtyRegion;
        }
        else
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        }

        Physics::HeightfieldProviderNotificationBus::Broadcast(
            &Physics::HeightfieldProviderNotificationBus::Events::OnHeightfieldDataChanged, worldSize, heightfieldChangeMask);
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
        NotifyListenersOfHeightfieldDataChange(
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::CreateEnd, AZ::Aabb::CreateNull());
    }

    void TerrainPhysicsColliderComponent::OnTerrainDataDestroyBegin()
    {
        m_terrainDataActive = false;

        // The terrain system is starting to destroy itself, so notify listeners of a change since the heightfield
        // will no longer have any valid data.
        NotifyListenersOfHeightfieldDataChange(
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::DestroyBegin, AZ::Aabb::CreateNull());
    }

    void TerrainPhysicsColliderComponent::OnTerrainDataChanged(
        const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if (m_terrainDataActive)
        {
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask physicsMask
                = TerrainToPhysicsHeightfieldChangeMask(dataChangedMask);

            NotifyListenersOfHeightfieldDataChange(physicsMask, dirtyRegion);
        }
    }

    AZ::Aabb TerrainPhysicsColliderComponent::GetHeightfieldAabb() const
    {
        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        return GetRegionClampedToGrid(worldSize);
    }

    AZ::Aabb TerrainPhysicsColliderComponent::GetRegionClampedToGrid(const AZ::Aabb& region) const
    {
        auto vector2Floor = [](const AZ::Vector2& in)
        {
            return AZ::Vector2(floor(in.GetX()), floor(in.GetY()));
        };
        auto vector2Ceil = [](const AZ::Vector2& in)
        {
            return AZ::Vector2(ceil(in.GetX()), ceil(in.GetY()));
        };

        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();
        const AZ::Vector3 boundsMin = region.GetMin();
        const AZ::Vector3 boundsMax = region.GetMax();

        const AZ::Vector2 gridMinBoundLower = vector2Floor(AZ::Vector2(boundsMin) / gridResolution) * gridResolution;
        const AZ::Vector2 gridMaxBoundUpper = vector2Ceil(AZ::Vector2(boundsMax) / gridResolution) * gridResolution;

        return AZ::Aabb::CreateFromMinMaxValues(
            gridMinBoundLower.GetX(), gridMinBoundLower.GetY(), boundsMin.GetZ(),
            gridMaxBoundUpper.GetX(), gridMaxBoundUpper.GetY(), boundsMax.GetZ()
        );
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldHeightBounds(float& minHeightBounds, float& maxHeightBounds) const
    {
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
        AZ::Vector3 translate;
        AZ::TransformBus::EventResult(translate, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        return AZ::Transform::CreateTranslation(translate);
    }

    void TerrainPhysicsColliderComponent::GenerateHeightsInBounds(AZStd::vector<float>& heights) const
    {
        AZ_PROFILE_FUNCTION(Terrain);

        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Aabb worldSize = GetHeightfieldAabb();

        const float worldCenterZ = worldSize.GetCenter().GetZ();

        int32_t gridWidth, gridHeight;
        GetHeightfieldGridSize(gridWidth, gridHeight);

        heights.clear();
        heights.reserve(gridWidth * gridHeight);

        auto perPositionHeightCallback = [&heights, worldCenterZ]
            ([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            heights.emplace_back(surfacePoint.m_position.GetZ() - worldCenterZ);
        };

        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(&AzFramework::Terrain::TerrainDataRequests::ProcessHeightsFromRegion,
            worldSize, gridResolution, perPositionHeightCallback, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT);
    }

    uint8_t TerrainPhysicsColliderComponent::GetMaterialIdIndex(const Physics::MaterialId& materialId, const AZStd::vector<Physics::MaterialId>& materialList) const
    {
        const auto& materialIter = AZStd::find(materialList.begin(), materialList.end(), materialId);
        if (materialIter != materialList.end())
        {
            return static_cast<uint8_t>(materialIter - materialList.begin());
        }

        return 0;
    }

    Physics::MaterialId TerrainPhysicsColliderComponent::FindMaterialIdForSurfaceTag(const SurfaceData::SurfaceTag tag) const
    {
        uint8_t index = 0;

        for (auto& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            if (mapping.m_surfaceTag == tag)
            {
                return mapping.m_materialId;
            }
            index++;
        }

        // If this surface isn't mapped, use the default material.
        return m_configuration.m_defaultMaterialSelection.GetMaterialId();
    }

    void TerrainPhysicsColliderComponent::UpdateHeightsAndMaterials(
        const Physics::UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback, const AZ::Aabb& regionIn) const
    {
        AZ_PROFILE_FUNCTION(Terrain);

        if (!m_terrainDataActive)
        {
            return;
        }

        AZ::Aabb region = regionIn;

        AZ::Aabb worldSize = GetHeightfieldAabb();
        if (!region.IsValid())
        {
            region = worldSize;
        }

        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        // Clamp region to world grid
        region = GetRegionClampedToGrid(region);

        size_t xOffset = 0, yOffset = 0;
        AZ::Aabb offsetRegion = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());

        if (region != worldSize)
        {
            const AZ::Vector3& worldSizeMin = worldSize.GetMin();
            const float worldMaxZ = worldSize.GetMax().GetZ();
            const AZ::Vector3& regionMin = region.GetMin();

            offsetRegion = AZ::Aabb::CreateFromMinMaxValues(worldSizeMin.GetX(),worldSizeMin.GetY(),worldSizeMin.GetZ(),regionMin.GetX(), regionMin.GetY(), worldMaxZ);

            AZStd::pair<size_t, size_t> numSamples;
            auto sampler = AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT;
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(numSamples, &AzFramework::Terrain::TerrainDataRequests::GetNumSamplesFromRegion, offsetRegion, gridResolution, sampler);

            xOffset = numSamples.first;
            yOffset = numSamples.second;
        }

        const float worldCenterZ = worldSize.GetCenter().GetZ();
        const float worldHeightBoundsMin = worldSize.GetMin().GetZ();
        const float worldHeightBoundsMax = worldSize.GetMax().GetZ();

        int32_t gridWidth, gridHeight;
        GetHeightfieldGridSize(gridWidth, gridHeight);

        AZStd::vector<Physics::MaterialId> materialList = GetMaterialList();

        auto perPositionCallback = [xOffset, yOffset, &updateHeightsMaterialsCallback, &materialList, this, worldCenterZ, worldHeightBoundsMin, worldHeightBoundsMax]
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
            Physics::MaterialId materialId = FindMaterialIdForSurfaceTag(surfaceWeight.m_surfaceType);
            point.m_materialIndex = GetMaterialIdIndex(materialId, materialList);

            int32_t column = aznumeric_cast<int32_t>(xOffset + xIndex);
            int32_t row = aznumeric_cast<int32_t>(yOffset + yIndex);
            updateHeightsMaterialsCallback(row, column, point);
        };

        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(&AzFramework::Terrain::TerrainDataRequests::ProcessSurfacePointsFromRegion,
            region, gridResolution, perPositionCallback, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT);
    }

    AZ::Vector2 TerrainPhysicsColliderComponent::GetHeightfieldGridSpacing() const
    {
        float gridResolution = 1.0f;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            gridResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        return AZ::Vector2(gridResolution);
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldGridSize(int32_t& numColumns, int32_t& numRows) const
    {
        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();
        const AZ::Aabb bounds = GetHeightfieldAabb();

        numColumns = aznumeric_cast<int32_t>((bounds.GetMax().GetX() - bounds.GetMin().GetX()) / gridResolution.GetX());
        numRows = aznumeric_cast<int32_t>((bounds.GetMax().GetY() - bounds.GetMin().GetY()) / gridResolution.GetY());
    }

    int32_t TerrainPhysicsColliderComponent::GetHeightfieldGridColumns() const
    {
        int32_t numColumns{ 0 };
        int32_t numRows{ 0 };

        GetHeightfieldGridSize(numColumns, numRows);
        return numColumns;
    }

    int32_t TerrainPhysicsColliderComponent::GetHeightfieldGridRows() const
    {
        int32_t numColumns{ 0 };
        int32_t numRows{ 0 };

        GetHeightfieldGridSize(numColumns, numRows);
        return numRows;
    }

    AZStd::vector<Physics::MaterialId> TerrainPhysicsColliderComponent::GetMaterialList() const
    {
        AZStd::vector<Physics::MaterialId> materialList;

        // Ensure the list contains the default material as the first entry.
        materialList.emplace_back(m_configuration.m_defaultMaterialSelection.GetMaterialId());

        for (auto& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            const auto& existingInstance = AZStd::find(materialList.begin(), materialList.end(), mapping.m_materialId);
            if (existingInstance == materialList.end())
            {
                materialList.emplace_back(mapping.m_materialId);
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
        int32_t gridWidth = 0, gridHeight = 0;
        GetHeightfieldGridSize(gridWidth, gridHeight);
        AZ_Assert(gridWidth * gridHeight != 0, "GetHeightsAndMaterials: Invalid grid size. Size cannot be zero.");

        AZStd::vector<Physics::HeightMaterialPoint> heightMaterials(gridWidth * gridHeight);
        UpdateHeightsAndMaterials([&heightMaterials, gridWidth](size_t row, size_t col, const Physics::HeightMaterialPoint& point)
        {
            heightMaterials[col + row * gridWidth] = point;
        });

        return heightMaterials;
    }
}
