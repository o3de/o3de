/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <Builders/MeshletPackBuilderCore.h>
#include <Builders/SourceMeshSet.h>

namespace UnitTest
{
    using namespace AZ::Meshlets::Builders;

    namespace
    {
        // Build a deterministic 100-vertex grid with ~150 triangles. Each
        // attribute is generated from the vertex index so the test is
        // self-contained and reproducible.
        SourceMeshSet MakeFixtureGrid()
        {
            SourceMeshSet s;
            s.m_sourceModelAssetId = AZ::Data::AssetId(AZ::Uuid("{11111111-1111-1111-1111-111111111111}"), 0);

            SourceMesh m;
            m.m_name = "fixture_grid";
            const int gridDim = 10;  // 10x10 vertices = 100
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
    }

    TEST(MeshletPackBuilderDeterminism, ByteIdenticalOutputForSameInput)
    {
        SourceMeshSet s1 = MakeFixtureGrid();
        SourceMeshSet s2 = MakeFixtureGrid();

        BuildResult r1 = BuildPackBytes(s1);
        BuildResult r2 = BuildPackBytes(s2);

        ASSERT_TRUE(r1.m_success) << r1.m_errorMessage.c_str();
        ASSERT_TRUE(r2.m_success) << r2.m_errorMessage.c_str();
        ASSERT_EQ(r1.m_packBytes.size(), r2.m_packBytes.size());
        EXPECT_EQ(0, std::memcmp(r1.m_packBytes.data(), r2.m_packBytes.data(),
                                  r1.m_packBytes.size()));
    }
}
