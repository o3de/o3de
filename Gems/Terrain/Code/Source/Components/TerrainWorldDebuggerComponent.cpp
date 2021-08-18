/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldDebuggerComponent.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Terrain
{
    void TerrainWorldDebuggerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainWorldDebuggerConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("DebugWireframe", &TerrainWorldDebuggerConfig::m_drawWireframe)
                ->Field("DebugWorldBounds", &TerrainWorldDebuggerConfig::m_drawWorldBounds)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainWorldDebuggerConfig>(
                    "Terrain World Debugger Component", "Optional component for enabling terrain debugging features.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldDebuggerConfig::m_drawWireframe, "Show Wireframe", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldDebuggerConfig::m_drawWorldBounds, "Show World Bounds", "");
            }
        }
    }

    void TerrainWorldDebuggerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainDebugService"));
    }

    void TerrainWorldDebuggerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainDebugService"));
    }

    void TerrainWorldDebuggerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainService"));
    }

    void TerrainWorldDebuggerComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainWorldDebuggerConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainWorldDebuggerComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainWorldDebuggerComponent::m_configuration)
            ;
        }
    }

    TerrainWorldDebuggerComponent::TerrainWorldDebuggerComponent(const TerrainWorldDebuggerConfig& configuration)
        : m_configuration(configuration)
    {
    }

    TerrainWorldDebuggerComponent::~TerrainWorldDebuggerComponent()
    {
    }

    void TerrainWorldDebuggerComponent::Activate()
    {
        TerrainSystemServiceRequestBus::Broadcast(
            &TerrainSystemServiceRequestBus::Events::SetDebugWireframe, m_configuration.m_drawWireframe);

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
    }

    void TerrainWorldDebuggerComponent::Deactivate()
    {
        TerrainSystemServiceRequestBus::Broadcast(
            &TerrainSystemServiceRequestBus::Events::SetDebugWireframe, false);

        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    bool TerrainWorldDebuggerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainWorldDebuggerConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainWorldDebuggerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainWorldDebuggerConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    AZ::Aabb TerrainWorldDebuggerComponent::GetWorldBounds()
    {
        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        return terrainAabb;
    }

    AZ::Aabb TerrainWorldDebuggerComponent::GetLocalBounds()
    {
        // This is a level component, so the local bounds will always be the same as the world bounds.
        return GetWorldBounds();
    }

    void TerrainWorldDebuggerComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        /*
        AZ::Vector3 cameraPos(0.0f);
        if (auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get(); viewportContextRequests)
        {
            AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetViewportContextById(viewportInfo.m_viewportId);
            cameraPos = viewportContext->GetCameraTransform().GetTranslation();
        }
        */

        // Draw a wireframe box around the entire terrain world bounds
        if (m_configuration.m_drawWorldBounds)
        {
            AZ::Color outlineColor(1.0f, 0.0f, 0.0f, 1.0f);
            AZ::Aabb aabb = GetWorldBounds();

            debugDisplay.SetColor(outlineColor);
            debugDisplay.DrawWireBox(aabb.GetMin(), aabb.GetMax());
        }

        if (m_configuration.m_drawWireframe && !m_wireframeGridPoints.empty())
        {
            const AZ::Color primaryColor = AZ::Color(0.25f, 0.25f, 0.25f, 1.0f);
            debugDisplay.DrawLines(m_wireframeGridPoints, primaryColor);
        }
    }

    void TerrainWorldDebuggerComponent::OnTerrainDataChanged(
        [[maybe_unused]] const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if (dataChangedMask & (TerrainDataChangedMask::Settings | TerrainDataChangedMask::HeightData))
        {
            m_wireframeGridPoints.clear();
            AZ::Aabb worldBounds = GetWorldBounds();

            AZ::Vector2 queryResolution = AZ::Vector2(1.0f);
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainGridResolution);

            m_wireframeGridPoints.reserve(
                8 * aznumeric_cast<size_t>(
                    (worldBounds.GetXExtent() / queryResolution.GetX()) *
                    (worldBounds.GetYExtent() / queryResolution.GetY())
                    ));

            const float minX = worldBounds.GetMin().GetX();
            const float minY = worldBounds.GetMin().GetY();

            for (float y = worldBounds.GetMin().GetY(); y < (worldBounds.GetMax().GetY() - queryResolution.GetY());
                 y += queryResolution.GetY())
            {
                for (float x = worldBounds.GetMin().GetX(); x < (worldBounds.GetMax().GetX() - queryResolution.GetX());
                     x += queryResolution.GetX())
                {
                    float x1 = x + queryResolution.GetX();
                    float y1 = y + queryResolution.GetY();

                    float z00 = 0.0f;
                    float z01 = 0.0f;
                    float z10 = 0.0f;
                    float z11 = 0.0f;
                    bool terrainExists;

                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        z00, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y,
                        AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT,
                        &terrainExists);
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        z01, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y1,
                        AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, &terrainExists);
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        z10, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x1, y,
                        AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, &terrainExists);
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        z11, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x1, y1,
                        AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, &terrainExists);

                    m_wireframeGridPoints.push_back(AZ::Vector3(x , y , z00));
                    m_wireframeGridPoints.push_back(AZ::Vector3(x1, y , z10));

                    m_wireframeGridPoints.push_back(AZ::Vector3(x , y1, z01));
                    m_wireframeGridPoints.push_back(AZ::Vector3(x1, y1, z11));

                    m_wireframeGridPoints.push_back(AZ::Vector3(x , y , z00));
                    m_wireframeGridPoints.push_back(AZ::Vector3(x , y1, z01));

                    m_wireframeGridPoints.push_back(AZ::Vector3(x1, y , z10));
                    m_wireframeGridPoints.push_back(AZ::Vector3(x1, y1, z11));
                }
            }

        }
    }


} // namespace Terrain
