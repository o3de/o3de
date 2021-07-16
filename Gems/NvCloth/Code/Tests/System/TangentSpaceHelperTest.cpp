/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/MathUtils.h>

#include <UnitTestHelper.h>
#include <TriangleInputHelper.h>

#include <NvCloth/ITangentSpaceHelper.h>

namespace UnitTest
{
    TEST(NvClothSystem, TangentSpaceHelper_CalculateNormalsWithNoMesh_ReturnsEmptyNormals)
    {
        AZStd::vector<AZ::Vector3> normals;
        bool normalsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            {}, {},
            normals);

        EXPECT_TRUE(normalsCalculated);
        EXPECT_TRUE(normals.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentsAndBitangentsWithNoMesh_ReturnsEmptyTangentsAndBitangents)
    {
        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            {}, {}, {}, {},
            tangents, bitangents);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_TRUE(tangents.empty());
        EXPECT_TRUE(bitangents.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentSpaceWithNoMesh_ReturnsEmptyTangentSpace)
    {
        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            {}, {}, {},
            tangents, bitangents, normals);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_TRUE(tangents.empty());
        EXPECT_TRUE(bitangents.empty());
        EXPECT_TRUE(normals.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateNormalsWithIncorrectIndices_ReturnsFalse)
    {
        const AZStd::vector<NvCloth::SimIndexType> incorrectIndices = { { 0, 1 } }; // Use incorrect number of indices for a triangle (multiple of 3)

        AZ_TEST_START_TRACE_SUPPRESSION;

        AZStd::vector<AZ::Vector3> normals;
        bool normalsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            {}, incorrectIndices,
            normals);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error

        EXPECT_FALSE(normalsCalculated);
        EXPECT_TRUE(normals.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentsAndBitangentsWithIncorrectIndices_ReturnsFalse)
    {
        const AZStd::vector<NvCloth::SimIndexType> incorrectIndices = { { 0, 1 } }; // Use incorrect number of indices for a triangle (multiple of 3)

        AZ_TEST_START_TRACE_SUPPRESSION;

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            {}, incorrectIndices, {}, {},
            tangents, bitangents);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error

        EXPECT_FALSE(tangentsCalculated);
        EXPECT_TRUE(tangents.empty());
        EXPECT_TRUE(bitangents.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentSpaceWithIncorrectIndices_ReturnsFalse)
    {
        const AZStd::vector<NvCloth::SimIndexType> incorrectIndices = { { 0, 1 } }; // Use incorrect number of indices for a triangle (multiple of 3)

        AZ_TEST_START_TRACE_SUPPRESSION;

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            {}, incorrectIndices, {},
            tangents, bitangents, normals);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error

