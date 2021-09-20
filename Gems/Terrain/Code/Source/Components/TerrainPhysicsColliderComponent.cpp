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
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace Terrain
{
    void TerrainPhysicsColliderConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainPhysicsColliderConfig, AZ::ComponentConfig>()
                ->Version(1)
            ;

            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<TerrainPhysicsColliderConfig>(
                        "Terrain Physics Collider Component",
                        "Provides terrain data to a physics collider with configurable surface mappings.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
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
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        Physics::HeightfieldProviderRequestsBus::Handler::BusConnect(entityId);

        NotifyListenersOfHeightfieldDataChange();
    }

    void TerrainPhysicsColliderComponent::Deactivate()
    {
        Physics::HeightfieldProviderRequestsBus::Handler ::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
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

    void TerrainPhysicsColliderComponent::NotifyListenersOfHeightfieldDataChange()
    {
        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        Physics::HeightfieldProviderNotificationBus::Broadcast(
            &Physics::HeightfieldProviderNotificationBus::Events::OnHeightfieldDataChanged, worldSize);
    }

    void TerrainPhysicsColliderComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        NotifyListenersOfHeightfieldDataChange();
    }

    void TerrainPhysicsColliderComponent::OnShapeChanged([[maybe_unused]] ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::TransformChanged)
        {
            // This will be handled by the OnTransformChanged handler.
            return;
        }

        NotifyListenersOfHeightfieldDataChange();
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldBounds(const AZ::Aabb& bounds, AZ::Vector3& minBounds, AZ::Vector3& maxBounds) const
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
        const AZ::Vector3 boundsMin = bounds.GetMin();
        const AZ::Vector3 boundsMax = bounds.GetMax();

        const AZ::Vector2 gridMinBoundLower = vector2Floor(AZ::Vector2(boundsMin) / gridResolution) * gridResolution;
        const AZ::Vector2 gridMaxBoundUpper = vector2Ceil(AZ::Vector2(boundsMax) / gridResolution) * gridResolution;

        minBounds = AZ::Vector3(gridMinBoundLower.GetX(), gridMinBoundLower.GetY(), boundsMin.GetZ());
        maxBounds = AZ::Vector3(gridMaxBoundUpper.GetX(), gridMaxBoundUpper.GetY(), boundsMax.GetZ());
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldGridSizeInBounds(const AZ::Aabb& bounds, int32_t& numColumns, int32_t& numRows) const
    {
        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Vector3 minBounds, maxBounds;
        GetHeightfieldBounds(bounds, minBounds, maxBounds);

        numColumns = aznumeric_cast<int32_t>((maxBounds.GetX() - minBounds.GetX()) / gridResolution.GetX());
        numRows = aznumeric_cast<int32_t>((maxBounds.GetY() - minBounds.GetY()) / gridResolution.GetY());
    }

    void TerrainPhysicsColliderComponent::GenerateHeightsInBounds(const AZ::Aabb& bounds, AZStd::vector<float>& heights) const
    {
        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        const float worldCenterZ = worldSize.GetCenter().GetZ();

        AZ::Vector3 minBounds, maxBounds;
        GetHeightfieldBounds(bounds, minBounds, maxBounds);

        int32_t gridWidth, gridHeight;
        GetHeightfieldGridSizeInBounds(bounds, gridWidth, gridHeight);

        heights.clear();
        heights.reserve(gridWidth * gridHeight);

        for (int32_t row = 0; row < gridHeight; row++)
        {
            const float y = row * gridResolution.GetY() + minBounds.GetY();
            for (int32_t col = 0; col < gridWidth; col++)
            {
                const float x = col * gridResolution.GetX() + minBounds.GetX();
                float height = 0.0f;

                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    height, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y,
                    AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, nullptr);

                heights.emplace_back(height - worldCenterZ);
            }
        }
    }

    void TerrainPhysicsColliderComponent::GenerateHeightsAndMaterialsInBounds(
        const AZ::Aabb& bounds, AZStd::vector<Physics::HeightMaterialPoint>& heightMaterials) const
    {
        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        const float worldCenterZ = worldSize.GetCenter().GetZ();

        AZ::Vector3 minBounds, maxBounds;
        GetHeightfieldBounds(bounds, minBounds, maxBounds);

        int32_t gridWidth, gridHeight;
        GetHeightfieldGridSize(gridWidth, gridHeight);

        heightMaterials.clear();
        heightMaterials.reserve(gridWidth * gridHeight);

        for (int32_t row = 0; row < gridHeight; row++)
        {
            const float y = row * gridResolution.GetY() + minBounds.GetY();
            for (int32_t col = 0; col < gridWidth; col++)
            {
                const float x = col * gridResolution.GetX() + minBounds.GetX();
                float height = 0.0f;

                bool terrainExists = true;
                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    height, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y,
                    AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, &terrainExists);

                Physics::HeightMaterialPoint point;
                point.m_height = height - worldCenterZ;
                heightMaterials.emplace_back(point);
            }
        }
    }

    AZ::Vector2 TerrainPhysicsColliderComponent::GetHeightfieldGridSpacing() const
    {
        AZ::Vector2 gridResolution = AZ::Vector2(1.0f);
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            gridResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        return gridResolution;
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldGridSize(int32_t& numColumns, int32_t& numRows) const
    {
        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        GetHeightfieldGridSizeInBounds(worldSize, numColumns, numRows);
    }

    AZStd::vector<Physics::MaterialId> TerrainPhysicsColliderComponent::GetMaterialList() const
    {
        return AZStd::vector<Physics::MaterialId>();
    }

    AZStd::vector<float> TerrainPhysicsColliderComponent::GetHeights() const
    {
        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        AZStd::vector<float> heights; 
        GenerateHeightsInBounds(worldSize, heights);

        return heights;
    }

    AZStd::vector<Physics::HeightMaterialPoint> TerrainPhysicsColliderComponent::GetHeightsAndMaterials() const
    {
        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        AZStd::vector<Physics::HeightMaterialPoint> heightMaterials;
        GenerateHeightsAndMaterialsInBounds(worldSize, heightMaterials);

        return heightMaterials;
    }

    AZStd::vector<float> TerrainPhysicsColliderComponent::UpdateHeights(const AZ::Aabb& dirtyRegion) const
    {
        AZStd::vector<float> heights;
        GenerateHeightsInBounds(dirtyRegion, heights);

        return heights;
    }

    AZStd::vector<Physics::HeightMaterialPoint> TerrainPhysicsColliderComponent::UpdateHeightsAndMaterials(const AZ::Aabb& dirtyRegion) const
    {
        AZStd::vector<Physics::HeightMaterialPoint> heightMaterials;
        GenerateHeightsAndMaterialsInBounds(dirtyRegion, heightMaterials);

        return heightMaterials;
    }
}
