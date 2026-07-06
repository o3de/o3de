/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Asset/EditorWhiteBoxMeshAsset.h"
#include "Asset/WhiteBoxMeshAssetHandler.h"
#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentMode.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "Rendering/WhiteBoxNullRenderMesh.h"
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

    // build intermediate data to be passed to WhiteBoxRenderMeshInterface
    // to be used to generate concrete render mesh
    static WhiteBoxRenderData CreateWhiteBoxRenderData(const WhiteBoxMesh& whiteBox, const WhiteBoxMaterial& material)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        WhiteBoxRenderData renderData;
        WhiteBoxFaces& faceData = renderData.m_faces;

        const auto faceCount = Api::MeshFaceCount(whiteBox);
        faceData.reserve(faceCount);

        const auto createWhiteBoxFaceFromHandle = [&whiteBox](const Api::FaceHandle& faceHandle) -> WhiteBoxFace
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

            copyVertex(faceHalfedgeHandles[0], face.m_v1);
            copyVertex(faceHalfedgeHandles[1], face.m_v2);
            copyVertex(faceHalfedgeHandles[2], face.m_v3);

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

            AZ_Printf(
                "WhiteBoxVoxel", "GenerateSurface: %d cells -> %d quad polygons, %d verts",
                static_cast<int>(cells.size()), polygonCount, static_cast<int>(verts.size()));
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

        // Remove ONLY the faces that belong to the previous voxel surface, leaving every
        // freeform face (and any voxel face the user has since hand-edited off the lattice)
        // untouched. Each cube size lives on its own grid, so match faces per size group:
        // pre-compute the surface signatures for every old size, then keep any mesh face
        // whose corners quantise (at that size) onto one of those signatures.
        AZStd::unordered_map<AZ::s32, VoxelDetail::FaceSignatureSet> oldSigsBySize;
        for (const auto& group : oldGroups)
        {
            oldSigsBySize.emplace(
                group.first, VoxelDetail::SurfaceFaceSignatures(group.second, VoxelDetail::SizeFromQuantized(group.first)));
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

        // Add the new voxel surface, one grid per cube size.
        for (const auto& group : newGroups)
        {
            VoxelDetail::GenerateSurface(*mesh, group.second, VoxelDetail::SizeFromQuantized(group.first));
        }

        // Purge vertices orphaned by the face removal above. RemoveFaces keeps isolated
        // vertices, which is why clearing / re-stamping used to leave stray points visible
        // in vertex and edge editing modes; deleting them here keeps the mesh clean.
        Api::RemoveIsolatedVertices(*mesh);

        AZ_Printf(
            "WhiteBoxVoxel", "RegenerateVoxelMesh: old=%d new=%d cells (%d old sizes, %d new sizes), removedFaces=%d",
            static_cast<int>(oldCells.size()), static_cast<int>(newCells.size()), static_cast<int>(oldGroups.size()),
            static_cast<int>(newGroups.size()), removedFaces);

        Api::CalculateNormals(*mesh);
        Api::CalculatePlanarUVs(*mesh);

        SerializeWhiteBox();
        RebuildWhiteBox();
    }

    void EditorWhiteBoxComponent::SetVoxelCell(const AZ::Vector3& cellMin, const bool filled)
    {
        NormalizeVoxelData();

        const AZ::s32 sizeKey = VoxelDetail::QuantizeSize(m_drawUnitCubeSize);
        const AZ::u64 key = VoxelDetail::PackCell(
            static_cast<int>(std::floor(cellMin.GetX())), static_cast<int>(std::floor(cellMin.GetY())),
            static_cast<int>(std::floor(cellMin.GetZ())));

        // Snapshot the old state (keeps regen/undo in sync), then add or remove this cell
        // in the group for the CURRENT Cube Size - existing cubes of other sizes are left
        // exactly as they are, so the size can change freely without clearing the stamp.
        const AZStd::vector<AZ::u64> oldCells = m_voxelCells;
        const AZStd::vector<float> oldSizes = m_voxelCellSizes;

        VoxelDetail::SizeGroups groups = VoxelDetail::GroupBySize(oldCells, oldSizes);
        const bool changed =
            filled ? groups[sizeKey].insert(key).second : (groups[sizeKey].erase(key) > 0);
        if (!changed)
        {
            return; // already in the requested state
        }

        AzToolsFramework::ScopedUndoBatch undoBatch(filled ? "Stamp Voxel" : "Remove Voxel");
        VoxelDetail::FlattenGroups(groups, m_voxelCells, m_voxelCellSizes);
        RegenerateVoxelMesh(oldCells, oldSizes, m_voxelCells, m_voxelCellSizes);
        undoBatch.MarkEntityDirty(GetEntityId());
    }

    void EditorWhiteBoxComponent::SetVoxelCells(const AZStd::vector<AZ::Vector3>& cellMins, const bool filled)
    {
        if (cellMins.empty())
        {
            return;
        }

        NormalizeVoxelData();

        const AZ::s32 sizeKey = VoxelDetail::QuantizeSize(m_drawUnitCubeSize);

        // Snapshot the old state, then apply every cell in the CURRENT Cube Size's group in
        // one shot (single undo step + regen). Cubes already placed at other sizes are
        // untouched, so mixed sizes coexist and the size can be changed between stamps.
        const AZStd::vector<AZ::u64> oldCells = m_voxelCells;
        const AZStd::vector<float> oldSizes = m_voxelCellSizes;

        VoxelDetail::SizeGroups groups = VoxelDetail::GroupBySize(oldCells, oldSizes);
        VoxelDetail::CellSet& target = groups[sizeKey];

        bool changed = false;
        for (const AZ::Vector3& cellMin : cellMins)
        {
            const AZ::u64 key = VoxelDetail::PackCell(
                static_cast<int>(std::floor(cellMin.GetX())), static_cast<int>(std::floor(cellMin.GetY())),
                static_cast<int>(std::floor(cellMin.GetZ())));
            changed = (filled ? target.insert(key).second : (target.erase(key) > 0)) || changed;
        }

        if (!changed)
        {
            return; // every requested cell was already in the requested state
        }

        AzToolsFramework::ScopedUndoBatch undoBatch(filled ? "Stamp Cube" : "Remove Cube");
        VoxelDetail::FlattenGroups(groups, m_voxelCells, m_voxelCellSizes);
        RegenerateVoxelMesh(oldCells, oldSizes, m_voxelCells, m_voxelCellSizes);
        undoBatch.MarkEntityDirty(GetEntityId());
    }

    AZ::Crc32 EditorWhiteBoxComponent::ClearVoxelCubes()
    {
        NormalizeVoxelData();

        if (m_voxelCells.empty())
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        // Remove the whole voxel surface (regenerate against an empty set) and purge the
        // orphaned vertices. Freeform / hand-edited faces are left untouched.
        const AZStd::vector<AZ::u64> oldCells = m_voxelCells;
        const AZStd::vector<float> oldSizes = m_voxelCellSizes;
        AzToolsFramework::ScopedUndoBatch undoBatch("Clear Cube Stamp");
        m_voxelCells.clear();
        m_voxelCellSizes.clear();
        RegenerateVoxelMesh(oldCells, oldSizes, m_voxelCells, m_voxelCellSizes);
        undoBatch.MarkEntityDirty(GetEntityId());
        return AZ::Edit::PropertyRefreshLevels::None;
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

        AZ_Printf(
            "WhiteBoxCollider", "BuildColliderMesh: cells=%d freeformTris=%d greedyTris=%d totalTris=%d verts=%d",
            static_cast<int>(cells.size()), freeformTris, totalTris - freeformTris, totalTris,
            static_cast<int>(vertices.size()));

        return !indices.empty();
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::EvaluatedMesh()
    {
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
        WhiteBoxMesh* sourceMesh = sourceComponents[0]->GetWhiteBoxMesh();
        if (sourceMesh == nullptr)
        {
            return nullptr;
        }

        AZ::Transform thisWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(thisWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Transform sourceWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(sourceWorldTM, m_booleanSourceEntity, &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Transform operandTransform = thisWorldTM.GetInverse() * sourceWorldTM;

        // Evaluate into a clone so the editable base is never modified.
        Api::WhiteBoxMeshPtr evaluated = Api::CloneMesh(*baseMesh);
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
        // the base variant, even on a clone where GetWhiteBoxMesh() is null.
        if (WhiteBoxMesh* baseMesh = GetWhiteBoxMesh())
        {
            m_bakedBaseRenderData = CreateWhiteBoxRenderData(*baseMesh, m_material);
        }

        // Cache the boolean-evaluated render data (serialized) so the game-mode bake can supply
        // the boolean render variant even when BuildGameEntity runs on a cloned entity, where
        // the non-serialized m_displayMesh is null.
        if (m_displayMesh)
        {
            m_bakedBooleanRenderData = CreateWhiteBoxRenderData(*m_displayMesh, m_material);
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

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWhiteBoxComponent, EditorComponentBase>()
                ->Version(2, &EditorWhiteBoxVersionConverter)
                ->Field("WhiteBoxData", &EditorWhiteBoxComponent::m_whiteBoxData)
                ->Field("DefaultShape", &EditorWhiteBoxComponent::m_defaultShape)
                ->Field("EditorMeshAsset", &EditorWhiteBoxComponent::m_editorMeshAsset)
                ->Field("Material", &EditorWhiteBoxComponent::m_material)
                ->Field("RenderData", &EditorWhiteBoxComponent::m_renderData)
                ->Field("ComponentMode", &EditorWhiteBoxComponent::m_componentModeDelegate)
                ->Field("FlipYZForExport", &EditorWhiteBoxComponent::m_flipYZForExport)
                ->Field("DrawShapeData", &EditorWhiteBoxComponent::m_drawShapeData)
                ->Field("DrawCarve", &EditorWhiteBoxComponent::m_drawCarve)
                ->Field("DrawUnitCube", &EditorWhiteBoxComponent::m_drawUnitCube)
                ->Field("DrawUnitCubeSize", &EditorWhiteBoxComponent::m_drawUnitCubeSize)
                ->Field("VoxelCellSize", &EditorWhiteBoxComponent::m_voxelCellSize)
                ->Field("DrawUnitCubeShowGrid", &EditorWhiteBoxComponent::m_drawUnitCubeShowGrid)
                ->Field("VoxelCells", &EditorWhiteBoxComponent::m_voxelCells)
                ->Field("VoxelCellSizes", &EditorWhiteBoxComponent::m_voxelCellSizes)
                ->Field("BooleanSource", &EditorWhiteBoxComponent::m_booleanSourceEntity)
                ->Field("BooleanOp", &EditorWhiteBoxComponent::m_booleanOperation)
                ->Field("BooleanHideSource", &EditorWhiteBoxComponent::m_hideSourceAfterApply)
                ->Field("BooleanDeleteSource", &EditorWhiteBoxComponent::m_deleteSourceAfterApply)
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
                    ->UIElement(AZ::Edit::UIHandlers::Button, "",
                    QT_TRANSLATE_NOOP("WhiteBox", "Remove every cube placed with the Unit Cube Stamp tool."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ClearVoxelCubes)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("WhiteBox", "Clear Cube Stamp"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::DrawUnitCubeSizeVisibility)
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
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_material,
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box Material"),
                        QT_TRANSLATE_NOOP("WhiteBox", "The properties of the White Box material."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnMaterialChange)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
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
            (*m_renderMesh)->UpdateMaterial(m_material);
            m_renderData.m_material = m_material;
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

        if (AzToolsFramework::IsEntityVisible(entityId))
        {
            ShowRenderMesh();
            OnMaterialChange();
        }
    }

    void EditorWhiteBoxComponent::Deactivate()
    {
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
        m_renderMesh.reset();
        m_whiteBox.reset();
        m_displayMesh.reset();
    }

    void EditorWhiteBoxComponent::DeserializeWhiteBox()
    {
        // create WhiteBoxMesh object from internal data
        m_whiteBox = Api::CreateWhiteBoxMesh();

        if (m_editorMeshAsset->InUse())
        {
            m_editorMeshAsset->Load();
        }
        else
        {
            // attempt to load the mesh
            const auto result = Api::ReadMesh(*m_whiteBox, m_whiteBoxData);
            AZ_Error("EditorWhiteBoxComponent", result != WhiteBox::Api::ReadResult::Error, "Error deserializing white box mesh stream");

            // if the read was successful but the byte stream is empty
            // (there was nothing to load), create a default mesh
            if (result == Api::ReadResult::Empty)
            {
                Api::InitializeAsUnitCube(*m_whiteBox);
            }
        }
    }

    void EditorWhiteBoxComponent::RebuildWhiteBox()
    {
        EvaluateLiveBoolean(); // refresh m_displayMesh so render/physics/bounds use the latest result
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
            whiteBoxComponent->GenerateWhiteBoxMesh(CreateWhiteBoxRenderData(*baseMesh, m_material));
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
            booleanRenderData = CreateWhiteBoxRenderData(*displayMesh, m_material);
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
            // cache the white box render data
            m_renderData = CreateWhiteBoxRenderData(*EvaluatedMesh(), m_material);

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

                // generate the mesh
                (*m_renderMesh)->BuildMesh(m_renderData, m_worldFromLocal);
                OnMaterialChange();
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
        // clear the optional
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
    }
} // namespace WhiteBox
