/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Misc/RecastNavigationMeshConfig.h>

namespace RecastNavigation
{
    void RecastNavigationMeshConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            using Self = RecastNavigationMeshConfig;

            serialize->Class<RecastNavigationMeshConfig>()
                ->Field("Agent Height", &Self::m_agentHeight)
                ->Field("Agent Max Climb", &Self::m_agentMaxClimb)
                ->Field("Agent Max Slope", &Self::m_agentMaxSlope)
                ->Field("Agent Radius", &Self::m_agentRadius)
                ->Field("Voxel Height", &Self::m_cellHeight)
                ->Field("Voxel Size", &Self::m_cellSize)
                ->Field("Detail Sample Distance", &Self::m_detailSampleDist)
                ->Field("Detail Sample Max Error", &Self::m_detailSampleMaxError)
                ->Field("Edge Max Error", &Self::m_edgeMaxError)
                ->Field("Edge Max Length", &Self::m_edgeMaxLen)
                ->Field("Filter Ledge Spans", &Self::m_filterLedgeSpans)
                ->Field("Filter Low Hanging Obstacles", &Self::m_filterLowHangingObstacles)
                ->Field("Filter Walkable Low Height Spans", &Self::m_filterWalkableLowHeightSpans)
                ->Field("Max Vertices Per Poly", &Self::m_maxVerticesPerPoly)
                ->Field("Region Merge Size", &Self::m_regionMergeSize)
                ->Field("Region Min Size", &Self::m_regionMinSize)
                ->Field("Tile Size", &Self::m_tileSize)
                ->Field("Border Size", &Self::m_borderSize)
                ->Field("Debug Draw", &Self::m_enableDebugDraw)
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<RecastNavigationMeshConfig>("Recast Navigation Mesh Config",
                    "[Navigation mesh configuration]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(nullptr, &RecastNavigationMeshConfig::m_enableDebugDraw,
                        "Debug Draw", "If enabled, draw the navigation mesh")

                    ->DataElement(nullptr, &Self::m_tileSize, "Tile Size",
                        "The width/height size of tile's on the xy-plane.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(nullptr, &Self::m_borderSize, "Border Size",
                        "The additional dimension around the tile to collect additional geometry in order to connect to adjacent tiles.")
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 10)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " voxels")

                    ->DataElement(nullptr, &Self::m_agentHeight, "Agent Height",
                        "Minimum floor to 'ceiling' height that will still allow the floor area to be considered walkable.")
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 3.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(nullptr, &Self::m_agentMaxClimb, "Agent Max Climb",
                        "Maximum ledge height that is considered to still be traversable.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(nullptr, &Self::m_agentMaxSlope, "Agent Max Slope",
                        "The maximum slope that is considered walkable.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Max, 90.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")

                    ->DataElement(nullptr, &Self::m_agentRadius, "Agent Radius",
                        "The distance to erode/shrink the walkable area of the heightfield away from obstructions.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(nullptr, &Self::m_cellHeight, "Voxel Height",
                        "The y-axis cell size to use for fields.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(nullptr, &Self::m_cellSize, "Voxel Size",
                        "The xz-plane cell size to use for fields. This defines the voxel sizes for other configuration attributes.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(nullptr, &Self::m_detailSampleDist, "Detail Sample Distance",
                        "Sets the sampling distance to use when generating the detail mesh. (For height detail only.)")
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 0.9f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")


                    ->DataElement(nullptr, &Self::m_detailSampleMaxError, "Detail Sample Max Error",
                        "The maximum distance the detail mesh surface should deviate from heightfield data. (For height detail only.)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)

                    ->DataElement(nullptr, &Self::m_edgeMaxError, "Edge Max Error",
                        "The maximum distance a simplified contour's border edges should deviate the original raw contour.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)

                    ->DataElement(nullptr, &Self::m_edgeMaxLen, "Edge Max Length",
                        "The maximum allowed length for contour edges along the border of the mesh.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)

                    ->DataElement(nullptr, &Self::m_filterLedgeSpans, "Filter Ledge Spans",
                        "A ledge is a span with one or more neighbors whose maximum is further away than @p walkableClimb "
                        " from the current span's maximum."
                        " This method removes the impact of the overestimation of conservative voxelization"
                        " so the resulting mesh will not have regions hanging in the air over ledges.")
                    ->DataElement(nullptr, &Self::m_filterLowHangingObstacles, "Filter Low Hanging Obstacles",
                        "Allows the formation of walkable regions that will flow over low lying objects such as curbs, and up structures such as stairways. ")
                    ->DataElement(nullptr, &Self::m_filterWalkableLowHeightSpans, "Filter Walkable Low Height Spans",
                        "For this filter, the clearance above the span is the distance from the span's maximum to the next higher span's minimum. (Same grid column.)")

                    ->DataElement(nullptr, &Self::m_maxVerticesPerPoly, "Max Vertices Per Poly",
                        "The maximum number of vertices allowed for polygons generated during the contour to polygon conversion process.")
                    ->Attribute(AZ::Edit::Attributes::Min, 3)

                    ->DataElement(nullptr, &Self::m_regionMergeSize, "Region Merge Size",
                        "Any regions with a span count smaller than this value will, if possible, be merged with larger regions. [Limit: >=0]")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)

                    ->DataElement(nullptr, &Self::m_regionMinSize, "Region Min Size",
                        "The minimum number of cells allowed to form isolated island areas.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ;
            }
        }
    }
} // namespace RecastNavigation
