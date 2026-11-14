/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxUvCanvas.h"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/hash.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <cmath>

namespace WhiteBox
{
    namespace
    {
        constexpr double PickRadius = 7.0;
        constexpr double MinPixelsPerUnit = 8.0;
        constexpr double MaxPixelsPerUnit = 200000.0;

        // Qt 5 names the mouse position localPos; Qt 6 renamed it position.
        QPointF EventPosition(const QMouseEvent* event)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            return event->position();
#else
            return event->localPos();
#endif
        }

        size_t CornerKey(const int vertex, const AZ::Vector2& uv)
        {
            size_t key = 0;
            AZStd::hash_combine(key, vertex);
            AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(uv.GetX() * 1e5)));
            AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(uv.GetY() * 1e5)));
            return key;
        }
    } // namespace

    UvModel BuildUvModel(const WhiteBoxMesh& whiteBox, const Api::PolygonHandles& polygons, const AZStd::unordered_set<int>& detached)
    {
        UvModel model;
        const auto faceCount = static_cast<int>(Api::MeshFaceCount(whiteBox));
        // Which selected polygon each face belongs to, so internal diagonals can be told from polygon edges.
        AZStd::unordered_map<int, size_t> polygonOf;
        for (size_t p = 0; p < polygons.size(); ++p)
        {
            for (const auto face : polygons[p].m_faceHandles)
            {
                if (face.IsValid() && face.Index() < faceCount)
                {
                    polygonOf.emplace(face.Index(), p);
                    AZStd::hash_combine(model.m_signature, face.Index());
                }
            }
        }

        AZStd::unordered_map<size_t, AZ::u32> vertexOf;
        AZStd::unordered_map<AZ::u64, size_t> edgeOf;
        for (size_t p = 0; p < polygons.size(); ++p)
        {
            for (const auto face : polygons[p].m_faceHandles)
            {
                if (!face.IsValid() || face.Index() >= faceCount)
                {
                    continue;
                }
                const auto halfedges = Api::FaceHalfedgeHandles(whiteBox, face);
                if (halfedges.size() != 3)
                {
                    continue;
                }
                AZStd::array<AZ::u32, 3> corners{};
                for (size_t c = 0; c < 3; ++c)
                {
                    const AZ::Vector2 uv = Api::HalfedgeUV(whiteBox, halfedges[c]);
                    const int tip = Api::HalfedgeVertexHandleAtTip(whiteBox, halfedges[c]).Index();
                    size_t key = CornerKey(tip, uv);
                    if (detached.find(halfedges[c].Index()) != detached.end())
                    {
                        AZStd::hash_combine(key, 0x5eedu); // a separate point from its undetached neighbours
                    }
                    auto [slot, fresh] = vertexOf.emplace(key, static_cast<AZ::u32>(model.m_vertices.size()));
                    if (fresh)
                    {
                        model.m_vertices.push_back({ uv, {}, tip });
                    }
                    model.m_vertices[slot->second].m_corners.push_back(halfedges[c]);
                    corners[c] = slot->second;
                }
                model.m_triangles.push_back(corners);
                model.m_faces.push_back(face);
                model.m_triangleCorners.push_back({ { halfedges[0], halfedges[1], halfedges[2] } });
                // Halfedge c runs from the previous corner to corner c.
                for (size_t c = 0; c < 3; ++c)
                {
                    const AZ::u32 from = corners[(c + 2) % 3];
                    const AZ::u32 to = corners[c];
                    const auto opposite = Api::HalfedgeOppositeFaceHandle(whiteBox, halfedges[c]);
                    const auto neighbour = opposite.IsValid() ? polygonOf.find(opposite.Index()) : polygonOf.end();
                    const bool border = neighbour == polygonOf.end() || neighbour->second != p;
                    const AZ::u64 key = (static_cast<AZ::u64>(AZStd::min(from, to)) << 32) | AZStd::max(from, to);
                    auto [edge, added] = edgeOf.emplace(key, model.m_edges.size());
                    if (added)
                    {
                        model.m_edges.push_back({ from, to, border });
                    }
                    else
                    {
                        model.m_edges[edge->second].m_border = model.m_edges[edge->second].m_border || border;
                    }
                }
            }
        }
        return model;
    }

    WhiteBoxUvCanvas::WhiteBoxUvCanvas(QWidget* parent)
        : QWidget(parent)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(200, 200);
        setToolTip(
            tr("Click or drag a box to select UV points (Shift adds, Ctrl removes), double-click for a whole island. Drag a "
               "selected point to move, rotate or scale; hold Ctrl to snap. Middle mouse or Alt pans, the wheel zooms, F frames, "
               "A selects all."));
    }

    void WhiteBoxUvCanvas::SetModel(UvModel model)
    {
        const bool selectionChanged = model.m_signature != m_model.m_signature;
        m_model = AZStd::move(model);
        if (selectionChanged)
        {
            m_selectedCorners.clear();
            m_detached.clear();
            if (!m_model.m_vertices.empty())
            {
                FrameAll();
            }
        }
        update();
        Report();
    }

    QPointF WhiteBoxUvCanvas::ToScreen(const AZ::Vector2& uv) const
    {
        return m_originOnScreen + QPointF(uv.GetX() * m_pixelsPerUnit, uv.GetY() * m_pixelsPerUnit);
    }

    AZ::Vector2 WhiteBoxUvCanvas::ToUv(const QPointF& screen) const
    {
        const QPointF local = (screen - m_originOnScreen) / m_pixelsPerUnit;
        return AZ::Vector2(static_cast<float>(local.x()), static_cast<float>(local.y()));
    }

    void WhiteBoxUvCanvas::Frame(const AZStd::vector<AZ::u32>& vertices)
    {
        // The unit square is always in view, plus whatever is framed.
        AZ::Vector2 low(0.0f, 0.0f);
        AZ::Vector2 high(1.0f, 1.0f);
        for (const AZ::u32 vertex : vertices)
        {
            low = low.GetMin(m_model.m_vertices[vertex].m_uv);
            high = high.GetMax(m_model.m_vertices[vertex].m_uv);
        }
        const AZ::Vector2 size = (high - low).GetMax(AZ::Vector2(1e-3f, 1e-3f));
        const double margin = 24.0;
        if (width() <= 2.0 * margin || height() <= 2.0 * margin)
        {
            m_framed = false; // not laid out yet; the first paint frames it
            return;
        }
        const double fit = AZStd::min((width() - 2.0 * margin) / size.GetX(), (height() - 2.0 * margin) / size.GetY());
        m_pixelsPerUnit = AZ::GetClamp(fit, MinPixelsPerUnit, MaxPixelsPerUnit);
        const AZ::Vector2 centre = (low + high) * 0.5f;
        m_originOnScreen = QPointF(width() * 0.5, height() * 0.5) - QPointF(centre.GetX() * m_pixelsPerUnit, centre.GetY() * m_pixelsPerUnit);
        m_framed = true;
        update();
    }

    void WhiteBoxUvCanvas::FrameAll()
    {
        AZStd::vector<AZ::u32> all(m_model.m_vertices.size());
        for (AZ::u32 i = 0; i < all.size(); ++i)
        {
            all[i] = i;
        }
        Frame(all);
    }

    void WhiteBoxUvCanvas::FrameSelection()
    {
        Frame(Targets());
    }

    void WhiteBoxUvCanvas::SelectAll()
    {
        for (AZ::u32 i = 0; i < m_model.m_vertices.size(); ++i)
        {
            SetSelected(i, true);
        }
        update();
        Report();
    }

    AZStd::vector<AZStd::vector<AZ::u32>> WhiteBoxUvCanvas::Neighbours(const bool polygonEdgesOnly) const
    {
        AZStd::vector<AZStd::vector<AZ::u32>> neighbours(m_model.m_vertices.size());
        for (const auto& edge : m_model.m_edges)
        {
            if (!polygonEdgesOnly || edge.m_border)
            {
                neighbours[edge.m_from].push_back(edge.m_to);
                neighbours[edge.m_to].push_back(edge.m_from);
            }
        }
        return neighbours;
    }

    AZStd::vector<bool> WhiteBoxUvCanvas::SelectionMask() const
    {
        AZStd::vector<bool> mask(m_model.m_vertices.size(), false);
        for (AZ::u32 i = 0; i < mask.size(); ++i)
        {
            mask[i] = IsSelected(i);
        }
        return mask;
    }

    void WhiteBoxUvCanvas::SelectPoints(const AZStd::vector<bool>& selected)
    {
        m_selectedCorners.clear();
        for (AZ::u32 i = 0; i < selected.size(); ++i)
        {
            if (selected[i])
            {
                SetSelected(i, true);
            }
        }
        update();
        Report();
    }

    void WhiteBoxUvCanvas::SelectNone()
    {
        SelectPoints(AZStd::vector<bool>(m_model.m_vertices.size(), false));
    }

    void WhiteBoxUvCanvas::InvertSelection()
    {
        AZStd::vector<bool> mask = SelectionMask();
        for (AZ::u32 i = 0; i < mask.size(); ++i)
        {
            mask[i] = !mask[i];
        }
        SelectPoints(mask);
    }

    void WhiteBoxUvCanvas::SelectLinked()
    {
        const AZStd::vector<bool> seeds = SelectionMask();
        AZStd::vector<bool> mask = seeds;
        AZStd::vector<bool> covered(seeds.size(), false); // points already reached from an earlier seed's island
        for (AZ::u32 i = 0; i < seeds.size(); ++i)
        {
            if (seeds[i] && !covered[i])
            {
                for (const AZ::u32 point : Island(i))
                {
                    mask[point] = true;
                    covered[point] = true;
                }
            }
        }
        SelectPoints(mask);
    }

    void WhiteBoxUvCanvas::GrowSelection()
    {
        const AZStd::vector<bool> seeds = SelectionMask();
        AZStd::vector<bool> mask = seeds;
        const auto neighbours = Neighbours(false);
        for (AZ::u32 i = 0; i < seeds.size(); ++i)
        {
            if (seeds[i])
            {
                for (const AZ::u32 next : neighbours[i])
                {
                    mask[next] = true;
                }
            }
        }
        SelectPoints(mask);
    }

    void WhiteBoxUvCanvas::ShrinkSelection()
    {
        const AZStd::vector<bool> seeds = SelectionMask();
        AZStd::vector<bool> mask = seeds;
        const auto neighbours = Neighbours(false);
        for (AZ::u32 i = 0; i < seeds.size(); ++i)
        {
            for (const AZ::u32 next : neighbours[i])
            {
                if (seeds[i] && !seeds[next])
                {
                    mask[i] = false; // touches the outside
                }
            }
        }
        SelectPoints(mask);
    }

    bool WhiteBoxUvCanvas::SelectLoop()
    {
        const AZStd::vector<bool> seeds = SelectionMask();
        AZStd::vector<bool> mask = seeds;
        const auto neighbours = Neighbours(true);
        // Straight on means within 45 degrees of the way the loop was already going.
        const float straight = std::cos(AZ::DegToRad(45.0f));
        const auto walk = [&](AZ::u32 from, AZ::u32 at)
        {
            AZStd::vector<bool> visited(m_model.m_vertices.size(), false);
            visited[from] = true;
            while (!visited[at])
            {
                visited[at] = true;
                mask[at] = true;
                const AZ::Vector2 heading = (m_model.m_vertices[at].m_uv - m_model.m_vertices[from].m_uv).GetNormalizedSafe();
                int best = -1;
                float bestDot = straight;
                for (const AZ::u32 next : neighbours[at])
                {
                    if (next == from)
                    {
                        continue;
                    }
                    const float dot = (m_model.m_vertices[next].m_uv - m_model.m_vertices[at].m_uv).GetNormalizedSafe().Dot(heading);
                    if (dot > bestDot)
                    {
                        bestDot = dot;
                        best = static_cast<int>(next);
                    }
                }
                if (best < 0)
                {
                    return;
                }
                from = at;
                at = static_cast<AZ::u32>(best);
            }
        };
        bool found = false;
        for (const auto& edge : m_model.m_edges)
        {
            if (edge.m_border && seeds[edge.m_from] && seeds[edge.m_to])
            {
                found = true;
                walk(edge.m_from, edge.m_to);
                walk(edge.m_to, edge.m_from);
            }
        }
        if (found)
        {
            SelectPoints(mask);
        }
        return found;
    }

    void WhiteBoxUvCanvas::SelectBorder()
    {
        // An edge one triangle uses is on an island's boundary; two means it is inside.
        AZStd::unordered_map<AZ::u64, int> uses;
        const auto key = [](const AZ::u32 a, const AZ::u32 b)
        {
            return (static_cast<AZ::u64>(AZStd::min(a, b)) << 32) | AZStd::max(a, b);
        };
        for (const auto& triangle : m_model.m_triangles)
        {
            for (size_t c = 0; c < 3; ++c)
            {
                ++uses[key(triangle[c], triangle[(c + 1) % 3])];
            }
        }
        // Only the islands already holding a selection, or all of them when nothing is selected.
        const AZStd::vector<bool> seeds = SelectionMask();
        AZStd::vector<bool> inScope(m_model.m_vertices.size(), true);
        if (AZStd::find(seeds.begin(), seeds.end(), true) != seeds.end())
        {
            inScope.assign(m_model.m_vertices.size(), false);
            for (AZ::u32 i = 0; i < seeds.size(); ++i)
            {
                if (seeds[i] && !inScope[i])
                {
                    for (const AZ::u32 point : Island(i))
                    {
                        inScope[point] = true;
                    }
                }
            }
        }
        AZStd::vector<bool> mask(m_model.m_vertices.size(), false);
        for (const auto& entry : uses)
        {
            if (entry.second == 1)
            {
                const auto a = static_cast<AZ::u32>(entry.first >> 32);
                const auto b = static_cast<AZ::u32>(entry.first & 0xffffffffu);
                mask[a] = mask[a] || inScope[a];
                mask[b] = mask[b] || inScope[b];
            }
        }
        SelectPoints(mask);
    }

    Api::FaceHandles WhiteBoxUvCanvas::SelectedFaces() const
    {
        Api::FaceHandles faces;
        for (size_t t = 0; t < m_model.m_triangles.size(); ++t)
        {
            const auto& triangle = m_model.m_triangles[t];
            if (IsSelected(triangle[0]) && IsSelected(triangle[1]) && IsSelected(triangle[2]))
            {
                faces.push_back(m_model.m_faces[t]);
            }
        }
        return faces;
    }

    AZStd::vector<int> WhiteBoxUvCanvas::SelectedMeshVertices() const
    {
        AZStd::unordered_set<int> seen;
        AZStd::vector<int> vertices;
        for (AZ::u32 i = 0; i < m_model.m_vertices.size(); ++i)
        {
            if (IsSelected(i) && seen.insert(m_model.m_vertices[i].m_meshVertex).second)
            {
                vertices.push_back(m_model.m_vertices[i].m_meshVertex);
            }
        }
        return vertices;
    }

    AZStd::vector<AZStd::pair<int, int>> WhiteBoxUvCanvas::SelectedMeshEdges() const
    {
        AZStd::vector<AZStd::pair<int, int>> edges;
        for (const auto& edge : m_model.m_edges)
        {
            if (edge.m_border && IsSelected(edge.m_from) && IsSelected(edge.m_to))
            {
                edges.emplace_back(m_model.m_vertices[edge.m_from].m_meshVertex, m_model.m_vertices[edge.m_to].m_meshVertex);
            }
        }
        return edges;
    }

    void WhiteBoxUvCanvas::SetBackground(const QImage& image)
    {
        if (image.cacheKey() != m_background.cacheKey())
        {
            m_background = image;
            update();
        }
    }

    void WhiteBoxUvCanvas::SetTrimBands(const AZStd::vector<float>& edges, const int highlighted)
    {
        if (edges != m_trimEdges || highlighted != m_trimHighlight)
        {
            m_trimEdges = edges;
            m_trimHighlight = highlighted;
            update();
        }
    }

    void WhiteBoxUvCanvas::SetHoveredFaces(const Api::FaceHandles& faces)
    {
        AZStd::unordered_set<int> hovered;
        for (const auto face : faces)
        {
            hovered.insert(face.Index());
        }
        bool changed = hovered.size() != m_hoveredFaces.size();
        for (auto it = hovered.begin(); !changed && it != hovered.end(); ++it)
        {
            changed = m_hoveredFaces.find(*it) == m_hoveredFaces.end();
        }
        if (changed)
        {
            m_hoveredFaces = AZStd::move(hovered);
            update();
        }
    }

    Api::FaceHandles WhiteBoxUvCanvas::TargetFaces() const
    {
        Api::FaceHandles faces = SelectedFaces();
        return faces.empty() ? m_model.m_faces : faces;
    }

    size_t WhiteBoxUvCanvas::SplitSelectedFaces()
    {
        AZStd::unordered_set<int> corners;
        size_t faces = 0;
        for (size_t t = 0; t < m_model.m_triangles.size(); ++t)
        {
            const auto& triangle = m_model.m_triangles[t];
            if (IsSelected(triangle[0]) && IsSelected(triangle[1]) && IsSelected(triangle[2]))
            {
                ++faces;
                for (const auto corner : m_model.m_triangleCorners[t])
                {
                    corners.insert(corner.Index());
                }
            }
        }
        if (faces == 0)
        {
            return 0;
        }
        // Their corners become their own points; the selection narrows to them, so a drag takes only the split-off faces.
        for (const int corner : corners)
        {
            m_detached.insert(corner);
        }
        m_selectedCorners = AZStd::move(corners);
        return faces;
    }

    size_t WhiteBoxUvCanvas::SewSelected()
    {
        AZStd::unordered_map<int, AZStd::vector<AZ::u32>> pointsAtVertex;
        for (AZ::u32 i = 0; i < m_model.m_vertices.size(); ++i)
        {
            if (IsSelected(i))
            {
                pointsAtVertex[m_model.m_vertices[i].m_meshVertex].push_back(i);
            }
        }
        AZStd::vector<AZ::u32> targets;
        AZStd::vector<AZ::Vector2> uvs;
        for (const auto& entry : pointsAtVertex)
        {
            const auto& points = entry.second;
            if (points.size() < 2)
            {
                continue;
            }
            AZ::Vector2 average = AZ::Vector2::CreateZero();
            for (const AZ::u32 point : points)
            {
                average += m_model.m_vertices[point].m_uv;
            }
            average /= static_cast<float>(points.size());
            for (const AZ::u32 point : points)
            {
                targets.push_back(point);
                uvs.push_back(average);
                // A sewn corner is no longer torn off, or it would stay a separate point at the same spot.
                for (const auto corner : m_model.m_vertices[point].m_corners)
                {
                    m_detached.erase(corner.Index());
                }
            }
        }
        if (!targets.empty())
        {
            Apply(targets, uvs, true);
        }
        return targets.size();
    }

    bool WhiteBoxUvCanvas::IsSelected(const AZ::u32 vertex) const
    {
        for (const auto corner : m_model.m_vertices[vertex].m_corners)
        {
            if (m_selectedCorners.find(corner.Index()) != m_selectedCorners.end())
            {
                return true;
            }
        }
        return false;
    }

    void WhiteBoxUvCanvas::SetSelected(const AZ::u32 vertex, const bool selected)
    {
        for (const auto corner : m_model.m_vertices[vertex].m_corners)
        {
            if (selected)
            {
                m_selectedCorners.insert(corner.Index());
            }
            else
            {
                m_selectedCorners.erase(corner.Index());
            }
        }
    }

    AZStd::vector<AZ::u32> WhiteBoxUvCanvas::Targets() const
    {
        AZStd::vector<AZ::u32> selected;
        for (AZ::u32 i = 0; i < m_model.m_vertices.size(); ++i)
        {
            if (IsSelected(i))
            {
                selected.push_back(i);
            }
        }
        if (selected.empty())
        {
            selected.resize(m_model.m_vertices.size());
            for (AZ::u32 i = 0; i < selected.size(); ++i)
            {
                selected[i] = i;
            }
        }
        return selected;
    }

    AZ::Vector2 WhiteBoxUvCanvas::Centre(const AZStd::vector<AZ::u32>& vertices) const
    {
        // The bounds' centre, so rotating and flipping pivot where the eye expects.
        AZ::Vector2 low(AZ::Constants::FloatMax, AZ::Constants::FloatMax);
        AZ::Vector2 high(-AZ::Constants::FloatMax, -AZ::Constants::FloatMax);
        for (const AZ::u32 vertex : vertices)
        {
            low = low.GetMin(m_model.m_vertices[vertex].m_uv);
            high = high.GetMax(m_model.m_vertices[vertex].m_uv);
        }
        return vertices.empty() ? AZ::Vector2::CreateZero() : (low + high) * 0.5f;
    }

    AZStd::vector<AZ::u32> WhiteBoxUvCanvas::Island(const AZ::u32 start) const
    {
        AZStd::vector<AZStd::vector<AZ::u32>> neighbours(m_model.m_vertices.size());
        for (const auto& triangle : m_model.m_triangles)
        {
            for (size_t c = 0; c < 3; ++c)
            {
                neighbours[triangle[c]].push_back(triangle[(c + 1) % 3]);
                neighbours[triangle[(c + 1) % 3]].push_back(triangle[c]);
            }
        }
        AZStd::vector<bool> seen(m_model.m_vertices.size(), false);
        AZStd::vector<AZ::u32> island{ start };
        seen[start] = true;
        for (size_t i = 0; i < island.size(); ++i)
        {
            for (const AZ::u32 next : neighbours[island[i]])
            {
                if (!seen[next])
                {
                    seen[next] = true;
                    island.push_back(next);
                }
            }
        }
        return island;
    }

    int WhiteBoxUvCanvas::PickVertex(const QPointF& screen) const
    {
        int best = -1;
        double bestDistance = PickRadius * PickRadius;
        for (AZ::u32 i = 0; i < m_model.m_vertices.size(); ++i)
        {
            const QPointF delta = ToScreen(m_model.m_vertices[i].m_uv) - screen;
            const double distance = delta.x() * delta.x() + delta.y() * delta.y();
            // Ties go to a selected point, so a stacked selection can still be dragged.
            if (distance < bestDistance || (distance == bestDistance && best >= 0 && !IsSelected(best) && IsSelected(i)))
            {
                bestDistance = distance;
                best = static_cast<int>(i);
            }
        }
        return best;
    }

    void WhiteBoxUvCanvas::Apply(const AZStd::vector<AZ::u32>& targets, const AZStd::vector<AZ::Vector2>& uvs, const bool final)
    {
        AZStd::vector<UvChange> changes;
        for (size_t i = 0; i < targets.size(); ++i)
        {
            UvModel::Vertex& vertex = m_model.m_vertices[targets[i]];
            vertex.m_uv = uvs[i];
            for (const auto corner : vertex.m_corners)
            {
                changes.emplace_back(corner, uvs[i]);
            }
        }
        update();
        if (m_onEdit && !changes.empty())
        {
            m_onEdit(changes, final);
        }
    }

    void WhiteBoxUvCanvas::TransformTargets(const AZStd::function<AZ::Vector2(const AZ::Vector2&, const AZ::Vector2&)>& transform)
    {
        const AZStd::vector<AZ::u32> targets = Targets();
        const AZ::Vector2 centre = Centre(targets);
        AZStd::vector<AZ::Vector2> uvs;
        uvs.reserve(targets.size());
        for (const AZ::u32 vertex : targets)
        {
            uvs.push_back(transform(m_model.m_vertices[vertex].m_uv, centre));
        }
        Apply(targets, uvs, true);
    }

    void WhiteBoxUvCanvas::FitTargetsToUnitSquare()
    {
        const AZStd::vector<AZ::u32> targets = Targets();
        if (targets.empty())
        {
            return;
        }
        AZ::Vector2 low(AZ::Constants::FloatMax, AZ::Constants::FloatMax);
        AZ::Vector2 high(-AZ::Constants::FloatMax, -AZ::Constants::FloatMax);
        for (const AZ::u32 vertex : targets)
        {
            low = low.GetMin(m_model.m_vertices[vertex].m_uv);
            high = high.GetMax(m_model.m_vertices[vertex].m_uv);
        }
        const AZ::Vector2 size = high - low;
        // Uniform, so the texture is not stretched; the longer side spans the square.
        const float scale = 1.0f / AZ::GetMax(AZ::GetMax(size.GetX(), size.GetY()), 1e-6f);
        TransformTargets([low, scale](const AZ::Vector2& uv, const AZ::Vector2&) { return (uv - low) * scale; });
    }

    void WhiteBoxUvCanvas::UpdateTransformDrag(const QPointF& screen, const bool snap, const bool final)
    {
        const AZ::Vector2 start = ToUv(m_pressScreen);
        const AZ::Vector2 current = ToUv(screen);
        AZStd::vector<AZ::Vector2> uvs(m_dragStartUvs.size());
        if (m_tool == Tool::Move)
        {
            AZ::Vector2 delta = current - start;
            if (snap)
            {
                delta = AZ::Vector2(std::round(delta.GetX() * 32.0f) / 32.0f, std::round(delta.GetY() * 32.0f) / 32.0f);
            }
            for (size_t i = 0; i < uvs.size(); ++i)
            {
                uvs[i] = m_dragStartUvs[i] + delta;
            }
        }
        else if (m_tool == Tool::Rotate)
        {
            const AZ::Vector2 from = start - m_dragPivot;
            const AZ::Vector2 to = current - m_dragPivot;
            float angle = std::atan2(to.GetY(), to.GetX()) - std::atan2(from.GetY(), from.GetX());
            if (snap)
            {
                const float step = AZ::DegToRad(15.0f);
                angle = std::round(angle / step) * step;
            }
            const float c = std::cos(angle);
            const float s = std::sin(angle);
            for (size_t i = 0; i < uvs.size(); ++i)
            {
                const AZ::Vector2 local = m_dragStartUvs[i] - m_dragPivot;
                uvs[i] = m_dragPivot + AZ::Vector2(local.GetX() * c - local.GetY() * s, local.GetX() * s + local.GetY() * c);
            }
        }
        else
        {
            const float from = (start - m_dragPivot).GetLength();
            float factor = from > 1e-6f ? (current - m_dragPivot).GetLength() / from : 1.0f;
            if (snap)
            {
                factor = AZ::GetMax(std::round(factor * 10.0f) / 10.0f, 0.1f);
            }
            for (size_t i = 0; i < uvs.size(); ++i)
            {
                uvs[i] = m_dragPivot + (m_dragStartUvs[i] - m_dragPivot) * factor;
            }
        }
        Apply(m_dragTargets, uvs, final);
    }

    void WhiteBoxUvCanvas::mousePressEvent(QMouseEvent* event)
    {
        setFocus();
        m_pressScreen = EventPosition(event);
        m_lastScreen = EventPosition(event);
        const bool alt = event->modifiers().testFlag(Qt::AltModifier);
        if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && alt))
        {
            m_drag = Drag::Pan;
            return;
        }
        if (event->button() != Qt::LeftButton)
        {
            return;
        }
        const int picked = PickVertex(EventPosition(event));
        const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);
        const bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
        if (picked < 0)
        {
            m_drag = Drag::Box;
            m_box = QRect(EventPosition(event).toPoint(), QSize());
            return;
        }
        if (ctrl && !shift)
        {
            SetSelected(picked, false);
            update();
            Report();
            return;
        }
        if (!IsSelected(picked))
        {
            if (!shift)
            {
                m_selectedCorners.clear();
            }
            SetSelected(picked, true);
        }
        // A press on a selected point starts a drag of the whole selection.
        m_drag = Drag::Transform;
        m_dragTargets = Targets();
        m_dragStartUvs.clear();
        for (const AZ::u32 vertex : m_dragTargets)
        {
            m_dragStartUvs.push_back(m_model.m_vertices[vertex].m_uv);
        }
        m_dragPivot = Centre(m_dragTargets);
        m_dragMoved = false;
        update();
        Report();
    }

    void WhiteBoxUvCanvas::mouseMoveEvent(QMouseEvent* event)
    {
        const QPointF screen = EventPosition(event);
        switch (m_drag)
        {
        case Drag::Pan:
            m_originOnScreen += screen - m_lastScreen;
            update();
            break;
        case Drag::Box:
            m_box = QRect(m_pressScreen.toPoint(), EventPosition(event).toPoint()).normalized();
            update();
            break;
        case Drag::Transform:
            if (m_dragMoved || (screen - m_pressScreen).manhattanLength() > 2.0)
            {
                m_dragMoved = true;
                UpdateTransformDrag(screen, event->modifiers().testFlag(Qt::ControlModifier), false);
            }
            break;
        default:
            break;
        }
        m_lastScreen = screen;
    }

    void WhiteBoxUvCanvas::mouseReleaseEvent(QMouseEvent* event)
    {
        const Drag drag = m_drag;
        m_drag = Drag::None;
        if (drag == Drag::Box)
        {
            const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);
            const bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
            if (!shift && !ctrl)
            {
                m_selectedCorners.clear();
            }
            const QRectF box = QRectF(m_box).adjusted(-1.0, -1.0, 1.0, 1.0);
            for (AZ::u32 i = 0; i < m_model.m_vertices.size(); ++i)
            {
                if (box.contains(ToScreen(m_model.m_vertices[i].m_uv)))
                {
                    SetSelected(i, !ctrl);
                }
            }
            m_box = QRect();
            update();
            Report();
        }
        else if (drag == Drag::Transform)
        {
            // A click without movement only selects; a real drag ends as one edit.
            if (m_dragMoved)
            {
                UpdateTransformDrag(EventPosition(event), event->modifiers().testFlag(Qt::ControlModifier), true);
            }
        }
    }

    void WhiteBoxUvCanvas::mouseDoubleClickEvent(QMouseEvent* event)
    {
        const int picked = PickVertex(EventPosition(event));
        if (picked < 0)
        {
            return;
        }
        if (!event->modifiers().testFlag(Qt::ShiftModifier))
        {
            m_selectedCorners.clear();
        }
        for (const AZ::u32 vertex : Island(picked))
        {
            SetSelected(vertex, true);
        }
        m_drag = Drag::None;
        update();
        Report();
    }

    void WhiteBoxUvCanvas::wheelEvent(QWheelEvent* event)
    {
        // Zoom about the cursor, so the point under it stays put.
        const QPointF cursor = event->position();
        const AZ::Vector2 anchor = ToUv(cursor);
        const double factor = std::pow(1.0015, event->angleDelta().y());
        m_pixelsPerUnit = AZ::GetClamp(m_pixelsPerUnit * factor, MinPixelsPerUnit, MaxPixelsPerUnit);
        m_originOnScreen = cursor - QPointF(anchor.GetX() * m_pixelsPerUnit, anchor.GetY() * m_pixelsPerUnit);
        update();
    }

    void WhiteBoxUvCanvas::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_F:
            FrameSelection();
            break;
        case Qt::Key_A:
            SelectAll();
            break;
        case Qt::Key_L:
            SelectLinked();
            break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            GrowSelection();
            break;
        case Qt::Key_Minus:
            ShrinkSelection();
            break;
        case Qt::Key_I:
            if (event->modifiers().testFlag(Qt::ControlModifier))
            {
                InvertSelection();
            }
            break;
        case Qt::Key_Escape:
            m_selectedCorners.clear();
            update();
            Report();
            break;
        default:
            QWidget::keyPressEvent(event);
        }
    }

    void WhiteBoxUvCanvas::Report()
    {
        if (!m_onStatus)
        {
            return;
        }
        size_t selected = 0;
        for (AZ::u32 i = 0; i < m_model.m_vertices.size(); ++i)
        {
            selected += IsSelected(i) ? 1 : 0;
        }
        m_onStatus(
            m_model.m_vertices.empty()
                ? tr("Select polygons in Transform mode to edit their UVs.")
                : tr("%1 UV points, %2 selected").arg(m_model.m_vertices.size()).arg(selected));
    }

    void WhiteBoxUvCanvas::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        if (!m_framed && !m_model.m_vertices.empty())
        {
            FrameAll();
        }
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(34, 34, 36));

        const QPointF squareTopLeft = ToScreen(AZ::Vector2(0.0f, 0.0f));
        const AZ::Vector2 visibleLow = ToUv(QPointF(0.0, 0.0));
        const AZ::Vector2 visibleHigh = ToUv(QPointF(width(), height()));
        if (!m_background.isNull())
        {
            // The texture repeats outside the unit square, as it does on the mesh, dimmed so the square stands out.
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            const int firstU = static_cast<int>(std::floor(visibleLow.GetX()));
            const int lastU = static_cast<int>(std::floor(visibleHigh.GetX()));
            const int firstV = static_cast<int>(std::floor(visibleLow.GetY()));
            const int lastV = static_cast<int>(std::floor(visibleHigh.GetY()));
            const bool fewTiles = (lastU - firstU + 1) * (lastV - firstV + 1) <= 400;
            for (int v = fewTiles ? firstV : 0; v <= (fewTiles ? lastV : 0); ++v)
            {
                for (int u = fewTiles ? firstU : 0; u <= (fewTiles ? lastU : 0); ++u)
                {
                    painter.setOpacity(u == 0 && v == 0 ? 1.0 : 0.35);
                    const QPointF corner = ToScreen(AZ::Vector2(static_cast<float>(u), static_cast<float>(v)));
                    painter.drawImage(QRectF(corner, QSizeF(m_pixelsPerUnit, m_pixelsPerUnit)), m_background);
                }
            }
            painter.setOpacity(1.0);
        }
        else
        {
            // No texture: the unit square as a checker, the tile a texture repeats on.
            const double cell = m_pixelsPerUnit / 8.0;
            for (int row = 0; row < 8; ++row)
            {
                for (int column = 0; column < 8; ++column)
                {
                    const QColor shade = (row + column) % 2 == 0 ? QColor(58, 58, 62) : QColor(50, 50, 54);
                    painter.fillRect(QRectF(squareTopLeft + QPointF(column * cell, row * cell), QSizeF(cell, cell)), shade);
                }
            }
        }

        // Whole-unit lines across the view, so repeats outside the square are readable.
        painter.setPen(QPen(QColor(70, 70, 76), 1.0));
        const AZ::Vector2& viewLow = visibleLow;
        const AZ::Vector2& viewHigh = visibleHigh;
        if (m_pixelsPerUnit >= 12.0)
        {
            for (int u = static_cast<int>(std::floor(viewLow.GetX())); u <= static_cast<int>(std::ceil(viewHigh.GetX())); ++u)
            {
                const double x = ToScreen(AZ::Vector2(static_cast<float>(u), 0.0f)).x();
                painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            }
            for (int v = static_cast<int>(std::floor(viewLow.GetY())); v <= static_cast<int>(std::ceil(viewHigh.GetY())); ++v)
            {
                const double y = ToScreen(AZ::Vector2(0.0f, static_cast<float>(v))).y();
                painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
            }
        }
        painter.setPen(QPen(QColor(150, 150, 160), 1.5));
        painter.drawRect(QRectF(squareTopLeft, QSizeF(m_pixelsPerUnit, m_pixelsPerUnit)));

        // Trim sheet bands across the square: the picked band tinted, every edge drawn, numbers down the left.
        if (m_trimEdges.size() >= 2)
        {
            const double left = squareTopLeft.x();
            const double right = left + m_pixelsPerUnit;
            if (m_trimHighlight >= 0 && m_trimHighlight + 1 < static_cast<int>(m_trimEdges.size()))
            {
                const double top = ToScreen(AZ::Vector2(0.0f, m_trimEdges[m_trimHighlight])).y();
                const double bottom = ToScreen(AZ::Vector2(0.0f, m_trimEdges[m_trimHighlight + 1])).y();
                painter.fillRect(QRectF(QPointF(left, top), QPointF(right, bottom)), QColor(103, 199, 232, 60));
            }
            painter.setPen(QPen(QColor(232, 170, 70), 1.2, Qt::DashLine));
            for (const float edge : m_trimEdges)
            {
                const double y = ToScreen(AZ::Vector2(0.0f, edge)).y();
                painter.drawLine(QPointF(left, y), QPointF(right, y));
            }
            painter.setPen(QColor(232, 170, 70));
            for (size_t band = 0; band + 1 < m_trimEdges.size(); ++band)
            {
                const double top = ToScreen(AZ::Vector2(0.0f, m_trimEdges[band])).y();
                const double bottom = ToScreen(AZ::Vector2(0.0f, m_trimEdges[band + 1])).y();
                painter.drawText(
                    QRectF(QPointF(left - 28.0, top), QPointF(left - 4.0, bottom)), Qt::AlignRight | Qt::AlignVCenter,
                    QString::number(band + 1));
            }
        }

        // Faces, then the diagonals inside polygons faintly, then polygon edges.
        painter.setPen(Qt::NoPen);
        for (size_t t = 0; t < m_model.m_triangles.size(); ++t)
        {
            const auto& triangle = m_model.m_triangles[t];
            const bool hovered = m_hoveredFaces.find(m_model.m_faces[t].Index()) != m_hoveredFaces.end();
            // A texture shows through a lighter tint; the face under the viewport cursor is picked out.
            painter.setBrush(hovered ? QColor(255, 200, 80, 110) : QColor(103, 199, 232, m_background.isNull() ? 40 : 25));
            const QPointF points[3] = { ToScreen(m_model.m_vertices[triangle[0]].m_uv), ToScreen(m_model.m_vertices[triangle[1]].m_uv),
                                        ToScreen(m_model.m_vertices[triangle[2]].m_uv) };
            painter.drawPolygon(points, 3);
        }
        for (const bool border : { false, true })
        {
            painter.setPen(border ? QPen(QColor(103, 199, 232), 1.5) : QPen(QColor(103, 199, 232, 70), 1.0, Qt::DashLine));
            for (const auto& edge : m_model.m_edges)
            {
                if (edge.m_border == border)
                {
                    painter.drawLine(ToScreen(m_model.m_vertices[edge.m_from].m_uv), ToScreen(m_model.m_vertices[edge.m_to].m_uv));
                }
            }
        }

        painter.setPen(Qt::NoPen);
        for (AZ::u32 i = 0; i < m_model.m_vertices.size(); ++i)
        {
            const bool selected = IsSelected(i);
            painter.setBrush(selected ? QColor(255, 160, 40) : QColor(218, 218, 218));
            const double radius = selected ? 4.0 : 3.0;
            painter.drawEllipse(ToScreen(m_model.m_vertices[i].m_uv), radius, radius);
        }

        if (m_drag == Drag::Box && !m_box.isNull())
        {
            painter.setPen(QPen(QColor(255, 160, 40), 1.0, Qt::DashLine));
            painter.setBrush(QColor(255, 160, 40, 30));
            painter.drawRect(m_box);
        }
    }
} // namespace WhiteBox
