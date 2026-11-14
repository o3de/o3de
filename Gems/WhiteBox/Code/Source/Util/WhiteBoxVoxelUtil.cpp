/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxVoxelUtil.h"

#include <AzCore/std/sort.h>
#include <cmath>

namespace WhiteBox
{
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

        // Cubes of different sizes are meshed one size group at a time; sizes are quantised to this.
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

        // A light surface for filled cells: exposed faces greedy-merged into maximal quads, so a cube is
        // 8 verts / 6 faces and walls carry only corner vertices.
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

            // Greedy-merge a plane's exposed cells into maximal {a0, a1, b0, b1} rectangles.
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

        // One quad per exposed cell face: always watertight and manifold (no T-junctions), so a valid CSG operand.
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

        // A vertex position's grid cell index; false for off-grid (freeform) vertices.
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

        // Triangle signatures of the voxel surface, generated the same way as the live mesh.
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

        // Greedy-merged collider triangles for the cells, appended to verts/indices.
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

            // Greedy-merge a plane's exposed cells. Not "emit": Qt defines that as an empty macro.
            const auto greedy = [&](AZStd::unordered_set<AZ::u64>& mask, auto&& emitQuad)
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
                    emitQuad(a, a1, b, b1);
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

    bool MeshFullyInsideCells(
        const WhiteBoxMesh& mesh, const AZStd::vector<AZ::u64>& cells, const AZStd::vector<float>& sizes)
    {
        const VoxelDetail::SizeGroups groups = VoxelDetail::GroupBySize(cells, sizes);
        if (groups.empty())
        {
            return false;
        }
        for (const Api::VertexHandle& vertexHandle : Api::MeshVertexHandles(mesh))
        {
            const AZ::Vector3 p = Api::VertexPosition(mesh, vertexHandle);
            bool inside = false;
            for (const auto& group : groups)
            {
                const float s = VoxelDetail::SizeFromQuantized(group.first);
                const float tol = s * 1e-3f;
                const int bx = static_cast<int>(std::floor(p.GetX() / s));
                const int by = static_cast<int>(std::floor(p.GetY() / s));
                const int bz = static_cast<int>(std::floor(p.GetZ() / s));
                // A vertex exactly on a cell boundary may mathematically belong to the
                // neighbouring cell, so test the 8 candidate cells around the point.
                for (int ox = -1; ox <= 0 && !inside; ++ox)
                {
                    for (int oy = -1; oy <= 0 && !inside; ++oy)
                    {
                        for (int oz = -1; oz <= 0 && !inside; ++oz)
                        {
                            const int cx = bx + ox;
                            const int cy = by + oy;
                            const int cz = bz + oz;
                            if (group.second.count(VoxelDetail::PackCell(cx, cy, cz)) == 0)
                            {
                                continue;
                            }
                            inside = p.GetX() >= cx * s - tol && p.GetX() <= (cx + 1) * s + tol &&
                                p.GetY() >= cy * s - tol && p.GetY() <= (cy + 1) * s + tol &&
                                p.GetZ() >= cz * s - tol && p.GetZ() <= (cz + 1) * s + tol;
                        }
                    }
                }
                if (inside)
                {
                    break;
                }
            }
            if (!inside)
            {
                return false;
            }
        }
        return true;
    }
} // namespace WhiteBox
