/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <Builders/MeshletPackBuilderCore.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <Meshlets/Reflect/MeshletPackReader.h>
#include <cstring>

namespace UnitTest
{
    using namespace AZ::Meshlets;
    using namespace AZ::Meshlets::Builders;

    static SourceMeshSet TwoTriangles()
    {
        SourceMeshSet s;
        s.m_sourceModelAssetId = AZ::Data::AssetId(AZ::Uuid("{55555555-5555-5555-5555-555555555555}"), 0);
        SourceMesh m;
        m.m_name = "twoTri";
        m.m_positions  = { 0,0,0, 1,0,0, 0,1,0, 1,1,0 };
        m.m_normals    = { 0,0,1, 0,0,1, 0,0,1, 0,0,1 };
        m.m_tangents   = { 1,0,0,1, 1,0,0,1, 1,0,0,1, 1,0,0,1 };
        m.m_bitangents = { 0,1,0, 0,1,0, 0,1,0, 0,1,0 };
        m.m_uv0        = { 0,0, 1,0, 0,1, 1,1 };
        m.m_indices    = { 0,1,2,  1,3,2 };
        SourceMeshLods meshLods;
        meshLods.m_name = m.m_name;
        meshLods.m_lods.push_back(AZStd::move(m));
        s.m_meshes.push_back(AZStd::move(meshLods));
        return s;
    }

    TEST(MeshletPackBuilderFormatCompliance, CoreSp1SectionsPresent)
    {
        BuildResult r = BuildPackBytes(TwoTriangles());
        ASSERT_TRUE(r.m_success) << r.m_errorMessage.c_str();
        MeshletPackReader reader;
        ASSERT_TRUE(reader.Parse(r.m_packBytes.data(), r.m_packBytes.size()));

        // The six core SP1 sections are mandatory and must always be emitted.
        EXPECT_TRUE(reader.HasSection(SectionKind::PackHeader));
        EXPECT_TRUE(reader.HasSection(SectionKind::MeshDescriptors));
        EXPECT_TRUE(reader.HasSection(SectionKind::ClusterDescriptors));
        EXPECT_TRUE(reader.HasSection(SectionKind::TriangleIndices));
        EXPECT_TRUE(reader.HasSection(SectionKind::VertexIndirection));
        EXPECT_TRUE(reader.HasSection(SectionKind::VertexStreams));

        // ConeBounds (cluster backface culling) and LodError (geometric-error LOD
        // selection) were added by later phases and are now emitted as well.
        //
        // This originally asserted GetSectionCount() == 6 and that ConeBounds was
        // absent. It had never run -- the module had no AZ_UNIT_TEST_HOOK -- so it
        // silently encoded the SP1-era format across every subsequent format change.
        // Asserting an exact section count makes every additive section a test
        // failure, which is the opposite of what the format's additive design wants,
        // so the mandatory sections are checked individually and only genuinely
        // unimplemented kinds are asserted absent.
        EXPECT_TRUE(reader.HasSection(SectionKind::ConeBounds));
        EXPECT_TRUE(reader.HasSection(SectionKind::LodError));
        EXPECT_FALSE(reader.HasSection(SectionKind::DagNodes));
    }

    TEST(MeshletPackBuilderFormatCompliance, PackHeaderRecordHasExpectedSourceAssetIdAndConstants)
    {
        SourceMeshSet src = TwoTriangles();
        BuildResult r = BuildPackBytes(src);
        ASSERT_TRUE(r.m_success);
        MeshletPackReader reader;
        ASSERT_TRUE(reader.Parse(r.m_packBytes.data(), r.m_packBytes.size()));
        auto bytes = reader.GetSection(SectionKind::PackHeader);
        ASSERT_EQ(sizeof(PackHeaderRecord), bytes.size());
        PackHeaderRecord h;
        std::memcpy(&h, bytes.data(), sizeof(h));
        EXPECT_EQ(0, std::memcmp(h.m_sourceModelGuid,
                                  src.m_sourceModelAssetId.m_guid.begin(),
                                  sizeof(h.m_sourceModelGuid)));
        EXPECT_EQ(src.m_sourceModelAssetId.m_subId, h.m_sourceModelSubId);
        EXPECT_EQ(1u, h.m_meshCount);
        EXPECT_EQ(64u, h.m_maxVerticesPerCluster);
        EXPECT_EQ(64u, h.m_maxTrianglesPerCluster);
        EXPECT_FLOAT_EQ(0.5f, h.m_coneWeight);
    }

    TEST(MeshletPackBuilderFormatCompliance, MaterialIdReservedForSp5IsInvalidMarker)
    {
        BuildResult r = BuildPackBytes(TwoTriangles());
        ASSERT_TRUE(r.m_success);
        MeshletPackReader reader;
        ASSERT_TRUE(reader.Parse(r.m_packBytes.data(), r.m_packBytes.size()));
        auto bytes = reader.GetSection(SectionKind::MeshDescriptors);
        ASSERT_GE(bytes.size(), sizeof(MeshDescriptorPrefix) + sizeof(MeshDescriptorLodEntry));
        const auto* lod = reinterpret_cast<const MeshDescriptorLodEntry*>(
            bytes.data() + sizeof(MeshDescriptorPrefix));
        EXPECT_EQ(InvalidMaterialId, lod->m_materialId)
            << "SP1 must emit InvalidMaterialId until SP5 wires up cluster->material map";
    }
}
