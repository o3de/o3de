/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationMeshComponent.h"

#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/EditContext.h>

AZ_CVAR(
    int, ed_navmesh_updateFrequencyMs, 1000, nullptr, AZ::ConsoleFunctorFlags::Null,
    "How often to update the navigation mesh preview in the Editor (in milliseconds).");

namespace RecastNavigation
{
    void EditorRecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorRecastNavigationMeshComponent, BaseClass>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorRecastNavigationMeshComponent>("Recast Navigation Mesh",
                    "[Calculates the walkable navigation mesh within a specified area.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                editContext->Class<RecastNavigationMeshComponentController>(
                    "MeshComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationMeshComponentController::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                using Config = RecastNavigationMeshConfig;
                editContext->Class<RecastNavigationMeshConfig>("Recast Navigation Mesh Config",
                    "[Navigation mesh configuration]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    // Agent configuration
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Agent Configuration")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_agentHeight, "Agent Height",
                        "Minimum floor to 'ceiling' height that will still allow the floor area to be considered walkable.")
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 3.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_agentMaxClimb, "Agent Max Climb",
                        "Maximum ledge height that is considered to still be traversable.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_agentMaxSlope, "Agent Max Slope",
                        "The maximum slope that is considered walkable.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Max, 90.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_agentRadius, "Agent Radius",
                        "The distance to erode/shrink the walkable area of the heightfield away from obstructions.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    // Editor-only configuration
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Editor-only Configuration")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationMeshConfig::m_enableEditorPreview,
                        "Editor Preview", "If enabled, frequently calculates navigation mesh and draws in the Editor viewport."
                        "Does not affect game mode.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                    // Debug configuration
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Debug Configuration")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationMeshConfig::m_enableDebugDraw,
                        "Debug Draw", "If enabled, draw the navigation mesh in game mode. Does not affect Editor preview.")

                    // Advanced configuration
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced Configuration")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_tileSize, "Tile Size",
                        "The width/height size of tile's on the xy-plane.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_borderSize, "Border Size",
                        "The additional dimension around the tile to collect additional geometry in order to connect to adjacent tiles.")
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 10)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " voxels")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_cellHeight, "Voxel Height",
                        "The y-axis cell size to use for fields.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_cellSize, "Voxel Size",
                        "The xz-plane cell size to use for fields. This defines the voxel sizes for other configuration attributes.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_detailSampleDist, "Detail Sample Distance",
                        "Sets the sampling distance to use when generating the detail mesh. (For height detail only.)")
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 0.9f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")


                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_detailSampleMaxError, "Detail Sample Max Error",
                        "The maximum distance the detail mesh surface should deviate from heightfield data. (For height detail only.)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_edgeMaxError, "Edge Max Error",
                        "The maximum distance a simplified contour's border edges should deviate the original raw contour.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_edgeMaxLen, "Edge Max Length",
                        "The maximum allowed length for contour edges along the border of the mesh.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " world units")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_filterLedgeSpans, "Filter Ledge Spans",
                        "A ledge is a span with one or more neighbors whose maximum is further away than walkableClimb "
                        " from the current span's maximum."
                        " This method removes the impact of the overestimation of conservative voxelization"
                        " so the resulting mesh will not have regions hanging in the air over ledges.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_filterLowHangingObstacles, "Filter Low Hanging Obstacles",
                        "Allows the formation of walkable regions that will flow over low lying objects such as curbs, and up structures such as stairways. ")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_filterWalkableLowHeightSpans, "Filter Walkable Low Height Spans",
                        "For this filter, the clearance above the span is the distance from the span's maximum to the next higher span's minimum. (Same grid column.)")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_maxVerticesPerPoly, "Max Vertices Per Poly",
                        "The maximum number of vertices allowed for polygons generated during the contour to polygon conversion process.")
                    ->Attribute(AZ::Edit::Attributes::Min, 3)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " vertices")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_regionMergeSize, "Region Merge Size",
                        "Any regions with a span count smaller than this value will, if possible, be merged with larger regions. [Limit: >=0]")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " voxels")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_regionMinSize, "Region Min Size",
                        "The minimum number of cells allowed to form isolated island areas.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " voxels")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("EditorRecastNavigationMeshComponentTypeId",
                BehaviorConstant(AZ::Uuid(EditorRecastNavigationMeshComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }

    EditorRecastNavigationMeshComponent::EditorRecastNavigationMeshComponent(const RecastNavigationMeshConfig& config)
        : BaseClass(config)
    {
    }

    void EditorRecastNavigationMeshComponent::Activate()
    {
        BaseClass::Activate();
        OnConfigurationChanged();
    }

    void EditorRecastNavigationMeshComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    void EditorRecastNavigationMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        const bool saveState = m_controller.m_configuration.m_enableEditorPreview;
        m_controller.m_configuration.m_enableEditorPreview = false;
        // The game entity must query the regular game PhysX scene, while the Editor component must query the Editor PhysX scene.
        BaseClass::BuildGameEntity(gameEntity);
        m_controller.m_configuration.m_enableEditorPreview = saveState;
    }

    AZ::u32 EditorRecastNavigationMeshComponent::OnConfigurationChanged()
    {
        m_controller.CreateNavigationMesh(GetEntityId());

        if (m_controller.m_configuration.m_enableEditorPreview)
        {
            m_inEditorUpdateTick.Enqueue(AZ::TimeMs{ aznumeric_cast<int>(ed_navmesh_updateFrequencyMs) }, true);
        }
        else
        {
            m_inEditorUpdateTick.RemoveFromQueue();
        }

        BaseClass::OnConfigurationChanged();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void EditorRecastNavigationMeshComponent::OnEditorUpdateTick()
    {
        m_controller.UpdateNavigationMeshAsync();
    }

    void EditorRecastNavigationMeshComponent::SetEditorPreview(bool enable)
    {
        m_controller.m_configuration.m_enableEditorPreview = enable;
    }
} // namespace RecastNavigation
