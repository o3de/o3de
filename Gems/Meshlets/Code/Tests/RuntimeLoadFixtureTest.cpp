/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Smoke-test: bake a pack in-test, load it via the asset class, confirm the
 * runtime can read its sections. Does NOT exercise the full RHI bind path --
 * that needs the renderer up and is done in the Phase 6 integration test.
 */

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Meshlets/Reflect/MeshletPackAsset.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <Meshlets/Reflect/MeshletPackWriter.h>
#include <cstring>

namespace UnitTest
{
    using namespace AZ::Meshlets;

    static AZStd::vector<AZ::u8> BakeFixturePack(const AZ::Data::AssetId& sourceId)
    {
        // Hand-build a minimal valid pack with a single 4-vertex 2-triangle quad.
        MeshletPackWriter w;
        w.BeginPack();

        PackHeaderRecord h{};
        // Per Task 2 spec correction: PackHeaderRecord stores AssetId as raw GUID + sub-id.
        std::memcpy(h.m_sourceModelGuid, sourceId.m_guid.begin(), sizeof(h.m_sourceModelGuid));
        h.m_sourceModelSubId = sourceId.m_subId;
        h.m_reserved0 = 0;
        h.m_meshCount             = 1;
        h.m_maxVerticesPerCluster = 64;
        h.m_maxTrianglesPerCluster = 64;
        h.m_coneWeight            = 0.5f;
        h.m_aabbMin[0] = h.m_aabbMin[1] = h.m_aabbMin[2] = 0;
        h.m_aabbMax[0] = h.m_aabbMax[1] = 1;  h.m_aabbMax[2] = 0;
        w.AddSection(SectionKind::PackHeader, &h, sizeof(h));

        MeshDescriptorPrefix prefix{};
        prefix.m_lodCount = 1;
        for (int a = 0; a < 3; ++a) { prefix.m_aabbMin[a] = h.m_aabbMin[a]; prefix.m_aabbMax[a] = h.m_aabbMax[a]; }
        MeshDescriptorLodEntry lod{};
        lod.m_clusterFirst = 0;
        lod.m_clusterCount = 1;
        lod.m_vertexFirst = 0;
        lod.m_vertexCount = 4;
        lod.m_materialId = InvalidMaterialId;
        AZStd::vector<AZ::u8> meshDescBytes;
        meshDescBytes.insert(meshDescBytes.end(),
            reinterpret_cast<AZ::u8*>(&prefix),
            reinterpret_cast<AZ::u8*>(&prefix) + sizeof(prefix));
        meshDescBytes.insert(meshDescBytes.end(),
            reinterpret_cast<AZ::u8*>(&lod),
            reinterpret_cast<AZ::u8*>(&lod) + sizeof(lod));
        w.AddSection(SectionKind::MeshDescriptors, meshDescBytes.data(), meshDescBytes.size());

        ClusterDescriptor cluster{};
        cluster.m_vertexOffset = 0;
        cluster.m_triangleOffset = 0;
        cluster.m_vertexCount = 4;
        cluster.m_triangleCount = 2;
        w.AddSection(SectionKind::ClusterDescriptors, &cluster, sizeof(cluster));

        // 2 triangles encoded: (0,1,2) and (1,3,2).
        AZ::u32 tris[2] = {
            (0 << 0) | (1 << 8) | (2 << 16),
            (1 << 0) | (3 << 8) | (2 << 16),
        };
        w.AddSection(SectionKind::TriangleIndices, tris, sizeof(tris));

        AZ::u32 indir[4] = { 0, 1, 2, 3 };
        w.AddSection(SectionKind::VertexIndirection, indir, sizeof(indir));

        // VertexStreams: sub-header + 5 descriptors + stream data (stub float zeros).
        AZStd::vector<AZ::u8> vs;
        VertexStreamSubHeader sub{}; sub.m_totalVertexCount = 4; sub.m_streamCount = 5;
        vs.insert(vs.end(), reinterpret_cast<AZ::u8*>(&sub), reinterpret_cast<AZ::u8*>(&sub) + sizeof(sub));
        VertexStreamDescriptor descs[5]{};
        AZ::u32 cur = sizeof(sub) + sizeof(descs);
        const AZ::u32 strides[5] = { 12, 12, 16, 12, 8 };
        const AZ::u32 sems[5] = { 0, 1, 2, 3, 4 };
        for (int i = 0; i < 5; ++i)
        {
            descs[i].m_byteOffsetInSection = cur;
            descs[i].m_byteStride = strides[i];
            descs[i].m_semanticKind = sems[i];
            cur += strides[i] * 4;
        }
        vs.insert(vs.end(), reinterpret_cast<AZ::u8*>(&descs[0]),
                  reinterpret_cast<AZ::u8*>(&descs[0]) + sizeof(descs));
        // Append zero-fill stream data.
        const AZ::u32 streamBytes = (12 + 12 + 16 + 12 + 8) * 4;
        vs.insert(vs.end(), streamBytes, AZ::u8(0));
        w.AddSection(SectionKind::VertexStreams, vs.data(), vs.size());

        AZStd::vector<AZ::u8> bytes;
        AZ_Assert(w.End(bytes), "writer should always succeed in test");
        return bytes;
    }

    TEST(RuntimeLoadFixture, AssetClassReadsBakedPackSections)
    {
        AZ::Data::AssetId src(AZ::Uuid::CreateRandom(), 0);
        AZStd::vector<AZ::u8> bytes = BakeFixturePack(src);

        MeshletPackAsset asset;
        ASSERT_TRUE(asset.LoadFromBuffer(AZStd::move(bytes)));
        const PackHeaderRecord* read = asset.GetPackHeader();
        ASSERT_NE(nullptr, read);

        // Reconstruct AssetId from raw GUID + sub-id (Task 2 spec correction).
        AZ::Uuid readGuid;
        std::memcpy(&readGuid, read->m_sourceModelGuid, sizeof(read->m_sourceModelGuid));
        const AZ::Data::AssetId readModelId(readGuid, read->m_sourceModelSubId);
        EXPECT_EQ(src, readModelId);

        EXPECT_EQ(1u, read->m_meshCount);
    }
}