        EXPECT_FALSE(tangentsCalculated);
        EXPECT_TRUE(tangents.empty());
        EXPECT_TRUE(bitangents.empty());
        EXPECT_TRUE(normals.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentsAndBitangentsWithIncorrectUVs_ReturnsFalse)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 1.0f, 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};
        const AZStd::vector<NvCloth::SimUVType> uvs = {{ // Wrong number of UVs, 2 instead of 3 like the number of vertices
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(1.0f, 0.0f)
        }};

        AZ_TEST_START_TRACE_SUPPRESSION;

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            vertices, indices, uvs, {},
            tangents, bitangents);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error

        EXPECT_FALSE(tangentsCalculated);
        EXPECT_TRUE(tangents.empty());
        EXPECT_TRUE(bitangents.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentsAndBitangentsWithIncorrectNormals_ReturnsFalse)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 1.0f, 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};
        const AZStd::vector<NvCloth::SimUVType> uvs = {{
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(1.0f, 0.0f),
            NvCloth::SimUVType(0.5f, 1.0f)
        }};

        // Wrong number of normals, 2 instead of 3 like the number of vertices
        const AZStd::vector<AZ::Vector3> normals = {{
            AZ::Vector3(0.0f, 0.0f, 1.0f),
            AZ::Vector3(0.0f, 0.0f, 1.0f)
        }};

        AZ_TEST_START_TRACE_SUPPRESSION;

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            vertices, indices, uvs, {},
            tangents, bitangents);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error

        EXPECT_FALSE(tangentsCalculated);
        EXPECT_TRUE(tangents.empty());
        EXPECT_TRUE(bitangents.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentSpaceWithIncorrectUVs_ReturnsFalse)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 1.0f, 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};
        const AZStd::vector<NvCloth::SimUVType> uvs = {{ // Wrong number of UVs, 2 instead of 3 like the number of vertices
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(1.0f, 0.0f)
        }};

        AZ_TEST_START_TRACE_SUPPRESSION;

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            vertices, indices, uvs,
            tangents, bitangents, normals);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error

        EXPECT_FALSE(tangentsCalculated);
        EXPECT_TRUE(tangents.empty());
        EXPECT_TRUE(bitangents.empty());
        EXPECT_TRUE(normals.empty());
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateNormalsWithNoAreaTriangle_ReturnsFiniteNormals)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};

        AZStd::vector<AZ::Vector3> normals;
        bool normalsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            vertices, indices,
            normals);

        EXPECT_TRUE(normalsCalculated);
        EXPECT_EQ(normals.size(), vertices.size());
        EXPECT_THAT(normals, ::testing::Each(IsFinite()));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentsAndBitangentsWithNoAreaTriangle_ReturnsFiniteTangentsAndBitangents)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};
        const AZStd::vector<NvCloth::SimUVType> uvs = {{
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(0.0f, 0.0f)
        }};

        const AZStd::vector<AZ::Vector3> normals = {{
            AZ::Vector3(0.0f, 0.0f, 1.0f),
            AZ::Vector3(0.0f, 0.0f, 1.0f),
            AZ::Vector3(0.0f, 0.0f, 1.0f)
        }};

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            vertices, indices, uvs, normals,
            tangents, bitangents);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_EQ(tangents.size(), vertices.size());
        EXPECT_EQ(bitangents.size(), vertices.size());
        EXPECT_THAT(tangents, ::testing::Each(IsFinite()));
        EXPECT_THAT(bitangents, ::testing::Each(IsFinite()));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentSpaceWithNoAreaTriangle_ReturnsFiniteTangentSpace)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};
        const AZStd::vector<NvCloth::SimUVType> uvs = {{
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(0.0f, 0.0f)
        }};

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            vertices, indices, uvs,
            tangents, bitangents, normals);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_EQ(tangents.size(), vertices.size());
        EXPECT_EQ(bitangents.size(), vertices.size());
        EXPECT_EQ(normals.size(), vertices.size());
        EXPECT_THAT(tangents, ::testing::Each(IsFinite()));
        EXPECT_THAT(bitangents, ::testing::Each(IsFinite()));
        EXPECT_THAT(normals, ::testing::Each(IsFinite()));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateNormalsWithNANVertices_ReturnsFiniteNormals)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};

        AZStd::vector<AZ::Vector3> normals;
        bool normalsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            vertices, indices,
            normals);

        EXPECT_TRUE(normalsCalculated);
        EXPECT_EQ(normals.size(), vertices.size());
        EXPECT_THAT(normals, ::testing::Each(IsFinite()));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentsAndBitangentsWithNANVertices_ReturnsFiniteTangentsAndBitangents)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};
        const AZStd::vector<NvCloth::SimUVType> uvs = {{
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(0.0f, 0.0f)
        }};

        const AZStd::vector<AZ::Vector3> normals = {{
            AZ::Vector3(0.0f, 0.0f, 1.0f),
            AZ::Vector3(0.0f, 0.0f, 1.0f),
            AZ::Vector3(0.0f, 0.0f, 1.0f)
        }};

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            vertices, indices, uvs, normals,
            tangents, bitangents);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_EQ(tangents.size(), vertices.size());
        EXPECT_EQ(bitangents.size(), vertices.size());
        EXPECT_THAT(tangents, ::testing::Each(IsFinite()));
        EXPECT_THAT(bitangents, ::testing::Each(IsFinite()));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentSpaceWithNANVertices_ReturnsFiniteTangentSpace)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
            NvCloth::SimParticleFormat(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = { {0, 1, 2} };
        const AZStd::vector<NvCloth::SimUVType> uvs = {{
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(0.0f, 0.0f)
        }};

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            vertices, indices, uvs,
            tangents, bitangents, normals);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_EQ(tangents.size(), vertices.size());
        EXPECT_EQ(bitangents.size(), vertices.size());
        EXPECT_EQ(normals.size(), vertices.size());
        EXPECT_THAT(tangents, ::testing::Each(IsFinite()));
        EXPECT_THAT(bitangents, ::testing::Each(IsFinite()));
        EXPECT_THAT(normals, ::testing::Each(IsFinite()));
    }
    
    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentSpaceWithTriangle_ReturnsCorrectTangentSpace)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 1.0f, 0.0f, 1.0f)
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{0, 1, 2}};
        const AZStd::vector<NvCloth::SimUVType> uvs = {{
            NvCloth::SimUVType(0.0f, 0.0f),
            NvCloth::SimUVType(1.0f, 0.0f),
            NvCloth::SimUVType(0.5f, 1.0f)
        }};

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            vertices, indices, uvs,
            tangents, bitangents, normals);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_THAT(tangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisX(), Tolerance)));
        EXPECT_THAT(bitangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisY(), Tolerance)));
        EXPECT_THAT(normals, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisZ(), Tolerance)));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateNormalsPlaneXY_ReturnsAxisZNomals)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);
        const size_t numVertices = planeXY.m_vertices.size();

        AZStd::vector<AZ::Vector3> normals;
        bool normalsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            planeXY.m_vertices, planeXY.m_indices, normals);

        EXPECT_TRUE(normalsCalculated);
        EXPECT_EQ(normals.size(), numVertices);
        EXPECT_THAT(normals, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisZ(), Tolerance)));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentsAndBitangentsPlaneXY_ReturnsAxisXTangentsAndAxisYBitangents)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);
        const size_t numVertices = planeXY.m_vertices.size();

        AZStd::vector<AZ::Vector3> normals;
        AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            planeXY.m_vertices, planeXY.m_indices, normals);

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            planeXY.m_vertices, planeXY.m_indices, planeXY.m_uvs, normals,
            tangents, bitangents);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_EQ(tangents.size(), numVertices);
        EXPECT_EQ(bitangents.size(), numVertices);
        EXPECT_THAT(tangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisX(), Tolerance)));
        EXPECT_THAT(bitangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisY(), Tolerance)));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentSpacePlaneXY_ReturnsCorrectTangentSpace)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);
        const size_t numVertices = planeXY.m_vertices.size();

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            planeXY.m_vertices, planeXY.m_indices, planeXY.m_uvs,
            tangents, bitangents, normals);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_EQ(tangents.size(), numVertices);
        EXPECT_EQ(bitangents.size(), numVertices);
        EXPECT_EQ(normals.size(), numVertices);
        EXPECT_THAT(tangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisX(), Tolerance)));
        EXPECT_THAT(bitangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisY(), Tolerance)));
        EXPECT_THAT(normals, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisZ(), Tolerance)));
    }


    TEST(NvClothSystem, TangentSpaceHelper_CalculateNormalsPlaneXYRot90Y_ReturnsCorrectNormals)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);
        const size_t numVertices = planeXY.m_vertices.size();

        const AZ::Transform rotation90Y = AZ::Transform::CreateRotationY(AZ::Constants::HalfPi);
        for (auto& vertex : planeXY.m_vertices)
        {
            vertex.Set(
                rotation90Y.TransformPoint(vertex.GetAsVector3()),
                vertex.GetW()
            );
        }

        AZStd::vector<AZ::Vector3> normals;
        bool normalsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            planeXY.m_vertices, planeXY.m_indices, normals);

        EXPECT_TRUE(normalsCalculated);
        EXPECT_EQ(normals.size(), numVertices);
        EXPECT_THAT(normals, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisX(), Tolerance)));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentsAndBitangentsPlaneXYRot90Y_ReturnsCorrectTangentsAndBitangents)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);
        const size_t numVertices = planeXY.m_vertices.size();

        const AZ::Transform rotation90Y = AZ::Transform::CreateRotationY(AZ::Constants::HalfPi);
        for (auto& vertex : planeXY.m_vertices)
        {
            vertex.Set(
                rotation90Y.TransformPoint(vertex.GetAsVector3()),
                vertex.GetW()
            );
        }

        AZStd::vector<AZ::Vector3> normals;
        AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            planeXY.m_vertices, planeXY.m_indices, normals);

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            planeXY.m_vertices, planeXY.m_indices, planeXY.m_uvs, normals,
            tangents, bitangents);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_EQ(tangents.size(), numVertices);
        EXPECT_EQ(bitangents.size(), numVertices);
        EXPECT_THAT(tangents, ::testing::Each(IsCloseTolerance(-AZ::Vector3::CreateAxisZ(), Tolerance)));
        EXPECT_THAT(bitangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisY(), Tolerance)));
    }

    TEST(NvClothSystem, TangentSpaceHelper_CalculateTangentSpacePlaneXYRot90Y_ReturnsCorrectTangentSpace)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);
        const size_t numVertices = planeXY.m_vertices.size();

        const AZ::Transform rotation90Y = AZ::Transform::CreateRotationY(AZ::Constants::HalfPi);
        for (auto& vertex : planeXY.m_vertices)
        {
            vertex.Set(
                rotation90Y.TransformPoint(vertex.GetAsVector3()),
                vertex.GetW()
            );
        }

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        bool tangentsCalculated = AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            planeXY.m_vertices, planeXY.m_indices, planeXY.m_uvs,
            tangents, bitangents, normals);

        EXPECT_TRUE(tangentsCalculated);
        EXPECT_EQ(tangents.size(), numVertices);
        EXPECT_EQ(bitangents.size(), numVertices);
        EXPECT_EQ(normals.size(), numVertices);
        EXPECT_THAT(tangents, ::testing::Each(IsCloseTolerance(-AZ::Vector3::CreateAxisZ(), Tolerance)));
        EXPECT_THAT(bitangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisY(), Tolerance)));
        EXPECT_THAT(normals, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisX(), Tolerance)));
    }
} // namespace UnitTest
