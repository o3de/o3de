/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * EditorWhiteBoxComponent - CORE translation unit: component lifecycle, reflection (serialize
 * + edit context), tick-driven coalescing, game-entity bake and small bus glue. The other
 * concerns live in sibling translation units:
 *   EditorWhiteBoxComponentLayers.cpp  - layer system, combine pipeline, parametric shapes
 *   EditorWhiteBoxComponentVoxel.cpp   - cube stamp gestures, cell records, mesh repair
 *   EditorWhiteBoxComponentBoolean.cpp - entity boolean (one-shot + live) and bake caches
 *   EditorWhiteBoxComponentRender.cpp  - render/physics meshes, bounds, selection, debug draw
 *   EditorWhiteBoxComponentAsset.cpp   - serialize/deserialize, Save As Asset, obj exports
 */

#include "Asset/EditorWhiteBoxMeshAsset.h"
#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentMode.h"
#include "Rendering/WhiteBoxNullRenderMesh.h"
#include "Tools/WhiteBoxLayerUtil.h"
#include "WhiteBoxComponent.h"

#include "Util/WhiteBoxMeshUtil.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/numeric.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Components/EditorWhiteBoxColliderComponent.h>

namespace WhiteBox
{
    // callback for when the default shape field is changed
    AZ::Crc32 EditorWhiteBoxComponent::OnDefaultShapeChange()
    {
        const AZStd::string entityIdStr = AZStd::string::format("%llu", static_cast<AZ::u64>(GetEntityId()));
        const AZStd::string componentIdStr = AZStd::string::format("%llu", GetId());
        const AZStd::string shapeTypeStr = AZStd::string::format("%d", aznumeric_cast<int>(m_defaultShape));
        const AZStd::vector<AZStd::string_view> scriptArgs{entityIdStr, componentIdStr, shapeTypeStr};

        // if the shape type has just changed and it is no longer an asset type, check if a mesh asset
        // is in use and clear it if so (switch back to using the component serialized White Box mesh)
        if (!DisplayingAsset(m_defaultShape) && m_editorMeshAsset->InUse())
        {
            m_editorMeshAsset->Reset();
        }

        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
            &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
            "@gemroot:WhiteBox@/Editor/Scripts/default_shapes.py", scriptArgs);

