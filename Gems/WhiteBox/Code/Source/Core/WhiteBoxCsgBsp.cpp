/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxCsgCore.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

// BSP-tree (brush-style) CSG solver: a std-only Naylor BSP boolean, as in csg.js. Closed inputs are
// normalized to outward winding for classification and the result takes the target's orientation, so
// inward-facing rooms are not treated as infinite complements. Tolerates open meshes; less robust than
// Manifold on slivers and may emit non-manifold output.

namespace WhiteBox
{
    namespace Csg
    {
        namespace
        {
            // Plane classification epsilon. White Box geometry is roughly unit-scaled, so a small
            // absolute epsilon keeps coplanar faces from being split into slivers.
            constexpr double PlaneEpsilon = 1e-5;

            // Per-vertex / per-polygon side classification (bitmask-combinable).
            enum PolygonSide
            {
                Coplanar = 0,
                Front = 1,
                Back = 2,
                Spanning = 3 // Front | Back
            };

            struct Vec3
            {
                double m_x = 0.0;
                double m_y = 0.0;
                double m_z = 0.0;

                Vec3() = default;
                Vec3(double x, double y, double z)
                    : m_x(x)
                    , m_y(y)
                    , m_z(z)
                {
                }

                Vec3 operator-(const Vec3& rhs) const
                {
                    return Vec3(m_x - rhs.m_x, m_y - rhs.m_y, m_z - rhs.m_z);
                }
                Vec3 operator+(const Vec3& rhs) const
                {
                    return Vec3(m_x + rhs.m_x, m_y + rhs.m_y, m_z + rhs.m_z);
                }
                Vec3 operator*(double s) const
                {
                    return Vec3(m_x * s, m_y * s, m_z * s);
                }

                double Dot(const Vec3& rhs) const
                {
                    return m_x * rhs.m_x + m_y * rhs.m_y + m_z * rhs.m_z;
                }
                Vec3 Cross(const Vec3& rhs) const
                {
                    return Vec3(
                        m_y * rhs.m_z - m_z * rhs.m_y, m_z * rhs.m_x - m_x * rhs.m_z, m_x * rhs.m_y - m_y * rhs.m_x);
                }
                double Length() const
                {
                    return std::sqrt(Dot(*this));
                }
            };

            // Linear interpolation between two positions (used when a polygon edge crosses a plane).
            Vec3 Lerp(const Vec3& a, const Vec3& b, double t)
            {
                return a + (b - a) * t;
            }

            struct Plane
            {
                Vec3 m_normal;
                double m_w = 0.0;
                bool m_valid = false;

                Plane() = default;

                //! Plane through three polygon vertices; winding sets the normal direction.
                static Plane FromPoints(const Vec3& a, const Vec3& b, const Vec3& c)
                {
                    Plane plane;
                    const Vec3 n = (b - a).Cross(c - a);
                    const double len = n.Length();
                    if (len <= 1e-12)
                    {
                        return plane; // degenerate (collinear) - leave invalid
                    }
                    plane.m_normal = n * (1.0 / len);
                    plane.m_w = plane.m_normal.Dot(a);
                    plane.m_valid = true;
                    return plane;
                }

                void Flip()
                {
                    m_normal = Vec3(-m_normal.m_x, -m_normal.m_y, -m_normal.m_z);
                    m_w = -m_w;
                }
            };

            struct Polygon
            {
                std::vector<Vec3> m_vertices;
                Plane m_plane;
                std::string m_material;
                uint32_t m_color = 0;

                Polygon() = default;
                Polygon(std::vector<Vec3> vertices, const Plane& plane, std::string material = {}, uint32_t color = 0)
                    : m_vertices(std::move(vertices))
                    , m_plane(plane)
                    , m_material(std::move(material))
                    , m_color(color)
                {
                }

                void Flip()
                {
                    std::reverse(m_vertices.begin(), m_vertices.end());
                    m_plane.Flip();
                }
            };

