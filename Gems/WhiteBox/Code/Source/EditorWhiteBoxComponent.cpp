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
#include <AzCore/std/sort.h>
#include <cmath>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
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
            "@gemroot:WhiteBox_test@/Editor/Scripts/default_shapes.py", scriptArgs);

        EditorWhiteBoxComponentNotificationBus::Event(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()),
            &EditorWhiteBoxComponentNotificationBus::Events::OnDefaultShapeTypeChanged, m_defaultShape);

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::u32 EditorWhiteBoxComponent::OnDrawShapeChange()
    {
        // Reset Draw Sides to a sensible default for the newly chosen shape:
        // angular shapes -> 4 (box / square base), round shapes -> 24 (smooth).
        switch (m_drawShape)
        {
        case DrawShapeType::Box:
        case DrawShapeType::Pyramid:
            m_drawSides = 4;
            break;
        case DrawShapeType::Cylinder:
        case DrawShapeType::Cone:
            m_drawSides = 24;
            break;
        case DrawShapeType::Sphere:
            m_drawSides = 16; // longitude segments; latitude rings derived from this
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

        if (!Api::MeshBoolean(*targetMesh, *sourceMesh, operandTransform, m_booleanOperation))
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

        // Build a watertight, vertex-shared surface for a set of filled unit cells:
        // for every cell, emit only the faces whose neighbour cell is empty.
        void GenerateSurface(WhiteBoxMesh& mesh, const AZStd::unordered_set<AZ::u64>& cells)
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
                    mesh, AZ::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
                verts.emplace(key, h);
                return h;
            };
            const auto filled = [&](int x, int y, int z) { return cells.count(PackCell(x, y, z)) != 0; };

            for (const AZ::u64 cellKey : cells)
            {
                int x, y, z;
                UnpackCell(cellKey, x, y, z);
                const auto corner = [&](int a, int b, int c) { return vert(x + a, y + b, z + c); };

                // each quad wound CCW as seen from outside (so its normal points out)
                if (!filled(x + 1, y, z)) // +X
                    Api::AddQuadPolygon(mesh, corner(1, 0, 0), corner(1, 1, 0), corner(1, 1, 1), corner(1, 0, 1));
                if (!filled(x - 1, y, z)) // -X
                    Api::AddQuadPolygon(mesh, corner(0, 1, 0), corner(0, 0, 0), corner(0, 0, 1), corner(0, 1, 1));
                if (!filled(x, y + 1, z)) // +Y
                    Api::AddQuadPolygon(mesh, corner(1, 1, 0), corner(0, 1, 0), corner(0, 1, 1), corner(1, 1, 1));
                if (!filled(x, y - 1, z)) // -Y
                    Api::AddQuadPolygon(mesh, corner(0, 0, 0), corner(1, 0, 0), corner(1, 0, 1), corner(0, 0, 1));
                if (!filled(x, y, z + 1)) // +Z
                    Api::AddQuadPolygon(mesh, corner(0, 0, 1), corner(1, 0, 1), corner(1, 1, 1), corner(0, 1, 1));
                if (!filled(x, y, z - 1)) // -Z
                    Api::AddQuadPolygon(mesh, corner(0, 1, 0), corner(1, 1, 0), corner(1, 0, 0), corner(0, 0, 0));
            }
        }

        // Quantize a vertex position to an integer lattice corner. Returns false if
        // the position is not (near) an integer point - such a vertex cannot belong
        // to the voxel surface, so its owning face is treated as freeform geometry.
        bool QuantizeCorner(const AZ::Vector3& p, int& x, int& y, int& z)
        {
            constexpr float eps = 1e-3f;
            const float rx = std::round(p.GetX());
            const float ry = std::round(p.GetY());
            const float rz = std::round(p.GetZ());
            if (std::abs(p.GetX() - rx) > eps || std::abs(p.GetY() - ry) > eps || std::abs(p.GetZ() - rz) > eps)
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

        bool FaceSignatureFromPositions(const AZStd::vector<AZ::Vector3>& positions, FaceSignature& outSig)
        {
            if (positions.size() != 3)
            {
                return false;
            }
            for (size_t i = 0; i < 3; ++i)
            {
                int x, y, z;
                if (!QuantizeCorner(positions[i], x, y, z))
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
        FaceSignatureSet SurfaceFaceSignatures(const AZStd::unordered_set<AZ::u64>& cells)
        {
            FaceSignatureSet sigs;
            if (cells.empty())
            {
                return sigs;
            }
            Api::WhiteBoxMeshPtr temp = Api::CreateWhiteBoxMesh();
            GenerateSurface(*temp, cells);
            for (const Api::FaceHandle& fh : Api::MeshFaceHandles(*temp))
            {
                FaceSignature sig;
                if (FaceSignatureFromPositions(Api::FaceVertexPositions(*temp, fh), sig))
                {
                    sigs.insert(sig);
                }
            }
            return sigs;
        }
    } // namespace VoxelDetail

    void EditorWhiteBoxComponent::RegenerateVoxelMesh(
        const AZStd::unordered_set<AZ::u64>& oldCells, const AZStd::unordered_set<AZ::u64>& newCells)
    {
        WhiteBoxMesh* mesh = GetWhiteBoxMesh();
        if (mesh == nullptr)
        {
            return;
        }

        // Remove ONLY the faces that belong to the previous voxel surface, leaving
        // every freeform face (and any voxel face the user has since hand-edited off
        // the integer lattice) untouched. Identifying the old faces by signature -
        // rather than clearing the whole mesh - is what preserves manual edits.
        const VoxelDetail::FaceSignatureSet oldSigs = VoxelDetail::SurfaceFaceSignatures(oldCells);
        if (!oldSigs.empty())
        {
            Api::FaceHandles toRemove;
            for (const Api::FaceHandle& fh : Api::MeshFaceHandles(*mesh))
            {
                VoxelDetail::FaceSignature sig;
                if (VoxelDetail::FaceSignatureFromPositions(Api::FaceVertexPositions(*mesh, fh), sig) &&
                    oldSigs.find(sig) != oldSigs.end())
                {
                    toRemove.push_back(fh);
                }
            }
            if (!toRemove.empty())
            {
                // Single batched removal - handles are invalidated by garbage_collect.
                Api::RemoveFaces(*mesh, toRemove);
            }
        }

        // Add the new voxel surface (a self-welded, watertight shell).
        VoxelDetail::GenerateSurface(*mesh, newCells);

        Api::CalculateNormals(*mesh);
        Api::CalculatePlanarUVs(*mesh);

        SerializeWhiteBox();
        RebuildWhiteBox();
    }

    void EditorWhiteBoxComponent::SetVoxelCell(const AZ::Vector3& cellMin, const bool filled)
    {
        const AZ::u64 key = VoxelDetail::PackCell(
            static_cast<int>(std::floor(cellMin.GetX())), static_cast<int>(std::floor(cellMin.GetY())),
            static_cast<int>(std::floor(cellMin.GetZ())));

        // rebuild the set from the serialized vector each time (keeps in sync with undo)
        const AZStd::unordered_set<AZ::u64> oldCells(m_voxelCells.begin(), m_voxelCells.end());
        AZStd::unordered_set<AZ::u64> newCells = oldCells;
        const bool changed = filled ? newCells.insert(key).second : (newCells.erase(key) > 0);
        if (!changed)
        {
            return; // already in the requested state
        }

        AzToolsFramework::ScopedUndoBatch undoBatch(filled ? "Stamp Voxel" : "Remove Voxel");
        m_voxelCells.assign(newCells.begin(), newCells.end());
        RegenerateVoxelMesh(oldCells, newCells);
        undoBatch.MarkEntityDirty(GetEntityId());
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

    void EditorWhiteBoxComponent::EvaluateLiveBoolean()
    {
        m_displayMesh.reset();

        if (!m_liveBoolean || !m_booleanSourceEntity.IsValid() || m_booleanSourceEntity == GetEntityId())
        {
            return;
        }

        WhiteBoxMesh* baseMesh = GetWhiteBoxMesh();
        if (baseMesh == nullptr)
        {
            return;
        }

        AZ::Entity* sourceEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, m_booleanSourceEntity);
        if (sourceEntity == nullptr)
        {
            return;
        }
        const auto sourceComponents = sourceEntity->FindComponents<EditorWhiteBoxComponent>();
        if (sourceComponents.empty())
        {
            return;
        }
        WhiteBoxMesh* sourceMesh = sourceComponents[0]->GetWhiteBoxMesh();
        if (sourceMesh == nullptr)
        {
            return;
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
            return;
        }
        if (Api::MeshBoolean(*evaluated, *sourceMesh, operandTransform, m_booleanOperation))
        {
            Api::CalculateNormals(*evaluated);
            Api::CalculatePlanarUVs(*evaluated);
            m_displayMesh = AZStd::move(evaluated);
        }
        // on failure (no overlap) m_displayMesh stays null -> falls back to the base.
    }

    void EditorWhiteBoxComponent::UpdateBooleanSourceListener()
    {
        m_booleanSourceListener.BusDisconnect();
        if (m_liveBoolean && m_booleanSourceEntity.IsValid() && m_booleanSourceEntity != GetEntityId())
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

    void EditorWhiteBoxComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorWhiteBoxMeshAsset::Reflect(context);

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
                ->Field("DrawSides", &EditorWhiteBoxComponent::m_drawSides)
                ->Field("DrawShape", &EditorWhiteBoxComponent::m_drawShape)
                ->Field("DrawStairSteps", &EditorWhiteBoxComponent::m_drawStairSteps)
                ->Field("DrawStairByHeight", &EditorWhiteBoxComponent::m_drawStairByHeight)
                ->Field("DrawStepHeight", &EditorWhiteBoxComponent::m_drawStepHeight)
                ->Field("DrawStairRotation", &EditorWhiteBoxComponent::m_drawStairRotation)
                ->Field("DrawUnitCube", &EditorWhiteBoxComponent::m_drawUnitCube)
                ->Field("VoxelCells", &EditorWhiteBoxComponent::m_voxelCells)
                ->Field("BooleanSource", &EditorWhiteBoxComponent::m_booleanSourceEntity)
                ->Field("BooleanOp", &EditorWhiteBoxComponent::m_booleanOperation)
                ->Field("BooleanHideSource", &EditorWhiteBoxComponent::m_hideSourceAfterApply)
                ->Field("BooleanDeleteSource", &EditorWhiteBoxComponent::m_deleteSourceAfterApply)
                ->Field("BooleanLive", &EditorWhiteBoxComponent::m_liveBoolean);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorWhiteBoxComponent>("White Box", "White Box level editing")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/WhiteBox.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/WhiteBox.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/white-box/")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_defaultShape, "Default Shape",
                        "Default shape of the white box mesh.")
                    ->EnumAttribute(DefaultShapeType::Cube, "Cube")
                    ->EnumAttribute(DefaultShapeType::Tetrahedron, "Tetrahedron")
                    ->EnumAttribute(DefaultShapeType::Icosahedron, "Icosahedron")
                    ->EnumAttribute(DefaultShapeType::Cylinder, "Cylinder")
                    ->EnumAttribute(DefaultShapeType::Sphere, "Sphere")
                    ->EnumAttribute(DefaultShapeType::Asset, "Mesh Asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnDefaultShapeChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_drawShape, "Draw Shape",
                        "Shape the Draw Shape tool builds. Changing this resets Draw Sides to a sensible default.")
                    ->EnumAttribute(DrawShapeType::Box, "Box")
                    ->EnumAttribute(DrawShapeType::Cylinder, "Cylinder")
                    ->EnumAttribute(DrawShapeType::Pyramid, "Pyramid")
                    ->EnumAttribute(DrawShapeType::Cone, "Cone")
                    ->EnumAttribute(DrawShapeType::Sphere, "Sphere")
                    ->EnumAttribute(DrawShapeType::Staircase, "Staircase")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnDrawShapeChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &EditorWhiteBoxComponent::m_drawSides, "Draw Sides",
                        "Number of sides for round / N-gon shapes (4 = box / square), or the subdivision of the Sphere.")
                    ->Attribute(AZ::Edit::Attributes::Min, 3)
                    ->Attribute(AZ::Edit::Attributes::Max, 128)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::DrawSidesVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_drawStairByHeight, "Stair By Step Height",
                        "When on, the Staircase is divided by a fixed step (riser) height; the step count is derived "
                        "from the pull height. When off, a fixed step count is used.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::StairVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &EditorWhiteBoxComponent::m_drawStairSteps, "Step Count",
                        "Number of steps the Draw Shape tool builds when the shape is a Staircase.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 128)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::DrawStepsVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_drawStepHeight, "Step Height",
                        "Riser height of each step; the step count is derived from the pull height.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::StepHeightVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &EditorWhiteBoxComponent::m_drawStairRotation, "Stair Rotation (x90)",
                        "Orientation of the Staircase in 90-degree steps about the drawn surface. 2 (180 degrees) puts "
                        "the tall end at the corner you first clicked.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 3)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::StairVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_drawUnitCube, "Unit Cube Stamp",
                        "In draw mode, click to stamp a grid-snapped 1x1x1 cube (CSG union; hold Ctrl to subtract) "
                        "instead of click-drag-pull.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_editorMeshAsset, "Editor Mesh Asset",
                        "Editor Mesh Asset")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::AssetVisibility)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "Save as asset", "Save as asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::SaveAsAsset)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Save As ...")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_material, "White Box Material",
                        "The properties of the White Box material.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnMaterialChange)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_componentModeDelegate,
                        "Component Mode", "White Box Tool Component Mode")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Export to obj")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ExportToFile)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Export")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Export all whiteboxes on descendant entities as a single obj (excluding this one)")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ExportDescendantsToFile)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Export Descendants")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorWhiteBoxComponent::m_flipYZForExport,
                        "Flip Y and Z for Export",
                        "Flip the Y and Z axes when exportings so they aren't imported sideways into coord systems where the Y-axis goes up.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_booleanSourceEntity, "Boolean Source",
                        "Another entity with a White Box component to use as the boolean operand.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLiveBooleanChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_booleanOperation, "Boolean Operation",
                        "How to combine the source mesh with this one.")
                    ->EnumAttribute(Api::BooleanOperation::Subtraction, "Subtract")
                    ->EnumAttribute(Api::BooleanOperation::Union, "Union")
                    ->EnumAttribute(Api::BooleanOperation::Intersection, "Intersect")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLiveBooleanChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_liveBoolean,
                        "Non-Destructive (Live)",
                        "Keep this mesh editable and show the boolean result live (re-evaluates when the source "
                        "moves or either mesh changes). Leave off to use the one-shot Apply Boolean below.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnLiveBooleanChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_hideSourceAfterApply,
                        "Hide Source After Apply", "Hide the source entity once the boolean is applied.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_deleteSourceAfterApply,
                        "Delete Source After Apply", "Delete the source entity once the boolean is applied.")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Apply the boolean using the source entity's mesh")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ApplyBoolean)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Apply Boolean");
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

    AZ::Crc32 EditorWhiteBoxComponent::DrawSidesVisibility() const
    {
        // Sides drives the resolution of round shapes (Cylinder/Cone) and the
        // subdivision of the Sphere; it is meaningless for Box/Pyramid/Staircase.
        const bool usesSides = m_drawShape == DrawShapeType::Cylinder || m_drawShape == DrawShapeType::Cone ||
            m_drawShape == DrawShapeType::Sphere;
        return usesSides ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 EditorWhiteBoxComponent::StairVisibility() const
    {
        return m_drawShape == DrawShapeType::Staircase ? AZ::Edit::PropertyVisibility::Show
                                                       : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 EditorWhiteBoxComponent::DrawStepsVisibility() const
    {
        // Step count only applies to a Staircase in step-count mode.
        const bool show = m_drawShape == DrawShapeType::Staircase && !m_drawStairByHeight;
        return show ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 EditorWhiteBoxComponent::StepHeightVisibility() const
    {
        // Step height only applies to a Staircase in step-height mode.
        const bool show = m_drawShape == DrawShapeType::Staircase && m_drawStairByHeight;
        return show ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
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
        if (auto* whiteBoxComponent = gameEntity->CreateComponent<WhiteBoxComponent>())
        {
            // note: it is important no edit time only functions are called here as BuildGameEntity
            // will be called by the Asset Processor when creating dynamic slices
            whiteBoxComponent->GenerateWhiteBoxMesh(m_renderData);
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

        if (m_liveBoolean)
        {
            RebuildWhiteBox(); // moving this entity changes the cut relative to the source
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
