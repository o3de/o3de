/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * EditorWhiteBoxComponent - RENDERING / BOUNDS / SELECTION translation unit: render mesh
 * rebuilds, per-layer coloured render data, material handling, physics mesh, bounds,
 * selection ray-casts, visible geometry and debug display.
 */

#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "Rendering/WhiteBoxNullRenderMesh.h"
#include "Rendering/WhiteBoxRenderDataUtil.h"
#include "Rendering/WhiteBoxRenderMeshInterface.h"

#include "Util/WhiteBoxMeshUtil.h"
#include "Util/WhiteBoxSnapUtil.h"

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/numeric.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <cmath>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
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
    namespace
    {
    // Maps each face of a merged CSG result back to the source layer whose surface it lies on (a coplanar
    // source triangle containing its centroid, else the nearest), so booleans keep per-layer tint.
    class LayerColorSource
    {
    public:
        void AddMesh(const WhiteBoxMesh& mesh, const int layerIndex)
        {
            const Api::Faces faces = Api::MeshFaces(mesh); // each Api::Face is a triangle
            for (const Api::Face& f : faces)
            {
                const AZ::Vector3 cross = (f[1] - f[0]).Cross(f[2] - f[0]);
                if (cross.GetLengthSq() < 1e-12f)
                {
                    continue; // degenerate triangle - contributes no surface
                }
                Tri tri;
                tri.m_a = f[0];
                tri.m_b = f[1];
                tri.m_c = f[2];
                tri.m_normal = cross.GetNormalized();
                tri.m_layer = layerIndex;
                m_planes[PlaneKey(tri.m_normal, f[0])].push_back(tri);
            }
        }

        bool Empty() const
        {
            return m_planes.empty();
        }

        int Match(const AZ::Vector3& centroid, const AZ::Vector3& normal, const int fallbackLayer) const
        {
            // Where coplanar layers overlap, the highest layer index wins (the one drawn on top or that cut).
            int bestContainLayer = -1;
            int nearestLayer = fallbackLayer;
            float nearestDistSq = 1e30f;

            const auto scanBucket = [&](const AZStd::vector<Tri>& tris, const bool requireContain)
            {
                for (const Tri& tri : tris)
                {
                    if (requireContain && tri.m_layer > bestContainLayer && PointInTriangle(centroid, tri))
                    {
                        bestContainLayer = tri.m_layer;
                    }
                    const AZ::Vector3 c = (tri.m_a + tri.m_b + tri.m_c) / 3.0f;
                    const float d = (c - centroid).GetLengthSq();
                    if (d < nearestDistSq)
                    {
                        nearestDistSq = d;
                        nearestLayer = tri.m_layer;
                    }
                }
            };

            const auto it = m_planes.find(PlaneKey(normal, centroid));
            if (it != m_planes.end())
            {
                scanBucket(it->second, true);
                return bestContainLayer >= 0 ? bestContainLayer : nearestLayer; // contained, else nearest on-plane
            }

            // No coplanar bucket (e.g. a stray reoriented face): fall back to the globally nearest source.
            for (const auto& kv : m_planes)
            {
                scanBucket(kv.second, false);
            }
            return nearestLayer;
        }

    private:
        struct Tri
        {
            AZ::Vector3 m_a;
            AZ::Vector3 m_b;
            AZ::Vector3 m_c;
            AZ::Vector3 m_normal;
            int m_layer;
        };

        static constexpr float PlaneQuant = 1e-3f;

        // Canonical (sign-independent) plane key so a face and its Subtract-flipped counterpart bucket
        // together: flip the normal to make its dominant component positive, then quantize (n, d).
        static AZ::u64 PlaneKey(const AZ::Vector3& normal, const AZ::Vector3& pointOnPlane)
        {
            AZ::Vector3 n = normal;
            const float ax = std::abs(n.GetX());
            const float ay = std::abs(n.GetY());
            const float az = std::abs(n.GetZ());
            const float dom = (ax >= ay && ax >= az) ? n.GetX() : (ay >= az ? n.GetY() : n.GetZ());
            if (dom < 0.0f)
            {
                n = -n;
            }
            const float d = n.Dot(pointOnPlane);
            const auto q = [](const float v)
            {
                return static_cast<AZ::s64>(std::llround(v / PlaneQuant));
            };
            AZ::u64 h = 1469598103934665603ull; // FNV-1a style fold of the four quantized values
            for (const AZ::s64 value : { q(n.GetX()), q(n.GetY()), q(n.GetZ()), q(d) })
            {
                h = (h ^ static_cast<AZ::u64>(value)) * 1099511628211ull;
            }
            return h;
        }

        static bool PointInTriangle(const AZ::Vector3& p, const Tri& tri)
        {
            // Project to 2D by dropping the axis most aligned with the normal, then a tolerant
            // same-side test (the centroid of a sub-face lies strictly inside its source triangle).
            const float ax = std::abs(tri.m_normal.GetX());
            const float ay = std::abs(tri.m_normal.GetY());
            const float az = std::abs(tri.m_normal.GetZ());
            const auto to2 = [ax, ay, az](const AZ::Vector3& v, float& x, float& y)
            {
                if (ax >= ay && ax >= az)
                {
                    x = v.GetY();
                    y = v.GetZ();
                }
                else if (ay >= az)
                {
                    x = v.GetX();
                    y = v.GetZ();
                }
                else
                {
                    x = v.GetX();
                    y = v.GetY();
                }
            };
            float axx, axy, bxx, bxy, cxx, cxy, pxx, pxy;
            to2(tri.m_a, axx, axy);
            to2(tri.m_b, bxx, bxy);
            to2(tri.m_c, cxx, cxy);
            to2(p, pxx, pxy);
            const auto cross2 = [](float ox, float oy, float ux, float uy, float vx, float vy)
            {
                return (ux - ox) * (vy - oy) - (uy - oy) * (vx - ox);
            };
            const float d1 = cross2(axx, axy, bxx, bxy, pxx, pxy);
            const float d2 = cross2(bxx, bxy, cxx, cxy, pxx, pxy);
            const float d3 = cross2(cxx, cxy, axx, axy, pxx, pxy);
            constexpr float Eps = 1e-4f;
            const bool hasNeg = (d1 < -Eps) || (d2 < -Eps) || (d3 < -Eps);
            const bool hasPos = (d1 > Eps) || (d2 > Eps) || (d3 > Eps);
            return !(hasNeg && hasPos);
        }

        AZStd::unordered_map<AZ::u64, AZStd::vector<Tri>> m_planes;
    };

    bool IsWhiteBoxNullRenderMesh(const AZStd::optional<AZStd::unique_ptr<RenderMeshInterface>>& renderMesh)
    {
        return azrtti_cast<WhiteBoxNullRenderMesh*>((*renderMesh).get()) != nullptr;
    }

    template<typename TransformFn>
    AZ::Aabb CalculateAabb(const WhiteBoxMesh& whiteBox, TransformFn&& transformFn)
    {
        const auto vertexHandles = Api::MeshVertexHandles(whiteBox);
        return AZStd::accumulate(
            AZStd::cbegin(vertexHandles), AZStd::cend(vertexHandles), AZ::Aabb::CreateNull(), transformFn);
    }

    bool DebugDrawingEnabled()
    {
        return cl_whiteBoxDebugVertexHandles || cl_whiteBoxDebugNormals || cl_whiteBoxDebugHalfedgeHandles ||
            cl_whiteBoxDebugEdgeHandles || cl_whiteBoxDebugFaceHandles || cl_whiteBoxDebugAabb;
    }

    void WhiteBoxDebugRendering(
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
    } // namespace

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

    void EditorWhiteBoxComponent::OnDefaultMaterialChange()
    {
        SetMaterialOverride(m_defaultMaterialAsset.GetId());
    }

    void EditorWhiteBoxComponent::SetMaterialOverride(const AZ::Data::AssetId& materialAssetId)
    {
        if (m_materialOverrideAssetId == materialAssetId)
        {
            return;
        }
        m_materialOverrideAssetId = materialAssetId;
        m_material.m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(
            materialAssetId, azrtti_typeid<AZ::RPI::MaterialAsset>());
        m_material.m_materialAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        m_defaultMaterialAsset = m_material.m_materialAsset; // the card follows changes made elsewhere
        // The material asset is baked into the model when it is created, so force the render mesh to
        // be recreated (drop to a null render mesh first) with the new material.
        if (m_renderMesh.has_value())
        {
            m_renderMesh.emplace(AZStd::make_unique<WhiteBoxNullRenderMesh>(AZ::EntityId{}));
        }
        RebuildWhiteBox(); // hidden entities also need fresh runtime bake data
    }

    void EditorWhiteBoxComponent::AssignPolygonMaterial(
        const Api::PolygonHandles& polygons, const AZ::Data::AssetId& materialAssetId)
    {
        WhiteBoxMesh* mesh = GetWhiteBoxMesh();
        if (!mesh || polygons.empty())
        {
            return;
        }
        const Api::PolygonHandles validPolygons = Api::MeshPolygonHandles(*mesh);
        bool changed = false;
        for (const Api::PolygonHandle& polygon : polygons)
        {
            if (AZStd::find(validPolygons.begin(), validPolygons.end(), polygon) == validPolygons.end())
            {
                continue; // stale selection after an undo or layer change
            }
            for (const Api::FaceHandle face : polygon.m_faceHandles)
            {
                if (Api::FaceMaterial(*mesh, face) != materialAssetId)
                {
                    Api::SetFaceMaterial(*mesh, face, materialAssetId);
                    changed = true;
                }
            }
        }
        if (changed)
        {
            // Explicit polygon edits freeze parametric geometry, just like vertex edits.
            BakeParametricLayer(m_activeLayerIndex);
            SerializeWhiteBox();
            OnWhiteBoxMeshModified();
        }
    }

    AZ::Data::AssetId EditorWhiteBoxComponent::GetMaterialOverride()
    {
        return m_materialOverrideAssetId;
    }

    AZ::u32 EditorWhiteBoxComponent::OnGlobalTintChange()
    {
        RebuildWhiteBox();                                 // recolour (booleans run in both tint modes;
                                                           // the coloured path re-tints merged faces)
        return AZ::Edit::PropertyRefreshLevels::EntireTree; // show / hide the global tint element
    }

    AZ::Crc32 EditorWhiteBoxComponent::GlobalTintVisibility() const
    {
        return m_useGlobalTint ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    bool EditorWhiteBoxComponent::PerLayerRenderActive() const
    {
        // Per-layer faces for per-layer tint and Edges Only layers (Invert Normals lives in the mesh now).
        if (!m_useGlobalTint)
        {
            return true;
        }
        for (const WhiteBoxLayer& layer : m_layers)
        {
            if (layer.m_visible && layer.m_edgesOnly)
            {
                return true;
            }
        }
        return false;
    }

    bool EditorWhiteBoxComponent::AnyVisibleLayerBoolean() const
    {
        bool seenVisible = false;
        for (const WhiteBoxLayer& layer : m_layers)
        {
            if (!layer.m_visible)
            {
                continue;
            }
            if (seenVisible &&
                (layer.m_combineMode == LayerCombineMode::Union || layer.m_combineMode == LayerCombineMode::Subtract ||
                 layer.m_combineMode == LayerCombineMode::Intersect))
            {
                return true; // the first visible layer is the base; its own mode is ignored
            }
            seenVisible = true;
        }
        return false;
    }

    WhiteBoxRenderData EditorWhiteBoxComponent::BuildColoredRenderData(WhiteBoxMesh* freeformOverride)
    {
        // When a boolean merges layers, geometry is combined (not separate islands), so the coloured
        // faces must be re-derived from the merged mesh and matched back to their source layer.
        if (AnyVisibleLayerBoolean())
        {
            return BuildColoredBooleanRenderData(freeformOverride);
        }

        WhiteBoxRenderData renderData;
        renderData.m_material = m_material;
        renderData.m_material.m_useVertexColor = !m_useGlobalTint;

        WhiteBoxMesh* freeform =
            freeformOverride ? freeformOverride : ((m_boolean.m_live && m_displayMesh) ? m_displayMesh.get() : GetWhiteBoxMesh());
        const int count = static_cast<int>(m_layers.size());
        const int activeIdx = m_layerRuntime.m_loadedIndex;
        const bool activeIdentity = activeIdx >= 0 && activeIdx < count &&
            m_layers[activeIdx].m_position.IsZero() && m_layers[activeIdx].m_rotation.IsZero() &&
            m_layers[activeIdx].m_scale.IsClose(AZ::Vector3::CreateOne(), 1e-6f);

        // Each visible layer contributes its own faces tinted with its colour. (In per-layer-tint
        // mode BuildCombined keeps the layers as separate islands, so this matches the geometry.)
        for (int i = 0; i < count; ++i)
        {
            if (!m_layers[i].m_visible || m_layers[i].m_edgesOnly)
            {
                continue; // Edges Only layers contribute no solid faces (edges drawn in DisplayEntityViewport)
            }
            // Borrow unchanged layer meshes. Only transformed/grid-combined active
            // geometry needs an owned copy; render-data generation is read-only.
            Api::WhiteBoxMeshPtr ownedMesh;
            WhiteBoxMesh* mesh = nullptr;
            if (i == activeIdx)
            {
                ownedMesh = CombinedWithGrid(freeform);
                if (!ownedMesh && freeform != nullptr &&
                    (!activeIdentity || m_layers[i].m_invertNormals || HasLayerModifiers(m_layers[i])))
                {
                    ownedMesh = Api::CloneMesh(*freeform);
                }
                ownedMesh = ApplyLayerModifiers(AZStd::move(ownedMesh), m_layers[i]);
                if (ownedMesh && !activeIdentity)
                {
                    ApplyTransformToMesh(
                        *ownedMesh, m_layers[i].m_position, m_layers[i].m_rotation, m_layers[i].m_scale);
                }
                if (ownedMesh && m_layers[i].m_invertNormals)
                {
                    ownedMesh = FlippedMeshWinding(*ownedMesh); // matches BuildCombined/physics
                }
                mesh = ownedMesh ? ownedMesh.get() : freeform;
            }
            else
            {
                const auto id = m_layers[i].m_id;
                auto cached = m_layerRuntime.m_meshCache.find(id);
                if (cached == m_layerRuntime.m_meshCache.end() || !cached->second)
                {
                    cached = m_layerRuntime.m_meshCache.insert_or_assign(id, BuildLayerMesh(m_layers[i])).first;
                }
                mesh = cached->second.get(); // already transformed + winding-flipped
            }
            if (!mesh)
            {
                continue;
            }
            const AZ::Vector3& t = m_layers[i].m_tint;
            const AZ::Vector4 color = m_useGlobalTint
                ? AZ::Vector4::CreateOne()
                : AZ::Vector4(t.GetX(), t.GetY(), t.GetZ(), 1.0f);
            WhiteBoxRenderData layerData = CreateWhiteBoxRenderData(*mesh, m_material);
            for (WhiteBoxFace& face : layerData.m_faces)
            {
                face.m_color = color;
            }
            renderData.m_faces.insert(
                renderData.m_faces.end(), layerData.m_faces.begin(), layerData.m_faces.end());
        }
        return renderData;
    }

    WhiteBoxRenderData EditorWhiteBoxComponent::BuildColoredBooleanRenderData(WhiteBoxMesh* freeformOverride)
    {
        WhiteBoxRenderData renderData;
        renderData.m_material = m_material;
        renderData.m_material.m_useVertexColor = !m_useGlobalTint;

        WhiteBoxMesh* freeform =
            freeformOverride ? freeformOverride : ((m_boolean.m_live && m_displayMesh) ? m_displayMesh.get() : GetWhiteBoxMesh());
        const int count = static_cast<int>(m_layers.size());
        const int activeIdx = m_layerRuntime.m_loadedIndex;
        const bool activeIdentity = activeIdx >= 0 && activeIdx < count &&
            m_layers[activeIdx].m_position.IsZero() && m_layers[activeIdx].m_rotation.IsZero() &&
            m_layers[activeIdx].m_scale.IsClose(AZ::Vector3::CreateOne(), 1e-6f);

        // Build one (transformed + winding-flipped) mesh per visible layer.
        const auto buildLayerMesh = [&](const int i) -> Api::WhiteBoxMeshPtr
        {
            if (i == activeIdx)
            {
                Api::WhiteBoxMeshPtr mesh = CombinedWithGrid(freeform);
                if (!mesh && freeform != nullptr)
                {
                    mesh = Api::CloneMesh(*freeform);
                }
                mesh = ApplyLayerModifiers(AZStd::move(mesh), m_layers[i]);
                if (mesh && !activeIdentity)
                {
                    ApplyTransformToMesh(
                        *mesh, m_layers[i].m_position, m_layers[i].m_rotation, m_layers[i].m_scale);
                }
                if (mesh && m_layers[i].m_invertNormals)
                {
                    mesh = FlippedMeshWinding(*mesh); // mesh-level flip (matches BuildCombined/physics)
                }
                return mesh;
            }
            return BuildLayerMesh(m_layers[i]);
        };

        // Accumulate the visible layers per their combine mode (matching BuildCombined), recording
        // every contributing layer's surface so each merged face can be re-coloured by source layer.
        LayerColorSource sources;
        Api::WhiteBoxMeshPtr acc;
        int baseLayer = -1;
        for (int i = 0; i < count; ++i)
        {
            if (!m_layers[i].m_visible)
            {
                continue;
            }
            Api::WhiteBoxMeshPtr mesh = buildLayerMesh(i);
            if (!mesh)
            {
                continue;
            }
            sources.AddMesh(*mesh, i);
            if (!acc)
            {
                acc = AZStd::move(mesh); // first visible layer is the base (its own mode is ignored)
                baseLayer = i;
                continue;
            }
            const AZ::Transform identity = AZ::Transform::CreateIdentity();
            switch (m_layers[i].m_combineMode)
            {
            case LayerCombineMode::Union:
                if (Api::MeshFaceHandles(*acc).empty() ||
                    !Api::ApplyMeshBoolean(*acc, *mesh, identity, Api::BooleanOperation::Union, m_csgSolver))
                {
                    AppendMesh(*acc, *mesh);
                }
                break;
            case LayerCombineMode::Subtract:
                Api::ApplyMeshBoolean(*acc, *mesh, identity, Api::BooleanOperation::Subtraction, m_csgSolver);
                break;
            case LayerCombineMode::Intersect:
                Api::ApplyMeshBoolean(*acc, *mesh, identity, Api::BooleanOperation::Intersection, m_csgSolver);
                break;
            case LayerCombineMode::Separate:
            default:
                AppendMesh(*acc, *mesh);
                break;
            }
        }

        if (!acc || baseLayer < 0)
        {
            return renderData; // nothing visible
        }
        Api::CalculateNormals(*acc);
        Api::CalculatePlanarUVs(*acc);

        // Colour every face by the layer its surface came from; new Subtract walls resolve to the
        // cutting layer. Winding is flipped per that layer's Invert Normals flag.
        const auto faceHandles = Api::MeshFaceHandles(*acc);
        renderData.m_faces.reserve(faceHandles.size());
        for (const Api::FaceHandle& faceHandle : faceHandles)
        {
            const AZStd::vector<AZ::Vector3> verts = Api::FaceVertexPositions(*acc, faceHandle);
            if (verts.size() < 3)
            {
                continue;
            }
            const AZ::Vector3 centroid = (verts[0] + verts[1] + verts[2]) / 3.0f;
            const AZ::Vector3 normal = Api::FaceNormal(*acc, faceHandle);
            const int layerIdx = sources.Match(centroid, normal, baseLayer);

            const WhiteBoxLayer& layer = m_layers[layerIdx];
            if (layer.m_edgesOnly)
            {
                continue; // Edges Only layers contribute no solid faces
            }
            const AZ::Vector3& t = layer.m_tint;
            const AZ::Vector4 color = m_useGlobalTint
                ? AZ::Vector4::CreateOne()
                : AZ::Vector4(t.GetX(), t.GetY(), t.GetZ(), 1.0f);

            // No per-face winding flip: inverted layers' meshes are flipped at mesh level.
            WhiteBoxFace face = BuildWhiteBoxFace(*acc, faceHandle, false);
            face.m_color = color;
            renderData.m_faces.push_back(face);
        }
        return renderData;
    }

    void EditorWhiteBoxComponent::BuildLayerRenderMeshes()
    {
        m_layerRenderMeshes.clear();

        WhiteBoxMesh* freeform = (m_boolean.m_live && m_displayMesh) ? m_displayMesh.get() : GetWhiteBoxMesh();
        const int count = static_cast<int>(m_layers.size());
        const int activeIdx = m_layerRuntime.m_loadedIndex;
        const bool activeIdentity = activeIdx >= 0 && activeIdx < count &&
            m_layers[activeIdx].m_position.IsZero() && m_layers[activeIdx].m_rotation.IsZero() &&
            m_layers[activeIdx].m_scale.IsClose(AZ::Vector3::CreateOne(), 1e-6f);

        for (int i = 0; i < count; ++i)
        {
            if (!m_layers[i].m_visible || m_layers[i].m_edgesOnly)
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
                mesh = ApplyLayerModifiers(AZStd::move(mesh), m_layers[i]);
                if (mesh && !activeIdentity)
                {
                    ApplyTransformToMesh(
                        *mesh, m_layers[i].m_position, m_layers[i].m_rotation, m_layers[i].m_scale);
                }
                if (mesh && m_layers[i].m_invertNormals)
                {
                    mesh = FlippedMeshWinding(*mesh);
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

    void EditorWhiteBoxComponent::RebuildRenderMesh()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // reset caches when the mesh changes
        m_worldAabb.reset();
        m_localAabb.reset();
        m_faces.reset();
        InvalidateSnapVertexCache();

        // Tell the render geometry intersector the triangles moved, or it will keep serving
        // raycasts from its cached copy of the old shape.
        AzFramework::RenderGeometry::IntersectionNotificationBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzFramework::RenderGeometry::IntersectionNotifications::OnGeometryChanged, GetEntityId());

        AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(GetEntityId());

        // must have been created in Activate or have had the Entity made visible again
        if (m_renderMesh.has_value())
        {
            m_layerRenderMeshes.clear(); // single mesh (with per-vertex colours) serves both modes

            // Per-layer build for per-layer tint or inverted layers; otherwise the CSG-combined mesh.
            const bool perLayerRender = PerLayerRenderActive();

            // Global tint -> plain (white-vertex) faces coloured by the material baseColor. Per-layer
            // tint -> per-face vertex colours (needs the material's "Use Vertex Color" enabled).
            m_renderData = perLayerRender ? BuildColoredRenderData()
                                          : CreateWhiteBoxRenderData(*EvaluatedMesh(), m_material);

            // The data may not be ready yet (e.g. an asset still loading), so skip empty render data.
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
                // The geometry became empty: swap to a null render mesh (the RHI rejects empty buffers).
                m_renderMesh.emplace(AZStd::make_unique<WhiteBoxNullRenderMesh>(AZ::EntityId{}));
            }
        }

        RebuildWireframeMesh();

        EditorWhiteBoxComponentModeRequestBus::Event(
            AZ::EntityComponentIdPair{GetEntityId(), GetId()},
            &EditorWhiteBoxComponentModeRequests::MarkWhiteBoxIntersectionDataDirty);

        // Mid-drag there is nothing to serialize into, and the committing rebuild that ends the drag
        // re-packs anyway, so a streaming edit skips it.
        if (!m_rebuild.m_streaming)
        {
            PackRenderDataBlob();
        }
    }

    void EditorWhiteBoxComponent::PackRenderDataBlob()
    {
        m_renderDataBlob = PackWhiteBoxRenderData({ &m_renderData });
    }

    void EditorWhiteBoxComponent::PackBakedDataBlob()
    {
        m_bakedDataBlob = PackWhiteBoxRenderData({ &m_bakedBaseRenderData, &m_bakedPhysicsBaseRenderData,
                                                   &m_bakedBooleanRenderData, &m_bakedPhysicsBooleanRenderData });
    }

    void EditorWhiteBoxComponent::EnsureRenderDataUnpacked()
    {
        // On a never-activated clone (BuildGameEntity) the render data is only in the blobs: unpack it.
        const bool haveFaces = !m_renderData.m_faces.empty() || !m_bakedBaseRenderData.m_faces.empty() ||
            !m_bakedPhysicsBaseRenderData.m_faces.empty() || !m_bakedBooleanRenderData.m_faces.empty() ||
            !m_bakedPhysicsBooleanRenderData.m_faces.empty();
        if (!haveFaces && (!m_renderDataBlob.empty() || !m_bakedDataBlob.empty()))
        {
            UnpackRenderDataBlobs();
        }
    }

    void EditorWhiteBoxComponent::UnpackRenderDataBlobs()
    {
        // Order must match PackRenderDataBlob / PackBakedDataBlob.
        UnpackWhiteBoxRenderData(m_renderDataBlob, { &m_renderData });
        UnpackWhiteBoxRenderData(
            m_bakedDataBlob, { &m_bakedBaseRenderData, &m_bakedPhysicsBaseRenderData, &m_bakedBooleanRenderData,
                               &m_bakedPhysicsBooleanRenderData });
    }

    void EditorWhiteBoxComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_worldAabb.reset();
        m_localAabb.reset();
        InvalidateSnapVertexCache(); // cached snap vertices are in world space
        m_worldFromLocal = world;

        AzFramework::RenderGeometry::IntersectionNotificationBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzFramework::RenderGeometry::IntersectionNotifications::OnGeometryChanged, GetEntityId());

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
        if (m_wireframeRenderMesh)
        {
            m_wireframeRenderMesh->UpdateTransform(world);
        }

        // Defer the heavy rebuild to OnTick instead of running it synchronously
        if (m_boolean.m_sourceEntity.IsValid() && m_boolean.m_sourceEntity != GetEntityId())
        {
            m_rebuild.m_liveBooleanPending = true;
        }
    }

    void EditorWhiteBoxComponent::RebuildPhysicsMesh()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        EditorWhiteBoxColliderRequestBus::Event(
            GetEntityId(), &EditorWhiteBoxColliderRequests::CreatePhysics, *GetPhysicsMesh()); // <-- CHANGED HERE
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
            AZ::Aabb bounds = GetLocalBounds();
            // Fold in the entity's non-uniform scale (Non-Uniform Scale component) before applying the
            // world transform, which only carries uniform scale. No-op when there is no such component.
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(
                nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
            bounds.MultiplyByScale(nonUniformScale);
            bounds.ApplyTransform(m_worldFromLocal);
            m_worldAabb = bounds;
        }

        return m_worldAabb.value();
    }

    AZ::Aabb EditorWhiteBoxComponent::GetLocalBounds() const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_localAabb.has_value())
        {
            // EvaluatedMesh() can be null while entities activate; an invalid Aabb culls it until ready.
            WhiteBoxMesh* evaluatedMesh = const_cast<EditorWhiteBoxComponent*>(this)->EvaluatedMesh();
            if (evaluatedMesh == nullptr)
            {
                return AZ::Aabb::CreateNull();
            }
            auto& whiteBoxMesh = *evaluatedMesh;

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

    AzFramework::RenderGeometry::RayResult EditorWhiteBoxComponent::RenderGeometryIntersect(
        const AzFramework::RenderGeometry::RayRequest& ray) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AzFramework::RenderGeometry::RayResult result;

        if (!m_faces.has_value())
        {
            // EvaluatedMesh is non-const (it can build the combined mesh on demand), but this
            // whole path is a cache fill - m_faces is mutable for exactly this reason.
            WhiteBoxMesh* evaluatedMesh = const_cast<EditorWhiteBoxComponent*>(this)->EvaluatedMesh();
            if (evaluatedMesh == nullptr)
            {
                return result;
            }

            m_faces = Api::MeshFaces(*evaluatedMesh);
        }

        if (m_faces->empty())
        {
            return result;
        }

        // Include Non-Uniform Scale, as the rendered geometry and snap vertices do.
        const AZ::Vector3 nonUniformScale = EntityNonUniformScale();
        const AZ::Transform localFromWorld = m_worldFromLocal.GetInverse();

        const AZ::Vector3 localRayStart = localFromWorld.TransformPoint(ray.m_startWorldPosition) / nonUniformScale;
        const AZ::Vector3 localRayEnd = localFromWorld.TransformPoint(ray.m_endWorldPosition) / nonUniformScale;

        AZ::Intersect::SegmentTriangleHitTester hitTester(localRayStart, localRayEnd);

        float closestNormalisedDistance = AZStd::numeric_limits<float>::max();
        AZ::Vector3 closestLocalNormal = AZ::Vector3::CreateAxisZ();
        bool hit = false;

        for (const auto& face : m_faces.value())
        {
            float normalisedDistance = 0.0f;
            AZ::Vector3 localNormal;
            if (hitTester.IntersectSegmentTriangle(face[0], face[1], face[2], localNormal, normalisedDistance))
            {
                if (normalisedDistance < closestNormalisedDistance)
                {
                    closestNormalisedDistance = normalisedDistance;
                    closestLocalNormal = localNormal;
                    hit = true;
                }
            }
        }

        if (!hit)
        {
            return result;
        }

        // IntersectSegmentTriangle reports t along the segment, so scale back into world units.
        const AZ::Vector3 worldRay = ray.m_endWorldPosition - ray.m_startWorldPosition;
        result.m_worldPosition = ray.m_startWorldPosition + worldRay * closestNormalisedDistance;
        result.m_worldNormal =
            AzToolsFramework::TransformDirectionNoScaling(m_worldFromLocal, closestLocalNormal).GetNormalizedSafe();
        result.m_distance = worldRay.GetLength() * closestNormalisedDistance;
        result.m_entityAndComponent = AZ::EntityComponentIdPair(GetEntityId(), GetId());

        return result;
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
        m_wireframeRenderMesh.reset();
        m_renderMesh.reset();
    }

    bool EditorWhiteBoxComponent::HasRenderMesh() const
    {
        // if the optional has a value we know a render mesh exists
        // note: This implicitly implies that the Entity is visible
        return m_renderMesh.has_value();
    }

    void EditorWhiteBoxComponent::OnWireframeModeChange()
    {
        RebuildWireframeMesh();
    }

    void EditorWhiteBoxComponent::RebuildWireframeMesh()
    {
        // Products of Assets/materials/Wireframe in this gem. Overlay edges are dark so they read on the shaded
        // surface; on their own they are light so they read against the dark viewport.
        static constexpr const char* OverlayMaterialPath = "materials/wireframe/whiteboxwireframe.azmaterial";
        static constexpr const char* WireframeOnlyMaterialPath = "materials/wireframe/whiteboxwireframeonly.azmaterial";

        // Needs the shaded mesh's render data, so it also waits for a concrete render mesh (none while the
        // entity is hidden or Edges Only is on).
        if (m_wireframeMode == WireframeMode::Off || !m_renderMesh.has_value() || m_renderData.m_faces.empty())
        {
            m_wireframeRenderMesh.reset();
            ApplyWireframeVisibility();
            return;
        }

        const AZ::Data::AssetId materialId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(
            m_wireframeMode == WireframeMode::Overlay ? OverlayMaterialPath : WireframeOnlyMaterialPath,
            AZ::RPI::AssetUtils::TraceLevel::Warning, azrtti_typeid<AZ::RPI::MaterialAsset>());
        if (!materialId.IsValid())
        {
            m_wireframeRenderMesh.reset();
            ApplyWireframeVisibility();
            return;
        }

        if (!m_wireframeRenderMesh)
        {
            WhiteBoxRequestBus::BroadcastResult(
                m_wireframeRenderMesh, &WhiteBoxRequests::CreateAuxiliaryRenderMeshInterface, GetEntityId());
        }
        if (!m_wireframeRenderMesh)
        {
            return;
        }

        // One material group for every face: per-face materials and paint colours would only split the draw.
        WhiteBoxRenderData wireframeData = m_renderData;
        wireframeData.m_material.m_materialAsset =
            AZ::Data::Asset<AZ::RPI::MaterialAsset>(materialId, azrtti_typeid<AZ::RPI::MaterialAsset>());
        wireframeData.m_material.m_lightmapUvs = false;
        for (WhiteBoxFace& face : wireframeData.m_faces)
        {
            face.m_materialAsset = {};
            face.m_paintColor = 0;
        }

        m_wireframeRenderMesh->SetMaterialAssetOverride(materialId);
        m_wireframeRenderMesh->BuildMesh(wireframeData, m_worldFromLocal);
        m_wireframeRenderMesh->UpdateMaterial(m_material); // compiles the material instance
        ApplyWireframeVisibility();
    }

    void EditorWhiteBoxComponent::ApplyWireframeVisibility()
    {
        const bool showShaded = m_wireframeMode != WireframeMode::WireframeOnly || !m_wireframeRenderMesh;
        if (m_renderMesh.has_value())
        {
            (*m_renderMesh)->SetVisiblity(showShaded);
        }
        for (auto& layerRenderMesh : m_layerRenderMeshes)
        {
            if (layerRenderMesh)
            {
                layerRenderMesh->SetVisiblity(showShaded);
            }
        }
        if (m_wireframeRenderMesh)
        {
            m_wireframeRenderMesh->SetVisiblity(true);
        }
    }

    void EditorWhiteBoxComponent::OnEdgesOnlyChange()
    {
        // Edges-only (also subtract cutters) hides the solid mesh; DisplayEntityViewport draws the edges.
        if (ShowEdgesOnly())
        {
            HideRenderMesh();
        }
        else if (AzToolsFramework::IsEntityVisible(GetEntityId()))
        {
            ShowRenderMesh();
        }
    }

    void EditorWhiteBoxComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // The layer gizmo can snap with component mode closed, so publish the viewport and draw the marker
        // here too. A wrong id only disables snapping.
        SnapUtil::SetActiveViewportId(viewportInfo.m_viewportId);
        SnapUtil::DrawActiveSnapTarget(debugDisplay);

        // m_worldFromLocal lacks Non-Uniform Scale, so apply it to each local point first.
        const AZ::Vector3 entityNonUniformScale = EntityNonUniformScale();
        const auto worldPoint = [this, &entityNonUniformScale](const AZ::Vector3& localPoint)
        {
            return m_worldFromLocal.TransformPoint(localPoint * entityNonUniformScale);
        };

        // One DrawLines per mesh (DrawLine is a draw call per edge); the buffer keeps its capacity.
        const auto drawMeshEdges = [this, &debugDisplay](
                                       const WhiteBoxMesh& mesh, const AZ::Color& color, auto&& toWorld)
        {
            const Api::EdgeHandles edgeHandles = Api::MeshPolygonEdgeHandles(mesh);
            m_debugLineBuffer.clear();
            m_debugLineBuffer.reserve(edgeHandles.size() * 2);
            for (const Api::EdgeHandle& edgeHandle : edgeHandles)
            {
                const AZStd::array<Api::VertexHandle, 2> edgeVerts = Api::EdgeVertexHandles(mesh, edgeHandle);
                m_debugLineBuffer.push_back(toWorld(Api::VertexPosition(mesh, edgeVerts[0])));
                m_debugLineBuffer.push_back(toWorld(Api::VertexPosition(mesh, edgeVerts[1])));
            }
            if (!m_debugLineBuffer.empty())
            {
                debugDisplay.SetColor(color);
                debugDisplay.DrawLines(m_debugLineBuffer, color);
            }
        };

        if (DebugDrawingEnabled())
        {
            WhiteBoxDebugRendering(
                *GetWhiteBoxMesh(), m_worldFromLocal, debugDisplay,
                GetEditorSelectionBoundsViewport(AzFramework::ViewportInfo{}));
        }

        // Edges-only overlay: draw the polygon edges of the hidden solid mesh.
        if (ShowEdgesOnly())
        {
            if (WhiteBoxMesh* mesh = EvaluatedMesh())
            {
                // Tint a subtract cutter differently (warm) so it reads as a boolean brush rather
                // than an ordinary edges-only mesh (cool cyan).
                const bool isCutter =
                    m_boolean.m_booleanOthers && m_boolean.m_cutterOperation == Api::BooleanOperation::Subtraction;
                drawMeshEdges(
                    *mesh,
                    isCutter ? AZ::Color(1.0f, 0.55f, 0.15f, 1.0f) : AZ::Color(0.30f, 0.90f, 1.0f, 1.0f),
                    worldPoint);
            }
        }

        // Per-layer Edges Only: these layers render no faces, so draw their edges (cached for inactive layers).
        if (!ShowEdgesOnly()) // the whole-component overlay above already covers everything
        {
            const int layerCount = static_cast<int>(m_layers.size());
            const int activeIdx = m_layerRuntime.m_loadedIndex;
            for (int i = 0; i < layerCount; ++i)
            {
                const WhiteBoxLayer& layer = m_layers[i];
                if (!layer.m_visible || !layer.m_edgesOnly)
                {
                    continue;
                }

                debugDisplay.SetColor(AZ::Color(0.30f, 0.90f, 1.0f, 1.0f));
                if (i == activeIdx)
                {
                    WhiteBoxMesh* mesh = GetWhiteBoxMesh();
                    if (mesh == nullptr)
                    {
                        continue;
                    }
                    const AZ::Quaternion layerRotation =
                        AZ::Quaternion::CreateFromEulerAnglesDegrees(layer.m_rotation);
                    const auto layerPoint = [&](const AZ::Vector3& p)
                    {
                        // Same order as ApplyTransformToMesh: scale -> rotate -> translate.
                        return worldPoint(layerRotation.TransformVector(p * layer.m_scale) + layer.m_position);
                    };
                    drawMeshEdges(*mesh, AZ::Color(0.30f, 0.90f, 1.0f, 1.0f), layerPoint);
                }
                else
                {
                    auto it = m_layerRuntime.m_meshCache.find(layer.m_id);
                    if (it == m_layerRuntime.m_meshCache.end() || !it->second)
                    {
                        it = m_layerRuntime.m_meshCache.insert_or_assign(layer.m_id, BuildLayerMesh(layer)).first;
                    }
                    WhiteBoxMesh* mesh = it->second.get();
                    if (mesh == nullptr)
                    {
                        continue;
                    }
                    drawMeshEdges(*mesh, AZ::Color(0.30f, 0.90f, 1.0f, 1.0f), worldPoint);
                }
            }
        }
    }
} // namespace WhiteBox
