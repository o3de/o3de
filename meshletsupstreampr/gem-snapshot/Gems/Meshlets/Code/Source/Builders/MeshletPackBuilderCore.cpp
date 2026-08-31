/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Builders/MeshletPackBuilderCore.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <Meshlets/Reflect/MeshletPackWriter.h>

#include <Atom/RHI.Reflect/Format.h>

#include <meshoptimizer.h>

#include <AzCore/std/containers/unordered_map.h>

#include <cstring>
#include <limits>

namespace AZ::Meshlets::Builders
{
    namespace
    {
        // Re-laid-out vertex streams + meshlet output for a single source mesh.
        struct PerMeshOutput
        {
            // Re-laid-out vertex streams (after meshopt_optimizeVertexFetch).
            AZStd::vector<float> m_positions;
            AZStd::vector<float> m_normals;
            AZStd::vector<float> m_tangents;
            AZStd::vector<float> m_bitangents;
            AZStd::vector<float> m_uv0;
            AZ::u32 m_vertexCount = 0;

            // Meshlet output (cluster set for this mesh's LOD 0).
            AZStd::vector<ClusterDescriptor> m_clusters;
            AZStd::vector<ClusterBoundsRecord> m_clusterBounds; //!< Parallel to m_clusters (Phase 6 culling).
            AZStd::vector<AZ::u32> m_encodedTriangles;  //!< 3×8-bit packed in u32.
            AZStd::vector<AZ::u32> m_vertexIndirection; //!< meshlet-local → mesh-global.

            // AABB.
            float m_aabbMin[3] = {  std::numeric_limits<float>::max(),
                                    std::numeric_limits<float>::max(),
                                    std::numeric_limits<float>::max() };
            float m_aabbMax[3] = { -std::numeric_limits<float>::max(),
                                   -std::numeric_limits<float>::max(),
                                   -std::numeric_limits<float>::max() };

            //! LodError section value for this LOD (0.0 for LOD0/baked LODs; see
            //! GenerateSimplifiedLod). Set by the caller after BuildOneMesh returns
            //! (BuildOneMesh itself has no notion of simplification error).
            float m_lodError = 0.0f;

            //! Phase 6 cluster DAG (BuildMeshDag): parallel to m_clusters; empty for
            //! non-DAG meshes. Leaves occupy [0, m_leafClusterCount); interior levels
            //! are appended after (leaf-first layout).
            AZStd::vector<DagNodeRecord> m_dagNodes;
            AZ::u32 m_leafClusterCount = 0;   //!< 0 = no DAG built for this mesh.

            //! Phase 7 pages: per-cluster (mesh-local) FIRST-parent cluster index,
            //! parallel to m_clusters; 0xFFFFFFFF = DAG root / no parent. Filled by
            //! BuildMeshDag only when pages are requested.
            AZStd::vector<AZ::u32> m_parentIndex;

            //! Phase 7: one self-contained leaf streaming page (see PageTableRecord's
            //! payload layout comment). m_leafFirst is MESH-LOCAL; assembly rebases it.
            struct PerMeshPage
            {
                AZ::u32 m_leafFirst = 0;
                AZ::u32 m_leafCount = 0;
                AZ::u32 m_vertexCount = 0;
                AZ::u32 m_triangleWords = 0;
                AZ::u32 m_indirCount = 0;
                float m_aabbMin[3] = { 0, 0, 0 };
                float m_aabbMax[3] = { 0, 0, 0 };
                float m_maxParentError = 0.0f;
                AZ::u32 m_flags = 0;   //!< PageFlagAlwaysResident for interior-level pages.
                AZStd::vector<AZ::u8> m_payload;
            };
            AZStd::vector<PerMeshPage> m_pages;

            //! Interior DAG level ranges (mesh-local {first, count}), recorded by
            //! BuildMeshDag so PageInteriorLevels can slice them into always-resident
            //! pages without any permutation (they are contiguous by emission).
            AZStd::vector<AZStd::pair<AZ::u32, AZ::u32>> m_dagLevelRanges;
        };

        AZ::u32 EncodeTriangleByteTriplet(unsigned char a, unsigned char b, unsigned char c)
        {
            // Lowest-24-bits encoding; top byte zero. Same shape as today's
            // MeshletsData::EncodeTrianglesData. Parens needed (operator
            // precedence bug history).
            return ((static_cast<AZ::u32>(a) << 0) |
                    (static_cast<AZ::u32>(b) << 8) |
                    (static_cast<AZ::u32>(c) << 16)) & 0x00FFFFFFu;
        }

        bool BuildOneMesh(const SourceMesh& src, AZ::u16 maxVerts, AZ::u16 maxTris,
                          float coneWeight, PerMeshOutput& out, AZStd::string& error)
        {
            // Clamp the cluster budgets to meshoptimizer's hard limits so any config
            // value (import rule or .meshlet sidecar) is safe. meshopt_buildMeshlets
            // asserts: max_vertices in [3,255] (a cluster must be able to hold at least
            // one triangle; cluster-local vertex indices are u8 in the 3xu8 triangle
            // encoding) and max_triangles in [1,512] AND a multiple of 4 (the triangle
            // byte buffer is 4-aligned). Out-of-range values would otherwise assert/UB
            // inside the clusterizer. Bigger clusters => fewer clusters => fewer
            // per-cluster draw commands + better vertex-cache reuse.
            //
            // The lower bound was 1, not 3, which is not a safe clamp: max_vertices of
            // 1 or 2 passed the guard and then tripped meshopt_buildMeshlets' own
            // `max_vertices >= 3` assert. Caught by MaxVerticesTooSmallFailsCleanly once
            // that suite became runnable -- it had no AzTest hook, so it had never
            // executed since it was written.
            {
                const AZ::u16 reqVerts = maxVerts;
                const AZ::u16 reqTris  = maxTris;
                if (maxVerts < 3)   { maxVerts = 3; }
                if (maxVerts > 255) { maxVerts = 255; }
                if (maxTris > 512)  { maxTris = 512; }
                maxTris = static_cast<AZ::u16>(maxTris & ~3u);    // round DOWN to a multiple of 4
                if (maxTris < 4)    { maxTris = 4; }
                AZ_Warning("Meshlets", reqVerts == maxVerts && reqTris == maxTris,
                    "BuildOneMesh '%s': clamped cluster budget verts %u->%u, tris %u->%u "
                    "(meshopt limits: verts<=255, tris<=512 & multiple-of-4).",
                    src.m_name.c_str(), reqVerts, maxVerts, reqTris, maxTris);
            }

            const size_t vertexCount = src.m_positions.size() / 3;
            if (vertexCount == 0 || src.m_indices.empty())
            {
                error = AZStd::string::format("Mesh '%s' has empty vertex/index data",
                                              src.m_name.c_str());
                return false;
            }
            if (src.m_normals.size()    != vertexCount * 3 ||
                src.m_tangents.size()   != vertexCount * 4 ||
                src.m_bitangents.size() != vertexCount * 3 ||
                src.m_uv0.size()        != vertexCount * 2)
            {
                error = AZStd::string::format("Mesh '%s' stream counts do not match vertex count",
                                              src.m_name.c_str());
                return false;
            }
            // Indices must come in triangle triples — meshoptimizer assumes
            // index_count is divisible by 3 and will read past the buffer
            // otherwise.
            if ((src.m_indices.size() % 3) != 0)
            {
                error = AZStd::string::format(
                    "Mesh '%s' has %zu indices which is not a multiple of 3",
                    src.m_name.c_str(), src.m_indices.size());
                return false;
            }
            // Every index must be < vertexCount. meshopt_buildMeshlets internally
            // calls buildTriangleAdjacency which dereferences indices into a
            // per-vertex counts array; an OOB index becomes
            // EXCEPTION_ACCESS_VIOLATION inside meshoptimizer's clusterizer.cpp
            // (no validation — it just trusts the caller). We've seen this on
            // .glb assets where SceneAPI emits sub-meshes whose index buffer
            // outlives a vertex-buffer trim — better to fail this one mesh than
            // crash the whole AssetBuilder process.
            for (size_t i = 0; i < src.m_indices.size(); ++i)
            {
                if (src.m_indices[i] >= vertexCount)
                {
                    error = AZStd::string::format(
                        "Mesh '%s' index[%zu]=%u is out of bounds (vertexCount=%zu) — "
                        "source mesh has malformed topology",
                        src.m_name.c_str(), i,
                        static_cast<unsigned>(src.m_indices[i]),
                        vertexCount);
                    return false;
                }
            }

            // 1. meshopt_optimizeVertexFetch — reorder vertices for cluster locality.
            AZStd::vector<AZ::u32> remappedIndices(src.m_indices);
            AZStd::vector<float> p(src.m_positions);
            AZStd::vector<float> n(src.m_normals);
            AZStd::vector<float> t(src.m_tangents);
            AZStd::vector<float> b(src.m_bitangents);
            AZStd::vector<float> u(src.m_uv0);

            // meshoptimizer's optimizeVertexFetchRemap returns a permutation; we
            // apply it manually to each stream so streams stay separate.
            AZStd::vector<unsigned int> remap(vertexCount);
            const size_t newVertexCount = meshopt_optimizeVertexFetchRemap(
                remap.data(), remappedIndices.data(), remappedIndices.size(), vertexCount);

            auto applyRemap = [&](AZStd::vector<float>& data, size_t componentsPerVertex)
            {
                AZStd::vector<float> dst(newVertexCount * componentsPerVertex);
                meshopt_remapVertexBuffer(dst.data(), data.data(), vertexCount,
                                          sizeof(float) * componentsPerVertex, remap.data());
                data = AZStd::move(dst);
            };
            applyRemap(p, 3);
            applyRemap(n, 3);
            applyRemap(t, 4);
            applyRemap(b, 3);
            applyRemap(u, 2);

            meshopt_remapIndexBuffer(remappedIndices.data(), remappedIndices.data(),
                                     remappedIndices.size(), remap.data());

            // 1b. CHEAP BONUS: optimize the index list for the post-transform
            // vertex cache BEFORE clusterizing. This improves intra-cluster ACMR
            // (meshopt_buildMeshlets respects the existing index order when it
            // greedily packs triangles into clusters). It only reorders triangles
            // — no geometry / vertex-attribute change — so it is safe for every LOD.
            meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(),
                                        remappedIndices.size(), newVertexCount);

