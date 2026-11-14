/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxUvOps.h"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/sort.h>

#include <cmath>
#include <numeric>

namespace WhiteBox::UvOps
{
    namespace
    {
        struct Corner
        {
            Api::HalfedgeHandle m_halfedge;
            AZ::Vector2 m_uv;
        };
        using Island = AZStd::vector<Corner>;
        using Triangle2 = AZStd::array<AZ::Vector2, 3>;

        // True when the triangles' interiors meet; touching along an edge or at a corner does not count.
        bool Overlap(const Triangle2& a, const Triangle2& b, const float tolerance)
        {
            for (const Triangle2* shape : { &a, &b })
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    const AZ::Vector2 edge = (*shape)[(i + 1) % 3] - (*shape)[i];
                    const AZ::Vector2 axis(-edge.GetY(), edge.GetX());
                    if (axis.GetLengthSq() <= 1e-20f)
                    {
                        continue;
                    }
                    float minA = AZ::Constants::FloatMax, maxA = -AZ::Constants::FloatMax;
                    float minB = AZ::Constants::FloatMax, maxB = -AZ::Constants::FloatMax;
                    for (size_t c = 0; c < 3; ++c)
                    {
                        const float pa = axis.Dot(a[c]);
                        const float pb = axis.Dot(b[c]);
                        minA = AZ::GetMin(minA, pa);
                        maxA = AZ::GetMax(maxA, pa);
                        minB = AZ::GetMin(minB, pb);
                        maxB = AZ::GetMax(maxB, pb);
                    }
                    const float slack = tolerance * axis.GetLength();
                    if (maxA <= minB + slack || maxB <= minA + slack)
                    {
                        return false; // a separating axis
                    }
                }
            }
            return true;
        }

        // A rotation then a translation in the plane: how a placed polygon's local coordinates land in the layout.
        struct Rigid
        {
            float m_cos = 1.0f;
            float m_sin = 0.0f;
            AZ::Vector2 m_offset = AZ::Vector2::CreateZero();

            AZ::Vector2 operator()(const AZ::Vector2& p) const
            {
                return AZ::Vector2(p.GetX() * m_cos - p.GetY() * m_sin, p.GetX() * m_sin + p.GetY() * m_cos) + m_offset;
            }

            // The motion taking segment (a, b) onto (to0, to1); both are the same 3D edge, so their lengths agree.
            static Rigid Matching(const AZ::Vector2& a, const AZ::Vector2& b, const AZ::Vector2& to0, const AZ::Vector2& to1)
            {
                const AZ::Vector2 from = b - a;
                const AZ::Vector2 to = to1 - to0;
                const float angle = std::atan2(to.GetY(), to.GetX()) - std::atan2(from.GetY(), from.GetX());
                Rigid rigid;
                rigid.m_cos = std::cos(angle);
                rigid.m_sin = std::sin(angle);
                rigid.m_offset = AZ::Vector2::CreateZero();
                rigid.m_offset = to0 - rigid(a);
                return rigid;
            }
        };

        // Lay islands out in shelves inside the unit square, one scale for all so their relative sizes survive.
        void PackIslands(AZStd::vector<Island>& islands, const float margin)
        {
            struct Box
            {
                AZ::Vector2 m_size;
                AZ::Vector2 m_position;
            };
            AZStd::vector<Box> boxes(islands.size());
            float area = 0.0f;
            float widest = 0.0f;
            for (size_t i = 0; i < islands.size(); ++i)
            {
                Island& island = islands[i];
                const auto bounds = [&island]()
                {
                    AZ::Vector2 low(AZ::Constants::FloatMax, AZ::Constants::FloatMax);
                    AZ::Vector2 high(-AZ::Constants::FloatMax, -AZ::Constants::FloatMax);
                    for (const Corner& corner : island)
                    {
                        low = low.GetMin(corner.m_uv);
                        high = high.GetMax(corner.m_uv);
                    }
                    return AZStd::make_pair(low, high);
                };
                AZ::Vector2 low = bounds().first;
                AZ::Vector2 high = bounds().second;
                // Wide shelves pack tighter, so a tall island is turned a quarter turn (a rotation, never a mirror).
                if (high.GetY() - low.GetY() > (high.GetX() - low.GetX()) * 1.001f)
                {
                    for (Corner& corner : island)
                    {
                        corner.m_uv = AZ::Vector2(-corner.m_uv.GetY(), corner.m_uv.GetX());
                    }
                    low = bounds().first;
                    high = bounds().second;
                }
                for (Corner& corner : island)
                {
                    corner.m_uv -= low;
                }
                boxes[i].m_size = (high - low).GetMax(AZ::Vector2(1e-6f, 1e-6f));
                area += boxes[i].m_size.GetX() * boxes[i].m_size.GetY();
                widest = AZ::GetMax(widest, boxes[i].m_size.GetX());
            }
            if (islands.empty())
            {
                return;
            }

            AZStd::vector<size_t> order(islands.size());
            std::iota(order.begin(), order.end(), size_t(0));
            AZStd::sort(order.begin(), order.end(), [&boxes](size_t a, size_t b) { return boxes[a].m_size.GetY() > boxes[b].m_size.GetY(); });

            // Grow the shelf width until the layout is about as tall as it is wide.
            float width = AZ::GetMax(std::sqrt(area), widest);
            float height = 0.0f;
            for (int attempt = 0; attempt < 64; ++attempt)
            {
                const float gap = margin * width;
                float x = 0.0f;
                float y = 0.0f;
                float shelf = 0.0f;
                for (const size_t index : order)
                {
                    Box& box = boxes[index];
                    if (x > 0.0f && x + box.m_size.GetX() > width)
                    {
                        y += shelf + gap;
                        x = 0.0f;
                        shelf = 0.0f;
                    }
                    box.m_position = AZ::Vector2(x, y);
                    x += box.m_size.GetX() + gap;
                    shelf = AZ::GetMax(shelf, box.m_size.GetY());
                }
                height = y + shelf;
                if (height <= width * 1.05f)
                {
                    break;
                }
                width *= 1.1f;
            }

            const float side = AZ::GetMax(width, height);
            const float scale = (1.0f - 2.0f * margin) / side;
            for (size_t i = 0; i < islands.size(); ++i)
            {
                for (Corner& corner : islands[i])
                {
                    corner.m_uv = (corner.m_uv + boxes[i].m_position) * scale + AZ::Vector2(margin, margin);
                }
            }
        }

        AZStd::vector<UvChange> ToChanges(const AZStd::vector<Island>& islands)
        {
            AZStd::vector<UvChange> changes;
            for (const Island& island : islands)
            {
                for (const Corner& corner : island)
                {
                    changes.emplace_back(corner.m_halfedge, corner.m_uv);
                }
            }
            return changes;
        }

        // The faces that still exist, once each.
        Api::FaceHandles LiveFaces(const WhiteBoxMesh& whiteBox, const Api::FaceHandles& faces)
        {
            const auto count = static_cast<int>(Api::MeshFaceCount(whiteBox));
            AZStd::unordered_set<int> seen;
            Api::FaceHandles live;
            for (const auto face : faces)
            {
                if (face.IsValid() && face.Index() < count && seen.insert(face.Index()).second)
                {
                    live.push_back(face);
                }
            }
            return live;
        }
    } // namespace

    AZStd::vector<UvChange> Unwrap(const WhiteBoxMesh& whiteBox, const Api::FaceHandles& inputFaces, const float margin)
    {
        const Api::FaceHandles faces = LiveFaces(whiteBox, inputFaces);

        // The faces grouped by polygon; a polygon is flat, so it unfolds without distortion.
        struct Polygon
        {
            Api::FaceHandles m_faces;
            AZ::Vector3 m_origin = AZ::Vector3::CreateZero();
            AZ::Vector3 m_u = AZ::Vector3::CreateAxisX();
            AZ::Vector3 m_v = AZ::Vector3::CreateAxisY();
            AZ::Vector3 m_normal = AZ::Vector3::CreateAxisZ();
            float m_area = 0.0f;
            int m_island = -1;
            Rigid m_placement;

            // In-plane coordinates; V is negated so a face seen from outside is not mirrored in a V-down texture.
            AZ::Vector2 Local(const AZ::Vector3& p) const
            {
                const AZ::Vector3 d = p - m_origin;
                return AZ::Vector2(d.Dot(m_u), -d.Dot(m_v));
            }
        };
        AZStd::vector<Polygon> polygons;
        AZStd::unordered_map<int, size_t> polygonOfFace;
        for (const auto face : faces)
        {
            if (polygonOfFace.find(face.Index()) != polygonOfFace.end())
            {
                continue;
            }
            const size_t index = polygons.size();
            polygons.emplace_back();
            for (const auto member : Api::FacePolygonHandle(whiteBox, face).m_faceHandles)
            {
                // Only the requested faces of the polygon take part.
                if (AZStd::find(faces.begin(), faces.end(), member) != faces.end() &&
                    polygonOfFace.emplace(member.Index(), index).second)
                {
                    polygons[index].m_faces.push_back(member);
                }
            }
        }

        const auto position = [&whiteBox](const Api::HalfedgeHandle h)
        {
            return Api::VertexPosition(whiteBox, Api::HalfedgeVertexHandleAtTip(whiteBox, h));
        };
        for (Polygon& polygon : polygons)
        {
            AZ::Vector3 normal = AZ::Vector3::CreateZero();
            float longest = -1.0f;
            for (const auto face : polygon.m_faces)
            {
                const auto h = Api::FaceHalfedgeHandles(whiteBox, face);
                const AZ::Vector3 p[3] = { position(h[0]), position(h[1]), position(h[2]) };
                const AZ::Vector3 cross = (p[1] - p[0]).Cross(p[2] - p[0]);
                normal += cross;
                polygon.m_area += 0.5f * cross.GetLength();
                for (size_t i = 0; i < 3; ++i)
                {
                    const AZ::Vector3 edge = p[(i + 1) % 3] - p[i];
                    if (edge.GetLengthSq() > longest)
                    {
                        longest = edge.GetLengthSq();
                        polygon.m_origin = p[i];
                        polygon.m_u = edge;
                    }
                }
            }
            polygon.m_normal = normal.GetNormalizedSafe();
            // U along the polygon's longest edge, flattened into its plane; V completes a right-handed frame.
            polygon.m_u = (polygon.m_u - polygon.m_normal * polygon.m_u.Dot(polygon.m_normal)).GetNormalizedSafe();
            polygon.m_v = polygon.m_normal.Cross(polygon.m_u);
        }

        // Hinges: polygon edges shared by two requested polygons.
        struct Hinge
        {
            size_t m_from = 0;
            size_t m_to = 0;
            Api::HalfedgeHandle m_halfedge; //!< On the from side.
            float m_angle = 0.0f;
        };
        AZStd::vector<Hinge> hinges;
        for (size_t p = 0; p < polygons.size(); ++p)
        {
            for (const auto face : polygons[p].m_faces)
            {
                for (const auto h : Api::FaceHalfedgeHandles(whiteBox, face))
                {
                    const auto other = Api::HalfedgeOppositeFaceHandle(whiteBox, h);
                    const auto found = other.IsValid() ? polygonOfFace.find(other.Index()) : polygonOfFace.end();
                    if (found != polygonOfFace.end() && found->second != p)
                    {
                        const float cosine = AZ::GetClamp(polygons[p].m_normal.Dot(polygons[found->second].m_normal), -1.0f, 1.0f);
                        hinges.push_back({ p, found->second, h, std::acos(cosine) });
                    }
                }
            }
        }

        // The placed triangles of each island, for the overlap test.
        AZStd::vector<AZStd::vector<Triangle2>> islandTriangles;
        const auto placedTriangles = [&](const Polygon& polygon, const Rigid& placement)
        {
            AZStd::vector<Triangle2> triangles;
            for (const auto face : polygon.m_faces)
            {
                const auto h = Api::FaceHalfedgeHandles(whiteBox, face);
                triangles.push_back({ { placement(polygon.Local(position(h[0]))), placement(polygon.Local(position(h[1]))),
                                        placement(polygon.Local(position(h[2]))) } });
            }
            return triangles;
        };
        float extent = 0.0f;
        for (const Polygon& polygon : polygons)
        {
            extent = AZ::GetMax(extent, std::sqrt(polygon.m_area));
        }
        const float tolerance = AZ::GetMax(extent, 1e-3f) * 1e-4f;

        // Largest polygon first as each island's root, then grow across the flattest hinge that fits.
        AZStd::vector<size_t> byArea(polygons.size());
        std::iota(byArea.begin(), byArea.end(), size_t(0));
        AZStd::sort(byArea.begin(), byArea.end(), [&polygons](size_t a, size_t b) { return polygons[a].m_area > polygons[b].m_area; });
        for (const size_t root : byArea)
        {
            if (polygons[root].m_island >= 0)
            {
                continue;
            }
            const int island = static_cast<int>(islandTriangles.size());
            polygons[root].m_island = island;
            polygons[root].m_placement = Rigid{};
            islandTriangles.push_back(placedTriangles(polygons[root], polygons[root].m_placement));
            AZStd::unordered_set<size_t> refused;
            for (;;)
            {
                const Hinge* best = nullptr;
                for (const Hinge& hinge : hinges)
                {
                    if (polygons[hinge.m_from].m_island == island && polygons[hinge.m_to].m_island < 0 &&
                        refused.find(hinge.m_to) == refused.end() && (best == nullptr || hinge.m_angle < best->m_angle))
                    {
                        best = &hinge;
                    }
                }
                if (best == nullptr)
                {
                    break;
                }
                const Polygon& from = polygons[best->m_from];
                Polygon& to = polygons[best->m_to];
                const AZ::Vector3 a = Api::VertexPosition(whiteBox, Api::HalfedgeVertexHandleAtTail(whiteBox, best->m_halfedge));
                const AZ::Vector3 b = position(best->m_halfedge);
                const Rigid placement =
                    Rigid::Matching(to.Local(a), to.Local(b), from.m_placement(from.Local(a)), from.m_placement(from.Local(b)));
                const AZStd::vector<Triangle2> triangles = placedTriangles(to, placement);
                bool overlaps = false;
                for (const Triangle2& triangle : triangles)
                {
                    for (const Triangle2& placed : islandTriangles[island])
                    {
                        if (Overlap(triangle, placed, tolerance))
                        {
                            overlaps = true;
                            break;
                        }
                    }
                    if (overlaps)
                    {
                        break;
                    }
                }
                if (overlaps)
                {
                    refused.insert(best->m_to); // it will root, or join, another island
                    continue;
                }
                to.m_island = island;
                to.m_placement = placement;
                islandTriangles[island].insert(islandTriangles[island].end(), triangles.begin(), triangles.end());
            }
        }

        AZStd::vector<Island> islands(islandTriangles.size());
        for (const Polygon& polygon : polygons)
        {
            for (const auto face : polygon.m_faces)
            {
                for (const auto h : Api::FaceHalfedgeHandles(whiteBox, face))
                {
                    islands[polygon.m_island].push_back({ h, polygon.m_placement(polygon.Local(position(h))) });
                }
            }
        }
        PackIslands(islands, margin);
        return ToChanges(islands);
    }

    namespace
    {
        // The faces' current UV islands: faces meeting at a corner with the same vertex and UV are one island.
        AZStd::vector<Island> CurrentIslands(const WhiteBoxMesh& whiteBox, const Api::FaceHandles& faces);
    } // namespace

    AZStd::vector<UvChange> Pack(const WhiteBoxMesh& whiteBox, const Api::FaceHandles& inputFaces, const float margin)
    {
        AZStd::vector<Island> islands = CurrentIslands(whiteBox, LiveFaces(whiteBox, inputFaces));
        PackIslands(islands, margin);
        return ToChanges(islands);
    }

    AZStd::vector<UvChange> FitToBand(
        const WhiteBoxMesh& whiteBox, const Api::FaceHandles& inputFaces, const float v0, const float v1, const float inset)
    {
        const float top = AZStd::min(v0, v1) + inset;
        const float height = AZStd::abs(v1 - v0) - 2.0f * inset;
        if (height <= 1e-6f)
        {
            return {};
        }
        AZStd::vector<Island> islands = CurrentIslands(whiteBox, LiveFaces(whiteBox, inputFaces));
        for (Island& island : islands)
        {
            AZ::Vector2 low(AZ::Constants::FloatMax);
            AZ::Vector2 high(-AZ::Constants::FloatMax);
            for (const Corner& corner : island)
            {
                low = low.GetMin(corner.m_uv);
                high = high.GetMax(corner.m_uv);
            }
            AZ::Vector2 size = high - low;
            // Trims are horizontal strips: a tall island is turned a quarter so its long side follows U.
            if (size.GetY() > size.GetX())
            {
                for (Corner& corner : island)
                {
                    const AZ::Vector2 local = corner.m_uv - low;
                    corner.m_uv = AZ::Vector2(size.GetY() - local.GetY(), local.GetX());
                }
                low = AZ::Vector2::CreateZero();
                size = AZ::Vector2(size.GetY(), size.GetX());
            }
            const float scale = size.GetY() > 1e-6f ? height / size.GetY() : 1.0f;
            for (Corner& corner : island)
            {
                corner.m_uv = (corner.m_uv - low) * scale + AZ::Vector2(0.0f, top);
            }
        }
        return ToChanges(islands);
    }

    namespace
    {
    AZStd::vector<Island> CurrentIslands(const WhiteBoxMesh& whiteBox, const Api::FaceHandles& faces)
    {
        // Faces meeting at a corner with the same vertex and UV are one island.
        AZStd::vector<size_t> parent(faces.size());
        std::iota(parent.begin(), parent.end(), size_t(0));
        const auto find = [&parent](size_t i)
        {
            while (parent[i] != i)
            {
                parent[i] = parent[parent[i]];
                i = parent[i];
            }
            return i;
        };
        AZStd::unordered_map<size_t, size_t> firstFaceAtCorner;
        for (size_t f = 0; f < faces.size(); ++f)
        {
            for (const auto h : Api::FaceHalfedgeHandles(whiteBox, faces[f]))
            {
                const AZ::Vector2 uv = Api::HalfedgeUV(whiteBox, h);
                size_t key = 0;
                AZStd::hash_combine(key, Api::HalfedgeVertexHandleAtTip(whiteBox, h).Index());
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(uv.GetX() * 1e5)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(uv.GetY() * 1e5)));
                const auto [slot, fresh] = firstFaceAtCorner.emplace(key, f);
                if (!fresh)
                {
                    parent[find(f)] = find(slot->second);
                }
            }
        }
        AZStd::unordered_map<size_t, size_t> islandOfRoot;
        AZStd::vector<Island> islands;
        for (size_t f = 0; f < faces.size(); ++f)
        {
            const auto [slot, fresh] = islandOfRoot.emplace(find(f), islands.size());
            if (fresh)
            {
                islands.emplace_back();
            }
            for (const auto h : Api::FaceHalfedgeHandles(whiteBox, faces[f]))
            {
                islands[slot->second].push_back({ h, Api::HalfedgeUV(whiteBox, h) });
            }
        }
        return islands;
    }
    } // namespace
} // namespace WhiteBox::UvOps
