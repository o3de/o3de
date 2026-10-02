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
#include "EditorWhiteBoxAuthoring.h"
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
#include <SubComponentModes/EditorWhiteBoxDefaultModeBus.h>
#include <SubComponentModes/EditorWhiteBoxTransformModeBus.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>

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
        case DrawShapeType::Pipe:
        case DrawShapeType::Torus:
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

    // Tube Sides was split out of Draw Sides. Seed it from the old derived value so a torus drawn
    // before the split keeps the tessellation it had.
    int LegacyTubeSidesFromSides(const int sides)
    {
        return AZStd::clamp(sides / 2, 8, 32);
    }

    bool DrawShapeDataVersionConverter(
        AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 2)
        {
            int sides = 4;
            if (const int idx = classElement.FindElement(AZ_CRC_CE("Sides")); idx != -1)
            {
                classElement.GetSubElement(idx).GetData(sides);
            }
            classElement.AddElementWithData(context, "TubeSides", LegacyTubeSidesFromSides(sides));
        }
        return true;
    }

    AZ::Crc32 EditorWhiteBoxComponent::DrawShapeData::TubeSidesVisibility() const
    {
        return m_shape == DrawShapeType::Torus ? AZ::Edit::PropertyVisibility::Show
                                               : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 EditorWhiteBoxComponent::DrawShapeData::SidesVisibility() const
    {
        // Sides applies to every solid shape: it sets the footprint resolution for
        // round shapes (Cylinder/Cone), the subdivision for the Sphere, and the
        // N-gon footprint for Box/Pyramid (3 = triangular prism, 4 = box, etc.).
        // Only the Staircase ignores it.
        return (m_shape == DrawShapeType::Staircase || m_shape == DrawShapeType::Plane || m_shape == DrawShapeType::Polygon)
            ? AZ::Edit::PropertyVisibility::Hide
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

        if (classElement.GetVersion() <= 5)
        {
            // v6 moved render data and bakes into byte-stream blobs; drop the old elements and rebake on load.
            for (const AZ::Crc32 field :
                 { AZ_CRC_CE("RenderData"), AZ_CRC_CE("BakedBooleanRenderData"), AZ_CRC_CE("BakedBaseRenderData"),
                   AZ_CRC_CE("BakedPhysicsBaseRenderData"), AZ_CRC_CE("BakedPhysicsBooleanRenderData") })
            {
                if (const int idx = classElement.FindElement(field); idx != -1)
                {
                    classElement.RemoveElement(idx);
                }
            }
            if (const int idx = classElement.FindElement(AZ_CRC_CE("PhysicsBaked")); idx != -1)
            {
                classElement.GetSubElement(idx).SetData(context, false);
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
                ->Version(4, &DrawShapeDataVersionConverter)
                ->Field("Shape", &DrawShapeData::m_shape)
                ->Field("Sides", &DrawShapeData::m_sides)
                ->Field("HoleRatio", &DrawShapeData::m_holeRatio)
                ->Field("TubeSides", &DrawShapeData::m_tubeSides)
                ->Field("Stair", &DrawShapeData::m_stair)
                ->Field("Carve", &DrawShapeData::m_carve)
                ->Field("MergeUnion", &DrawShapeData::m_mergeUnion)
                ->Field("PolygonExtrude", &DrawShapeData::m_polygonExtrude)
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
                    // Room, Door and Circular Stairs are parametric-only - they are created as a layer,
                    // never dragged out as a brush - so they are not offered here.
                    ->EnumAttribute(DrawShapeType::Plane, QT_TRANSLATE_NOOP("WhiteBox", "Plane"))
                    ->EnumAttribute(DrawShapeType::Torus, QT_TRANSLATE_NOOP("WhiteBox", "Torus"))
                    ->EnumAttribute(DrawShapeType::Pipe, QT_TRANSLATE_NOOP("WhiteBox", "Pipe"))
                    ->EnumAttribute(DrawShapeType::Polygon, QT_TRANSLATE_NOOP("WhiteBox", "Freeform Polygon"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DrawShapeData::OnShapeChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &DrawShapeData::m_sides,
                        QT_TRANSLATE_NOOP("WhiteBox", "Draw Sides"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Number of sides for round / N-gon shapes (4 = box / square), or the subdivision of the Sphere."))
                    ->Attribute(AZ::Edit::Attributes::Min, 3)
                    ->Attribute(AZ::Edit::Attributes::Max, 128)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DrawShapeData::SidesVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &DrawShapeData::m_tubeSides,
                        QT_TRANSLATE_NOOP("WhiteBox", "Tube Sides"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Segments around the torus tube's cross-section. A torus has Draw Sides x Tube Sides "
                        "quads, so this is the other half of its triangle count."))
                    ->Attribute(AZ::Edit::Attributes::Min, MinTubeSides)
                    ->Attribute(AZ::Edit::Attributes::Max, MaxTubeSides)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DrawShapeData::TubeSidesVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &DrawShapeData::m_stair, QT_TRANSLATE_NOOP("WhiteBox", "Stair"),
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

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BooleanSettings>(
                    QT_TRANSLATE_NOOP("WhiteBox", "Boolean"),
                    QT_TRANSLATE_NOOP("WhiteBox", "Cut this mesh with another entity's."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BooleanSettings::m_sourceEntity,
                        QT_TRANSLATE_NOOP("WhiteBox", "Source"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Another entity whose White Box mesh is the operand."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &BooleanSettings::m_operation,
                        QT_TRANSLATE_NOOP("WhiteBox", "Operation"),
                        QT_TRANSLATE_NOOP("WhiteBox", "How the source mesh combines with this one."))
                    ->EnumAttribute(Api::BooleanOperation::Subtraction, QT_TRANSLATE_NOOP("WhiteBox", "Subtract"))
                    ->EnumAttribute(Api::BooleanOperation::Union, QT_TRANSLATE_NOOP("WhiteBox", "Union"))
                    ->EnumAttribute(Api::BooleanOperation::Intersection, QT_TRANSLATE_NOOP("WhiteBox", "Intersect"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &BooleanSettings::m_live, QT_TRANSLATE_NOOP("WhiteBox", "Live"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Evaluate for display only; the editable mesh underneath is left alone."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &BooleanSettings::m_affectActiveOnly,
                        QT_TRANSLATE_NOOP("WhiteBox", "Active Layer Only"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Cut only the active layer instead of the whole combined mesh."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &BooleanSettings::m_booleanOthers,
                        QT_TRANSLATE_NOOP("WhiteBox", "Cut Others"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Make this entity a cutter: it booleans every overlapping entity that is not excluded."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &BooleanSettings::m_cutterOperation,
                        QT_TRANSLATE_NOOP("WhiteBox", "Cutter Operation"),
                        QT_TRANSLATE_NOOP("WhiteBox", "The operation this entity applies to the meshes it cuts."))
                    ->EnumAttribute(Api::BooleanOperation::Subtraction, QT_TRANSLATE_NOOP("WhiteBox", "Subtract"))
                    ->EnumAttribute(Api::BooleanOperation::Union, QT_TRANSLATE_NOOP("WhiteBox", "Union"))
                    ->EnumAttribute(Api::BooleanOperation::Intersection, QT_TRANSLATE_NOOP("WhiteBox", "Intersect"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &BooleanSettings::m_excludeFromBoolean,
                        QT_TRANSLATE_NOOP("WhiteBox", "Never Cut This"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Keep this entity out of every other entity's global boolean."));
            }
        }
    }

    void EditorWhiteBoxComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorWhiteBoxMeshAsset::Reflect(context);
        DrawShapeData::Reflect(context);
        WhiteBoxLayer::Reflect(context);
        TrimSheet::Reflect(context);
        VoxelData::Reflect(context);
        BooleanSettings::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWhiteBoxComponent, EditorComponentBase>()
                ->Version(6, &EditorWhiteBoxVersionConverter)
                ->Field("WhiteBoxData", &EditorWhiteBoxComponent::m_whiteBoxData)
                ->Field("Layers", &EditorWhiteBoxComponent::m_layers)
                ->Field("ActiveLayer", &EditorWhiteBoxComponent::m_activeLayerIndex)
                ->Field("NextLayerId", &EditorWhiteBoxComponent::m_nextLayerId)
                ->Field("DefaultShape", &EditorWhiteBoxComponent::m_defaultShape)
                ->Field("EditorMeshAsset", &EditorWhiteBoxComponent::m_editorMeshAsset)
                ->Field("Material", &EditorWhiteBoxComponent::m_material)
                ->Field("ComponentMode", &EditorWhiteBoxComponent::m_componentModeDelegate)
                ->Field("FlipYZForExport", &EditorWhiteBoxComponent::m_flipYZForExport)
                ->Field("DrawShapeData", &EditorWhiteBoxComponent::m_drawShapeData)
                ->Field("EdgesOnly", &EditorWhiteBoxComponent::m_edgesOnly)
                ->Field("UseGlobalTint", &EditorWhiteBoxComponent::m_useGlobalTint)
                ->Field("MaterialOverride", &EditorWhiteBoxComponent::m_materialOverrideAssetId)
                ->Field("DefaultMaterialAsset", &EditorWhiteBoxComponent::m_defaultMaterialAsset)
                ->Field("MaterialPalette", &EditorWhiteBoxComponent::m_materialPalette)
                ->Field("PaletteChoice", &EditorWhiteBoxComponent::m_paletteChoice)
                ->Field("TrimSheets", &EditorWhiteBoxComponent::m_trimSheets)
                ->Field("LightmapUvs", &EditorWhiteBoxComponent::m_lightmapUvs)
                ->Field("LightmapMargin", &EditorWhiteBoxComponent::m_lightmapMargin)
                ->Field("Voxel", &EditorWhiteBoxComponent::m_voxel)
                ->Field("Boolean", &EditorWhiteBoxComponent::m_boolean)
                // Whether this entity is an active global-boolean target (show the composed result by default).
                ->Field("GlobalBooleanActive", &EditorWhiteBoxComponent::m_globalBooleanActive)
                ->Field("CsgSolver", &EditorWhiteBoxComponent::m_csgSolver)
                // Render data and game-mode bakes, as byte streams (see m_renderDataBlob / m_bakedDataBlob).
                ->Field("RenderDataBlob", &EditorWhiteBoxComponent::m_renderDataBlob)
                ->Field("BakedDataBlob", &EditorWhiteBoxComponent::m_bakedDataBlob)
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
                        QT_TRANSLATE_NOOP("WhiteBox", "Open the White Box pane, which holds the layer stack."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnOpenPane)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Open White Box Pane"))

                    // Set-once per-entity state belongs on the card (search, collapsing, prefab overrides).
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_defaultShape,
                        QT_TRANSLATE_NOOP("WhiteBox", "Default Shape"),
                        QT_TRANSLATE_NOOP("WhiteBox", "The shape a new or reset White Box mesh starts as."))
                    ->EnumAttribute(DefaultShapeType::Cube, QT_TRANSLATE_NOOP("WhiteBox", "Cube"))
                    ->EnumAttribute(DefaultShapeType::Tetrahedron, QT_TRANSLATE_NOOP("WhiteBox", "Tetrahedron"))
                    ->EnumAttribute(DefaultShapeType::Icosahedron, QT_TRANSLATE_NOOP("WhiteBox", "Icosahedron"))
                    ->EnumAttribute(DefaultShapeType::Cylinder, QT_TRANSLATE_NOOP("WhiteBox", "Cylinder"))
                    ->EnumAttribute(DefaultShapeType::Sphere, QT_TRANSLATE_NOOP("WhiteBox", "Sphere"))
                    ->EnumAttribute(DefaultShapeType::Asset, QT_TRANSLATE_NOOP("WhiteBox", "Asset"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnDefaultShapeChange)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Material")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Add a Material component to this entity: swap any slot's material or edit this entity's own material instance."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnAddMaterial)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Add Material"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_defaultMaterialAsset,
                        QT_TRANSLATE_NOOP("WhiteBox", "Default Material"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Older default material for this entity; clear it and use the Material component instead."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::LegacyDefaultMaterialVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnDefaultMaterialChange)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Polygon Materials")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_materialPalette,
                        QT_TRANSLATE_NOOP("WhiteBox", "Materials"),
                        QT_TRANSLATE_NOOP("WhiteBox", "The materials to pick from when assigning to polygons."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnMaterialPaletteChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_paletteChoice,
                        QT_TRANSLATE_NOOP("WhiteBox", "Material"),
                        QT_TRANSLATE_NOOP("WhiteBox", "The material Assign puts on the selected polygons."))
                    ->Attribute(AZ::Edit::Attributes::StringList, &EditorWhiteBoxComponent::GetPaletteChoices)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnPaletteChoiceChange)
                    // Buttons open their own group: the inspector only places a button at a group's start, not after a field.
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Assign to Selected Polygons")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Put the chosen material on the polygons selected in edit mode. Parametric layers become editable meshes."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnAssignPaletteMaterial)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Assign"))
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Return the polygons selected in edit mode to the entity's default material."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnResetPolygonMaterial)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Reset to Default"))

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Display")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_useGlobalTint,
                        QT_TRANSLATE_NOOP("WhiteBox", "Use Global Tint"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Render every layer with the material tint below. Turn off to let each layer use its own."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnGlobalTintChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_material,
                        QT_TRANSLATE_NOOP("WhiteBox", "Material"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Tint, texture and game-mode visibility for this White Box."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::GlobalTintVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnMaterialChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_edgesOnly,
                        QT_TRANSLATE_NOOP("WhiteBox", "Edges Only"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Hide the solid render mesh and draw only the mesh edges."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnEdgesOnlyChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_lightmapUvs,
                        QT_TRANSLATE_NOOP("WhiteBox", "Lightmap UVs"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Give the mesh a second, non-overlapping UV set (UV1, exported as glTF TEXCOORD_1) for baked lighting. "
                        "Flat regions become charts packed into the unit square in proportion to their size. Without it, "
                        "UV1 repeats the texture UVs."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLightmapChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_lightmapMargin,
                        QT_TRANSLATE_NOOP("WhiteBox", "Lightmap Margin"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Gap between lightmap charts in UV units; about 2 / lightmap resolution keeps bleeding apart."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.001f)
                    ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::LightmapMarginVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLightmapChange)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Boolean")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_boolean, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Cut this mesh with another entity's mesh."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLiveBooleanChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_csgSolver,
                        QT_TRANSLATE_NOOP("WhiteBox", "Solver"),
                        QT_TRANSLATE_NOOP("WhiteBox", "CSG solver for every boolean on this component: the layer Combine modes and the entity boolean. "
                        "Fast (BSP) is brush-style and face-based; it handles inward-facing rooms, inverted normals and "
                        "open or non-manifold meshes, and suits blocky level geometry. Manifold is volumetric and precise, "
                        "but needs closed watertight meshes and treats inverted normals as the shape's complement."))
                    ->EnumAttribute(Api::CsgSolver::Manifold, QT_TRANSLATE_NOOP("WhiteBox", "Manifold"))
                    ->EnumAttribute(Api::CsgSolver::Fast, QT_TRANSLATE_NOOP("WhiteBox", "Fast (BSP)"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnCsgSolverChange)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Mesh / Asset")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Weld coincident vertices and regroup coplanar faces so the whole mesh is a clean manifold."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::FixNonManifoldMesh)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Fix Non-Manifold"))
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Remove every cube placed with the Cube Stamp tool."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ClearVoxelCubes)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Clear Cube Stamp"))
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Add a White Box collider component to this entity."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnAddCollision)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Add Collision"))
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Save this mesh as a shareable White Box asset."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::SaveAsAsset)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Save As Asset..."))
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Export this mesh to an obj file."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ExportToFile)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Export"))
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Export the rendered mesh (all layers, materials, paint and smoothing) as glTF 2.0; a .glb name writes binary. Always Y-up."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ExportToGltf)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Export glTF..."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_flipYZForExport,
                        QT_TRANSLATE_NOOP("WhiteBox", "Flip Y and Z"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Swap Y and Z on export, for tools that use a different up axis."))

                    ->ClassElement(AZ::Edit::ClassElements::Group, "")
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

    AZ::Crc32 EditorWhiteBoxComponent::OnAddMaterial()
    {
        AZ::Entity* entity = GetEntity();
        if (entity == nullptr || entity->FindComponent(AZ::Render::EditorMaterialComponentTypeId) != nullptr)
        {
            return AZ::Edit::PropertyRefreshLevels::None; // already has one
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("Add White Box Material");
        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome outcome =
            AZ::Failure(AZStd::string("uninitialized"));
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            outcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
            AzToolsFramework::EntityIdList{ GetEntityId() }, AZ::ComponentTypeList{ AZ::Render::EditorMaterialComponentTypeId });
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void EditorWhiteBoxComponent::TrimSheet::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TrimSheet>()
                ->Version(1)
                ->Field("Material", &TrimSheet::m_material)
                ->Field("Edges", &TrimSheet::m_edges);
        }
    }

    AZStd::vector<float> EditorWhiteBoxComponent::GetTrimEdges(const AZ::Data::AssetId& material) const
    {
        for (const TrimSheet& sheet : m_trimSheets)
        {
            if (sheet.m_material == material)
            {
                return sheet.m_edges;
            }
        }
        return {};
    }

    void EditorWhiteBoxComponent::SetTrimEdges(const AZ::Data::AssetId& material, AZStd::vector<float> edges)
    {
        for (float& edge : edges)
        {
            edge = AZ::GetClamp(edge, 0.0f, 1.0f);
        }
        AZStd::sort(edges.begin(), edges.end());
        edges.erase(AZStd::unique(edges.begin(), edges.end(), [](float a, float b) { return AZStd::abs(a - b) < 1e-5f; }), edges.end());
        if (edges.size() < 2)
        {
            edges.clear(); // one edge makes no band
        }
        if (GetTrimEdges(material) == edges)
        {
            return;
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Trim Sheet");
        auto sheet = AZStd::find_if(
            m_trimSheets.begin(), m_trimSheets.end(), [&material](const TrimSheet& entry) { return entry.m_material == material; });
        if (edges.empty())
        {
            if (sheet != m_trimSheets.end())
            {
                m_trimSheets.erase(sheet);
            }
        }
        else if (sheet != m_trimSheets.end())
        {
            sheet->m_edges = AZStd::move(edges);
        }
        else
        {
            TrimSheet added;
            added.m_material = material;
            added.m_edges = AZStd::move(edges);
            m_trimSheets.push_back(AZStd::move(added));
        }
        undoBatch.MarkEntityDirty(GetEntityId());
    }

    AZStd::string EditorWhiteBoxComponent::MaterialDisplayName(const AZ::Data::AssetId& material)
    {
        AZStd::string path;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(path, &AZ::Data::AssetCatalogRequests::GetAssetPathById, material);
        return path.empty() ? material.ToString<AZStd::string>() : AZStd::string(AZ::IO::PathView(path).Stem().Native());
    }

    AZStd::vector<AZStd::pair<AZStd::string, AZ::Data::AssetId>> EditorWhiteBoxComponent::GetPaletteEntries() const
    {
        // Numbered, so two materials with the same file name stay apart.
        AZStd::vector<AZStd::pair<AZStd::string, AZ::Data::AssetId>> entries;
        for (size_t i = 0; i < m_materialPalette.size(); ++i)
        {
            const AZ::Data::AssetId id = m_materialPalette[i].GetId();
            if (id.IsValid())
            {
                entries.emplace_back(AZStd::string::format("%zu. %s", i + 1, MaterialDisplayName(id).c_str()), id);
            }
        }
        return entries;
    }

    AZStd::vector<AZStd::string> EditorWhiteBoxComponent::GetPaletteChoices() const
    {
        AZStd::vector<AZStd::string> choices;
        for (const auto& entry : GetPaletteEntries())
        {
            choices.push_back(entry.first);
        }
        return choices;
    }

    AZ::Data::AssetId EditorWhiteBoxComponent::PaletteChoiceAsset() const
    {
        for (const auto& entry : GetPaletteEntries())
        {
            if (entry.first == m_paletteChoice)
            {
                return entry.second;
            }
        }
        return {};
    }

    void EditorWhiteBoxComponent::SetPaletteChoice(const AZ::Data::AssetId& material)
    {
        for (const auto& entry : GetPaletteEntries())
        {
            if (entry.second == material && entry.first != m_paletteChoice)
            {
                m_paletteChoice = entry.first;
                InvalidatePropertyDisplay(AzToolsFramework::Refresh_Values);
                return;
            }
        }
    }

    void EditorWhiteBoxComponent::AddToMaterialPalette(const AZ::Data::AssetId& material)
    {
        if (!material.IsValid())
        {
            return;
        }
        const bool listed = AZStd::any_of(
            m_materialPalette.begin(), m_materialPalette.end(),
            [&material](const AZ::Data::Asset<AZ::RPI::MaterialAsset>& entry) { return entry.GetId() == material; });
        if (!listed)
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Add Material To List");
            // NoLoad: the list only names materials; the render mesh loads whichever faces use.
            m_materialPalette.emplace_back(material, azrtti_typeid<AZ::RPI::MaterialAsset>());
            m_materialPalette.back().SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
            SetPaletteChoice(material);
            undoBatch.MarkEntityDirty(GetEntityId());
        }
        else
        {
            SetPaletteChoice(material);
        }
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
    }

    AZ::u32 EditorWhiteBoxComponent::OnMaterialPaletteChange()
    {
        // Keep the choice pointing at a listed material, and pick up a newly added one straight away.
        const AZStd::vector<AZStd::string> choices = GetPaletteChoices();
        if (AZStd::find(choices.begin(), choices.end(), m_paletteChoice) == choices.end())
        {
            m_paletteChoice = choices.empty() ? AZStd::string() : choices.back();
        }
        OnPaletteChoiceChange();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::u32 EditorWhiteBoxComponent::OnPaletteChoiceChange()
    {
        // The Paint Material brush follows the card's choice, as the card follows the brush.
        if (const AZ::Data::AssetId chosen = PaletteChoiceAsset(); chosen.IsValid())
        {
            m_facePaintSettings.m_material = chosen;
        }
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    Api::PolygonHandles EditorWhiteBoxComponent::SelectedPolygonsInEditMode() const
    {
        // Transform mode keeps its own polygon selection; the default mode has another.
        const AZ::EntityComponentIdPair pair{ GetEntityId(), GetId() };
        Api::PolygonHandles polygons;
        EditorWhiteBoxTransformModeRequestBus::EventResult(polygons, pair, &EditorWhiteBoxTransformModeRequests::GetSelectedPolygons);
        if (polygons.empty())
        {
            EditorWhiteBoxDefaultModeRequestBus::EventResult(polygons, pair, &EditorWhiteBoxDefaultModeRequests::SelectedPolygonHandles);
        }
        return polygons;
    }

    void EditorWhiteBoxComponent::AssignMaterialToSelection(const AZ::Data::AssetId& material, const char* undoLabel)
    {
        const Api::PolygonHandles polygons = SelectedPolygonsInEditMode();
        if (polygons.empty())
        {
            AZ_Warning("WhiteBox", false, "Select polygons in White Box edit mode first.");
            return;
        }
        AzToolsFramework::ScopedUndoBatch undoBatch(undoLabel);
        AssignPolygonMaterial(polygons, material);
        undoBatch.MarkEntityDirty(GetEntityId());
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnAssignPaletteMaterial()
    {
        const AZ::Data::AssetId material = PaletteChoiceAsset();
        if (!material.IsValid())
        {
            AZ_Warning("WhiteBox", false, "Add a material to the list and choose it first.");
            return AZ::Edit::PropertyRefreshLevels::None;
        }
        AssignMaterialToSelection(material, "White Box Assign Polygon Material");
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnResetPolygonMaterial()
    {
        AssignMaterialToSelection({}, "White Box Reset Polygon Material");
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnLightmapChange()
    {
        // The settings ride on the material so they reach the render mesh, the baked game data and the exporter.
        m_material.m_lightmapUvs = m_lightmapUvs;
        m_material.m_lightmapMargin = m_lightmapMargin;
        RebuildWhiteBox();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::Crc32 EditorWhiteBoxComponent::LightmapMarginVisibility() const
    {
        return m_lightmapUvs ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 EditorWhiteBoxComponent::LegacyDefaultMaterialVisibility() const
    {
        // Only shown while an older entity still carries one, so it can be cleared.
        return m_defaultMaterialAsset.GetId().IsValid() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
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
        provided.push_back(AZ_CRC_CE("MaterialConsumerService")); // lets a Material component override slots
    }

    void EditorWhiteBoxComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Note: NonUniformScaleService is intentionally NOT listed - White Box supports the entity's
        // Non-Uniform Scale component (render, collider and bounds all honour it via the bus).
        incompatible.push_back(AZ_CRC_CE("MeshService"));
        incompatible.push_back(AZ_CRC_CE("WhiteBoxService"));
        incompatible.push_back(AZ_CRC_CE("MaterialConsumerService"));
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
        m_material.m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(
            m_materialOverrideAssetId, azrtti_typeid<AZ::RPI::MaterialAsset>());
        m_material.m_materialAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        m_material.m_lightmapUvs = m_lightmapUvs;
        m_material.m_lightmapMargin = m_lightmapMargin;
        // Data saved before the card carried this control has no mirror stored, so derive it here
        // rather than trusting what was serialized.
        m_defaultMaterialAsset = m_material.m_materialAsset;
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

        // Restore the serialized render data and bakes first, for clones that never rebuild and for undo.
        UnpackRenderDataBlobs();

        // deserialize the white box data into a mesh object or load the serialized asset ref
        // (the serialized stream already contains the full combined geometry - freeform
        // faces plus any voxel-stamped surface - so no voxel regeneration is needed here;
        // regenerating would discard freeform edits made before the level was saved).
        DeserializeWhiteBox();

        // Script access (Python / MCP) to this entity's White Box values and mesh tools.
        m_authoring = AZStd::make_unique<EditorWhiteBoxAuthoring>(*this);

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
        m_authoring.reset();
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
                m_rebuild.m_bakedDataDelay = 0.0f;
                if (m_rebuild.m_workingMeshUnwritten)
                {
                    SerializeWhiteBox(); // a preview left the working mesh out of the layer's stream
                }
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
        m_rebuild.m_bakedDataDelay = 0.0f;
        m_rebuild.m_physicsPending = false; // cooked synchronously just above
        RebuildBakedRenderData();
    }

    void EditorWhiteBoxComponent::RebuildWhiteBoxDeferred()
    {
        // Update geometry and display now; the collider cook and game-mode bake wait for OnTick until edits stop.
        EvaluateLiveBoolean();
        RebuildCombinedMesh();

        m_rebuild.m_streaming = true;
        RebuildRenderMesh();
        m_rebuild.m_streaming = false;

        m_rebuild.m_physicsPending = true;
        m_rebuild.m_physicsTimer = 0.0f;
        m_rebuild.m_bakedDataDirty = true;
        m_rebuild.m_bakedDataDelay = 0.0f;
    }

    void EditorWhiteBoxComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* whiteBoxComponent = gameEntity->CreateComponent<WhiteBoxComponent>();
        if (whiteBoxComponent == nullptr)
        {
            return;
        }

        // Every fallback below reads the live render data, which on a never-activated clone exists
        // only in the serialized blobs.
        EnsureRenderDataUnpacked();

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