            // 2. meshopt_buildMeshlets.
            const size_t maxMeshlets = meshopt_buildMeshletsBound(
                remappedIndices.size(), maxVerts, maxTris);
            AZStd::vector<meshopt_Meshlet> meshlets(maxMeshlets);
            AZStd::vector<unsigned int> meshletVertices(maxMeshlets * maxVerts);
            AZStd::vector<unsigned char> meshletTriangleBytes(maxMeshlets * maxTris * 3);

            const size_t meshletCount = meshopt_buildMeshlets(
                meshlets.data(), meshletVertices.data(), meshletTriangleBytes.data(),
                remappedIndices.data(), remappedIndices.size(),
                p.data(), newVertexCount, sizeof(float) * 3,
                maxVerts, maxTris, coneWeight);

            if (meshletCount == 0)
            {
                error = AZStd::string::format(
                    "Mesh '%s' produced 0 meshlets (max_vertices=%u max_triangles=%u may be too small)",
                    src.m_name.c_str(), maxVerts, maxTris);
                return false;
            }

            meshlets.resize(meshletCount);
            const meshopt_Meshlet& last = meshlets.back();
            meshletVertices.resize(last.vertex_offset + last.vertex_count);
            meshletTriangleBytes.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3u));

            // 3. Encode triangles 3×u8 → u32, rewrite triangle_offset from
            //    byte-offset into u32-offset (one u32 per triangle). Also compute
            //    per-cluster bounds (Phase 6 GPU culling): a bounding sphere
            //    (frustum cull) and a normal cone (backface cull) via meshopt.
            AZStd::vector<AZ::u32> encoded;
            for (const meshopt_Meshlet& m : meshlets)
            {
                ClusterDescriptor c;
                c.m_vertexOffset   = m.vertex_offset;
                c.m_triangleOffset = static_cast<AZ::u32>(encoded.size());
                c.m_vertexCount    = m.vertex_count;
                c.m_triangleCount  = m.triangle_count;
                out.m_clusters.push_back(c);

                // Per-cluster bounds from meshopt. Inputs are this meshlet's slice
                // of the vertex-indirection and triangle-byte arrays plus the
                // re-laid-out position stream (p, 3 floats/vertex).
                const meshopt_Bounds bounds = meshopt_computeMeshletBounds(
                    &meshletVertices[m.vertex_offset],
                    &meshletTriangleBytes[m.triangle_offset],
                    m.triangle_count,
                    p.data(), newVertexCount, sizeof(float) * 3);

                ClusterBoundsRecord cb{};
                cb.m_center[0]  = bounds.center[0];
                cb.m_center[1]  = bounds.center[1];
                cb.m_center[2]  = bounds.center[2];
                cb.m_radius     = bounds.radius;
                cb.m_coneApex[0]= bounds.cone_apex[0];
                cb.m_coneApex[1]= bounds.cone_apex[1];
                cb.m_coneApex[2]= bounds.cone_apex[2];
                cb.m_coneCutoff = bounds.cone_cutoff;
                cb.m_coneAxis[0]= bounds.cone_axis[0];
                cb.m_coneAxis[1]= bounds.cone_axis[1];
                cb.m_coneAxis[2]= bounds.cone_axis[2];
                cb.m_pad        = 0.0f;
                out.m_clusterBounds.push_back(cb);

                for (AZ::u32 i = 0; i < m.triangle_count; ++i)
                {
                    const unsigned char* tri = &meshletTriangleBytes[m.triangle_offset + i * 3];
                    encoded.push_back(EncodeTriangleByteTriplet(tri[0], tri[1], tri[2]));
                }
            }
            out.m_encodedTriangles = AZStd::move(encoded);

            // 4. Indirection.
            out.m_vertexIndirection.assign(meshletVertices.begin(), meshletVertices.end());

            // 5. Streams + counts + AABB.
            out.m_positions  = AZStd::move(p);
            out.m_normals    = AZStd::move(n);
            out.m_tangents   = AZStd::move(t);
            out.m_bitangents = AZStd::move(b);
            out.m_uv0        = AZStd::move(u);
            out.m_vertexCount = static_cast<AZ::u32>(newVertexCount);

            for (size_t v = 0; v < newVertexCount; ++v)
            {
                const float x = out.m_positions[v * 3 + 0];
                const float y = out.m_positions[v * 3 + 1];
                const float z = out.m_positions[v * 3 + 2];
                out.m_aabbMin[0] = AZStd::GetMin(out.m_aabbMin[0], x);
                out.m_aabbMin[1] = AZStd::GetMin(out.m_aabbMin[1], y);
                out.m_aabbMin[2] = AZStd::GetMin(out.m_aabbMin[2], z);
                out.m_aabbMax[0] = AZStd::GetMax(out.m_aabbMax[0], x);
                out.m_aabbMax[1] = AZStd::GetMax(out.m_aabbMax[1], y);
                out.m_aabbMax[2] = AZStd::GetMax(out.m_aabbMax[2], z);
            }

            return true;
        }

        // Target number of LODs per logical mesh. Levels beyond what the source
        // model ships are generated from LOD0 via meshopt_simplify.
        constexpr AZ::u32 kTargetLodCount = 4;

        // Simplification ratio per generated LOD relative to LOD0's index count.
        // Index [0] is LOD1, [1] is LOD2, [2] is LOD3 (LOD0 is never simplified).
        // Length must be >= kTargetLodCount-1.
        constexpr float kLodIndexRatios[3] = { 0.5f, 0.25f, 0.1f };
        static_assert(
            sizeof(kLodIndexRatios) / sizeof(kLodIndexRatios[0]) >= (kTargetLodCount - 1),
            "kLodIndexRatios must supply a ratio for every generated LOD (kTargetLodCount-1).");

        // Generate a coarser SourceMesh from \p lod0 by collapsing its index
        // buffer with meshopt_simplify to \p ratio of LOD0's index count. The
        // simplified index buffer references a SUBSET of LOD0's vertices, so all
        // vertex attributes (normal/tangent/bitangent/UV) are PRESERVED verbatim
        // for the surviving vertices — no attribute re-derivation. Unreferenced
        // vertices are left in place; BuildOneMesh's meshopt_optimizeVertexFetch
        // pass compacts them out when it builds the LOD's streams.
        //
        // \p outNormalizedError receives meshopt_simplify's resulting error,
        // which meshoptimizer documents as already "relative to mesh extents"
        // (dimensionless, [0,1] range) — i.e. already scale-independent, the same
        // property the LodError pack section wants. No further AABB-diagonal
        // division is needed on top of what meshopt already normalizes by.
        //
        // Returns false if simplify could not meaningfully reduce the mesh
        // (result index count >= ~95% of the input) — the caller stops adding
        // LODs for that mesh (fewer LODs is fine). \p outNormalizedError is left
        // at 0.0 in that case.
        bool GenerateSimplifiedLod(const SourceMesh& lod0, float ratio, SourceMesh& out, float& outNormalizedError)
        {
            outNormalizedError = 0.0f;
            const size_t vertexCount = lod0.m_positions.size() / 3;
            const size_t indexCount = lod0.m_indices.size();
            if (vertexCount == 0 || indexCount < 3)
            {
                return false;
            }

            // Don't bother generating LODs for tiny meshes: the simplifier can't
            // remove meaningful detail and a degenerate collapse on a handful of
            // triangles just wastes pack space. ~256 triangles (768 indices) is the
            // floor below which LOD generation is skipped. This also keeps small
            // unit-test fixtures (a quad, a couple of triangles) at a single LOD.
            constexpr size_t kMinIndicesForLodGen = 768;
            if (indexCount < kMinIndicesForLodGen)
            {
                return false;
            }

            size_t targetIndexCount = static_cast<size_t>(indexCount * ratio);
            targetIndexCount -= (targetIndexCount % 3);   // keep a multiple of 3
            if (targetIndexCount < 3)
            {
                targetIndexCount = 3;
            }
            if (targetIndexCount >= indexCount)
            {
                return false;   // nothing to collapse at this ratio
            }

            // target_error is a relative error bound (fraction of mesh extent).
            // 0.01-0.05 keeps the silhouette close while allowing real collapse.
            const float targetError = 0.05f;
            float resultError = 0.0f;
            AZStd::vector<AZ::u32> simplified(indexCount);   // worst case = input size
            const size_t newIndexCount = meshopt_simplify(
                simplified.data(),
                lod0.m_indices.data(), indexCount,
                lod0.m_positions.data(), vertexCount, sizeof(float) * 3,
                targetIndexCount, targetError, /*options*/ 0u, &resultError);

            // meshopt_simplify returns ~the input size when it cannot collapse the
            // mesh to anywhere near the target (locked topology / already minimal).
            // Treat "did not shrink by at least 5%" as "no more useful LODs".
            if (newIndexCount == 0 || newIndexCount >= (indexCount - indexCount / 20))
            {
                return false;
            }
            simplified.resize(newIndexCount);

            // The simplified mesh shares LOD0's vertex arrays (attributes intact);
            // only the index buffer changes. BuildOneMesh compacts unused verts.
            out.m_name       = lod0.m_name;
            out.m_positions  = lod0.m_positions;
            out.m_normals    = lod0.m_normals;
            out.m_tangents   = lod0.m_tangents;
            out.m_bitangents = lod0.m_bitangents;
            out.m_uv0        = lod0.m_uv0;
            out.m_indices    = AZStd::move(simplified);
            outNormalizedError = resultError;
            return true;
        }

        // ================================================================
        // Phase 6 — cluster-simplification DAG (see
        // docs/superpowers/specs/2026-08-31-meshlets-phase6-cluster-dag-lod-design.md).
        // Runs AFTER BuildOneMesh, entirely in the mesh's compacted (post-
        // optimizeVertexFetch) vertex space: simplification only ever selects a
        // SUBSET of existing vertices, so every DAG level shares the mesh's one
        // vertex slice and only the cluster/triangle/indirection arrays grow.
        // ================================================================

        //! Clusterize \p indices (mesh-local vertex ids) and APPEND the resulting
        //! meshlets to \p out's cluster/bounds/triangle/indirection arrays — the same
        //! emission BuildOneMesh does for LOD0, with offsets continuing the existing
        //! arrays. Returns the number of clusters appended (0 on failure).
        AZ::u32 AppendClustersFromIndexBuffer(
            const AZ::u32* indices, size_t indexCount,
            AZ::u16 maxVerts, AZ::u16 maxTris, float coneWeight, PerMeshOutput& out)
        {
            if (indexCount < 3)
            {
                return 0;
            }
            const size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, maxVerts, maxTris);
            AZStd::vector<meshopt_Meshlet> meshlets(maxMeshlets);
            AZStd::vector<unsigned int> meshletVertices(maxMeshlets * maxVerts);
            AZStd::vector<unsigned char> meshletTriangleBytes(maxMeshlets * maxTris * 3);

            const size_t meshletCount = meshopt_buildMeshlets(
                meshlets.data(), meshletVertices.data(), meshletTriangleBytes.data(),
                indices, indexCount,
                out.m_positions.data(), out.m_vertexCount, sizeof(float) * 3,
                maxVerts, maxTris, coneWeight);
            if (meshletCount == 0)
            {
                return 0;
            }

            for (size_t mi = 0; mi < meshletCount; ++mi)
            {
                const meshopt_Meshlet& m = meshlets[mi];

                ClusterDescriptor c;
                c.m_vertexOffset   = static_cast<AZ::u32>(out.m_vertexIndirection.size());
                c.m_triangleOffset = static_cast<AZ::u32>(out.m_encodedTriangles.size());
                c.m_vertexCount    = m.vertex_count;
                c.m_triangleCount  = m.triangle_count;
                out.m_clusters.push_back(c);

                const meshopt_Bounds bounds = meshopt_computeMeshletBounds(
                    &meshletVertices[m.vertex_offset],
                    &meshletTriangleBytes[m.triangle_offset],
                    m.triangle_count,
                    out.m_positions.data(), out.m_vertexCount, sizeof(float) * 3);

                ClusterBoundsRecord cb{};
                cb.m_center[0]  = bounds.center[0];
                cb.m_center[1]  = bounds.center[1];
                cb.m_center[2]  = bounds.center[2];
                cb.m_radius     = bounds.radius;
                cb.m_coneApex[0]= bounds.cone_apex[0];
                cb.m_coneApex[1]= bounds.cone_apex[1];
                cb.m_coneApex[2]= bounds.cone_apex[2];
                cb.m_coneCutoff = bounds.cone_cutoff;
                cb.m_coneAxis[0]= bounds.cone_axis[0];
                cb.m_coneAxis[1]= bounds.cone_axis[1];
                cb.m_coneAxis[2]= bounds.cone_axis[2];
                cb.m_pad        = 0.0f;
                out.m_clusterBounds.push_back(cb);

                for (AZ::u32 i = 0; i < m.vertex_count; ++i)
                {
                    out.m_vertexIndirection.push_back(meshletVertices[m.vertex_offset + i]);
                }
                for (AZ::u32 i = 0; i < m.triangle_count; ++i)
                {
                    const unsigned char* tri = &meshletTriangleBytes[m.triangle_offset + i * 3];
                    out.m_encodedTriangles.push_back(EncodeTriangleByteTriplet(tri[0], tri[1], tri[2]));
                }
            }
            return static_cast<AZ::u32>(meshletCount);
        }

        //! Append cluster \p clusterIdx's triangles to \p outIndices as mesh-local
        //! vertex-index triples (decodes the 3x8-bit cluster-local encoding through
        //! the cluster's indirection slice).
        void ExpandClusterIndices(const PerMeshOutput& out, AZ::u32 clusterIdx, AZStd::vector<AZ::u32>& outIndices)
        {
            const ClusterDescriptor& c = out.m_clusters[clusterIdx];
            for (AZ::u32 t = 0; t < c.m_triangleCount; ++t)
            {
                const AZ::u32 enc = out.m_encodedTriangles[c.m_triangleOffset + t];
                outIndices.push_back(out.m_vertexIndirection[c.m_vertexOffset + ((enc      ) & 0xffu)]);
                outIndices.push_back(out.m_vertexIndirection[c.m_vertexOffset + ((enc >>  8) & 0xffu)]);
                outIndices.push_back(out.m_vertexIndirection[c.m_vertexOffset + ((enc >> 16) & 0xffu)]);
            }
        }

        //! Build the cluster DAG on top of \p out's leaf clusters (which BuildOneMesh
        //! just produced). Appends interior-level clusters (leaf-first layout) and
        //! fills m_dagNodes / m_leafClusterCount. Crack-freedom invariants (design §2):
        //! locked group boundaries, ONE shared error/sphere per simplification group
        //! (stamped on every parent's self record and every member's parent record),
        //! and max()-propagated monotonic errors.
        void BuildMeshDag(AZ::u16 maxVerts, AZ::u16 maxTris, float coneWeight, PerMeshOutput& out)
        {
            constexpr AZ::u32 kDagMaxLevels        = 16;
            constexpr size_t  kDagTargetGroupSize  = 8;    // clusters per simplification group
            constexpr float   kInfError            = std::numeric_limits<float>::max();

            // Same clamp BuildOneMesh applies (meshopt asserts otherwise).
            if (maxVerts < 3)   { maxVerts = 3; }
            if (maxVerts > 255) { maxVerts = 255; }
            if (maxTris > 512)  { maxTris = 512; }
            maxTris = static_cast<AZ::u16>(maxTris & ~3u);
            if (maxTris < 4)    { maxTris = 4; }

            const AZ::u32 leafCount = static_cast<AZ::u32>(out.m_clusters.size());
            out.m_leafClusterCount = leafCount;
            out.m_dagNodes.clear();
            out.m_dagNodes.reserve(leafCount * 2);
            out.m_dagLevelRanges.clear();
            out.m_parentIndex.assign(leafCount, 0xFFFFFFFFu);
            for (AZ::u32 i = 0; i < leafCount; ++i)
            {
                // Leaves: selfError 0 (always eligible; sphere then irrelevant to the
                // <= test but kept meaningful for debug views), parent open until a
                // group simplifies them.
                const ClusterBoundsRecord& b = out.m_clusterBounds[i];
                DagNodeRecord n{};
                n.m_selfSphere[0] = b.m_center[0];
                n.m_selfSphere[1] = b.m_center[1];
                n.m_selfSphere[2] = b.m_center[2];
                n.m_selfSphere[3] = b.m_radius;
                n.m_parentSphere[0] = b.m_center[0];
                n.m_parentSphere[1] = b.m_center[1];
                n.m_parentSphere[2] = b.m_center[2];
                n.m_parentSphere[3] = b.m_radius;
                n.m_selfError   = 0.0f;
                n.m_parentError = kInfError;
                out.m_dagNodes.push_back(n);
            }

            AZ::u32 levelFirst = 0;
            AZ::u32 levelCount = leafCount;
            for (AZ::u32 level = 0; level < kDagMaxLevels && levelCount > 1; ++level)
            {
                // ---- Partition this level's clusters into adjacency groups ----
                AZStd::vector<unsigned int> clusterVertexIds;
                AZStd::vector<unsigned int> clusterVertexCounts(levelCount);
                for (AZ::u32 i = 0; i < levelCount; ++i)
                {
                    const ClusterDescriptor& c = out.m_clusters[levelFirst + i];
                    clusterVertexCounts[i] = c.m_vertexCount;
                    for (AZ::u32 v = 0; v < c.m_vertexCount; ++v)
                    {
                        clusterVertexIds.push_back(out.m_vertexIndirection[c.m_vertexOffset + v]);
                    }
                }
                AZStd::vector<unsigned int> partitionIds(levelCount);
                const size_t partitionCount = meshopt_partitionClusters(
                    partitionIds.data(),
                    clusterVertexIds.data(), clusterVertexIds.size(),
                    clusterVertexCounts.data(), levelCount,
                    out.m_positions.data(), out.m_vertexCount, sizeof(float) * 3,
                    kDagTargetGroupSize);

                AZStd::vector<AZStd::vector<AZ::u32>> groups(partitionCount);   // absolute cluster ids
                for (AZ::u32 i = 0; i < levelCount; ++i)
                {
                    groups[partitionIds[i]].push_back(levelFirst + i);
                }

                // ---- Simplify each group to ~half and emit its parent clusters ----
                const AZ::u32 nextLevelFirst = static_cast<AZ::u32>(out.m_clusters.size());
                AZ::u32 parentsEmitted = 0;
                AZStd::vector<AZ::u32> mergedIndices;
                AZStd::vector<AZ::u32> simplified;
                AZStd::vector<float> memberSphereCenters;   // float3 per member
                AZStd::vector<float> memberSphereRadii;
                for (const AZStd::vector<AZ::u32>& members : groups)
                {
                    if (members.size() < 2)
                    {
                        continue;   // nothing to merge — stays a root of the DAG
                    }
                    mergedIndices.clear();
                    for (AZ::u32 member : members)
                    {
                        ExpandClusterIndices(out, member, mergedIndices);
                    }

                    size_t targetIndexCount = mergedIndices.size() / 2;
                    targetIndexCount -= (targetIndexCount % 3);
                    if (targetIndexCount < 3)
                    {
                        continue;
                    }

                    // LOCK_BORDER: group-boundary vertices are pinned -> adjacent groups
                    // simplified independently still meet exactly (THE crack mechanism).
                    // SPARSE: indices are a small subset of the whole mesh's vertex set.
                    // ERROR_ABSOLUTE: resultError comes back in object-space units, which
                    // is what the runtime's pixel projection expects.
                    static constexpr float kNormalAttrWeights[3] = { 0.5f, 0.5f, 0.5f };
                    float resultError = 0.0f;
                    simplified.resize(mergedIndices.size());
                    const size_t newIndexCount = meshopt_simplifyWithAttributes(
                        simplified.data(), mergedIndices.data(), mergedIndices.size(),
                        out.m_positions.data(), out.m_vertexCount, sizeof(float) * 3,
                        out.m_normals.data(), sizeof(float) * 3,
                        kNormalAttrWeights, 3,
                        /*vertex_lock*/ nullptr,
                        targetIndexCount, /*target_error*/ std::numeric_limits<float>::max(),
                        meshopt_SimplifyLockBorder | meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute,
                        &resultError);

                    // "Did not shrink by at least ~15%" => locked borders dominate this
                    // group; keep its clusters as DAG roots rather than emitting a
                    // near-duplicate level (design §9: the DAG just ends shallower).
                    if (newIndexCount == 0 || newIndexCount >= mergedIndices.size() - mergedIndices.size() / 7)
                    {
                        continue;
                    }

                    // Group-shared, monotonic error + enclosing sphere (design §2).
                    float groupError = resultError;
                    memberSphereCenters.clear();
                    memberSphereRadii.clear();
                    for (AZ::u32 member : members)
                    {
                        const DagNodeRecord& mn = out.m_dagNodes[member];
                        groupError = AZStd::GetMax(groupError, mn.m_selfError);
                        memberSphereCenters.push_back(mn.m_selfSphere[0]);
                        memberSphereCenters.push_back(mn.m_selfSphere[1]);
                        memberSphereCenters.push_back(mn.m_selfSphere[2]);
                        memberSphereRadii.push_back(mn.m_selfSphere[3]);
                    }
                    const meshopt_Bounds groupBounds = meshopt_computeSphereBounds(
                        memberSphereCenters.data(), members.size(), sizeof(float) * 3,
                        memberSphereRadii.data(), sizeof(float));

                    const AZ::u32 firstParentIndex = static_cast<AZ::u32>(out.m_clusters.size());
                    const AZ::u32 parentCount = AppendClustersFromIndexBuffer(
                        simplified.data(), newIndexCount, maxVerts, maxTris, coneWeight, out);
                    if (parentCount == 0)
                    {
                        continue;   // members stay roots
                    }
                    parentsEmitted += parentCount;
                    // Phase 7: exact group->children mapping (ParentIndex section) —
                    // every member records its group's FIRST parent (parents are
                    // contiguous); the new parents start as roots.
                    out.m_parentIndex.resize(out.m_clusters.size(), 0xFFFFFFFFu);
                    for (AZ::u32 member : members)
                    {
                        out.m_parentIndex[member] = firstParentIndex;
                    }

                    for (AZ::u32 p = 0; p < parentCount; ++p)
                    {
                        DagNodeRecord n{};
                        n.m_selfSphere[0] = groupBounds.center[0];
                        n.m_selfSphere[1] = groupBounds.center[1];
                        n.m_selfSphere[2] = groupBounds.center[2];
                        n.m_selfSphere[3] = groupBounds.radius;
                        n.m_parentSphere[0] = groupBounds.center[0];
                        n.m_parentSphere[1] = groupBounds.center[1];
                        n.m_parentSphere[2] = groupBounds.center[2];
                        n.m_parentSphere[3] = groupBounds.radius;
                        n.m_selfError   = groupError;
                        n.m_parentError = kInfError;   // open until (possibly) grouped next level
                        out.m_dagNodes.push_back(n);
                    }
                    for (AZ::u32 member : members)
                    {
                        DagNodeRecord& mn = out.m_dagNodes[member];
                        mn.m_parentSphere[0] = groupBounds.center[0];
                        mn.m_parentSphere[1] = groupBounds.center[1];
                        mn.m_parentSphere[2] = groupBounds.center[2];
                        mn.m_parentSphere[3] = groupBounds.radius;
                        mn.m_parentError = groupError;
                    }
                }

                if (parentsEmitted == 0)
                {
                    break;   // no group could simplify — DAG complete
                }
                levelFirst = nextLevelFirst;
                levelCount = parentsEmitted;
                out.m_dagLevelRanges.emplace_back(levelFirst, levelCount);
            }

            AZ_TracePrintf("Meshlets",
                "Meshlets DAG build: %u leaf + %zu interior clusters (%zu total)\n",
                leafCount, out.m_clusters.size() - leafCount, out.m_clusters.size());
        }

        // ================================================================
        // Phase 7 — leaf streaming pages (see
        // docs/superpowers/specs/2026-08-31-meshlets-streaming-paging-design.md).
        // ================================================================

        //! Partition the mesh's LEAF clusters into pages (spatial adjacency via
        //! meshopt_partitionClusters, hard-capped at PageMaxClusters) and PERMUTE the
        //! leaf prefix of m_clusters/m_clusterBounds into page order so each page's
        //! clusters are contiguous. MUST run after BuildOneMesh and BEFORE BuildMeshDag:
        //! descriptors are self-describing (a permutation touches no offsets) and the
        //! DAG is built on top of whatever leaf order exists, but DagNodes/ParentIndex
        //! entries could not be permuted safely afterwards.
        void PartitionLeafPages(PerMeshOutput& out)
        {
            const AZ::u32 leafCount = static_cast<AZ::u32>(out.m_clusters.size());
            out.m_pages.clear();
            if (leafCount == 0)
            {
                return;
            }

            AZStd::vector<unsigned int> clusterVertexIds;
            AZStd::vector<unsigned int> clusterVertexCounts(leafCount);
            for (AZ::u32 i = 0; i < leafCount; ++i)
            {
                const ClusterDescriptor& c = out.m_clusters[i];
                clusterVertexCounts[i] = c.m_vertexCount;
                for (AZ::u32 v = 0; v < c.m_vertexCount; ++v)
                {
                    clusterVertexIds.push_back(out.m_vertexIndirection[c.m_vertexOffset + v]);
                }
            }
            AZStd::vector<unsigned int> partitionIds(leafCount);
            const size_t partitionCount = meshopt_partitionClusters(
                partitionIds.data(), clusterVertexIds.data(), clusterVertexIds.size(),
                clusterVertexCounts.data(), leafCount,
                out.m_positions.data(), out.m_vertexCount, sizeof(float) * 3,
                PageMaxClusters);

            // Gather members per partition, splitting anything over the hard cap
            // (meshopt may exceed the target by up to a third).
            AZStd::vector<AZStd::vector<AZ::u32>> pages;
            {
                AZStd::vector<AZStd::vector<AZ::u32>> partitions(partitionCount);
                for (AZ::u32 i = 0; i < leafCount; ++i)
                {
                    partitions[partitionIds[i]].push_back(i);
                }
                for (AZStd::vector<AZ::u32>& part : partitions)
                {
                    for (size_t start = 0; start < part.size(); start += PageMaxClusters)
                    {
                        const size_t end = AZStd::GetMin(part.size(), start + PageMaxClusters);
                        pages.emplace_back(part.begin() + start, part.begin() + end);
                    }
                }
            }

            // Permute the leaf prefix into page order.
            AZStd::vector<ClusterDescriptor> newClusters;
            AZStd::vector<ClusterBoundsRecord> newBounds;
            newClusters.reserve(leafCount);
            newBounds.reserve(leafCount);
            for (const AZStd::vector<AZ::u32>& page : pages)
            {
                PerMeshOutput::PerMeshPage rec;
                rec.m_leafFirst = static_cast<AZ::u32>(newClusters.size());
                rec.m_leafCount = static_cast<AZ::u32>(page.size());
                for (AZ::u32 oldIdx : page)
                {
                    newClusters.push_back(out.m_clusters[oldIdx]);
                    newBounds.push_back(out.m_clusterBounds[oldIdx]);
                }
                out.m_pages.push_back(AZStd::move(rec));
            }
            out.m_clusters = AZStd::move(newClusters);
            out.m_clusterBounds = AZStd::move(newBounds);
        }

        //! Slice the interior DAG levels into always-resident pages: plain contiguous
        //! runs of <= PageMaxClusters (no spatial partition, no permutation — they are
        //! pinned resident, so locality does not matter; what matters is that EVERY
        //! cluster the AS path can draw has a pool slot, which is what lets the
        //! monolithic buffers eventually be dropped for the AS path). Runs after
        //! BuildMeshDag (needs the level ranges); payloads build with the leaf pages.
        void PageInteriorLevels(PerMeshOutput& out)
        {
            for (const auto& [levelFirst, levelCount] : out.m_dagLevelRanges)
            {
                for (AZ::u32 start = 0; start < levelCount; start += PageMaxClusters)
                {
                    PerMeshOutput::PerMeshPage rec;
                    rec.m_leafFirst = levelFirst + start;
                    rec.m_leafCount = AZStd::GetMin(PageMaxClusters, levelCount - start);
                    rec.m_flags = PageFlagAlwaysResident;
                    out.m_pages.push_back(AZStd::move(rec));
                }
            }
        }

        //! Build each page's self-contained payload (PagedClusterRecords + triangle
        //! words + PAGE-LOCAL indirection + duplicated vertex-stream slices) and its
        //! classifier fields. Runs AFTER BuildMeshDag (needs m_dagNodes for
        //! m_maxParentError). Layout documented on PageTableRecord.
        void BuildLeafPagePayloads(PerMeshOutput& out)
        {
            for (PerMeshOutput::PerMeshPage& page : out.m_pages)
            {
                // Page-local vertex dedup: mesh-local id -> page-local id.
                AZStd::unordered_map<AZ::u32, AZ::u32> vertexRemap;
                AZStd::vector<AZ::u32> pageVerts;           // page-local -> mesh-local
                AZStd::vector<PagedClusterRecord> records(page.m_leafCount);
                AZStd::vector<AZ::u32> triWords;
                AZStd::vector<AZ::u32> indirection;
                float aabbMin[3] = {  std::numeric_limits<float>::max(),
                                      std::numeric_limits<float>::max(),
                                      std::numeric_limits<float>::max() };
                float aabbMax[3] = { -std::numeric_limits<float>::max(),
                                     -std::numeric_limits<float>::max(),
                                     -std::numeric_limits<float>::max() };
                float maxParentError = 0.0f;

                for (AZ::u32 i = 0; i < page.m_leafCount; ++i)
                {
                    const AZ::u32 clusterIdx = page.m_leafFirst + i;
                    const ClusterDescriptor& c = out.m_clusters[clusterIdx];
                    PagedClusterRecord& r = records[i];
                    r.m_triangleWordFirst = static_cast<AZ::u32>(triWords.size());
                    r.m_triangleCount     = c.m_triangleCount;
                    r.m_indirFirst        = static_cast<AZ::u32>(indirection.size());
                    r.m_vertexCount       = c.m_vertexCount;

                    for (AZ::u32 t = 0; t < c.m_triangleCount; ++t)
                    {
                        triWords.push_back(out.m_encodedTriangles[c.m_triangleOffset + t]);
                    }
                    for (AZ::u32 v = 0; v < c.m_vertexCount; ++v)
                    {
                        const AZ::u32 meshVertex = out.m_vertexIndirection[c.m_vertexOffset + v];
                        auto it = vertexRemap.find(meshVertex);
                        AZ::u32 pageVertex;
                        if (it == vertexRemap.end())
                        {
                            pageVertex = static_cast<AZ::u32>(pageVerts.size());
                            vertexRemap.emplace(meshVertex, pageVertex);
                            pageVerts.push_back(meshVertex);
                        }
                        else
                        {
                            pageVertex = it->second;
                        }
                        indirection.push_back(pageVertex);
                    }

                    const ClusterBoundsRecord& b = out.m_clusterBounds[clusterIdx];
                    for (int a = 0; a < 3; ++a)
                    {
                        aabbMin[a] = AZStd::GetMin(aabbMin[a], b.m_center[a] - b.m_radius);
                        aabbMax[a] = AZStd::GetMax(aabbMax[a], b.m_center[a] + b.m_radius);
                    }
                    if (clusterIdx < out.m_dagNodes.size())
                    {
                        maxParentError = AZStd::GetMax(maxParentError, out.m_dagNodes[clusterIdx].m_parentError);
                    }
                }

                page.m_vertexCount   = static_cast<AZ::u32>(pageVerts.size());
                page.m_triangleWords = static_cast<AZ::u32>(triWords.size());
                page.m_indirCount    = static_cast<AZ::u32>(indirection.size());
                for (int a = 0; a < 3; ++a)
                {
                    page.m_aabbMin[a] = aabbMin[a];
                    page.m_aabbMax[a] = aabbMax[a];
                }
                // Interior (always-resident) pages classify as permanently wanted;
                // their parent errors are irrelevant to the leaf-page classifier.
                page.m_maxParentError = (page.m_flags & PageFlagAlwaysResident)
                    ? std::numeric_limits<float>::max()
                    : maxParentError;

                // Serialize: records + tris + indirection + 5 vertex streams.
                AZStd::vector<AZ::u8>& payload = page.m_payload;
                payload.clear();
                auto append = [&payload](const void* src, size_t bytes)
                {
                    const auto* p8 = reinterpret_cast<const AZ::u8*>(src);
                    payload.insert(payload.end(), p8, p8 + bytes);
                };
                append(records.data(), records.size() * sizeof(PagedClusterRecord));
                append(triWords.data(), triWords.size() * sizeof(AZ::u32));
                append(indirection.data(), indirection.size() * sizeof(AZ::u32));
                auto appendStream = [&](const AZStd::vector<float>& stream, AZ::u32 components)
                {
                    for (AZ::u32 meshVertex : pageVerts)
                    {
                        append(stream.data() + static_cast<size_t>(meshVertex) * components,
                               sizeof(float) * components);
                    }
                };
                appendStream(out.m_positions, 3);
                appendStream(out.m_normals, 3);
                appendStream(out.m_tangents, 4);
                appendStream(out.m_bitangents, 3);
                appendStream(out.m_uv0, 2);
            }
        }
    }

    BuildResult BuildPackBytes(const SourceMeshSet& source)
    {
        BuildResult r;
        if (source.m_meshes.empty())
        {
            r.m_errorMessage = "SourceMeshSet has no meshes";
            return r;
        }

        // ----------------------------------------------------------------
        // Step 1: build one PerMeshOutput per (logical mesh, LOD slice).
        //
        // For each logical mesh we assemble a LOD chain:
        //   - levels supplied by the source model (m_lods[0..N-1]) are used
        //     directly (the "bake" path);
        //   - if fewer than kTargetLodCount levels exist, the remaining coarser
        //     levels are GENERATED from LOD0 via meshopt_simplify (the
        //     "generate" path). Generation stops early if simplify can't shrink
        //     the mesh further — a mesh may therefore end with < kTargetLodCount
        //     LODs, which is fine.
        // Each LOD is then clusterized by BuildOneMesh independently.
        //
        // m_meshLodOutputs[meshIdx] holds that mesh's built LODs in order
        // (index 0 = LOD0 = finest).
        // ----------------------------------------------------------------
        AZStd::vector<AZStd::vector<PerMeshOutput>> meshLodOutputs(source.m_meshes.size());

        float globalAabbMin[3] = {  std::numeric_limits<float>::max(),
                                    std::numeric_limits<float>::max(),
                                    std::numeric_limits<float>::max() };
        float globalAabbMax[3] = { -std::numeric_limits<float>::max(),
                                   -std::numeric_limits<float>::max(),
                                   -std::numeric_limits<float>::max() };

        for (size_t i = 0; i < source.m_meshes.size(); ++i)
        {
            const SourceMeshLods& meshLods = source.m_meshes[i];
            if (meshLods.m_lods.empty())
            {
                r.m_errorMessage = AZStd::string::format(
                    "Logical mesh %zu ('%s') has no LOD0 source data",
                    i, meshLods.m_name.c_str());
                return r;
            }

            // Assemble the source-mesh LOD chain (bake supplied levels, then
            // generate coarser ones from LOD0 until we reach kTargetLodCount or
            // simplify gives up).
            // lodErrorChain[i] parallels lodChain[i]: the LodError section value for
            // that LOD (0.0 for LOD0 and for source-supplied/"baked" LODs, which the
            // builder never runs meshopt_simplify on; meshopt's own resultError for
            // generated LODs).
            AZStd::vector<SourceMesh> lodChain;
            AZStd::vector<float> lodErrorChain;
            lodChain.reserve(kTargetLodCount);
            lodErrorChain.reserve(kTargetLodCount);
            // Phase 6 cluster DAG: the DAG *is* the LOD system — a single "LOD" entry
            // whose cluster range holds every DAG level (leaves first). The discrete
            // chain (baked or generated) is skipped entirely.
            const AZ::u32 effectiveTargetLodCount = source.m_generateClusterDag ? 1 : kTargetLodCount;
            for (const SourceMesh& srcLod : meshLods.m_lods)
            {
                lodChain.push_back(srcLod);
                lodErrorChain.push_back(0.0f);
                if (lodChain.size() >= effectiveTargetLodCount)
                {
                    break;
                }
            }
            // Generation fallback for the missing coarser levels. Each generated
            // LOD is derived from LOD0 (the finest, attribute-complete level) at a
            // progressively smaller index ratio.
            const SourceMesh& lod0 = meshLods.m_lods[0];
            while (lodChain.size() < effectiveTargetLodCount)
            {
                const AZ::u32 genIndex = static_cast<AZ::u32>(lodChain.size()); // 1..kTargetLodCount-1
                const float ratio = kLodIndexRatios[genIndex - 1];
                SourceMesh generated;
                float generatedError = 0.0f;
                if (!GenerateSimplifiedLod(lod0, ratio, generated, generatedError))
                {
                    // Could not collapse further — stop adding LODs for this mesh.
                    break;
                }
                lodChain.push_back(AZStd::move(generated));
                lodErrorChain.push_back(generatedError);
            }

            // Clusterize each LOD slice. A LOD that fails to build (e.g. a
            // degenerate generated level) is dropped rather than failing the
            // whole pack — but LOD0 failing is fatal.
            meshLodOutputs[i].reserve(lodChain.size());
            for (size_t lodIdx = 0; lodIdx < lodChain.size(); ++lodIdx)
            {
                PerMeshOutput out;
                AZStd::string buildErr;
                if (!BuildOneMesh(lodChain[lodIdx],
                                  source.m_maxVerticesPerCluster,
                                  source.m_maxTrianglesPerCluster,
                                  source.m_coneWeight,
                                  out, buildErr))
                {
                    if (lodIdx == 0)
                    {
                        r.m_errorMessage = AZStd::move(buildErr);
                        return r;   // LOD0 must build.
                    }
                    AZ_Warning("Meshlets", false,
                        "Mesh %zu LOD %zu failed to build (%s); dropping this and any coarser LODs.",
                        i, lodIdx, buildErr.c_str());
                    break;   // stop the chain at the last good LOD.
                }
                out.m_lodError = lodErrorChain[lodIdx];
                if (source.m_generateClusterDag && lodIdx == 0)
                {
                    // Phase 7 pages: leaves must be permuted into contiguous page
                    // order BEFORE the DAG is built on top of them; payloads need the
                    // DAG's parentError fields, so they build after.
                    if (source.m_generatePages)
                    {
                        PartitionLeafPages(out);
                    }
                    BuildMeshDag(source.m_maxVerticesPerCluster,
                                 source.m_maxTrianglesPerCluster,
                                 source.m_coneWeight, out);
                    if (source.m_generatePages)
                    {
                        PageInteriorLevels(out);
                        BuildLeafPagePayloads(out);
                    }
                }
                for (int a = 0; a < 3; ++a)
                {
                    globalAabbMin[a] = AZStd::GetMin(globalAabbMin[a], out.m_aabbMin[a]);
                    globalAabbMax[a] = AZStd::GetMax(globalAabbMax[a], out.m_aabbMax[a]);
                }
                meshLodOutputs[i].push_back(AZStd::move(out));
            }

            AZ_TracePrintf("Meshlets",
                "Meshlets LOD build: mesh %zu -> %zu LOD(s) (source supplied %zu, target %u)\n",
                i, meshLodOutputs[i].size(), meshLods.m_lods.size(),
                static_cast<unsigned>(kTargetLodCount));
        }

        // ---------- Assemble pack-level arrays ----------
        AZStd::vector<ClusterDescriptor> allClusters;
        AZStd::vector<ClusterBoundsRecord> allClusterBounds;  //!< Parallel to allClusters (Phase 6).
        AZStd::vector<DagNodeRecord> allDagNodes;             //!< Parallel to allClusters (Phase 6 DAG; only for v3 packs).
        AZStd::vector<AZ::u32> allParentIndex;                //!< Parallel to allClusters (Phase 7 v4; pack-global first-parent ids).
        AZStd::vector<PageTableRecord> allPageRecords;        //!< Phase 7 v4 streaming pages.
        AZStd::vector<AZ::u8> allPageData;                    //!< Concatenated page payloads (16-aligned).
        AZStd::vector<AZ::u32> allEncodedTris;
        AZStd::vector<AZ::u32> allIndirection;
        AZStd::vector<float> allLodErrors;  //!< Parallel to the MeshDescriptorLodEntry records, in the same order.
        // Per-mesh descriptor bytes: one MeshDescriptorPrefix followed by
        // K_actual MeshDescriptorLodEntry records, in mesh order. The name blob
        // is appended after all prefix/LOD records (offsets are section-relative).
        AZStd::vector<AZ::u8> meshDescPrefixAndLods;
        AZStd::vector<AZ::u8> nameBlob;

        // Aggregate vertex streams: sum over every (mesh, LOD) output — each LOD
        // gets its OWN vertex slice in the pack-global streams.
        AZ::u32 totalVertexCount = 0;
        for (const auto& lods : meshLodOutputs)
        {
            for (const auto& o : lods) { totalVertexCount += o.m_vertexCount; }
        }

        AZStd::vector<float> allPositions  (totalVertexCount * 3);
        AZStd::vector<float> allNormals    (totalVertexCount * 3);
        AZStd::vector<float> allTangents   (totalVertexCount * 4);
        AZStd::vector<float> allBitangents (totalVertexCount * 3);
        AZStd::vector<float> allUv0        (totalVertexCount * 2);

        AZ::u32 vertexCursor  = 0;
        AZ::u32 clusterCursor = 0;
        for (size_t i = 0; i < meshLodOutputs.size(); ++i)
        {
            const AZStd::vector<PerMeshOutput>& lods = meshLodOutputs[i];
            const AZ::u16 lodCount = static_cast<AZ::u16>(lods.size());

            // Mesh descriptor prefix. The prefix AABB unions every LOD's AABB
            // (LOD0 dominates but generated LODs share LOD0's vertex extent).
            MeshDescriptorPrefix prefix{};
            prefix.m_nameOffset = static_cast<AZ::u32>(nameBlob.size());
            prefix.m_nameSize   = static_cast<AZ::u32>(source.m_meshes[i].m_name.size());
            prefix.m_lodCount   = lodCount;   // K_actual (>=1)
            float meshMin[3] = {  std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::max() };
            float meshMax[3] = { -std::numeric_limits<float>::max(),
                                 -std::numeric_limits<float>::max(),
                                 -std::numeric_limits<float>::max() };
            for (const PerMeshOutput& o : lods)
            {
                for (int a = 0; a < 3; ++a)
                {
                    meshMin[a] = AZStd::GetMin(meshMin[a], o.m_aabbMin[a]);
                    meshMax[a] = AZStd::GetMax(meshMax[a], o.m_aabbMax[a]);
                }
            }
            for (int a = 0; a < 3; ++a)
            {
                prefix.m_aabbMin[a] = meshMin[a];
                prefix.m_aabbMax[a] = meshMax[a];
            }
            // Append the prefix bytes now; the K LOD-entry records follow.
            {
                const auto* pb = reinterpret_cast<const AZ::u8*>(&prefix);
                meshDescPrefixAndLods.insert(meshDescPrefixAndLods.end(),
                                             pb, pb + sizeof(MeshDescriptorPrefix));
            }

            // Append name bytes (section-relative offset already recorded above).
            const auto& nm = source.m_meshes[i].m_name;
            nameBlob.insert(nameBlob.end(),
                            reinterpret_cast<const AZ::u8*>(nm.data()),
                            reinterpret_cast<const AZ::u8*>(nm.data() + nm.size()));

            // PER-LOD assembly: each LOD gets its own cluster range, vertex slice,
            // encoded triangles, indirection, and a LOD entry that records them.
            for (const PerMeshOutput& o : lods)
            {
                MeshDescriptorLodEntry lod{};
                lod.m_clusterFirst = clusterCursor;
                // DAG meshes: m_clusterCount = LEAVES only (everything that draws "all
                // clusters" keeps drawing exactly the LOD0 set — no interior overlap);
                // m_dagClusterCount = the full leaf+interior range for the DAG-aware
                // AS path. Non-DAG meshes: total clusters / 0, exactly as before.
                const AZ::u32 totalClusters = static_cast<AZ::u32>(o.m_clusters.size());
                lod.m_clusterCount    = (o.m_leafClusterCount != 0) ? o.m_leafClusterCount : totalClusters;
                lod.m_dagClusterCount = (o.m_leafClusterCount != 0) ? totalClusters : 0;
                lod.m_vertexFirst  = vertexCursor;
                lod.m_vertexCount  = o.m_vertexCount;
                lod.m_materialId   = InvalidMaterialId;  // SP5
                {
                    const auto* lb = reinterpret_cast<const AZ::u8*>(&lod);
                    meshDescPrefixAndLods.insert(meshDescPrefixAndLods.end(),
                                                 lb, lb + sizeof(MeshDescriptorLodEntry));
                }
                // LodError section (Kind 8): one float per LOD entry, appended in the
                // same order the entries above are written.
                allLodErrors.push_back(o.m_lodError);

                // Append cluster descriptors with offsets adjusted to pack-global space.
                for (ClusterDescriptor c : o.m_clusters)
                {
                    c.m_vertexOffset   += static_cast<AZ::u32>(allIndirection.size());
                    c.m_triangleOffset += static_cast<AZ::u32>(allEncodedTris.size());
                    allClusters.push_back(c);
                }

                // Append cluster bounds (parallel to allClusters). Bounds are spatial
                // (object-space sphere + cone), so no index-offset rebasing is needed.
                allClusterBounds.insert(allClusterBounds.end(),
                                        o.m_clusterBounds.begin(),
                                        o.m_clusterBounds.end());

                // Phase 6 DAG nodes (parallel to allClusters; spatial — no rebasing).
                // BuildMeshDag fills one record per cluster for DAG meshes; the section
                // is only written when the pack opts in, so the parallel invariant only
                // has to hold then.
                if (source.m_generateClusterDag)
                {
                    AZ_Assert(o.m_dagNodes.size() == o.m_clusters.size(),
                        "DAG mesh must have one DagNodeRecord per cluster (%zu vs %zu)",
                        o.m_dagNodes.size(), o.m_clusters.size());
                    allDagNodes.insert(allDagNodes.end(), o.m_dagNodes.begin(), o.m_dagNodes.end());
                }

                // Phase 7 pages: rebase per-mesh records into pack-global space.
                if (source.m_generatePages)
                {
                    for (AZ::u32 pi : o.m_parentIndex)
                    {
                        allParentIndex.push_back(pi == 0xFFFFFFFFu ? 0xFFFFFFFFu : pi + clusterCursor);
                    }
                    const AZ::u32 lodEntryIndex = static_cast<AZ::u32>(allLodErrors.size()) - 1;
                    for (const PerMeshOutput::PerMeshPage& page : o.m_pages)
                    {
                        // 16-align each payload inside PageData.
                        const AZ::u64 alignedOffset =
                            (static_cast<AZ::u64>(allPageData.size()) + (SectionAlignment - 1)) &
                            ~static_cast<AZ::u64>(SectionAlignment - 1);
                        allPageData.resize(static_cast<size_t>(alignedOffset), 0);
                        allPageData.insert(allPageData.end(), page.m_payload.begin(), page.m_payload.end());

                        PageTableRecord rec{};
                        rec.m_dataOffset     = alignedOffset;
                        rec.m_dataSize       = static_cast<AZ::u32>(page.m_payload.size());
                        rec.m_lodEntryIndex  = lodEntryIndex;
                        rec.m_clusterFirst   = clusterCursor + page.m_leafFirst;
                        rec.m_clusterCount   = page.m_leafCount;
                        rec.m_vertexCount    = page.m_vertexCount;
                        rec.m_triangleWords  = page.m_triangleWords;
                        rec.m_indirCount     = page.m_indirCount;
                        for (int a = 0; a < 3; ++a)
                        {
                            rec.m_aabbMin[a] = page.m_aabbMin[a];
                            rec.m_aabbMax[a] = page.m_aabbMax[a];
                        }
                        rec.m_maxParentError = page.m_maxParentError;
                        rec.m_flags = page.m_flags;
                        allPageRecords.push_back(rec);
                    }
                }

                // Indirection: rewrite local mesh-relative indices into pack-global
                // indices using THIS LOD's vertex cursor (each LOD has its own slice).
                for (AZ::u32 idx : o.m_vertexIndirection)
                {
                    allIndirection.push_back(idx + vertexCursor);
                }

                allEncodedTris.insert(allEncodedTris.end(),
                                      o.m_encodedTriangles.begin(),
                                      o.m_encodedTriangles.end());

                // Vertex streams (this LOD's own vertices).
                std::memcpy(allPositions.data()  + vertexCursor * 3,
                            o.m_positions.data(),  sizeof(float) * 3 * o.m_vertexCount);
                std::memcpy(allNormals.data()    + vertexCursor * 3,
                            o.m_normals.data(),    sizeof(float) * 3 * o.m_vertexCount);
                std::memcpy(allTangents.data()   + vertexCursor * 4,
                            o.m_tangents.data(),   sizeof(float) * 4 * o.m_vertexCount);
                std::memcpy(allBitangents.data() + vertexCursor * 3,
                            o.m_bitangents.data(), sizeof(float) * 3 * o.m_vertexCount);
                std::memcpy(allUv0.data()        + vertexCursor * 2,
                            o.m_uv0.data(),        sizeof(float) * 2 * o.m_vertexCount);

                vertexCursor  += o.m_vertexCount;
                clusterCursor += static_cast<AZ::u32>(o.m_clusters.size());
            }
        }

        // ---------- Serialize sections ----------
        MeshletPackWriter writer;
        writer.BeginPack();

        // Kind 0 — PackHeader (1 record).
        PackHeaderRecord packHeader{};
        // Write GUID as 16 raw bytes + sub-id (matching the AssetId-as-raw-bytes
        // layout introduced in the Task 2 spec correction).
        std::memcpy(packHeader.m_sourceModelGuid,
                    source.m_sourceModelAssetId.m_guid.begin(),
                    sizeof(packHeader.m_sourceModelGuid));
        packHeader.m_sourceModelSubId         = source.m_sourceModelAssetId.m_subId;
        packHeader.m_reserved0                = 0;
        packHeader.m_meshCount                = static_cast<AZ::u32>(meshLodOutputs.size());
        packHeader.m_maxVerticesPerCluster    = source.m_maxVerticesPerCluster;
        packHeader.m_maxTrianglesPerCluster   = source.m_maxTrianglesPerCluster;
        packHeader.m_coneWeight               = source.m_coneWeight;
        for (int a = 0; a < 3; ++a)
        {
            packHeader.m_aabbMin[a] = globalAabbMin[a];
            packHeader.m_aabbMax[a] = globalAabbMax[a];
        }
        writer.AddSection(SectionKind::PackHeader, &packHeader, sizeof(packHeader));

        // Kind 1 — MeshDescriptors. Layout: for each mesh, one MeshDescriptorPrefix
        // followed by prefix.m_lodCount MeshDescriptorLodEntry records (already
        // assembled into meshDescPrefixAndLods in mesh order during the assembly
        // loop), then the concatenated name blob.
        AZStd::vector<AZ::u8> meshDescBytes;
        meshDescBytes.reserve(meshDescPrefixAndLods.size() + nameBlob.size());
        meshDescBytes.insert(meshDescBytes.end(),
                             meshDescPrefixAndLods.begin(), meshDescPrefixAndLods.end());
        meshDescBytes.insert(meshDescBytes.end(), nameBlob.begin(), nameBlob.end());
        writer.AddSection(SectionKind::MeshDescriptors, meshDescBytes.data(), meshDescBytes.size());

        // Kind 2 — ClusterDescriptors.
        writer.AddSection(SectionKind::ClusterDescriptors,
                          allClusters.data(),
                          allClusters.size() * sizeof(ClusterDescriptor));

        // Kind 6 — ConeBounds (Phase 6 GPU culling): one ClusterBoundsRecord per
        // cluster, in the same pack-global order as ClusterDescriptors. The runtime
        // uploads these to a StructuredBuffer the cull compute reads. Optional —
        // older packs without this section simply skip culling.
        writer.AddSection(SectionKind::ConeBounds,
                          allClusterBounds.data(),
                          allClusterBounds.size() * sizeof(ClusterBoundsRecord));

        // Kind 7 — DagNodes (Phase 6, pack v3): per-cluster DAG cut records, parallel
        // to ClusterDescriptors. Written ONLY for DAG-enabled sidecars so every other
        // pack stays a byte-identical v2 (no global re-bake).
        if (source.m_generateClusterDag)
        {
            writer.SetVersion(PackVersionDag);
            writer.AddSection(SectionKind::DagNodes,
                              allDagNodes.data(),
                              allDagNodes.size() * sizeof(DagNodeRecord));
        }

        // Kind 11/12/14 — streaming pages (Phase 7, pack v4). The v3 monolithic
        // sections above stay in the pack too (duplicate-fallback: streaming OFF
        // renders exactly like a v3 pack).
        if (source.m_generatePages)
        {
            writer.SetVersion(PackVersionPaged);
            writer.AddSection(SectionKind::PageTable,
                              allPageRecords.data(),
                              allPageRecords.size() * sizeof(PageTableRecord));
            writer.AddSection(SectionKind::PageData,
                              allPageData.data(), allPageData.size());
            writer.AddSection(SectionKind::ParentIndex,
                              allParentIndex.data(),
                              allParentIndex.size() * sizeof(AZ::u32));
        }

        // Kind 8 — LodError: one float per MeshDescriptorLodEntry record (see the
        // per-LOD assembly loop above), parallel to those records in the same
        // order. Optional — older packs without this section fall back to
        // screen-coverage LOD selection at runtime.
        writer.AddSection(SectionKind::LodError,
                          allLodErrors.data(),
                          allLodErrors.size() * sizeof(float));

        // Kind 3 — TriangleIndices (encoded).
        writer.AddSection(SectionKind::TriangleIndices,
                          allEncodedTris.data(),
                          allEncodedTris.size() * sizeof(AZ::u32));

        // Kind 4 — VertexIndirection.
        writer.AddSection(SectionKind::VertexIndirection,
                          allIndirection.data(),
                          allIndirection.size() * sizeof(AZ::u32));

        // Kind 13 — ExpandedIndices (SP1 v2).
        //
        // Pre-compute the flat triangle vertex index list here so the runtime
        // doesn't have to do CPU expansion AND doesn't need to rely on the
        // compute pass writing the index buffer (which had a cross-pass
        // UAV->SRV barrier problem on AMD that we could not reliably solve
        // without changing the upload mechanism). Putting the data in the
        // pack means PackInit can point an SRV's m_bufferData directly at
        // pack-asset memory — the exact same upload path that positions,
        // normals, tangents, and bitangents use.
        //
        // Encoding: for each cluster in pack-global order, walk its triangles
        // and emit 3 pack-global vertex indices per triangle (in u32 units).
        // The result is a flat u32 array of total length sum_over_clusters
        // (cluster.triangleCount) * 3. Per-mesh slice starts at 3 * triBase
        // (where triBase is the cluster's pack-global m_triangleOffset).
        {
            AZStd::vector<AZ::u32> expandedIndices;
            // Reserve worst-case total. Each cluster contributes 3 indices per triangle.
            AZ::u64 totalTriangles = 0;
            for (const ClusterDescriptor& c : allClusters)
            {
                totalTriangles += c.m_triangleCount;
            }
            expandedIndices.resize(totalTriangles * 3, 0u);

            for (const ClusterDescriptor& c : allClusters)
            {
                for (AZ::u32 t = 0; t < c.m_triangleCount; ++t)
                {
                    const AZ::u32 encodedTri = allEncodedTris[c.m_triangleOffset + t];
                    const AZ::u32 localX = (encodedTri >>  0) & 0xff;
                    const AZ::u32 localY = (encodedTri >>  8) & 0xff;
                    const AZ::u32 localZ = (encodedTri >> 16) & 0xff;
                    // allIndirection is already pack-global (re-based at line
                    // 308-311 when the cluster was emitted), so the lookup
                    // returns a directly-usable pack-global vertex index.
                    const AZ::u32 vX = allIndirection[c.m_vertexOffset + localX];
                    const AZ::u32 vY = allIndirection[c.m_vertexOffset + localY];
                    const AZ::u32 vZ = allIndirection[c.m_vertexOffset + localZ];
                    const AZ::u64 dst = (AZ::u64)(c.m_triangleOffset + t) * 3;
                    expandedIndices[dst + 0] = vX;
                    expandedIndices[dst + 1] = vY;
                    expandedIndices[dst + 2] = vZ;
                }
            }

            writer.AddSection(SectionKind::ExpandedIndices,
                              expandedIndices.data(),
                              expandedIndices.size() * sizeof(AZ::u32));
        }

        // Kind 5 — VertexStreams (sub-header + 5 descriptors + stream data).
        VertexStreamSubHeader sub{};
        sub.m_totalVertexCount = totalVertexCount;
        sub.m_streamCount      = 5;

        VertexStreamDescriptor descs[5]{};
        AZ::u32 byteCursor = sizeof(VertexStreamSubHeader) + 5 * sizeof(VertexStreamDescriptor);
        auto fillDesc = [&](AZ::u32 i, AZ::RHI::Format fmt, AZ::u32 stride, StreamSemanticKind sem)
        {
            descs[i].m_format              = static_cast<AZ::u32>(fmt);
            descs[i].m_byteOffsetInSection = byteCursor;
            descs[i].m_byteStride          = stride;
            descs[i].m_semanticKind        = static_cast<AZ::u32>(sem);
            byteCursor += stride * totalVertexCount;
        };
        fillDesc(0, AZ::RHI::Format::R32G32B32_FLOAT,    12, StreamSemanticKind::Position);
        fillDesc(1, AZ::RHI::Format::R32G32B32_FLOAT,    12, StreamSemanticKind::Normal);
        fillDesc(2, AZ::RHI::Format::R32G32B32A32_FLOAT, 16, StreamSemanticKind::Tangent);
        fillDesc(3, AZ::RHI::Format::R32G32B32_FLOAT,    12, StreamSemanticKind::Bitangent);
        fillDesc(4, AZ::RHI::Format::R32G32_FLOAT,        8, StreamSemanticKind::UV0);

        AZStd::vector<AZ::u8> vertexSection;
        vertexSection.reserve(byteCursor);
        auto append = [&](const void* src, AZ::u64 sz)
        {
            const auto* p = reinterpret_cast<const AZ::u8*>(src);
            vertexSection.insert(vertexSection.end(), p, p + sz);
        };
        append(&sub, sizeof(sub));
        append(descs, sizeof(descs));
        append(allPositions.data(),  allPositions.size()  * sizeof(float));
        append(allNormals.data(),    allNormals.size()    * sizeof(float));
        append(allTangents.data(),   allTangents.size()   * sizeof(float));
        append(allBitangents.data(), allBitangents.size() * sizeof(float));
        append(allUv0.data(),        allUv0.size()        * sizeof(float));

        writer.AddSection(SectionKind::VertexStreams, vertexSection.data(), vertexSection.size());

        if (!writer.End(r.m_packBytes))
        {
            r.m_errorMessage = "MeshletPackWriter::End failed";
            return r;
        }

        r.m_success = true;
        return r;
    }

} // namespace AZ::Meshlets::Builders
