/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Rendering/WhiteBoxLightmapUv.h>

#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/sort.h>

#include <cmath>
#include <numeric>

namespace WhiteBox
{
    namespace
    {
        // Corners closer than this are the same point when finding which triangles share an edge.
        constexpr float WeldScale = 1e4f;
        // Normals this close (cosine) count as one plane.
        constexpr float CoplanarCosine = 0.9999f;

        struct PointKey
        {
            AZ::s64 m_x, m_y, m_z;
            bool operator==(const PointKey& other) const
            {
                return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
            }
        };

        struct PointKeyHash
        {
            size_t operator()(const PointKey& key) const
            {
                size_t seed = 0;
                AZStd::hash_combine(seed, key.m_x);
                AZStd::hash_combine(seed, key.m_y);
                AZStd::hash_combine(seed, key.m_z);
                return seed;
            }
        };

        PointKey KeyOf(const AZ::Vector3& point)
        {
            return { std::llround(point.GetX() * WeldScale), std::llround(point.GetY() * WeldScale), std::llround(point.GetZ() * WeldScale) };
        }

        struct Chart
        {
            AZStd::vector<size_t> m_faces;
            AZ::Vector3 m_u; //!< Chart plane axes, u along its longest edge.
            AZ::Vector3 m_v;
            AZ::Vector2 m_min; //!< Bounds of the projected corners, world units.
            AZ::Vector2 m_size;
            bool m_turned = false; //!< Laid on its side, so shelves stay low.
            AZ::Vector2 m_offset;  //!< Where the chart's corner lands in the unit square.
        };

        // Shelf packing, tallest first; true when every chart fits the unit square at this scale.
        bool Place(AZStd::vector<Chart*>& charts, const float scale, const float margin)
        {
            float x = margin;
            float y = margin;
            float shelf = 0.0f;
            for (Chart* chart : charts)
            {
                const float w = chart->m_size.GetX() * scale;
                const float h = chart->m_size.GetY() * scale;
                if (x + w + margin > 1.0f && x > margin)
                {
                    y += shelf + margin;
                    x = margin;
                    shelf = 0.0f;
                }
                if (x + w + margin > 1.0f || y + h + margin > 1.0f)
                {
                    return false;
                }
                chart->m_offset = AZ::Vector2(x, y);
                x += w + margin;
                shelf = AZStd::max(shelf, h);
            }
            return true;
        }
    } // namespace