            //! Split @p polygon by @p plane into the front / back / coplanar buckets (csg.js splitPolygon).
            void SplitPolygon(
                const Plane& plane, const Polygon& polygon, std::vector<Polygon>& coplanarFront,
                std::vector<Polygon>& coplanarBack, std::vector<Polygon>& front, std::vector<Polygon>& back)
            {
                const size_t count = polygon.m_vertices.size();

                int polygonType = 0;
                std::vector<int> types(count);
                for (size_t i = 0; i < count; ++i)
                {
                    const double t = plane.m_normal.Dot(polygon.m_vertices[i]) - plane.m_w;
                    const int type = (t < -PlaneEpsilon) ? Back : (t > PlaneEpsilon) ? Front : Coplanar;
                    polygonType |= type;
                    types[i] = type;
                }

                switch (polygonType)
                {
                case Coplanar:
                    (plane.m_normal.Dot(polygon.m_plane.m_normal) > 0.0 ? coplanarFront : coplanarBack)
                        .push_back(polygon);
                    break;
                case Front:
                    front.push_back(polygon);
                    break;
                case Back:
                    back.push_back(polygon);
                    break;
                case Spanning:
                default:
                {
                    std::vector<Vec3> frontVerts;
                    std::vector<Vec3> backVerts;
                    for (size_t i = 0; i < count; ++i)
                    {
                        const size_t j = (i + 1) % count;
                        const int ti = types[i];
                        const int tj = types[j];
                        const Vec3& vi = polygon.m_vertices[i];
                        const Vec3& vj = polygon.m_vertices[j];

                        if (ti != Back)
                        {
                            frontVerts.push_back(vi);
                        }
                        if (ti != Front)
                        {
                            backVerts.push_back(vi);
                        }
                        if ((ti | tj) == Spanning)
                        {
                            const double denom = plane.m_normal.Dot(vj - vi);
                            // denom cannot be ~0 here: the edge genuinely crosses the plane.
                            const double t = (plane.m_w - plane.m_normal.Dot(vi)) / denom;
                            const Vec3 crossing = Lerp(vi, vj, t);
                            frontVerts.push_back(crossing);
                            backVerts.push_back(crossing);
                        }
                    }
                    // The split pieces are coplanar with the original, so reuse its plane (more stable
                    // than recomputing from a possibly near-degenerate sub-polygon).
                    if (frontVerts.size() >= 3)
                    {
                        front.emplace_back(std::move(frontVerts), polygon.m_plane, polygon.m_material, polygon.m_color);
                    }
                    if (backVerts.size() >= 3)
                    {
                        back.emplace_back(std::move(backVerts), polygon.m_plane, polygon.m_material, polygon.m_color);
                    }
                    break;
                }
                }
            }

            //! A BSP tree node (owns its sub-trees). Built lazily from a set of polygons.
            struct Node
            {
                Plane m_plane;
                std::unique_ptr<Node> m_front;
                std::unique_ptr<Node> m_back;
                std::vector<Polygon> m_polygons; // coplanar with m_plane

                //! Insert @p polygons into this node, choosing this node's split plane from the first
                //! polygon if not already set.
                void Build(std::vector<Polygon>& polygons)
                {
                    if (polygons.empty())
                    {
                        return;
                    }
                    if (!m_plane.m_valid)
                    {
                        m_plane = polygons.front().m_plane;
                    }

                    std::vector<Polygon> frontList;
                    std::vector<Polygon> backList;
                    for (const Polygon& polygon : polygons)
                    {
                        // coplanar polygons (either facing) stay on this node.
                        SplitPolygon(m_plane, polygon, m_polygons, m_polygons, frontList, backList);
                    }

                    if (!frontList.empty())
                    {
                        if (!m_front)
                        {
                            m_front = std::make_unique<Node>();
                        }
                        m_front->Build(frontList);
                    }
                    if (!backList.empty())
                    {
                        if (!m_back)
                        {
                            m_back = std::make_unique<Node>();
                        }
                        m_back->Build(backList);
                    }
                }

                //! Return @p polygons clipped so that everything inside this solid is removed (i.e.
                //! keep the parts that are in front of the surface, discard the parts behind it).
                std::vector<Polygon> ClipPolygons(const std::vector<Polygon>& polygons) const
                {
                    if (!m_plane.m_valid)
                    {
                        return polygons;
                    }

                    std::vector<Polygon> frontList;
                    std::vector<Polygon> backList;
                    for (const Polygon& polygon : polygons)
                    {
                        // coplanar-front -> front, coplanar-back -> back (csg.js clipPolygons routing).
                        SplitPolygon(m_plane, polygon, frontList, backList, frontList, backList);
                    }

                    if (m_front)
                    {
                        frontList = m_front->ClipPolygons(frontList);
                    }
                    if (m_back)
                    {
                        backList = m_back->ClipPolygons(backList);
                    }
                    else
                    {
                        backList.clear(); // no back sub-tree -> "behind" means inside the solid -> drop
                    }

                    frontList.insert(frontList.end(), backList.begin(), backList.end());
                    return frontList;
                }

