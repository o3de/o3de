/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Asset/EditorWhiteBoxMeshAsset.h"
#include "Asset/WhiteBoxMeshAssetHandler.h"
#include "Components/EditorWhiteBoxColliderComponent.h"
#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentMode.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "Rendering/WhiteBoxNullRenderMesh.h"
#include "Tools/WhiteBoxLayerUtil.h"

#include "Rendering/WhiteBoxRenderDataUtil.h"
#include "Rendering/WhiteBoxRenderMeshInterface.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "WhiteBoxComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/utils.h>
#include <cmath>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <AzCore/std/numeric.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <QMessageBox>
#include <WhiteBox/EditorWhiteBoxColliderBus.h>
#include <WhiteBox/WhiteBoxBus.h>

// developer debug properties for the White Box mesh to globally enable/disable
AZ_CVAR(bool, cl_whiteBoxDebugVertexHandles, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display vertex handles");
AZ_CVAR(bool, cl_whiteBoxDebugNormals, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display normals");
AZ_CVAR(
    bool, cl_whiteBoxDebugHalfedgeHandles, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display halfedge handles");
AZ_CVAR(bool, cl_whiteBoxDebugEdgeHandles, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display edge handles");
AZ_CVAR(bool, cl_whiteBoxDebugFaceHandles, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display face handles");
AZ_CVAR(bool, cl_whiteBoxDebugAabb, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display Aabb for the White Box");

namespace WhiteBox
{
    static const char* const AssetSavedUndoRedoDesc = "White Box Mesh asset saved";
    static const char* const ObjExtension = "obj";

    static void RefreshProperties()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    // Copy every polygon of @p src into @p dest (welding shared vertices within src by
    // vertex handle), leaving dest's existing geometry untouched. The two meshes stay
    // topologically separate islands - this is an APPEND, not a CSG merge - which is how the
    // stamp/grid layer is folded into the freeform mesh only for output.
    static void AppendMesh(WhiteBoxMesh& dest, const WhiteBoxMesh& src)
    {
        AZStd::unordered_map<int, Api::VertexHandle> vmap;
        const auto destVertex = [&](const Api::VertexHandle& srcVertex) -> Api::VertexHandle
        {
            const auto it = vmap.find(srcVertex.Index());
            if (it != vmap.end())
            {
                return it->second;
            }
            const Api::VertexHandle dh = Api::AddVertex(dest, Api::VertexPosition(src, srcVertex));
            vmap.emplace(srcVertex.Index(), dh);
            return dh;
        };

        for (const Api::PolygonHandle& polygon : Api::MeshPolygonHandles(src))
        {
            Api::FaceVertHandlesList faceVertHandles;
            faceVertHandles.reserve(polygon.m_faceHandles.size());
            for (const Api::FaceHandle& faceHandle : polygon.m_faceHandles)
            {
                const auto halfedges = Api::FaceHalfedgeHandles(src, faceHandle);
                if (halfedges.size() >= 3)
                {
                    faceVertHandles.push_back(Api::FaceVertHandles{
                        { destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[0])),
                          destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[1])),
                          destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[2])) }});
                }
            }
            if (!faceVertHandles.empty())
            {
                Api::AddPolygon(dest, faceVertHandles);
            }
        }
    }

    // Combine a freeform mesh with the two cube grids into one output mesh: merged cubes are
    // CSG-unioned with the freeform (falling back to append if the union is rejected), separate
    // cubes are appended. Any argument may be null/empty. Returns null only if there is nothing
    // to combine. Used for both the active layer and stored (inactive) layers.
    static Api::WhiteBoxMeshPtr CombineFreeformAndGrids(
        WhiteBoxMesh* freeform, WhiteBoxMesh* grid, WhiteBoxMesh* gridMerged)
    {
        const bool hasFreeform = freeform != nullptr && !Api::MeshFaceHandles(*freeform).empty();
        const bool hasMerged = gridMerged != nullptr && !Api::MeshFaceHandles(*gridMerged).empty();
        const bool hasSeparate = grid != nullptr && !Api::MeshFaceHandles(*grid).empty();
        if (!hasFreeform && !hasMerged && !hasSeparate)
        {
            return nullptr;
        }
        Api::WhiteBoxMeshPtr combined = hasFreeform ? Api::CloneMesh(*freeform) : Api::CreateWhiteBoxMesh();
        if (!combined)
        {
            return nullptr;
        }
        if (hasMerged)
        {
            if (Api::MeshFaceHandles(*combined).empty() ||
                !Api::ApplyMeshBoolean(
                    *combined, *gridMerged, AZ::Transform::CreateIdentity(), Api::BooleanOperation::Union))
            {
                AppendMesh(*combined, *gridMerged);
            }
        }
        if (hasSeparate)
        {
            AppendMesh(*combined, *grid);
        }
        Api::CalculateNormals(*combined);
        Api::CalculatePlanarUVs(*combined);
        return combined;
    }

    // build intermediate data to be passed to WhiteBoxRenderMeshInterface
    // to be used to generate concrete render mesh
    static WhiteBoxRenderData CreateWhiteBoxRenderData(
        const WhiteBoxMesh& whiteBox, const WhiteBoxMaterial& material, const bool flipWinding = false)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        WhiteBoxRenderData renderData;
        WhiteBoxFaces& faceData = renderData.m_faces;

        const auto faceCount = Api::MeshFaceCount(whiteBox);
        faceData.reserve(faceCount);

        const auto createWhiteBoxFaceFromHandle = [&whiteBox, flipWinding](const Api::FaceHandle& faceHandle) -> WhiteBoxFace
        {
            const auto copyVertex = [&whiteBox](const Api::HalfedgeHandle& in, WhiteBoxVertex& out)
            {
                const auto vh = Api::HalfedgeVertexHandleAtTip(whiteBox, in);
                out.m_position = Api::VertexPosition(whiteBox, vh);
                out.m_uv = Api::HalfedgeUV(whiteBox, in);
            };

            WhiteBoxFace face;
            face.m_normal = Api::FaceNormal(whiteBox, faceHandle);
            const auto faceHalfedgeHandles = Api::FaceHalfedgeHandles(whiteBox, faceHandle);

            if (flipWinding)
            {
                // Reverse winding (swap v1/v3) and flip the face normal so the layer renders inside-out.
                copyVertex(faceHalfedgeHandles[0], face.m_v3);
                copyVertex(faceHalfedgeHandles[1], face.m_v2);
                copyVertex(faceHalfedgeHandles[2], face.m_v1);
                face.m_normal = -face.m_normal;
            }
            else
            {
                copyVertex(faceHalfedgeHandles[0], face.m_v1);
                copyVertex(faceHalfedgeHandles[1], face.m_v2);
                copyVertex(faceHalfedgeHandles[2], face.m_v3);
            }

            return face;
        };

        const auto faceHandles = Api::MeshFaceHandles(whiteBox);
        for (const auto& faceHandle : faceHandles)
        {
            faceData.push_back(createWhiteBoxFaceFromHandle(faceHandle));
        }

        renderData.m_material = material;

        return renderData;
    }

    static bool IsWhiteBoxNullRenderMesh(const AZStd::optional<AZStd::unique_ptr<RenderMeshInterface>>& m_renderMesh)
    {
        return azrtti_cast<WhiteBoxNullRenderMesh*>((*m_renderMesh).get()) != nullptr;
    }

    static bool DisplayingAsset(const DefaultShapeType defaultShapeType)
    {
        // checks if the default shape is set to a custom asset
        return defaultShapeType == DefaultShapeType::Asset;
    }

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

    void EditorWhiteBoxComponent::ApplyBoolean()
    {
        if (!m_booleanSourceEntity.IsValid() || m_booleanSourceEntity == GetEntityId())
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Boolean Source is not set (or is this same entity).");
            return;
        }

        WhiteBoxMesh* targetMesh = GetWhiteBoxMesh();
        if (targetMesh == nullptr)
        {
            return;
        }

        AZ::Entity* sourceEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, m_booleanSourceEntity);
        if (sourceEntity == nullptr)
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Boolean Source entity could not be found.");
            return;
        }

        const auto sourceComponents = sourceEntity->FindComponents<EditorWhiteBoxComponent>();
        if (sourceComponents.empty())
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Boolean Source entity has no White Box component.");
            return;
        }
        WhiteBoxMesh* sourceMesh = sourceComponents[0]->GetWhiteBoxMesh();
        if (sourceMesh == nullptr)
        {
            return;
        }

        // Bring the source mesh from its local space into this entity's local space:
        // operandTransform = thisWorldFromLocal^-1 * sourceWorldFromLocal.
        AZ::Transform thisWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(thisWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Transform sourceWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(sourceWorldTM, m_booleanSourceEntity, &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Transform operandTransform = thisWorldTM.GetInverse() * sourceWorldTM;

        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Boolean");

        if (!Api::ApplyMeshBoolean(*targetMesh, *sourceMesh, operandTransform, m_booleanOperation))
        {
            AZ_Warning(
                "EditorWhiteBoxComponent", false,
                "White Box boolean produced no result (the meshes may not overlap, or are not closed).");
            return;
        }

        Api::CalculateNormals(*targetMesh);
        Api::CalculatePlanarUVs(*targetMesh);

        SerializeWhiteBox();
        RebuildWhiteBox();
        undoBatch.MarkEntityDirty(GetEntityId());

        // Optionally tidy up the source entity once it has been consumed.
        if (m_deleteSourceAfterApply)
        {
            const AZ::EntityId sourceId = m_booleanSourceEntity;
            m_booleanSourceEntity = AZ::EntityId{}; // clear the now-dangling reference
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::DeleteEntityById, sourceId);
        }
        else if (m_hideSourceAfterApply)
        {
            AzToolsFramework::SetEntityVisibility(m_booleanSourceEntity, false);
        }
    }

    namespace VoxelDetail
    {
        // Pack/unpack integer cell coordinates into a single key (21 bits per axis,
        // biased so negatives work; range ~ +/-1,000,000 cells per axis).
        constexpr AZ::s64 CellBias = 1 << 20;
        constexpr AZ::u64 CellMask = (AZ::u64(1) << 21) - 1;

        AZ::u64 PackCell(int x, int y, int z)
        {
            return ((AZ::u64(x + CellBias) & CellMask) << 42) | ((AZ::u64(y + CellBias) & CellMask) << 21) |
                (AZ::u64(z + CellBias) & CellMask);
        }
        void UnpackCell(AZ::u64 key, int& x, int& y, int& z)
        {
            x = static_cast<int>((key >> 42) & CellMask) - CellBias;
            y = static_cast<int>((key >> 21) & CellMask) - CellBias;
            z = static_cast<int>(key & CellMask) - CellBias;
        }

        // Voxel sizes are stored per cube (so cubes of different sizes can coexist). To
        // keep the geometry keyed on a single integer lattice, cells are grouped by size
        // and each group is meshed on its own grid. Sizes are quantised to this many world
        // units so tiny float differences map to the same group.
        constexpr float VoxelSizeQuantum = 1.0e-4f;
        AZ::s32 QuantizeSize(float size)
        {
            const float s = size < 0.05f ? 0.05f : size;
            return static_cast<AZ::s32>(std::lround(s / VoxelSizeQuantum));
        }
        float SizeFromQuantized(AZ::s32 q)
        {
            const float s = static_cast<float>(q) * VoxelSizeQuantum;
            return s < 0.05f ? 0.05f : s;
        }

        // Voxel cells are stored as two parallel arrays (a packed integer coordinate and a
        // world-space cube size per cell). Grouping them by quantised size gives one
        // single-grid cell set per distinct cube size, which the surface/collider builders
        // consume one size at a time - this is what lets differently sized cubes coexist in
        // the same mesh without a shared baked grid.
        using CellSet = AZStd::unordered_set<AZ::u64>;
        using SizeGroups = AZStd::unordered_map<AZ::s32, CellSet>;

        SizeGroups GroupBySize(const AZStd::vector<AZ::u64>& cells, const AZStd::vector<float>& sizes)
        {
            SizeGroups groups;
            const size_t count = cells.size() < sizes.size() ? cells.size() : sizes.size();
            for (size_t i = 0; i < count; ++i)
            {
                groups[QuantizeSize(sizes[i])].insert(cells[i]);
            }
            return groups;
        }

        void FlattenGroups(
            const SizeGroups& groups, AZStd::vector<AZ::u64>& outCells, AZStd::vector<float>& outSizes)
        {
            outCells.clear();
            outSizes.clear();
            for (const auto& group : groups)
            {
                if (group.second.empty())
                {
                    continue; // drop emptied size groups so they don't linger
                }
                const float size = SizeFromQuantized(group.first);
                for (const AZ::u64 cell : group.second)
                {
                    outCells.push_back(cell);
                    outSizes.push_back(size);
                }
            }
        }

        // Per-cell "merged with the freeform mesh" flags travel alongside the cell coords and
        // sizes (three parallel arrays). These group / flatten them together as
        // sizeKey -> (coord -> merged).
        using CellMerge = AZStd::unordered_map<AZ::u64, AZ::u8>;
        using SizeMergeGroups = AZStd::unordered_map<AZ::s32, CellMerge>;

        SizeMergeGroups GroupBySizeMerge(
            const AZStd::vector<AZ::u64>& cells, const AZStd::vector<float>& sizes,
            const AZStd::vector<AZ::u8>& merged)
        {
            SizeMergeGroups groups;
            for (size_t i = 0; i < cells.size(); ++i)
            {
                const float size = i < sizes.size() ? sizes[i] : 1.0f;
                const AZ::u8 m = i < merged.size() ? merged[i] : AZ::u8{0};
                groups[QuantizeSize(size)][cells[i]] = m;
            }
            return groups;
        }

        void FlattenMergeGroups(
            const SizeMergeGroups& groups, AZStd::vector<AZ::u64>& outCells, AZStd::vector<float>& outSizes,
            AZStd::vector<AZ::u8>& outMerged)
        {
            outCells.clear();
            outSizes.clear();
            outMerged.clear();
            for (const auto& group : groups)
            {
                if (group.second.empty())
                {
                    continue;
                }
                const float size = SizeFromQuantized(group.first);
                for (const auto& cell : group.second)
                {
                    outCells.push_back(cell.first);
                    outSizes.push_back(size);
                    outMerged.push_back(cell.second);
                }
            }
        }

        // Build a light, vertex-shared surface for a set of filled voxel cells (each cell
        // is a cellSize cube). Exposed faces are greedy-merged into maximal rectangles, so
        // a single cube stays 8 verts / 6 faces and a wall or block becomes a handful of
        // quads with only corner vertices - no interior/per-cell vertices exist to weigh
        // the mesh down or clutter vertex/edge editing. Each merged rectangle is committed
        // as one quad polygon. (Deliberately written with plain loops - no nested generic
        // lambdas - so it behaves identically across compilers.)
        void GenerateSurface(WhiteBoxMesh& mesh, const AZStd::unordered_set<AZ::u64>& cells, const float cellSize)
        {
            AZStd::unordered_map<AZ::u64, Api::VertexHandle> verts;
            const auto vert = [&](int x, int y, int z) -> Api::VertexHandle
            {
                const AZ::u64 key = PackCell(x, y, z);
                const auto it = verts.find(key);
                if (it != verts.end())
                {
                    return it->second;
                }
                const Api::VertexHandle h = Api::AddVertex(
                    mesh, AZ::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)) * cellSize);
                verts.emplace(key, h);
                return h;
            };
            const auto filled = [&](int x, int y, int z) { return cells.count(PackCell(x, y, z)) != 0; };

            int polygonCount = 0;
            const auto addQuad =
                [&](Api::VertexHandle a, Api::VertexHandle b, Api::VertexHandle c, Api::VertexHandle d)
            {
                // One merged rectangle -> one quad polygon (two triangles sharing the four
                // corner vertices), so its interior reads as a single face.
                Api::FaceVertHandlesList quadFaces;
                quadFaces.push_back(Api::FaceVertHandles{{a, b, c}});
                quadFaces.push_back(Api::FaceVertHandles{{a, c, d}});
                Api::AddPolygon(mesh, quadFaces);
                ++polygonCount;
            };

            // Greedy-merge a plane's exposed footprint cells (packed as PackCell(a, b, 0))
            // into maximal (a0..a1, b0..b1) rectangles. Returns them as flat quads of four
            // ints {a0, a1, b0, b1}.
            struct Rect { int a0, a1, b0, b1; };
            const auto greedyRects = [](AZStd::unordered_set<AZ::u64> mask) -> AZStd::vector<Rect>
            {
                AZStd::vector<Rect> rects;
                AZStd::vector<AZStd::pair<int, int>> list;
                list.reserve(mask.size());
                for (const AZ::u64 k : mask)
                {
                    int a, b, unused;
                    UnpackCell(k, a, b, unused);
                    list.push_back({ a, b });
                }
                AZStd::sort(list.begin(), list.end());
                const auto has = [&mask](int a, int b) { return mask.count(PackCell(a, b, 0)) != 0; };
                for (const auto& ab : list)
                {
                    const int a = ab.first;
                    const int b = ab.second;
                    if (!has(a, b))
                    {
                        continue;
                    }
                    int b1 = b;
                    while (has(a, b1 + 1))
                    {
                        ++b1;
                    }
                    int a1 = a;
                    bool grow = true;
                    while (grow)
                    {
                        const int na = a1 + 1;
                        for (int bb = b; bb <= b1; ++bb)
                        {
                            if (!has(na, bb))
                            {
                                grow = false;
                                break;
                            }
                        }
                        if (grow)
                        {
                            a1 = na;
                        }
                    }
                    for (int aa = a; aa <= a1; ++aa)
                    {
                        for (int bb = b; bb <= b1; ++bb)
                        {
                            mask.erase(PackCell(aa, bb, 0));
                        }
                    }
                    rects.push_back({ a, a1, b, b1 });
                }
                return rects;
            };

            // Collect exposed footprint cells per plane for one face direction. The plane
            // key is the coordinate along the face normal; (a, b) are the two in-plane axes.
            AZStd::unordered_map<int, AZStd::unordered_set<AZ::u64>> planes;
            const auto clearPlanes = [&]() { planes.clear(); };

            // +X : plane = x, footprint (a = y, b = z), face at px = x + 1.
            for (const AZ::u64 cellKey : cells)
            {
                int x, y, z;
                UnpackCell(cellKey, x, y, z);
                if (!filled(x + 1, y, z))
                {
                    planes[x].insert(PackCell(y, z, 0));
                }
            }
            for (auto& plane : planes)
            {
                const int px = plane.first + 1;
                for (const Rect& r : greedyRects(plane.second))
                {
                    addQuad(vert(px, r.a0, r.b0), vert(px, r.a1 + 1, r.b0), vert(px, r.a1 + 1, r.b1 + 1),
                        vert(px, r.a0, r.b1 + 1));
                }
            }
            clearPlanes();

            // -X : plane = x, footprint (a = y, b = z), face at px = x.
            for (const AZ::u64 cellKey : cells)
            {
                int x, y, z;
                UnpackCell(cellKey, x, y, z);
                if (!filled(x - 1, y, z))
                {
                    planes[x].insert(PackCell(y, z, 0));
                }
            }
            for (auto& plane : planes)
            {
                const int px = plane.first;
                for (const Rect& r : greedyRects(plane.second))
                {
                    addQuad(vert(px, r.a1 + 1, r.b0), vert(px, r.a0, r.b0), vert(px, r.a0, r.b1 + 1),
                        vert(px, r.a1 + 1, r.b1 + 1));
                }
            }
            clearPlanes();

            // +Y : plane = y, footprint (a = x, b = z), face at py = y + 1.
            for (const AZ::u64 cellKey : cells)
            {
                int x, y, z;
                UnpackCell(cellKey, x, y, z);
                if (!filled(x, y + 1, z))
                {
                    planes[y].insert(PackCell(x, z, 0));
                }
            }
            for (auto& plane : planes)
            {
                const int py = plane.first + 1;
                for (const Rect& r : greedyRects(plane.second))
                {
                    addQuad(vert(r.a1 + 1, py, r.b0), vert(r.a0, py, r.b0), vert(r.a0, py, r.b1 + 1),
                        vert(r.a1 + 1, py, r.b1 + 1));
                }
            }
            clearPlanes();

            // -Y : plane = y, footprint (a = x, b = z), face at py = y.
            for (const AZ::u64 cellKey : cells)
            {
                int x, y, z;
                UnpackCell(cellKey, x, y, z);
                if (!filled(x, y - 1, z))
                {
                    planes[y].insert(PackCell(x, z, 0));
                }
            }
            for (auto& plane : planes)
            {
                const int py = plane.first;
                for (const Rect& r : greedyRects(plane.second))
                {
                    addQuad(vert(r.a0, py, r.b0), vert(r.a1 + 1, py, r.b0), vert(r.a1 + 1, py, r.b1 + 1),
                        vert(r.a0, py, r.b1 + 1));
                }
            }
            clearPlanes();

            // +Z : plane = z, footprint (a = x, b = y), face at pz = z + 1.
            for (const AZ::u64 cellKey : cells)
            {
                int x, y, z;
                UnpackCell(cellKey, x, y, z);
                if (!filled(x, y, z + 1))
                {
                    planes[z].insert(PackCell(x, y, 0));
                }
            }
            for (auto& plane : planes)
            {
                const int pz = plane.first + 1;
                for (const Rect& r : greedyRects(plane.second))
                {
                    addQuad(vert(r.a0, r.b0, pz), vert(r.a1 + 1, r.b0, pz), vert(r.a1 + 1, r.b1 + 1, pz),
                        vert(r.a0, r.b1 + 1, pz));
                }
            }
            clearPlanes();

            // -Z : plane = z, footprint (a = x, b = y), face at pz = z.
            for (const AZ::u64 cellKey : cells)
            {
                int x, y, z;
                UnpackCell(cellKey, x, y, z);
                if (!filled(x, y, z - 1))
                {
                    planes[z].insert(PackCell(x, y, 0));
                }
            }
            for (auto& plane : planes)
            {
                const int pz = plane.first;
                for (const Rect& r : greedyRects(plane.second))
                {
                    addQuad(vert(r.a0, r.b1 + 1, pz), vert(r.a1 + 1, r.b1 + 1, pz), vert(r.a1 + 1, r.b0, pz),
                        vert(r.a0, r.b0, pz));
                }
            }
            clearPlanes();

            
        }

        // Build the surface WITHOUT greedy merging: one quad per exposed cell face. Unlike
        // the greedy version this is guaranteed watertight and 2-manifold for ANY cell set
        // (including non-convex clusters), because every face is a full unit-cell face with
        // no T-junctions. That makes it a valid CSG boolean operand - the greedy surface can
        // contain T-junctions on concave clusters, which the boolean would reject.
        void GenerateSurfacePerCell(WhiteBoxMesh& mesh, const AZStd::unordered_set<AZ::u64>& cells, const float cellSize)
        {
            AZStd::unordered_map<AZ::u64, Api::VertexHandle> verts;
            const auto vert = [&](int x, int y, int z) -> Api::VertexHandle
            {
                const AZ::u64 key = PackCell(x, y, z);
                const auto it = verts.find(key);
                if (it != verts.end())
                {
                    return it->second;
                }
                const Api::VertexHandle h = Api::AddVertex(
                    mesh, AZ::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)) * cellSize);
                verts.emplace(key, h);
                return h;
            };
            const auto filled = [&](int x, int y, int z) { return cells.count(PackCell(x, y, z)) != 0; };
            const auto quad =
                [&](Api::VertexHandle a, Api::VertexHandle b, Api::VertexHandle c, Api::VertexHandle d)
            {
                Api::FaceVertHandlesList f;
                f.push_back(Api::FaceVertHandles{{a, b, c}});
                f.push_back(Api::FaceVertHandles{{a, c, d}});
                Api::AddPolygon(mesh, f);
            };

            for (const AZ::u64 cellKey : cells)
            {
                int x, y, z;
                UnpackCell(cellKey, x, y, z);
                if (!filled(x + 1, y, z))
                    quad(vert(x + 1, y, z), vert(x + 1, y + 1, z), vert(x + 1, y + 1, z + 1), vert(x + 1, y, z + 1));
                if (!filled(x - 1, y, z))
                    quad(vert(x, y + 1, z), vert(x, y, z), vert(x, y, z + 1), vert(x, y + 1, z + 1));
                if (!filled(x, y + 1, z))
                    quad(vert(x + 1, y + 1, z), vert(x, y + 1, z), vert(x, y + 1, z + 1), vert(x + 1, y + 1, z + 1));
                if (!filled(x, y - 1, z))
                    quad(vert(x, y, z), vert(x + 1, y, z), vert(x + 1, y, z + 1), vert(x, y, z + 1));
                if (!filled(x, y, z + 1))
                    quad(vert(x, y, z + 1), vert(x + 1, y, z + 1), vert(x + 1, y + 1, z + 1), vert(x, y + 1, z + 1));
                if (!filled(x, y, z - 1))
                    quad(vert(x, y + 1, z), vert(x + 1, y + 1, z), vert(x + 1, y, z), vert(x, y, z));
            }
        }

        // Quantize a vertex position to its voxel-grid cell index (positions are cell
        // indices scaled by VoxelCellSize). Returns false if the position is not on the
        // grid - such a vertex belongs to freeform geometry, not the voxel surface.
        bool QuantizeCorner(const AZ::Vector3& p, const float cellSize, int& x, int& y, int& z)
        {
            constexpr float eps = 1e-3f;
            const float sx = p.GetX() / cellSize;
            const float sy = p.GetY() / cellSize;
            const float sz = p.GetZ() / cellSize;
            const float rx = std::round(sx);
            const float ry = std::round(sy);
            const float rz = std::round(sz);
            if (std::abs(sx - rx) > eps || std::abs(sy - ry) > eps || std::abs(sz - rz) > eps)
            {
                return false;
            }
            x = static_cast<int>(rx);
            y = static_cast<int>(ry);
            z = static_cast<int>(rz);
            return true;
        }

        // Order-independent identity of a voxel-surface triangle: the sorted packed
        // keys of its three integer corners. Two triangles with the same three
        // lattice corners (regardless of winding) compare equal.
        using FaceSignature = AZStd::array<AZ::u64, 3>;

        // AZStd::array provides no operator< in this engine version, so order the
        // set explicitly (lexicographically over the three packed corner keys).
        struct FaceSignatureLess
        {
            bool operator()(const FaceSignature& a, const FaceSignature& b) const
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    if (a[i] != b[i])
                    {
                        return a[i] < b[i];
                    }
                }
                return false;
            }
        };
        using FaceSignatureSet = AZStd::set<FaceSignature, FaceSignatureLess>;

        bool FaceSignatureFromPositions(
            const AZStd::vector<AZ::Vector3>& positions, const float cellSize, FaceSignature& outSig)
        {
            if (positions.size() != 3)
            {
                return false;
            }
            for (size_t i = 0; i < 3; ++i)
            {
                int x, y, z;
                if (!QuantizeCorner(positions[i], cellSize, x, y, z))
                {
                    return false;
                }
                outSig[i] = PackCell(x, y, z);
            }
            AZStd::sort(outSig.begin(), outSig.end());
            return true;
        }

        // The set of triangle signatures that make up the voxel surface for `cells`.
        // Generated through the exact same path as the live mesh, so the signatures
        // match the faces actually present after a stamp/load.
        FaceSignatureSet SurfaceFaceSignatures(const AZStd::unordered_set<AZ::u64>& cells, const float cellSize)
        {
            FaceSignatureSet sigs;
            if (cells.empty())
            {
                return sigs;
            }
            Api::WhiteBoxMeshPtr temp = Api::CreateWhiteBoxMesh();
            GenerateSurface(*temp, cells, cellSize);
            for (const Api::FaceHandle& fh : Api::MeshFaceHandles(*temp))
            {
                FaceSignature sig;
                if (FaceSignatureFromPositions(Api::FaceVertexPositions(*temp, fh), cellSize, sig))
                {
                    sigs.insert(sig);
                }
            }
            return sigs;
        }

        // Greedy-mesh the voxel cells into a minimal indexed triangle set for a physics
        // collider. A PhysX triangle mesh is a plain soup, so coplanar exposed faces can be
        // merged into big rectangles (2 triangles each) - hugely fewer triangles than one
        // quad per cell. Triangles are APPENDED to verts/indices (absolute indices).
        void GreedyColliderTriangles(
            const AZStd::unordered_set<AZ::u64>& cells, const float cellSize, AZStd::vector<AZ::Vector3>& verts,
            AZStd::vector<AZ::u32>& indices)
        {
            AZStd::unordered_map<AZ::u64, AZ::u32> vmap;
            const auto vert = [&](int x, int y, int z) -> AZ::u32
            {
                const AZ::u64 key = PackCell(x, y, z);
                const auto it = vmap.find(key);
                if (it != vmap.end())
                {
                    return it->second;
                }
                const AZ::u32 idx = static_cast<AZ::u32>(verts.size());
                verts.push_back(
                    AZ::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)) * cellSize);
                vmap.emplace(key, idx);
                return idx;
            };
            const auto filled = [&](int x, int y, int z) { return cells.count(PackCell(x, y, z)) != 0; };
            const auto packAB = [](int a, int b) -> AZ::u64 { return PackCell(a, b, 0); };

            // A quad (a,b,c,d wound CCW) -> two triangles.
            const auto quad = [&](AZ::u32 a, AZ::u32 b, AZ::u32 c, AZ::u32 d)
            {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(d);
            };

            // Greedy-merge a plane's exposed (a,b) cells into maximal rectangles.
            const auto greedy = [&](AZStd::unordered_set<AZ::u64>& mask, auto&& emit)
            {
                AZStd::vector<AZStd::pair<int, int>> list;
                list.reserve(mask.size());
                for (const AZ::u64 k : mask)
                {
                    int a, b, unused;
                    UnpackCell(k, a, b, unused);
                    list.push_back({a, b});
                }
                AZStd::sort(list.begin(), list.end());
                for (const auto& ab : list)
                {
                    const int a = ab.first;
                    const int b = ab.second;
                    if (mask.count(packAB(a, b)) == 0)
                    {
                        continue;
                    }
                    int b1 = b;
                    while (mask.count(packAB(a, b1 + 1)) != 0)
                    {
                        ++b1;
                    }
                    int a1 = a;
                    bool grow = true;
                    while (grow)
                    {
                        const int na = a1 + 1;
                        for (int bb = b; bb <= b1; ++bb)
                        {
                            if (mask.count(packAB(na, bb)) == 0)
                            {
                                grow = false;
                                break;
                            }
                        }
                        if (grow)
                        {
                            a1 = na;
                        }
                    }
                    for (int aa = a; aa <= a1; ++aa)
                    {
                        for (int bb = b; bb <= b1; ++bb)
                        {
                            mask.erase(packAB(aa, bb));
                        }
                    }
                    emit(a, a1, b, b1);
                }
            };

            const auto buildDir = [&](auto neighbourEmpty, auto planeIndex, auto inA, auto inB, auto makeQuad)
            {
                AZStd::unordered_map<int, AZStd::unordered_set<AZ::u64>> planes;
                for (const AZ::u64 cellKey : cells)
                {
                    int x, y, z;
                    UnpackCell(cellKey, x, y, z);
                    if (neighbourEmpty(x, y, z))
                    {
                        planes[planeIndex(x, y, z)].insert(packAB(inA(x, y, z), inB(x, y, z)));
                    }
                }
                for (auto& plane : planes)
                {
                    const int p = plane.first;
                    greedy(plane.second, [&](int a0, int a1, int b0, int b1) { makeQuad(p, a0, a1, b0, b1); });
                }
            };

            buildDir(
                [&](int x, int y, int z) { return !filled(x + 1, y, z); }, [](int x, int, int) { return x; },
                [](int, int y, int) { return y; }, [](int, int, int z) { return z; },
                [&](int px1, int y0, int y1, int z0, int z1)
                {
                    const int px = px1 + 1;
                    quad(vert(px, y0, z0), vert(px, y1 + 1, z0), vert(px, y1 + 1, z1 + 1), vert(px, y0, z1 + 1));
                });
            buildDir(
                [&](int x, int y, int z) { return !filled(x - 1, y, z); }, [](int x, int, int) { return x; },
                [](int, int y, int) { return y; }, [](int, int, int z) { return z; },
                [&](int px, int y0, int y1, int z0, int z1)
                { quad(vert(px, y1 + 1, z0), vert(px, y0, z0), vert(px, y0, z1 + 1), vert(px, y1 + 1, z1 + 1)); });
            buildDir(
                [&](int x, int y, int z) { return !filled(x, y + 1, z); }, [](int, int y, int) { return y; },
                [](int x, int, int) { return x; }, [](int, int, int z) { return z; },
                [&](int py1, int x0, int x1, int z0, int z1)
                {
                    const int py = py1 + 1;
                    quad(vert(x1 + 1, py, z0), vert(x0, py, z0), vert(x0, py, z1 + 1), vert(x1 + 1, py, z1 + 1));
                });
            buildDir(
                [&](int x, int y, int z) { return !filled(x, y - 1, z); }, [](int, int y, int) { return y; },
                [](int x, int, int) { return x; }, [](int, int, int z) { return z; },
                [&](int py, int x0, int x1, int z0, int z1)
                { quad(vert(x0, py, z0), vert(x1 + 1, py, z0), vert(x1 + 1, py, z1 + 1), vert(x0, py, z1 + 1)); });
            buildDir(
                [&](int x, int y, int z) { return !filled(x, y, z + 1); }, [](int, int, int z) { return z; },
                [](int x, int, int) { return x; }, [](int, int y, int) { return y; },
                [&](int pz1, int x0, int x1, int y0, int y1)
                {
                    const int pz = pz1 + 1;
                    quad(vert(x0, y0, pz), vert(x1 + 1, y0, pz), vert(x1 + 1, y1 + 1, pz), vert(x0, y1 + 1, pz));
                });
            buildDir(
                [&](int x, int y, int z) { return !filled(x, y, z - 1); }, [](int, int, int z) { return z; },
                [](int x, int, int) { return x; }, [](int, int y, int) { return y; },
                [&](int pz, int x0, int x1, int y0, int y1)
                { quad(vert(x0, y1 + 1, pz), vert(x1 + 1, y1 + 1, pz), vert(x1 + 1, y0, pz), vert(x0, y0, pz)); });
        }
    } // namespace VoxelDetail

    void EditorWhiteBoxComponent::NormalizeVoxelData()
    {
        // Older scenes stored just the cell coordinates plus a single baked cell size
        // (m_voxelCellSize). Give every such cell its own size entry so the rest of the
        // pipeline can treat all data as per-cell sized. Also guards against any stale
        // mismatch between the two parallel arrays.
        if (m_voxelCellSizes.size() != m_voxelCells.size())
        {
            const float legacy = m_voxelCellSize < 0.05f ? 0.05f : m_voxelCellSize;
            m_voxelCellSizes.assign(m_voxelCells.size(), legacy);
        }
        if (m_voxelMerged.size() != m_voxelCells.size())
        {
            // Legacy data (before per-cube merge flags) is treated as separate cubes.
            m_voxelMerged.assign(m_voxelCells.size(), AZ::u8{0});
        }
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::GridMergedMesh()
    {
        if (!m_gridMergedMesh)
        {
            m_gridMergedMesh = Api::CreateWhiteBoxMesh();
        }
        return m_gridMergedMesh.get();
    }

    bool EditorWhiteBoxComponent::CarveCubeGrids(const WhiteBoxMesh& cutter, const AZ::Transform& cutterTransform)
    {
        // Subtract the draw-shape cutter from both cube grids so a carve cuts through stamped
        // cubes too. The freeform mesh is handled separately by the draw mode. The caller
        // serializes and rebuilds afterwards.
        bool changed = false;
        for (WhiteBoxMesh* grid : { m_gridMesh.get(), m_gridMergedMesh.get() })
        {
            if (grid != nullptr && !Api::MeshFaceHandles(*grid).empty() &&
                Api::ApplyMeshBoolean(*grid, cutter, cutterTransform, Api::BooleanOperation::Subtraction))
            {
                changed = true;
            }
        }
        return changed;
    }

    void EditorWhiteBoxComponent::RegenerateVoxelMesh(
        const AZStd::vector<AZ::u64>& oldCells, const AZStd::vector<float>& oldSizes,
        const AZStd::vector<AZ::u64>& newCells, const AZStd::vector<float>& newSizes)
    {
        WhiteBoxMesh* mesh = GetWhiteBoxMesh();
        if (mesh == nullptr)
        {
            return;
        }

        const VoxelDetail::SizeGroups oldGroups = VoxelDetail::GroupBySize(oldCells, oldSizes);
        const VoxelDetail::SizeGroups newGroups = VoxelDetail::GroupBySize(newCells, newSizes);

        // Only touch the size-groups that actually CHANGED. A group whose cell set is
        // identical in old and new is left completely alone - its faces are neither removed
        // nor regenerated - so stamping never re-runs face creation over cubes of other
        // sizes (which is what previously duplicated geometry into a non-manifold mesh) and
        // never disturbs cubes the user has hand-edited.
        AZStd::unordered_set<AZ::s32> changedSizes;
        for (const auto& group : oldGroups)
        {
            const auto it = newGroups.find(group.first);
            if (it == newGroups.end() || it->second != group.second)
            {
                changedSizes.insert(group.first);
            }
        }
        for (const auto& group : newGroups)
        {
            const auto it = oldGroups.find(group.first);
            if (it == oldGroups.end() || it->second != group.second)
            {
                changedSizes.insert(group.first);
            }
        }

        // Remove the previous voxel surface for the CHANGED sizes only, matching mesh faces
        // by signature (freeform / hand-edited faces never match, so they are preserved).
        AZStd::unordered_map<AZ::s32, VoxelDetail::FaceSignatureSet> oldSigsBySize;
        for (const AZ::s32 sizeKey : changedSizes)
        {
            const auto it = oldGroups.find(sizeKey);
            if (it != oldGroups.end())
            {
                oldSigsBySize.emplace(
                    sizeKey, VoxelDetail::SurfaceFaceSignatures(it->second, VoxelDetail::SizeFromQuantized(sizeKey)));
            }
        }

        int removedFaces = 0;
        if (!oldSigsBySize.empty())
        {
            Api::FaceHandles toRemove;
            for (const Api::FaceHandle& fh : Api::MeshFaceHandles(*mesh))
            {
                const AZStd::vector<AZ::Vector3> positions = Api::FaceVertexPositions(*mesh, fh);
                for (const auto& sizeSigs : oldSigsBySize)
                {
                    VoxelDetail::FaceSignature sig;
                    if (VoxelDetail::FaceSignatureFromPositions(
                            positions, VoxelDetail::SizeFromQuantized(sizeSigs.first), sig) &&
                        sizeSigs.second.find(sig) != sizeSigs.second.end())
                    {
                        toRemove.push_back(fh);
                        break;
                    }
                }
            }
            if (!toRemove.empty())
            {
                // Single batched removal - handles are invalidated by garbage_collect.
                removedFaces = static_cast<int>(toRemove.size());
                Api::RemoveFaces(*mesh, toRemove);
            }
        }

        // Regenerate the surface for the CHANGED sizes only, one grid per cube size.
        for (const AZ::s32 sizeKey : changedSizes)
        {
            const auto it = newGroups.find(sizeKey);
            if (it != newGroups.end())
            {
                VoxelDetail::GenerateSurface(*mesh, it->second, VoxelDetail::SizeFromQuantized(sizeKey));
            }
        }

        // Purge vertices orphaned by the face removal above. RemoveFaces keeps isolated
        // vertices, which is why clearing / re-stamping used to leave stray points visible
        // in vertex and edge editing modes; deleting them here keeps the mesh clean.
        Api::RemoveIsolatedVertices(*mesh);

        

        Api::CalculateNormals(*mesh);
        Api::CalculatePlanarUVs(*mesh);

        SerializeWhiteBox();
        RebuildWhiteBox();
    }

    void EditorWhiteBoxComponent::StampCubeCells(
        WhiteBoxMesh* grid, const AZStd::unordered_set<AZ::u64>& cells, const float cellSize, const bool add)
    {
        if (grid == nullptr || cells.empty())
        {
            return;
        }

        // Build the affected cells as their own clean, watertight, per-cell manifold surface -
        // a valid CSG boolean operand (greedy merging can leave T-junctions on a non-convex
        // cluster, which the boolean rejects).
        Api::WhiteBoxMeshPtr cube = Api::CreateWhiteBoxMesh();
        VoxelDetail::GenerateSurfacePerCell(*cube, cells, cellSize);
        Api::CalculateNormals(*cube);
        Api::CalculatePlanarUVs(*cube);

        const bool gridEmpty = Api::MeshFaceHandles(*grid).empty();
        const auto addDirect = [&]()
        {
            VoxelDetail::GenerateSurfacePerCell(*grid, cells, cellSize);
            Api::CalculateNormals(*grid);
            Api::CalculatePlanarUVs(*grid);
        };

        if (add)
        {
            // Union the new cubes into the grid so cubes stay watertight/manifold among
            // themselves; fall back to a direct add if the grid is empty or not a manifold.
            if (gridEmpty ||
                !Api::ApplyMeshBoolean(*grid, *cube, AZ::Transform::CreateIdentity(), Api::BooleanOperation::Union))
            {
                addDirect();
            }
            
        }
        else if (!gridEmpty)
        {
            // Carve: subtract the cubes from this grid only (the freeform mesh is never touched).
            const bool ok =
                Api::ApplyMeshBoolean(*grid, *cube, AZ::Transform::CreateIdentity(), Api::BooleanOperation::Subtraction);
            
        }
    }

    void EditorWhiteBoxComponent::SetVoxelCell(const AZ::Vector3& cellMin, const bool filled)
    {
        SetVoxelCells(AZStd::vector<AZ::Vector3>{cellMin}, filled);
    }

    void EditorWhiteBoxComponent::SetVoxelCells(const AZStd::vector<AZ::Vector3>& cellMins, const bool filled)
    {
        if (cellMins.empty())
        {
            return;
        }

        NormalizeVoxelData();

        const AZ::s32 sizeKey = VoxelDetail::QuantizeSize(m_drawUnitCubeSize);
        const float cellSize = VoxelDetail::SizeFromQuantized(sizeKey);
        const AZ::u8 newMerged = m_mergeGridWithMesh ? AZ::u8{1} : AZ::u8{0};

        // Occupancy: which cells are filled, and per cell whether it is a "merged" cube (lives
        // in the merged grid) or a "separate" cube (lives in the separate grid). A stamp adds
        // to the CURRENT toggle's grid; a carve removes each cell from whichever grid it is in.
        VoxelDetail::SizeMergeGroups groups =
            VoxelDetail::GroupBySizeMerge(m_voxelCells, m_voxelCellSizes, m_voxelMerged);
        VoxelDetail::CellMerge& occupancy = groups[sizeKey];

        VoxelDetail::CellSet affectedMerged;
        VoxelDetail::CellSet affectedSeparate;
        for (const AZ::Vector3& cellMin : cellMins)
        {
            const AZ::u64 key = VoxelDetail::PackCell(
                static_cast<int>(std::floor(cellMin.GetX())), static_cast<int>(std::floor(cellMin.GetY())),
                static_cast<int>(std::floor(cellMin.GetZ())));
            if (filled)
            {
                const auto inserted = occupancy.emplace(key, newMerged);
                if (inserted.second)
                {
                    (newMerged != 0 ? affectedMerged : affectedSeparate).insert(key);
                }
            }
            else
            {
                const auto it = occupancy.find(key);
                if (it != occupancy.end())
                {
                    (it->second != 0 ? affectedMerged : affectedSeparate).insert(key);
                    occupancy.erase(it);
                }
            }
        }

        if (affectedMerged.empty() && affectedSeparate.empty())
        {
            return; // nothing to add or carve (cells were already in the requested state)
        }

        AzToolsFramework::ScopedUndoBatch undoBatch(filled ? "Stamp Cube" : "Carve Cube");
        VoxelDetail::FlattenMergeGroups(groups, m_voxelCells, m_voxelCellSizes, m_voxelMerged);

        // How many cubes remain in each grid AFTER this operation. A carve that removes the
        // last cube from a grid must reset that grid mesh directly: subtracting the final cube
        // produces an empty CSG result, which the boolean reports as failure and leaves the
        // grid unchanged, stranding that last cube.
        bool anyMerged = false;
        bool anySeparate = false;
        for (const AZ::u8 mergedFlag : m_voxelMerged)
        {
            if (mergedFlag != 0)
            {
                anyMerged = true;
            }
            else
            {
                anySeparate = true;
            }
        }

        if (!affectedMerged.empty())
        {
            if (!filled && !anyMerged)
            {
                m_gridMergedMesh = Api::CreateWhiteBoxMesh(); // carved the last merged cube
            }
            else
            {
                StampCubeCells(GridMergedMesh(), affectedMerged, cellSize, filled);
            }
        }
        if (!affectedSeparate.empty())
        {
            if (!filled && !anySeparate)
            {
                m_gridMesh = Api::CreateWhiteBoxMesh(); // carved the last separate cube
            }
            else
            {
                StampCubeCells(GridMesh(), affectedSeparate, cellSize, filled);
            }
        }

        SerializeWhiteBox();
        RebuildWhiteBox();
        undoBatch.MarkEntityDirty(GetEntityId());
    }

    AZ::Crc32 EditorWhiteBoxComponent::ClearVoxelCubes()
    {
        NormalizeVoxelData();

        if (m_voxelCells.empty())
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        // The stamp/grid layer is separate from the freeform mesh, so clearing removes only
        // the stamped cubes and leaves any freeform / draw-shape geometry intact.
        AzToolsFramework::ScopedUndoBatch undoBatch("Clear Cube Stamp");
        m_voxelCells.clear();
        m_voxelCellSizes.clear();
        m_voxelMerged.clear();
        m_gridMesh = Api::CreateWhiteBoxMesh();
        m_gridMergedMesh = Api::CreateWhiteBoxMesh();
        Api::WriteMesh(*m_gridMesh, m_gridMeshData);
        Api::WriteMesh(*m_gridMergedMesh, m_gridMergedData);
        RebuildWhiteBox();
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorWhiteBoxComponent::FixNonManifoldMesh()
    {
        WhiteBoxMesh* mesh = GetWhiteBoxMesh();
        if (mesh == nullptr)
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        // Weld coincident vertices and regroup coplanar faces so the whole mesh becomes a
        // clean manifold. A single non-manifold region otherwise blocks every boolean.
        AzToolsFramework::ScopedUndoBatch undoBatch("Fix Non-Manifold Mesh");
        if (Api::RepairMesh(*mesh))
        {
            SerializeWhiteBox();
            RebuildWhiteBox();
            undoBatch.MarkEntityDirty(GetEntityId());
        }
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorWhiteBoxComponent::CreateChildLayer()
    {
        // All of the entity/component-mode work lives in the standalone layer utility.
        CreateChildWhiteBoxLayer(GetEntityId());
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

    void EditorWhiteBoxComponent::WhiteBoxLayer::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxLayer>()
                ->Version(1)
                ->Field("Name", &WhiteBoxLayer::m_name)
                ->Field("Id", &WhiteBoxLayer::m_id)
                ->Field("Visible", &WhiteBoxLayer::m_visible)
                ->Field("Tint", &WhiteBoxLayer::m_tint)
                ->Field("Combine", &WhiteBoxLayer::m_combineMode)
                ->Field("InvertNormals", &WhiteBoxLayer::m_invertNormals)
                ->Field("Position", &WhiteBoxLayer::m_position)
                ->Field("Rotation", &WhiteBoxLayer::m_rotation)
                ->Field("Scale", &WhiteBoxLayer::m_scale)
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
        layer.m_freeformData = m_whiteBoxData;
        layer.m_gridData = m_gridMeshData;
        layer.m_gridMergedData = m_gridMergedData;
        layer.m_voxelCells = m_voxelCells;
        layer.m_voxelCellSizes = m_voxelCellSizes;
        layer.m_voxelMerged = m_voxelMerged;
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
        const WhiteBoxLayer& layer = m_layers[m_activeLayerIndex];
        m_whiteBoxData = layer.m_freeformData;
        m_gridMeshData = layer.m_gridData;
        m_gridMergedData = layer.m_gridMergedData;
        m_voxelCells = layer.m_voxelCells;
        m_voxelCellSizes = layer.m_voxelCellSizes;
        m_voxelMerged = layer.m_voxelMerged;

        m_whiteBox = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*m_whiteBox, m_whiteBoxData); // new/empty layers stay empty (no default cube)
        m_gridMesh = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*m_gridMesh, m_gridMeshData);
        m_gridMergedMesh = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*m_gridMergedMesh, m_gridMergedData);
        m_loadedLayerIndex = m_activeLayerIndex;
        m_loadedLayerId = m_layers[m_activeLayerIndex].m_id;
        m_lastLayerCount = static_cast<int>(m_layers.size());
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

    Api::WhiteBoxMeshPtr EditorWhiteBoxComponent::BuildLayerMesh(const WhiteBoxLayer& layer)
    {
        // Deserialize a stored layer and combine its geometry into one display mesh.
        Api::WhiteBoxMeshPtr freeform = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*freeform, layer.m_freeformData);
        Api::WhiteBoxMeshPtr grid = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*grid, layer.m_gridData);
        Api::WhiteBoxMeshPtr gridMerged = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*gridMerged, layer.m_gridMergedData);
        Api::WhiteBoxMeshPtr combined = CombineFreeformAndGrids(freeform.get(), grid.get(), gridMerged.get());
        if (combined)
        {
            ApplyTransformToMesh(*combined, layer.m_position, layer.m_rotation, layer.m_scale);
        }
        return combined;
    }

    AZStd::vector<AZStd::pair<int, AZStd::string>> EditorWhiteBoxComponent::GetLayerNames()
    {
        AZStd::vector<AZStd::pair<int, AZStd::string>> names;
        if (m_layers.empty())
        {
            // Never return an empty value list: an empty combo would select index -1 and log
            // "Out of range combo box index -1". A single placeholder keeps value 0 valid until a
            // layer is created (drawing into the empty component auto-creates "Layer 1").
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
        if (m_activeLayerIndex == m_loadedLayerIndex)
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

    AZ::Crc32 EditorWhiteBoxComponent::OnNewLayer()
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("New White Box Layer");
        SerializeWhiteBox(); // commit the current layer
        WhiteBoxLayer layer;
        layer.m_name = AZStd::string::format("Layer %d", static_cast<int>(m_layers.size()) + 1);
        layer.m_id = AllocLayerId();
        m_layers.push_back(AZStd::move(layer));
        m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
        m_lastLayerCount = static_cast<int>(m_layers.size());
        LoadActiveLayer(); // load the new empty layer
        m_lastLayerSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnDeleteLayer()
    {
        if (m_layers.empty() || m_activeLayerIndex < 0 || m_activeLayerIndex >= static_cast<int>(m_layers.size()))
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("Delete White Box Layer");
        m_layers.erase(m_layers.begin() + m_activeLayerIndex);
        m_lastLayerCount = static_cast<int>(m_layers.size());
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
            m_loadedLayerIndex = -1; // force a reload of the working members
            LoadActiveLayer();
        }
        m_lastLayerSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::u32 EditorWhiteBoxComponent::OnLayersMetaChanged()
    {
        // If the id order/count changed, the list was added-to, removed-from or reordered (some
        // O3DE versions deliver these via this ChangeNotify) - do the full structural resync.
        if (LayerSignature() != m_lastLayerSignature)
        {
            SyncLayerStructure();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        // Otherwise this is an in-place edit (name / visibility / combine mode / transform). Keep
        // the working index pointing at the loaded layer and recombine to reflect the change.
        const int k = IndexOfLayerId(m_loadedLayerId);
        if (k >= 0)
        {
            m_loadedLayerIndex = k;
        }
        RebuildWhiteBox();
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    void EditorWhiteBoxComponent::RefreshComponentMode()
    {
        EditorWhiteBoxComponentModeRequestBus::Event(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()),
            &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);
    }

    void EditorWhiteBoxComponent::ClearWorkingLayer()
    {
        // Reset the working members to empty geometry. Used when there are zero layers so the
        // white box shows nothing and the stamp occupancy is cleared.
        m_whiteBox = Api::CreateWhiteBoxMesh();
        m_gridMesh = Api::CreateWhiteBoxMesh();
        m_gridMergedMesh = Api::CreateWhiteBoxMesh();
        m_voxelCells.clear();
        m_voxelCellSizes.clear();
        m_voxelMerged.clear();
        Api::WriteMesh(*m_whiteBox, m_whiteBoxData);
        Api::WriteMesh(*m_gridMesh, m_gridMeshData);
        Api::WriteMesh(*m_gridMergedMesh, m_gridMergedData);
        m_loadedLayerIndex = -1;
        m_loadedLayerId = 0;
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

        const bool isActive = m_activeLayerIndex == m_loadedLayerIndex;
        if (isActive)
        {
            SerializeWhiteBox(); // flush the working members into this layer's streams first
        }

        // Fold the grids into the freeform, bake the transform into every vertex, then clear the
        // grids + voxel occupancy (rotated/scaled cubes cannot map back to axis-aligned voxels, so
        // the stamps are frozen into the mesh) and reset the transform to identity.
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
            m_loadedLayerIndex = -1; // force the working members to reload from the baked streams
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
            const int k = IndexOfLayerId(m_loadedLayerId);
            if (k >= 0)
            {
                // The working members still belong to a live layer: the list was reordered or
                // added to. Just reindex (so the render skips the correct slot) and keep the
                // selection on the same layer - no reload, so no in-progress edits are lost.
                m_loadedLayerIndex = k;
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
                m_loadedLayerIndex = -1;
                LoadActiveLayer();
            }
        }

        m_lastLayerCount = count;
        m_lastLayerSignature = LayerSignature();
        RebuildWhiteBox();
        RefreshComponentMode();
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
    }

    void EditorWhiteBoxComponent::OnEdgesOnlyChange()
    {
        // Edges-only hides the solid render mesh; the edges themselves are drawn in
        // DisplayEntityViewport so the shape still reads as a wireframe.
        if (m_edgesOnly)
        {
            HideRenderMesh();
        }
        else if (AzToolsFramework::IsEntityVisible(GetEntityId()))
        {
            ShowRenderMesh();
        }
    }

    bool EditorWhiteBoxComponent::BuildColliderMesh(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices)
    {
        vertices.clear();
        indices.clear();

        // Only override the collider when this mesh actually has stamped cubes; otherwise
        // let the collider use its normal per-face path (default shapes, freeform, CSG).
        if (m_voxelCells.empty())
        {
            return false;
        }

        WhiteBoxMesh* mesh = GetWhiteBoxMesh();
        if (mesh == nullptr)
        {
            return false;
        }

        const float cellSize = m_voxelCellSize < 0.05f ? 0.05f : m_voxelCellSize;
        const AZStd::unordered_set<AZ::u64> cells(m_voxelCells.begin(), m_voxelCells.end());
        const VoxelDetail::FaceSignatureSet voxelSigs = VoxelDetail::SurfaceFaceSignatures(cells, cellSize);

        // 1. Keep per-face triangles for anything that is NOT part of the voxel surface
        //    (hand-edited / freeform faces), welded by white box vertex handle.
        AZStd::unordered_map<int, AZ::u32> vmap;
        const auto vtx = [&](const Api::VertexHandle& vh) -> AZ::u32
        {
            const int key = vh.Index();
            const auto it = vmap.find(key);
            if (it != vmap.end())
            {
                return it->second;
            }
            const AZ::u32 idx = static_cast<AZ::u32>(vertices.size());
            vertices.push_back(Api::VertexPosition(*mesh, vh));
            vmap.emplace(key, idx);
            return idx;
        };
        for (const Api::FaceHandle& fh : Api::MeshFaceHandles(*mesh))
        {
            VoxelDetail::FaceSignature sig;
            if (VoxelDetail::FaceSignatureFromPositions(Api::FaceVertexPositions(*mesh, fh), cellSize, sig) &&
                voxelSigs.find(sig) != voxelSigs.end())
            {
                continue; // voxel face - emitted by the greedy pass below
            }
            const auto he = Api::FaceHalfedgeHandles(*mesh, fh);
            if (he.size() < 3)
            {
                continue;
            }
            AZStd::vector<AZ::u32> fi;
            fi.reserve(he.size());
            for (const auto& h : he)
            {
                fi.push_back(vtx(Api::HalfedgeVertexHandleAtTip(*mesh, h)));
            }
            for (size_t i = 1; i + 1 < fi.size(); ++i)
            {
                if (fi[0] == fi[i] || fi[i] == fi[i + 1] || fi[0] == fi[i + 1])
                {
                    continue;
                }
                indices.push_back(fi[0]);
                indices.push_back(fi[i]);
                indices.push_back(fi[i + 1]);
            }
        }

        const int freeformTris = static_cast<int>(indices.size() / 3);

        // 2. Greedy-meshed triangles for the voxel surface (far fewer than per cell).
        VoxelDetail::GreedyColliderTriangles(cells, cellSize, vertices, indices);
        const int totalTris = static_cast<int>(indices.size() / 3);

        

        return !indices.empty();
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::EvaluatedMesh()
    {
        // When a stamp/grid layer exists it is combined with the freeform mesh (see
        // RebuildCombinedMesh) so render / collision / bounds / selection all see both.
        if (m_combinedMesh)
        {
            return m_combinedMesh.get();
        }
        // Non-destructive: render/collide/select against the evaluated result while
        // the editable base (GetWhiteBoxMesh) stays untouched.
        if (m_liveBoolean && m_displayMesh)
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
        const bool hasMerged = m_gridMergedMesh && !Api::MeshFaceHandles(*m_gridMergedMesh).empty();
        const bool hasSeparate = m_gridMesh && !Api::MeshFaceHandles(*m_gridMesh).empty();
        if (freeform == nullptr || (!hasMerged && !hasSeparate))
        {
            return nullptr; // no grid layer -> caller uses the freeform mesh as-is
        }
        Api::WhiteBoxMeshPtr combined = Api::CloneMesh(*freeform);
        if (!combined)
        {
            return nullptr;
        }

        // Merged cubes: CSG-union with the freeform so they read as one watertight solid. This
        // is a throwaway OUTPUT copy - the real freeform and grid meshes are never modified, so
        // removing a stamped cube can never damage the freeform it overlapped. Falls back to a
        // plain append if the union is rejected (e.g. the freeform mesh is not a valid manifold).
        if (hasMerged)
        {
            if (Api::MeshFaceHandles(*combined).empty() ||
                !Api::ApplyMeshBoolean(
                    *combined, *m_gridMergedMesh, AZ::Transform::CreateIdentity(), Api::BooleanOperation::Union))
            {
                AppendMesh(*combined, *m_gridMergedMesh);
            }
        }

        // Separate cubes: always appended alongside the freeform (their own islands).
        if (hasSeparate)
        {
            AppendMesh(*combined, *m_gridMesh);
        }

        Api::CalculateNormals(*combined);
        Api::CalculatePlanarUVs(*combined);
        return combined;
    }

    Api::WhiteBoxMeshPtr EditorWhiteBoxComponent::BuildCombined(WhiteBoxMesh* activeFreeform)
    {
        const int count = static_cast<int>(m_layers.size());
        const int activeIdx = m_loadedLayerIndex;

        AZStd::vector<int> visible;
        visible.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            if (m_layers[i].m_visible)
            {
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
        const bool activeNoGrid = !((m_gridMesh && !Api::MeshFaceHandles(*m_gridMesh).empty()) ||
                                    (m_gridMergedMesh && !Api::MeshFaceHandles(*m_gridMergedMesh).empty()));

        // Fast path: only the active layer is visible, identity transform, no grid -> return null so
        // EvaluatedMesh falls back to the raw working freeform (no clone, original behaviour).
        if (visible.size() == 1 && visible[0] == activeIdx && activeIdentity && activeNoGrid)
        {
            return nullptr;
        }

        // Accumulate the visible layers in list order (index 0 = bottom). The first visible layer is
        // the base; every subsequent layer combines with the running result per its combine mode.
        Api::WhiteBoxMeshPtr acc;
        for (const int idx : visible)
        {
            Api::WhiteBoxMeshPtr mesh;
            if (idx == activeIdx)
            {
                mesh = CombinedWithGrid(activeFreeform);
                if (!mesh && activeFreeform != nullptr)
                {
                    mesh = Api::CloneMesh(*activeFreeform);
                }
                if (mesh && !activeIdentity)
                {
                    ApplyTransformToMesh(
                        *mesh, m_layers[activeIdx].m_position, m_layers[activeIdx].m_rotation,
                        m_layers[activeIdx].m_scale);
                }
            }
            else
            {
                mesh = BuildLayerMesh(m_layers[idx]); // already transformed
            }
            if (!mesh)
            {
                continue;
            }
            if (!acc)
            {
                acc = AZStd::move(mesh); // first visible layer is the base (its own mode is ignored)
                continue;
            }
            const AZ::Transform identity = AZ::Transform::CreateIdentity();
            // Inter-layer booleans only apply in global-tint mode. With per-layer tint on, each
            // layer stays a separate coloured island so its colour is unambiguous.
            const LayerCombineMode mode = m_useGlobalTint ? m_layers[idx].m_combineMode : LayerCombineMode::Separate;
            switch (mode)
            {
            case LayerCombineMode::Union:
                if (Api::MeshFaceHandles(*acc).empty() ||
                    !Api::ApplyMeshBoolean(*acc, *mesh, identity, Api::BooleanOperation::Union))
                {
                    AppendMesh(*acc, *mesh);
                }
                break;
            case LayerCombineMode::Subtract:
                Api::ApplyMeshBoolean(*acc, *mesh, identity, Api::BooleanOperation::Subtraction);
                break;
            case LayerCombineMode::Intersect:
                Api::ApplyMeshBoolean(*acc, *mesh, identity, Api::BooleanOperation::Intersection);
                break;
            case LayerCombineMode::Separate:
            default:
                AppendMesh(*acc, *mesh);
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
        // When the entity boolean is live and applies to the WHOLE mesh (not just the active layer),
        // m_displayMesh already folds in every layer, grid and transform, so use it directly.
        if (m_liveBoolean && m_displayMesh && !m_booleanAffectActiveOnly)
        {
            m_combinedMesh = Api::CloneMesh(*m_displayMesh);
            return;
        }

        // Otherwise the active-layer base is the live-boolean result (active-only mode) or the raw
        // editable mesh; BuildCombined folds in the grids, the other layers and all transforms.
        WhiteBoxMesh* freeform = (m_liveBoolean && m_displayMesh) ? m_displayMesh.get() : GetWhiteBoxMesh();
        m_combinedMesh = BuildCombined(freeform);
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::GetLiveBooleanDisplayMesh()
    {
        return m_displayMesh.get();
    }

    Api::WhiteBoxMeshPtr EditorWhiteBoxComponent::EvaluateBooleanMesh()
    {
        if (!m_booleanSourceEntity.IsValid() || m_booleanSourceEntity == GetEntityId())
        {
            return nullptr;
        }

        WhiteBoxMesh* baseMesh = GetWhiteBoxMesh();
        if (baseMesh == nullptr)
        {
            return nullptr;
        }

        AZ::Entity* sourceEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, m_booleanSourceEntity);
        if (sourceEntity == nullptr)
        {
            return nullptr;
        }
        const auto sourceComponents = sourceEntity->FindComponents<EditorWhiteBoxComponent>();
        if (sourceComponents.empty())
        {
            return nullptr;
        }
        WhiteBoxMesh* sourceMesh = sourceComponents[0]->GetEvaluatedWhiteBoxMesh();
        if (sourceMesh == nullptr)
        {
            return nullptr;
        }

        AZ::Transform thisWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(thisWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Transform sourceWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(sourceWorldTM, m_booleanSourceEntity, &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Transform operandTransform = thisWorldTM.GetInverse() * sourceWorldTM;

        // Evaluate into a fresh mesh so the editable base is never modified. By default the cut
        // applies to the WHOLE combined geometry (every visible layer, its grid/stamps and its
        // transform); "Affect only the active layer" restricts it to the active layer's base mesh.
        Api::WhiteBoxMeshPtr evaluated;
        if (m_booleanAffectActiveOnly)
        {
            evaluated = Api::CloneMesh(*baseMesh);
        }
        else
        {
            evaluated = BuildCombined(baseMesh); // full base combined (no live boolean -> no recursion)
            if (!evaluated)
            {
                evaluated = Api::CloneMesh(*baseMesh);
            }
        }
        if (!evaluated)
        {
            return nullptr;
        }
        if (Api::ApplyMeshBoolean(*evaluated, *sourceMesh, operandTransform, m_booleanOperation))
        {
            Api::CalculateNormals(*evaluated);
            Api::CalculatePlanarUVs(*evaluated);
            return evaluated;
        }
        // on failure (no overlap) return null -> callers fall back to the base mesh.
        return nullptr;
    }

    void EditorWhiteBoxComponent::EvaluateLiveBoolean()
    {
        // Always evaluate the boolean whenever a source is set, independent of the live flag,
        // so the baked game-mode boolean variant exists even when the live boolean is off.
        // This is what lets runtime Lua toggle the boolean on from a base start. The live flag
        // only affects what EvaluatedMesh() returns (what is displayed / used by the edit-time
        // collider); the game entity always receives both variants when a source is present.
        m_displayMesh = EvaluateBooleanMesh();

        // Cache the true (uncut) base render data (serialized) so the game-mode bake always has
        // the base variant, even on a clone where GetWhiteBoxMesh() is null. Use the full combined
        // base (all visible layers, grids and transforms) so the bake matches the editor view.
        if (WhiteBoxMesh* baseMesh = GetWhiteBoxMesh())
        {
            const Api::WhiteBoxMeshPtr combined = BuildCombined(baseMesh);
            m_bakedBaseRenderData = CreateWhiteBoxRenderData(combined ? *combined : *baseMesh, m_material);
        }

        // Cache the boolean-evaluated render data (serialized) so the game-mode bake can supply the
        // boolean render variant even when BuildGameEntity runs on a cloned entity (m_displayMesh
        // null). In whole-mesh mode m_displayMesh is already the full combined result; in
        // active-only mode fold the grids/other layers around the active boolean result.
        if (m_displayMesh)
        {
            const Api::WhiteBoxMeshPtr combined =
                m_booleanAffectActiveOnly ? BuildCombined(m_displayMesh.get()) : nullptr;
            m_bakedBooleanRenderData =
                CreateWhiteBoxRenderData(combined ? *combined : *m_displayMesh, m_material);
        }
        else if (!m_booleanSourceEntity.IsValid() || m_booleanSourceEntity == GetEntityId())
        {
            // no boolean source -> there is no boolean variant; clear the cache
            m_bakedBooleanRenderData = WhiteBoxRenderData{};
        }
        // else: a source is set but evaluation transiently failed (e.g. the source entity is
        // not active yet at load time). Keep any previously cached boolean render data.
    }

    void EditorWhiteBoxComponent::UpdateBooleanSourceListener()
    {
        m_booleanSourceListener.BusDisconnect();
        // listen whenever a source is set (not just while live) so the baked boolean stays
        // current when the source entity moves, even with the live boolean off.
        if (m_booleanSourceEntity.IsValid() && m_booleanSourceEntity != GetEntityId())
        {
            m_booleanSourceListener.m_owner = this;
            m_booleanSourceListener.BusConnect(m_booleanSourceEntity);
        }
    }

    AZ::u32 EditorWhiteBoxComponent::OnLiveBooleanChange()
    {
        UpdateBooleanSourceListener();
        RebuildWhiteBox(); // evaluates the live boolean (or reverts to base) + rebuilds render/physics
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::u32 EditorWhiteBoxComponent::OnBooleanSourceChange()
    {
        OnLiveBooleanChange();
        // The Boolean group's visibility depends on whether a source is set. EntityId
        // fields don't reliably honor the ChangeNotify refresh-level return value, so
        // force the property tree to rebuild explicitly so the group shows/hides.
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorWhiteBoxComponent::BooleanGroupVisibility() const
    {
        return m_booleanSourceEntity.IsValid() ? AZ::Edit::PropertyVisibility::Show
                                               : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 EditorWhiteBoxComponent::DrawUnitCubeSizeVisibility() const
    {
        // The Cube Size control only matters while the Unit Cube Stamp tool is active.
        return m_drawUnitCube ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void EditorWhiteBoxComponent::BooleanSourceListener::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        if (m_owner != nullptr)
        {
            m_owner->RebuildWhiteBox(); // source moved -> re-evaluate + rebuild
        }
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
                ->Field("Stair", &DrawShapeData::m_stair);

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

    void EditorWhiteBoxComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorWhiteBoxMeshAsset::Reflect(context);
        DrawShapeData::Reflect(context);
        WhiteBoxLayer::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWhiteBoxComponent, EditorComponentBase>()
                ->Version(2, &EditorWhiteBoxVersionConverter)
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
                ->Field("DrawCarve", &EditorWhiteBoxComponent::m_drawCarve)
                ->Field("DrawMergeUnion", &EditorWhiteBoxComponent::m_drawMergeUnion)
                ->Field("EdgesOnly", &EditorWhiteBoxComponent::m_edgesOnly)
                ->Field("MergeGridWithMesh", &EditorWhiteBoxComponent::m_mergeGridWithMesh)
                ->Field("UseGlobalTint", &EditorWhiteBoxComponent::m_useGlobalTint)
                ->Field("MaterialOverride", &EditorWhiteBoxComponent::m_materialOverrideAssetId)
                ->Field("DrawUnitCube", &EditorWhiteBoxComponent::m_drawUnitCube)
                ->Field("DrawUnitCubeSize", &EditorWhiteBoxComponent::m_drawUnitCubeSize)
                ->Field("VoxelCellSize", &EditorWhiteBoxComponent::m_voxelCellSize)
                ->Field("DrawUnitCubeShowGrid", &EditorWhiteBoxComponent::m_drawUnitCubeShowGrid)
                ->Field("VoxelCells", &EditorWhiteBoxComponent::m_voxelCells)
                ->Field("VoxelCellSizes", &EditorWhiteBoxComponent::m_voxelCellSizes)
                ->Field("GridMeshData", &EditorWhiteBoxComponent::m_gridMeshData)
                ->Field("GridMergedData", &EditorWhiteBoxComponent::m_gridMergedData)
                ->Field("VoxelMerged", &EditorWhiteBoxComponent::m_voxelMerged)
                ->Field("BooleanSource", &EditorWhiteBoxComponent::m_booleanSourceEntity)
                ->Field("BooleanOp", &EditorWhiteBoxComponent::m_booleanOperation)
                ->Field("BooleanHideSource", &EditorWhiteBoxComponent::m_hideSourceAfterApply)
                ->Field("BooleanDeleteSource", &EditorWhiteBoxComponent::m_deleteSourceAfterApply)
                ->Field("BooleanAffectActive", &EditorWhiteBoxComponent::m_booleanAffectActiveOnly)
                ->Field("BooleanLive", &EditorWhiteBoxComponent::m_liveBoolean)
                ->Field("BakedBooleanRenderData", &EditorWhiteBoxComponent::m_bakedBooleanRenderData)
                ->Field("BakedBaseRenderData", &EditorWhiteBoxComponent::m_bakedBaseRenderData);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
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
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_defaultShape,
                        QT_TRANSLATE_NOOP("WhiteBox", "Default Shape"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Default shape of the white box mesh."))
                    ->EnumAttribute(DefaultShapeType::Cube, QT_TRANSLATE_NOOP("WhiteBox", "Cube"))
                    ->EnumAttribute(DefaultShapeType::Tetrahedron, QT_TRANSLATE_NOOP("WhiteBox", "Tetrahedron"))
                    ->EnumAttribute(DefaultShapeType::Icosahedron, QT_TRANSLATE_NOOP("WhiteBox", "Icosahedron"))
                    ->EnumAttribute(DefaultShapeType::Cylinder, QT_TRANSLATE_NOOP("WhiteBox", "Cylinder"))
                    ->EnumAttribute(DefaultShapeType::Sphere, QT_TRANSLATE_NOOP("WhiteBox", "Sphere"))
                    ->EnumAttribute(DefaultShapeType::Asset, QT_TRANSLATE_NOOP("WhiteBox", "Mesh Asset"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnDefaultShapeChange)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Layers")
                    // Placed as the FIRST child of the group and separated from the New/Delete
                    // buttons by the Active Layer combo: the property editor drops the LAST button
                    // in a run of consecutive UIElements, so three buttons in a row would lose one.
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Bake the active layer's Position/Rotation/Scale into its geometry and reset them to identity "
                        "(so drawing / edge-restore line up). Stamps on the layer are frozen into the mesh."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnApplyLayerTransform)
                    ->Attribute(AZ::Edit::Attributes::ButtonText,
                    QT_TRANSLATE_NOOP("WhiteBox", "Apply Transform (Active Layer)"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_activeLayerIndex,
                        QT_TRANSLATE_NOOP("WhiteBox", "Active Layer"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Which layer edits (draw / stamp / carve) target. Switching commits the current layer and "
                        "loads the selected one."))
                    ->Attribute(AZ::Edit::Attributes::GenericValueList, &EditorWhiteBoxComponent::GetLayerNames)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnActiveLayerChange)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Add a new empty layer and make it the edit target."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnNewLayer)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "New Layer"))
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Delete the active layer (at least one layer is always kept)."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnDeleteLayer)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Delete Layer"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_layers,
                        QT_TRANSLATE_NOOP("WhiteBox", "Layer List"),
                        QT_TRANSLATE_NOOP("WhiteBox", "All layers. Edit a layer name or toggle its visibility here; use New/Delete Layer above to "
                        "add or remove layers."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLayersMetaChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_drawShapeData,
                        QT_TRANSLATE_NOOP("WhiteBox", "Draw Shape"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Draw Shape tool settings."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_drawCarve,
                        QT_TRANSLATE_NOOP("WhiteBox", "Carve (Boolean)"),
                        QT_TRANSLATE_NOOP("WhiteBox", "When on, drawing performs a CSG boolean (same as holding Ctrl): pull into the surface to "
                        "carve/subtract, pull out to add/union."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_drawMergeUnion,
                        QT_TRANSLATE_NOOP("WhiteBox", "Merge Draw Shape (Union)"),
                        QT_TRANSLATE_NOOP("WhiteBox", "When on, committing a drawn shape CSG-unions it into the mesh (a clean, watertight, "
                        "manifold merge with no T-junctions) instead of leaving overlapping geometry."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_drawUnitCube,
                        QT_TRANSLATE_NOOP("WhiteBox", "Unit Cube Stamp"),
                        QT_TRANSLATE_NOOP("WhiteBox", "In draw mode, press to place a grid-snapped cube (of Cube Size) and drag to extrude its "
                        "depth along the surface, then release to stamp it (union; hold Ctrl before drawing to subtract)."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::SpinBox, &EditorWhiteBoxComponent::m_drawUnitCubeSize,
                        QT_TRANSLATE_NOOP("WhiteBox", "Cube Size"),
                        QT_TRANSLATE_NOOP("WhiteBox", "World-space size of the next stamped cube. Each click places a single clean cube (8 verts, "
                        "6 faces) of this size; drag to lay more. You can change this at any time - each cube keeps "
                        "its own size, so new cubes of the new size sit alongside ones you already placed (no need "
                        "to clear the stamp first)."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::DrawUnitCubeSizeVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_drawUnitCubeShowGrid,
                        QT_TRANSLATE_NOOP("WhiteBox", "Show Cube Grid Preview"),
                        QT_TRANSLATE_NOOP("WhiteBox", "When on, the stamp ghost shows the individual cubes (a grid); when off it shows a single "
                        "outer box."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::DrawUnitCubeSizeVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_mergeGridWithMesh,
                        QT_TRANSLATE_NOOP("WhiteBox", "Merge With Mesh"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Applies to cubes stamped AFTER you change it (per-cube, not global). When ON, a stamped cube "
                        "is CSG-unioned with the freeform mesh for display so it reads as one watertight solid; when "
                        "OFF it sits as a separate island. Either way the merge is non-destructive - the freeform mesh "
                        "is never modified, so removing a cube can never damage it - and Carve/Clear only affect cubes."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::DrawUnitCubeSizeVisibility)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Remove every cube placed with the Unit Cube Stamp tool."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ClearVoxelCubes)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Clear Cube Stamp"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::DrawUnitCubeSizeVisibility)
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Weld coincident vertices and regroup coplanar faces so the whole mesh is a clean manifold. "
                        "Run this if a boolean fails because part of the mesh is non-manifold."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::FixNonManifoldMesh)
                    ->Attribute(AZ::Edit::Attributes::ButtonText,
                    QT_TRANSLATE_NOOP("WhiteBox", "Fix Non-Manifold Mesh"))
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Create a new layer: a child entity with its own White Box component, and switch edit focus "
                        "to it so the next shape/cube you draw goes into the new layer."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::CreateChildLayer)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "New Layer (Child)"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_edgesOnly,
                        QT_TRANSLATE_NOOP("WhiteBox", "Edges Only"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Hide the solid render mesh and draw only the mesh edges (wireframe-style preview)."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnEdgesOnlyChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_editorMeshAsset,
                        QT_TRANSLATE_NOOP("WhiteBox", "Editor Mesh Asset"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Editor Mesh Asset"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::AssetVisibility)
                    ->UIElement(AZ::Edit::UIHandlers::Button,
                        QT_TRANSLATE_NOOP("WhiteBox", "Save as asset"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Save as asset"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::SaveAsAsset)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Save As ..."))
                    ->UIElement(
                        AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Add a White Box collider component to this entity so it has physics collision."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnAddCollision)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Add Collision"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_useGlobalTint,
                        QT_TRANSLATE_NOOP("WhiteBox", "Use Global Tint"),
                        QT_TRANSLATE_NOOP("WhiteBox", "When on, every layer renders with the global White Box Material tint below. When off, each "
                        "layer uses its own per-layer Tint (set in the Layer List)."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnGlobalTintChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_material,
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box Material"),
                        QT_TRANSLATE_NOOP("WhiteBox", "The properties of the White Box material (global tint)."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnMaterialChange)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::GlobalTintVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_componentModeDelegate,
                        QT_TRANSLATE_NOOP("WhiteBox", "Component Mode"),
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box Tool Component Mode"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Export to obj"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ExportToFile)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Export"))
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Export all whiteboxes on descendant entities as a single obj (excluding this one)"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ExportDescendantsToFile)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Export Descendants"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorWhiteBoxComponent::m_flipYZForExport,
                        QT_TRANSLATE_NOOP("WhiteBox", "Flip Y and Z for Export"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Flip the Y and Z axes when exportings so they aren't imported sideways into coord systems where the Y-axis goes up."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_booleanSourceEntity,
                        QT_TRANSLATE_NOOP("WhiteBox", "Boolean Source"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Another entity with a White Box component to use as the boolean operand."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnBooleanSourceChange)
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("WhiteBox", "Boolean"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::BooleanGroupVisibility)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    // Apply Boolean is placed as the FIRST child of the group: a UIElement
                    // that is the LAST child of a group is dropped by the property editor.
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                        QT_TRANSLATE_NOOP("WhiteBox", "Apply the boolean using the source entity's mesh"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ApplyBoolean)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Apply Boolean"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::BooleanGroupVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_booleanOperation,
                        QT_TRANSLATE_NOOP("WhiteBox", "Boolean Operation"),
                        QT_TRANSLATE_NOOP("WhiteBox", "How to combine the source mesh with this one."))
                    ->EnumAttribute(Api::BooleanOperation::Subtraction, QT_TRANSLATE_NOOP("WhiteBox", "Subtract"))
                    ->EnumAttribute(Api::BooleanOperation::Union, QT_TRANSLATE_NOOP("WhiteBox", "Union"))
                    ->EnumAttribute(Api::BooleanOperation::Intersection, QT_TRANSLATE_NOOP("WhiteBox", "Intersect"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLiveBooleanChange)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::BooleanGroupVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_liveBoolean,
                        QT_TRANSLATE_NOOP("WhiteBox", "Non-Destructive (Live)"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Keep this mesh editable and show the boolean result live (re-evaluates when the source "
                        "moves or either mesh changes). Leave off to use the one-shot Apply Boolean button."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLiveBooleanChange)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::BooleanGroupVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorWhiteBoxComponent::m_booleanAffectActiveOnly,
                        QT_TRANSLATE_NOOP("WhiteBox", "Affect only the active layer"),
                        QT_TRANSLATE_NOOP("WhiteBox", "When off (default) the boolean cuts the whole combined mesh (all visible layers, stamps "
                        "and transforms). When on it only cuts the active layer's base mesh."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLiveBooleanChange)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::BooleanGroupVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_hideSourceAfterApply,
                        QT_TRANSLATE_NOOP("WhiteBox", "Hide Source After Apply"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Hide the source entity once the boolean is applied."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::BooleanGroupVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_deleteSourceAfterApply,
                        QT_TRANSLATE_NOOP("WhiteBox", "Delete Source After Apply"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Delete the source entity once the boolean is applied."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::BooleanGroupVisibility);
            }
        }
    }

    void EditorWhiteBoxComponent::OnMaterialChange()
    {
        if (m_renderMesh.has_value())
        {
            WhiteBoxMaterial material = m_material;
            material.m_useVertexColor = !m_useGlobalTint; // per-layer tint travels in the vertex colours
            (*m_renderMesh)->UpdateMaterial(material);
            m_renderData.m_material = material;
        }
    }

    void EditorWhiteBoxComponent::SetMaterialTint(const AZ::Color& tint)
    {
        m_material.m_tint = tint.GetAsVector3();
        OnMaterialChange(); // push the new tint to the live material instance
    }

    AZ::Color EditorWhiteBoxComponent::GetMaterialTint()
    {
        return AZ::Color(m_material.m_tint);
    }

    void EditorWhiteBoxComponent::SetMaterialUseTexture(bool useTexture)
    {
        m_material.m_useTexture = useTexture;
        OnMaterialChange();
    }

    void EditorWhiteBoxComponent::SetMaterialOverride(const AZ::Data::AssetId& materialAssetId)
    {
        if (m_materialOverrideAssetId == materialAssetId)
        {
            return;
        }
        m_materialOverrideAssetId = materialAssetId;
        // The material asset is baked into the model when it is created, so force the render mesh to
        // be recreated (drop to a null render mesh first) with the new material.
        if (m_renderMesh.has_value())
        {
            m_renderMesh.emplace(AZStd::make_unique<WhiteBoxNullRenderMesh>(AZ::EntityId{}));
            RebuildRenderMesh();
        }
    }

    AZ::Data::AssetId EditorWhiteBoxComponent::GetMaterialOverride()
    {
        return m_materialOverrideAssetId;
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

    AZ::u32 EditorWhiteBoxComponent::OnGlobalTintChange()
    {
        RebuildWhiteBox();                                 // recolour (and re-fold, since combine
                                                           // modes only apply in global-tint mode)
        return AZ::Edit::PropertyRefreshLevels::EntireTree; // show / hide the global tint element
    }

    AZ::Crc32 EditorWhiteBoxComponent::GlobalTintVisibility() const
    {
        return m_useGlobalTint ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    WhiteBoxRenderData EditorWhiteBoxComponent::BuildColoredRenderData()
    {
        WhiteBoxRenderData renderData;
        renderData.m_material = m_material;
        renderData.m_material.m_useVertexColor = !m_useGlobalTint;

        WhiteBoxMesh* freeform = (m_liveBoolean && m_displayMesh) ? m_displayMesh.get() : GetWhiteBoxMesh();
        const int count = static_cast<int>(m_layers.size());
        const int activeIdx = m_loadedLayerIndex;
        const bool activeIdentity = activeIdx >= 0 && activeIdx < count &&
            m_layers[activeIdx].m_position.IsZero() && m_layers[activeIdx].m_rotation.IsZero() &&
            m_layers[activeIdx].m_scale.IsClose(AZ::Vector3::CreateOne(), 1e-6f);

        // Each visible layer contributes its own faces tinted with its colour. (In per-layer-tint
        // mode BuildCombined keeps the layers as separate islands, so this matches the geometry.)
        for (int i = 0; i < count; ++i)
        {
            if (!m_layers[i].m_visible)
            {
                continue;
            }
            Api::WhiteBoxMeshPtr mesh;
            if (i == activeIdx)
            {
                mesh = CombinedWithGrid(freeform);
                if (!mesh && freeform != nullptr)
                {
                    mesh = Api::CloneMesh(*freeform);
                }
                if (mesh && !activeIdentity)
                {
                    ApplyTransformToMesh(
                        *mesh, m_layers[i].m_position, m_layers[i].m_rotation, m_layers[i].m_scale);
                }
            }
            else
            {
                mesh = BuildLayerMesh(m_layers[i]);
            }
            if (!mesh)
            {
                continue;
            }
            const AZ::Vector3& t = m_layers[i].m_tint;
            const AZ::Vector4 color = m_useGlobalTint
                ? AZ::Vector4::CreateOne()
                : AZ::Vector4(t.GetX(), t.GetY(), t.GetZ(), 1.0f);
            WhiteBoxRenderData layerData = CreateWhiteBoxRenderData(*mesh, m_material, m_layers[i].m_invertNormals);
            for (WhiteBoxFace& face : layerData.m_faces)
            {
                face.m_color = color;
            }
            renderData.m_faces.insert(
                renderData.m_faces.end(), layerData.m_faces.begin(), layerData.m_faces.end());
        }
        return renderData;
    }

    void EditorWhiteBoxComponent::BuildLayerRenderMeshes()
    {
        m_layerRenderMeshes.clear();

        WhiteBoxMesh* freeform = (m_liveBoolean && m_displayMesh) ? m_displayMesh.get() : GetWhiteBoxMesh();
        const int count = static_cast<int>(m_layers.size());
        const int activeIdx = m_loadedLayerIndex;
        const bool activeIdentity = activeIdx >= 0 && activeIdx < count &&
            m_layers[activeIdx].m_position.IsZero() && m_layers[activeIdx].m_rotation.IsZero() &&
            m_layers[activeIdx].m_scale.IsClose(AZ::Vector3::CreateOne(), 1e-6f);

        for (int i = 0; i < count; ++i)
        {
            if (!m_layers[i].m_visible)
            {
                continue;
            }
            Api::WhiteBoxMeshPtr mesh;
            if (i == activeIdx)
            {
                mesh = CombinedWithGrid(freeform);
                if (!mesh && freeform != nullptr)
                {
                    mesh = Api::CloneMesh(*freeform);
                }
                if (mesh && !activeIdentity)
                {
                    ApplyTransformToMesh(
                        *mesh, m_layers[i].m_position, m_layers[i].m_rotation, m_layers[i].m_scale);
                }
            }
            else
            {
                mesh = BuildLayerMesh(m_layers[i]);
            }
            if (!mesh || Api::MeshFaceHandles(*mesh).empty())
            {
                continue;
            }

            WhiteBoxRenderData layerData = CreateWhiteBoxRenderData(*mesh, m_material);
            if (layerData.m_faces.empty())
            {
                continue;
            }

            AZStd::unique_ptr<RenderMeshInterface> renderMesh;
            WhiteBoxRequestBus::BroadcastResult(
                renderMesh, &WhiteBoxRequests::CreateAuxiliaryRenderMeshInterface, GetEntityId());
            if (!renderMesh)
            {
                continue;
            }
            renderMesh->BuildMesh(layerData, m_worldFromLocal);

            WhiteBoxMaterial layerMaterial = m_material;
            layerMaterial.m_tint = m_layers[i].m_tint; // per-layer tint via baseColor.color
            layerMaterial.m_useVertexColor = false;
            renderMesh->UpdateMaterial(layerMaterial);

            m_layerRenderMeshes.push_back(AZStd::move(renderMesh));
        }
    }

    AZ::Crc32 EditorWhiteBoxComponent::AssetVisibility() const
    {
        return DisplayingAsset(m_defaultShape) ? AZ::Edit::PropertyVisibility::ShowChildrenOnly
                                               : AZ::Edit::PropertyVisibility::Hide;
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
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
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
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(entityId);
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

        // re-evaluate the live boolean and listen for the source entity moving
        UpdateBooleanSourceListener();
        EvaluateLiveBoolean();
        RebuildCombinedMesh(); // fold in the stamp/grid layer so it renders on load

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
        AZ::TickBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
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

    void EditorWhiteBoxComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        // The reflected layer container's native add/remove ("+"/"-") does not reliably invoke
        // the DataElement ChangeNotify, so a structural edit made directly in the property grid
        // would otherwise go undetected until the next stamp/draw. Poll the layer count here and
        // resync the working state (and the rendered mesh) the moment it changes.
        // The layer container's own add/remove/reorder does not always notify, so poll the id order.
        if (LayerSignature() != m_lastLayerSignature)
        {
            SyncLayerStructure();
        }
    }

    void EditorWhiteBoxComponent::DeserializeWhiteBox()
    {
        // Migrate a LEGACY scene (single mesh stored in the loose fields, no layer list) that
        // actually has geometry into one layer. Components with no layers and no loose geometry
        // (freshly added, or intentionally emptied by deleting all layers) stay empty.
        const bool looseHasGeometry = !m_whiteBoxData.empty() || !m_gridMeshData.empty() ||
            !m_gridMergedData.empty() || !m_voxelCells.empty();
        if (m_layers.empty() && looseHasGeometry)
        {
            WhiteBoxLayer layer;
            layer.m_name = "Layer 1";
            layer.m_freeformData = m_whiteBoxData;
            layer.m_gridData = m_gridMeshData;
            layer.m_gridMergedData = m_gridMergedData;
            layer.m_voxelCells = m_voxelCells;
            layer.m_voxelCellSizes = m_voxelCellSizes;
            layer.m_voxelMerged = m_voxelMerged;
            m_layers.push_back(AZStd::move(layer));
            m_activeLayerIndex = 0;
        }

        // Ensure every layer has a stable id (migrate old scenes) and keep the allocator ahead.
        for (const WhiteBoxLayer& existing : m_layers)
        {
            if (existing.m_id != 0 && existing.m_id >= m_nextLayerId)
            {
                m_nextLayerId = existing.m_id + 1;
            }
        }
        for (WhiteBoxLayer& toId : m_layers)
        {
            if (toId.m_id == 0)
            {
                toId.m_id = AllocLayerId();
            }
        }

        m_lastLayerCount = static_cast<int>(m_layers.size());
        m_lastLayerSignature = LayerSignature();

        if (m_layers.empty())
        {
            // No layers -> an empty white box (nothing renders, no stamps).
            ClearWorkingLayer();
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

        // Point the working streams at the active layer, then build the working meshes from them.
        const WhiteBoxLayer& layer = m_layers[m_activeLayerIndex];
        m_whiteBoxData = layer.m_freeformData;
        m_gridMeshData = layer.m_gridData;
        m_gridMergedData = layer.m_gridMergedData;
        m_voxelCells = layer.m_voxelCells;
        m_voxelCellSizes = layer.m_voxelCellSizes;
        m_voxelMerged = layer.m_voxelMerged;

        m_whiteBox = Api::CreateWhiteBoxMesh();
        if (m_editorMeshAsset->InUse())
        {
            m_editorMeshAsset->Load();
        }
        else
        {
            const auto result = Api::ReadMesh(*m_whiteBox, m_whiteBoxData);
            AZ_Error(
                "EditorWhiteBoxComponent", result != WhiteBox::Api::ReadResult::Error,
                "Error deserializing white box mesh stream");
        }

        m_gridMesh = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*m_gridMesh, m_gridMeshData);
        m_gridMergedMesh = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*m_gridMergedMesh, m_gridMergedData);
        m_loadedLayerIndex = m_activeLayerIndex;
        m_loadedLayerId = m_layers[m_activeLayerIndex].m_id;
    }

    void EditorWhiteBoxComponent::RebuildWhiteBox()
    {
        EvaluateLiveBoolean(); // refresh m_displayMesh so render/physics/bounds use the latest result
        RebuildCombinedMesh(); // fold the stamp/grid layer into the mesh used for output
        RebuildRenderMesh();
        RebuildPhysicsMesh();
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

        // Base render geometry: the live base mesh, else the cached base data (m_renderData may be the cut result).
        if (WhiteBoxMesh* baseMesh = GetWhiteBoxMesh())
        {
            const Api::WhiteBoxMeshPtr combined = CombinedWithGrid(baseMesh);
            whiteBoxComponent->GenerateWhiteBoxMesh(
                CreateWhiteBoxRenderData(combined ? *combined : *baseMesh, m_material));
        }
        else if (!m_bakedBaseRenderData.m_faces.empty())
        {
            whiteBoxComponent->GenerateWhiteBoxMesh(m_bakedBaseRenderData);
        }
        else
        {
            whiteBoxComponent->GenerateWhiteBoxMesh(m_renderData);
        }

        // The boolean variant can only be evaluated in the Editor, and its source is not resolvable during
        // the game build, so use the cached display mesh, or the serialized render data on a clone.
        WhiteBoxRenderData booleanRenderData;
        if (WhiteBoxMesh* displayMesh = GetLiveBooleanDisplayMesh())
        {
            const Api::WhiteBoxMeshPtr combined = CombinedWithGrid(displayMesh);
            booleanRenderData = CreateWhiteBoxRenderData(combined ? *combined : *displayMesh, m_material);
        }
        else
        {
            booleanRenderData = m_bakedBooleanRenderData;
        }

        if (!booleanRenderData.m_faces.empty())
        {
            whiteBoxComponent->SetBooleanRenderData(booleanRenderData);
            whiteBoxComponent->SetLiveBooleanState(true, m_liveBoolean);
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

    void EditorWhiteBoxComponent::RebuildRenderMesh()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // reset caches when the mesh changes
        m_worldAabb.reset();
        m_localAabb.reset();
        m_faces.reset();

        AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(GetEntityId());

        // must have been created in Activate or have had the Entity made visible again
        if (m_renderMesh.has_value())
        {
            m_layerRenderMeshes.clear(); // single mesh (with per-vertex colours) serves both modes

            // Use the per-layer build when per-layer tint is on OR any visible layer inverts its
            // normals (both need per-layer faces). Otherwise the CSG-combined mesh is used so
            // inter-layer booleans render correctly.
            bool anyLayerInverted = false;
            for (const WhiteBoxLayer& layer : m_layers)
            {
                if (layer.m_visible && layer.m_invertNormals)
                {
                    anyLayerInverted = true;
                    break;
                }
            }
            const bool perLayerRender = !m_useGlobalTint || anyLayerInverted;

            // Global tint -> plain (white-vertex) faces coloured by the material baseColor. Per-layer
            // tint -> per-face vertex colours (needs the material's "Use Vertex Color" enabled).
            m_renderData = perLayerRender ? BuildColoredRenderData()
                                          : CreateWhiteBoxRenderData(*EvaluatedMesh(), m_material);

            // it's possible the white box mesh data isn't yet ready (for example if it's stored
            // in an asset which hasn't finished loading yet) so don't attempt to create a render
            // mesh with no data
            if (!m_renderData.m_faces.empty())
            {
                // check if we need to instantiate a concrete render mesh implementation
                if (IsWhiteBoxNullRenderMesh(m_renderMesh))
                {
                    // create a concrete implementation of the render mesh
                    WhiteBoxRequestBus::BroadcastResult(m_renderMesh, &WhiteBoxRequests::CreateRenderMeshInterface, GetEntityId());
                }

                // apply any external material asset override before the model is (re)created
                (*m_renderMesh)->SetMaterialAssetOverride(m_materialOverrideAssetId);

                // generate the mesh
                (*m_renderMesh)->BuildMesh(m_renderData, m_worldFromLocal);
                OnMaterialChange();
            }
            else if (!IsWhiteBoxNullRenderMesh(m_renderMesh))
            {
                // The geometry became empty (e.g. every layer was deleted) while a concrete render
                // mesh is still showing the previous geometry. Building a zero-size mesh makes the
                // RHI reject the (empty) buffers, so instead swap the concrete mesh back to a null
                // render mesh: it draws nothing and stays rebuildable.
                m_renderMesh.emplace(AZStd::make_unique<WhiteBoxNullRenderMesh>(AZ::EntityId{}));
            }
        }

        EditorWhiteBoxComponentModeRequestBus::Event(
            AZ::EntityComponentIdPair{GetEntityId(), GetId()},
            &EditorWhiteBoxComponentModeRequests::MarkWhiteBoxIntersectionDataDirty);
    }

    void EditorWhiteBoxComponent::WriteAssetToComponent()
    {
        if (m_editorMeshAsset->Loaded())
        {
            Api::WriteMesh(*m_editorMeshAsset->GetWhiteBoxMesh(), m_whiteBoxData);
        }
    }

    void EditorWhiteBoxComponent::SerializeWhiteBox()
    {
        if (m_editorMeshAsset->Loaded())
        {
            m_editorMeshAsset->Serialize();
        }
        else
        {
            Api::WriteMesh(*m_whiteBox, m_whiteBoxData);
        }

        // The stamp/grid layers are component-local, persist them alongside the freeform mesh.
        if (m_gridMesh)
        {
            Api::WriteMesh(*m_gridMesh, m_gridMeshData);
        }
        if (m_gridMergedMesh)
        {
            Api::WriteMesh(*m_gridMergedMesh, m_gridMergedData);
        }

        // If there are no layers yet but the working mesh now has geometry (a draw/stamp into an
        // empty white box), start a first layer so the edit is preserved.
        if (m_layers.empty())
        {
            const bool hasGeometry =
                (m_whiteBox != nullptr && !Api::MeshFaceHandles(*m_whiteBox).empty()) || !m_voxelCells.empty();
            if (hasGeometry)
            {
                WhiteBoxLayer layer;
                layer.m_name = "Layer 1";
                layer.m_id = AllocLayerId();
                m_layers.push_back(AZStd::move(layer));
                m_activeLayerIndex = 0;
                m_loadedLayerIndex = 0;
                m_loadedLayerId = m_layers[0].m_id;
                // Deliberately leave m_lastLayerCount unchanged so the tick handler notices the
                // 0 -> 1 layer change and refreshes the property grid (the new layer appears).
            }
        }

        // Mirror the just-written working state into the layer it belongs to so the serialized
        // layer list always reflects the latest edits.
        StoreLayer(m_loadedLayerIndex);
    }

    void EditorWhiteBoxComponent::SetDefaultShape(const DefaultShapeType defaultShape)
    {
        m_defaultShape = defaultShape;
        OnDefaultShapeChange();
    }

    void EditorWhiteBoxComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_worldAabb.reset();
        m_localAabb.reset();

        m_worldFromLocal = world;

        if (m_renderMesh.has_value())
        {
            (*m_renderMesh)->UpdateTransform(world);
        }
        for (auto& layerRenderMesh : m_layerRenderMeshes)
        {
            if (layerRenderMesh)
            {
                layerRenderMesh->UpdateTransform(world);
            }
        }

        // moving this entity changes the cut relative to the source; re-evaluate whenever a
        // source is set (not only while live) so the baked boolean variant stays current.
        if (m_booleanSourceEntity.IsValid() && m_booleanSourceEntity != GetEntityId())
        {
            RebuildWhiteBox();
        }
    }

    void EditorWhiteBoxComponent::RebuildPhysicsMesh()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        EditorWhiteBoxColliderRequestBus::Event(
            GetEntityId(), &EditorWhiteBoxColliderRequests::CreatePhysics, *EvaluatedMesh());
    }

    static AZStd::string WhiteBoxPathAtProjectRoot(const AZStd::string_view name, const AZStd::string_view extension)
    {
        AZ::IO::Path whiteBoxPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(whiteBoxPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
        }
        whiteBoxPath /= AZ::IO::FixedMaxPathString::format("%.*s.%.*s", AZ_STRING_ARG(name), AZ_STRING_ARG(extension));
        return whiteBoxPath.Native();
    }

    void EditorWhiteBoxComponent::ExportToFile()
    {
        const AZStd::string initialAbsolutePathToExport =
            WhiteBoxPathAtProjectRoot(GetEntity()->GetName(), ObjExtension);

        const QString fileFilter = AZStd::string::format("*.%s", ObjExtension).c_str();
        const QString absoluteSaveFilePath = AzQtComponents::FileDialog::GetSaveFileName(
            nullptr, "Save As...", QString(initialAbsolutePathToExport.c_str()), fileFilter);

        if (m_flipYZForExport)
        {
            Api::VertexHandles vHandles = Api::MeshVertexHandles(*GetWhiteBoxMesh());
            for (auto& handle : vHandles)
            {
                AZ::Vector3 p = Api::VertexPosition(*GetWhiteBoxMesh(), handle);
                float temp = p.GetY();
                p.SetY(p.GetZ());
                p.SetZ(-temp);
                Api::SetVertexPosition(*GetWhiteBoxMesh(), handle, p);
            }
        }

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();
        if (WhiteBox::Api::SaveToObj(*GetWhiteBoxMesh(), absoluteSaveFilePathCstr))
        {
            AZ_Printf("EditorWhiteBoxComponent", "Exported white box mesh to: %s", absoluteSaveFilePathCstr);
            RequestEditSourceControl(absoluteSaveFilePathCstr);
        }
        else
        {
            AZ_Warning(
                "EditorWhiteBoxComponent", false, "Failed to export white box mesh to: %s", absoluteSaveFilePathCstr);
        }
    }

    void EditorWhiteBoxComponent::ExportDescendantsToFile()
    {
        // Get all child entities in the viewport
        AzToolsFramework::EntityIdList children;
        AZ::TransformBus::EventResult(children, GetEntityId(), &AZ::TransformBus::Events::GetAllDescendants);

        if (children.empty())
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Failed to export descendant whitebox meshes: No descendant entities found.");
            return;
        }

        const AZStd::string initialAbsolutePathToExport = WhiteBoxPathAtProjectRoot(GetEntity()->GetName(), ObjExtension);

        const QString fileFilter = AZStd::string::format("*.%s", ObjExtension).c_str();
        const QString absoluteSaveFilePath =
            AzQtComponents::FileDialog::GetSaveFileName(nullptr, "Save As...", QString(initialAbsolutePathToExport.c_str()), fileFilter);

        // Create a new empty white box mesh
        Api::WhiteBoxMeshPtr mesh = Api::CreateWhiteBoxMesh();
        for (auto& id : children)
        {
            AZ::Entity* e;
            AZ::ComponentApplicationBus::BroadcastResult(e, &AZ::ComponentApplicationRequests::FindEntity, id);
            AZ::Transform worldTM = e->GetTransform()->GetWorldTM();

            // Add all polys from selected white boxes
            for (auto component : e->FindComponents<EditorWhiteBoxComponent>())
            {
                WhiteBoxMesh* m = component->GetWhiteBoxMesh();
                Api::PolygonHandles polys = Api::MeshPolygonHandles(*m);
                for (auto& poly : polys)
                {
                    AZStd::vector<AZ::Vector3> verts = Api::PolygonVertexPositions(*m, poly);
                    if (verts.size() == 4) // if this is in fact a quad
                    {
                        Api::VertexHandle vertexHandles[4];

                        for (unsigned int i = 0; i < 4; i++)
                        {
                            AZ::Vector3 worldV = worldTM.TransformPoint(verts[i]);
                            if (m_flipYZForExport)
                            {
                                float temp = worldV.GetY();
                                worldV.SetY(worldV.GetZ());
                                worldV.SetZ(-temp);
                            }
                            vertexHandles[i] = Api::AddVertex(*mesh.get(), worldV);
                        }
                        Api::AddQuadPolygon(*mesh.get(), vertexHandles[0], vertexHandles[1], vertexHandles[2], vertexHandles[3]);
                    }
                }
            }
        }

        Api::CalculateNormals(*mesh.get());
        Api::CalculatePlanarUVs(*mesh.get());

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();
        if (WhiteBox::Api::SaveToObj(*mesh.get(), absoluteSaveFilePathCstr))
        {
            AZ_Printf("EditorWhiteBoxComponent", "Exported white box mesh to: %s", absoluteSaveFilePathCstr);
            RequestEditSourceControl(absoluteSaveFilePathCstr);
        }
        else
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Failed to export white box mesh to: %s", absoluteSaveFilePathCstr);
        }
    }

    AZStd::optional<WhiteBoxSaveResult> TrySaveAs(
        const AZStd::string_view entityName,
        const AZStd::function<AZStd::string(const AZStd::string&)>& absoluteSavePathFn,
        const AZStd::function<AZStd::optional<AZStd::string>(const AZStd::string&)>& relativePathFn,
        const AZStd::function<int()>& saveDecisionFn)
    {
        const AZStd::string initialAbsolutePathToSave =
            WhiteBoxPathAtProjectRoot(entityName, Pipeline::WhiteBoxMeshAssetHandler::AssetFileExtension);

        const QString absoluteSaveFilePath = QString(absoluteSavePathFn(initialAbsolutePathToSave).c_str());

        // user pressed cancel
        if (absoluteSaveFilePath.isEmpty())
        {
            return AZStd::nullopt;
        }

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();

        const AZStd::optional<AZStd::string> relativePath =
            relativePathFn(AZStd::string(absoluteSaveFilePathCstr, absoluteSaveFilePathUtf8.length()));

        if (!relativePath.has_value())
        {
            int saveDecision = saveDecisionFn();

            // save the file but do not attempt to create an asset
            if (saveDecision == QMessageBox::Save)
            {
                return WhiteBoxSaveResult{AZStd::nullopt, AZStd::string(absoluteSaveFilePathCstr)};
            }

            // the user decided not to save the asset outside the project folder after the prompt
            return AZStd::nullopt;
        }

        return WhiteBoxSaveResult{relativePath, AZStd::string(absoluteSaveFilePathCstr)};
    }

    AZ::Crc32 EditorWhiteBoxComponent::SaveAsAsset()
    {
        // let the user select final location of the saved asset
        const auto absoluteSavePathFn = [](const AZStd::string& initialAbsolutePath)
        {
            const QString fileFilter =
                AZStd::string::format("WhiteBoxMesh (*.%s)", Pipeline::WhiteBoxMeshAssetHandler::AssetFileExtension).c_str();
            const QString absolutePath =
                AzQtComponents::FileDialog::GetSaveFileName(nullptr, "Save As Asset...", QString(initialAbsolutePath.c_str()), fileFilter);

            return AZStd::string(absolutePath.toUtf8());
        };

        // ask the asset system to try and convert the absolutePath to a cache relative path
        const auto relativePathFn = [](const AZStd::string& absolutePath) -> AZStd::optional<AZStd::string>
        {
            AZStd::string relativePath;
            bool foundRelativePath = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                foundRelativePath,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath,
                absolutePath,
                relativePath);

            if (foundRelativePath)
            {
                return relativePath;
            }

            return AZStd::nullopt;
        };

        // present the user with the option of accepting saving outside the project folder or allow them to cancel the
        // operation
        const auto saveDecisionFn = []()
        {
            return QMessageBox::warning(
                AzToolsFramework::GetActiveWindow(),
                "Warning",
                "Saving a White Box Mesh Asset (.wbm) outside of the project root will not create an Asset for the "
                "Component to use. The file will be saved but will not be processed. For live updates to happen the "
                "asset must be saved somewhere in the current project folder. Would you like to continue?",
                (QMessageBox::Save | QMessageBox::Cancel),
                QMessageBox::Cancel);
        };

        const AZStd::optional<WhiteBoxSaveResult> saveResult =
            TrySaveAs(GetEntity()->GetName(), absoluteSavePathFn, relativePathFn, saveDecisionFn);

        // user pressed cancel
        if (!saveResult.has_value())
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        const char* const absoluteSaveFilePath = saveResult.value().m_absoluteFilePath.c_str();
        if (saveResult.value().m_relativeAssetPath.has_value())
        {
            const auto& relativeAssetPath = saveResult.value().m_relativeAssetPath.value();

            // notify undo system the entity has been changed (m_meshAsset)
            AzToolsFramework::ScopedUndoBatch undoBatch(AssetSavedUndoRedoDesc);

            // if there was a previous asset selected, it has to be cloned to a new one
            // otherwise the internal mesh can simply be moved into the new asset
            m_editorMeshAsset->TakeOwnershipOfWhiteBoxMesh(
                relativeAssetPath,
                m_editorMeshAsset->InUse() ? Api::CloneMesh(*GetWhiteBoxMesh()) : AZStd::exchange(m_whiteBox, Api::CreateWhiteBoxMesh()));

            // change default shape to asset
            m_defaultShape = DefaultShapeType::Asset;

            // ensure this change gets tracked
            undoBatch.MarkEntityDirty(GetEntityId());

            RefreshProperties();

            m_editorMeshAsset->Save(absoluteSaveFilePath);
        }
        else
        {
            // save the asset to disk outside the project folder
            if (Api::SaveToWbm(*GetWhiteBoxMesh(), absoluteSaveFilePath))
            {
                RequestEditSourceControl(absoluteSaveFilePath);
            }
        }

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    template<typename TransformFn>
    AZ::Aabb CalculateAabb(const WhiteBoxMesh& whiteBox, TransformFn&& transformFn)
    {
        const auto vertexHandles = Api::MeshVertexHandles(whiteBox);
        return AZStd::accumulate(
            AZStd::cbegin(vertexHandles), AZStd::cend(vertexHandles), AZ::Aabb::CreateNull(), transformFn);
    }

    AZ::Aabb EditorWhiteBoxComponent::GetEditorSelectionBoundsViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
    {
        return GetWorldBounds();
    }

    AZ::Aabb EditorWhiteBoxComponent::GetWorldBounds() const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_worldAabb.has_value())
        {
            m_worldAabb = GetLocalBounds();
            m_worldAabb->ApplyTransform(m_worldFromLocal);
        }

        return m_worldAabb.value();
    }

    AZ::Aabb EditorWhiteBoxComponent::GetLocalBounds() const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_localAabb.has_value())
        {
            auto& whiteBoxMesh = *const_cast<EditorWhiteBoxComponent*>(this)->EvaluatedMesh();

            m_localAabb = CalculateAabb(
                whiteBoxMesh,
                [&whiteBox = whiteBoxMesh](AZ::Aabb aabb, const Api::VertexHandle vertexHandle)
                {
                    aabb.AddPoint(Api::VertexPosition(whiteBox, vertexHandle));
                    return aabb;
                });
        }

        return m_localAabb.value();
    }

    void EditorWhiteBoxComponent::BuildVisibleGeometry(const AZ::Aabb& bounds, AzFramework::VisibleGeometryContainer& geometryContainer) const
    {
        // Only add the white box geometry if its bounds overlap the input bounds
        if (bounds.IsValid() && !bounds.Overlaps(GetWorldBounds()))
        {
            return;
        }

        // Extract white box geometry data to convert to visible geometry vertices and indices
        const WhiteBoxRenderData renderData =
            CreateWhiteBoxRenderData(*const_cast<EditorWhiteBoxComponent*>(this)->EvaluatedMesh(), m_material);

        // Convert the white box render data into visible geometry data
        const AzFramework::VisibleGeometry geometry = BuildVisibleGeometryFromWhiteBoxRenderData(GetEntityId(), renderData);

        if (!geometry.m_indices.empty() && !geometry.m_vertices.empty())
        {
            geometryContainer.push_back(geometry);
        }
    }

    bool EditorWhiteBoxComponent::EditorSelectionIntersectRayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir,
        float& distance)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_faces.has_value())
        {
            m_faces = Api::MeshFaces(*EvaluatedMesh());
        }

        // must have at least one triangle
        if (m_faces->empty())
        {
            return false;
        }

        // transform ray into local space
        const AZ::Transform localFromWorld = m_worldFromLocal.GetInverse();

        // setup beginning/end of segment
        const float rayLength = 1000.0f;
        const AZ::Vector3 localRayOrigin = localFromWorld.TransformPoint(src);
        const AZ::Vector3 localRayDirection = localFromWorld.TransformVector(dir);
        const AZ::Vector3 localRayEnd = localRayOrigin + localRayDirection * rayLength;

        bool intersection = false;
        AZ::Intersect::SegmentTriangleHitTester hitTester(localRayOrigin, localRayEnd);
        for (const auto& face : m_faces.value())
        {
            float t;
            AZ::Vector3 normal;
            if (hitTester.IntersectSegmentTriangle(face[0], face[1], face[2], normal, t))
            {
                intersection = true;

                // find closest intersection
                const float dist = t * rayLength;
                if (dist < distance)
                {
                    distance = dist;
                }
            }
        }

        return intersection;
    }

    void EditorWhiteBoxComponent::OnEntityVisibilityChanged(const bool visibility)
    {
        if (visibility)
        {
            ShowRenderMesh();
        }
        else
        {
            HideRenderMesh();
        }
    }

    void EditorWhiteBoxComponent::ShowRenderMesh()
    {
        // if we wish to display the render mesh, set a null render mesh indicating a mesh can exist
        // note: if the optional remains empty, no render mesh will be created
        m_renderMesh.emplace(AZStd::make_unique<WhiteBoxNullRenderMesh>(AZ::EntityId{}));
        RebuildRenderMesh();
    }

    void EditorWhiteBoxComponent::HideRenderMesh()
    {
        // clear the optional (and any per-layer tinted meshes)
        m_layerRenderMeshes.clear();
        m_renderMesh.reset();
    }

    bool EditorWhiteBoxComponent::AssetInUse() const
    {
        return m_editorMeshAsset->InUse();
    }

    bool EditorWhiteBoxComponent::HasRenderMesh() const
    {
        // if the optional has a value we know a render mesh exists
        // note: This implicitly implies that the Entity is visible
        return m_renderMesh.has_value();
    }

    void EditorWhiteBoxComponent::OverrideEditorWhiteBoxMeshAsset(EditorWhiteBoxMeshAsset* editorMeshAsset)
    {
        // ensure we do not leak resources
        delete m_editorMeshAsset;

        m_editorMeshAsset = editorMeshAsset;
    }

    static bool DebugDrawingEnabled()
    {
        return cl_whiteBoxDebugVertexHandles || cl_whiteBoxDebugNormals || cl_whiteBoxDebugHalfedgeHandles ||
            cl_whiteBoxDebugEdgeHandles || cl_whiteBoxDebugFaceHandles || cl_whiteBoxDebugAabb;
    }

    static void WhiteBoxDebugRendering(
        const WhiteBoxMesh& whiteBoxMesh, const AZ::Transform& worldFromLocal,
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Aabb& editorBounds)
    {
        const AZ::Quaternion worldOrientationFromLocal = worldFromLocal.GetRotation();

        debugDisplay.DepthTestOn();

        for (const auto& faceHandle : Api::MeshFaceHandles(whiteBoxMesh))
        {
            const auto faceHalfedgeHandles = Api::FaceHalfedgeHandles(whiteBoxMesh, faceHandle);

            const AZ::Vector3 localFaceCenter =
                AZStd::accumulate(
                    faceHalfedgeHandles.cbegin(), faceHalfedgeHandles.cend(), AZ::Vector3::CreateZero(),
                    [&whiteBoxMesh](AZ::Vector3 start, const Api::HalfedgeHandle halfedgeHandle)
                    {
                        return start +
                            Api::VertexPosition(
                                   whiteBoxMesh, Api::HalfedgeVertexHandleAtTip(whiteBoxMesh, halfedgeHandle));
                    }) /
                3.0f;

            for (const auto& halfedgeHandle : faceHalfedgeHandles)
            {
                const Api::VertexHandle vertexHandleAtTip =
                    Api::HalfedgeVertexHandleAtTip(whiteBoxMesh, halfedgeHandle);
                const Api::VertexHandle vertexHandleAtTail =
                    Api::HalfedgeVertexHandleAtTail(whiteBoxMesh, halfedgeHandle);

                const AZ::Vector3 localTailPoint = Api::VertexPosition(whiteBoxMesh, vertexHandleAtTail);
                const AZ::Vector3 localTipPoint = Api::VertexPosition(whiteBoxMesh, vertexHandleAtTip);
                const AZ::Vector3 localFaceNormal = Api::FaceNormal(whiteBoxMesh, faceHandle);
                const AZ::Vector3 localHalfedgeCenter = (localTailPoint + localTipPoint) * 0.5f;

                // offset halfedge slightly based on the face it is associated with
                const AZ::Vector3 localHalfedgePositionWithOffset =
                    localHalfedgeCenter + ((localFaceCenter - localHalfedgeCenter).GetNormalized() * 0.1f);

                const AZ::Vector3 worldVertexPosition = worldFromLocal.TransformPoint(localTipPoint);
                const AZ::Vector3 worldHalfedgePosition =
                    worldFromLocal.TransformPoint(localHalfedgePositionWithOffset);
                const AZ::Vector3 worldNormal =
                    (worldOrientationFromLocal.TransformVector(localFaceNormal)).GetNormalized();

                if (cl_whiteBoxDebugVertexHandles)
                {
                    debugDisplay.SetColor(AZ::Colors::Cyan);
                    const AZStd::string vertex = AZStd::string::format("%d", vertexHandleAtTip.Index());
                    debugDisplay.DrawTextLabel(worldVertexPosition, 3.0f, vertex.c_str(), true, 0, 1);
                }

                if (cl_whiteBoxDebugHalfedgeHandles)
                {
                    debugDisplay.SetColor(AZ::Colors::LawnGreen);
                    const AZStd::string halfedge = AZStd::string::format("%d", halfedgeHandle.Index());
                    debugDisplay.DrawTextLabel(worldHalfedgePosition, 2.0f, halfedge.c_str(), true);
                }

                if (cl_whiteBoxDebugNormals)
                {
                    debugDisplay.SetColor(AZ::Colors::White);
                    debugDisplay.DrawBall(worldVertexPosition, 0.025f);
                    debugDisplay.DrawLine(worldVertexPosition, worldVertexPosition + worldNormal * 0.4f);
                }
            }

            if (cl_whiteBoxDebugFaceHandles)
            {
                debugDisplay.SetColor(AZ::Colors::White);
                const AZ::Vector3 worldFacePosition = worldFromLocal.TransformPoint(localFaceCenter);
                const AZStd::string face = AZStd::string::format("%d", faceHandle.Index());
                debugDisplay.DrawTextLabel(worldFacePosition, 2.0f, face.c_str(), true);
            }
        }

        if (cl_whiteBoxDebugEdgeHandles)
        {
            for (const auto& edgeHandle : Api::MeshEdgeHandles(whiteBoxMesh))
            {
                const AZ::Vector3 localEdgeMidpoint = Api::EdgeMidpoint(whiteBoxMesh, edgeHandle);
                const AZ::Vector3 worldEdgeMidpoint = worldFromLocal.TransformPoint(localEdgeMidpoint);
                debugDisplay.SetColor(AZ::Colors::CornflowerBlue);
                const AZStd::string edge = AZStd::string::format("%d", edgeHandle.Index());
                debugDisplay.DrawTextLabel(worldEdgeMidpoint, 2.0f, edge.c_str(), true);
            }
        }

        if (cl_whiteBoxDebugAabb)
        {
            debugDisplay.SetColor(AZ::Colors::Blue);
            debugDisplay.DrawWireBox(editorBounds.GetMin(), editorBounds.GetMax());
        }
    }

    void EditorWhiteBoxComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (DebugDrawingEnabled())
        {
            WhiteBoxDebugRendering(
                *GetWhiteBoxMesh(), m_worldFromLocal, debugDisplay,
                GetEditorSelectionBoundsViewport(AzFramework::ViewportInfo{}));
        }

        // Edges-only wireframe overlay: the solid render mesh is hidden, so draw the mesh's
        // polygon edges here so the shape is still visible.
        if (m_edgesOnly)
        {
            if (WhiteBoxMesh* mesh = EvaluatedMesh())
            {
                debugDisplay.SetColor(AZ::Color(0.30f, 0.90f, 1.0f, 1.0f));
                for (const Api::EdgeHandle& edgeHandle : Api::MeshPolygonEdgeHandles(*mesh))
                {
                    const AZStd::array<Api::VertexHandle, 2> edgeVerts = Api::EdgeVertexHandles(*mesh, edgeHandle);
                    debugDisplay.DrawLine(
                        m_worldFromLocal.TransformPoint(Api::VertexPosition(*mesh, edgeVerts[0])),
                        m_worldFromLocal.TransformPoint(Api::VertexPosition(*mesh, edgeVerts[1])));
                }
            }
        }
    }
} // namespace WhiteBox