    void GenerateLightmapUvs(WhiteBoxFaces& faces, const float margin)
    {
        const size_t count = faces.size();
        if (count == 0)
        {
            return;
        }

        // Charts: triangles joined across a shared edge when they lie in the same plane.
        AZStd::vector<size_t> parent(count);
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
        AZStd::unordered_map<PointKey, AZ::u32, PointKeyHash> pointIds;
        const auto pointId = [&pointIds](const AZ::Vector3& point)
        {
            return pointIds.emplace(KeyOf(point), static_cast<AZ::u32>(pointIds.size())).first->second;
        };
        AZStd::unordered_map<AZ::u64, size_t> edgeFace;
        for (size_t f = 0; f < count; ++f)
        {
            const WhiteBoxFace& face = faces[f];
            const AZ::u32 ids[3] = { pointId(face.m_v1.m_position), pointId(face.m_v2.m_position), pointId(face.m_v3.m_position) };
            for (int e = 0; e < 3; ++e)
            {
                const AZ::u32 a = AZStd::min(ids[e], ids[(e + 1) % 3]);
                const AZ::u32 b = AZStd::max(ids[e], ids[(e + 1) % 3]);
                const AZ::u64 edge = (static_cast<AZ::u64>(a) << 32) | b;
                const auto [slot, fresh] = edgeFace.emplace(edge, f);
                if (!fresh && faces[slot->second].m_normal.GetNormalizedSafe().Dot(face.m_normal.GetNormalizedSafe()) > CoplanarCosine)
                {
                    parent[find(f)] = find(slot->second);
                }
            }
        }
        AZStd::unordered_map<size_t, size_t> chartOfRoot;
        AZStd::vector<Chart> charts;
        for (size_t f = 0; f < count; ++f)
        {
            const auto [slot, fresh] = chartOfRoot.emplace(find(f), charts.size());
            if (fresh)
            {
                charts.emplace_back();
            }
            charts[slot->second].m_faces.push_back(f);
        }

        // Flatten each chart onto its plane, u along its longest edge so rectangular walls pack tightly.
        double area = 0.0;
        for (Chart& chart : charts)
        {
            const AZ::Vector3 normal = faces[chart.m_faces.front()].m_normal.GetNormalizedSafe();
            AZ::Vector3 longest = AZ::Vector3::CreateZero();
            for (const size_t f : chart.m_faces)
            {
                const WhiteBoxFace& face = faces[f];
                for (const AZ::Vector3& edge : { face.m_v2.m_position - face.m_v1.m_position, face.m_v3.m_position - face.m_v2.m_position,
                                                 face.m_v1.m_position - face.m_v3.m_position })
                {
                    if (edge.GetLengthSq() > longest.GetLengthSq())
                    {
                        longest = edge;
                    }
                }
            }
            chart.m_u = (longest - normal * normal.Dot(longest)).GetNormalizedSafe();
            if (chart.m_u.IsZero())
            {
                chart.m_u = normal.GetOrthogonalVector().GetNormalizedSafe();
            }
            chart.m_v = normal.Cross(chart.m_u).GetNormalizedSafe();
            AZ::Vector2 low(AZ::Constants::FloatMax, AZ::Constants::FloatMax);
            AZ::Vector2 high(-AZ::Constants::FloatMax, -AZ::Constants::FloatMax);
            for (const size_t f : chart.m_faces)
            {
                for (const WhiteBoxVertex* vertex : { &faces[f].m_v1, &faces[f].m_v2, &faces[f].m_v3 })
                {
                    const AZ::Vector2 p(chart.m_u.Dot(vertex->m_position), chart.m_v.Dot(vertex->m_position));
                    low = low.GetMin(p);
                    high = high.GetMax(p);
                }
            }
            chart.m_min = low;
            chart.m_size = (high - low).GetMax(AZ::Vector2(1e-4f));
            chart.m_turned = chart.m_size.GetY() > chart.m_size.GetX();
            if (chart.m_turned)
            {
                chart.m_size = AZ::Vector2(chart.m_size.GetY(), chart.m_size.GetX());
            }
            area += double(chart.m_size.GetX()) * double(chart.m_size.GetY());
        }

        // Many charts need thinner gaps, or the gaps alone would fill the square.
        const float gap = AZStd::min(margin, 0.25f / std::sqrt(static_cast<float>(charts.size())));
        AZStd::vector<Chart*> order;
        order.reserve(charts.size());
        for (Chart& chart : charts)
        {
            order.push_back(&chart);
        }
        AZStd::sort(order.begin(), order.end(), [](const Chart* a, const Chart* b) { return a->m_size.GetY() > b->m_size.GetY(); });

        // Start from the scale that would fill most of the square, shrink until it fits, then grow while it still does.
        float scale = static_cast<float>(std::sqrt(0.7 / AZStd::max(area, 1e-12)));
        for (int i = 0; i < 200 && !Place(order, scale, gap); ++i)
        {
            scale *= 0.95f;
        }
        for (int i = 0; i < 60 && Place(order, scale * 1.02f, gap); ++i)
        {
            scale *= 1.02f;
        }
        if (!Place(order, scale, gap))
        {
            return; // nothing sensible fits; leave the faces on their texture UVs
        }

        for (const Chart& chart : charts)
        {
            for (const size_t f : chart.m_faces)
            {
                WhiteBoxFace& face = faces[f];
                for (WhiteBoxVertex* vertex : { &face.m_v1, &face.m_v2, &face.m_v3 })
                {
                    AZ::Vector2 local =
                        AZ::Vector2(chart.m_u.Dot(vertex->m_position), chart.m_v.Dot(vertex->m_position)) - chart.m_min;
                    if (chart.m_turned)
                    {
                        local = AZ::Vector2(local.GetY(), local.GetX());
                    }
                    vertex->m_lightmapUv = chart.m_offset + local * scale;
                }
                face.m_hasLightmapUv = true;
            }
        }
    }
} // namespace WhiteBox
