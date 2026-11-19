/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxMeshUtil.h"

#include "Viewport/WhiteBoxShapeBuilders.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <cmath>

namespace WhiteBox
{
    void AppendMesh(WhiteBoxMesh& dest, const WhiteBoxMesh& src)
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
            Api::CopyVertexBlend(dest, dh, src, srcVertex); // layer merges keep painted blends
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
                Api::CopyFaceAttributes(dest, Api::AddPolygon(dest, faceVertHandles).m_faceHandles, src, polygon.m_faceHandles);
            }
        }
    }

    namespace
    {
        // AppendMesh with the positions mapped, the winding optionally reversed and some polygons left out.
        void AppendMapped(
            WhiteBoxMesh& dest, const WhiteBoxMesh& src, const AZStd::function<AZ::Vector3(const AZ::Vector3&)>& map, const bool reverse,
            const AZStd::function<bool(const Api::PolygonHandle&)>& skip)
        {
            AZStd::unordered_map<int, Api::VertexHandle> vmap;
            const auto destVertex = [&](const Api::VertexHandle& srcVertex) -> Api::VertexHandle
            {
                const auto it = vmap.find(srcVertex.Index());
                if (it != vmap.end())
                {
                    return it->second;
                }
                const Api::VertexHandle dh = Api::AddVertex(dest, map(Api::VertexPosition(src, srcVertex)));
                Api::CopyVertexBlend(dest, dh, src, srcVertex);
                vmap.emplace(srcVertex.Index(), dh);
                return dh;
            };
            for (const Api::PolygonHandle& polygon : Api::MeshPolygonHandles(src))
            {
                if (skip && skip(polygon))
                {
                    continue;
                }
                Api::FaceVertHandlesList faceVertHandles;
                faceVertHandles.reserve(polygon.m_faceHandles.size());
                for (const Api::FaceHandle& faceHandle : polygon.m_faceHandles)
                {
                    const auto halfedges = Api::FaceHalfedgeHandles(src, faceHandle);
                    if (halfedges.size() >= 3)
                    {
                        const int first = reverse ? 2 : 0;
                        const int last = reverse ? 0 : 2;
                        faceVertHandles.push_back(Api::FaceVertHandles{
                            { destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[first])),
                              destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[1])),
                              destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[last])) } });
                    }
                }
                if (!faceVertHandles.empty())
                {
                    Api::CopyFaceAttributes(dest, Api::AddPolygon(dest, faceVertHandles).m_faceHandles, src, polygon.m_faceHandles);
                }
            }
        }
    } // namespace

    Api::WhiteBoxMeshPtr MirrorMesh(const WhiteBoxMesh& src, const int axis)
    {
        const int a = AZ::GetClamp(axis, 0, 2);
        constexpr float OnPlane = 1e-4f;
        // The origin plane, unless the mesh straddles it (a centred parametric shape would land on itself): then its low side.
        float low = AZ::Constants::FloatMax;
        float high = -AZ::Constants::FloatMax;
        for (const Api::VertexHandle vertex : Api::MeshVertexHandles(src))
        {
            const float value = Api::VertexPosition(src, vertex).GetElement(a);
            low = AZStd::min(low, value);
            high = AZStd::max(high, value);
        }
        const float plane = (low < -OnPlane && high > OnPlane) ? low : 0.0f;
        const auto onPlane = [&src, a, plane](const Api::PolygonHandle& polygon)
        {
            for (const Api::FaceHandle& face : polygon.m_faceHandles)
            {
                for (const AZ::Vector3& position : Api::FaceVertexPositions(src, face))
                {
                    if (AZStd::abs(position.GetElement(a) - plane) > OnPlane)
                    {
                        return false;
                    }
                }
            }
            return true;
        };
        Api::WhiteBoxMeshPtr result = Api::CreateWhiteBoxMesh();
        AppendMapped(*result, src, [](const AZ::Vector3& p) { return p; }, false, onPlane);
        // A reflection turns the surface inside out, so the copy's winding is reversed to face outwards again.
        AppendMapped(
            *result, src,
            [a, plane](const AZ::Vector3& p)
            {
                AZ::Vector3 mirrored = p;
                mirrored.SetElement(a, 2.0f * plane - p.GetElement(a));
                return mirrored;
            },
            true, onPlane);
        Api::CalculateNormals(*result);
        Api::CalculatePlanarUVs(*result);
        return result;
    }

    Api::WhiteBoxMeshPtr ArrayMesh(const WhiteBoxMesh& src, const int count, const AZ::Vector3& offset)
    {
        Api::WhiteBoxMeshPtr result = Api::CreateWhiteBoxMesh();
        for (int i = 0; i < AZStd::max(count, 1); ++i)
        {
            const AZ::Vector3 shift = offset * static_cast<float>(i);
            AppendMapped(*result, src, [shift](const AZ::Vector3& p) { return p + shift; }, false, {});
        }
        Api::CalculateNormals(*result);
        Api::CalculatePlanarUVs(*result);
        return result;
    }

    Api::WhiteBoxMeshPtr CombineFreeformAndGrids(WhiteBoxMesh* freeform, WhiteBoxMesh* grid)
    {
        const bool hasFreeform = freeform != nullptr && !Api::MeshFaceHandles(*freeform).empty();
        const bool hasSeparate = grid != nullptr && !Api::MeshFaceHandles(*grid).empty();
        if (!hasFreeform && !hasSeparate)
        {
            return nullptr;
        }
        Api::WhiteBoxMeshPtr combined = hasFreeform ? Api::CloneMesh(*freeform) : Api::CreateWhiteBoxMesh();
        if (!combined)
        {
            return nullptr;
        }
        if (hasSeparate)
        {
            AppendMesh(*combined, *grid);
        }
        Api::CalculateNormals(*combined);
        Api::CalculatePlanarUVs(*combined);
        return combined;
    }

    void ClearMeshInPlace(WhiteBoxMesh& mesh)
    {
        Api::RemoveFaces(mesh, Api::MeshFaceHandles(mesh));
        Api::RemoveIsolatedVertices(mesh);
    }

    bool RestoreMeshFromSnapshot(WhiteBoxMesh& target, const WhiteBoxMesh& snapshot)
    {
        // Round-trip through the serialised form.
        Api::WhiteBoxMeshStream snapshotData;
        if (!Api::WriteMesh(snapshot, snapshotData))
        {
            AZ_Error("WhiteBox", false, "Failed to serialise the White Box mesh snapshot; drag not reverted.");
            return false;
        }

        if (Api::ReadMesh(target, snapshotData) != Api::ReadResult::Full)
        {
            AZ_Error("WhiteBox", false, "Failed to restore the White Box mesh from its snapshot.");
            return false;
        }

        return true;
    }

    Api::WhiteBoxMeshPtr FlippedMeshWinding(const WhiteBoxMesh& src)
    {
        Api::WhiteBoxMeshPtr dest = Api::CreateWhiteBoxMesh();
        AZStd::unordered_map<int, Api::VertexHandle> vmap;
        const auto destVertex = [&](const Api::VertexHandle& srcVertex) -> Api::VertexHandle
        {
            const auto it = vmap.find(srcVertex.Index());
            if (it != vmap.end())
            {
                return it->second;
            }
            const Api::VertexHandle dh = Api::AddVertex(*dest, Api::VertexPosition(src, srcVertex));
            Api::CopyVertexBlend(*dest, dh, src, srcVertex);
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
                    // Reversed vertex order flips the triangle's winding (and thus its normal).
                    faceVertHandles.push_back(Api::FaceVertHandles{
                        { destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[2])),
                          destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[1])),
                          destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[0])) }});
                }
            }
            if (!faceVertHandles.empty())
            {
                Api::CopyFaceAttributes(*dest, Api::AddPolygon(*dest, faceVertHandles).m_faceHandles, src, polygon.m_faceHandles);
            }
        }

        if (!Api::MeshFaceHandles(*dest).empty())
        {
            Api::CalculateNormals(*dest);
            Api::CalculatePlanarUVs(*dest);
        }
        return dest;
    }

    WhiteBoxFace BuildWhiteBoxFace(
        const WhiteBoxMesh& whiteBox, const Api::FaceHandle& faceHandle, const bool flipWinding, const bool withBlend)
    {
        const auto copyVertex = [&whiteBox, withBlend](const Api::HalfedgeHandle& in, WhiteBoxVertex& out)
        {
            const auto vh = Api::HalfedgeVertexHandleAtTip(whiteBox, in);
            out.m_position = Api::VertexPosition(whiteBox, vh);
            out.m_uv = Api::HalfedgeUV(whiteBox, in);
            out.m_blend = withBlend ? Api::VertexBlend(whiteBox, vh) : AZ::Vector3::CreateZero();
        };

        WhiteBoxFace face;
        face.m_normal = Api::FaceNormal(whiteBox, faceHandle);
        face.m_paintColor = Api::FacePaintColor(whiteBox, faceHandle);
        face.m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(
            Api::FaceMaterial(whiteBox, faceHandle), azrtti_typeid<AZ::RPI::MaterialAsset>());
        face.m_materialAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        const auto faceHalfedgeHandles = Api::FaceHalfedgeHandles(whiteBox, faceHandle);
        // Flat faces leave the corner normals zero, so the blob and the renderer skip them.
        const bool smooth = Api::FaceSmoothingGroups(whiteBox, faceHandle) != 0;
        const auto cornerNormals = smooth ? Api::FaceCornerNormals(whiteBox, faceHandle)
                                          : AZStd::array<AZ::Vector3, 3>{ { AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero() } };

        if (flipWinding)
        {
            // Reverse winding (swap v1/v3) and flip the face normal so the layer renders inside-out.
            copyVertex(faceHalfedgeHandles[0], face.m_v3);
            copyVertex(faceHalfedgeHandles[1], face.m_v2);
            copyVertex(faceHalfedgeHandles[2], face.m_v1);
            face.m_normal = -face.m_normal;
            face.m_v3.m_normal = -cornerNormals[0];
            face.m_v2.m_normal = -cornerNormals[1];
            face.m_v1.m_normal = -cornerNormals[2];
        }
        else
        {
            copyVertex(faceHalfedgeHandles[0], face.m_v1);
            copyVertex(faceHalfedgeHandles[1], face.m_v2);
            copyVertex(faceHalfedgeHandles[2], face.m_v3);
            face.m_v1.m_normal = cornerNormals[0];
            face.m_v2.m_normal = cornerNormals[1];
            face.m_v3.m_normal = cornerNormals[2];
        }

        return face;
    }

    WhiteBoxRenderData CreateWhiteBoxRenderData(
        const WhiteBoxMesh& whiteBox, const WhiteBoxMaterial& material, const bool flipWinding)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        WhiteBoxRenderData renderData;
        WhiteBoxFaces& faceData = renderData.m_faces;

        const auto faceCount = Api::MeshFaceCount(whiteBox);
        faceData.reserve(faceCount);

        const auto faceHandles = Api::MeshFaceHandles(whiteBox);
        const bool withBlend = Api::MeshHasVertexBlend(whiteBox);
        for (const auto& faceHandle : faceHandles)
        {
            faceData.push_back(BuildWhiteBoxFace(whiteBox, faceHandle, flipWinding, withBlend));
        }

        renderData.m_material = material;

        return renderData;
    }

    namespace
    {
        // Prefer a boundary fan (two triangles for a quad). Retain every boundary
        // vertex so adjoining stepped faces share complete edges without T-junctions.
        void AddParametricFace(
            WhiteBoxMesh& mesh, const AZStd::vector<Api::VertexHandle>& handles,
            const AZStd::vector<AZ::Vector3>& positions, const AZStd::vector<AZ::u32>& loop,
            const AZ::Vector3& outward)
        {
            for (size_t anchor = 0; anchor < loop.size(); ++anchor)
            {
                Api::FaceVertHandlesList triangles;
                triangles.reserve(loop.size() - 2);
                for (size_t i = 1; i + 1 < loop.size(); ++i)
                {
                    const auto a = loop[anchor];
                    const auto b = loop[(anchor + i) % loop.size()];
                    const auto c = loop[(anchor + i + 1) % loop.size()];
                    const auto normal = (positions[b] - positions[a]).Cross(positions[c] - positions[a]);
                    if (normal.GetLengthSq() <= 1e-12f)
                    {
                        break; // This anchor would lose a collinear boundary segment.
                    }
                    const bool flip = normal.Dot(outward) < 0.0f;
                    triangles.push_back(Api::FaceVertHandles{{handles[a], handles[flip ? c : b], handles[flip ? b : c]}});
                }
                if (triangles.size() == loop.size() - 2)
                {
                    Api::AddPolygon(mesh, triangles);
                    return;
                }
            }
            // A center fan is only needed when no boundary anchor preserves all edges.
            AZ::Vector3 center = AZ::Vector3::CreateZero();
            for (const auto index : loop)
            {
                center += positions[index];
            }
            center /= static_cast<float>(loop.size());
            const auto middle = Api::AddVertex(mesh, center);
            Api::FaceVertHandlesList faces;
            for (size_t i = 0; i < loop.size(); ++i)
            {
                const auto a = loop[i];
                const auto b = loop[(i + 1) % loop.size()];
                const bool flip = (positions[a] - center).Cross(positions[b] - center).Dot(outward) < 0.0f;
                faces.push_back(Api::FaceVertHandles{{middle, handles[flip ? b : a], handles[flip ? a : b]}});
            }
            Api::AddPolygon(mesh, faces);
        }

        void BuildDoor(
            WhiteBoxMesh& mesh, float width, float depth, float height, int segments,
            float thickness, bool frame, float archHeight)
        {
            const float radius = width * 0.5f;
            const float rise = AZStd::clamp(archHeight, 0.0f, height - 0.001f);
            const float spring = height - rise;
            const float t = AZStd::max(thickness, 0.001f);
            AZStd::vector<AZ::Vector3> inner{{-radius, 0.0f, 0.0f}};
            AZStd::vector<AZ::Vector3> outer{{-radius - t, 0.0f, 0.0f}};
            if (rise == 0.0f)
            {
                inner.push_back(AZ::Vector3(-radius, 0.0f, height));
                inner.push_back(AZ::Vector3(radius, 0.0f, height));
                outer.push_back(AZ::Vector3(-radius - t, 0.0f, height + t));
                outer.push_back(AZ::Vector3(radius + t, 0.0f, height + t));
            }
            else
            {
                // An even segment count includes the exact apex.
                segments += segments % 2;
                for (int i = 0; i <= segments; ++i)
                {
                    const float angle = AZ::Constants::Pi * static_cast<float>(i) / segments;
                    const float x = -std::cos(angle);
                    const float z = (i == 0 || i == segments) ? 0.0f : std::sin(angle);
                    inner.push_back(AZ::Vector3(radius * x, 0.0f, spring + rise * z));
                    outer.push_back(AZ::Vector3((radius + t) * x, 0.0f, spring + (rise + t) * z));
                }
            }
            inner.push_back(AZ::Vector3(radius, 0.0f, 0.0f));
            outer.push_back(AZ::Vector3(radius + t, 0.0f, 0.0f));
            AZStd::vector<AZ::Vector3> positions;
            AZStd::vector<Api::VertexHandle> handles;
            const AZ::u32 count = static_cast<AZ::u32>(inner.size());
            for (int side = 0; side < 2; ++side)
            {
                for (int ring = 0; ring < (frame ? 2 : 1); ++ring)
                {
                    for (const auto& point : (ring == 0 ? inner : outer))
                    {
                        const AZ::Vector3 position = point + AZ::Vector3(0.0f, (side ? 0.5f : -0.5f) * depth, 0.0f);
                        positions.push_back(position);
                        handles.push_back(Api::AddVertex(mesh, position));
                    }
                }
            }
            if (!frame)
            {
                AZStd::vector<AZ::u32> front, back;
                for (AZ::u32 i = 0; i < count; ++i)
                {
                    front.push_back(i);
                    back.push_back(count + i);
                    const AZ::u32 next = (i + 1) % count;
                    Detail::AddOutwardFace(mesh, handles, positions, {i, next, count + next, count + i},
                        AZ::Vector3(0.0f, 0.0f, height * 0.5f));
                }
                AddParametricFace(mesh, handles, positions, front, -AZ::Vector3::CreateAxisY());
                AddParametricFace(mesh, handles, positions, back, AZ::Vector3::CreateAxisY());
                return;
            }
            for (AZ::u32 i = 0; i + 1 < count; ++i)
            {
                const AZ::u32 j = i + 1;
                const AZ::Vector3 center = (inner[i] + inner[j] + outer[i] + outer[j]) * 0.25f;
                const auto face = [&](AZStd::vector<AZ::u32> loop)
                {
                    Detail::AddOutwardFace(mesh, handles, positions, loop, center);
                };
                face({i, j, count + j, count + i});
                face({2 * count + i, 3 * count + i, 3 * count + j, 2 * count + j});
                face({i, 2 * count + i, 2 * count + j, j});
                face({count + i, count + j, 3 * count + j, 3 * count + i});
                if (i == 0)
                {
                    face({i, count + i, 3 * count + i, 2 * count + i});
                }
                if (j == count - 1)
                {
                    face({j, 2 * count + j, 3 * count + j, count + j});
                }
            }
        }

        void BuildCircularStairs(WhiteBoxMesh& mesh, float width, float height, int steps, float innerRadius, float sweepAngle)
        {
            const float radius = AZStd::max(innerRadius, 0.01f);
            const float degrees = AZStd::clamp(sweepAngle, 1.0f, 360.0f);
            const bool closed = degrees == 360.0f;
            const int subdivisions = AZStd::max(1, static_cast<int>(std::ceil(degrees / (steps * 15.0f))));
            const int columns = steps * subdivisions;
            const float sweep = degrees * AZ::Constants::Pi / 180.0f;
            AZStd::vector<AZ::Vector3> positions;
            AZStd::vector<Api::VertexHandle> handles;
            AZStd::unordered_map<int, AZ::u32> indices;
            const auto vertex = [&](int side, int column, int level)
            {
                if (closed && column == columns)
                {
                    column = 0; // Shared seam, including the bottom ring.
                }
                const int key = (side * (columns + 1) + column) * (steps + 1) + level;
                const auto found = indices.find(key);
                if (found != indices.end())
                {
                    return found->second;
                }
                const float angle = sweep * static_cast<float>(column) / columns;
                const float r = radius + side * width;
                const AZ::Vector3 position(r * std::cos(angle), r * std::sin(angle), height * level / steps);
                const AZ::u32 index = static_cast<AZ::u32>(handles.size());
                positions.push_back(position);
                handles.push_back(Api::AddVertex(mesh, position));
                indices.emplace(key, index);
                return index;
            };
            const auto levelAt = [&](int column)
            {
                if (column < 0) { return closed ? steps : 0; }
                if (column >= columns) { return closed ? 1 : 0; }
                return column / subdivisions + 1;
            };
            const auto face = [&](const AZStd::vector<AZ::u32>& loop, const AZ::Vector3& outward)
            {
                AddParametricFace(mesh, handles, positions, loop, outward);
            };
            for (int c = 0; c < columns; ++c)
            {
                const int level = levelAt(c);
                const int previous = levelAt(c - 1);
                const int next = levelAt(c + 1);
                const float angle = sweep * (c + 0.5f) / columns;
                const AZ::Vector3 radial(std::cos(angle), std::sin(angle), 0.0f);
                for (int side = 0; side < 2; ++side)
                {
                    AZStd::vector<AZ::u32> loop{vertex(side, c + 1, 0)};
                    if (next > 0 && next < level) { loop.push_back(vertex(side, c + 1, next)); }
                    loop.push_back(vertex(side, c + 1, level));
                    loop.push_back(vertex(side, c, level));
                    if (previous > 0 && previous < level) { loop.push_back(vertex(side, c, previous)); }
                    loop.push_back(vertex(side, c, 0));
                    face(loop, side ? radial : -radial);
                }
                face({vertex(0, c, 0), vertex(1, c, 0), vertex(1, c + 1, 0), vertex(0, c + 1, 0)},
                    -AZ::Vector3::CreateAxisZ());
                face({vertex(0, c, level), vertex(1, c, level), vertex(1, c + 1, level), vertex(0, c + 1, level)},
                    AZ::Vector3::CreateAxisZ());
                if (previous < level)
                {
                    const float a = sweep * c / columns;
                    face({vertex(0, c, previous), vertex(1, c, previous), vertex(1, c, level), vertex(0, c, level)},
                        AZ::Vector3(std::sin(a), -std::cos(a), 0.0f));
                }
                if (next < level)
                {
                    const float a = sweep * (c + 1) / columns;
                    face({vertex(0, c + 1, next), vertex(1, c + 1, next),
                        vertex(1, c + 1, level), vertex(0, c + 1, level)},
                        AZ::Vector3(-std::sin(a), std::cos(a), 0.0f));
                }
            }
        }
    }

    Api::WhiteBoxMeshPtr BuildParametricShapeMesh(
        const DrawShapeType shape, const float width, const float depth, const float height, const int sides,
        const int steps, const float wallThickness, const float cavityGap, const bool floor, const bool ceiling,
        const bool doorFrame, const float archHeight, const float innerRadius, const float sweepAngle,
        const bool stepsByHeight, const float stepHeight, const float holeRatio, const int tubeSides)
    {
        const float w = AZStd::max(width, 0.01f);
        const float d = AZStd::max(depth, 0.01f);
        const float h = AZStd::max(height, 0.01f);
        const int clampedSides = AZStd::clamp(sides, 3, 128);
        // Match Draw Shape's nearest-count behavior while retaining the layer's 128-step limit.
        // Clamp before conversion so very small riser heights cannot overflow the count.
        const bool stair = shape == DrawShapeType::Staircase || shape == DrawShapeType::CircularStairs;
        const int clampedSteps = stair && stepsByHeight
            ? static_cast<int>(std::lround(AZStd::clamp(h / AZStd::max(stepHeight, 0.001f), 1.0f, 128.0f)))
            : AZStd::clamp(steps, 1, 128);

        Api::WhiteBoxMeshPtr mesh = Api::CreateWhiteBoxMesh();

        // The Room is a shell, not a footprint-extruded solid, so it has its own builder rather than
        // going through BuildShapeSolid. width/depth/height are its INTERIOR dimensions here.
        if (shape == DrawShapeType::Room)
        {
            Detail::BuildRoomSolid(*mesh, w, d, h, wallThickness, cavityGap, floor, ceiling);
        }
        else if (shape == DrawShapeType::Door)
        {
            BuildDoor(*mesh, w, d, h, clampedSides, wallThickness, doorFrame, archHeight);
        }
        else if (shape == DrawShapeType::CircularStairs)
        {
            BuildCircularStairs(*mesh, w, h, clampedSteps, innerRadius, sweepAngle);
        }
        else
        {
            // Note: the builders take FULL-extent axis vectors (they halve internally).
            Detail::BuildShapeSolid(
                *mesh, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateZero(),
                AZ::Vector3(w, 0.0f, 0.0f), AZ::Vector3(0.0f, d, 0.0f),
                AZ::Vector3::CreateAxisZ(), 0.0f, h, shape, clampedSides, clampedSteps, holeRatio, tubeSides);
        }
        if (!Api::MeshFaceHandles(*mesh).empty())
        {
            Api::CalculateNormals(*mesh);
            Api::CalculatePlanarUVs(*mesh);
        }
        return mesh;
    }

    bool DisplayingAsset(const DefaultShapeType defaultShapeType)
    {
        // checks if the default shape is set to a custom asset
        return defaultShapeType == DefaultShapeType::Asset;
    }

    void RefreshProperties()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }
} // namespace WhiteBox
