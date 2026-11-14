/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxCsg.h"

#include "WhiteBoxCsgCore.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/utils.h>
#include <WhiteBox/WhiteBoxToolApi.h>

#include <cmath>
#include <cstdlib>

namespace WhiteBox
{
    namespace Api
    {
        namespace
        {
            // tolerance (in meters) within which two vertices are considered identical
            constexpr double WeldTolerance = 1e-6;
            // two adjacent triangles are merged into one polygon when their normals deviate
            // by less than this (dot product threshold, ~0.5 degrees)
            constexpr float CoplanarNormalDotThreshold = 0.99996f;
            // The solvers keep one string per triangle, so a non-default UV projection rides after the material id.
            constexpr char UvProjectionSeparator = '';
            constexpr char SmoothingSeparator = '\x1e'; // smoothing groups ride at the end of the key

            std::string EncodeFaceKey(const AZ::Data::AssetId& material, const UvProjection& projection, const AZ::u32 smoothing)
            {
                std::string key = material.IsValid() ? material.ToString<AZStd::string>().c_str() : "";
                if (!projection.IsDefault())
                {
                    key += AZStd::string::format(
                               "%c%u %.9g %.9g %.9g %.9g %.9g", UvProjectionSeparator, static_cast<unsigned>(projection.m_mode),
                               projection.m_scale.GetX(), projection.m_scale.GetY(), projection.m_offset.GetX(),
                               projection.m_offset.GetY(), projection.m_rotationDegrees)
                               .c_str();
                    // A Manual map is anchored at the origin and rounded, so the triangles of one flat polygon share a key.
                    if (projection.m_mode == UvProjectionMode::Manual && projection.m_manual.IsValid())
                    {
                        const ManualUvMap& map = projection.m_manual;
                        const AZ::Vector2 atOrigin = map.Evaluate(AZ::Vector3::CreateZero());
                        key += AZStd::string::format(
                                   " %.6g %.6g %.6g %.6g %.6g %.6g %.6g %.6g %.6g %.6g %.6g", atOrigin.GetX(), atOrigin.GetY(),
                                   map.m_uGradient.GetX(), map.m_uGradient.GetY(), map.m_uGradient.GetZ(), map.m_vGradient.GetX(),
                                   map.m_vGradient.GetY(), map.m_vGradient.GetZ(), map.m_normal.GetX(), map.m_normal.GetY(),
                                   map.m_normal.GetZ())
                                   .c_str();
                    }
                }
                if (smoothing != 0)
                {
                    key += AZStd::string::format("%c%u", SmoothingSeparator, static_cast<unsigned>(smoothing)).c_str();
                }
                return key;
            }

            void DecodeFaceKey(const std::string& fullKey, AZ::Data::AssetId& material, UvProjection& projection, AZ::u32& smoothing)
            {
                const size_t smoothingSplit = fullKey.find(SmoothingSeparator);
                smoothing = smoothingSplit == std::string::npos
                    ? 0u : static_cast<AZ::u32>(std::strtoul(fullKey.c_str() + smoothingSplit + 1, nullptr, 10));
                const std::string key = fullKey.substr(0, smoothingSplit);
                const size_t split = key.find(UvProjectionSeparator);
                const std::string materialId = key.substr(0, split);
                material = materialId.empty() ? AZ::Data::AssetId{} : AZ::Data::AssetId::CreateString(materialId.c_str());
                projection = UvProjection{};
                if (split == std::string::npos)
                {
                    return;
                }
                char* end = nullptr;
                const unsigned long mode = std::strtoul(key.c_str() + split + 1, &end, 10);
                float values[5] = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
                for (float& value : values)
                {
                    const char* cursor = end;
                    value = std::strtof(cursor, &end);
                }
                projection.m_mode = mode == static_cast<unsigned long>(UvProjectionMode::Planar)   ? UvProjectionMode::Planar
                    : mode == static_cast<unsigned long>(UvProjectionMode::Manual) ? UvProjectionMode::Manual
                                                                                   : UvProjectionMode::World;
                projection.m_scale = AZ::Vector2(values[0], values[1]);
                projection.m_offset = AZ::Vector2(values[2], values[3]);
                projection.m_rotationDegrees = values[4];
                if (projection.m_mode == UvProjectionMode::Manual)
                {
                    float map[11] = {};
                    for (float& value : map)
                    {
                        const char* cursor = end;
                        value = std::strtof(cursor, &end);
                    }
                    projection.m_manual.m_origin = AZ::Vector3::CreateZero();
                    projection.m_manual.m_originUv = AZ::Vector2(map[0], map[1]);
                    projection.m_manual.m_uGradient = AZ::Vector3(map[2], map[3], map[4]);
                    projection.m_manual.m_vGradient = AZ::Vector3(map[5], map[6], map[7]);
                    projection.m_manual.m_normal = AZ::Vector3(map[8], map[9], map[10]);
                }
            }

            // A Manual map moved into the space the solver works in, so a cutter's authored UVs land where its faces do.
            UvProjection TransformedProjection(UvProjection projection, const AZ::Transform& transform)
            {
                if (projection.m_mode != UvProjectionMode::Manual || !projection.m_manual.IsValid() || transform.IsClose(AZ::Transform::CreateIdentity()))
                {
                    return projection;
                }
                // For p' = sRp + t the gradient becomes Rg / s, which is TransformVector(g) / s^2.
                const float scale = transform.GetUniformScale();
                const float inverseScaleSq = scale > 0.0f ? 1.0f / (scale * scale) : 1.0f;
                ManualUvMap& map = projection.m_manual;
                map.m_origin = transform.TransformPoint(map.m_origin);
                map.m_uGradient = transform.TransformVector(map.m_uGradient) * inverseScaleSq;
                map.m_vGradient = transform.TransformVector(map.m_vGradient) * inverseScaleSq;
                map.m_normal = transform.TransformVector(map.m_normal).GetNormalizedSafe();
                return projection;
            }
        } // namespace

