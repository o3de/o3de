/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * EditorWhiteBoxComponent - LAYER SYSTEM translation unit: layer storage/loading, the
 * combine pipeline (per-layer cache + CSG combine modes), layer operations (add / delete /
 * switch / meta edits / apply transform) and parametric shape layers.
 */

#include "EditorWhiteBoxComponent.h"

#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include "Util/WhiteBoxMeshUtil.h"
#include "Util/WhiteBoxVoxelUtil.h"
#include "Viewport/WhiteBoxShapeBuilders.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace WhiteBox
{
    static bool WhiteBoxLayerVersionConverter(
        AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 5)
        {
            // v6 split the torus tube tessellation out of ParamSides. Seed it from the value the old
            // builder derived, so a torus made before the split renders identically.
            int sides = 4;
            if (const int idx = classElement.FindElement(AZ_CRC_CE("ParamSides")); idx != -1)
            {
                classElement.GetSubElement(idx).GetData(sides);
            }
            classElement.AddElementWithData(context, "ParamTubeSides", LegacyTubeSidesFromSides(sides));
        }
        return true;
    }

    void EditorWhiteBoxComponent::WhiteBoxLayer::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxLayer>()
                ->Version(7, &WhiteBoxLayerVersionConverter)
                ->Field("Name", &WhiteBoxLayer::m_name)
                ->Field("Id", &WhiteBoxLayer::m_id)
                ->Field("Visible", &WhiteBoxLayer::m_visible)
                ->Field("Collision", &WhiteBoxLayer::m_collision)
                ->Field("Parametric", &WhiteBoxLayer::m_parametric)
                ->Field("ParamShape", &WhiteBoxLayer::m_paramShape)
                ->Field("ParamWidth", &WhiteBoxLayer::m_paramWidth)
                ->Field("ParamDepth", &WhiteBoxLayer::m_paramDepth)
                ->Field("ParamHeight", &WhiteBoxLayer::m_paramHeight)
                ->Field("ParamSides", &WhiteBoxLayer::m_paramSides)
                ->Field("ParamSteps", &WhiteBoxLayer::m_paramSteps)
                ->Field("ParamStepsByHeight", &WhiteBoxLayer::m_paramStepsByHeight)
                ->Field("ParamStepHeight", &WhiteBoxLayer::m_paramStepHeight)
                ->Field("ParamWallThickness", &WhiteBoxLayer::m_paramWallThickness)
                ->Field("ParamCavityGap", &WhiteBoxLayer::m_paramCavityGap)
                ->Field("ParamFloor", &WhiteBoxLayer::m_paramFloor)
                ->Field("ParamCeiling", &WhiteBoxLayer::m_paramCeiling)
                ->Field("ParamDoorFrame", &WhiteBoxLayer::m_paramDoorFrame)
                ->Field("ParamArchHeight", &WhiteBoxLayer::m_paramArchHeight)
                ->Field("ParamInnerRadius", &WhiteBoxLayer::m_paramInnerRadius)
                ->Field("ParamSweepAngle", &WhiteBoxLayer::m_paramSweepAngle)
                ->Field("ParamHoleRatio", &WhiteBoxLayer::m_paramHoleRatio)
                ->Field("ParamTubeSides", &WhiteBoxLayer::m_paramTubeSides)
                ->Field("Tint", &WhiteBoxLayer::m_tint)
                ->Field("Combine", &WhiteBoxLayer::m_combineMode)
                ->Field("InvertNormals", &WhiteBoxLayer::m_invertNormals)
                ->Field("MirrorX", &WhiteBoxLayer::m_mirrorX)
                ->Field("MirrorY", &WhiteBoxLayer::m_mirrorY)
                ->Field("MirrorZ", &WhiteBoxLayer::m_mirrorZ)
                ->Field("ArrayCount", &WhiteBoxLayer::m_arrayCount)
                ->Field("ArrayOffset", &WhiteBoxLayer::m_arrayOffset)
                ->Field("EdgesOnly", &WhiteBoxLayer::m_edgesOnly)
                ->Field("Position", &WhiteBoxLayer::m_position)
                ->Field("Rotation", &WhiteBoxLayer::m_rotation)
                ->Field("Scale", &WhiteBoxLayer::m_scale)
                ->Field("BevelSourceParametric", &WhiteBoxLayer::m_bevelSourceParametric)
                ->Field("BevelSource", &WhiteBoxLayer::m_bevelSource)
                ->Field("BevelEdges", &WhiteBoxLayer::m_bevelEdges)
                ->Field("BevelWidth", &WhiteBoxLayer::m_bevelWidth)
                ->Field("BevelSegments", &WhiteBoxLayer::m_bevelSegments)
                ->Field("BevelProfile", &WhiteBoxLayer::m_bevelProfile)
                ->Field("Freeform", &WhiteBoxLayer::m_freeformData)
                ->Field("Grid", &WhiteBoxLayer::m_gridData)
                ->Field("GridMerged", &WhiteBoxLayer::m_gridMergedData)
                ->Field("VoxelCells", &WhiteBoxLayer::m_voxelCells)
                ->Field("VoxelCellSizes", &WhiteBoxLayer::m_voxelCellSizes)
                ->Field("VoxelMerged", &WhiteBoxLayer::m_voxelMerged);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<WhiteBoxLayer>(
                    QT_TRANSLATE_NOOP("WhiteBox", "White Box Layer"),
                    QT_TRANSLATE_NOOP("WhiteBox", "One editable White Box layer."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WhiteBoxLayer::m_name,
                    QT_TRANSLATE_NOOP("WhiteBox", "Name"),
                    QT_TRANSLATE_NOOP("WhiteBox", "Layer name."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &WhiteBoxLayer::m_visible,
                        QT_TRANSLATE_NOOP("WhiteBox", "Visible"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Show or hide this layer (hidden layers are excluded from the combined output)."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &WhiteBoxLayer::m_collision,
                        QT_TRANSLATE_NOOP("WhiteBox", "Collision"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Include or exclude this layer from the physics collision mesh."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color, &WhiteBoxLayer::m_tint, QT_TRANSLATE_NOOP("WhiteBox", "Tint"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Render colour for this layer (used when 'Use Global Tint' is off)."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &WhiteBoxLayer::m_combineMode,
                        QT_TRANSLATE_NOOP("WhiteBox", "Combine"),
                        QT_TRANSLATE_NOOP("WhiteBox", "How this layer combines with the layers below it: Separate keeps it as its own island; "
                        "Union fuses it; Subtract carves it out; Intersect keeps only the overlap."))
                    ->EnumAttribute(LayerCombineMode::Separate, QT_TRANSLATE_NOOP("WhiteBox", "Separate"))
                    ->EnumAttribute(LayerCombineMode::Union, QT_TRANSLATE_NOOP("WhiteBox", "Union"))
                    ->EnumAttribute(LayerCombineMode::Subtract, QT_TRANSLATE_NOOP("WhiteBox", "Subtract"))
                    ->EnumAttribute(LayerCombineMode::Intersect, QT_TRANSLATE_NOOP("WhiteBox", "Intersect"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &WhiteBoxLayer::m_invertNormals,
                        QT_TRANSLATE_NOOP("WhiteBox", "Invert Normals"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Render this layer inside-out (flips its normals / winding). Non-destructive and reversible; "
                        "applies to existing and new geometry in the layer."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &WhiteBoxLayer::m_position,
                        QT_TRANSLATE_NOOP("WhiteBox", "Position"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Translate this layer's geometry (applied non-destructively at combine time)."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &WhiteBoxLayer::m_rotation,
                        QT_TRANSLATE_NOOP("WhiteBox", "Rotation"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Rotate this layer's geometry, Euler degrees (XYZ)."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &WhiteBoxLayer::m_scale, QT_TRANSLATE_NOOP("WhiteBox", "Scale"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Scale this layer's geometry (non-uniform)."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.001f);
            }
        }
    }

    void EditorWhiteBoxComponent::StoreLayer(const int index)
    {
        if (index < 0 || index >= static_cast<int>(m_layers.size()))
        {
            return;
        }
        // The working streams are kept current by SerializeWhiteBox, so copy them (plus the
        // occupancy arrays) into the layer.
        WhiteBoxLayer& layer = m_layers[index];
        // Direct topology/vertex/paint edits implicitly bake a live bevel. Never
        // regenerate over hand edits from an obsolete source snapshot.
        if (!layer.m_bevelSource.empty() && layer.m_freeformData != m_whiteBoxData)
        {
            layer.m_bevelSource.clear();
            layer.m_bevelEdges.clear();
        }
        layer.m_freeformData = m_whiteBoxData;
        layer.m_gridData = m_voxel.m_legacyGridData;
        layer.m_gridMergedData.clear(); // legacy merged-grid stream, feature removed
        layer.m_voxelCells = m_voxel.m_cells;
        layer.m_voxelCellSizes = m_voxel.m_sizes;
        layer.m_voxelMerged.clear(); // legacy flags, feature removed
        m_layerRuntime.m_meshCache.erase(layer.m_id); // the stored data changed -> cached display mesh is stale
    }

    void EditorWhiteBoxComponent::LoadActiveLayer()
    {
        if (m_layers.empty())
        {
            return;
        }
        if (m_activeLayerIndex < 0)
        {
            m_activeLayerIndex = 0;
        }
        if (m_activeLayerIndex >= static_cast<int>(m_layers.size()))
        {
            m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
        }
        WhiteBoxLayer& layer = m_layers[m_activeLayerIndex];
        m_whiteBoxData = layer.m_freeformData;
        m_voxel.m_legacyGridData = layer.m_gridData;
        m_voxel.m_cells = layer.m_voxelCells;
        m_voxel.m_sizes = layer.m_voxelCellSizes;
        m_voxel.m_legacyMerged.clear(); // legacy flags, feature removed

        m_whiteBox = Api::CreateWhiteBoxMesh();
        const auto readResult = Api::ReadMesh(*m_whiteBox, m_whiteBoxData); // empty layers stay empty
        if (readResult == Api::ReadResult::Full && !layer.m_bevelSource.empty())
        {
            // OpenMesh may reorder connectivity on read; re-baseline so StoreLayer does not see an edit.
            if (Api::WriteMesh(*m_whiteBox, m_whiteBoxData)) { layer.m_freeformData = m_whiteBoxData; }
        }
        m_gridMesh = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*m_gridMesh, m_voxel.m_legacyGridData);
        m_layerRuntime.m_loadedIndex = m_activeLayerIndex;
        m_layerRuntime.m_loadedId = m_layers[m_activeLayerIndex].m_id;
        m_layerRuntime.m_lastCount = static_cast<int>(m_layers.size());
    }

    void EditorWhiteBoxComponent::ApplyTransformToMesh(
        WhiteBoxMesh& mesh, const AZ::Vector3& position, const AZ::Vector3& eulerDegrees, const AZ::Vector3& scale)
    {
        const bool identity = position.IsZero() && eulerDegrees.IsZero() &&
            scale.IsClose(AZ::Vector3::CreateOne(), 1e-6f);
        if (identity)
        {
            return; // nothing to do - leave the mesh (and its UVs/normals) untouched
        }

        const AZ::Quaternion rotation = AZ::Quaternion::CreateFromEulerAnglesDegrees(eulerDegrees);
        for (const Api::VertexHandle vertexHandle : Api::MeshVertexHandles(mesh))
        {
            AZ::Vector3 p = Api::VertexPosition(mesh, vertexHandle);
            p *= scale;                       // component-wise (non-uniform) scale
            p = rotation.TransformVector(p);  // rotate
            p += position;                    // translate
            Api::SetVertexPosition(mesh, vertexHandle, p);
        }
        Api::CalculateNormals(mesh);
        Api::CalculatePlanarUVs(mesh);
    }

    AZStd::vector<AZ::Vector3> EditorWhiteBoxComponent::GetLayerVertexPositionsEntityLocal(const int index)
    {
        if (index < 0 || index >= GetLayerCount())
        {
            return {};
        }

        const WhiteBoxLayer& layer = m_layers[index];

        // The active layer lives in the working mesh; transform its positions directly (no mesh copy).
        if (index == m_layerRuntime.m_loadedIndex)
        {
            WhiteBoxMesh* working = GetWhiteBoxMesh();
            if (working == nullptr)
            {
                return {};
            }

            const AZ::Quaternion rotation = AZ::Quaternion::CreateFromEulerAnglesDegrees(layer.m_rotation);
            AZStd::vector<AZ::Vector3> positions = Api::MeshVertexPositions(*working);
            for (AZ::Vector3& position : positions)
            {
                position = layer.m_position + rotation.TransformVector(position * layer.m_scale);
            }
            return positions;
        }

        // Non-active layers: BuildLayerMesh already returns the transformed display mesh.
        if (const Api::WhiteBoxMeshPtr layerMesh = BuildLayerMesh(layer))
        {
            return Api::MeshVertexPositions(*layerMesh);
        }

        return {};
    }

    Api::WhiteBoxMeshPtr EditorWhiteBoxComponent::BuildLayerMesh(const WhiteBoxLayer& layer)
    {
        // Deserialize a stored layer and combine its geometry into one display mesh.
        Api::WhiteBoxMeshPtr freeform = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*freeform, layer.m_freeformData);
        Api::WhiteBoxMeshPtr grid = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*grid, layer.m_gridData);
        if (!layer.m_gridMergedData.empty())
        {
            // Legacy "Merge With Mesh" cubes (feature removed): fold them into the grid.
            Api::WhiteBoxMeshPtr gridMerged = Api::CreateWhiteBoxMesh();
            Api::ReadMesh(*gridMerged, layer.m_gridMergedData);
            if (!Api::MeshFaceHandles(*gridMerged).empty())
            {
                AppendMesh(*grid, *gridMerged);
            }
        }
        Api::WhiteBoxMeshPtr combined = ApplyLayerModifiers(CombineFreeformAndGrids(freeform.get(), grid.get()), layer);
        if (combined)
        {
            ApplyTransformToMesh(*combined, layer.m_position, layer.m_rotation, layer.m_scale);
            if (layer.m_invertNormals)
            {
                // Flip the ACTUAL winding (not just the render faces) so rendering, physics
                // cooking and selection all agree - an inverted "room" collides from inside.
                combined = FlippedMeshWinding(*combined);
            }
        }
        return combined;
    }

    AZStd::vector<AZStd::pair<int, AZStd::string>> EditorWhiteBoxComponent::GetLayerNames()
    {
        AZStd::vector<AZStd::pair<int, AZStd::string>> names;
        if (m_layers.empty())
        {
            // A placeholder keeps the combo valid (an empty list selects index -1 and logs).
            names.emplace_back(0, AZStd::string("(no layers)"));
            return names;
        }
        names.reserve(m_layers.size());
        for (int i = 0; i < static_cast<int>(m_layers.size()); ++i)
        {
            const AZStd::string& name = m_layers[i].m_name;
            names.emplace_back(i, AZStd::string::format("%d: %s", i, name.empty() ? "Layer" : name.c_str()));
        }
        return names;
    }

    AZ::u32 EditorWhiteBoxComponent::OnActiveLayerChange()
    {
        // With no layers there is nothing to switch to; force a tree rebuild so a stale dropdown
        // (still listing deleted layers) is replaced by the "(no layers)" placeholder.
        if (m_layers.empty())
        {
            m_activeLayerIndex = 0;
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }
        const int incoming = m_activeLayerIndex;
        if (m_activeLayerIndex < 0)
        {
            m_activeLayerIndex = 0;
        }
        if (m_activeLayerIndex >= static_cast<int>(m_layers.size()))
        {
            m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
        }
        // The combo box can briefly hold entries for layers that were just deleted. If the picked
        // index was out of range we clamped it, so rebuild the tree to resync the dropdown.
        const bool wasStale = m_activeLayerIndex != incoming;
        const AZ::u32 refresh = wasStale ? AZ::Edit::PropertyRefreshLevels::EntireTree
                                         : AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        if (m_activeLayerIndex == m_layerRuntime.m_loadedIndex)
        {
            return refresh;
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("Switch White Box Layer");
        SerializeWhiteBox(); // commit the currently loaded layer into m_layers
        LoadActiveLayer();   // load the newly selected layer into the working members
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        return refresh;
    }

    int EditorWhiteBoxComponent::AddParametricShapeLayer(const DrawShapeType shape)
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Add Parametric Shape");
        SerializeWhiteBox(); // commit the current layer

        WhiteBoxLayer layer;
        layer.m_name =
            AZStd::string::format("%s %d", Detail::DrawShapeName(shape), static_cast<int>(m_layers.size()) + 1);
        layer.m_id = AllocLayerId();
        layer.m_parametric = true;
        layer.m_paramShape = shape;
        // Shape-appropriate defaults (mirrors the Draw Shape side-count defaults).
        switch (shape)
        {
        case DrawShapeType::Cylinder:
        case DrawShapeType::Cone:
        case DrawShapeType::Pipe:
        case DrawShapeType::Torus:
            layer.m_paramSides = 24;
            break;
        case DrawShapeType::Sphere:
            layer.m_paramSides = 16;
            break;
        case DrawShapeType::Room:
            // A 1x1x1 interior is uselessly small for a room; start with a walkable box.
            layer.m_paramSides = 4;
            layer.m_paramWidth = 4.0f;
            layer.m_paramDepth = 4.0f;
            layer.m_paramHeight = 3.0f;
            break;
        case DrawShapeType::Door:
            layer.m_paramWidth = 1.0f;
            layer.m_paramDepth = 0.2f;
            layer.m_paramHeight = 2.1f;
            layer.m_paramSides = 16;
            break;
        case DrawShapeType::CircularStairs:
            layer.m_paramWidth = 1.0f;
            layer.m_paramHeight = 3.0f;
            layer.m_paramSteps = 16;
            break;
        default:
            layer.m_paramSides = 4;
            break;
        }

        if (shape == DrawShapeType::Torus)
        {
            layer.m_paramHeight = 0.25f;
            layer.m_paramTubeSides = DefaultTubeSides;
        }

        const Api::WhiteBoxMeshPtr mesh = BuildParametricShapeMesh(
            layer.m_paramShape, layer.m_paramWidth, layer.m_paramDepth, layer.m_paramHeight, layer.m_paramSides,
            layer.m_paramSteps, layer.m_paramWallThickness, layer.m_paramCavityGap, layer.m_paramFloor,
            layer.m_paramCeiling, layer.m_paramDoorFrame, layer.m_paramArchHeight,
            layer.m_paramInnerRadius, layer.m_paramSweepAngle, layer.m_paramStepsByHeight, layer.m_paramStepHeight,
            layer.m_paramHoleRatio, layer.m_paramTubeSides);
        Api::WriteMesh(*mesh, layer.m_freeformData);

        m_layers.push_back(AZStd::move(layer));
        m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
        m_layerRuntime.m_lastCount = static_cast<int>(m_layers.size());
        LoadActiveLayer(); // make the new shape the edit target
        m_layerRuntime.m_lastSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        return m_activeLayerIndex;
    }

    int EditorWhiteBoxComponent::AddParametricShapeLayer(
        const ShapeParams& params, const AZ::Vector3& position, const AZ::Vector3& rotationDegrees,
        const LayerCombineMode combineMode)
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Draw Parametric Shape");
        SerializeWhiteBox(); // commit the current layer

        WhiteBoxLayer layer;
        layer.m_name =
            AZStd::string::format("%s %d", Detail::DrawShapeName(params.m_shape), static_cast<int>(m_layers.size()) + 1);
        layer.m_id = AllocLayerId();
        layer.m_parametric = true;
        layer.m_combineMode = combineMode;
        layer.m_position = position;
        layer.m_rotation = rotationDegrees;
        m_layers.push_back(AZStd::move(layer));

        const int index = static_cast<int>(m_layers.size()) - 1;
        StoreLayerShapeParams(index, params); // clamps
        const ShapeParams stored = GetLayerShapeParams(index);
        const Api::WhiteBoxMeshPtr mesh = BuildParametricShapeMesh(
            stored.m_shape, stored.m_width, stored.m_depth, stored.m_height, stored.m_sides, stored.m_steps,
            stored.m_wallThickness, stored.m_cavityGap, stored.m_floor, stored.m_ceiling, stored.m_doorFrame,
            stored.m_archHeight, stored.m_innerRadius, stored.m_sweepAngle, stored.m_stepsByHeight,
            stored.m_stepHeight, stored.m_holeRatio, stored.m_tubeSides);
        Api::WriteMesh(*mesh, m_layers[index].m_freeformData);

        m_activeLayerIndex = index;
        m_layerRuntime.m_lastCount = static_cast<int>(m_layers.size());
        LoadActiveLayer();
        m_layerRuntime.m_lastSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        EditorWhiteBoxComponentNotificationBus::Event(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()), &EditorWhiteBoxComponentNotificationBus::Events::OnLayerStructureChanged);
        return index;
    }

    EditorWhiteBoxComponent::ShapeParams EditorWhiteBoxComponent::GetLayerShapeParams(const int index) const
    {
        ShapeParams params;
        if (index >= 0 && index < GetLayerCount())
        {
            const WhiteBoxLayer& layer = m_layers[index];
            params.m_shape = layer.m_paramShape;
            params.m_width = layer.m_paramWidth;
            params.m_depth = layer.m_paramDepth;
            params.m_height = layer.m_paramHeight;
            params.m_sides = layer.m_paramSides;
            params.m_steps = layer.m_paramSteps;
            params.m_stepsByHeight = layer.m_paramStepsByHeight;
            params.m_stepHeight = layer.m_paramStepHeight;
            params.m_wallThickness = layer.m_paramWallThickness;
            params.m_cavityGap = layer.m_paramCavityGap;
            params.m_floor = layer.m_paramFloor;
            params.m_ceiling = layer.m_paramCeiling;
            params.m_doorFrame = layer.m_paramDoorFrame;
            params.m_archHeight = layer.m_paramArchHeight;
            params.m_innerRadius = layer.m_paramInnerRadius;
            params.m_sweepAngle = layer.m_paramSweepAngle;
            params.m_holeRatio = layer.m_paramHoleRatio;
            params.m_tubeSides = layer.m_paramTubeSides;
        }
        return params;
    }

    void EditorWhiteBoxComponent::SetLayerShapeParams(const int index, const ShapeParams& params)
    {
        if (StoreLayerShapeParams(index, params))
        {
            RegenerateParametricLayer(index);
        }
    }

    void EditorWhiteBoxComponent::SetLayerShapeParamsPreview(const int index, const ShapeParams& params)
    {
        if (StoreLayerShapeParams(index, params))
        {
            RegenerateParametricLayer(index, false);
        }
    }

    bool EditorWhiteBoxComponent::StoreLayerShapeParams(const int index, const ShapeParams& params)
    {
        if (!IsLayerParametric(index))
        {
            return false;
        }
        WhiteBoxLayer& layer = m_layers[index];
        layer.m_paramShape = params.m_shape;
        layer.m_paramWidth = AZStd::max(params.m_width, 0.01f);
        layer.m_paramDepth = AZStd::max(params.m_depth, 0.01f);
        layer.m_paramHeight = AZStd::max(params.m_height, 0.01f);
        layer.m_paramSides = AZStd::clamp(params.m_sides, 3, 128);
        layer.m_paramSteps = AZStd::clamp(params.m_steps, 1, 128);
        layer.m_paramStepsByHeight = params.m_stepsByHeight;
        layer.m_paramStepHeight = AZStd::max(params.m_stepHeight, 0.001f);
        layer.m_paramWallThickness = AZStd::max(params.m_wallThickness, 0.001f);
        layer.m_paramCavityGap = AZStd::max(params.m_cavityGap, 0.0f);
        layer.m_paramFloor = params.m_floor;
        layer.m_paramCeiling = params.m_ceiling;
        layer.m_paramDoorFrame = params.m_doorFrame;
        layer.m_paramArchHeight = AZStd::clamp(params.m_archHeight, 0.0f, layer.m_paramHeight - 0.001f);
        layer.m_paramInnerRadius = AZStd::max(params.m_innerRadius, 0.01f);
        layer.m_paramSweepAngle = AZStd::clamp(params.m_sweepAngle, 1.0f, 360.0f);
        layer.m_paramHoleRatio = AZStd::clamp(params.m_holeRatio, 0.05f, 0.95f);
        layer.m_paramTubeSides = AZStd::clamp(params.m_tubeSides, MinTubeSides, MaxTubeSides);
        return true;
    }

    void EditorWhiteBoxComponent::BakeParametricLayer(const int index)
    {
        if (IsLayerParametric(index))
        {
            // The generated mesh stays exactly as-is; the parameters just stop driving it,
            // which makes vertex-level edits safe (nothing will regenerate over them).
            m_layers[index].m_parametric = false;
        }
    }

    bool EditorWhiteBoxComponent::HasActiveBevel() const
    {
        return m_activeLayerIndex >= 0 && m_activeLayerIndex < GetLayerCount() &&
            !m_layers[m_activeLayerIndex].m_bevelSource.empty();
    }

    EditorWhiteBoxComponent::BevelParams EditorWhiteBoxComponent::GetBevelParams() const
    {
        if (!HasActiveBevel()) { return m_pendingBevelParams; }
        const auto& layer = m_layers[m_activeLayerIndex];
        return {layer.m_bevelWidth, layer.m_bevelSegments, layer.m_bevelProfile};
    }

    bool EditorWhiteBoxComponent::SetParametricBevel(
        const Api::EdgeHandles& edges, const BevelParams& params, AZStd::string& error)
    {
        if (!GetWhiteBoxMesh()) { error = "No active mesh."; return false; }
        SerializeWhiteBox();
        if (m_activeLayerIndex < 0 || m_activeLayerIndex >= GetLayerCount())
        {
            error = "Select a mesh layer."; return false;
        }
        auto& layer = m_layers[m_activeLayerIndex];
        const bool updating = !layer.m_bevelSource.empty();
        if (updating && !edges.empty()) { error = "Bake or cancel the current bevel before starting another."; return false; }
        auto source = layer.m_bevelSource;
        auto indices = layer.m_bevelEdges;
        auto result = Api::CreateWhiteBoxMesh();
        if (!updating)
        {
            if (edges.empty()) { error = "Select edges or polygons in Transform mode."; return false; }
            if (!Api::WriteMesh(*GetWhiteBoxMesh(), source) || Api::ReadMesh(*result, source) != Api::ReadResult::Full)
            {
                error = "Could not save the bevel source."; return false;
            }
            // Resolve the selected edges on the serialized source. OpenMesh can
            // reorder edges during a stream round trip.
            const auto originalEdges = Api::MeshEdgeHandles(*GetWhiteBoxMesh());
            const auto sourceEdges = Api::MeshEdgeHandles(*result);
            for (const auto edge : edges)
            {
                if (AZStd::find(originalEdges.begin(), originalEdges.end(), edge) == originalEdges.end())
                {
                    error = "The edge selection is stale."; return false;
                }
                const auto points = Api::EdgeVertexPositions(*GetWhiteBoxMesh(), edge);
                Api::EdgeHandle match;
                for (const auto candidate : sourceEdges)
                {
                    const auto pair = Api::EdgeVertexPositions(*result, candidate);
                    if ((points[0].IsClose(pair[0], 1e-6f) && points[1].IsClose(pair[1], 1e-6f)) ||
                        (points[0].IsClose(pair[1], 1e-6f) && points[1].IsClose(pair[0], 1e-6f)))
                    {
                        if (match.IsValid()) { error = "Overlapping duplicate edges make this selection ambiguous."; return false; }
                        match = candidate;
                    }
                }
                if (!match.IsValid()) { error = "Could not restore the selected edge in the bevel source."; return false; }
                indices.push_back(match.Index());
            }
        }
        else if (Api::ReadMesh(*result, source) != Api::ReadResult::Full)
        {
            error = "Could not restore the saved bevel source."; return false;
        }
        Api::EdgeHandles selection;
        for (const int index : indices) { selection.push_back(Api::EdgeHandle{index}); }
        if (!Api::BevelEdges(*result, selection, params.m_width, params.m_segments, error, params.m_profile)) { return false; }
        Api::WhiteBoxMeshStream output;
        if (!Api::WriteMesh(*result, output)) { error = "Could not save the bevel result."; return false; }
        // Everything above operated on a disposable mesh; only now commit.
        if (Api::ReadMesh(*GetWhiteBoxMesh(), output) != Api::ReadResult::Full)
        {
            error = "Could not load the bevel result."; return false;
        }
        if (!updating) { layer.m_bevelSourceParametric = layer.m_parametric; }
        layer.m_parametric = false;
        layer.m_bevelSource = AZStd::move(source);
        layer.m_bevelEdges = AZStd::move(indices);
        layer.m_bevelWidth = params.m_width;
        layer.m_bevelSegments = params.m_segments;
        layer.m_bevelProfile = params.m_profile;
        // Keep StoreLayer's direct-edit detection from baking our own update.
        Api::WriteMesh(*GetWhiteBoxMesh(), layer.m_freeformData);
        SerializeWhiteBox();
        RefreshComponentMode();
        if (updating) { RebuildWhiteBoxDeferred(); }
        else { RebuildWhiteBox(); }
        return true;
    }

    void EditorWhiteBoxComponent::BakeBevel()
    {
        if (!HasActiveBevel()) { return; }
        auto& layer = m_layers[m_activeLayerIndex];
        layer.m_bevelSource.clear();
        layer.m_bevelEdges.clear();
        SerializeWhiteBox();
        RebuildWhiteBox();
    }

    void EditorWhiteBoxComponent::CancelBevel()
    {
        if (!HasActiveBevel()) { return; }
        auto& layer = m_layers[m_activeLayerIndex];
        if (Api::ReadMesh(*GetWhiteBoxMesh(), layer.m_bevelSource) != Api::ReadResult::Full) { return; }
        layer.m_parametric = layer.m_bevelSourceParametric;
        layer.m_bevelSource.clear();
        layer.m_bevelEdges.clear();
        SerializeWhiteBox();
        RefreshComponentMode();
        RebuildWhiteBox();
    }

    void EditorWhiteBoxComponent::RegenerateParametricLayer(const int index, const bool commit)
    {
        if (!IsLayerParametric(index))
        {
            return;
        }
        WhiteBoxLayer& layer = m_layers[index];
        const ShapeParams params = GetLayerShapeParams(index);
        Api::WhiteBoxMeshPtr mesh = BuildParametricShapeMesh(
            params.m_shape, params.m_width, params.m_depth, params.m_height, params.m_sides, params.m_steps,
            params.m_wallThickness, params.m_cavityGap, params.m_floor, params.m_ceiling,
            params.m_doorFrame, params.m_archHeight, params.m_innerRadius, params.m_sweepAngle,
            params.m_stepsByHeight, params.m_stepHeight, params.m_holeRatio, params.m_tubeSides);
        if (index == m_layerRuntime.m_loadedIndex)
        {
            // Active layer: replace the working mesh; a preview skips the stream write, the commit does it.
            m_whiteBox = AZStd::move(mesh);
            if (commit)
            {
                SerializeWhiteBox();
            }
            else
            {
                m_rebuild.m_workingMeshUnwritten = true;
            }
        }
        else
        {
            Api::WriteMesh(*mesh, layer.m_freeformData);
        }
        m_layerRuntime.m_meshCache.erase(layer.m_id); // the cached display mesh is stale now
        if (commit)
        {
            RebuildWhiteBox();
        }
        else
        {
            RebuildWhiteBoxDeferred();
        }
        RefreshComponentMode();
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnNewLayer()
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("New White Box Layer");
        SerializeWhiteBox(); // commit the current layer
        WhiteBoxLayer layer;
        layer.m_name = AZStd::string::format("Layer %d", static_cast<int>(m_layers.size()) + 1);
        layer.m_id = AllocLayerId();
        m_layers.push_back(AZStd::move(layer));
        m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
        m_layerRuntime.m_lastCount = static_cast<int>(m_layers.size());
        LoadActiveLayer(); // load the new empty layer
        m_layerRuntime.m_lastSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZStd::string EditorWhiteBoxComponent::UniqueLayerName(const AZStd::string& base, const char* suffix) const
    {
        const auto taken = [this](const AZStd::string& name)
        {
            return AZStd::any_of(
                m_layers.begin(), m_layers.end(),
                [&name](const WhiteBoxLayer& layer)
                {
                    return layer.m_name == name;
                });
        };

        AZStd::string candidate = base + suffix;
        for (int number = 2; taken(candidate); ++number)
        {
            candidate = AZStd::string::format("%s%s %d", base.c_str(), suffix, number);
        }
        return candidate;
    }

    bool EditorWhiteBoxComponent::DetachPolygonsToLayer(const Api::PolygonHandles& polygons, AZStd::string& error)
    {
        if (m_activeLayerIndex < 0 || m_activeLayerIndex >= static_cast<int>(m_layers.size()) || !m_whiteBox)
        {
            error = "There is no active layer to detach from.";
            return false;
        }
        if (AssetInUse())
        {
            error = "This mesh lives in a shared White Box asset, which has no layers. Detach needs a component-stored mesh.";
            return false;
        }
        Api::WhiteBoxMeshPtr detached = Api::CreateWhiteBoxMesh();
        if (!Api::DetachPolygons(*m_whiteBox, polygons, *detached, error))
        {
            return false;
        }
        // The source lost faces, so its shape parameters no longer describe it.
        BakeParametricLayer(m_activeLayerIndex);
        SerializeWhiteBox(); // commit the reduced source layer

        // Same placement and look as the source, so nothing moves on screen; only the geometry is new.
        const WhiteBoxLayer& source = m_layers[m_activeLayerIndex];
        WhiteBoxLayer layer;
        layer.m_name = UniqueLayerName(source.m_name, " detached");
        layer.m_id = AllocLayerId();
        layer.m_visible = source.m_visible;
        layer.m_collision = source.m_collision;
        layer.m_tint = source.m_tint;
        layer.m_combineMode = source.m_combineMode;
        layer.m_invertNormals = source.m_invertNormals;
        layer.m_edgesOnly = source.m_edgesOnly;
        layer.m_position = source.m_position;
        layer.m_rotation = source.m_rotation;
        layer.m_scale = source.m_scale;
        Api::WriteMesh(*detached, layer.m_freeformData);
        m_layers.insert(m_layers.begin() + m_activeLayerIndex + 1, AZStd::move(layer));

        ++m_activeLayerIndex;
        m_layerRuntime.m_lastCount = static_cast<int>(m_layers.size());
        m_layerRuntime.m_loadedIndex = -1; // the working members still hold the source
        LoadActiveLayer();
        m_layerRuntime.m_lastSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        EditorWhiteBoxComponentNotificationBus::Event(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()), &EditorWhiteBoxComponentNotificationBus::Events::OnLayerStructureChanged);
        return true;
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnDuplicateLayer()
    {
        if (m_activeLayerIndex < 0 || m_activeLayerIndex >= static_cast<int>(m_layers.size()))
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AzToolsFramework::ScopedUndoBatch undoBatch("Duplicate White Box Layer");
        SerializeWhiteBox(); // the active layer's edits live in the working mesh until this runs

        // Every member of a layer is a value, mesh streams included, so the copy is the whole thing -
        // geometry, transform, combine mode, parametric shape and any live bevel.
        WhiteBoxLayer copy = m_layers[m_activeLayerIndex];
        copy.m_id = AllocLayerId(); // ...except the id, which has to stay unique
        copy.m_name = UniqueLayerName(m_layers[m_activeLayerIndex].m_name);
        m_layers.insert(m_layers.begin() + m_activeLayerIndex + 1, AZStd::move(copy));

        // Leave the copy active: duplicating is nearly always the first half of "and now change this
        // one", and the original is one click away.
        ++m_activeLayerIndex;
        m_layerRuntime.m_lastCount = static_cast<int>(m_layers.size());
        m_layerRuntime.m_loadedIndex = -1; // the working members still hold the original
        LoadActiveLayer();
        m_layerRuntime.m_lastSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    EditorWhiteBoxComponent* EditorWhiteBoxComponent::CloneWithSingleLayer(const int index)
    {
        if (index < 0 || index >= GetLayerCount() || AssetInUse())
        {
            return nullptr;
        }
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (serializeContext == nullptr)
        {
            return nullptr;
        }

        SerializeWhiteBox(); // flush the active layer's working mesh
        EditorWhiteBoxComponent* clone = serializeContext->CloneObject(this);
        if (clone == nullptr)
        {
            return nullptr;
        }
        clone->SetId(AZ::InvalidComponentId);

        WhiteBoxLayer layer = m_layers[index];
        clone->m_layers.clear();
        clone->m_layers.push_back(AZStd::move(layer));
        clone->m_activeLayerIndex = 0;
        clone->m_whiteBoxData = clone->m_layers[0].m_freeformData;

        // The live boolean and global-target state belong to the original entity; cutter/exclude flags carry over.
        clone->m_boolean.m_sourceEntity = AZ::EntityId();
        clone->m_boolean.m_live = false;
        clone->m_boolean.m_affectActiveOnly = false;
        clone->m_globalBooleanActive = false;

        // Cached render data/bakes are the original's combined result; the copy rebuilds its own.
        clone->m_renderDataBlob.clear();
        clone->m_bakedDataBlob.clear();
        clone->m_physicsBaked = false;
        return clone;
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnDeleteLayer()
    {
        if (m_layers.empty() || m_activeLayerIndex < 0 || m_activeLayerIndex >= static_cast<int>(m_layers.size()))
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("Delete White Box Layer");
        m_layers.erase(m_layers.begin() + m_activeLayerIndex);
        m_layerRuntime.m_lastCount = static_cast<int>(m_layers.size());
        if (m_layers.empty())
        {
            // Deleting the last layer leaves an empty white box (nothing renders, stamps cleared).
            m_activeLayerIndex = 0;
            ClearWorkingLayer();
        }
        else
        {
            if (m_activeLayerIndex >= static_cast<int>(m_layers.size()))
            {
                m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
            }
            m_layerRuntime.m_loadedIndex = -1; // force a reload of the working members
            LoadActiveLayer();
        }
        m_layerRuntime.m_lastSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void EditorWhiteBoxComponent::MoveLayer(const int from, const int to)
    {
        const int count = static_cast<int>(m_layers.size());
        if (from < 0 || from >= count || to < 0 || to >= count || from == to)
        {
            return;
        }

        AzToolsFramework::ScopedUndoBatch undoBatch("Reorder White Box Layer");
        // The working members hold the active layer's live edits; flush them into m_layers
        // before the vector shuffles, otherwise the move would drop them.
        SerializeWhiteBox();

        WhiteBoxLayer moved = AZStd::move(m_layers[from]);
        m_layers.erase(m_layers.begin() + from);
        m_layers.insert(m_layers.begin() + to, AZStd::move(moved));

        // Re-finds the edited layer by id, recombines, refreshes component mode and notifies the pane.
        SyncLayerStructure();
        undoBatch.MarkEntityDirty(GetEntityId());
    }

    AZ::u32 EditorWhiteBoxComponent::OnLayersMetaChanged()
    {
        // If the id order/count changed, the list was added-to, removed-from or reordered (some
        // O3DE versions deliver these via this ChangeNotify) - do the full structural resync.
        if (LayerSignature() != m_layerRuntime.m_lastSignature)
        {
            SyncLayerStructure();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        // Otherwise this is an in-place edit (name / visibility / combine mode / transform). Keep
        // the working index pointing at the loaded layer and recombine to reflect the change.
        const int k = IndexOfLayerId(m_layerRuntime.m_loadedId);
        if (k >= 0)
        {
            m_layerRuntime.m_loadedIndex = k;
        }
        RebuildWhiteBox();
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::Transform EditorWhiteBoxComponent::GetActiveLayerTransform() const
    {
        const int index = GetActiveLayerIndex();
        if (index < 0 || index >= GetLayerCount())
        {
            return AZ::Transform::CreateIdentity();
        }

        const WhiteBoxLayer& layer = m_layers[index];
        // Same order ApplyTransformToMesh uses: scale, then rotate, then translate - which is exactly
        // how AZ::Transform composes. A non-uniform scale cannot survive the trip; see EditorSpaceFromLocal.
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromEulerAnglesDegrees(layer.m_rotation), layer.m_position);
        transform.SetUniformScale(layer.m_scale.GetMaxElement());
        return transform;
    }

    void EditorWhiteBoxComponent::RefreshManipulatorSpaces()
    {
        const AZ::EntityComponentIdPair pair(GetEntityId(), GetId());
        EditorWhiteBoxDefaultModeRequestBus::Event(
            pair, &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonTranslationModifier);
        EditorWhiteBoxDefaultModeRequestBus::Event(
            pair, &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonScaleModifier);
        EditorWhiteBoxDefaultModeRequestBus::Event(
            pair, &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeTranslationModifier);
        EditorWhiteBoxDefaultModeRequestBus::Event(
            pair, &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeScaleModifier);
        EditorWhiteBoxDefaultModeRequestBus::Event(
            pair, &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshVertexSelectionModifier);
        EditorWhiteBoxTransformModeRequestBus::Event(
            pair, &EditorWhiteBoxTransformModeRequestBus::Events::RefreshManipulatorSpace);
    }

    void EditorWhiteBoxComponent::RefreshComponentMode()
    {
        // A full refresh, not just dirty intersection data: sub-modes hold handles into the old mesh
        // (Transform mode's selection crashes if it survives a layer swap). No-op outside component mode.
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    void EditorWhiteBoxComponent::ClearWorkingLayer()
    {
        // Reset the working members to empty geometry. Used when there are zero layers so the
        // white box shows nothing and the stamp occupancy is cleared.
        m_whiteBox = Api::CreateWhiteBoxMesh();
        m_gridMesh = Api::CreateWhiteBoxMesh();
        m_voxel.m_cells.clear();
        m_voxel.m_sizes.clear();
        m_voxel.m_legacyMerged.clear();
        Api::WriteMesh(*m_whiteBox, m_whiteBoxData);
        Api::WriteMesh(*m_gridMesh, m_voxel.m_legacyGridData);
        m_voxel.m_legacyGridMergedData.clear();
        m_layerRuntime.m_loadedIndex = -1;
        m_layerRuntime.m_loadedId = 0;
    }

    Api::WhiteBoxMeshPtr EditorWhiteBoxComponent::ApplyLayerModifiers(Api::WhiteBoxMeshPtr mesh, const WhiteBoxLayer& layer)
    {
        if (!mesh || !HasLayerModifiers(layer))
        {
            return mesh;
        }
        const bool mirror[3] = { layer.m_mirrorX, layer.m_mirrorY, layer.m_mirrorZ };
        for (int axis = 0; axis < 3; ++axis)
        {
            if (mirror[axis])
            {
                mesh = MirrorMesh(*mesh, axis);
            }
        }
        if (layer.m_arrayCount > 1)
        {
            mesh = ArrayMesh(*mesh, layer.m_arrayCount, layer.m_arrayOffset);
        }
        return mesh;
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnApplyLayerModifiers()
    {
        if (m_activeLayerIndex < 0 || m_activeLayerIndex >= static_cast<int>(m_layers.size()) ||
            !HasLayerModifiers(m_layers[m_activeLayerIndex]))
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }
        WhiteBoxLayer& layer = m_layers[m_activeLayerIndex];
        AzToolsFramework::ScopedUndoBatch undoBatch("Apply White Box Layer Modifiers");
        const bool isActive = m_activeLayerIndex == m_layerRuntime.m_loadedIndex;
        if (isActive)
        {
            SerializeWhiteBox(); // flush the working members into this layer's streams first
        }
        // As Apply Transform: fold the grids in (stamped cubes cannot stay voxels once copied), then bake.
        Api::WhiteBoxMeshPtr freeform = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*freeform, layer.m_freeformData);
        Api::WhiteBoxMeshPtr grid = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*grid, layer.m_gridData);
        if (!Api::MeshFaceHandles(*grid).empty())
        {
            AppendMesh(*freeform, *grid);
        }
        freeform = ApplyLayerModifiers(AZStd::move(freeform), layer);
        Api::WriteMesh(*freeform, layer.m_freeformData);
        layer.m_gridData.clear();
        layer.m_gridMergedData.clear();
        layer.m_voxelCells.clear();
        layer.m_voxelCellSizes.clear();
        layer.m_voxelMerged.clear();
        layer.m_mirrorX = layer.m_mirrorY = layer.m_mirrorZ = false;
        layer.m_arrayCount = 1;
        layer.m_parametric = false; // the copies are hand geometry now
        layer.m_bevelSource.clear(); // a live bevel would regenerate from the single, unmodified source
        layer.m_bevelEdges.clear();
        m_layerRuntime.m_meshCache.erase(layer.m_id);
        if (isActive)
        {
            m_layerRuntime.m_loadedIndex = -1; // reload the working members from the baked streams
            LoadActiveLayer();
        }
        OnLayersMetaChanged();
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnApplyLayerTransform()
    {
        if (m_layers.empty() || m_activeLayerIndex < 0 || m_activeLayerIndex >= static_cast<int>(m_layers.size()))
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }
        WhiteBoxLayer& layer = m_layers[m_activeLayerIndex];
        const bool identity = layer.m_position.IsZero() && layer.m_rotation.IsZero() &&
            layer.m_scale.IsClose(AZ::Vector3::CreateOne(), 1e-6f);
        if (identity)
        {
            return AZ::Edit::PropertyRefreshLevels::None; // nothing to bake
        }

        AzToolsFramework::ScopedUndoBatch undoBatch("Apply White Box Layer Transform");

        const bool isActive = m_activeLayerIndex == m_layerRuntime.m_loadedIndex;
        if (isActive)
        {
            SerializeWhiteBox(); // flush the working members into this layer's streams first
        }

        // Fold the grids in and bake the transform; rotated cubes cannot stay voxels, so stamps are frozen.
        Api::WhiteBoxMeshPtr freeform = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*freeform, layer.m_freeformData);
        Api::WhiteBoxMeshPtr grid = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*grid, layer.m_gridData);
        Api::WhiteBoxMeshPtr gridMerged = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*gridMerged, layer.m_gridMergedData);
        if (!Api::MeshFaceHandles(*grid).empty())
        {
            AppendMesh(*freeform, *grid);
        }
        if (!Api::MeshFaceHandles(*gridMerged).empty())
        {
            AppendMesh(*freeform, *gridMerged);
        }
        ApplyTransformToMesh(*freeform, layer.m_position, layer.m_rotation, layer.m_scale);

        Api::WriteMesh(*freeform, layer.m_freeformData);
        layer.m_gridData.clear();
        layer.m_gridMergedData.clear();
        layer.m_voxelCells.clear();
        layer.m_voxelCellSizes.clear();
        layer.m_voxelMerged.clear();
        layer.m_position = AZ::Vector3::CreateZero();
        layer.m_rotation = AZ::Vector3::CreateZero();
        layer.m_scale = AZ::Vector3::CreateOne();

        if (isActive)
        {
            m_layerRuntime.m_loadedIndex = -1; // force the working members to reload from the baked streams
            LoadActiveLayer();
        }
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree; // reset the Position/Rotation/Scale fields
    }

    AZ::u64 EditorWhiteBoxComponent::AllocLayerId()
    {
        if (m_nextLayerId == 0)
        {
            m_nextLayerId = 1;
        }
        return m_nextLayerId++;
    }

    int EditorWhiteBoxComponent::IndexOfLayerId(const AZ::u64 id) const
    {
        if (id == 0)
        {
            return -1;
        }
        for (int i = 0; i < static_cast<int>(m_layers.size()); ++i)
        {
            if (m_layers[i].m_id == id)
            {
                return i;
            }
        }
        return -1;
    }

    AZ::u64 EditorWhiteBoxComponent::LayerSignature() const
    {
        AZ::u64 h = 1469598103934665603ull ^ static_cast<AZ::u64>(m_layers.size());
        for (const WhiteBoxLayer& layer : m_layers)
        {
            h ^= layer.m_id;
            h *= 1099511628211ull;
        }
        return h;
    }

    void EditorWhiteBoxComponent::SyncLayerStructure()
    {
        // Give any natively-added ("+") layers a stable id first so ids are unique.
        for (WhiteBoxLayer& layer : m_layers)
        {
            if (layer.m_id == 0)
            {
                layer.m_id = AllocLayerId();
            }
        }

        const int count = static_cast<int>(m_layers.size());
        if (count == 0)
        {
            m_activeLayerIndex = 0;
            ClearWorkingLayer();
        }
        else
        {
            const int k = IndexOfLayerId(m_layerRuntime.m_loadedId);
            if (k >= 0)
            {
                // The loaded layer still exists (reordered or added to): reindex only, keeping in-progress edits.
                m_layerRuntime.m_loadedIndex = k;
                m_activeLayerIndex = k;
            }
            else
            {
                // The loaded layer was removed: load whatever the active index now points at.
                if (m_activeLayerIndex < 0)
                {
                    m_activeLayerIndex = 0;
                }
                if (m_activeLayerIndex >= count)
                {
                    m_activeLayerIndex = count - 1;
                }
                m_layerRuntime.m_loadedIndex = -1;
                LoadActiveLayer();
            }
        }

        m_layerRuntime.m_lastCount = count;
        m_layerRuntime.m_lastSignature = LayerSignature();

        // Prune cached display meshes for layers that no longer exist (ids are never reused).
        for (auto it = m_layerRuntime.m_meshCache.begin(); it != m_layerRuntime.m_meshCache.end();)
        {
            it = (IndexOfLayerId(it->first) < 0) ? m_layerRuntime.m_meshCache.erase(it) : ++it;
        }

        RebuildWhiteBox();
        RefreshComponentMode();
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
        // Every structural change ends here, including the first layer auto-created by drawing.
        EditorWhiteBoxComponentNotificationBus::Event(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()),
            &EditorWhiteBoxComponentNotificationBus::Events::OnLayerStructureChanged);
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::EvaluatedMesh()
    {
        // When a stamp/grid layer exists it is combined with the freeform mesh (see
        // RebuildCombinedMesh) so render / collision / bounds / selection all see both.
        if (m_combinedMesh)
        {
            return m_combinedMesh.get();
        }
        // Non-destructive: show the evaluated boolean result while the editable base stays untouched.
        if (BooleanDisplayActive() && m_displayMesh)
        {
            return m_displayMesh.get();
        }
        return GetWhiteBoxMesh();
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::GetEvaluatedWhiteBoxMesh()
    {
        return EvaluatedMesh();
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::GridMesh()
    {
        if (!m_gridMesh)
        {
            m_gridMesh = Api::CreateWhiteBoxMesh();
        }
        return m_gridMesh.get();
    }

    Api::WhiteBoxMeshPtr EditorWhiteBoxComponent::CombinedWithGrid(WhiteBoxMesh* freeform)
    {
        const bool hasSeparate = m_gridMesh && !Api::MeshFaceHandles(*m_gridMesh).empty();
        if (freeform == nullptr || !hasSeparate)
        {
            return nullptr; // no grid layer -> caller uses the freeform mesh as-is
        }
        Api::WhiteBoxMeshPtr combined = Api::CloneMesh(*freeform);
        if (!combined)
        {
            return nullptr;
        }

        // Stamped cubes are appended alongside the freeform (their own islands) - plain append,
        // no CSG; the occupancy mesher already produced a clean watertight cube surface.
        AppendMesh(*combined, *m_gridMesh);

        Api::CalculateNormals(*combined);
        Api::CalculatePlanarUVs(*combined);
        return combined;
    }

    Api::WhiteBoxMeshPtr EditorWhiteBoxComponent::BuildCombined(WhiteBoxMesh* activeFreeform, bool physicsPass)
    {
        const int count = static_cast<int>(m_layers.size());
        const int activeIdx = m_layerRuntime.m_loadedIndex;

        AZStd::vector<int> visible;
        visible.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            if (m_layers[i].m_visible)
            {
                // Skip this layer if we are building the physics mesh and collision is off
                if (physicsPass && !m_layers[i].m_collision)
                {
                    continue;
                }
                visible.push_back(i);
            }
        }
        if (visible.empty())
        {
            return Api::CreateWhiteBoxMesh(); // nothing visible -> empty mesh (renders nothing)
        }

        const bool activeIdentity = activeIdx >= 0 && activeIdx < count &&
            m_layers[activeIdx].m_position.IsZero() && m_layers[activeIdx].m_rotation.IsZero() &&
            m_layers[activeIdx].m_scale.IsClose(AZ::Vector3::CreateOne(), 1e-6f);
        const bool activeNoGrid = !(m_gridMesh && !Api::MeshFaceHandles(*m_gridMesh).empty());
        const bool activeInverted = activeIdx >= 0 && activeIdx < count && m_layers[activeIdx].m_invertNormals;
        const bool activeModified = activeIdx >= 0 && activeIdx < count && HasLayerModifiers(m_layers[activeIdx]);

        // Fast path: a single visible identity layer without grid, inversion or modifiers uses the raw
        // working mesh. Never on the physics pass, where filtered-out layers would sneak back in.
        if (!physicsPass && visible.size() == 1 && visible[0] == activeIdx && activeIdentity && activeNoGrid &&
            !activeInverted && !activeModified)
        {
            return nullptr;
        }

        // Accumulate visible layers bottom-up per combine mode. Inactive layers come from the mesh cache.
        Api::WhiteBoxMeshPtr acc;
        for (const int idx : visible)
        {
            Api::WhiteBoxMeshPtr ownedMesh; //!< Active layer only - always built fresh from the working mesh.
            WhiteBoxMesh* operand = nullptr; //!< The layer's display mesh (owned locally or by the cache).
            if (idx == activeIdx)
            {
                ownedMesh = CombinedWithGrid(activeFreeform);
                if (!ownedMesh && activeFreeform != nullptr)
                {
                    ownedMesh = Api::CloneMesh(*activeFreeform);
                }
                ownedMesh = ApplyLayerModifiers(AZStd::move(ownedMesh), m_layers[activeIdx]);
                if (ownedMesh && !activeIdentity)
                {
                    ApplyTransformToMesh(
                        *ownedMesh, m_layers[activeIdx].m_position, m_layers[activeIdx].m_rotation,
                        m_layers[activeIdx].m_scale);
                }
                if (ownedMesh && activeInverted)
                {
                    ownedMesh = FlippedMeshWinding(*ownedMesh); // mesh-level flip (see BuildLayerMesh)
                }
                operand = ownedMesh.get();
            }
            else
            {
                const AZ::u64 layerId = m_layers[idx].m_id;
                auto it = m_layerRuntime.m_meshCache.find(layerId);
                if (it == m_layerRuntime.m_meshCache.end() || !it->second)
                {
                    it = m_layerRuntime.m_meshCache.insert_or_assign(layerId, BuildLayerMesh(m_layers[idx])).first;
                }
                operand = it->second.get(); // already transformed; used read-only below
            }
            if (operand == nullptr)
            {
                continue;
            }
            if (!acc)
            {
                // First visible layer is the base (its own mode is ignored). The accumulator gets
                // mutated, so a cached operand must be cloned; the active layer's mesh is moved.
                acc = ownedMesh ? AZStd::move(ownedMesh) : Api::CloneMesh(*operand);
                continue;
            }
            const AZ::Transform identity = AZ::Transform::CreateIdentity();
            // Inter-layer booleans run in both tint modes; BuildColoredRenderData re-tints the merged faces.
            const LayerCombineMode mode = m_layers[idx].m_combineMode;
            switch (mode)
            {
            case LayerCombineMode::Union:
                if (Api::MeshFaceHandles(*acc).empty() ||
                    !Api::ApplyMeshBoolean(*acc, *operand, identity, Api::BooleanOperation::Union, m_csgSolver))
                {
                    AppendMesh(*acc, *operand);
                }
                break;
            case LayerCombineMode::Subtract:
                Api::ApplyMeshBoolean(*acc, *operand, identity, Api::BooleanOperation::Subtraction, m_csgSolver);
                break;
            case LayerCombineMode::Intersect:
                Api::ApplyMeshBoolean(*acc, *operand, identity, Api::BooleanOperation::Intersection, m_csgSolver);
                break;
            case LayerCombineMode::Separate:
            default:
                AppendMesh(*acc, *operand);
                break;
            }
        }
        if (!acc)
        {
            return Api::CreateWhiteBoxMesh();
        }
        Api::CalculateNormals(*acc);
        Api::CalculatePlanarUVs(*acc);
        return acc;
    }

    void EditorWhiteBoxComponent::RebuildCombinedMesh()
    {
        if (BooleanDisplayActive() && m_displayMesh && !m_boolean.m_affectActiveOnly)
        {
            m_combinedMesh = Api::CloneMesh(*m_displayMesh);
            // Physics uses the collision-filtered cut result; the visual one only if that was not evaluated.
            m_physicsCombinedMesh =
                Api::CloneMesh(m_physicsDisplayMesh ? *m_physicsDisplayMesh : *m_displayMesh);
            BakeEntityScaleIntoPhysicsMesh();
            return;
        }

        WhiteBoxMesh* freeform = (m_boolean.m_live && m_displayMesh) ? m_displayMesh.get() : GetWhiteBoxMesh();
        m_combinedMesh = BuildCombined(freeform, false);       // Visual Mesh (entity non-uniform scale via render transform)
        m_physicsCombinedMesh = BuildCombined(freeform, true); // Physics Mesh
        BakeEntityScaleIntoPhysicsMesh();
    }

    AZ::Vector3 EditorWhiteBoxComponent::EntityNonUniformScale() const
    {
        AZ::Vector3 scale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(scale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        return scale;
    }

    void EditorWhiteBoxComponent::BakeEntityScaleIntoPhysicsMesh()
    {
        // Bake non-uniform scale into the physics mesh (cooked colliders ignore it; render applies it
        // via the transform). The physics mesh is always a fresh copy, so this is safe.
        const AZ::Vector3 scale = EntityNonUniformScale();
        if (m_physicsCombinedMesh && !scale.IsClose(AZ::Vector3::CreateOne()))
        {
            ApplyTransformToMesh(*m_physicsCombinedMesh, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), scale);
            Api::CalculateNormals(*m_physicsCombinedMesh);
        }
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::GetPhysicsMesh()
    {
        if (m_physicsCombinedMesh)
        {
            return m_physicsCombinedMesh.get();
        }
        return EvaluatedMesh(); // Fallback safely
    }
} // namespace WhiteBox
