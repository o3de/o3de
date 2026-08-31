/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

// Phase 7 streaming-page builder invariants (see
// docs/superpowers/specs/2026-08-31-meshlets-streaming-paging-design.md §1).
// Self-containment is the load-bearing property: rendering a resident page must
// touch nothing outside its payload, so every payload is decoded here and checked
// bit-for-bit against the monolithic (fallback) data.

#include <AzTest/AzTest.h>
#include <cstring>
#include <limits>
#include <AzCore/std/containers/unordered_set.h>
#include <Builders/MeshletPackBuilderCore.h>
#include <Builders/SourceMeshSet.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <Meshlets/Reflect/MeshletPackReader.h>

namespace UnitTest
{
    using namespace AZ::Meshlets;
    using namespace AZ::Meshlets::Builders;

    namespace
    {
        SourceMeshSet MakePagedGridFixture()
        {
            SourceMeshSet s;
            s.m_sourceModelAssetId = AZ::Data::AssetId(AZ::Uuid("{55555555-5555-5555-5555-555555555555}"), 0);
            s.m_maxVerticesPerCluster  = 64;
            s.m_maxTrianglesPerCluster = 64;
            s.m_generateClusterDag = true;
            s.m_generatePages = true;

            SourceMesh m;
            m.m_name = "paged_grid";
            const int gridDim = 64;
            for (int y = 0; y < gridDim; ++y)
            {
                for (int x = 0; x < gridDim; ++x)
                {
                    m.m_positions.push_back(static_cast<float>(x));
                    m.m_positions.push_back(static_cast<float>(y));
                    m.m_positions.push_back(0.0f);
                    m.m_normals.push_back(0); m.m_normals.push_back(0); m.m_normals.push_back(1);
                    m.m_tangents.push_back(1); m.m_tangents.push_back(0); m.m_tangents.push_back(0); m.m_tangents.push_back(1);
                    m.m_bitangents.push_back(0); m.m_bitangents.push_back(1); m.m_bitangents.push_back(0);
                    m.m_uv0.push_back(static_cast<float>(x) / gridDim);
                    m.m_uv0.push_back(static_cast<float>(y) / gridDim);
                }
            }
            for (int y = 0; y < gridDim - 1; ++y)
            {
                for (int x = 0; x < gridDim - 1; ++x)
                {
                    const AZ::u32 i0 = y * gridDim + x;
                    const AZ::u32 i1 = i0 + 1;
                    const AZ::u32 i2 = i0 + gridDim;
                    const AZ::u32 i3 = i2 + 1;
                    m.m_indices.push_back(i0); m.m_indices.push_back(i1); m.m_indices.push_back(i2);
                    m.m_indices.push_back(i1); m.m_indices.push_back(i3); m.m_indices.push_back(i2);
                }
            }
            SourceMeshLods meshLods;
            meshLods.m_name = m.m_name;
            meshLods.m_lods.push_back(AZStd::move(m));
            s.m_meshes.push_back(AZStd::move(meshLods));
            return s;
        }
    }