        namespace Detail
        {
            // convert a white box mesh to an indexed triangle mesh, transforming all
            // positions by the given transform
            Csg::TriangleMesh ToTriangleMesh(const WhiteBoxMesh& whiteBox, const AZ::Transform& transform)
            {
                const Faces faces = MeshFaces(whiteBox);

                Csg::TriangleMesh triangleMesh;
                triangleMesh.m_positions.reserve(faces.size() * 9);
                triangleMesh.m_indices.reserve(faces.size() * 3);

                const FaceHandles faceHandles = MeshFaceHandles(whiteBox);
                size_t faceIndex = 0;
                uint32_t index = 0;
                for (const Face& face : faces)
                {
                    triangleMesh.m_colors.push_back(FacePaintColor(whiteBox, faceHandles[faceIndex]));
                    triangleMesh.m_materials.emplace_back(EncodeFaceKey(
                        FaceMaterial(whiteBox, faceHandles[faceIndex]),
                        TransformedProjection(FaceUvProjection(whiteBox, faceHandles[faceIndex]), transform),
                        FaceSmoothingGroups(whiteBox, faceHandles[faceIndex])));
                    ++faceIndex;
                    for (const AZ::Vector3& position : face)
                    {
                        const AZ::Vector3 transformedPosition = transform.TransformPoint(position);
                        triangleMesh.m_positions.push_back(transformedPosition.GetX());
                        triangleMesh.m_positions.push_back(transformedPosition.GetY());
                        triangleMesh.m_positions.push_back(transformedPosition.GetZ());
                        triangleMesh.m_indices.push_back(index++);
                    }
                }

                // index the triangle soup so shared vertices are welded
                Csg::WeldVertices(triangleMesh, WeldTolerance);

                return triangleMesh;
            }

            AZ::Vector3 TriangleMeshPosition(const Csg::TriangleMesh& triangleMesh, const uint32_t vertexIndex)
            {
                return AZ::Vector3(
                    static_cast<float>(triangleMesh.m_positions[vertexIndex * 3 + 0]),
                    static_cast<float>(triangleMesh.m_positions[vertexIndex * 3 + 1]),
                    static_cast<float>(triangleMesh.m_positions[vertexIndex * 3 + 2]));
            }

            AZ::Vector3 TriangleNormal(const Csg::TriangleMesh& triangleMesh, const size_t triangleIndex)
            {
                const AZ::Vector3 a = TriangleMeshPosition(triangleMesh, triangleMesh.m_indices[triangleIndex * 3 + 0]);
                const AZ::Vector3 b = TriangleMeshPosition(triangleMesh, triangleMesh.m_indices[triangleIndex * 3 + 1]);
                const AZ::Vector3 c = TriangleMeshPosition(triangleMesh, triangleMesh.m_indices[triangleIndex * 3 + 2]);
                return (b - a).Cross(c - a).GetNormalizedSafe();
            }

