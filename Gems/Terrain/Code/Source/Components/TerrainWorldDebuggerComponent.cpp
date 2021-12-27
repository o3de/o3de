/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldDebuggerComponent.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Visibility/EntityBoundsUnionBus.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldDebuggerConfig::m_drawWireframe, "Show Wireframe", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldDebuggerConfig::m_drawWorldBounds, "Show World Bounds", "");
            }
        }
    }

    void TerrainWorldDebuggerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainDebugService"));
    }

    void TerrainWorldDebuggerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainDebugService"));
    }

    void TerrainWorldDebuggerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainService"));
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
        m_wireframeBounds = AZ::Aabb::CreateNull();
        m_dirtyRegion = GetWorldBounds();

        // We will create an NxN sector grid for our wireframe, based on the maximum number of sectors that we can draw.
        // Get the square root so that we have the max sector count for rows and columns.
        m_sectorGridSize = aznumeric_cast<int32_t>(sqrtf(MaxSectorsToDraw));

        // We're always going to keep the camera in the center square, so if we currently have an even number of sectors, reduce it
        // so that we have an odd number of sectors.
        m_sectorGridSize = (m_sectorGridSize & 0x01) ? m_sectorGridSize : m_sectorGridSize - 1;

        // Create our fixed set of sectors that we'll draw.  By default, they'll all be constructed as dirty, so they'll get refreshed
        // whenever they're needed.
        m_wireframeSectors.clear();
        m_wireframeSectors.resize(m_sectorGridSize * m_sectorGridSize);

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());
    }

    void TerrainWorldDebuggerComponent::Deactivate()
    {
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();

        m_wireframeBounds = AZ::Aabb::CreateNull();
        m_dirtyRegion = AZ::Aabb::CreateNull();

        m_wireframeSectors.clear();
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
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // Draw a wireframe box around the entire terrain world bounds
        if (m_configuration.m_drawWorldBounds)
        {
            AZ::Color outlineColor(1.0f, 0.0f, 0.0f, 1.0f);
            AZ::Aabb aabb = GetWorldBounds();

            debugDisplay.SetColor(outlineColor);
            debugDisplay.DrawWireBox(aabb.GetMin(), aabb.GetMax());
        }

        // Draw a wireframe representation of the terrain surface
        if (m_configuration.m_drawWireframe)
        {
            AZ::Aabb worldBounds = GetWorldBounds();
            float worldMinZ = worldBounds.GetMin().GetZ();

            // If we have a dirty region, mark any overlapping valid sectors as dirty so we know to update them below.
            if (m_dirtyRegion.IsValid())
            {
                for (auto& sector : m_wireframeSectors)
                {
                    if (m_dirtyRegion.Overlaps(sector.m_aabb))
                    {
                        sector.m_isDirty = true;
                    }
                }

                // We're done processing the dirty region itself, so clear it out.
                m_dirtyRegion = AZ::Aabb::CreateNull();
            }

            // Start by assuming we'll draw the entire world.
            AZ::Aabb drawingAabb = GetWorldBounds();

            int32_t cameraSectorX = 0;
            int32_t cameraSectorY = 0;

            AZ::Vector2 queryResolution = AZ::Vector2(1.0f);
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

            // Assuming we can get the camera, reduce the drawing bounds to a fixed distance around the camera.
            if (auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get(); viewportContextRequests)
            {
                // Get the current camera position.
                AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetViewportContextById(viewportInfo.m_viewportId);
                AZ::Vector3 cameraPos = viewportContext->GetCameraTransform().GetTranslation();

                // Determine how far to draw in each direction in world space based on our MaxSectorsToDraw
                AZ::Vector3 viewDistance(
                    queryResolution.GetX() * SectorSizeInGridPoints * m_sectorGridSize,
                    queryResolution.GetY() * SectorSizeInGridPoints * m_sectorGridSize,
                    0.0f);

                // Create an AABB around the camera based on how far we want to be able to draw in each direction and clamp the
                // drawing AABB to it.
                AZ::Aabb cameraAabb = AZ::Aabb::CreateFromMinMax(
                    AZ::Vector3(
                        cameraPos.GetX() - viewDistance.GetX(), cameraPos.GetY() - viewDistance.GetY(), drawingAabb.GetMin().GetZ()),
                    AZ::Vector3(
                        cameraPos.GetX() + viewDistance.GetX(), cameraPos.GetY() + viewDistance.GetY(), drawingAabb.GetMin().GetZ()));
                drawingAabb.Clamp(cameraAabb);

                // Calculate the current camera sector based on position.
                cameraSectorX = aznumeric_cast<int32_t>(cameraPos.GetX() / (queryResolution.GetX() * SectorSizeInGridPoints));
                cameraSectorY = aznumeric_cast<int32_t>(cameraPos.GetY() / (queryResolution.GetY() * SectorSizeInGridPoints));
            }

            // Calculate the world size of each sector.
            // Ex:  If our sectors have 10 grid points, and the query resolution is 1 m, each sector will be 10 m.
            const float xSectorSize = (queryResolution.GetX() * SectorSizeInGridPoints);
            const float ySectorSize = (queryResolution.GetY() * SectorSizeInGridPoints);

            for (int32_t sectorY = cameraSectorY - (m_sectorGridSize / 2); sectorY <= cameraSectorY + (m_sectorGridSize / 2); sectorY++)
            {
                for (int32_t sectorX = cameraSectorX - (m_sectorGridSize / 2); sectorX <= cameraSectorX + (m_sectorGridSize / 2); sectorX++)
                {
                    int32_t sectorIndex = (abs(sectorY % m_sectorGridSize) * m_sectorGridSize) + abs(sectorX % m_sectorGridSize);
                    auto& sector = m_wireframeSectors[sectorIndex];
                    AZ::Aabb sectorAabb = AZ::Aabb::CreateFromMinMax(
                        AZ::Vector3(sectorX * xSectorSize, sectorY * ySectorSize, worldBounds.GetMin().GetZ()),
                        AZ::Vector3((sectorX + 1) * xSectorSize, (sectorY + 1) * ySectorSize, worldBounds.GetMax().GetZ()));

                    sectorAabb.Clamp(worldBounds);

                    if (sector.m_aabb != sectorAabb)
                    {
                        sector.m_aabb = sectorAabb;
                        sector.m_isDirty = true;
                    }
                }
            }

            // For each sector, if it appears within our view distance, draw it.
            for (auto& sector : m_wireframeSectors)
            {
                if (drawingAabb.Overlaps(sector.m_aabb))
                {
                    if (sector.m_isDirty)
                    {
                        RebuildSectorWireframe(sector, queryResolution, worldMinZ);
                    }

                    if (!sector.m_lineVertices.empty())
                    {
                        const AZ::Color primaryColor = AZ::Color(0.25f, 0.25f, 0.25f, 1.0f);
                        debugDisplay.DrawLines(sector.m_lineVertices, primaryColor);
                    }
                    else
                    {
                        AZ_Warning("Debug", false, "empty sector!");
                    }
                }
            }
        }
    }

    void TerrainWorldDebuggerComponent::RebuildSectorWireframe(WireframeSector& sector, const AZ::Vector2& gridResolution, float worldMinZ)
    {
        if (!sector.m_isDirty)
        {
            return;
        }

        sector.m_isDirty = false;

        // To rebuild the wireframe, we walk through the sector by X, then by Y.  For each point, we add two lines in a _| shape.
        // To do that, we'll need to cache the height from the previous point to draw the _ line, and from the previous row to draw
        // the | line.

        // When walking through the bounding box, the loops will be inclusive on one side, and exclusive on the other.  However, since
        // our box is exactly aligned with grid points, we want to get the grid points on both sides in each direction, so we need to
        // expand our query region by one extra point.
        // For example, if our AABB is 2 m and our grid resolution is 1 m, we'll want to query (*--*--*--), not (*--*--).
        // Since we're processing lines based on the grid points and going backwards, this will give us (*--*--*).

        AZ::Aabb region = sector.m_aabb;
        region.SetMax(region.GetMax() + AZ::Vector3(gridResolution.GetX(), gridResolution.GetY(), 0.0f));

        // This keeps track of the height from the previous point for the _ line.
        float previousHeight = 0.0f;

        // This keeps track of the heights from the previous row for the | line.
        AZStd::vector<float> rowHeights(aznumeric_cast<size_t>(ceil(region.GetExtents().GetX() / gridResolution.GetX())));

        // We need 4 vertices for each grid point in our sector to hold the _| shape.
        const uint32_t numSamplesX = static_cast<uint32_t>((region.GetMax().GetX() - region.GetMin().GetX()) / gridResolution.GetX());
        const uint32_t numSamplesY = static_cast<uint32_t>((region.GetMax().GetY() - region.GetMin().GetY()) / gridResolution.GetY());
        sector.m_lineVertices.clear();
        sector.m_lineVertices.reserve(numSamplesX * numSamplesY * 4);

        auto HeightCallback = [gridResolution, &previousHeight, &rowHeights,
                               &sector](uint32_t xIndex, uint32_t yIndex, const AZ::Vector3& position, [[maybe_unused]] bool terrainExists)
        {
            // Don't add any vertices for the first column or first row.  These grid lines will be handled by an adjacent sector, if
            // there is one.
            if ((xIndex > 0) && (yIndex > 0))
            {
                float x = position.GetX() - gridResolution.GetX();
                float y = position.GetY() - gridResolution.GetY();

                sector.m_lineVertices.emplace_back(AZ::Vector3(x, position.GetY(), previousHeight));
                sector.m_lineVertices.emplace_back(position);

                sector.m_lineVertices.emplace_back(AZ::Vector3(position.GetX(), y, rowHeights[xIndex]));
                sector.m_lineVertices.emplace_back(position);
            }

            // Save off the heights so that we can use them to draw subsequent columns and rows.
            previousHeight = position.GetZ();
            rowHeights[xIndex] = position.GetZ();
        };

        // This set of nested loops will get replaced with a call to ProcessHeightsFromRegion once the API exists.
        uint32_t yIndex = 0;
        for (float y = region.GetMin().GetY(); y < region.GetMax().GetY(); y += gridResolution.GetY())
        {
            uint32_t xIndex = 0;
            for (float x = region.GetMin().GetX(); x < region.GetMax().GetX(); x += gridResolution.GetX())
            {
                float height = worldMinZ;
                bool terrainExists = false;
                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    height, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y,
                    AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);
                HeightCallback(xIndex, yIndex, AZ::Vector3(x, y, height), terrainExists);
                xIndex++;
            }
            yIndex++;
        }
    }

    void TerrainWorldDebuggerComponent::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if (dataChangedMask & (TerrainDataChangedMask::Settings | TerrainDataChangedMask::HeightData))
        {
            if (dirtyRegion.IsValid())
            {
                m_dirtyRegion.AddAabb(dirtyRegion);
            }
            else
            {
                m_dirtyRegion = GetWorldBounds();
            }

            if (dataChangedMask & TerrainDataChangedMask::Settings)
            {
                AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
                    &AzFramework::IEntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());
            }
        }
    }


} // namespace Terrain
