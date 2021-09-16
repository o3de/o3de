/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainHeightGradientListComponent.h>

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
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("GradientService"))
                ;
            }
        }
    }

    void TerrainHeightGradientListComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainHeightProviderService"));
    }

    void TerrainHeightGradientListComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainHeightProviderService"));
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void TerrainHeightGradientListComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainAreaService"));
        services.push_back(AZ_CRC_CE("BoxShapeService"));
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
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        // Make sure we get update notifications whenever this entity or any dependent gradient entity changes in any way.
        // We'll use that to notify the terrain system that the height information needs to be refreshed.
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

        // Cache any height data needed and notify that the area has changed.
        OnCompositionChanged();
    }

    void TerrainHeightGradientListComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        Terrain::TerrainAreaHeightRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        // Since this height data will no longer exist, notify the terrain system to refresh the area.
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

    float TerrainHeightGradientListComponent::GetGradientValue(float x, float y)
    {
        float maxSample = 0.0f;

        GradientSignal::GradientSampleParams params(AZ::Vector3(x, y, 0.0f));

        // Right now, when the list contains multiple entries, we will use the highest point from each gradient.
        // This is needed in part because gradients don't really have world bounds, so they exist everywhere but generally have a value
        // of 0 outside their data bounds if they're using bounded data.  We should examine the possibility of extending the gradient API
        // to provide actual bounds so that it's possible to detect if the gradient even 'exists' in an area, at which point we could just
        // make this list a prioritized list from top to bottom for any points that overlap.
        for (auto& gradientId : m_configuration.m_gradientEntities)
        {
            float sample = 0.0f;
            GradientSignal::GradientRequestBus::EventResult(sample, gradientId, &GradientSignal::GradientRequestBus::Events::GetValue, params);
            maxSample = AZ::GetMax(maxSample, sample);
        }

        return maxSample;
    }

    float TerrainHeightGradientListComponent::ConvertGradientValueToHeight(float gradientValue)
    {
        const float height = AZ::Lerp(m_cachedShapeBounds.GetMin().GetZ(), m_cachedShapeBounds.GetMax().GetZ(), gradientValue);
        return AZ::GetClamp(height, m_cachedMinWorldHeight, m_cachedMaxWorldHeight);
    }

    float TerrainHeightGradientListComponent::GetHeight(float x, float y, AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter)
    {
        float gradientValue = 0.0f;

        switch (sampleFilter)
        {
        // Get the value at the requested location, using the terrain grid to bilinear filter between sample grid points.
        case AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR:
            {
                AZ::Vector2 delta;
                AZ::Vector2 pos0;
                ClampPosition(x, y, pos0, delta);

                const AZ::Vector2 pos1 = pos0 + m_cachedHeightQueryResolution;
                const float gradientX0Y0 = GetGradientValue(pos0.GetX(), pos0.GetY());
                const float gradientX1Y0 = GetGradientValue(pos1.GetX(), pos0.GetY());
                const float gradientX0Y1 = GetGradientValue(pos0.GetX(), pos1.GetY());
                const float gradientX1Y1 = GetGradientValue(pos1.GetX(), pos1.GetY());
                const float gradientXY0 = AZ::Lerp(gradientX0Y0, gradientX1Y0, delta.GetX());
                const float gradientXY1 = AZ::Lerp(gradientX0Y1, gradientX1Y1, delta.GetX());
                gradientValue = AZ::Lerp(gradientXY0, gradientXY1, delta.GetY());
            }
            break;

        //! Clamp the input point to the terrain sample grid, then get the height at the given grid location.
        case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
            {
                AZ::Vector2 delta;
                AZ::Vector2 clampedPosition;
                ClampPosition(x, y, clampedPosition, delta);

                gradientValue = GetGradientValue(clampedPosition.GetX(), clampedPosition.GetY());
            }
            break;

        //! Directly get the value at the location, regardless of terrain sample grid density.
        case AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT:
            [[fallthrough]];
        default:
            gradientValue = GetGradientValue(x, y);
            break;
        }

        return ConvertGradientValueToHeight(gradientValue);
    }

    void TerrainHeightGradientListComponent::ClampPosition(float x, float y, AZ::Vector2& outPosition, AZ::Vector2& delta)
    {
        AZ::Vector2 normalizedPosition = AZ::Vector2(x, y) / m_cachedHeightQueryResolution;
        delta = AZ::Vector2(
            normalizedPosition.GetX() - floor(normalizedPosition.GetX()), normalizedPosition.GetY() - floor(normalizedPosition.GetY()));

        outPosition = (normalizedPosition - delta) * m_cachedHeightQueryResolution;
    }

    void TerrainHeightGradientListComponent::GetHeight(
        const AZ::Vector3& inPosition,
        AZ::Vector3& outPosition,
        AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter = AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT)
    {
        outPosition.SetZ(GetHeight(inPosition.GetX(), inPosition.GetY(), sampleFilter));
    }

    void TerrainHeightGradientListComponent::GetNormal(
        const AZ::Vector3& inPosition,
        AZ::Vector3& outNormal,
        AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter = AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT)
    {
        const float x = inPosition.GetX();
        const float y = inPosition.GetY();

        if ((x >= m_cachedShapeBounds.GetMin().GetX()) && (x <= m_cachedShapeBounds.GetMax().GetX()) &&
            (y >= m_cachedShapeBounds.GetMin().GetY()) && (y <= m_cachedShapeBounds.GetMax().GetY()))
        {
            AZ::Vector2 fRange = (m_cachedHeightQueryResolution / 2.0f) + AZ::Vector2(0.05f);

            AZ::Vector3 v1(x - fRange.GetX(), y - fRange.GetY(), GetHeight(x - fRange.GetX(), y - fRange.GetY(), sampleFilter));
            AZ::Vector3 v2(x - fRange.GetX(), y + fRange.GetY(), GetHeight(x - fRange.GetX(), y + fRange.GetY(), sampleFilter));
            AZ::Vector3 v3(x + fRange.GetX(), y - fRange.GetY(), GetHeight(x + fRange.GetX(), y - fRange.GetY(), sampleFilter));
            AZ::Vector3 v4(x + fRange.GetX(), y + fRange.GetY(), GetHeight(x + fRange.GetX(), y + fRange.GetY(), sampleFilter));
            outNormal = (v3 - v2).Cross(v4 - v1).GetNormalized();
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
            m_cachedHeightQueryResolution, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightQueryResolution);

        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb);

        // Save off the min/max heights so that we don't have to re-query them on every single height query.
        m_cachedMinWorldHeight = worldBounds.GetMin().GetZ();
        m_cachedMaxWorldHeight = worldBounds.GetMax().GetZ();
    }

    void TerrainHeightGradientListComponent::OnTerrainDataChanged(
        [[maybe_unused]] const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if (dataChangedMask & TerrainDataChangedMask::Settings)
        {
            // If the terrain system settings changed, it's possible that the world bounds have changed, which can affect our height data.
            // Refresh the min/max heights and notify that the height data for this area needs to be refreshed.
            OnCompositionChanged();
        }
    }

}
