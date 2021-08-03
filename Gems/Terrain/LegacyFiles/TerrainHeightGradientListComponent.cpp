/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "TerrainHeightGradientListComponent.h"
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>


namespace Terrain
{
    void TerrainHeightGradientListConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainHeightGradientListConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("GradientEntities", &TerrainHeightGradientListConfig::m_gradientEntities)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainHeightGradientListConfig>(
                    "Terrain Height Gradient List Component", "Provide height data for a region of the world")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(0, &TerrainHeightGradientListConfig::m_gradientEntities, "Gradient Entities", "Ordered list of gradients to use as height providers.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("GradientService", 0x21c18d23))
                ;
            }
        }
    }

    void TerrainHeightGradientListComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainHeightProviderService", 0x5be2c613));
    }

    void TerrainHeightGradientListComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainHeightProviderService", 0x5be2c613));
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void TerrainHeightGradientListComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        //services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("TerrainAreaService", 0x98f9f606));
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void TerrainHeightGradientListComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainHeightGradientListConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainHeightGradientListComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainHeightGradientListComponent::m_configuration)
            ;
        }
    }

    TerrainHeightGradientListComponent::TerrainHeightGradientListComponent(const TerrainHeightGradientListConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainHeightGradientListComponent::Activate()
    {
        //AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        Terrain::TerrainAreaHeightRequestBus::Handler::BusConnect(GetEntityId());

        if (!(m_configuration.m_gradientEntities.empty()) && (*(m_configuration.m_gradientEntities.begin()) != GetEntityId()))
        {
            AZ::EntityBus::Handler::BusConnect(*(m_configuration.m_gradientEntities.begin()));
            //AZ::TransformNotificationBus::Handler::BusConnect(*(m_configuration.m_gradientEntities.begin()));
            //LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_configuration.m_gradientEntityId);
        }

        m_refreshHeightData = true;
        RefreshMinMaxHeights();

        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::RefreshArea, GetEntityId());
    }

    void TerrainHeightGradientListComponent::Deactivate()
    {
        Terrain::TerrainAreaHeightRequestBus::Handler::BusDisconnect();

        AZ::EntityBus::Handler::BusDisconnect();
        //AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();

        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::RefreshArea, GetEntityId());
    }

    bool TerrainHeightGradientListComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainHeightGradientListConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainHeightGradientListComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainHeightGradientListConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainHeightGradientListComponent::RefreshHeightData(bool& refresh)
    {
        // TODO:  Find better way to refresh this.  The problem is that the world component might get activated after us.
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        TerrainProviderRequestBus::BroadcastResult(worldBounds, &TerrainProviderRequestBus::Events::GetWorldBounds);
        m_cachedHeightRange = AZ::Vector2(worldBounds.GetMin().GetZ(), worldBounds.GetMax().GetZ());
        TerrainDataRequestBus::BroadcastResult(m_cachedHeightQueryResolution, &TerrainDataRequestBus::Events::GetHeightmapCellSize);

        refresh = refresh || m_refreshHeightData;
        m_refreshHeightData = false;
    }

    float TerrainHeightGradientListComponent::GetHeight(float x, float y)
    {
        for (auto& gradientId : m_configuration.m_gradientEntities)
        {
            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, gradientId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

            if (aabb.Contains(AZ::Vector3(x, y, aabb.GetMin().GetZ())))
            {
                GradientSignal::GradientSampleParams params(AZ::Vector3(x, y, 0.0f));
                float sample = 0.0f;

                GradientSignal::GradientRequestBus::EventResult(sample, gradientId, &GradientSignal::GradientRequestBus::Events::GetValue, params);
                float scaledHeight = AZ::Lerp(aabb.GetMin().GetZ(), aabb.GetMax().GetZ(), sample);
                return AZ::GetClamp(scaledHeight, m_cachedHeightRange.GetX(), m_cachedHeightRange.GetY());
            }

        }

        /*
        const AZ::Vector3 rayOrigin = AZ::Vector3((float)x, (float)y, m_cachedShapeBounds.GetMax().GetZ() + 1.0f);
        const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();
        AZ::VectorFloat intersectionDistance = 0.0f;
        bool hitShape = false;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(hitShape, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, rayOrigin, rayDirection, intersectionDistance);
        if (hitShape)
        {
            AZ::Vector3 hitPosition = rayOrigin + intersectionDistance * rayDirection;
            return AZ::GetClamp((float)(hitPosition.GetZ()), m_cachedHeightRange.GetX(), m_cachedHeightRange.GetY());
        }

        return m_cachedHeightRange.GetX();
        */
        return 0.0f;
    }

    void TerrainHeightGradientListComponent::GetHeight(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, Sampler sampleFilter = Sampler::DEFAULT)
    {
        float height = outPosition.GetZ();

        for (auto& gradientId : m_configuration.m_gradientEntities)
        {
            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, gradientId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            GradientSignal::GradientSampleParams params(AZ::Vector3(inPosition.GetX(), inPosition.GetY(), aabb.GetMin().GetZ()));

            if (aabb.Contains(params.m_position))
            {
                float sample = 0.0f;

                GradientSignal::GradientRequestBus::EventResult(sample, gradientId, &GradientSignal::GradientRequestBus::Events::GetValue, params);
                float scaledHeight = AZ::Lerp(aabb.GetMin().GetZ(), aabb.GetMax().GetZ(), sample);
                //height = AZ::GetClamp(scaledHeight, m_cachedHeightRange.GetX(), m_cachedHeightRange.GetY());
                height = scaledHeight;
                break;
            }

        }

        outPosition.SetZ(height);
    }

    void TerrainHeightGradientListComponent::GetNormal(const AZ::Vector3& inPosition, AZ::Vector3& outNormal, Sampler sampleFilter = Sampler::DEFAULT)
    {
        GetNormalSynchronous(inPosition.GetX(), inPosition.GetY(), outNormal);
    }


    void TerrainHeightGradientListComponent::GetHeightSynchronous(float x, float y, float& height)
    {
        if ((x >= m_cachedShapeBounds.GetMin().GetX()) && (x <= m_cachedShapeBounds.GetMax().GetX()) &&
            (y >= m_cachedShapeBounds.GetMin().GetY()) && (y <= m_cachedShapeBounds.GetMax().GetY()))
        {
            height = GetHeight(x, y);
        }
    }

    void TerrainHeightGradientListComponent::GetNormalSynchronous(float x, float y, AZ::Vector3& normal)
    {
        if ((x >= m_cachedShapeBounds.GetMin().GetX()) && (x <= m_cachedShapeBounds.GetMax().GetX()) &&
            (y >= m_cachedShapeBounds.GetMin().GetY()) && (y <= m_cachedShapeBounds.GetMax().GetY()))
        {
            float fRange = (m_cachedHeightQueryResolution / 2.0f) + 0.05f;

            AZ::Vector3 v1(x - fRange, y - fRange, GetHeight(x - fRange, y - fRange));
            AZ::Vector3 v2(x - fRange, y + fRange, GetHeight(x - fRange, y + fRange));
            AZ::Vector3 v3(x + fRange, y - fRange, GetHeight(x + fRange, y - fRange));
            AZ::Vector3 v4(x + fRange, y + fRange, GetHeight(x + fRange, y + fRange));
            normal = (v3 - v2).Cross(v4 - v1).GetNormalized();
        }
    }

    void TerrainHeightGradientListComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_refreshHeightData = true;
        RefreshMinMaxHeights();
    }

    void TerrainHeightGradientListComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        m_refreshHeightData = true;
        RefreshMinMaxHeights();
    }

    void TerrainHeightGradientListComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        m_refreshHeightData = true;
        RefreshMinMaxHeights();
    }

    void TerrainHeightGradientListComponent::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        m_refreshHeightData = true;
        RefreshMinMaxHeights();
    }

    void TerrainHeightGradientListComponent::RefreshMinMaxHeights()
    {
        // Get the height range of our height provider based on the shape component.
        LmbrCentral::ShapeComponentRequestsBus::EventResult(m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        m_cachedHeightRange = AZ::Vector2(m_cachedShapeBounds.GetMin().GetZ(), m_cachedShapeBounds.GetMax().GetZ());
        m_cachedHeightQueryResolution = 1.0f;

        // Get the height range of the entire world
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        TerrainProviderRequestBus::BroadcastResult(worldBounds, &TerrainProviderRequestBus::Events::GetWorldBounds);
        m_cachedHeightRange = AZ::Vector2(worldBounds.GetMin().GetZ(), worldBounds.GetMax().GetZ());
        TerrainDataRequestBus::BroadcastResult(m_cachedHeightQueryResolution, &TerrainDataRequestBus::Events::GetHeightmapCellSize);
    }

}