    TEST(MeshletPackBuilderPages, PagedPackInvariantsAndSelfContainment)
    {
        const BuildResult r = BuildPackBytes(MakePagedGridFixture());
        ASSERT_TRUE(r.m_success) << r.m_errorMessage.c_str();

        MeshletPackReader reader;
        ASSERT_TRUE(reader.Parse(r.m_packBytes.data(), r.m_packBytes.size()));
        FileHeader header{};
        memcpy(&header, r.m_packBytes.data(), sizeof(header));
        EXPECT_EQ(header.m_version, PackVersionPaged);
        ASSERT_TRUE(reader.HasSection(SectionKind::PageTable));
        ASSERT_TRUE(reader.HasSection(SectionKind::PageData));
        ASSERT_TRUE(reader.HasSection(SectionKind::ParentIndex));
        // Duplicate-fallback: the full v3 layout must still be present.
        ASSERT_TRUE(reader.HasSection(SectionKind::DagNodes));
        ASSERT_TRUE(reader.HasSection(SectionKind::TriangleIndices));

        auto meshDesc = reader.GetSection(SectionKind::MeshDescriptors);
        MeshDescriptorLodEntry lod{};
        memcpy(&lod, meshDesc.data() + sizeof(MeshDescriptorPrefix), sizeof(lod));
        const AZ::u32 leafCount = lod.m_clusterCount;
        ASSERT_GT(leafCount, PageMaxClusters);   // multiple pages expected

        auto clusterBytes = reader.GetSection(SectionKind::ClusterDescriptors);
        auto triBytes = reader.GetSection(SectionKind::TriangleIndices);
        auto indirBytes = reader.GetSection(SectionKind::VertexIndirection);
        auto dagBytes = reader.GetSection(SectionKind::DagNodes);
        auto vsBytes = reader.GetSection(SectionKind::VertexStreams);
        auto tableBytes = reader.GetSection(SectionKind::PageTable);
        auto dataBytes = reader.GetSection(SectionKind::PageData);
        auto parentBytes = reader.GetSection(SectionKind::ParentIndex);

        const auto* clusters = reinterpret_cast<const ClusterDescriptor*>(clusterBytes.data());
        const auto* tris = reinterpret_cast<const AZ::u32*>(triBytes.data());
        const auto* indir = reinterpret_cast<const AZ::u32*>(indirBytes.data());
        const auto* dagNodes = reinterpret_cast<const DagNodeRecord*>(dagBytes.data());
        const auto* pages = reinterpret_cast<const PageTableRecord*>(tableBytes.data());
        const size_t pageCount = tableBytes.size() / sizeof(PageTableRecord);
        const auto* parents = reinterpret_cast<const AZ::u32*>(parentBytes.data());
        ASSERT_GT(pageCount, 1u);
        ASSERT_EQ(parentBytes.size() / sizeof(AZ::u32), lod.m_dagClusterCount);

        // Positions stream base (descriptor [0] is Position at a known offset).
        const auto* vsDescs = reinterpret_cast<const VertexStreamDescriptor*>(
            vsBytes.data() + sizeof(VertexStreamSubHeader));
        const auto* packPositions = reinterpret_cast<const float*>(vsBytes.data() + vsDescs[0].m_byteOffsetInSection);

        AZStd::unordered_set<AZ::u32> pagedClusters;
        AZ::u32 leafPageCount = 0;
        AZ::u32 interiorPageCount = 0;
        for (size_t pg = 0; pg < pageCount; ++pg)
        {
            const PageTableRecord& rec = pages[pg];
            EXPECT_LE(rec.m_clusterCount, PageMaxClusters);
            EXPECT_GT(rec.m_clusterCount, 0u);
            ASSERT_LE(rec.m_dataOffset + rec.m_dataSize, dataBytes.size());
            if (rec.m_flags & PageFlagAlwaysResident)
            {
                // Interior page: clusters strictly above the leaf range, pinned wanted.
                ++interiorPageCount;
                EXPECT_GE(rec.m_clusterFirst, leafCount);
                EXPECT_EQ(rec.m_maxParentError, std::numeric_limits<float>::max());
            }
            else
            {
                ++leafPageCount;
                EXPECT_LE(rec.m_clusterFirst + rec.m_clusterCount, leafCount) << "leaf pages hold leaves only";
                EXPECT_GT(rec.m_maxParentError, 0.0f);   // every leaf of a dense grid gets a parent
            }
            EXPECT_LE(rec.m_clusterFirst + rec.m_clusterCount, lod.m_dagClusterCount);

            // Every DAG cluster is paged exactly once across all pages.
            for (AZ::u32 c = 0; c < rec.m_clusterCount; ++c)
            {
                EXPECT_TRUE(pagedClusters.insert(rec.m_clusterFirst + c).second);
            }

            // Decode the payload and check self-containment + fallback equivalence.
            const AZ::u8* payload = dataBytes.data() + rec.m_dataOffset;
            const auto* records = reinterpret_cast<const PagedClusterRecord*>(payload);
            const auto* pageTris = reinterpret_cast<const AZ::u32*>(
                payload + rec.m_clusterCount * sizeof(PagedClusterRecord));
            const auto* pageIndir = pageTris + rec.m_triangleWords;
            const auto* pagePositions = reinterpret_cast<const float*>(pageIndir + rec.m_indirCount);

            for (AZ::u32 c = 0; c < rec.m_clusterCount; ++c)
            {
                const ClusterDescriptor& mono = clusters[rec.m_clusterFirst + c];
                const PagedClusterRecord& paged = records[c];
                ASSERT_EQ(paged.m_triangleCount, mono.m_triangleCount);
                ASSERT_EQ(paged.m_vertexCount, mono.m_vertexCount);
                for (AZ::u32 t = 0; t < mono.m_triangleCount; ++t)
                {
                    // Triangle words are verbatim copies (cluster-local 8-bit ids).
                    ASSERT_EQ(pageTris[paged.m_triangleWordFirst + t], tris[mono.m_triangleOffset + t]);
                }
                for (AZ::u32 v = 0; v < mono.m_vertexCount; ++v)
                {
                    const AZ::u32 pageVertex = pageIndir[paged.m_indirFirst + v];
                    ASSERT_LT(pageVertex, rec.m_vertexCount) << "page-local id out of page range";
                    const AZ::u32 packVertex = indir[mono.m_vertexOffset + v];   // pack-global
                    // The duplicated page vertex must equal the monolithic one.
                    for (int a = 0; a < 3; ++a)
                    {
                        ASSERT_EQ(pagePositions[pageVertex * 3 + a], packPositions[packVertex * 3 + a]);
                    }
                }
            }
        }
        EXPECT_EQ(pagedClusters.size(), lod.m_dagClusterCount)
            << "every DAG cluster (leaf AND interior) must be paged exactly once";
        EXPECT_GT(leafPageCount, 1u);
        EXPECT_GT(interiorPageCount, 0u);
        // Every payload must fit the fixed runtime slot.
        for (size_t pg = 0; pg < pageCount; ++pg)
        {
            EXPECT_LE(PageSlotHeaderU32s * sizeof(AZ::u32) + pages[pg].m_dataSize,
                PageSlotU32s * sizeof(AZ::u32)) << "page " << pg << " exceeds the pool slot";
        }

        // ParentIndex: every leaf with a finite parentError points at an interior
        // cluster whose self record equals the leaf's parent record (group-shared).
        const float kInf = std::numeric_limits<float>::max();
        for (AZ::u32 leaf = 0; leaf < leafCount; ++leaf)
        {
            if (dagNodes[leaf].m_parentError == kInf)
            {
                EXPECT_EQ(parents[leaf], 0xFFFFFFFFu);
                continue;
            }
            const AZ::u32 parent = parents[leaf];
            ASSERT_NE(parent, 0xFFFFFFFFu);
            ASSERT_GE(parent, leafCount);
            ASSERT_LT(parent, lod.m_dagClusterCount);
            EXPECT_EQ(dagNodes[parent].m_selfError, dagNodes[leaf].m_parentError);
        }
    }
} // namespace UnitTest