            // group triangles into connected coplanar regions - each region becomes
            // one white box polygon (interior edges hidden, matching how the white box
            // tool represents quads and n-gons as groups of triangles)
            AZStd::vector<AZStd::vector<size_t>> GroupCoplanarTriangles(const Csg::TriangleMesh& triangleMesh)
            {
                const size_t triangleCount = triangleMesh.TriangleCount();

                // undirected edge (low vertex index, high vertex index) -> adjacent triangles
                using EdgeKey = AZStd::pair<uint32_t, uint32_t>;
                AZStd::unordered_map<EdgeKey, AZStd::vector<size_t>> edgeToTriangles;
                for (size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
                {
                    for (int edge = 0; edge < 3; ++edge)
                    {
                        const uint32_t v0 = triangleMesh.m_indices[triangleIndex * 3 + edge];
                        const uint32_t v1 = triangleMesh.m_indices[triangleIndex * 3 + (edge + 1) % 3];
                        edgeToTriangles[EdgeKey(AZStd::min(v0, v1), AZStd::max(v0, v1))].push_back(triangleIndex);
                    }
                }

                AZStd::vector<AZ::Vector3> triangleNormals;
                triangleNormals.reserve(triangleCount);
                for (size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
                {
                    triangleNormals.push_back(TriangleNormal(triangleMesh, triangleIndex));
                }

                // flood fill across shared edges while normals stay (close to) coplanar
                AZStd::vector<AZStd::vector<size_t>> groups;
                AZStd::vector<bool> visited(triangleCount, false);
                for (size_t seedTriangle = 0; seedTriangle < triangleCount; ++seedTriangle)
                {
                    if (visited[seedTriangle])
                    {
                        continue;
                    }

                    AZStd::vector<size_t> group;
                    AZStd::vector<size_t> stack{ seedTriangle };
                    visited[seedTriangle] = true;

                    while (!stack.empty())
                    {
                        const size_t triangleIndex = stack.back();
                        stack.pop_back();
                        group.push_back(triangleIndex);

                        for (int edge = 0; edge < 3; ++edge)
                        {
                            const uint32_t v0 = triangleMesh.m_indices[triangleIndex * 3 + edge];
                            const uint32_t v1 = triangleMesh.m_indices[triangleIndex * 3 + (edge + 1) % 3];
                            const auto& neighbors =
                                edgeToTriangles[EdgeKey(AZStd::min(v0, v1), AZStd::max(v0, v1))];

                            for (const size_t neighborTriangle : neighbors)
                            {
                                if (!visited[neighborTriangle] &&
                                    triangleMesh.Material(triangleIndex) == triangleMesh.Material(neighborTriangle) &&
                                    triangleMesh.Color(triangleIndex) == triangleMesh.Color(neighborTriangle) &&
                                    triangleNormals[triangleIndex].Dot(triangleNormals[neighborTriangle]) >
                                        CoplanarNormalDotThreshold)
                                {
                                    visited[neighborTriangle] = true;
                                    stack.push_back(neighborTriangle);
                                }
                            }
                        }
                    }

                    groups.push_back(AZStd::move(group));
                }

                return groups;
            }

            // ---- coplanar-face cleanup -----------------------------------------------
            // A boolean leaves each flat region as a fan of triangles plus extra
            // vertices inserted along straight edges (where the cutter crossed). The
            // helpers below rebuild each coplanar region as a single polygon with those
            // redundant vertices removed and a clean triangulation (holes supported).
            // Everything is validated (all-CCW + matching area); on ANY failure the
            // caller keeps the region's original triangles, so this never makes the
            // result worse than the un-cleaned mesh.

            // Twice the signed area of triangle (a,b,c); > 0 when CCW.
            double Cross2(const V2& a, const V2& b, const V2& c)
            {
                return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            }

            // Is point p inside (or on) triangle (a,b,c)?
            bool PointInTri(const V2& p, const V2& a, const V2& b, const V2& c)
            {
                const double d1 = Cross2(a, b, p);
                const double d2 = Cross2(b, c, p);
                const double d3 = Cross2(c, a, p);
                const bool hasNeg = (d1 < 0.0) || (d2 < 0.0) || (d3 < 0.0);
                const bool hasPos = (d1 > 0.0) || (d2 > 0.0) || (d3 > 0.0);
                return !(hasNeg && hasPos);
            }

            // Ear-clip a simple CCW polygon (2D), emitting triangle index-triples into 'pts'.
            bool EarClip(const AZStd::vector<V2>& pts, AZStd::vector<AZStd::array<uint32_t, 3>>& outTris)
            {
                const int n = static_cast<int>(pts.size());
                if (n < 3)
                {
                    return false;
                }

                AZStd::vector<int> v(n);
                for (int i = 0; i < n; ++i)
                {
                    v[i] = i;
                }

                int nv = n;
                int guard = 2 * nv;
                for (int idx = nv - 1; nv > 2;)
                {
                    if ((guard--) <= 0)
                    {
                        return false; // couldn't find an ear (bad polygon)
                    }
                    int u = idx;       if (nv <= u) { u = 0; }
                    idx = u + 1;       if (nv <= idx) { idx = 0; }
                    int w = idx + 1;   if (nv <= w) { w = 0; }

                    const V2& A = pts[v[u]];
                    const V2& B = pts[v[idx]];
                    const V2& C = pts[v[w]];

                    if (Cross2(A, B, C) > 1e-12) // convex corner
                    {
                        bool ear = true;
                        for (int p = 0; p < nv; ++p)
                        {
                            if (p == u || p == idx || p == w)
                            {
                                continue;
                            }
                            if (PointInTri(pts[v[p]], A, B, C))
                            {
                                ear = false;
                                break;
                            }
                        }

                        if (ear)
                        {
                            outTris.push_back({ static_cast<uint32_t>(v[u]), static_cast<uint32_t>(v[idx]),
                                                static_cast<uint32_t>(v[w]) });
                            for (int s = idx, t = idx + 1; t < nv; ++s, ++t)
                            {
                                v[s] = v[t];
                            }
                            --nv;
                            guard = 2 * nv;
                            idx = nv - 1;
                        }
                    }
                }
                return true;
            }

            // Merge holes (CW) into the outer loop (CCW) producing one simple CCW polygon
            // by inserting a 'bridge' edge per hole (classic ear-clipping hole handling).
            bool BridgeHoles(
                const AZStd::vector<V2>& outerPts, const AZStd::vector<uint32_t>& outerGid,
                AZStd::vector<AZStd::vector<V2>>& holePts, AZStd::vector<AZStd::vector<uint32_t>>& holeGid,
                AZStd::vector<V2>& mergedPts, AZStd::vector<uint32_t>& mergedGid)
            {
                mergedPts = outerPts;
                mergedGid = outerGid;

                // process holes by descending rightmost-x so bridges don't cross
                AZStd::vector<size_t> order(holePts.size());
                for (size_t i = 0; i < order.size(); ++i)
                {
                    order[i] = i;
                }
                const auto holeMaxX = [&](size_t h)
                {
                    double mx = -1e300;
                    for (const V2& p : holePts[h])
                    {
                        mx = AZStd::max(mx, p.x);
                    }
                    return mx;
                };
                AZStd::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return holeMaxX(a) > holeMaxX(b); });

                for (const size_t oi : order)
                {
                    const AZStd::vector<V2>& hp = holePts[oi];
                    const AZStd::vector<uint32_t>& hg = holeGid[oi];
                    const int hn = static_cast<int>(hp.size());
                    if (hn < 3)
                    {
                        return false;
                    }

                    // M = rightmost hole vertex
                    int m = 0;
                    for (int i = 1; i < hn; ++i)
                    {
                        if (hp[i].x > hp[m].x || (hp[i].x == hp[m].x && hp[i].y > hp[m].y))
                        {
                            m = i;
                        }
                    }
                    const V2 mp = hp[m];

                    // cast a ray from M in +x; find the merged edge it first crosses
                    const int mn = static_cast<int>(mergedPts.size());
                    double bestX = 1e300;
                    int bestEdge = -1;
                    V2 hitPt{ 0.0, 0.0 };
                    for (int i = 0; i < mn; ++i)
                    {
                        const V2& a = mergedPts[i];
                        const V2& b = mergedPts[(i + 1) % mn];
                        if ((a.y > mp.y) == (b.y > mp.y))
                        {
                            continue; // edge doesn't straddle the ray
                        }
                        const double tt = (mp.y - a.y) / (b.y - a.y);
                        const double ix = a.x + tt * (b.x - a.x);
                        if (ix > mp.x - 1e-12 && ix < bestX)
                        {
                            bestX = ix;
                            bestEdge = i;
                            hitPt = V2{ ix, mp.y };
                        }
                    }
                    if (bestEdge < 0)
                    {
                        return false;
                    }

                    // bridge target P: the straddled edge's vertex with larger x, then
                    // refined to any reflex merged vertex inside triangle (M, hit, P)
                    int pIdx = (mergedPts[bestEdge].x > mergedPts[(bestEdge + 1) % mn].x) ? bestEdge : (bestEdge + 1) % mn;
                    {
                        const V2 pp = mergedPts[pIdx];
                        double bestAng = 1e300;
                        for (int i = 0; i < mn; ++i)
                        {
                            if (i == pIdx)
                            {
                                continue;
                            }
                            const V2& r = mergedPts[i];
                            if (r.x <= mp.x)
                            {
                                continue;
                            }
                            if (PointInTri(r, mp, hitPt, pp))
                            {
                                const double ang = std::atan2(std::abs(r.y - mp.y), r.x - mp.x);
                                if (ang < bestAng)
                                {
                                    bestAng = ang;
                                    pIdx = i;
                                }
                            }
                        }
                    }

                    // splice: merged[0..P] + hole(M..around..M) + merged[P..]
                    AZStd::vector<V2> np;
                    AZStd::vector<uint32_t> ng;
                    np.reserve(mergedPts.size() + hn + 2);
                    ng.reserve(mergedPts.size() + hn + 2);
                    for (int i = 0; i <= pIdx; ++i)
                    {
                        np.push_back(mergedPts[i]);
                        ng.push_back(mergedGid[i]);
                    }
                    for (int k = 0; k <= hn; ++k)
                    {
                        const int hi = (m + k) % hn;
                        np.push_back(hp[hi]);
                        ng.push_back(hg[hi]);
                    }
                    for (int i = pIdx; i < static_cast<int>(mergedPts.size()); ++i)
                    {
                        np.push_back(mergedPts[i]);
                        ng.push_back(mergedGid[i]);
                    }
                    mergedPts = AZStd::move(np);
                    mergedGid = AZStd::move(ng);
                }
                return true;
            }