                //! Remove all of this node's polygons that are inside the @p other solid.
                void ClipTo(const Node& other)
                {
                    m_polygons = other.ClipPolygons(m_polygons);
                    if (m_front)
                    {
                        m_front->ClipTo(other);
                    }
                    if (m_back)
                    {
                        m_back->ClipTo(other);
                    }
                }

                //! Convert this solid to its complement (flip every polygon and the tree structure).
                void Invert()
                {
                    for (Polygon& polygon : m_polygons)
                    {
                        polygon.Flip();
                    }
                    if (m_plane.m_valid)
                    {
                        m_plane.Flip();
                    }
                    if (m_front)
                    {
                        m_front->Invert();
                    }
                    if (m_back)
                    {
                        m_back->Invert();
                    }
                    m_front.swap(m_back);
                }

                void AllPolygons(std::vector<Polygon>& out) const
                {
                    out.insert(out.end(), m_polygons.begin(), m_polygons.end());
                    if (m_front)
                    {
                        m_front->AllPolygons(out);
                    }
                    if (m_back)
                    {
                        m_back->AllPolygons(out);
                    }
                }
            };

            Vec3 MeshVertex(const TriangleMesh& mesh, uint32_t index)
            {
                return Vec3(
                    mesh.m_positions[index * 3 + 0], mesh.m_positions[index * 3 + 1], mesh.m_positions[index * 3 + 2]);
            }

            //! One (triangle) polygon per input triangle. Degenerate triangles are dropped so their
            //! invalid planes never become a node's split plane.
            std::vector<Polygon> TrianglesToPolygons(const TriangleMesh& mesh)
            {
                std::vector<Polygon> polygons;
                const size_t triangleCount = mesh.TriangleCount();
                polygons.reserve(triangleCount);
                for (size_t t = 0; t < triangleCount; ++t)
                {
                    const Vec3 a = MeshVertex(mesh, mesh.m_indices[t * 3 + 0]);
                    const Vec3 b = MeshVertex(mesh, mesh.m_indices[t * 3 + 1]);
                    const Vec3 c = MeshVertex(mesh, mesh.m_indices[t * 3 + 2]);
                    const Plane plane = Plane::FromPoints(a, b, c);
                    if (!plane.m_valid)
                    {
                        continue;
                    }
                    polygons.emplace_back(std::vector<Vec3>{ a, b, c }, plane, mesh.Material(t), mesh.Color(t));
                }
                return polygons;
            }

            //! BSP assumes the back of every plane is solid, so an inward-wound closed mesh (a room) is
            //! normalized before classification. Open meshes keep their winding.
            bool IsClosedMesh(const TriangleMesh& mesh)
            {
                std::unordered_map<uint64_t, uint32_t> edgeUses;
                edgeUses.reserve(mesh.m_indices.size());
                for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
                {
                    for (size_t edge = 0; edge < 3; ++edge)
                    {
                        const uint32_t a = mesh.m_indices[triangle * 3 + edge];
                        const uint32_t b = mesh.m_indices[triangle * 3 + (edge + 1) % 3];
                        const uint32_t low = std::min(a, b);
                        const uint32_t high = std::max(a, b);
                        const uint64_t key = (static_cast<uint64_t>(low) << 32) | high;
                        ++edgeUses[key];
                    }
                }
                return !edgeUses.empty() &&
                    std::all_of(
                        edgeUses.begin(), edgeUses.end(), [](const auto& edge) { return edge.second == 2; });
            }

            double SignedVolume(const TriangleMesh& mesh)
            {
                double volumeTimesSix = 0.0;
                for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
                {
                    const Vec3 a = MeshVertex(mesh, mesh.m_indices[triangle * 3 + 0]);
                    const Vec3 b = MeshVertex(mesh, mesh.m_indices[triangle * 3 + 1]);
                    const Vec3 c = MeshVertex(mesh, mesh.m_indices[triangle * 3 + 2]);
                    volumeTimesSix += a.Dot(b.Cross(c));
                }
                return volumeTimesSix / 6.0;
            }

            bool IsClosedInwardMesh(const TriangleMesh& mesh)
            {
                return IsClosedMesh(mesh) && SignedVolume(mesh) < 0.0;
            }

            void FlipPolygons(std::vector<Polygon>& polygons)
            {
                for (Polygon& polygon : polygons)
                {
                    polygon.Flip();
                }
            }

