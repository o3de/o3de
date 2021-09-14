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
    // Scale value used to convert from float heights to int16 values in the heightfield.
    constexpr float DefaultHeightScale = 1.f / 256.f;

    void TerrainPhysicsColliderConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainPhysicsColliderConfig, AZ::ComponentConfig>()
                ->Version(1)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
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

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainPhysicsColliderComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainPhysicsColliderComponent::m_configuration)
            ;
        }
    }

    TerrainPhysicsColliderComponent::TerrainPhysicsColliderComponent(const TerrainPhysicsColliderConfig& configuration)
        : m_configuration(configuration)
        , m_heightScale(DefaultHeightScale)
    {
    }

    TerrainPhysicsColliderComponent::TerrainPhysicsColliderComponent()
        : m_heightScale(DefaultHeightScale)
    {

    }

    void TerrainPhysicsColliderComponent::Activate()
    {
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        Physics::HeightfieldProviderRequestsBus::Handler::BusConnect(GetEntityId());

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
        NotifyListenersOfHeightfieldDataChange();
    }

    AZ::Vector2 TerrainPhysicsColliderComponent::GetHeightfieldGridSpacing() const
    {
        AZ::Vector2 gridResolution = AZ::Vector2(1.0f);
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            gridResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainGridResolution);

        return gridResolution;
    }

    AZStd::vector<int16_t> TerrainPhysicsColliderComponent::GetHeights() const
    {
        AZ::Vector2 gridResolution = GetHeightfieldGridSpacing();

        AZ::Aabb worldSize = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            worldSize, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        const float worldCenterZ = worldSize.GetCenter().GetZ();

        // Expand heightfield to contain aabb
        AZ::Vector3 minBounds = worldSize.GetMin();
        minBounds.SetX(minBounds.GetX() - fmodf(minBounds.GetX(), gridResolution.GetX()));
        minBounds.SetY(minBounds.GetY() - fmodf(minBounds.GetY(), gridResolution.GetY()));

        AZ::Vector3 maxBounds = worldSize.GetMax();
        maxBounds.SetX(maxBounds.GetX() + gridResolution.GetX() - fmodf(maxBounds.GetX(), gridResolution.GetX()));
        maxBounds.SetY(maxBounds.GetY() + gridResolution.GetX() - fmodf(maxBounds.GetY(), gridResolution.GetY()));

        const int32_t gridWidth = aznumeric_cast<int32_t>((maxBounds.GetX() - minBounds.GetX()) / gridResolution.GetX());
        const int32_t gridHeight = aznumeric_cast<int32_t>((maxBounds.GetY() - minBounds.GetY()) / gridResolution.GetY());

        AZStd::vector<int16_t> heights; 
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

                heights.emplace_back(azlossy_cast<int16_t>((height - worldCenterZ) * m_heightScale));
            }
        }

        return heights;
    }

    float TerrainPhysicsColliderComponent::GetHeightScale() const
    {
        return m_heightScale;
    }
}
