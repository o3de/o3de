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
#include <AzFramework/Translation/TranslationDef.h>

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
                editContext->Class<EditorRecastNavigationMeshComponent>(
                    QT_TRANSLATE_NOOP("RecastNavigation", "Recast Navigation Mesh"),
                    QT_TRANSLATE_NOOP("RecastNavigation", "[Calculates the walkable navigation mesh within a specified area.]"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                editContext->Class<RecastNavigationMeshComponentController>(
                    QT_TRANSLATE_NOOP("RecastNavigation", "MeshComponentController"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationMeshComponentController::m_configuration,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Configuration"), "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                using Config = RecastNavigationMeshConfig;
                editContext->Class<RecastNavigationMeshConfig>(
                    QT_TRANSLATE_NOOP("RecastNavigation", "Recast Navigation Mesh Config"),
                    QT_TRANSLATE_NOOP("RecastNavigation", "[Navigation mesh configuration]"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    // Agent configuration
                    ->ClassElement(AZ::Edit::ClassElements::Group,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Agent Configuration"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_agentHeight,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Agent Height"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "Minimum floor to 'ceiling' height that will still allow the floor area to be considered walkable."))
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 3.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_agentMaxClimb,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Agent Max Climb"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "Maximum ledge height that is considered to still be traversable."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_agentMaxSlope,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Agent Max Slope"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The maximum slope that is considered walkable."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Max, 90.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " degrees"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_agentRadius,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Agent Radius"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The distance to erode/shrink the walkable area of the heightfield away from obstructions."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    // Editor-only configuration
                    ->ClassElement(AZ::Edit::ClassElements::Group,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Editor-only Configuration"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationMeshConfig::m_enableEditorPreview,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Editor Preview"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "If enabled, frequently calculates navigation mesh and draws in the Editor viewport."
                        "Does not affect game mode."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                    // Debug configuration
                    ->ClassElement(AZ::Edit::ClassElements::Group,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Debug Configuration"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationMeshConfig::m_enableDebugDraw,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Debug Draw"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "If enabled, draw the navigation mesh in game mode. Does not affect Editor preview."))

                    // Advanced configuration
                    ->ClassElement(AZ::Edit::ClassElements::Group,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Advanced Configuration"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_tileSize,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Tile Size"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The width/height size of tile's on the xy-plane."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_borderSize,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Border Size"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The additional dimension around the tile to collect additional geometry in order to connect to adjacent tiles."))
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 10)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " voxels"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_cellHeight,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Voxel Height"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The y-axis cell size to use for fields."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_cellSize,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Voxel Size"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The xz-plane cell size to use for fields. This defines the voxel sizes for other configuration attributes."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_detailSampleDist,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Detail Sample Distance"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "Sets the sampling distance to use when generating the detail mesh. (For height detail only.)"))
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 0.9f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))


                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_detailSampleMaxError,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Detail Sample Max Error"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The maximum distance the detail mesh surface should deviate from heightfield data. (For height detail only.)"))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_edgeMaxError,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Edge Max Error"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The maximum distance a simplified contour's border edges should deviate the original raw contour."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_edgeMaxLen,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Edge Max Length"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The maximum allowed length for contour edges along the border of the mesh."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("RecastNavigation", " world units"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_filterLedgeSpans,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Filter Ledge Spans"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "A ledge is a span with one or more neighbors whose maximum is further away than walkableClimb "
                        " from the current span's maximum."
                        " This method removes the impact of the overestimation of conservative voxelization"
                        " so the resulting mesh will not have regions hanging in the air over ledges."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_filterLowHangingObstacles,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Filter Low Hanging Obstacles"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "Allows the formation of walkable regions that will flow over low lying objects such as curbs, and up structures such as stairways. "))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_filterWalkableLowHeightSpans,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Filter Walkable Low Height Spans"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "For this filter, the clearance above the span is the distance from the span's maximum to the next higher span's minimum. (Same grid column.)"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_maxVerticesPerPoly,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Max Vertices Per Poly"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The maximum number of vertices allowed for polygons generated during the contour to polygon conversion process."))
                    ->Attribute(AZ::Edit::Attributes::Min, 3)
                        ->Attribute(AZ::Edit::Attributes::Suffix,
                            QT_TRANSLATE_NOOP("RecastNavigation", " vertices"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_regionMergeSize,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Region Merge Size"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "Any regions with a span count smaller than this value will, if possible, be merged with larger regions. [Limit: >=0]"))
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Suffix,
                            QT_TRANSLATE_NOOP("RecastNavigation", " voxels"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_regionMinSize,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Region Min Size"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "The minimum number of cells allowed to form isolated island areas."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Suffix,
                            QT_TRANSLATE_NOOP("RecastNavigation", " voxels"))
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