            //! Fan-triangulate the (convex, coplanar) result polygons back into the flat triangle mesh.
            void PolygonsToTriangleMesh(const std::vector<Polygon>& polygons, TriangleMesh& mesh)
            {
                mesh.m_positions.clear();
                mesh.m_indices.clear();
                mesh.m_materials.clear();
                mesh.m_colors.clear();
                for (const Polygon& polygon : polygons)
                {
                    const size_t count = polygon.m_vertices.size();
                    if (count < 3)
                    {
                        continue;
                    }
                    // Not "emit": Qt defines that as an empty macro, which unity builds could pull in.
                    const auto emitVertex = [&mesh](const Vec3& v) -> uint32_t
                    {
                        const auto index = static_cast<uint32_t>(mesh.m_positions.size() / 3);
                        mesh.m_positions.push_back(v.m_x);
                        mesh.m_positions.push_back(v.m_y);
                        mesh.m_positions.push_back(v.m_z);
                        return index;
                    };
                    const uint32_t i0 = emitVertex(polygon.m_vertices[0]);
                    for (size_t i = 1; i + 1 < count; ++i)
                    {
                        const uint32_t i1 = emitVertex(polygon.m_vertices[i]);
                        const uint32_t i2 = emitVertex(polygon.m_vertices[i + 1]);
                        mesh.m_indices.push_back(i0);
                        mesh.m_indices.push_back(i1);
                        mesh.m_indices.push_back(i2);
                        mesh.m_materials.push_back(polygon.m_material);
                        mesh.m_colors.push_back(polygon.m_color);
                    }
                }
            }
        } // namespace

        bool MeshBooleanBsp(
            const TriangleMesh& meshA, const TriangleMesh& meshB, const BooleanOperation operation, TriangleMesh& result)
        {
            if (meshA.TriangleCount() == 0 || meshB.TriangleCount() == 0)
            {
                return false;
            }

            std::vector<Polygon> polygonsA = TrianglesToPolygons(meshA);
            std::vector<Polygon> polygonsB = TrianglesToPolygons(meshB);
            if (polygonsA.empty() || polygonsB.empty())
            {
                return false;
            }

            // Winding is presentation only for closed solids: the target decides the result's orientation.
            const bool targetWasInward = IsClosedInwardMesh(meshA);
            const bool sourceWasInward = IsClosedInwardMesh(meshB);
            if (targetWasInward)
            {
                FlipPolygons(polygonsA);
            }
            if (sourceWasInward)
            {
                FlipPolygons(polygonsB);
            }

            Node a;
            a.Build(polygonsA);
            Node b;
            b.Build(polygonsB);

            std::vector<Polygon> out;
            switch (operation)
            {
            case BooleanOperation::Union:
                // a.clipTo(b); b.clipTo(a); b.invert; b.clipTo(a); b.invert; a.build(b.all)
                a.ClipTo(b);
                b.ClipTo(a);
                b.Invert();
                b.ClipTo(a);
                b.Invert();
                {
                    std::vector<Polygon> bPolys;
                    b.AllPolygons(bPolys);
                    a.Build(bPolys);
                }
                a.AllPolygons(out);
                break;
            case BooleanOperation::Subtraction:
                // a.invert; a.clipTo(b); b.clipTo(a); b.invert; b.clipTo(a); b.invert; a.build(b.all); a.invert
                a.Invert();
                a.ClipTo(b);
                b.ClipTo(a);
                b.Invert();
                b.ClipTo(a);
                b.Invert();
                {
                    std::vector<Polygon> bPolys;
                    b.AllPolygons(bPolys);
                    a.Build(bPolys);
                }
                a.Invert();
                a.AllPolygons(out);
                break;
            case BooleanOperation::Intersection:
            default:
                // a.invert; b.clipTo(a); b.invert; a.clipTo(b); b.clipTo(a); a.build(b.all); a.invert
                a.Invert();
                b.ClipTo(a);
                b.Invert();
                a.ClipTo(b);
                b.ClipTo(a);
                {
                    std::vector<Polygon> bPolys;
                    b.AllPolygons(bPolys);
                    a.Build(bPolys);
                }
                a.Invert();
                a.AllPolygons(out);
                break;
            }

            if (targetWasInward)
            {
                FlipPolygons(out);
            }

            PolygonsToTriangleMesh(out, result);

            // Weld coincident vertices produced by the many independent split pieces so the rebuilt
            // White Box mesh shares vertices (matching what the Manifold path returns).
            WeldVertices(result, 1e-6);

            return result.TriangleCount() > 0;
        }
    } // namespace Csg
} // namespace WhiteBox