            // A vertex is "removable" iff it sits mid-way along a STRAIGHT visible edge -
            // i.e. it has exactly two feature-edge neighbours (edges shared between two
            // different coplanar groups) and they are collinear through it. Such a vertex
            // is redundant on both adjacent faces, so dropping it never opens a crack.
            AZStd::unordered_set<uint32_t> ComputeRemovableVertices(
                const Csg::TriangleMesh& mesh, const AZStd::vector<AZStd::vector<size_t>>& groups)
            {
                AZStd::vector<int> triGroup(mesh.TriangleCount(), -1);
                for (int gi = 0; gi < static_cast<int>(groups.size()); ++gi)
                {
                    for (const size_t t : groups[gi])
                    {
                        triGroup[t] = gi;
                    }
                }

                const auto ukey = [](uint32_t a, uint32_t b)
                {
                    const uint32_t lo = AZStd::min(a, b);
                    const uint32_t hi = AZStd::max(a, b);
                    return (static_cast<uint64_t>(lo) << 32) | static_cast<uint64_t>(hi);
                };

                AZStd::unordered_map<uint64_t, AZStd::vector<size_t>> edgeTris;
                for (size_t t = 0; t < mesh.TriangleCount(); ++t)
                {
                    const uint32_t v0 = mesh.m_indices[t * 3 + 0];
                    const uint32_t v1 = mesh.m_indices[t * 3 + 1];
                    const uint32_t v2 = mesh.m_indices[t * 3 + 2];
                    edgeTris[ukey(v0, v1)].push_back(t);
                    edgeTris[ukey(v1, v2)].push_back(t);
                    edgeTris[ukey(v2, v0)].push_back(t);
                }

                // feature-edge neighbours per vertex
                AZStd::unordered_map<uint32_t, AZStd::vector<uint32_t>> featNbr;
                for (const auto& kv : edgeTris)
                {
                    const uint32_t a = static_cast<uint32_t>(kv.first >> 32);
                    const uint32_t b = static_cast<uint32_t>(kv.first & 0xffffffffu);
                    const AZStd::vector<size_t>& tris = kv.second;
                    bool feature = true; // boundary / non-manifold edges are always kept
                    if (tris.size() == 2)
                    {
                        feature = (triGroup[tris[0]] != triGroup[tris[1]]);
                    }
                    if (feature)
                    {
                        featNbr[a].push_back(b);
                        featNbr[b].push_back(a);
                    }
                }

                AZStd::unordered_set<uint32_t> removable;
                for (const auto& kv : featNbr)
                {
                    const AZStd::vector<uint32_t>& nbrs = kv.second;
                    if (nbrs.size() != 2)
                    {
                        continue; // corner, junction, or dead-end -> keep
                    }
                    const AZ::Vector3 p = TriangleMeshPosition(mesh, kv.first);
                    const AZ::Vector3 d1 = TriangleMeshPosition(mesh, nbrs[0]) - p;
                    const AZ::Vector3 d2 = TriangleMeshPosition(mesh, nbrs[1]) - p;
                    const float l1 = d1.GetLength();
                    const float l2 = d2.GetLength();
                    if (l1 < 1e-7f || l2 < 1e-7f)
                    {
                        continue;
                    }
                    const AZ::Vector3 u1 = d1 / l1;
                    const AZ::Vector3 u2 = d2 / l2;
                    if (u1.Cross(u2).GetLength() < 1e-4f && u1.Dot(u2) < -0.9999f) // straight through p
                    {
                        removable.insert(kv.first);
                    }
                }
                return removable;
            }

