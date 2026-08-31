/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/containers/set.h>
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
        // Build a deterministic 4-triangle quad (2 triangles per face x 2 faces).
        // 6 vertices, 4 triangles, total 12 source indices.
        SourceMeshSet MakeQuadFixture()
        {
            SourceMeshSet s;
            s.m_sourceModelAssetId = AZ::Data::AssetId(AZ::Uuid("{22222222-2222-2222-2222-222222222222}"), 0);
            SourceMesh m;
            m.m_name = "quad";
            const float positions[] = {
                0,0,0,  1,0,0,  0,1,0,
                1,1,0,  2,0,0,  2,1,0,
            };
            for (float v : positions) { m.m_positions.push_back(v); }
            for (size_t i = 0; i < 6; ++i)
            {
                m.m_normals.push_back(0); m.m_normals.push_back(0); m.m_normals.push_back(1);
                m.m_tangents.push_back(1); m.m_tangents.push_back(0); m.m_tangents.push_back(0); m.m_tangents.push_back(1);
                m.m_bitangents.push_back(0); m.m_bitangents.push_back(1); m.m_bitangents.push_back(0);
                m.m_uv0.push_back(0); m.m_uv0.push_back(0);
            }
            // 4 triangles.
            const AZ::u32 idx[] = { 0,1,2,  1,3,2,  1,4,3,  4,5,3 };
            for (AZ::u32 i : idx) { m.m_indices.push_back(i); }
            SourceMeshLods meshLods;
            meshLods.m_name = m.m_name;
            meshLods.m_lods.push_back(AZStd::move(m));
            s.m_meshes.push_back(AZStd::move(meshLods));
            return s;
        }

        // Build a flat NxN grid mesh big enough (>768 indices) to clear
        // GenerateSimplifiedLod's minimum-size floor, so BuildPackBytes actually
        // generates coarser LODs via meshopt_simplify (a flat grid simplifies very
        // easily, so all kTargetLodCount-1 generated levels are expected to build).
        SourceMeshSet MakeSimplifiableGridFixture()
        {
            SourceMeshSet s;
            s.m_sourceModelAssetId = AZ::Data::AssetId(AZ::Uuid("{33333333-3333-3333-3333-333333333333}"), 0);

            SourceMesh m;
            m.m_name = "lod_grid";
            const int gridDim = 40;   // 40x40 verts, 39*39*2=3042 tris, 9126 indices.
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
                    m.m_uv0.push_back(static_cast<float>(x) / (gridDim - 1));
                    m.m_uv0.push_back(static_cast<float>(y) / (gridDim - 1));
                }
            }
            for (int y = 0; y < gridDim - 1; ++y)
            {
                for (int x = 0; x < gridDim - 1; ++x)
                {
                    AZ::u32 a = y * gridDim + x;
                    AZ::u32 b = a + 1;
                    AZ::u32 c = a + gridDim;
                    AZ::u32 d = c + 1;
                    m.m_indices.push_back(a); m.m_indices.push_back(c); m.m_indices.push_back(b);
                    m.m_indices.push_back(b); m.m_indices.push_back(c); m.m_indices.push_back(d);
                }
            }
            SourceMeshLods meshLods;
            meshLods.m_name = m.m_name;
            meshLods.m_lods.push_back(AZStd::move(m));
            s.m_meshes.push_back(AZStd::move(meshLods));
            return s;
        }

        // Decode pack triangles back into a flat (v0,v1,v2) triangle list using
        // pack-global vertex indices. Mirrors what MeshletsData::GenerateDecodedIndices
        // does today, but reading from the on-disk pack instead of in-memory structs.
        AZStd::vector<AZStd::array<AZ::u32, 3>> DecodePackTriangles(const MeshletPackReader& r)
        {
            auto clustersBytes = r.GetSection(SectionKind::ClusterDescriptors);
            auto trisBytes     = r.GetSection(SectionKind::TriangleIndices);
            auto indirBytes    = r.GetSection(SectionKind::VertexIndirection);

            const auto* clusters    = reinterpret_cast<const ClusterDescriptor*>(clustersBytes.data());
            const auto* tris        = reinterpret_cast<const AZ::u32*>(trisBytes.data());
            const auto* indirection = reinterpret_cast<const AZ::u32*>(indirBytes.data());
            const size_t clusterCount = clustersBytes.size() / sizeof(ClusterDescriptor);

            AZStd::vector<AZStd::array<AZ::u32, 3>> out;
            for (size_t c = 0; c < clusterCount; ++c)
            {
                for (AZ::u32 t = 0; t < clusters[c].m_triangleCount; ++t)
                {
                    AZ::u32 enc = tris[clusters[c].m_triangleOffset + t];
                    AZ::u8 a = (enc >> 0) & 0xff;
                    AZ::u8 b = (enc >> 8) & 0xff;
                    AZ::u8 cc = (enc >> 16) & 0xff;
                    AZStd::array<AZ::u32, 3> globalTri = {
                        indirection[clusters[c].m_vertexOffset + a],
                        indirection[clusters[c].m_vertexOffset + b],
                        indirection[clusters[c].m_vertexOffset + cc]
                    };
                    out.push_back(globalTri);
                }
            }
            return out;
        }
    }

    TEST(MeshletPackBuilderRoundtrip, DecodedTriangleSetEqualsSource)
    {
        SourceMeshSet src = MakeQuadFixture();
        BuildResult r = BuildPackBytes(src);
        ASSERT_TRUE(r.m_success) << r.m_errorMessage.c_str();

        MeshletPackReader reader;
        ASSERT_TRUE(reader.Parse(r.m_packBytes.data(), r.m_packBytes.size()));
        auto decoded = DecodePackTriangles(reader);
        EXPECT_EQ(4u, decoded.size())
            << "Source has 4 triangles; pack should decode to 4 triangles";

        // The vertex order may have been remapped by optimizeVertexFetch. To
        // compare triangle SETS, normalize each triangle to its lowest rotation
        // and compare the vertex POSITIONS (which round-trip exactly because
        // float copy is lossless).
        auto positionsBytes = reader.GetSection(SectionKind::VertexStreams);
        ASSERT_GE(positionsBytes.size(),
                  sizeof(VertexStreamSubHeader) + 5 * sizeof(VertexStreamDescriptor));
        const auto* sub = reinterpret_cast<const VertexStreamSubHeader*>(positionsBytes.data());
        const auto* descs = reinterpret_cast<const VertexStreamDescriptor*>(positionsBytes.data() + sizeof(VertexStreamSubHeader));
        ASSERT_EQ(5u, sub->m_streamCount);
        const auto& posDesc = descs[0];
        ASSERT_EQ(static_cast<AZ::u32>(StreamSemanticKind::Position), posDesc.m_semanticKind);
        const float* packPositions = reinterpret_cast<const float*>(positionsBytes.data() + posDesc.m_byteOffsetInSection);

        AZStd::set<AZStd::tuple<int,int,int,int,int,int,int,int,int>> sourceTriPosSet;
        AZStd::set<AZStd::tuple<int,int,int,int,int,int,int,int,int>> packTriPosSet;
        auto sortedKey = [](float x0, float y0, float z0,
                            float x1, float y1, float z1,
                            float x2, float y2, float z2)
        {
            // Normalize triangle order: sort the three (x,y,z) tuples
            // lexicographically. Pack into ints (bit_cast) for set membership.
            float ax[3] = {x0,x1,x2}, ay[3] = {y0,y1,y2}, az[3] = {z0,z1,z2};
            // Simple insertion sort of 3 entries by (x,y,z).
            for (int i = 1; i < 3; ++i)
            {
                for (int j = i; j > 0; --j)
                {
                    if (std::tie(ax[j-1],ay[j-1],az[j-1]) > std::tie(ax[j],ay[j],az[j]))
                    {
                        std::swap(ax[j-1], ax[j]); std::swap(ay[j-1], ay[j]); std::swap(az[j-1], az[j]);
                    }
                }
            }
            auto bits = [](float f){ AZ::u32 u; std::memcpy(&u,&f,4); return static_cast<int>(u); };
            return AZStd::make_tuple(bits(ax[0]),bits(ay[0]),bits(az[0]),
                                     bits(ax[1]),bits(ay[1]),bits(az[1]),
                                     bits(ax[2]),bits(ay[2]),bits(az[2]));
        };

        // Source triangles (LOD0).
        const SourceMesh& srcLod0 = src.m_meshes[0].m_lods[0];
        for (size_t i = 0; i < srcLod0.m_indices.size(); i += 3)
        {
            AZ::u32 a = srcLod0.m_indices[i];
            AZ::u32 b = srcLod0.m_indices[i+1];
            AZ::u32 c = srcLod0.m_indices[i+2];
            const auto& p = srcLod0.m_positions;
            sourceTriPosSet.insert(sortedKey(p[a*3],p[a*3+1],p[a*3+2],
                                             p[b*3],p[b*3+1],p[b*3+2],
                                             p[c*3],p[c*3+1],p[c*3+2]));
        }
        // Pack triangles.
        for (auto tri : decoded)
        {
            packTriPosSet.insert(sortedKey(
                packPositions[tri[0]*3], packPositions[tri[0]*3+1], packPositions[tri[0]*3+2],
                packPositions[tri[1]*3], packPositions[tri[1]*3+1], packPositions[tri[1]*3+2],
                packPositions[tri[2]*3], packPositions[tri[2]*3+1], packPositions[tri[2]*3+2]));
        }

        // Compare via size + element-membership rather than direct EXPECT_EQ(set, set).
        // AZStd::set<tuple<...>> operator== routes through AZStd::equal which has
        // multiple overloads that confuse the resolver with our 9-int tuple key.
        ASSERT_EQ(sourceTriPosSet.size(), packTriPosSet.size())
            << "Pack and source triangle counts differ";
        for (const auto& tri : sourceTriPosSet)
        {
            EXPECT_TRUE(packTriPosSet.find(tri) != packTriPosSet.end())
                << "Source triangle not present in pack";
        }
    }

    TEST(MeshletPackBuilderRoundtrip, AllTriangleIndicesAreInBoundsOfIndirection)
    {
        SourceMeshSet src = MakeQuadFixture();
        BuildResult r = BuildPackBytes(src);
        ASSERT_TRUE(r.m_success) << r.m_errorMessage.c_str();

        MeshletPackReader reader;
        ASSERT_TRUE(reader.Parse(r.m_packBytes.data(), r.m_packBytes.size()));

        auto clustersBytes = reader.GetSection(SectionKind::ClusterDescriptors);
        auto trisBytes     = reader.GetSection(SectionKind::TriangleIndices);
        auto indirBytes    = reader.GetSection(SectionKind::VertexIndirection);

        const auto* clusters    = reinterpret_cast<const ClusterDescriptor*>(clustersBytes.data());
        const auto* tris        = reinterpret_cast<const AZ::u32*>(trisBytes.data());
        const size_t clusterCount = clustersBytes.size() / sizeof(ClusterDescriptor);
        const size_t indirCount   = indirBytes.size() / sizeof(AZ::u32);

        for (size_t c = 0; c < clusterCount; ++c)
        {
            for (AZ::u32 t = 0; t < clusters[c].m_triangleCount; ++t)
            {
                AZ::u32 enc = tris[clusters[c].m_triangleOffset + t];
                AZ::u8 a = (enc >> 0) & 0xff;
                AZ::u8 b = (enc >> 8) & 0xff;
                AZ::u8 cc = (enc >> 16) & 0xff;
                EXPECT_LT(clusters[c].m_vertexOffset + a, indirCount);
                EXPECT_LT(clusters[c].m_vertexOffset + b, indirCount);
                EXPECT_LT(clusters[c].m_vertexOffset + cc, indirCount);
            }
        }
    }

    // Item 2 (geometric-error LOD metric): SectionKind::LodError round-trip.
    TEST(MeshletPackBuilderRoundtrip, LodErrorSectionIsParallelToLodEntriesAndLod0IsZero)
    {
        SourceMeshSet src = MakeSimplifiableGridFixture();
        BuildResult r = BuildPackBytes(src);
        ASSERT_TRUE(r.m_success) << r.m_errorMessage.c_str();

        MeshletPackReader reader;
        ASSERT_TRUE(reader.Parse(r.m_packBytes.data(), r.m_packBytes.size()));

        auto meshDescBytes = reader.GetSection(SectionKind::MeshDescriptors);
        auto lodErrorBytes = reader.GetSection(SectionKind::LodError);
        ASSERT_FALSE(meshDescBytes.empty());
        ASSERT_FALSE(lodErrorBytes.empty());

        // Walk MeshDescriptors the same way the runtime does: one MeshDescriptorPrefix
        // per mesh followed by prefix.m_lodCount MeshDescriptorLodEntry records. Sum the
        // total LOD-entry count so it can be checked against the LodError section size.
        const AZ::u8* p = meshDescBytes.data();
        const AZ::u8* end = p + meshDescBytes.size();
        AZ::u32 totalLodEntries = 0;
        AZ::u16 firstMeshLodCount = 0;
        while (p + sizeof(MeshDescriptorPrefix) <= end)
        {
            const auto* prefix = reinterpret_cast<const MeshDescriptorPrefix*>(p);
            if (totalLodEntries == 0) { firstMeshLodCount = prefix->m_lodCount; }
            totalLodEntries += prefix->m_lodCount;
            p += sizeof(MeshDescriptorPrefix) +
                 static_cast<size_t>(prefix->m_lodCount) * sizeof(MeshDescriptorLodEntry);
        }

        ASSERT_GT(firstMeshLodCount, 1u)
            << "Fixture should have generated at least one coarser LOD via meshopt_simplify";

        const size_t lodErrorCount = lodErrorBytes.size() / sizeof(float);
        ASSERT_EQ(static_cast<size_t>(totalLodEntries), lodErrorCount)
            << "LodError section must have exactly one float per MeshDescriptorLodEntry, "
               "in the same pack-global order";

        const float* lodErrors = reinterpret_cast<const float*>(lodErrorBytes.data());
        // LOD0 (index 0, the first record of the first mesh) is never simplified.
        EXPECT_FLOAT_EQ(0.0f, lodErrors[0]) << "LOD0's error must be exactly 0.0";

        // Every value is a valid meshopt "relative to mesh extents" error: non-negative.
        for (size_t i = 0; i < lodErrorCount; ++i)
        {
            EXPECT_GE(lodErrors[i], 0.0f) << "LodError[" << i << "] must be non-negative";
        }

        // The first GENERATED LOD (index 1 of the first mesh) should show actual
        // simplification error -- otherwise this fixture isn't exercising the metric.
        EXPECT_GT(lodErrors[1], 0.0f) << "LOD1's error should be non-zero for a simplified grid";
    }

    TEST(MeshletPackBuilderRoundtrip, LodErrorSectionIsAllZeroForSingleLodMesh)
    {
        // The small quad fixture is below GenerateSimplifiedLod's minimum-size floor,
        // so it stays at a single LOD (LOD0, never simplified). The LodError section
        // is still written (one 0.0 record per LOD entry -- see BuildPackBytes) but must
        // never contain a non-zero value here; the runtime's fallback-to-screen-coverage
        // decision (MeshRenderData::HasLodError) is a separate, section-presence check,
        // not a value check, so this guards the "always well-defined, never garbage"
        // half of that contract.
        SourceMeshSet src = MakeQuadFixture();
        BuildResult r = BuildPackBytes(src);
        ASSERT_TRUE(r.m_success) << r.m_errorMessage.c_str();

        MeshletPackReader reader;
        ASSERT_TRUE(reader.Parse(r.m_packBytes.data(), r.m_packBytes.size()));
        auto lodErrorBytes = reader.GetSection(SectionKind::LodError);
        const size_t lodErrorCount = lodErrorBytes.size() / sizeof(float);
        ASSERT_EQ(1u, lodErrorCount) << "Single-LOD mesh should have exactly one LodError record";

        const float* lodErrors = reinterpret_cast<const float*>(lodErrorBytes.data());
        EXPECT_FLOAT_EQ(0.0f, lodErrors[0]);
    }
}
