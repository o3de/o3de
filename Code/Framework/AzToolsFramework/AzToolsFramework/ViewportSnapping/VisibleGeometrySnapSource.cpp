/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportSnapping/VisibleGeometrySnapSource.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/math.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/utils.h>
#include <AzFramework/Visibility/VisibleGeometryBus.h>

AZ_CVAR(
    AZ::u32, ed_snapMaxVerticesPerEntity, 300000, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Cap on the number of snap candidates cached for a single entity. Meshes past this are ignored "
    "rather than eating memory and build time.");
AZ_CVAR(
    AZ::u32, ed_snapCacheLifetimeQueries, 600, nullptr, AZ::ConsoleFunctorFlags::Null,
    "How many snap queries a cached mesh survives without being used before it is evicted.");
AZ_CVAR(
    float, ed_snapFeatureAngleDegrees, 20.0f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "How sharply a triangulated mesh has to crease before an edge counts as a real edge for "
    "snapping. Lower values expose more edges, eventually including triangulation diagonals on "
    "curved surfaces; higher values keep only hard corners.");

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        namespace
        {
            //! Items per block. Small enough that a rejected block saves real work, large enough
            //! that the block list stays short.
            constexpr AZ::u32 ItemsPerBlock = 128;

            //! Positions closer together than this are treated as the same point.
            //!
            //! Mesh vertex buffers repeat positions per face corner, so this typically collapses
            //! the count a great deal - and the welding it performs is what makes neighbouring
            //! triangles share an edge at all, without which no topology could be recovered.
            constexpr float DuplicateEpsilon = 1.0e-4f;

            //! How many queries before an entity that reported no geometry is inspected again.
            constexpr AZ::u64 EmptyCacheRetryQueries = 120;

            //! Quantised position, used only for deduplication.
            struct QuantisedPosition
            {
                AZ::s32 m_x = 0;
                AZ::s32 m_y = 0;
                AZ::s32 m_z = 0;

                bool operator==(const QuantisedPosition& other) const
                {
                    return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
                }
            };

            struct QuantisedPositionHash
            {
                size_t operator()(const QuantisedPosition& position) const
                {
                    size_t hash = aznumeric_cast<size_t>(aznumeric_cast<AZ::u32>(position.m_x));
                    hash = hash * 0x9E3779B1u + aznumeric_cast<size_t>(aznumeric_cast<AZ::u32>(position.m_y));
                    hash = hash * 0x9E3779B1u + aznumeric_cast<size_t>(aznumeric_cast<AZ::u32>(position.m_z));
                    return hash;
                }
            };

            QuantisedPosition Quantise(const AZ::Vector3& position)
            {
                return QuantisedPosition{ aznumeric_cast<AZ::s32>(AZStd::round(position.GetX() / DuplicateEpsilon)),
                                          aznumeric_cast<AZ::s32>(AZStd::round(position.GetY() / DuplicateEpsilon)),
                                          aznumeric_cast<AZ::s32>(AZStd::round(position.GetZ() / DuplicateEpsilon)) };
            }

            //! An undirected edge between two welded vertex indices.
            struct EdgeKey
            {
                AZ::u32 m_low = 0;
                AZ::u32 m_high = 0;

                bool operator==(const EdgeKey& other) const
                {
                    return m_low == other.m_low && m_high == other.m_high;
                }
            };

            struct EdgeKeyHash
            {
                size_t operator()(const EdgeKey& edge) const
                {
                    return (aznumeric_cast<size_t>(edge.m_low) * 0x9E3779B1u) ^ aznumeric_cast<size_t>(edge.m_high);
                }
            };

            EdgeKey MakeEdgeKey(const AZ::u32 a, const AZ::u32 b)
            {
                return a < b ? EdgeKey{ a, b } : EdgeKey{ b, a };
            }

            //! What the reconstruction knows about an edge: which triangles use it, and their
            //! normals, which is what the crease angle is measured between.
            struct EdgeInfo
            {
                AZ::u32 m_triangleCount = 0;
                AZ::u32 m_firstTriangle = 0;
                AZ::u32 m_secondTriangle = 0;
                AZ::Vector3 m_firstNormal = AZ::Vector3::CreateZero();
                AZ::Vector3 m_secondNormal = AZ::Vector3::CreateZero();
            };

            AZ::Transform EntityWorldTransform(const AZ::EntityId entityId)
            {
                AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
                return worldTransform;
            }

            //! Sort items into coarse spatial cells, so that a block of consecutive items is also
            //! close together in space.
            //!
            //! Without this the blocks below are useless: items arrive in whatever order the vertex
            //! buffer happened to store them, so a block of 128 of them can span the whole mesh and
            //! its bounding sphere rejects nothing.
            //!
            //! @param getPosition maps an element to the point that decides where it sorts. Edges
            //! use their midpoint; plain points map to themselves.
            template<typename Element, typename GetPositionFn>
            void SpatiallySort(AZStd::vector<Element>& elements, GetPositionFn getPosition)
            {
                if (elements.size() < 2)
                {
                    return;
                }

                AZ::Aabb bounds = AZ::Aabb::CreateNull();
                for (const Element& element : elements)
                {
                    bounds.AddPoint(getPosition(element));
                }

                const float extent = AZStd::max(bounds.GetExtents().GetMaxElement(), 1.0e-3f);
                const float cellSize = extent / 64.0f;
                const AZ::Vector3 boundsMin = bounds.GetMin();

                struct SortKey
                {
                    AZ::s32 m_cellX = 0;
                    AZ::s32 m_cellY = 0;
                    AZ::s32 m_cellZ = 0;
                    AZ::u32 m_index = 0;
                };

                AZStd::vector<SortKey> keys;
                keys.reserve(elements.size());
                for (size_t index = 0; index < elements.size(); ++index)
                {
                    const AZ::Vector3 cell = (getPosition(elements[index]) - boundsMin) / cellSize;
                    keys.push_back(SortKey{ aznumeric_cast<AZ::s32>(cell.GetX()), aznumeric_cast<AZ::s32>(cell.GetY()),
                                            aznumeric_cast<AZ::s32>(cell.GetZ()), aznumeric_cast<AZ::u32>(index) });
                }

                AZStd::sort(
                    keys.begin(), keys.end(),
                    [](const SortKey& lhs, const SortKey& rhs)
                    {
                        if (lhs.m_cellX != rhs.m_cellX)
                        {
                            return lhs.m_cellX < rhs.m_cellX;
                        }
                        if (lhs.m_cellY != rhs.m_cellY)
                        {
                            return lhs.m_cellY < rhs.m_cellY;
                        }
                        return lhs.m_cellZ < rhs.m_cellZ;
                    });

                AZStd::vector<Element> sorted;
                sorted.reserve(elements.size());
                for (const SortKey& key : keys)
                {
                    sorted.push_back(elements[key.m_index]);
                }

                elements = AZStd::move(sorted);
            }
        } // namespace

        void VisibleGeometrySnapSource::Clear()
        {
            m_caches.clear();
        }

        void VisibleGeometrySnapSource::EvictStaleCaches()
        {
            ++m_queryCounter;

            const AZ::u64 lifetime = aznumeric_cast<AZ::u64>(static_cast<AZ::u32>(ed_snapCacheLifetimeQueries));
            if (m_queryCounter <= lifetime)
            {
                return;
            }

            const AZ::u64 oldestAllowed = m_queryCounter - lifetime;
            for (auto it = m_caches.begin(); it != m_caches.end();)
            {
                if (it->second.m_lastUsedQuery < oldestAllowed)
                {
                    it = m_caches.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void VisibleGeometrySnapSource::BuildPointBlocks(
            const AZStd::vector<AZ::Vector3>& positions, AZStd::vector<Block>& blocks)
        {
            const AZ::u32 count = aznumeric_cast<AZ::u32>(positions.size());
            blocks.reserve((count / ItemsPerBlock) + 1);

            for (AZ::u32 begin = 0; begin < count; begin += ItemsPerBlock)
            {
                const AZ::u32 end = AZStd::min(begin + ItemsPerBlock, count);

                AZ::Aabb blockBounds = AZ::Aabb::CreateNull();
                for (AZ::u32 index = begin; index < end; ++index)
                {
                    blockBounds.AddPoint(positions[index]);
                }

                Block block;
                block.m_center = blockBounds.GetCenter();
                block.m_radius = blockBounds.GetExtents().GetLength() * 0.5f;
                block.m_begin = begin;
                block.m_end = end;
                blocks.push_back(block);
            }
        }

        void VisibleGeometrySnapSource::BuildCache(
            const AZ::EntityId entityId, const AZ::Transform& worldTransform, const bool buildTopology, EntityCache& cache)
        {
            cache.m_worldTransform = worldTransform;
            cache.m_topologyBuilt = buildTopology;
            cache.m_worldPositions.clear();
            cache.m_positionBlocks.clear();
            cache.m_edges.clear();
            cache.m_edgeBlocks.clear();
            cache.m_faceCenters.clear();
            cache.m_faceCenterBlocks.clear();
            cache.m_empty = true;

            AzFramework::VisibleGeometryContainer geometryContainer;

            // A null Aabb means "everything" per the bus contract. Asking for a sub-region would
            // not help: this runs once per cache rather than per query, and implementations are
            // free to ignore the bounds anyway.
            AzFramework::VisibleGeometryRequestBus::Event(
                entityId, &AzFramework::VisibleGeometryRequests::BuildVisibleGeometry, AZ::Aabb::CreateNull(),
                geometryContainer);

            if (geometryContainer.empty())
            {
                return;
            }

            const AZ::u32 maxVertices = ed_snapMaxVerticesPerEntity;

            // ---- Pass 1: weld positions into world space, remembering where each source vertex
            // went.
            AZStd::unordered_map<QuantisedPosition, AZ::u32, QuantisedPositionHash> weldMap;
            AZStd::vector<AZ::Vector3> welded;

            //! Triangles as welded indices. Kept separate from the source buffers so the topology
            //! passes below do not care which VisibleGeometry a triangle came from.
            AZStd::vector<AZ::u32> triangles;

            bool truncated = false;

            for (const AzFramework::VisibleGeometry& geometry : geometryContainer)
            {
                // VisibleGeometry carries its own local-to-world matrix; it is not pre-transformed.
                const AZ::Matrix4x4& localToWorld = geometry.m_transform;
                const size_t vertexCount = geometry.m_vertices.size() / 3;

                // Only the topology passes need to know where each source vertex ended up.
                AZStd::vector<AZ::u32> remap;
                if (buildTopology)
                {
                    remap.reserve(vertexCount);
                }

                for (size_t vertex = 0; vertex < vertexCount; ++vertex)
                {
                    if (welded.size() >= maxVertices)
                    {
                        truncated = true;
                        break;
                    }

                    const AZ::Vector3 localPosition(
                        geometry.m_vertices[vertex * 3], geometry.m_vertices[vertex * 3 + 1],
                        geometry.m_vertices[vertex * 3 + 2]);
                    const AZ::Vector3 worldPosition = localToWorld * localPosition;

                    const auto inserted =
                        weldMap.insert({ Quantise(worldPosition), aznumeric_cast<AZ::u32>(welded.size()) });
                    if (inserted.second)
                    {
                        welded.push_back(worldPosition);
                    }

                    if (buildTopology)
                    {
                        remap.push_back(inserted.first->second);
                    }
                }

                if (truncated)
                {
                    break;
                }

                if (!buildTopology)
                {
                    continue;
                }

                // Triangles referencing vertices past the cap are dropped rather than mis-indexed.
                for (size_t index = 0; index + 2 < geometry.m_indices.size(); index += 3)
                {
                    const AZ::u32 i0 = geometry.m_indices[index];
                    const AZ::u32 i1 = geometry.m_indices[index + 1];
                    const AZ::u32 i2 = geometry.m_indices[index + 2];

                    if (i0 >= remap.size() || i1 >= remap.size() || i2 >= remap.size())
                    {
                        continue;
                    }

                    const AZ::u32 a = remap[i0];
                    const AZ::u32 b = remap[i1];
                    const AZ::u32 c = remap[i2];

                    // Welding can collapse a sliver triangle to a line - it has no normal and no
                    // edges worth keeping.
                    if (a == b || b == c || a == c)
                    {
                        continue;
                    }

                    triangles.push_back(a);
                    triangles.push_back(b);
                    triangles.push_back(c);
                }
            }

            AZ_Warning(
                "ViewportSnapping", !truncated,
                "Entity %s has more than %u snap candidates; the rest are ignored. Raise "
                "ed_snapMaxVerticesPerEntity if you need to snap to all of it.",
                entityId.ToString().c_str(), maxVertices);

            if (welded.empty())
            {
                return;
            }

            // Passes 2-4 reconstruct topology, which costs several times what collecting the
            // positions does. Vertex snapping never looks at the result, so it does not pay for it.
            if (!buildTopology)
            {
                cache.m_worldPositions = AZStd::move(welded);
                SpatiallySort(
                    cache.m_worldPositions,
                    [](const AZ::Vector3& position)
                    {
                        return position;
                    });
                BuildPointBlocks(cache.m_worldPositions, cache.m_positionBlocks);
                cache.m_empty = cache.m_worldPositions.empty();
                return;
            }

            // ---- Pass 2: edge adjacency. Every triangle contributes its three edges; an edge
            // shared by two triangles records both their normals so the crease angle can be
            // measured.
            const size_t triangleCount = triangles.size() / 3;

            AZStd::unordered_map<EdgeKey, EdgeInfo, EdgeKeyHash> edgeMap;
            edgeMap.reserve(triangleCount * 3);

            for (size_t triangle = 0; triangle < triangleCount; ++triangle)
            {
                const AZ::u32 a = triangles[triangle * 3];
                const AZ::u32 b = triangles[triangle * 3 + 1];
                const AZ::u32 c = triangles[triangle * 3 + 2];

                const AZ::Vector3 normal = (welded[b] - welded[a]).Cross(welded[c] - welded[a]).GetNormalizedSafe();

                const AZ::u32 triangleIndex = aznumeric_cast<AZ::u32>(triangle);
                for (const EdgeKey key : { MakeEdgeKey(a, b), MakeEdgeKey(b, c), MakeEdgeKey(c, a) })
                {
                    EdgeInfo& info = edgeMap[key];
                    if (info.m_triangleCount == 0)
                    {
                        info.m_firstTriangle = triangleIndex;
                        info.m_firstNormal = normal;
                    }
                    else if (info.m_triangleCount == 1)
                    {
                        info.m_secondTriangle = triangleIndex;
                        info.m_secondNormal = normal;
                    }
                    ++info.m_triangleCount;
                }
            }

            // ---- Pass 3: keep the edges that read as edges. A boundary edge, used by one
            // triangle, always does; a shared edge only if the surface creases across it. This is
            // what separates the silhouette of a box from the diagonals its quads were
            // triangulated into.
            //
            // Read through a plain float first: AZ_CVAR declares a wrapper type which does not
            // convert implicitly where template argument deduction is involved.
            const float featureAngleDegrees = ed_snapFeatureAngleDegrees;
            const float featureAngleCos = AZ::Cos(AZ::DegToRad(featureAngleDegrees));

            AZ::s64 edgeIndex = 0;

            for (const auto& entry : edgeMap)
            {
                const EdgeInfo& info = entry.second;

                const bool boundary = info.m_triangleCount == 1;
                const bool creased =
                    info.m_triangleCount >= 2 && info.m_firstNormal.Dot(info.m_secondNormal) < featureAngleCos;

                if (!boundary && !creased)
                {
                    continue;
                }

                const AZ::Vector3& start = welded[entry.first.m_low];
                const AZ::Vector3& end = welded[entry.first.m_high];

                cache.m_edges.emplace_back(start, end, entityId, edgeIndex++);
            }

            // ---- Pass 4: group triangles joined across non-feature edges, so a quad split into
            // two triangles yields one centre rather than two. Union-find over triangle adjacency.
            if (triangleCount > 0)
            {
                AZStd::vector<AZ::u32> parent(triangleCount);
                for (AZ::u32 triangle = 0; triangle < triangleCount; ++triangle)
                {
                    parent[triangle] = triangle;
                }

                const auto find = [&parent](const AZ::u32 triangle) -> AZ::u32
                {
                    AZ::u32 root = triangle;
                    while (parent[root] != root)
                    {
                        root = parent[root];
                    }

                    // Path compression, so repeated lookups on a large flat surface stay cheap.
                    AZ::u32 walk = triangle;
                    while (parent[walk] != root)
                    {
                        const AZ::u32 next = parent[walk];
                        parent[walk] = root;
                        walk = next;
                    }

                    return root;
                };

                for (const auto& entry : edgeMap)
                {
                    const EdgeInfo& info = entry.second;
                    if (info.m_triangleCount != 2)
                    {
                        continue;
                    }

                    if (info.m_firstNormal.Dot(info.m_secondNormal) < featureAngleCos)
                    {
                        continue; // creased - the two triangles belong to different faces
                    }

                    const AZ::u32 rootA = find(info.m_firstTriangle);
                    const AZ::u32 rootB = find(info.m_secondTriangle);
                    if (rootA != rootB)
                    {
                        parent[rootA] = rootB;
                    }
                }

                //! Explicitly zeroed - AZ::Vector3's default constructor does not initialise, so
                //! relying on value-initialisation through a map's operator[] would be a trap.
                struct CentroidAccumulator
                {
                    AZ::Vector3 m_sum = AZ::Vector3::CreateZero();
                    AZ::u32 m_count = 0;
                };

                AZStd::unordered_map<AZ::u32, CentroidAccumulator> groupCentroids;
                for (size_t triangle = 0; triangle < triangleCount; ++triangle)
                {
                    const AZ::Vector3 centroid = (welded[triangles[triangle * 3]] + welded[triangles[triangle * 3 + 1]] +
                                                  welded[triangles[triangle * 3 + 2]]) /
                        3.0f;

                    CentroidAccumulator& accumulator = groupCentroids[find(aznumeric_cast<AZ::u32>(triangle))];
                    accumulator.m_sum += centroid;
                    ++accumulator.m_count;
                }

                cache.m_faceCenters.reserve(groupCentroids.size());
                for (const auto& group : groupCentroids)
                {
                    cache.m_faceCenters.push_back(group.second.m_sum / aznumeric_cast<float>(group.second.m_count));
                }
            }

            // ---- Blocks. Sort each set spatially first; blocks of arbitrarily ordered items have
            // bounding spheres so large they reject nothing.
            const auto identity = [](const AZ::Vector3& position)
            {
                return position;
            };

            cache.m_worldPositions = AZStd::move(welded);
            SpatiallySort(cache.m_worldPositions, identity);
            SpatiallySort(cache.m_faceCenters, identity);
            SpatiallySort(
                cache.m_edges,
                [](const SnapEdge& edge)
                {
                    return (edge.m_start + edge.m_end) * 0.5f;
                });

            BuildPointBlocks(cache.m_worldPositions, cache.m_positionBlocks);
            BuildPointBlocks(cache.m_faceCenters, cache.m_faceCenterBlocks);

            // Edges are blocked by their midpoints, with the radius grown to cover both endpoints.
            if (!cache.m_edges.empty())
            {
                const AZ::u32 edgeCount = aznumeric_cast<AZ::u32>(cache.m_edges.size());
                cache.m_edgeBlocks.reserve((edgeCount / ItemsPerBlock) + 1);

                for (AZ::u32 begin = 0; begin < edgeCount; begin += ItemsPerBlock)
                {
                    const AZ::u32 end = AZStd::min(begin + ItemsPerBlock, edgeCount);

                    AZ::Aabb blockBounds = AZ::Aabb::CreateNull();
                    for (AZ::u32 index = begin; index < end; ++index)
                    {
                        blockBounds.AddPoint(cache.m_edges[index].m_start);
                        blockBounds.AddPoint(cache.m_edges[index].m_end);
                    }

                    Block block;
                    block.m_center = blockBounds.GetCenter();
                    block.m_radius = blockBounds.GetExtents().GetLength() * 0.5f;
                    block.m_begin = begin;
                    block.m_end = end;
                    cache.m_edgeBlocks.push_back(block);
                }
            }

            cache.m_empty = cache.m_worldPositions.empty();
        }

        VisibleGeometrySnapSource::EntityCache* VisibleGeometrySnapSource::AcquireCache(
            const AZ::EntityId entityId, const bool needTopology)
        {
            const AZ::Transform worldTransform = EntityWorldTransform(entityId);

            EntityCache& cache = m_caches[entityId];
            cache.m_lastUsedQuery = m_queryCounter;

            // An entity whose model is still streaming in reports no geometry. Retry it
            // periodically rather than writing it off for the rest of the session.
            const bool retryEmpty = cache.m_built && cache.m_empty &&
                (m_queryCounter - cache.m_lastBuildQuery) >= EmptyCacheRetryQueries;

            // Switching from Vertex to Edge mode upgrades an existing cache in place rather than
            // rebuilding it on every subsequent query.
            const bool missingTopology = needTopology && !cache.m_topologyBuilt;

            if (!cache.m_built || retryEmpty || missingTopology || !cache.m_worldTransform.IsClose(worldTransform))
            {
                // Once topology has been paid for, keep it: the entity is evidently being used with
                // a mode that wants it, and dropping it would mean rebuilding on the next mode
                // switch.
                BuildCache(entityId, worldTransform, needTopology || cache.m_topologyBuilt, cache);
                cache.m_built = true;
                cache.m_lastBuildQuery = m_queryCounter;
            }

            return cache.m_empty ? nullptr : &cache;
        }

        void VisibleGeometrySnapSource::CollectSnapVertices(
            const AZ::EntityId entityId, const SnapQueryVolume& volume, AZStd::vector<SnapVertex>& out)
        {
            EntityCache* cache = AcquireCache(entityId, /*needTopology=*/false);
            if (cache == nullptr)
            {
                return;
            }

            for (const Block& block : cache->m_positionBlocks)
            {
                if (!volume.IntersectsSphere(block.m_center, block.m_radius))
                {
                    continue;
                }

                for (AZ::u32 index = block.m_begin; index < block.m_end; ++index)
                {
                    const AZ::Vector3& worldPosition = cache->m_worldPositions[index];
                    if (volume.Contains(worldPosition))
                    {
                        out.emplace_back(worldPosition, entityId, aznumeric_cast<AZ::s64>(index));
                    }
                }
            }
        }

        void VisibleGeometrySnapSource::CollectSnapEdges(
            const AZ::EntityId entityId, const SnapQueryVolume& volume, AZStd::vector<SnapEdge>& out)
        {
            EntityCache* cache = AcquireCache(entityId, /*needTopology=*/true);
            if (cache == nullptr)
            {
                return;
            }

            for (const Block& block : cache->m_edgeBlocks)
            {
                if (!volume.IntersectsSphere(block.m_center, block.m_radius))
                {
                    continue;
                }

                for (AZ::u32 index = block.m_begin; index < block.m_end; ++index)
                {
                    const SnapEdge& edge = cache->m_edges[index];
                    if (volume.IntersectsSegment(edge.m_start, edge.m_end))
                    {
                        out.push_back(edge);
                    }
                }
            }
        }

        void VisibleGeometrySnapSource::CollectSnapEdgeMidpoints(
            const AZ::EntityId entityId, const SnapQueryVolume& volume, AZStd::vector<SnapVertex>& out)
        {
            EntityCache* cache = AcquireCache(entityId, /*needTopology=*/true);
            if (cache == nullptr)
            {
                return;
            }

            // Derived on the fly rather than stored - it is one add and one multiply, against a
            // whole extra array per cached mesh.
            for (const Block& block : cache->m_edgeBlocks)
            {
                if (!volume.IntersectsSphere(block.m_center, block.m_radius))
                {
                    continue;
                }

                for (AZ::u32 index = block.m_begin; index < block.m_end; ++index)
                {
                    const SnapEdge& edge = cache->m_edges[index];
                    const AZ::Vector3 midpoint = (edge.m_start + edge.m_end) * 0.5f;

                    if (volume.Contains(midpoint))
                    {
                        out.emplace_back(midpoint, entityId, edge.m_sourceIndex);
                    }
                }
            }
        }

        void VisibleGeometrySnapSource::CollectSnapFaceCenters(
            const AZ::EntityId entityId, const SnapQueryVolume& volume, AZStd::vector<SnapVertex>& out)
        {
            EntityCache* cache = AcquireCache(entityId, /*needTopology=*/true);
            if (cache == nullptr)
            {
                return;
            }

            for (const Block& block : cache->m_faceCenterBlocks)
            {
                if (!volume.IntersectsSphere(block.m_center, block.m_radius))
                {
                    continue;
                }

                for (AZ::u32 index = block.m_begin; index < block.m_end; ++index)
                {
                    const AZ::Vector3& center = cache->m_faceCenters[index];
                    if (volume.Contains(center))
                    {
                        out.emplace_back(center, entityId, aznumeric_cast<AZ::s64>(index));
                    }
                }
            }
        }
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
