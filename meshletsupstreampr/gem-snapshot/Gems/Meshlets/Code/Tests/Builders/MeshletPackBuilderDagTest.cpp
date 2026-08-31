/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

// Phase 6 cluster-DAG builder invariants (see
// docs/superpowers/specs/2026-08-31-meshlets-phase6-cluster-dag-lod-design.md §2/§7).
// These are the crack-freedom guarantees the runtime cut test relies on; a
// violation here shows up on screen as holes/overlaps, so they get a hard test.

#include <AzTest/AzTest.h>
#include <AzCore/Math/Vector3.h>
#include <cstring>
#include <limits>
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
        // Dense flat grid: simplifies trivially, so the DAG is expected to build
        // several interior levels. Small cluster budget (64/64) forces many leaves.
        SourceMeshSet MakeDagGridFixture(bool generateDag)
        {
            SourceMeshSet s;
            s.m_sourceModelAssetId = AZ::Data::AssetId(AZ::Uuid("{44444444-4444-4444-4444-444444444444}"), 0);
            s.m_maxVerticesPerCluster  = 64;
            s.m_maxTrianglesPerCluster = 64;
            s.m_generateClusterDag = generateDag;

            SourceMesh m;
            m.m_name = "dag_grid";
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

        struct ParsedDagPack
        {
            AZ::u32 m_version = 0;
            MeshDescriptorLodEntry m_lod{};
            AZStd::span<const AZ::u8> m_dagBytes;
            AZStd::span<const AZ::u8> m_clusterBytes;
        };

        bool ParsePack(const AZStd::vector<AZ::u8>& packBytes, MeshletPackReader& reader, ParsedDagPack& out)
        {
            if (!reader.Parse(packBytes.data(), packBytes.size()))
            {
                return false;
            }
            FileHeader header{};
            memcpy(&header, packBytes.data(), sizeof(header));
            out.m_version = header.m_version;

            auto meshDesc = reader.GetSection(SectionKind::MeshDescriptors);
            if (meshDesc.size() < sizeof(MeshDescriptorPrefix) + sizeof(MeshDescriptorLodEntry))
            {
                return false;
            }
            memcpy(&out.m_lod, meshDesc.data() + sizeof(MeshDescriptorPrefix), sizeof(MeshDescriptorLodEntry));
            out.m_dagBytes = reader.GetSection(SectionKind::DagNodes);
            out.m_clusterBytes = reader.GetSection(SectionKind::ClusterDescriptors);
            return true;
        }
    }

    TEST(MeshletPackBuilderDag, NonDagPackStaysVersion2WithoutDagSection)
    {
        const BuildResult r = BuildPackBytes(MakeDagGridFixture(/*generateDag*/ false));
        ASSERT_TRUE(r.m_success) << r.m_errorMessage.c_str();

        MeshletPackReader reader;
        ParsedDagPack pack;
        ASSERT_TRUE(ParsePack(r.m_packBytes, reader, pack));
        EXPECT_EQ(pack.m_version, PackVersion);
        EXPECT_FALSE(reader.HasSection(SectionKind::DagNodes));
        EXPECT_EQ(pack.m_lod.m_dagClusterCount, 0u);
    }

    TEST(MeshletPackBuilderDag, DagPackInvariants)
    {
        const BuildResult r = BuildPackBytes(MakeDagGridFixture(/*generateDag*/ true));
        ASSERT_TRUE(r.m_success) << r.m_errorMessage.c_str();

        MeshletPackReader reader;
        ParsedDagPack pack;
        ASSERT_TRUE(ParsePack(r.m_packBytes, reader, pack));
        EXPECT_EQ(pack.m_version, PackVersionDag);
        ASSERT_TRUE(reader.HasSection(SectionKind::DagNodes));

        // DAG packs write a single LOD entry: leaves first, interiors appended.
        const AZ::u32 leafCount = pack.m_lod.m_clusterCount;
        const AZ::u32 dagCount = pack.m_lod.m_dagClusterCount;
        ASSERT_GT(leafCount, 1u);
        // A trivially-simplifiable dense grid MUST have produced interior levels —
        // if this fires, the group/simplify loop silently built nothing.
        ASSERT_GT(dagCount, leafCount);

        // One DagNodeRecord per cluster, parallel to ClusterDescriptors.
        const AZ::u32 totalClusters =
            static_cast<AZ::u32>(pack.m_clusterBytes.size() / sizeof(ClusterDescriptor));
        EXPECT_EQ(totalClusters, dagCount);
        ASSERT_EQ(pack.m_dagBytes.size(), static_cast<size_t>(dagCount) * sizeof(DagNodeRecord));
        const auto* nodes = reinterpret_cast<const DagNodeRecord*>(pack.m_dagBytes.data());

        const float kInf = std::numeric_limits<float>::max();
        AZ::u32 finiteParents = 0;
        for (AZ::u32 i = 0; i < dagCount; ++i)
        {
            const DagNodeRecord& n = nodes[i];

            // Leaves are always eligible; interiors always carry a positive error.
            if (i < leafCount)
            {
                EXPECT_EQ(n.m_selfError, 0.0f) << "leaf " << i;
            }
            else
            {
                EXPECT_GT(n.m_selfError, 0.0f) << "interior " << i;
            }

            // Monotonic up the DAG: the cut test's uniqueness depends on this.
            EXPECT_GE(n.m_parentError, n.m_selfError) << "cluster " << i;
            EXPECT_GE(n.m_selfSphere[3], 0.0f);

            if (n.m_parentError != kInf)
            {
                ++finiteParents;
                // The parent (group) sphere must enclose this cluster's self sphere,
                // or projected parent error could shrink below projected self error
                // for some camera and break cut uniqueness.
                const AZ::Vector3 selfC(n.m_selfSphere[0], n.m_selfSphere[1], n.m_selfSphere[2]);
                const AZ::Vector3 parentC(n.m_parentSphere[0], n.m_parentSphere[1], n.m_parentSphere[2]);
                const float centerDist = selfC.GetDistance(parentC);
                EXPECT_LE(centerDist + n.m_selfSphere[3], n.m_parentSphere[3] + 1e-3f)
                    << "cluster " << i << " self sphere not enclosed by parent sphere";
            }
        }
        // At least every leaf that got grouped has a finite parent; on a dense grid
        // that must be the overwhelming majority of leaves.
        EXPECT_GT(finiteParents, leafCount / 2);
    }
} // namespace UnitTest