            // Rebuild one coplanar group as clean triangles (global vertex indices) with
            // redundant collinear vertices removed. Returns false (caller falls back to
            // the original triangles) for anything it can't safely simplify.
            bool CleanCoplanarGroup(
                const Csg::TriangleMesh& mesh, const AZStd::vector<size_t>& group, const AZ::Vector3& groupNormal,
                const AZStd::unordered_set<uint32_t>& removable,
                AZStd::vector<AZStd::array<uint32_t, 3>>& outTris)
            {
                outTris.clear();
                if (group.empty())
                {
                    return false;
                }
                if (group.size() == 1)
                {
                    const size_t t = group[0];
                    outTris.push_back(
                        { mesh.m_indices[t * 3 + 0], mesh.m_indices[t * 3 + 1], mesh.m_indices[t * 3 + 2] });
                    return true;
                }

                const auto ekey = [](uint32_t a, uint32_t b)
                { return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b); };

                // directed edges of the group; an undirected edge is interior when both
                // directions are present, boundary when only one is.
                AZStd::unordered_set<uint64_t> dir;
                dir.reserve(group.size() * 3);
                for (const size_t t : group)
                {
                    const uint32_t a = mesh.m_indices[t * 3 + 0];
                    const uint32_t b = mesh.m_indices[t * 3 + 1];
                    const uint32_t c = mesh.m_indices[t * 3 + 2];
                    dir.insert(ekey(a, b));
                    dir.insert(ekey(b, c));
                    dir.insert(ekey(c, a));
                }

                AZStd::unordered_map<uint32_t, uint32_t> nextOf;
                for (const size_t t : group)
                {
                    const uint32_t vtx[3] = { mesh.m_indices[t * 3 + 0], mesh.m_indices[t * 3 + 1],
                                              mesh.m_indices[t * 3 + 2] };
                    for (int e = 0; e < 3; ++e)
                    {
                        const uint32_t a = vtx[e];
                        const uint32_t b = vtx[(e + 1) % 3];
                        if (dir.find(ekey(b, a)) == dir.end())
                        {
                            const auto it = nextOf.find(a);
                            if (it != nextOf.end() && it->second != b)
                            {
                                return false; // non-manifold boundary vertex
                            }
                            nextOf[a] = b;
                        }
                    }
                }
                if (nextOf.empty())
                {
                    return false;
                }

                // trace boundary loops
                AZStd::vector<AZStd::vector<uint32_t>> loops;
                AZStd::unordered_set<uint32_t> seen;
                for (const auto& kv : nextOf)
                {
                    if (seen.count(kv.first))
                    {
                        continue;
                    }
                    AZStd::vector<uint32_t> loop;
                    uint32_t cur = kv.first;
                    for (size_t g = 0; g <= nextOf.size(); ++g)
                    {
                        if (seen.count(cur))
                        {
                            break;
                        }
                        seen.insert(cur);
                        loop.push_back(cur);
                        const auto it = nextOf.find(cur);
                        if (it == nextOf.end())
                        {
                            return false;
                        }
                        cur = it->second;
                    }
                    if (loop.size() < 3)
                    {
                        return false;
                    }
                    loops.push_back(AZStd::move(loop));
                }

                // plane basis
                AZ::Vector3 nrm = groupNormal.GetNormalizedSafe();
                if (nrm.GetLengthSq() < 0.25f)
                {
                    return false;
                }
                AZ::Vector3 uAxis = (AZStd::abs(nrm.GetX()) < 0.9f) ? AZ::Vector3::CreateAxisX() : AZ::Vector3::CreateAxisY();
                uAxis = uAxis - nrm * uAxis.Dot(nrm);
                if (uAxis.GetLengthSq() < 1e-12f)
                {
                    return false;
                }
                uAxis.Normalize();
                const AZ::Vector3 vAxis = nrm.Cross(uAxis);

                const auto to2 = [&](uint32_t gi) -> V2
                {
                    const AZ::Vector3 p = TriangleMeshPosition(mesh, gi);
                    return V2{ static_cast<double>(p.Dot(uAxis)), static_cast<double>(p.Dot(vAxis)) };
                };