        EditorWhiteBoxComponentNotificationBus::Event(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()),
            &EditorWhiteBoxComponentNotificationBus::Events::OnDefaultShapeTypeChanged, m_defaultShape);

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::u32 EditorWhiteBoxComponent::DrawShapeData::OnShapeChange()
    {
        // Reset Draw Sides to a sensible default for the newly chosen shape:
        // angular shapes -> 4 (box / square base), round shapes -> 24 (smooth).
        switch (m_shape)
        {
        case DrawShapeType::Box:
        case DrawShapeType::Pyramid:
            m_sides = 4;
            break;
        case DrawShapeType::Cylinder:
        case DrawShapeType::Cone:
            m_sides = 24;
            break;
        case DrawShapeType::Sphere:
            m_sides = 16; // longitude segments; latitude rings derived from this
            break;
        default:
            break;
        }

        // refresh so the Draw Sides / Draw Steps fields show the new value (and
        // toggle visibility of the Sphere-only / Staircase-only controls)
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorWhiteBoxComponent::DrawShapeData::SidesVisibility() const
    {
        // Sides applies to every solid shape: it sets the footprint resolution for
        // round shapes (Cylinder/Cone), the subdivision for the Sphere, and the
        // N-gon footprint for Box/Pyramid (3 = triangular prism, 4 = box, etc.).
        // Only the Staircase ignores it.
        return m_shape == DrawShapeType::Staircase ? AZ::Edit::PropertyVisibility::Hide
                                                   : AZ::Edit::PropertyVisibility::Show;
    }

    AZ::Crc32 EditorWhiteBoxComponent::DrawShapeData::StairVisibility() const
    {
        // The whole Stair group only shows for a Staircase. Within the group the
        // step-count / step-height split is handled by DrawStairData itself.
        return m_shape == DrawShapeType::Staircase ? AZ::Edit::PropertyVisibility::Show
                                                   : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 EditorWhiteBoxComponent::DrawStairData::StepsVisibility() const
    {
        // Step count only applies in step-count mode (the group is already hidden
        // unless the draw shape is a Staircase).
        return m_byHeight ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    AZ::Crc32 EditorWhiteBoxComponent::DrawStairData::StepHeightVisibility() const
    {
        // Step height only applies in step-height mode.
        return m_byHeight ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    bool EditorWhiteBoxVersionConverter(
        AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 1)
        {
            // find the old WhiteBoxMeshAsset stored directly on the component
            AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> meshAsset;
            const int meshAssetIndex = classElement.FindElement(AZ_CRC_CE("MeshAsset"));
            if (meshAssetIndex != -1)
            {
                classElement.GetSubElement(meshAssetIndex).GetData(meshAsset);
                classElement.RemoveElement(meshAssetIndex);
            }
            else
            {
                return false;
            }

            // add the new EditorWhiteBoxMeshAsset which will contain the previous WhiteBoxMeshAsset
            const int editorMeshAssetIndex =
                classElement.AddElement<EditorWhiteBoxMeshAsset>(context, "EditorMeshAsset");

            if (editorMeshAssetIndex != -1)
            {
                // insert the existing WhiteBoxMeshAsset into the new EditorWhiteBoxMeshAsset
                classElement.GetSubElement(editorMeshAssetIndex)
                    .AddElementWithData<AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>>(context, "MeshAsset", meshAsset);
            }
            else
            {
                return false;
            }
        }

        if (classElement.GetVersion() <= 2)
        {
            // v3 grouped the flat voxel / boolean / draw fields into structs; move each old field across.

            // -- voxel / legacy grid fields -> VoxelData ("Voxel") --
            {
                EditorWhiteBoxComponent::VoxelData voxel;
                if (const int idx = classElement.FindElement(AZ_CRC_CE("VoxelCells")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(voxel.m_cells);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("VoxelCellSizes")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(voxel.m_sizes);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("VoxelCellSize")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(voxel.m_legacySize);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("VoxelMerged")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(voxel.m_legacyMerged);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("GridMeshData")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(voxel.m_legacyGridData);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("GridMergedData")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(voxel.m_legacyGridMergedData);
                    classElement.RemoveElement(idx);
                }
                classElement.AddElementWithData(context, "Voxel", voxel);
            }

            // -- boolean fields -> BooleanSettings ("Boolean"); the two mutually exclusive
            //    hide/delete bools collapse into the SourceAfterApply enum --
            {
                EditorWhiteBoxComponent::BooleanSettings boolean;
                if (const int idx = classElement.FindElement(AZ_CRC_CE("BooleanSource")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(boolean.m_sourceEntity);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("BooleanOp")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(boolean.m_operation);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("BooleanLive")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(boolean.m_live);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("BooleanAffectActive")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(boolean.m_affectActiveOnly);
                    classElement.RemoveElement(idx);
                }
                bool hideSource = false;
                bool deleteSource = false;
                if (const int idx = classElement.FindElement(AZ_CRC_CE("BooleanHideSource")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(hideSource);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("BooleanDeleteSource")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(deleteSource);
                    classElement.RemoveElement(idx);
                }
                boolean.m_sourceAfterApply =
                    deleteSource ? SourceAfterApply::Delete : (hideSource ? SourceAfterApply::Hide : SourceAfterApply::Keep);
                classElement.AddElementWithData(context, "Boolean", boolean);
            }

            // -- loose draw fields fold into the existing nested DrawShapeData element --
            {
                bool carve = false;
                bool mergeUnion = false;
                bool unitCube = false;
                float unitCubeSize = 1.0f;
                bool unitCubeShowGrid = true;
                if (const int idx = classElement.FindElement(AZ_CRC_CE("DrawCarve")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(carve);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("DrawMergeUnion")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(mergeUnion);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("DrawUnitCube")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(unitCube);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("DrawUnitCubeSize")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(unitCubeSize);
                    classElement.RemoveElement(idx);
                }
                if (const int idx = classElement.FindElement(AZ_CRC_CE("DrawUnitCubeShowGrid")); idx != -1)
                {
                    classElement.GetSubElement(idx).GetData(unitCubeShowGrid);
                    classElement.RemoveElement(idx);
                }
                if (const int dsIdx = classElement.FindElement(AZ_CRC_CE("DrawShapeData")); dsIdx != -1)
                {
                    AZ::SerializeContext::DataElementNode& drawShapeNode = classElement.GetSubElement(dsIdx);
                    drawShapeNode.AddElementWithData(context, "Carve", carve);
                    drawShapeNode.AddElementWithData(context, "MergeUnion", mergeUnion);
                    drawShapeNode.AddElementWithData(context, "UnitCube", unitCube);
                    drawShapeNode.AddElementWithData(context, "UnitCubeSize", unitCubeSize);
                    drawShapeNode.AddElementWithData(context, "UnitCubeShowGrid", unitCubeShowGrid);
                }
                // the "Merge With Mesh" feature was removed entirely
                if (const int idx = classElement.FindElement(AZ_CRC_CE("MergeGridWithMesh")); idx != -1)
                {
                    classElement.RemoveElement(idx);
                }
            }
        }

        return true;
    }

    void EditorWhiteBoxComponent::DrawStairData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DrawStairData>()
                ->Version(1)
                ->Field("ByHeight", &DrawStairData::m_byHeight)
                ->Field("Steps", &DrawStairData::m_steps)
                ->Field("StepHeight", &DrawStairData::m_stepHeight)
                ->Field("Rotation", &DrawStairData::m_rotation);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DrawStairData>(
                    QT_TRANSLATE_NOOP("WhiteBox", "Stair"),
                    QT_TRANSLATE_NOOP("WhiteBox", "Staircase-specific Draw Shape settings."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &DrawStairData::m_byHeight,
                        QT_TRANSLATE_NOOP("WhiteBox", "Stair By Step Height"),
                        QT_TRANSLATE_NOOP("WhiteBox", "When on, the Staircase is divided by a fixed step (riser) height; the step count is derived "
                        "from the pull height. When off, a fixed step count is used."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &DrawStairData::m_steps,
                        QT_TRANSLATE_NOOP("WhiteBox", "Step Count"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Number of steps the Draw Shape tool builds when the shape is a Staircase."))
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 128)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DrawStairData::StepsVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &DrawStairData::m_stepHeight,
                        QT_TRANSLATE_NOOP("WhiteBox", "Step Height"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Riser height of each step; the step count is derived from the pull height."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DrawStairData::StepHeightVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &DrawStairData::m_rotation,
                        QT_TRANSLATE_NOOP("WhiteBox", "Stair Rotation (x90)"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Orientation of the Staircase in 90-degree steps about the drawn surface. 2 (180 degrees) puts "
                        "the tall end at the corner you first clicked."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 3);
            }
        }
    }

    void EditorWhiteBoxComponent::DrawShapeData::Reflect(AZ::ReflectContext* context)
    {
        DrawStairData::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DrawShapeData>()
                ->Version(1)
                ->Field("Shape", &DrawShapeData::m_shape)
                ->Field("Sides", &DrawShapeData::m_sides)
                ->Field("Stair", &DrawShapeData::m_stair)
                ->Field("Carve", &DrawShapeData::m_carve)
                ->Field("MergeUnion", &DrawShapeData::m_mergeUnion)
                ->Field("UnitCube", &DrawShapeData::m_unitCube)
                ->Field("UnitCubeSize", &DrawShapeData::m_unitCubeSize)
                ->Field("UnitCubeShowGrid", &DrawShapeData::m_unitCubeShowGrid);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DrawShapeData>(
                    QT_TRANSLATE_NOOP("WhiteBox", "Draw Shape"),
                    QT_TRANSLATE_NOOP("WhiteBox", "Draw Shape tool settings."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &DrawShapeData::m_shape,
                        QT_TRANSLATE_NOOP("WhiteBox", "Draw Shape"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Shape the Draw Shape tool builds. Changing this resets Draw Sides to a sensible default."))
                    ->EnumAttribute(DrawShapeType::Box, QT_TRANSLATE_NOOP("WhiteBox", "Box"))
                    ->EnumAttribute(DrawShapeType::Cylinder, QT_TRANSLATE_NOOP("WhiteBox", "Cylinder"))
                    ->EnumAttribute(DrawShapeType::Pyramid, QT_TRANSLATE_NOOP("WhiteBox", "Pyramid"))
                    ->EnumAttribute(DrawShapeType::Cone, QT_TRANSLATE_NOOP("WhiteBox", "Cone"))
                    ->EnumAttribute(DrawShapeType::Sphere, QT_TRANSLATE_NOOP("WhiteBox", "Sphere"))
                    ->EnumAttribute(DrawShapeType::Staircase, QT_TRANSLATE_NOOP("WhiteBox", "Staircase"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DrawShapeData::OnShapeChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &DrawShapeData::m_sides,
                        QT_TRANSLATE_NOOP("WhiteBox", "Draw Sides"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Number of sides for round / N-gon shapes (4 = box / square), or the subdivision of the Sphere."))
                    ->Attribute(AZ::Edit::Attributes::Min, 3)
                    ->Attribute(AZ::Edit::Attributes::Max, 128)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DrawShapeData::SidesVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &DrawShapeData::m_stair,
                        QT_TRANSLATE_NOOP("WhiteBox", "Stair"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Staircase-specific settings."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DrawShapeData::StairVisibility)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void EditorWhiteBoxComponent::VoxelData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VoxelData>()
                ->Version(1)
                ->Field("Cells", &VoxelData::m_cells)
                ->Field("Sizes", &VoxelData::m_sizes)
                ->Field("LegacySize", &VoxelData::m_legacySize)
                ->Field("LegacyMerged", &VoxelData::m_legacyMerged)
                ->Field("LegacyGridData", &VoxelData::m_legacyGridData)
                ->Field("LegacyGridMergedData", &VoxelData::m_legacyGridMergedData);
        }
    }

    void EditorWhiteBoxComponent::BooleanSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BooleanSettings>()
                ->Version(2)
                ->Field("Source", &BooleanSettings::m_sourceEntity)
                ->Field("Operation", &BooleanSettings::m_operation)
                ->Field("Live", &BooleanSettings::m_live)
                ->Field("AffectActiveOnly", &BooleanSettings::m_affectActiveOnly)
                ->Field("SourceAfterApply", &BooleanSettings::m_sourceAfterApply)
                ->Field("ExcludeFromBoolean", &BooleanSettings::m_excludeFromBoolean)
                ->Field("BooleanOthers", &BooleanSettings::m_booleanOthers)
                ->Field("CutterOperation", &BooleanSettings::m_cutterOperation);
        }
    }

    void EditorWhiteBoxComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorWhiteBoxMeshAsset::Reflect(context);
        DrawShapeData::Reflect(context);
        WhiteBoxLayer::Reflect(context);
        VoxelData::Reflect(context);
        BooleanSettings::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWhiteBoxComponent, EditorComponentBase>()
                ->Version(5, &EditorWhiteBoxVersionConverter)
                ->Field("WhiteBoxData", &EditorWhiteBoxComponent::m_whiteBoxData)
                ->Field("Layers", &EditorWhiteBoxComponent::m_layers)
                ->Field("ActiveLayer", &EditorWhiteBoxComponent::m_activeLayerIndex)
                ->Field("NextLayerId", &EditorWhiteBoxComponent::m_nextLayerId)
                ->Field("DefaultShape", &EditorWhiteBoxComponent::m_defaultShape)
                ->Field("EditorMeshAsset", &EditorWhiteBoxComponent::m_editorMeshAsset)
                ->Field("Material", &EditorWhiteBoxComponent::m_material)
                ->Field("RenderData", &EditorWhiteBoxComponent::m_renderData)
                ->Field("ComponentMode", &EditorWhiteBoxComponent::m_componentModeDelegate)
                ->Field("FlipYZForExport", &EditorWhiteBoxComponent::m_flipYZForExport)
                ->Field("DrawShapeData", &EditorWhiteBoxComponent::m_drawShapeData)
                ->Field("EdgesOnly", &EditorWhiteBoxComponent::m_edgesOnly)
                ->Field("UseGlobalTint", &EditorWhiteBoxComponent::m_useGlobalTint)
                ->Field("MaterialOverride", &EditorWhiteBoxComponent::m_materialOverrideAssetId)
                ->Field("Voxel", &EditorWhiteBoxComponent::m_voxel)
                ->Field("Boolean", &EditorWhiteBoxComponent::m_boolean)
                // Whether this entity is an active global-boolean target (show the composed result by default).
                ->Field("GlobalBooleanActive", &EditorWhiteBoxComponent::m_globalBooleanActive)
                ->Field("CsgSolver", &EditorWhiteBoxComponent::m_csgSolver)
                ->Field("BakedBooleanRenderData", &EditorWhiteBoxComponent::m_bakedBooleanRenderData)
                ->Field("BakedBaseRenderData", &EditorWhiteBoxComponent::m_bakedBaseRenderData)
                // Persist the collision-filtered physics bakes too, so the game-mode / asset-processor
                // build (which runs on a clone that is never live-edited) has the FILTERED physics
                // geometry available - otherwise it falls back to the full visual mesh and the runtime
                // collider wireframe shows layers whose Collision flag is off.
                ->Field("BakedPhysicsBaseRenderData", &EditorWhiteBoxComponent::m_bakedPhysicsBaseRenderData)
                ->Field("BakedPhysicsBooleanRenderData", &EditorWhiteBoxComponent::m_bakedPhysicsBooleanRenderData)
                ->Field("PhysicsBaked", &EditorWhiteBoxComponent::m_physicsBaked);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                // The editing UI lives in the White Box pane; the card only holds per-entity settings.
                editContext->Class<EditorWhiteBoxComponent>(
                    QT_TRANSLATE_NOOP("WhiteBox", "White Box"),
                    QT_TRANSLATE_NOOP("WhiteBox", "White Box level editing"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/WhiteBox.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/WhiteBox.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/white-box/")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Open the White Box pane - all White Box editing lives there."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnOpenPane)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Open White Box Pane"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_editorMeshAsset,
                        QT_TRANSLATE_NOOP("WhiteBox", "Editor Mesh Asset"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Editor Mesh Asset"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::AssetVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_componentModeDelegate,
                        QT_TRANSLATE_NOOP("WhiteBox", "Component Mode"),
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box Tool Component Mode"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnAddCollision()
    {
        AZ::Entity* entity = GetEntity();
        if (entity == nullptr)
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }
        if (entity->FindComponent<EditorWhiteBoxColliderComponent>() != nullptr)
        {
            return AZ::Edit::PropertyRefreshLevels::None; // already has a White Box collider
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("Add White Box Collision");
        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome outcome =
            AZ::Failure(AZStd::string("uninitialized"));
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            outcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
            AzToolsFramework::EntityIdList{ GetEntityId() },
            AZ::ComponentTypeList{ azrtti_typeid<EditorWhiteBoxColliderComponent>() });
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree; // refresh the inspector to show the new component
    }

    void EditorWhiteBoxComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorWhiteBoxComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("WhiteBoxService"));
    }

    void EditorWhiteBoxComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Note: NonUniformScaleService is intentionally NOT listed - White Box supports the entity's
        // Non-Uniform Scale component (render, collider and bounds all honour it via the bus).
        incompatible.push_back(AZ_CRC_CE("MeshService"));
        incompatible.push_back(AZ_CRC_CE("WhiteBoxService"));
    }

    EditorWhiteBoxComponent::EditorWhiteBoxComponent() = default;

    EditorWhiteBoxComponent::~EditorWhiteBoxComponent()
    {
        // note: m_editorMeshAsset is (usually) serialized so it is created by the reflection system
        // in Reflect (no explicit `new`) - we must still clean-up the resource on destruction though
        // to not leak resources.
        delete m_editorMeshAsset;
    }

    void EditorWhiteBoxComponent::Init()
    {
        if (m_editorMeshAsset)
        {
            return;
        }

        // if the m_editorMeshAsset has not been created by the serialization system
        // create a new EditorWhiteBoxMeshAsset here
        m_editorMeshAsset = aznew EditorWhiteBoxMeshAsset();
    }

    void EditorWhiteBoxComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();
        const AZ::EntityComponentIdPair entityComponentIdPair{entityId, GetId()};

        AzToolsFramework::Components::EditorComponentBase::Activate();
        EditorWhiteBoxComponentRequestBus::Handler::BusConnect(entityComponentIdPair);
        EditorWhiteBoxComponentNotificationBus::Handler::BusConnect(entityComponentIdPair);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);
        AzFramework::VisibleGeometryRequestBus::Handler::BusConnect(entityId);
        // Answer scene raycasts; geometry changes are signalled from RebuildRenderMesh.
        AzFramework::RenderGeometry::IntersectionRequestBus::Handler::BusConnect(
            AzFramework::RenderGeometry::EntityIdAndContext(entityId, AzToolsFramework::GetEntityContextId()));
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(entityId);
        // Offer this mesh's vertices to the editor-wide vertex snapper.
        AzToolsFramework::ViewportSnapping::ViewportSnapSourceRequestBus::Handler::BusConnect(entityId);
        AZ::TickBus::Handler::BusConnect();

        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorWhiteBoxComponent, EditorWhiteBoxComponentMode>(
            entityComponentIdPair, this);

        m_worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

        m_editorMeshAsset->Associate(entityComponentIdPair);

        // deserialize the white box data into a mesh object or load the serialized asset ref
        // (the serialized stream already contains the full combined geometry - freeform
        // faces plus any voxel-stamped surface - so no voxel regeneration is needed here;
        // regenerating would discard freeform edits made before the level was saved).
        DeserializeWhiteBox();

        // Refresh render/physics/bounds when the entity's non-uniform scale changes (it is separate
        // from the Transform, so it does not raise OnTransformChanged).
        m_nonUniformScaleChangedHandler = AZ::NonUniformScaleChangedEvent::Handler(
            [this]([[maybe_unused]] const AZ::Vector3& scale)
            {
                m_worldAabb.reset();
                InvalidateSnapVertexCache(); // cached snap vertices fold in the non-uniform scale
                // Full rebuild: re-bakes the scale into the physics mesh and the render transform.
                RebuildWhiteBox();
            });
        AZ::NonUniformScaleRequestBus::Event(
            entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);

        // re-evaluate the live boolean and listen for the source entity moving
        UpdateBooleanSourceListener();
        EvaluateLiveBoolean();
        RebuildCombinedMesh(); // fold in the stamp/grid layer so it renders on load

        // Older levels have no collision-filtered physics bake; make it now so game mode uses it.
        if (!m_physicsBaked)
        {
            RebuildBakedRenderData();
        }

        if (AzToolsFramework::IsEntityVisible(entityId))
        {
            if (m_edgesOnly)
            {
                HideRenderMesh();
            }
            else
            {
                ShowRenderMesh();
            }
            OnMaterialChange();
        }
    }

    void EditorWhiteBoxComponent::Deactivate()
    {
        m_nonUniformScaleChangedHandler.Disconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportSnapping::ViewportSnapSourceRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzFramework::RenderGeometry::IntersectionRequestBus::Handler::BusDisconnect();
        AzFramework::VisibleGeometryRequestBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxComponentRequestBus::Handler::BusDisconnect();
        EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_booleanSourceListener.BusDisconnect();

        m_componentModeDelegate.Disconnect();
        m_editorMeshAsset->Release();
        m_layerRenderMeshes.clear();
        m_renderMesh.reset();
        m_whiteBox.reset();
        m_displayMesh.reset();
    }

    void EditorWhiteBoxComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        // The layer container's own add/remove/reorder does not always notify, so poll the id order.
        if (LayerSignature() != m_layerRuntime.m_lastSignature)
        {
            SyncLayerStructure();
        }

        // The boolean source moved: re-evaluate once per frame, deferring physics until it settles.
        if (m_rebuild.m_liveBooleanPending)
        {
            m_rebuild.m_liveBooleanPending = false;
            EvaluateLiveBoolean();
            RebuildCombinedMesh();
            RebuildRenderMesh();
            m_rebuild.m_physicsPending = true;
            m_rebuild.m_physicsTimer = 0.0f;
        }
        else if (m_rebuild.m_physicsPending)
        {
            m_rebuild.m_physicsTimer += deltaTime;
            if (m_rebuild.m_physicsTimer >= 0.25f)
            {
                m_rebuild.m_physicsPending = false;
                RebuildPhysicsMesh();
            }
        }

        // Debounced game-mode bake for drags; discrete edits bake synchronously in RebuildWhiteBox.
        if (m_rebuild.m_bakedDataDirty)
        {
            m_rebuild.m_bakedDataDelay += deltaTime;
            if (m_rebuild.m_bakedDataDelay >= 0.5f)
            {
                m_rebuild.m_bakedDataDirty = false;
                RebuildBakedRenderData();
                // The bake writes SERIALIZED members; without a dirty mark the prefab state
                // (what play-in-editor spawns from) would keep the stale previous bake.
                AzToolsFramework::ScopedUndoBatch undoBatch("White Box Bake");
                undoBatch.MarkEntityDirty(GetEntityId());
            }
        }
    }

    void EditorWhiteBoxComponent::RebuildWhiteBox()
    {
        EvaluateLiveBoolean(); // refresh m_displayMesh so render/physics/bounds use the latest result
        RebuildCombinedMesh(); // fold the stamp/grid layer into the mesh used for output
        RebuildRenderMesh();
        RebuildPhysicsMesh();

        // Bake synchronously for discrete edits: play-in-editor spawns from the serialized state captured
        // when the edit marks the entity dirty, so a debounced bake would leave game mode one edit behind.
        m_rebuild.m_bakedDataDirty = false;
        RebuildBakedRenderData();
    }

    void EditorWhiteBoxComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* whiteBoxComponent = gameEntity->CreateComponent<WhiteBoxComponent>();
        if (whiteBoxComponent == nullptr)
        {
            return;
        }

        // note: it is important no edit time only functions are called here as BuildGameEntity
        // will be called by the Asset Processor when creating dynamic slices

        // Flush a pending debounced bake so the game entity is built from current data.
        if (m_rebuild.m_bakedDataDirty)
        {
            m_rebuild.m_bakedDataDirty = false;
            RebuildBakedRenderData();
        }

        // Base render geometry: the live base mesh, else the cached base data (m_renderData may be the cut result).
        if (WhiteBoxMesh* baseMesh = GetWhiteBoxMesh())
        {
            // Per-layer tint / inverted-normal layers must bake the coloured (winding-flipped) faces
            // so the effect survives into game mode; otherwise use the plain CSG-combined faces.
            if (PerLayerRenderActive())
            {
                whiteBoxComponent->GenerateWhiteBoxMesh(BuildColoredRenderData(baseMesh));
            }
            else
            {
                // Full combine, so the game entity matches the editor view.
                const Api::WhiteBoxMeshPtr combined = BuildCombined(baseMesh);
                whiteBoxComponent->GenerateWhiteBoxMesh(
                    CreateWhiteBoxRenderData(combined ? *combined : *baseMesh, m_material));
            }
        }
        else if (!m_bakedBaseRenderData.m_faces.empty())
        {
            whiteBoxComponent->GenerateWhiteBoxMesh(m_bakedBaseRenderData);
        }
        else
        {
            whiteBoxComponent->GenerateWhiteBoxMesh(m_renderData);
        }

        // The baked physics render data drives the runtime "Draw Collider" wireframe. Once baked, empty
        // means collision off - never fall back to the visual geometry; only legacy data does.
        if (m_physicsBaked)
        {
            whiteBoxComponent->SetPhysicsGeometryData(m_bakedPhysicsBaseRenderData);
        }
        else
        {
            // Legacy data with no physics bake: fall back to the visual geometry.
            whiteBoxComponent->SetPhysicsGeometryData(m_renderData);
        }
        // The boolean variant can only be evaluated in the Editor, and its source is not resolvable during
        // the game build, so use the cached display mesh, or the serialized render data on a clone.
        WhiteBoxRenderData booleanRenderData;
        if (WhiteBoxMesh* displayMesh = GetLiveBooleanDisplayMesh())
        {
            if (PerLayerRenderActive())
            {
                booleanRenderData = BuildColoredRenderData(displayMesh);
            }
            else
            {
                const Api::WhiteBoxMeshPtr combined = CombinedWithGrid(displayMesh);
                booleanRenderData = CreateWhiteBoxRenderData(combined ? *combined : *displayMesh, m_material);
            }
        }
        else
        {
            booleanRenderData = m_bakedBooleanRenderData;
        }

        if (!booleanRenderData.m_faces.empty())
        {
            whiteBoxComponent->SetBooleanRenderData(booleanRenderData);
            // A single-source target starts live only if its Live flag is set; a global-boolean
            // target shows its composed result by default (the manual refresh is the "apply").
            whiteBoxComponent->SetLiveBooleanState(true, m_boolean.m_live || m_globalBooleanActive);

            if (!m_bakedPhysicsBooleanRenderData.m_faces.empty()) {
                whiteBoxComponent->SetBooleanPhysicsGeometryData(m_bakedPhysicsBooleanRenderData);
            }
        }
        else
        {
            whiteBoxComponent->SetLiveBooleanState(false, false);
        }
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::GetWhiteBoxMesh()
    {
        if (WhiteBoxMesh* whiteBox = m_editorMeshAsset->GetWhiteBoxMesh())
        {
            return whiteBox;
        }

        return m_whiteBox.get();
    }

    void EditorWhiteBoxComponent::OnWhiteBoxMeshModified()
    {
        // if using an asset, notify other editor mesh assets using the same id that
        // the asset has been modified, this will in turn cause all components to update
        // their render and physics meshes
        if (m_editorMeshAsset->InUse())
        {
            WhiteBoxMeshAssetNotificationBus::Event(
                m_editorMeshAsset->GetWhiteBoxMeshAssetId(),
                &WhiteBoxMeshAssetNotificationBus::Events::OnWhiteBoxMeshAssetModified,
                m_editorMeshAsset->GetWhiteBoxMeshAsset());
        }
        // otherwise, update the render and physics mesh immediately
        else
        {
            RebuildWhiteBox();
        }
    }

    void EditorWhiteBoxComponent::SetDefaultShape(const DefaultShapeType defaultShape)
    {
        m_defaultShape = defaultShape;
        OnDefaultShapeChange();
    }

    AZ::Crc32 EditorWhiteBoxComponent::CreateChildLayer()
    {
        // All of the entity/component-mode work lives in the standalone layer utility.
        CreateChildWhiteBoxLayer(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnOpenPane()
    {
        // All White Box editing UI lives in the dockable pane - this button is the only
        // affordance left on the component card (besides entering component mode).
        AzToolsFramework::OpenViewPane("White Box");
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorWhiteBoxComponent::EnterComponentMode()
    {
        // Go through the "edit selected components" request so the card's enter/exit button stays in sync.
        // The entity must be selected first.
        namespace Cmf = AzToolsFramework::ComponentModeFramework;
        Cmf::ComponentModeSystemRequestBus::Broadcast(
            &Cmf::ComponentModeSystemRequests::AddSelectedComponentModesOfType,
            azrtti_typeid<EditorWhiteBoxComponent>());
    }
} // namespace WhiteBox