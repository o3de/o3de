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

AZ_DECLARE_BUDGET(Terrain);

namespace Terrain
{
    void TerrainWorldDebuggerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainDebugQueryVisualizerConfig>()
                ->Version(1)
                ->Field("DrawQueries", &TerrainDebugQueryVisualizerConfig::m_drawQueries)
                ->Field("Sampler", &TerrainDebugQueryVisualizerConfig::m_sampler)
                ->Field("PointsPerDirection", &TerrainDebugQueryVisualizerConfig::m_pointsPerDirection)
                ->Field("Spacing", &TerrainDebugQueryVisualizerConfig::m_spacing)
                ->Field("DrawHeights", &TerrainDebugQueryVisualizerConfig::m_drawHeights)
                ->Field("HeightPointSize", &TerrainDebugQueryVisualizerConfig::m_heightPointSize)
                ->Field("DrawNormals", &TerrainDebugQueryVisualizerConfig::m_drawNormals)
                ->Field("NormalHeight", &TerrainDebugQueryVisualizerConfig::m_normalHeight)
                ->Field("UseCameraPos", &TerrainDebugQueryVisualizerConfig::m_useCameraPosition)
                ->Field("CenterPos", &TerrainDebugQueryVisualizerConfig::m_centerPosition)
                ;

            serialize->Class<TerrainWorldDebuggerConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("DebugWireframe", &TerrainWorldDebuggerConfig::m_drawWireframe)
                ->Field("DebugWorldBounds", &TerrainWorldDebuggerConfig::m_drawWorldBounds)
                ->Field("DebugDirtyRegion", &TerrainWorldDebuggerConfig::m_drawLastDirtyRegion)
                ->Field("DebugQueries", &TerrainWorldDebuggerConfig::m_debugQueries)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainDebugQueryVisualizerConfig>(
                    "Terrain Debug Queries", "Settings related to visualizing the results of terrain queries.")
                    ->GroupElementToggle("Show Terrain Queries", &TerrainDebugQueryVisualizerConfig::m_drawQueries)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TerrainDebugQueryVisualizerConfig::m_sampler, "Sampler",
                        "The type of query sampler to use for querying the terrain values (Exact, Clamp, Bilinear)")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DrawQueriesDisabled)
                        ->EnumAttribute(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, "Exact")
                        ->EnumAttribute(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP, "Clamp")
                        ->EnumAttribute(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, "Bilinear")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TerrainDebugQueryVisualizerConfig::m_pointsPerDirection, "Point count",
                        "The number of points in each direction to visualize")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DrawQueriesDisabled)
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 64)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TerrainDebugQueryVisualizerConfig::m_spacing, "Spacing (m)",
                        "Determines how far apart the query results should be drawn in meters")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DrawQueriesDisabled)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.25f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 4.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainDebugQueryVisualizerConfig::m_drawHeights, "Draw Heights",
                        "Enable visualization of terrain height queries")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DrawQueriesDisabled)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TerrainDebugQueryVisualizerConfig::m_heightPointSize,
                        "Height Point Size (m)", "Determines the size of the height point in meters")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DisableHeights)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f / 128.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 4.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainDebugQueryVisualizerConfig::m_drawNormals, "Draw Normals",
                        "Enable visualization of terrain normal queries")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DrawQueriesDisabled)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TerrainDebugQueryVisualizerConfig::m_normalHeight, "Normal Height (m)",
                        "Determines the height of the normal line in meters")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DisableNormals)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.25f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 16.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainDebugQueryVisualizerConfig::m_useCameraPosition, "Use Camera Position",
                        "Determines whether to use the current camera position or a specified position")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DrawQueriesDisabled)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainDebugQueryVisualizerConfig::m_centerPosition, "World Position",
                        "Center of the area to draw query results in")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainDebugQueryVisualizerConfig::DisableCenterPosition)
                    ;

                edit->Class<TerrainWorldDebuggerConfig>(
                    "Terrain World Debugger Component", "Optional component for enabling terrain debugging features.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldDebuggerConfig::m_drawWireframe, "Show Wireframe",
                        "Draw a wireframe for the terrain quads in an area around the camera")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldDebuggerConfig::m_drawWorldBounds, "Show World Bounds",
                        "Draw the current world bounds for the terrain")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldDebuggerConfig::m_drawLastDirtyRegion,
                        "Show Dirty Region", "Draw the most recent dirty region for the terrain")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldDebuggerConfig::m_debugQueries, "Show Normals",
                        "Settings for drawing terrain normals")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
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

    AZ::Aabb TerrainWorldDebuggerComponent::GetWorldBounds() const
    {
        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        return terrainAabb;
    }

    AZ::Aabb TerrainWorldDebuggerComponent::GetLocalBounds() const
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
            AZStd::lock_guard<AZStd::recursive_mutex> lock(sector.m_sectorStateMutex);
            if (!dirtyRegion2D.IsValid() || dirtyRegion2D.Overlaps(sector.m_aabb))
            {
                sector.SetDirty();
            }
        }
    }

    void TerrainWorldDebuggerComponent::DrawLastDirtyRegion(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        using DataChangedMask = AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask;

        if (!m_configuration.m_drawLastDirtyRegion)
        {
            return;
        }

        // Draw a translucent box around the terrain dirty region
        const AZ::Color dirtyRegionColor(
            (m_lastDirtyData & (DataChangedMask::HeightData | DataChangedMask::Settings)) != DataChangedMask::None ? 1.0f : 0.0f,
            (m_lastDirtyData & (DataChangedMask::SurfaceData | DataChangedMask::Settings)) != DataChangedMask::None ? 1.0f : 0.0f,
            (m_lastDirtyData & (DataChangedMask::ColorData | DataChangedMask::Settings)) != DataChangedMask::None ? 1.0f : 0.0f,
            0.25f);

        if (m_lastDirtyRegion.IsValid())
        {
            debugDisplay.SetColor(dirtyRegionColor);
            debugDisplay.DrawSolidBox(m_lastDirtyRegion.GetMin(), m_lastDirtyRegion.GetMax());
        }
    }

    void TerrainWorldDebuggerComponent::DrawWorldBounds(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_configuration.m_drawWorldBounds)
        {
            return;
        }

        // Draw a wireframe box around the entire terrain world bounds
        const AZ::Color outlineColor(1.0f, 0.0f, 0.0f, 1.0f);
        const AZ::Aabb aabb = GetWorldBounds();

        if (aabb.IsValid())
        {
            debugDisplay.SetColor(outlineColor);
            debugDisplay.DrawWireBox(aabb.GetMin(), aabb.GetMax());
        }
    }

    void TerrainWorldDebuggerComponent::DrawQueries(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(Terrain);

        if (!m_configuration.m_debugQueries.m_drawQueries)
        {
            return;
        }

        // Early out if none of our draw toggles are enabled.
        if (!(m_configuration.m_debugQueries.m_drawHeights || m_configuration.m_debugQueries.m_drawNormals))
        {
            return;
        }

        const float spacing = m_configuration.m_debugQueries.m_spacing;
        const float halfDistance = spacing * (m_configuration.m_debugQueries.m_pointsPerDirection / 2.0f);
        const size_t totalPositions =
            m_configuration.m_debugQueries.m_pointsPerDirection * m_configuration.m_debugQueries.m_pointsPerDirection;

        // Get the center point for our visualization area either from the camera or a configured location.
        AZ::Vector3 centerPos = m_configuration.m_debugQueries.m_centerPosition;
        if (m_configuration.m_debugQueries.m_useCameraPosition)
        {
            if (auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get(); viewportContextRequests)
            {
                AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetViewportContextById(viewportInfo.m_viewportId);
                const AZ::Transform cameraTransform = viewportContext->GetCameraTransform();

                // Get our camera's center point
                centerPos = cameraTransform.GetTranslation();
            }
        }

        // Build up the list of positions to query.
        AZStd::vector<AZ::Vector3> positionList;
        positionList.reserve(totalPositions);

        for (size_t yPoint = 0; yPoint < m_configuration.m_debugQueries.m_pointsPerDirection; yPoint++)
        {
            for (size_t xPoint = 0; xPoint < m_configuration.m_debugQueries.m_pointsPerDirection; xPoint++)
            {
                float x = centerPos.GetX() - halfDistance + (spacing * xPoint);
                float y = centerPos.GetY() - halfDistance + (spacing * yPoint);
                positionList.emplace_back(x, y, 0.0f);
            }
        }

        // Process the terrain data query and turn the results into debug visualizations.
        // We'll reuse the normalLineVertices buffer for drawing both heights and normals. The first point of each normal line
        // always starts at the height position, so we can use those to draw heights.
        const float normalHeight = m_configuration.m_debugQueries.m_normalHeight;
        AZStd::vector<AZ::Vector3> normalLineVertices;
        normalLineVertices.reserve(totalPositions * 2);
        auto ProcessSurfacePoint =
            [normalHeight, &normalLineVertices](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
        {
            if (terrainExists)
            {
                normalLineVertices.emplace_back(surfacePoint.m_position);
                normalLineVertices.emplace_back(surfacePoint.m_position + (surfacePoint.m_normal * normalHeight));
            }
        };

        // Query for the terrain data. For now we'll just query both heights and normals all the time, but we could eventually
        // get more selective and only query for heights if we've disabled drawing normals. We can never query just for normals
        // because we still need the heights in that case to know where to draw the normals at.
        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequests::QueryList,
            positionList,
            static_cast<AzFramework::Terrain::TerrainDataRequests::TerrainDataMask>(
            AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights |
            AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Normals),
            ProcessSurfacePoint,
            m_configuration.m_debugQueries.m_sampler);

        // Draw the heights
        if (m_configuration.m_debugQueries.m_drawHeights && !normalLineVertices.empty())
        {
            const AZ::Color heightColor = AZ::Color(0.0f, 0.0f, 1.0f, 1.0f);
            const AZ::Vector3 boxHalfSize(m_configuration.m_debugQueries.m_heightPointSize / 2.0f);
            debugDisplay.SetColor(heightColor);
            for (size_t index = 0; index < normalLineVertices.size(); index += 2)
            {
                // We use SolidBox instead of Point because DX12 doesn't support point sizes, so they're too small to see.
                debugDisplay.DrawSolidBox(normalLineVertices[index] - boxHalfSize, normalLineVertices[index] + boxHalfSize);
            }
        }

        // Draw the normals
        if (m_configuration.m_debugQueries.m_drawNormals && !normalLineVertices.empty())
        {
            for (size_t index = 0; index < normalLineVertices.size(); index += 2)
            {
                AZ::Vector3 normal = (normalLineVertices[index + 1] - normalLineVertices[index]).GetNormalized();
                const AZ::Vector4 normalColor = (AZ::Vector4(normal, 1.0f) + AZ::Vector4(1.0f)) / 2.0f;
                debugDisplay.DrawLine(normalLineVertices[index], normalLineVertices[index + 1], normalColor, normalColor);
            }
        }

    }

    void TerrainWorldDebuggerComponent::DrawWireframe(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(Terrain);

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
        float heightDataResolution = 1.0f;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            heightDataResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        // Get the size of a wireframe sector in world space
        const AZ::Vector2 sectorSize = AZ::Vector2(heightDataResolution * SectorSizeInGridPoints);

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

                // If the world space box for the sector doesn't match, set it and mark the sector as dirty so we refresh the height data.
                {
                    AZStd::lock_guard<AZStd::recursive_mutex> lock(sector.m_sectorStateMutex);
                    if (sector.m_aabb != sectorAabb)
                    {
                        sector.m_aabb = sectorAabb;
                        if (worldBounds.Overlaps(sector.m_aabb))
                        {
                            sector.SetDirty();
                        }
                        else
                        {
                            // If this sector doesn't appear in the terrain world bounds, just clear it out.
                            sector.m_lineVertices.clear();
                        }
                    }
                }
            }
        }

        // Finally, for each sector, rebuild the data if it's dirty, then draw it assuming it has valid data.
        // (Sectors that are outside the world bounds won't have any valid data, so they'll get skipped)
        for (auto& sector : m_wireframeSectors)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(sector.m_sectorStateMutex);
            if (sector.m_jobContext)
            {
                // The previous async request for this sector has yet to complete.
                continue;
            }

            if (sector.m_isDirty)
            {
                RebuildSectorWireframe(sector, heightDataResolution);
            }
            else if (!sector.m_lineVertices.empty())
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
        DrawLastDirtyRegion(debugDisplay);
        DrawWireframe(viewportInfo, debugDisplay);
        DrawQueries(viewportInfo, debugDisplay);
    }

    void TerrainWorldDebuggerComponent::RebuildSectorWireframe(WireframeSector& sector, float gridResolution)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(sector.m_sectorStateMutex);
        if (!sector.m_isDirty)
        {
            return;
        }

        sector.m_isDirty = false;

        // To rebuild the wireframe for the sector, we grab all the sector vertex positions and whether or not that vertex has
        // terrain data that exists.              _
        // For each point, we add two lines in a |  shape. (inverted L)
        // We need to query one extra point in each direction so that we can get the endpoints for the final lines in each direction.
        AzFramework::Terrain::TerrainQueryRegion queryRegion(
            sector.m_aabb.GetMin(), SectorSizeInGridPoints + 1, SectorSizeInGridPoints + 1, AZ::Vector2(gridResolution));

        const size_t numSamplesX = queryRegion.m_numPointsX;
        const size_t numSamplesY = queryRegion.m_numPointsY;

        // We need 4 vertices for each grid point in our sector to hold the inverted L shape.
        sector.m_lineVertices.clear();
        sector.m_lineVertices.reserve(SectorSizeInGridPoints * SectorSizeInGridPoints * 4);

        // Clear and prepare our temporary buffers to hold all the vertex position data and "exists" flags.
        // (If we're multithreading, there's no guaranteed order to which each point will get filled in)
        sector.m_sectorVertices.clear();
        sector.m_sectorVertexExists.clear();
        sector.m_sectorVertices.resize(numSamplesX * numSamplesY);
        sector.m_sectorVertexExists.resize(numSamplesX * numSamplesY);

        // Cache off the vertex position data and "exists" flags.
        auto ProcessHeightValue = [numSamplesX, &sector]
            (size_t xIndex, size_t yIndex, const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(sector.m_sectorStateMutex);
            if (sector.m_isDirty)
            {
                // Bail out if this sector has become dirty again since the async request started.
                return;
            }

            sector.m_sectorVertices[(yIndex * numSamplesX) + xIndex] = surfacePoint.m_position;
            sector.m_sectorVertexExists[(yIndex * numSamplesX) + xIndex] = terrainExists;
        };

        // When we've finished gathering all the height data, create all the wireframe lines.
        auto completionCallback =
            [&sector, numSamplesX, numSamplesY](AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext>)
        {
            // This must happen outside the lock, otherwise we will get a deadlock if
            // WireframeSector::Reset is waiting for the completion event to be signalled.
            sector.m_jobCompletionEvent->release();

            // Reset the job context once the async request has completed,
            // clearing the way for future requests to be made for this sector.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(sector.m_sectorStateMutex);
            sector.m_jobContext.reset();

            // For each vertex in the sector, try to create the inverted L shape. We'll only draw a wireframe line
            // if both the start and the end vertex has terrain data.
            for (size_t yIndex = 0; yIndex < (numSamplesY - 1); yIndex++)
            {
                for (size_t xIndex = 0; xIndex < (numSamplesX - 1); xIndex++)
                {
                    size_t curIndex = (yIndex * numSamplesX) + xIndex;
                    size_t rightIndex = (yIndex * numSamplesX) + xIndex + 1;
                    size_t bottomIndex = ((yIndex + 1) * numSamplesX) + xIndex;

                    if (sector.m_sectorVertexExists[curIndex] && sector.m_sectorVertexExists[bottomIndex])
                    {
                        sector.m_lineVertices.emplace_back(sector.m_sectorVertices[curIndex]);
                        sector.m_lineVertices.emplace_back(sector.m_sectorVertices[bottomIndex]);
                    }

                    if (sector.m_sectorVertexExists[curIndex] && sector.m_sectorVertexExists[rightIndex])
                    {
                        sector.m_lineVertices.emplace_back(sector.m_sectorVertices[curIndex]);
                        sector.m_lineVertices.emplace_back(sector.m_sectorVertices[rightIndex]);
                    }
                }
            }

            // We're done with our temporary height buffers so clear them back out.
            sector.m_sectorVertices.clear();
            sector.m_sectorVertexExists.clear();
        };

        AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> asyncParams
            = AZStd::make_shared<AzFramework::Terrain::QueryAsyncParams>();
        asyncParams->m_completionCallback = completionCallback;

        // Only allow one thread per sector because we'll likely have multiple sectors processing at once.
        asyncParams->m_desiredNumberOfJobs = 1;

        // We can use an "EXACT" sampler here because our points are guaranteed to be aligned with terrain grid points.
        sector.m_jobCompletionEvent = AZStd::make_unique<AZStd::semaphore>();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            sector.m_jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryRegionAsync,
            queryRegion,
            AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights,
            ProcessHeightValue,
            AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT,
            asyncParams);
    }


    void TerrainWorldDebuggerComponent::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        m_lastDirtyRegion = dirtyRegion;
        m_lastDirtyData = dataChangedMask;

        if ((dataChangedMask & (TerrainDataChangedMask::Settings | TerrainDataChangedMask::HeightData)) != TerrainDataChangedMask::None)
        {
            MarkDirtySectors(dirtyRegion);

            // Any time the world bounds potentially changes, notify that the terrain debugger's visibility bounds also changed.
            AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
                &AzFramework::IEntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());
        }
    }

    TerrainWorldDebuggerComponent::WireframeSector::WireframeSector(const WireframeSector& other)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_sectorStateMutex);
        m_jobContext = other.m_jobContext;
        m_aabb = other.m_aabb;
        m_lineVertices = other.m_lineVertices;
        m_sectorVertices = other.m_sectorVertices;
        m_sectorVertexExists = other.m_sectorVertexExists;
        m_isDirty = other.m_isDirty;
    }

    TerrainWorldDebuggerComponent::WireframeSector::WireframeSector(WireframeSector&& other)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_sectorStateMutex);
        m_jobContext = AZStd::move(other.m_jobContext);
        m_aabb = AZStd::move(other.m_aabb);
        m_lineVertices = AZStd::move(other.m_lineVertices);
        m_sectorVertices = AZStd::move(other.m_sectorVertices);
        m_sectorVertexExists = AZStd::move(other.m_sectorVertexExists);
        m_isDirty = AZStd::move(other.m_isDirty);
    }

    TerrainWorldDebuggerComponent::WireframeSector& TerrainWorldDebuggerComponent::WireframeSector::operator=(const WireframeSector& other)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_sectorStateMutex);
        m_jobContext = other.m_jobContext;
        m_aabb = other.m_aabb;
        m_lineVertices = other.m_lineVertices;
        m_sectorVertices = other.m_sectorVertices;
        m_sectorVertexExists = other.m_sectorVertexExists;
        m_isDirty = other.m_isDirty;
        return *this;
    }

    TerrainWorldDebuggerComponent::WireframeSector& TerrainWorldDebuggerComponent::WireframeSector::operator=(WireframeSector&& other)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_sectorStateMutex);
        m_jobContext = AZStd::move(other.m_jobContext);
        m_aabb = AZStd::move(other.m_aabb);
        m_lineVertices = AZStd::move(other.m_lineVertices);
        m_sectorVertices = AZStd::move(other.m_sectorVertices);
        m_sectorVertexExists = AZStd::move(other.m_sectorVertexExists);
        m_isDirty = AZStd::move(other.m_isDirty);
        return *this;
    }

    void TerrainWorldDebuggerComponent::WireframeSector::Reset()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_sectorStateMutex);
        if (m_jobContext)
        {
            // Cancel the job and wait until it completes.
            m_jobContext->Cancel();
            m_jobCompletionEvent->acquire();
            m_jobCompletionEvent.reset();
            m_jobContext.reset();
        }
        m_aabb = AZ::Aabb::CreateNull();
        m_lineVertices.clear();
        m_sectorVertices.clear();
        m_sectorVertexExists.clear();
        m_isDirty = true;
    }

    void TerrainWorldDebuggerComponent::WireframeSector::SetDirty()
    {
        m_isDirty = true;
        if (m_jobContext)
        {
            m_jobContext->Cancel();
        }
    }
} // namespace Terrain