                // Drop only globally-removable vertices (mid-points of straight visible
                // edges). These are collinear on EVERY face that shares them, so removing
                // them here and in the adjacent face stays consistent - no cracks.
                for (AZStd::vector<uint32_t>& loop : loops)
                {
                    AZStd::vector<uint32_t> kept;
                    kept.reserve(loop.size());
                    for (const uint32_t v : loop)
                    {
                        if (removable.find(v) == removable.end())
                        {
                            kept.push_back(v);
                        }
                    }
                    if (kept.size() >= 3)
                    {
                        loop = AZStd::move(kept);
                    }
                    if (loop.size() < 3)
                    {
                        return false;
                    }
                }

                const auto area2 = [&](const AZStd::vector<uint32_t>& loop) -> double
                {
                    double s = 0.0;
                    const size_t mm = loop.size();
                    for (size_t i = 0; i < mm; ++i)
                    {
                        const V2 p = to2(loop[i]);
                        const V2 q = to2(loop[(i + 1) % mm]);
                        s += p.x * q.y - q.x * p.y;
                    }
                    return 0.5 * s;
                };

                // exactly one outer (CCW) loop expected; the rest are holes (CW)
                int outerIdx = -1;
                int outerCount = 0;
                for (size_t i = 0; i < loops.size(); ++i)
                {
                    if (area2(loops[i]) > 0.0)
                    {
                        ++outerCount;
                        outerIdx = static_cast<int>(i);
                    }
                }
                if (outerCount != 1)
                {
                    return false;
                }

                AZStd::vector<V2> outerPts;
                AZStd::vector<uint32_t> outerGid;
                for (const uint32_t g : loops[outerIdx])
                {
                    outerPts.push_back(to2(g));
                    outerGid.push_back(g);
                }

                AZStd::vector<V2> mergedPts;
                AZStd::vector<uint32_t> mergedGid;
                if (loops.size() == 1)
                {
                    mergedPts = outerPts;
                    mergedGid = outerGid;
                }
                else
                {
                    AZStd::vector<AZStd::vector<V2>> holePts;
                    AZStd::vector<AZStd::vector<uint32_t>> holeGid;
                    for (size_t i = 0; i < loops.size(); ++i)
                    {
                        if (static_cast<int>(i) == outerIdx)
                        {
                            continue;
                        }
                        AZStd::vector<V2> hp;
                        AZStd::vector<uint32_t> hg;
                        for (const uint32_t g : loops[i])
                        {
                            hp.push_back(to2(g));
                            hg.push_back(g);
                        }
                        holePts.push_back(AZStd::move(hp));
                        holeGid.push_back(AZStd::move(hg));
                    }
                    if (!BridgeHoles(outerPts, outerGid, holePts, holeGid, mergedPts, mergedGid))
                    {
                        return false;
                    }
                }

                AZStd::vector<AZStd::array<uint32_t, 3>> localTris;
                if (!EarClip(mergedPts, localTris))
                {
                    return false;
                }

                // validate: every triangle CCW and total area matches the polygon
                double polyArea = area2(loops[outerIdx]);
                for (size_t i = 0; i < loops.size(); ++i)
                {
                    if (static_cast<int>(i) != outerIdx)
                    {
                        polyArea += area2(loops[i]); // holes are CW (negative)
                    }
                }
                double triArea = 0.0;
                for (const AZStd::array<uint32_t, 3>& t : localTris)
                {
                    const double ar = 0.5 * Cross2(mergedPts[t[0]], mergedPts[t[1]], mergedPts[t[2]]);
                    if (ar <= 1e-12)
                    {
                        return false; // degenerate / inverted triangle
                    }
                    triArea += ar;
                }
                if (std::abs(triArea - polyArea) > 1e-4 * (std::abs(polyArea) + 1e-6))
                {
                    return false;
                }

                outTris.reserve(localTris.size());
                for (const AZStd::array<uint32_t, 3>& t : localTris)
                {
                    outTris.push_back({ mergedGid[t[0]], mergedGid[t[1]], mergedGid[t[2]] });
                }
                return true;
            }

            // average (area-weighted) normal of a coplanar triangle group
            AZ::Vector3 GroupNormal(const Csg::TriangleMesh& mesh, const AZStd::vector<size_t>& group)
            {
                AZ::Vector3 n = AZ::Vector3::CreateZero();
                for (const size_t t : group)
                {
                    n += TriangleNormal(mesh, t);
                }
                return n.GetNormalizedSafe();
            }

