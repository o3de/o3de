/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        Terrain::TerrainAreaHeightRequestBus::Handler::BusConnect(GetEntityId());

        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(GetEntityId());

        for (auto& entityId : m_configuration.m_gradientEntities)
        {
            if (entityId != GetEntityId())
            {
                m_dependencyMonitor.ConnectDependency(entityId);
            }
        }

        RefreshMinMaxHeights();
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::RefreshArea, GetEntityId());
    }

    void TerrainHeightGradientListComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        Terrain::TerrainAreaHeightRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

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

    float TerrainHeightGradientListComponent::GetHeight(float x, float y)
    {
        float maxSample = 0.0f;

        GradientSignal::GradientSampleParams params(AZ::Vector3(x, y, 0.0f));

        for (auto& gradientId : m_configuration.m_gradientEntities)
        {
            float sample = 0.0f;
            GradientSignal::GradientRequestBus::EventResult(sample, gradientId, &GradientSignal::GradientRequestBus::Events::GetValue, params);
            maxSample = AZ::GetMax(maxSample, sample);
        }

        return AZ::Lerp(m_cachedMinHeight, m_cachedMaxHeight, maxSample);
    }

    void TerrainHeightGradientListComponent::GetHeight(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, [[maybe_unused]] Sampler sampleFilter = Sampler::DEFAULT)
    {
        float maxSample = 0.0f;

        GradientSignal::GradientSampleParams params(AZ::Vector3(inPosition.GetX(), inPosition.GetY(), 0.0f));

        for (auto& gradientId : m_configuration.m_gradientEntities)
        {
            float sample = 0.0f;
            GradientSignal::GradientRequestBus::EventResult(sample, gradientId, &GradientSignal::GradientRequestBus::Events::GetValue, params);
            maxSample = AZ::GetMax(maxSample, sample);
        }

        outPosition.SetZ(AZ::Lerp(m_cachedMinHeight, m_cachedMaxHeight, maxSample));
    }

    void TerrainHeightGradientListComponent::GetNormal(const AZ::Vector3& inPosition, AZ::Vector3& outNormal, [[maybe_unused]] Sampler sampleFilter = Sampler::DEFAULT)
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
            AZ::Vector2 fRange = (m_cachedHeightQueryResolution / 2.0f) + AZ::Vector2(0.05f);

            AZ::Vector3 v1(x - fRange.GetX(), y - fRange.GetY(), GetHeight(x - fRange.GetX(), y - fRange.GetY()));
            AZ::Vector3 v2(x - fRange.GetX(), y + fRange.GetY(), GetHeight(x - fRange.GetX(), y + fRange.GetY()));
            AZ::Vector3 v3(x + fRange.GetX(), y - fRange.GetY(), GetHeight(x + fRange.GetX(), y - fRange.GetY()));
            AZ::Vector3 v4(x + fRange.GetX(), y + fRange.GetY(), GetHeight(x + fRange.GetX(), y + fRange.GetY()));
            normal = (v3 - v2).Cross(v4 - v1).GetNormalized();
        }
    }

    void TerrainHeightGradientListComponent::OnCompositionChanged()
    {
        RefreshMinMaxHeights();
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::RefreshArea, GetEntityId());
    }

    void TerrainHeightGradientListComponent::RefreshMinMaxHeights()
    {
        // Get the height range of our height provider based on the shape component.
        LmbrCentral::ShapeComponentRequestsBus::EventResult(m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        // Get the height range of the entire world
        m_cachedHeightQueryResolution = AZ::Vector2(1.0f);
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            m_cachedHeightQueryResolution, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainGridResolution);

        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb);

        m_cachedMinHeight = AZ::GetClamp(m_cachedShapeBounds.GetMin().GetZ(), worldBounds.GetMin().GetZ(), worldBounds.GetMax().GetZ());
        m_cachedMaxHeight = AZ::GetClamp(m_cachedShapeBounds.GetMax().GetZ(), worldBounds.GetMin().GetZ(), worldBounds.GetMax().GetZ());
    }

}
