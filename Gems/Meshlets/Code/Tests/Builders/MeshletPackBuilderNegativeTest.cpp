/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <Builders/MeshletPackBuilderCore.h>

namespace UnitTest
{
    using namespace AZ::Meshlets::Builders;

    namespace
    {
        SourceMeshSet OneTriangle()
        {
            SourceMeshSet s;
            s.m_sourceModelAssetId = AZ::Data::AssetId(AZ::Uuid("{33333333-3333-3333-3333-333333333333}"), 0);
            SourceMesh m;
            m.m_name = "tri";
            m.m_positions  = { 0,0,0, 1,0,0, 0,1,0 };
            m.m_normals    = { 0,0,1, 0,0,1, 0,0,1 };
            m.m_tangents   = { 1,0,0,1, 1,0,0,1, 1,0,0,1 };
            m.m_bitangents = { 0,1,0, 0,1,0, 0,1,0 };
            m.m_uv0        = { 0,0, 1,0, 0,1 };
            m.m_indices    = { 0, 1, 2 };
            SourceMeshLods meshLods;
            meshLods.m_name = m.m_name;
            meshLods.m_lods.push_back(AZStd::move(m));
            s.m_meshes.push_back(AZStd::move(meshLods));
            return s;
        }
    }

    TEST(MeshletPackBuilderNegative, EmptyMeshSetFailsCleanly)
    {
        SourceMeshSet s;
        s.m_sourceModelAssetId = AZ::Data::AssetId(AZ::Uuid("{44444444-4444-4444-4444-444444444444}"), 0);
        BuildResult r = BuildPackBytes(s);
        EXPECT_FALSE(r.m_success);
        EXPECT_FALSE(r.m_errorMessage.empty());
        EXPECT_TRUE(r.m_packBytes.empty());
    }

    TEST(MeshletPackBuilderNegative, MismatchedStreamCountFailsCleanly)
    {
        SourceMeshSet s = OneTriangle();
        s.m_meshes[0].m_lods[0].m_normals.pop_back();  // 8 floats now, expected 9
        BuildResult r = BuildPackBytes(s);
        EXPECT_FALSE(r.m_success);
        EXPECT_FALSE(r.m_errorMessage.empty());
    }

    TEST(MeshletPackBuilderNegative, MaxVerticesTooSmallFailsCleanly)
    {
        // meshopt_buildMeshlets asserts max_vertices >= 3. BuildOneMesh's documented
        // contract is to CLAMP any out-of-range cluster budget to meshoptimizer's real
        // limits rather than fail the asset build, so that a bad import-rule or sidecar
        // value degrades gracefully instead of breaking content.
        //
        // This test previously asserted the opposite (that the build fails with a
        // "0 meshlets" error). It had never actually run -- the Meshlets.Builders.Tests
        // module had no AZ_UNIT_TEST_HOOK, so AzTestRunner could not execute any of it --
        // and when it was first executed it crashed inside meshoptimizer, because the
        // clamp's lower bound was 1 rather than 3 and let max_vertices=2 through.
        // The clamp is now correct; this asserts the documented clamping behaviour.
        SourceMeshSet s = OneTriangle();
        s.m_maxVerticesPerCluster = 2;
        s.m_maxTrianglesPerCluster = 1;
        BuildResult r = BuildPackBytes(s);
        EXPECT_TRUE(r.m_success) << r.m_errorMessage.c_str();
    }
}