            // replace the contents of the white box mesh with the given triangle mesh
            void RebuildFromTriangleMesh(WhiteBoxMesh& whiteBox, const Csg::TriangleMesh& triangleMesh)
            {
                Clear(whiteBox);

                // Add only the vertices that survive cleanup, on first use.
                AZStd::unordered_map<uint32_t, VertexHandle> handleOf;
                const auto handleFor = [&](uint32_t globalIndex) -> VertexHandle
                {
                    const auto it = handleOf.find(globalIndex);
                    if (it != handleOf.end())
                    {
                        return it->second;
                    }
                    const VertexHandle h = AddVertex(whiteBox, TriangleMeshPosition(triangleMesh, globalIndex));
                    handleOf.emplace(globalIndex, h);
                    return h;
                };

                const AZStd::vector<AZStd::vector<size_t>> groups = GroupCoplanarTriangles(triangleMesh);

                // Coplanar-face cleanup (vertex removal + re-triangulation of merged
                // faces) proved fragile under heavy CSG - it can mis-triangulate the
                // faces a boolean exposes, giving inverted/diagonal geometry and the
                // odd non-manifold. It is disabled until hardened; the rebuild just
                // uses Manifold's (correct) triangulation grouped into faces. Flip the
                // flag to re-enable. The 'if' (not 'if constexpr') keeps the cleanup
                // code compiled/referenced.
                constexpr bool EnableCoplanarCleanup = false;

                AZStd::vector<AZStd::vector<AZStd::array<uint32_t, 3>>> cleaned(groups.size());
                bool allClean = false;
                if (EnableCoplanarCleanup)
                {
                    // All-or-nothing: a vertex removed on one face but kept on a
                    // fallback neighbour would open a crack, so either every group
                    // cleans or none do.
                    const AZStd::unordered_set<uint32_t> removable = ComputeRemovableVertices(triangleMesh, groups);
                    allClean = true;
                    for (size_t gi = 0; gi < groups.size(); ++gi)
                    {
                        if (!CleanCoplanarGroup(
                                triangleMesh, groups[gi], GroupNormal(triangleMesh, groups[gi]), removable, cleaned[gi]))
                        {
                            allClean = false;
                            break;
                        }
                    }
                }

                for (size_t gi = 0; gi < groups.size(); ++gi)
                {
                    AZStd::vector<AZStd::array<uint32_t, 3>> originalTris;
                    if (!allClean)
                    {
                        originalTris.reserve(groups[gi].size());
                        for (const size_t t : groups[gi])
                        {
                            originalTris.push_back({ triangleMesh.m_indices[t * 3 + 0], triangleMesh.m_indices[t * 3 + 1],
                                                     triangleMesh.m_indices[t * 3 + 2] });
                        }
                    }
                    const AZStd::vector<AZStd::array<uint32_t, 3>>& tris = allClean ? cleaned[gi] : originalTris;

                    FaceVertHandlesList faceVertHandlesList;
                    faceVertHandlesList.reserve(tris.size());
                    for (const AZStd::array<uint32_t, 3>& t : tris)
                    {
                        faceVertHandlesList.push_back(
                            FaceVertHandles{ handleFor(t[0]), handleFor(t[1]), handleFor(t[2]) });
                    }

                    const PolygonHandle polygon = AddPolygon(whiteBox, faceVertHandlesList);
                    AZ::Data::AssetId material;
                    UvProjection projection;
                    AZ::u32 smoothing = 0;
                    DecodeFaceKey(triangleMesh.Material(groups[gi].front()), material, projection, smoothing);
                    SetPolygonMaterial(whiteBox, polygon, material);
                    for (const FaceHandle face : polygon.m_faceHandles)
                    {
                        SetFacePaintColor(whiteBox, face, triangleMesh.Color(groups[gi].front()));
                        SetFaceUvProjection(whiteBox, face, projection);
                        SetFaceSmoothingGroups(whiteBox, face, smoothing);
                    }
                }

                CalculateNormals(whiteBox);
                CalculatePlanarUVs(whiteBox);
            }
        } // namespace Detail

        namespace
        {
            // Painted vertex-blend weights keyed by position, so a rebuild can hand them back to the vertices that survive it.
            using BlendByPosition = AZStd::unordered_map<size_t, AZ::Vector3>;

