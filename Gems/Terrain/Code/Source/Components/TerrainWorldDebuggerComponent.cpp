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
        // Given the AuxGeom vertex limits, MaxSectorsToDraw is the max number of wireframe sectors we can draw without exceeding the
        // limits.  Since we want an N x N sector grid, take the square root to get the number of sectors in each direction.
        m_sectorGridSize = aznumeric_cast<int32_t>(sqrtf(MaxSectorsToDraw));

        // We're always going to keep the camera in the center square, so "round" downwards to an odd number of sectors if we currently
        // have an even number.  (If we added a sector, we'll go above the max sectors that we can draw with our vertex limits)
        m_sectorGridSize = (m_sectorGridSize & 0x01) ? m_sectorGridSize : m_sectorGridSize - 1;

        // Create our fixed set of sectors that we'll draw.  By default, they'll all be constructed as dirty, so they'll get refreshed
        // the first time we try to draw them.  (If wireframe drawing is disabled, we'll never refresh them)
        m_wireframeSectors.clear();
        m_wireframeSectors.resize(m_sectorGridSize * m_sectorGridSize);

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        // Any time the world bounds potentially changes, notify that the terrain debugger's visibility bounds also changed.
        // Otherwise, DisplayEntityViewport() won't get called at the appropriate times, since the visibility could get incorrectly
        // culled out.
        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());
    }

    void TerrainWorldDebuggerComponent::Deactivate()
    {
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();

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

    void TerrainWorldDebuggerComponent::MarkDirtySectors(const AZ::Aabb& dirtyRegion)
    {
        // Create a 2D version of dirtyRegion that has Z set to min/max float values, so that we can just check for XY overlap with
        // each sector.
        const AZ::Aabb dirtyRegion2D = AZ::Aabb::CreateFromMinMaxValues(
            dirtyRegion.GetMin().GetX(), dirtyRegion.GetMin().GetY(), AZStd::numeric_limits<float>::lowest(),
            dirtyRegion.GetMax().GetX(), dirtyRegion.GetMax().GetY(), AZStd::numeric_limits<float>::max());

        // For each sector that overlaps the dirty region (or all of them if the region is invalid), mark them as dirty so that
        // they'll get refreshed the next time we need to draw them.
        for (auto& sector : m_wireframeSectors)
        {
            if (!dirtyRegion2D.IsValid() || dirtyRegion2D.Overlaps(sector.m_aabb))
            {
                sector.m_isDirty = true;
            }
        }
    }

    void TerrainWorldDebuggerComponent::DrawWorldBounds(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_configuration.m_drawWorldBounds)
        {
            return;
        }

        // Draw a wireframe box around the entire terrain world bounds
        AZ::Color outlineColor(1.0f, 0.0f, 0.0f, 1.0f);
        AZ::Aabb aabb = GetWorldBounds();

        debugDisplay.SetColor(outlineColor);
        debugDisplay.DrawWireBox(aabb.GetMin(), aabb.GetMax());
    }

    void TerrainWorldDebuggerComponent::DrawWireframe(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (!m_configuration.m_drawWireframe)
        {
            return;
        }

        /* This draws a wireframe centered on the camera that extends out to a certain distance at all times. To reduce the amount of
         * recalculations we need to do on each camera movement, we divide the world into a conceptual grid of sectors, where each sector
         * contains a fixed number of terrain height points. So for example, if the terrain has height data at 1 m spacing, the sectors
         * might be 10 m x 10 m in size. If the height data is spaced at 0.5 m, the sectors might be 5 m x 5 m in size. The wireframe
         * draws N x N sectors centered around the camera, as determined by m_sectorGridSize. So a gridSize of 7 with a sector size of
         * 10 m means that we'll be drawing 7 x 7 sectors, or 70 m x 70 m, centered around the camera. Each time the camera moves into
         * a new sector, we refresh the changed sectors before drawing them.
         *
         * The only tricky bit to this design is the way the sectors are stored and indexed. They're stored in a single vector as NxN
         * entries, so they would normally be indexed as (y * N) + x. Since we want this to be centered on the camera, the easy answer
         * would be to take the camera position - (N / 2) (since we're centering) as the relative offset to the first entry. But this
         * would mean that the entire set of entries would change every time we move the camera. For example, if we had 5 entries,
         * they might map to 0-4, 1-5, 2-6, 3-7, etc as the camera moves.
         *
         * Instead, we use mod (%) to rotate our indices around, so it would go (0 1 2 3 4), (5 1 2 3 4), (5 6 2 3 4), (5 6 7 3 4), etc
         * as the camera moves. For negative entries, we rotate the indices in reverse, so that we get results like (0 1 2 3 4),
         * (0 1 2 3 -1), (0 1 2 -2 -1), (0 1 -3 -2 -1), etc. This way we always have the correct range of sectors, and sectors that have
         * remained visible are left alone and don't need to be updated again.
         */

        // Get the terrain world bounds
        AZ::Aabb worldBounds = GetWorldBounds();
        float worldMinZ = worldBounds.GetMin().GetZ();

        // Get the terrain height data resolution
        AZ::Vector2 heightDataResolution = AZ::Vector2(1.0f);
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            heightDataResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        // Get the size of a wireframe sector in world space
        const AZ::Vector2 sectorSize = heightDataResolution * SectorSizeInGridPoints;

        // Try to get the current camera position, or default to (0,0) if we can't.
        AZ::Vector3 cameraPos = AZ::Vector3::CreateZero();
        if (auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get(); viewportContextRequests)
        {
            AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetViewportContextById(viewportInfo.m_viewportId);
            cameraPos = viewportContext->GetCameraTransform().GetTranslation();
        }

        // Convert our camera position to a wireframe grid sector.  We first convert from world space to sector space by dividing by
        // sectorSize, so that integer values are sectors, and fractional values are the distance within the sector.  Then we get the
        // floor, so that we consistently get the next lowest integer - i.e. 2.3 -> 2, and -2.3 -> -3. This gives us consistent behavior
        // across both positive and negative positions.
        AZ::Vector2 gridPosition = AZ::Vector2(cameraPos.GetX(), cameraPos.GetY()) / sectorSize;
        int32_t cameraSectorX = aznumeric_cast<int32_t>(gridPosition.GetFloor().GetX());
        int32_t cameraSectorY = aznumeric_cast<int32_t>(gridPosition.GetFloor().GetY());

        // Loop through each sector that we *want* to draw, based on camera position.  If the current sector at that index in
        // m_wireframeSectors doesn't match the world position we want, update its world position and mark it as dirty.
        // (We loop from -gridSize/2 to gridSize/2 so that the camera is always in the center sector.)
        for (int32_t sectorY = cameraSectorY - (m_sectorGridSize / 2); sectorY <= cameraSectorY + (m_sectorGridSize / 2); sectorY++)
        {
            for (int32_t sectorX = cameraSectorX - (m_sectorGridSize / 2); sectorX <= cameraSectorX + (m_sectorGridSize / 2); sectorX++)
            {

                // Calculate the index in m_wireframeSectors for this sector.  Our indices should rotate through 0 - gridSize, but just
                // using a single mod will produce a negative result for negative sector indices.  Using abs() will give us incorrect
                // "backwards" indices for negative numbers, so instead we add the grid size and mod a second time.
                // Ex:  For a grid size of 5, we want the indices to map like this:
                //      Index   0   1  2  3  4
                //      Values -10 -9 -8 -7 -6
                //             -5  -4 -3 -2 -1
                //              0   1  2  3  4
                //              5   6  7  8  9
                // For -9, (-9 % 5) = -4, then (-4 + 5) % 5 = 1.  If we used abs(), we'd get 4, which is backwards from what we want.
                int32_t sectorYIndex = ((sectorY % m_sectorGridSize) + m_sectorGridSize) % m_sectorGridSize;
                int32_t sectorXIndex = ((sectorX % m_sectorGridSize) + m_sectorGridSize) % m_sectorGridSize;
                int32_t sectorIndex = (sectorYIndex * m_sectorGridSize) + sectorXIndex;

                WireframeSector& sector = m_wireframeSectors[sectorIndex];

                // Calculate the new world space box for this sector.
                AZ::Aabb sectorAabb = AZ::Aabb::CreateFromMinMax(
                    AZ::Vector3(sectorX * sectorSize.GetX(), sectorY * sectorSize.GetY(), worldMinZ),
                    AZ::Vector3((sectorX + 1) * sectorSize.GetX(), (sectorY + 1) * sectorSize.GetY(), worldMinZ));

                // Clamp it to the terrain world bounds.
                sectorAabb.Clamp(worldBounds);

                // If the world space box for the sector doesn't match, set it and mark the sector as dirty so we refresh the height data.
                if (sector.m_aabb != sectorAabb)
                {
                    sector.m_aabb = sectorAabb;
                    sector.m_isDirty = true;
                }
            }
        }

        // Finally, for each sector, rebuild the data if it's dirty, then draw it assuming it has valid data.
        // (Sectors that are outside the world bounds won't have any valid data, so they'll get skipped)
        for (auto& sector : m_wireframeSectors)
        {
            if (sector.m_isDirty)
            {
                RebuildSectorWireframe(sector, heightDataResolution);
            }

            if (!sector.m_lineVertices.empty())
            {
                const AZ::Color primaryColor = AZ::Color(0.25f, 0.25f, 0.25f, 1.0f);
                debugDisplay.DrawLines(sector.m_lineVertices, primaryColor);
            }
        }
    }


    void TerrainWorldDebuggerComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DrawWorldBounds(debugDisplay);
        DrawWireframe(viewportInfo, debugDisplay);

    }

    void TerrainWorldDebuggerComponent::RebuildSectorWireframe(WireframeSector& sector, const AZ::Vector2& gridResolution)
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

        // We need 4 vertices for each grid point in our sector to hold the _| shape.
        const size_t numSamplesX = aznumeric_cast<size_t>(ceil(region.GetExtents().GetX() / gridResolution.GetX()));
        const size_t numSamplesY = aznumeric_cast<size_t>(ceil(region.GetExtents().GetY() / gridResolution.GetY()));
        sector.m_lineVertices.clear();
        sector.m_lineVertices.reserve(numSamplesX * numSamplesY * 4);

        // This keeps track of the height from the previous point for the _ line.
        float previousHeight = 0.0f;

        // This keeps track of the heights from the previous row for the | line.
        AZStd::vector<float> rowHeights(numSamplesX);

        // For each terrain height value in the region, create the _| grid lines for that point and cache off the height value
        // for use with subsequent grid line calculations.
        auto ProcessHeightValue = [gridResolution, &previousHeight, &rowHeights, &sector]
            (size_t xIndex, size_t yIndex, const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            // Don't add any vertices for the first column or first row.  These grid lines will be handled by an adjacent sector, if
            // there is one.
            if ((xIndex > 0) && (yIndex > 0))
            {
                float x = surfacePoint.m_position.GetX() - gridResolution.GetX();
                float y = surfacePoint.m_position.GetY() - gridResolution.GetY();

                sector.m_lineVertices.emplace_back(AZ::Vector3(x, surfacePoint.m_position.GetY(), previousHeight));
                sector.m_lineVertices.emplace_back(surfacePoint.m_position);

                sector.m_lineVertices.emplace_back(AZ::Vector3(surfacePoint.m_position.GetX(), y, rowHeights[xIndex]));
                sector.m_lineVertices.emplace_back(surfacePoint.m_position);
            }

            // Save off the heights so that we can use them to draw subsequent columns and rows.
            previousHeight = surfacePoint.m_position.GetZ();
            rowHeights[xIndex] = surfacePoint.m_position.GetZ();
        };

        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(&AzFramework::Terrain::TerrainDataRequests::ProcessHeightsFromRegion,
            region, gridResolution, ProcessHeightValue, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);
    }

    void TerrainWorldDebuggerComponent::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if (dataChangedMask & (TerrainDataChangedMask::Settings | TerrainDataChangedMask::HeightData))
        {
            MarkDirtySectors(dirtyRegion);
        }

        if (dataChangedMask & TerrainDataChangedMask::Settings)
        {
            // Any time the world bounds potentially changes, notify that the terrain debugger's visibility bounds also changed.
            AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
                &AzFramework::IEntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());
        }
    }


} // namespace Terrain
