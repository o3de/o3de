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
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        Physics::HeightfieldProviderRequestsBus::Handler::BusConnect(entityId);
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        NotifyListenersOfHeightfieldDataChange();
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

    void TerrainPhysicsColliderComponent::NotifyListenersOfHeightfieldDataChange()
    {
        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        Physics::HeightfieldProviderNotificationBus::Broadcast(
            &Physics::HeightfieldProviderNotificationBus::Events::OnHeightfieldDataChanged, worldSize);
    }

    void TerrainPhysicsColliderComponent::OnShapeChanged([[maybe_unused]] ShapeChangeReasons changeReason)
    {
        // This will notify us of both shape changes and transform changes.
        // It's important to use this event for transform changes instead of listening to OnTransformChanged, because we need to guarantee
        // the shape has received the transform change message and updated its internal state before passing it along to us.

        NotifyListenersOfHeightfieldDataChange();
    }

    void TerrainPhysicsColliderComponent::OnTerrainDataCreateEnd()
    {
        // The terrain system has finished creating itself, so we should now have data for creating a heightfield.
        NotifyListenersOfHeightfieldDataChange();
    }

    void TerrainPhysicsColliderComponent::OnTerrainDataDestroyBegin()
    {
        // The terrain system is starting to destroy itself, so notify listeners of a change since the heightfield
        // will no longer have any valid data.
        NotifyListenersOfHeightfieldDataChange();
    }

    void TerrainPhysicsColliderComponent::OnTerrainDataChanged(
        [[maybe_unused]] const AZ::Aabb& dirtyRegion, [[maybe_unused]] TerrainDataChangedMask dataChangedMask)
    {
        NotifyListenersOfHeightfieldDataChange();
    }

    AZ::Aabb TerrainPhysicsColliderComponent::GetHeightfieldAabb() const
    {
        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        auto vector2Floor = [](const AZ::Vector2& in)
        {
            return AZ::Vector2(floor(in.GetX()), floor(in.GetY()));
        };
        auto vector2Ceil = [](const AZ::Vector2& in)
        {
            return AZ::Vector2(ceil(in.GetX()), ceil(in.GetY()));
        };

        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();
        const AZ::Vector3 boundsMin = worldSize.GetMin();
        const AZ::Vector3 boundsMax = worldSize.GetMax();

        const AZ::Vector2 gridMinBoundLower = vector2Floor(AZ::Vector2(boundsMin) / gridResolution) * gridResolution;
        const AZ::Vector2 gridMaxBoundUpper = vector2Ceil(AZ::Vector2(boundsMax) / gridResolution) * gridResolution;

        return AZ::Aabb::CreateFromMinMaxValues(
            gridMinBoundLower.GetX(), gridMinBoundLower.GetY(), boundsMin.GetZ(),
            gridMaxBoundUpper.GetX(), gridMaxBoundUpper.GetY(), boundsMax.GetZ()
        );
    }

    void TerrainPhysicsColliderComponent::GetHeightfieldHeightBounds(float& minHeightBounds, float& maxHeightBounds) const
    {
        AZ::Aabb heightfieldAabb = GetHeightfieldAabb();

        // Because our terrain heights are relative to the center of the bounding box, the min and max allowable heights are also
        // relative to the center.  They are also clamped to the size of the bounding box.
        minHeightBounds = -(heightfieldAabb.GetZExtent() / 2.0f);
        maxHeightBounds = heightfieldAabb.GetZExtent() / 2.0f;
    }

    AZ::Transform TerrainPhysicsColliderComponent::GetHeightfieldTransform() const
    {
        // We currently don't support rotation of terrain heightfields.
        AZ::Vector3 translate;
        AZ::TransformBus::EventResult(translate, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        AZ::Transform transform = AZ::Transform::CreateTranslation(translate);

        return transform;
    }

    void TerrainPhysicsColliderComponent::GenerateHeightsInBounds(AZStd::vector<float>& heights) const
    {
        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Aabb worldSize = GetHeightfieldAabb();

        const float worldCenterZ = worldSize.GetCenter().GetZ();

        int32_t gridWidth, gridHeight;
        GetHeightfieldGridSize(gridWidth, gridHeight);

        heights.clear();
        heights.reserve(gridWidth * gridHeight);

        for (int32_t row = 0; row < gridHeight; row++)
        {
            const float y = row * gridResolution.GetY() + worldSize.GetMin().GetY();
            for (int32_t col = 0; col < gridWidth; col++)
            {
                const float x = col * gridResolution.GetX() + worldSize.GetMin().GetX();
                float height = 0.0f;

                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    height, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y,
                    AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, nullptr);

                heights.emplace_back(height - worldCenterZ);
            }
        }
    }

    void TerrainPhysicsColliderComponent::GenerateHeightsAndMaterialsInBounds(
        AZStd::vector<Physics::HeightMaterialPoint>& heightMaterials) const
    {
        const AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Aabb worldSize = GetHeightfieldAabb();

        const float worldCenterZ = worldSize.GetCenter().GetZ();
        const float worldHeightBoundsMin = worldSize.GetMin().GetZ();
        const float worldHeightBoundsMax = worldSize.GetMax().GetZ();

        int32_t gridWidth, gridHeight;
        GetHeightfieldGridSize(gridWidth, gridHeight);

        heightMaterials.clear();
        heightMaterials.reserve(gridWidth * gridHeight);

        for (int32_t row = 0; row < gridHeight; row++)
        {
            const float y = row * gridResolution.GetY() + worldSize.GetMin().GetY();
            for (int32_t col = 0; col < gridWidth; col++)
            {
                const float x = col * gridResolution.GetX() + worldSize.GetMin().GetX();
                float height = 0.0f;

                bool terrainExists = true;
                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    height, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y,
                    AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, &terrainExists);

                // Any heights that fall outside the range of our bounding box will get turned into holes.
                if ((height < worldHeightBoundsMin) || (height > worldHeightBoundsMax))
                {
                    height = worldHeightBoundsMin;
                    terrainExists = false;
                }

                Physics::HeightMaterialPoint point;
                point.m_height = height - worldCenterZ;
                point.m_quadMeshType = terrainExists ? Physics::QuadMeshType::SubdivideUpperLeftToBottomRight : Physics::QuadMeshType::Hole;
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
        const AZ::Aabb bounds = GetHeightfieldAabb();

        numColumns = aznumeric_cast<int32_t>((bounds.GetMax().GetX() - bounds.GetMin().GetX()) / gridResolution.GetX());
        numRows = aznumeric_cast<int32_t>((bounds.GetMax().GetY() - bounds.GetMin().GetY()) / gridResolution.GetY());
    }

    AZStd::vector<Physics::MaterialId> TerrainPhysicsColliderComponent::GetMaterialList() const
    {
        return AZStd::vector<Physics::MaterialId>();
    }

    AZStd::vector<float> TerrainPhysicsColliderComponent::GetHeights() const
    {
        AZStd::vector<float> heights; 
        GenerateHeightsInBounds(heights);

        return heights;
    }

    AZStd::vector<Physics::HeightMaterialPoint> TerrainPhysicsColliderComponent::GetHeightsAndMaterials() const
    {
        AZStd::vector<Physics::HeightMaterialPoint> heightMaterials;
        GenerateHeightsAndMaterialsInBounds(heightMaterials);

        return heightMaterials;
    }
}