            size_t PositionKey(const AZ::Vector3& position)
            {
                size_t key = 0;
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(position.GetX() * 1e4)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(position.GetY() * 1e4)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(position.GetZ() * 1e4)));
                return key;
            }

            void CaptureBlend(const WhiteBoxMesh& whiteBox, const AZ::Transform& transform, BlendByPosition& blend)
            {
                if (!MeshHasVertexBlend(whiteBox))
                {
                    return;
                }
                for (const auto vertex : MeshVertexHandles(whiteBox))
                {
                    if (VertexBlendPainted(whiteBox, vertex))
                    {
                        blend[PositionKey(transform.TransformPoint(VertexPosition(whiteBox, vertex)))] = VertexBlend(whiteBox, vertex);
                    }
                }
            }

            // Vertices a cut created find nothing here, and blend in from their painted neighbours when drawn.
            void RestoreBlend(WhiteBoxMesh& whiteBox, const BlendByPosition& blend)
            {
                if (blend.empty())
                {
                    return;
                }
                for (const auto vertex : MeshVertexHandles(whiteBox))
                {
                    if (const auto found = blend.find(PositionKey(VertexPosition(whiteBox, vertex))); found != blend.end())
                    {
                        SetVertexBlend(whiteBox, vertex, found->second);
                    }
                }
            }
        } // namespace

        bool ApplyMeshBoolean(
            WhiteBoxMesh& whiteBox, const WhiteBoxMesh& operand, const AZ::Transform& operandTransform,
            const BooleanOperation operation, const CsgSolver solver)
        {
            const auto csgOperation = [operation]
            {
                switch (operation)
                {
                case BooleanOperation::Union:
                    return Csg::BooleanOperation::Union;
                case BooleanOperation::Intersection:
                    return Csg::BooleanOperation::Intersection;
                case BooleanOperation::Subtraction:
                default:
                    return Csg::BooleanOperation::Subtraction;
                }
            }();

            const Csg::TriangleMesh meshA = Detail::ToTriangleMesh(whiteBox, AZ::Transform::CreateIdentity());
            const Csg::TriangleMesh meshB = Detail::ToTriangleMesh(operand, operandTransform);
            BlendByPosition blend;
            CaptureBlend(operand, operandTransform, blend);
            CaptureBlend(whiteBox, AZ::Transform::CreateIdentity(), blend); // the target's own paint wins where both meet

            Csg::TriangleMesh result;

            // Fast (BSP) composes inward-facing shells like Blender's Fast solver. Either solver falls
            // back to the other when it returns nothing.
            const bool preferBsp = (solver == CsgSolver::Fast);
            const bool primaryOk = preferBsp ? Csg::MeshBooleanBsp(meshA, meshB, csgOperation, result)
                                             : Csg::MeshBoolean(meshA, meshB, csgOperation, result);
            if (!primaryOk)
            {
                const bool fallbackOk = preferBsp ? Csg::MeshBoolean(meshA, meshB, csgOperation, result)
                                                  : Csg::MeshBooleanBsp(meshA, meshB, csgOperation, result);
                if (!fallbackOk)
                {
                    return false;
                }
            }
            Detail::RebuildFromTriangleMesh(whiteBox, result);
            RestoreBlend(whiteBox, blend);

            return true;
        }

        bool RepairMesh(WhiteBoxMesh& whiteBox)
        {
            // Round-trip through a triangle soup, exactly as the boolean does with its inputs.
            const Csg::TriangleMesh soup = Detail::ToTriangleMesh(whiteBox, AZ::Transform::CreateIdentity());
            if (soup.m_indices.empty())
            {
                return false;
            }
            BlendByPosition blend;
            CaptureBlend(whiteBox, AZ::Transform::CreateIdentity(), blend);
            Detail::RebuildFromTriangleMesh(whiteBox, soup);
            RestoreBlend(whiteBox, blend);
            return true;
        }

        bool BuildFromTriangles(
            WhiteBoxMesh& whiteBox, const AZStd::vector<AZ::Vector3>& positions, const AZStd::vector<AZ::u32>& indices,
            const AZStd::vector<AZ::Data::AssetId>& triangleMaterials)
        {
            Csg::TriangleMesh soup;
            soup.m_positions.reserve(positions.size() * 3);
            for (const AZ::Vector3& position : positions)
            {
                soup.m_positions.push_back(position.GetX());
                soup.m_positions.push_back(position.GetY());
                soup.m_positions.push_back(position.GetZ());
            }
            const size_t triangleCount = indices.size() / 3;
            soup.m_indices.reserve(triangleCount * 3);
            for (size_t triangle = 0; triangle < triangleCount; ++triangle)
            {
                const AZ::u32 a = indices[triangle * 3 + 0];
                const AZ::u32 b = indices[triangle * 3 + 1];
                const AZ::u32 c = indices[triangle * 3 + 2];
                if (a >= positions.size() || b >= positions.size() || c >= positions.size() || a == b || b == c || a == c)
                {
                    continue;
                }
                soup.m_indices.insert(soup.m_indices.end(), { a, b, c });
                const AZ::Data::AssetId material = triangle < triangleMaterials.size() ? triangleMaterials[triangle] : AZ::Data::AssetId{};
                soup.m_materials.emplace_back(EncodeFaceKey(material, UvProjection{}, 0));
            }
            // Seams split for UVs or normals in the source weld back into one surface.
            Csg::WeldVertices(soup, WeldTolerance);
            if (soup.m_indices.empty())
            {
                return false;
            }
            Detail::RebuildFromTriangleMesh(whiteBox, soup);
            return true;
        }

        bool BuildColliderTriangles(
            const WhiteBoxMesh& whiteBox, AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices)
        {
            vertices.clear();
            indices.clear();

            // Weld into an indexed triangle soup, then group coplanar-adjacent triangles into regions.
            const Csg::TriangleMesh triangleMesh = Detail::ToTriangleMesh(whiteBox, AZ::Transform::CreateIdentity());
            if (triangleMesh.m_indices.empty())
            {
                return false;
            }
            const AZStd::vector<AZStd::vector<size_t>> groups = Detail::GroupCoplanarTriangles(triangleMesh);
            const AZStd::unordered_set<uint32_t> removable = Detail::ComputeRemovableVertices(triangleMesh, groups);

            // Emit vertices lazily so the collider buffer only holds the vertices actually used.
            AZStd::unordered_map<uint32_t, AZ::u32> remap;
            const auto emitVertex = [&](uint32_t globalIndex) -> AZ::u32
            {
                const auto it = remap.find(globalIndex);
                if (it != remap.end())
                {
                    return it->second;
                }
                const AZ::u32 index = static_cast<AZ::u32>(vertices.size());
                vertices.push_back(Detail::TriangleMeshPosition(triangleMesh, globalIndex));
                remap.emplace(globalIndex, index);
                return index;
            };

            for (const AZStd::vector<size_t>& group : groups)
            {
                // Re-triangulate the coplanar region with collinear edge vertices removed; on any
                // failure keep the region's original triangles (never worse than a per-face cook).
                AZStd::vector<AZStd::array<uint32_t, 3>> tris;
                if (!Detail::CleanCoplanarGroup(
                        triangleMesh, group, Detail::GroupNormal(triangleMesh, group), removable, tris))
                {
                    tris.clear();
                    tris.reserve(group.size());
                    for (const size_t t : group)
                    {
                        tris.push_back(
                            { triangleMesh.m_indices[t * 3 + 0], triangleMesh.m_indices[t * 3 + 1],
                              triangleMesh.m_indices[t * 3 + 2] });
                    }
                }

                for (const AZStd::array<uint32_t, 3>& t : tris)
                {
                    const AZ::u32 a = emitVertex(t[0]);
                    const AZ::u32 b = emitVertex(t[1]);
                    const AZ::u32 c = emitVertex(t[2]);
                    if (a == b || b == c || a == c)
                    {
                        continue; // drop degenerate triangles
                    }
                    indices.push_back(a);
                    indices.push_back(b);
                    indices.push_back(c);
                }
            }

            return !indices.empty();
        }
    } // namespace Api
} // namespace WhiteBox
