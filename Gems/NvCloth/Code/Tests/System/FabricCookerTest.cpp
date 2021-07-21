/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <UnitTestHelper.h>
#include <TriangleInputHelper.h>

#include <NvCloth/IFabricCooker.h>
#include <System/SystemComponent.h>

namespace NvCloth
{
    namespace Internal
    {
        FabricId ComputeFabricId(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            const AZ::Vector3& fabricGravity,
            bool useGeodesicTether);
        void CopyCookedData(FabricCookedData::InternalCookedData& azCookedData, const nv::cloth::CookedData& nvCookedData);
        AZStd::optional<FabricCookedData> Cook(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            const AZ::Vector3& fabricGravity,
            bool useGeodesicTether);
        void WeldVertices(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            AZStd::vector<SimParticleFormat>& weldedParticles,
            AZStd::vector<SimIndexType>& weldedIndices,
            AZStd::vector<int>& remappedVertices,
            float weldingDistance = AZ::Constants::FloatEpsilon);
        void RemoveStaticTriangles(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            AZStd::vector<SimParticleFormat>& simplifiedParticles,
            AZStd::vector<SimIndexType>& simplifiedIndices,
            AZStd::vector<int>& remappedVertices);
    } // namespace Internal
} // namespace NvCloth

namespace UnitTest
{
    TEST(NvClothSystem, FactoryCooker_ComputeFabricIdWithNoData_IsValid)
    {
        NvCloth::FabricId fabricId = NvCloth::Internal::ComputeFabricId({}, {}, {}, false);

        EXPECT_TRUE(fabricId.IsValid());
    }

    TEST(NvClothSystem, FactoryCooker_ComputeFabricIdWithData_IsValid)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> particles = {{
            NvCloth::SimParticleFormat(1.0f,0.0f,0.0f,1.0f),
            NvCloth::SimParticleFormat(0.0f,1.0f,0.0f,1.0f),
            NvCloth::SimParticleFormat(0.0f,0.0f,1.0f,1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{
            0, 1, 2
        }};
        const AZ::Vector3 gravity(0.0f, 0.0f, -9.8f);
        const bool useGeodesicTether = true;

        NvCloth::FabricId fabricId = NvCloth::Internal::ComputeFabricId(particles, indices, gravity, useGeodesicTether);

        EXPECT_TRUE(fabricId.IsValid());
    }

    TEST(NvClothSystem, FactoryCooker_ComputeFabricIdsWithDifferentGravityParameter_ResultInDifferentIDs)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> particles = {{
            NvCloth::SimParticleFormat(1.0f,0.0f,0.0f,1.0f),
            NvCloth::SimParticleFormat(0.0f,1.0f,0.0f,1.0f),
            NvCloth::SimParticleFormat(0.0f,0.0f,1.0f,1.0f),
        } };
        const AZStd::vector<NvCloth::SimIndexType> indices = {{
            0, 1, 2
        }};
        const AZ::Vector3 gravity(0.0f, 0.0f, -9.8f);
        const bool useGeodesicTether = true;

        NvCloth::FabricId fabricId1 = NvCloth::Internal::ComputeFabricId(particles, indices, gravity, useGeodesicTether);
        NvCloth::FabricId fabricId2 = NvCloth::Internal::ComputeFabricId(particles, indices, 0.5f * gravity, useGeodesicTether);

        EXPECT_TRUE(fabricId1.IsValid());
        EXPECT_TRUE(fabricId2.IsValid());
        EXPECT_NE(fabricId1, fabricId2);
    }

    TEST(NvClothSystem, FactoryCooker_ComputeFabricIdsWithDifferentUseGeodesicTetherParameter_ResultInDifferentIDs)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> particles = {{
            NvCloth::SimParticleFormat(1.0f,0.0f,0.0f,1.0f),
            NvCloth::SimParticleFormat(0.0f,1.0f,0.0f,1.0f),
            NvCloth::SimParticleFormat(0.0f,0.0f,1.0f,1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{
            0, 1, 2
        }};
        const AZ::Vector3 gravity(0.0f, 0.0f, -9.8f);
        const bool useGeodesicTether = true;

        NvCloth::FabricId fabricId1 = NvCloth::Internal::ComputeFabricId(particles, indices, gravity, useGeodesicTether);
        NvCloth::FabricId fabricId2 = NvCloth::Internal::ComputeFabricId(particles, indices, gravity, !useGeodesicTether);

        EXPECT_TRUE(fabricId1.IsValid());
        EXPECT_TRUE(fabricId2.IsValid());
        EXPECT_NE(fabricId1, fabricId2);
    }

    TEST(NvClothSystem, FactoryCooker_CopyInternalCookedDataEmpty_CopiedDataIsEmpty)
    {
        nv::cloth::CookedData nvCookedData;
        nvCookedData.mNumParticles = 0;

        NvCloth::FabricCookedData::InternalCookedData azCookedData;
        NvCloth::Internal::CopyCookedData(azCookedData, nvCookedData);

        EXPECT_EQ(azCookedData.m_numParticles, 0);
        EXPECT_TRUE(azCookedData.m_phaseIndices.empty());
        EXPECT_TRUE(azCookedData.m_phaseTypes.empty());
        EXPECT_TRUE(azCookedData.m_sets.empty());
        EXPECT_TRUE(azCookedData.m_restValues.empty());
        EXPECT_TRUE(azCookedData.m_stiffnessValues.empty());
        EXPECT_TRUE(azCookedData.m_indices.empty());
        EXPECT_TRUE(azCookedData.m_anchors.empty());
        EXPECT_TRUE(azCookedData.m_tetherLengths.empty());
        EXPECT_TRUE(azCookedData.m_triangles.empty());
        ExpectEq(azCookedData, nvCookedData);
    }

    TEST(NvClothSystem, FactoryCooker_CopyInternalCookedData_CopiedDataMatchesSource)
    {
        const AZ::u32 data[] = { 0, 2, 45, 64, 125 };
        const size_t numDataElements = sizeof(data) / sizeof(data[0]);

        nv::cloth::CookedData nvCookedData;
        nvCookedData.mNumParticles = 0;

        NvCloth::FabricCookedData::InternalCookedData azCookedData;
        NvCloth::Internal::CopyCookedData(azCookedData, nvCookedData);

        ExpectEq(azCookedData, nvCookedData);
    }

    TEST(NvClothSystem, FactoryCooker_CookEmptyMesh_ReturnsNoData)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(NvCloth::SystemComponent::CheckLastClothError());

        AZStd::optional<NvCloth::FabricCookedData> fabricCookedData = NvCloth::Internal::Cook({}, {}, {}, false);

        NvCloth::SystemComponent::ResetLastClothError(); // Reset the nvcloth error left in SystemComponent
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error

        EXPECT_FALSE(fabricCookedData.has_value());
    }

    TEST(NvClothSystem, FactoryCooker_CookWithIncorrectIndices_ReturnsNoData)
    {
        const AZStd::vector<NvCloth::SimIndexType> incorrectIndices = {{ 0, 1 }}; // Use incorrect number of indices for a triangle (multiple of 3)

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(NvCloth::SystemComponent::CheckLastClothError());

        AZStd::optional<NvCloth::FabricCookedData> fabricCookedData = NvCloth::Internal::Cook({}, incorrectIndices, {}, false);

        NvCloth::SystemComponent::ResetLastClothError(); // Reset the nvcloth error left in SystemComponent
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error

        EXPECT_FALSE(fabricCookedData.has_value());
    }

    TEST(NvClothSystem, FactoryCooker_CookTriangle_CooksDataCorrectly)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f, 0.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(0.0f, 1.0f, 0.0f, 1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = { {0, 1, 2} };
        const AZ::Vector3 gravity(0.0f, 0.0f, -9.8f);
        const bool useGeodesicTether = true;

        AZStd::optional<NvCloth::FabricCookedData> fabricCookedData =
            NvCloth::Internal::Cook(vertices, indices, gravity, useGeodesicTether);

        EXPECT_TRUE(fabricCookedData.has_value());
        EXPECT_TRUE(fabricCookedData->m_id.IsValid());
        EXPECT_THAT(fabricCookedData->m_particles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), vertices));
        EXPECT_EQ(fabricCookedData->m_indices, indices);
        EXPECT_THAT(fabricCookedData->m_gravity, IsCloseTolerance(gravity, Tolerance));
        EXPECT_EQ(fabricCookedData->m_useGeodesicTether, useGeodesicTether);
        EXPECT_EQ(fabricCookedData->m_internalData.m_numParticles, vertices.size());
    }

    TEST(NvClothSystem, FactoryCooker_CookTriangleAllStatic_CooksDataCorrectly)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 0.0f, 0.0f, 0.0f),
            NvCloth::SimParticleFormat(1.0f, 0.0f, 0.0f, 0.0f),
            NvCloth::SimParticleFormat(0.0f, 1.0f, 0.0f, 0.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 0, 1, 2 }};
        const AZ::Vector3 gravity(0.0f, 0.0f, -9.8f);
        const bool useGeodesicTether = true;

        AZStd::optional<NvCloth::FabricCookedData> fabricCookedData =
            NvCloth::Internal::Cook(vertices, indices, gravity, useGeodesicTether);

        EXPECT_TRUE(fabricCookedData.has_value());
        EXPECT_TRUE(fabricCookedData->m_id.IsValid());
        EXPECT_THAT(fabricCookedData->m_particles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), vertices));
        EXPECT_EQ(fabricCookedData->m_indices, indices);
        EXPECT_THAT(fabricCookedData->m_gravity, IsCloseTolerance(gravity, Tolerance));
        EXPECT_EQ(fabricCookedData->m_useGeodesicTether, useGeodesicTether);
        EXPECT_EQ(fabricCookedData->m_internalData.m_numParticles, vertices.size());
    }

    TEST(NvClothSystem, FactoryCooker_CookMesh_CooksDataCorrectly)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 10;
        const AZ::u32 segmentsY = 10;
        const AZ::Vector3 gravity(0.0f, 0.0f, -9.8f);
        const bool useGeodesicTether = true;

        const TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);

        AZStd::optional<NvCloth::FabricCookedData> fabricCookedData =
            NvCloth::Internal::Cook(planeXY.m_vertices, planeXY.m_indices, gravity, useGeodesicTether);

        EXPECT_TRUE(fabricCookedData.has_value());
        EXPECT_TRUE(fabricCookedData->m_id.IsValid());
        EXPECT_THAT(fabricCookedData->m_particles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), planeXY.m_vertices));
        EXPECT_EQ(fabricCookedData->m_indices, planeXY.m_indices);
        EXPECT_THAT(fabricCookedData->m_gravity, IsCloseTolerance(gravity, Tolerance));
        EXPECT_EQ(fabricCookedData->m_useGeodesicTether, useGeodesicTether);
        EXPECT_EQ(fabricCookedData->m_internalData.m_numParticles, planeXY.m_vertices.size());
    }

    TEST(NvClothSystem, FactoryCooker_WeldVerticesEmptyMesh_ReturnsEmptyData)
    {
        AZStd::vector<NvCloth::SimParticleFormat> weldedVertices;
        AZStd::vector<NvCloth::SimIndexType> weldedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::WeldVertices({}, {}, weldedVertices, weldedIndices, remappedVertices);

        EXPECT_TRUE(weldedVertices.empty());
        EXPECT_TRUE(weldedIndices.empty());
        EXPECT_TRUE(remappedVertices.empty());
    }

    TEST(NvClothSystem, FactoryCooker_WeldVerticesTriangle_KeepsLowestInverseMass)
    {
        const AZ::Vector3 vertexPosition(100.2f, 300.2f, -30.62f);
        const size_t expectedSizeAfterWelding = 1;
        const float lowestInverseMass = 0.2f;

        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(vertexPosition, 1.0f),
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(vertexPosition, lowestInverseMass), // This vertex has the lowest inverse mass
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(vertexPosition, 0.5f)
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 0, 1, 2 }};

        AZStd::vector<NvCloth::SimParticleFormat> weldedVertices;
        AZStd::vector<NvCloth::SimIndexType> weldedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::WeldVertices(vertices, indices, weldedVertices, weldedIndices, remappedVertices);

        ASSERT_EQ(weldedVertices.size(), expectedSizeAfterWelding);
        EXPECT_THAT(weldedVertices[0].GetAsVector3(), IsCloseTolerance(vertexPosition, Tolerance));
        EXPECT_NEAR(weldedVertices[0].GetW(), lowestInverseMass, Tolerance);
    }

    TEST(NvClothSystem, FactoryCooker_WeldVerticesSquareWithDuplicatedVertices_DuplicatedVerticesAreRemoved)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 1.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat( 1.0f, 1.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 1.0f),

            NvCloth::SimParticleFormat( 1.0f, 1.0f, 0.0f, 1.0f), // Duplicated vertex
            NvCloth::SimParticleFormat( 1.0f,-1.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 1.0f)  // Duplicated vertex
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 0, 1, 2, 3, 4, 5 }};
        const size_t expectedSizeAfterWelding = vertices.size() - 2;

        AZStd::vector<NvCloth::SimParticleFormat> weldedVertices;
        AZStd::vector<NvCloth::SimIndexType> weldedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::WeldVertices(vertices, indices, weldedVertices, weldedIndices, remappedVertices);

        ASSERT_EQ(weldedVertices.size(), expectedSizeAfterWelding);
        ASSERT_EQ(weldedIndices.size(), indices.size());
        ASSERT_EQ(remappedVertices.size(), vertices.size());

        for (size_t i = 0; i < remappedVertices.size(); ++i)
        {
            EXPECT_GE(remappedVertices[i], 0);
            EXPECT_LT(remappedVertices[i], weldedVertices.size());
            EXPECT_THAT(weldedVertices[remappedVertices[i]], IsCloseTolerance(vertices[i], Tolerance));
        }

        for (size_t i = 0; i < weldedIndices.size(); ++i)
        {
            EXPECT_GE(weldedIndices[i], 0);
            EXPECT_LT(weldedIndices[i], weldedVertices.size());
            EXPECT_EQ(weldedIndices[i], remappedVertices[indices[i]]);
            EXPECT_THAT(weldedVertices[weldedIndices[i]], IsCloseTolerance(vertices[indices[i]], Tolerance));
        }
    }

    TEST(NvClothSystem, FactoryCooker_WeldVerticesTrianglesWithoutDuplicatedVertices_ResultIsTheSame)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 1.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f, 1.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 1.0f),

            NvCloth::SimParticleFormat(1.0f, 1.0f, 1.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f,-1.0f, 1.0f, 1.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 1.0f, 1.0f)
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 0, 1, 2, 3, 4, 5 }};

        AZStd::vector<NvCloth::SimParticleFormat> weldedVertices;
        AZStd::vector<NvCloth::SimIndexType> weldedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::WeldVertices(vertices, indices, weldedVertices, weldedIndices, remappedVertices);

        // The result after calling WeldVertices is expected to have the same size.
        // The vertices inside will be reordered though due to the welding process.
        ASSERT_EQ(weldedVertices.size(), vertices.size());
        ASSERT_EQ(weldedIndices.size(), indices.size());
        ASSERT_EQ(remappedVertices.size(), vertices.size());

        for (size_t i = 0; i < remappedVertices.size(); ++i)
        {
            EXPECT_GE(remappedVertices[i], 0);
            EXPECT_LT(remappedVertices[i], weldedVertices.size());
            EXPECT_THAT(weldedVertices[remappedVertices[i]], IsCloseTolerance(vertices[i], Tolerance));
        }

        for (size_t i = 0; i < weldedIndices.size(); ++i)
        {
            EXPECT_GE(weldedIndices[i], 0);
            EXPECT_LT(weldedIndices[i], weldedVertices.size());
            EXPECT_EQ(weldedIndices[i], remappedVertices[indices[i]]);
            EXPECT_THAT(weldedVertices[weldedIndices[i]], IsCloseTolerance(vertices[indices[i]], Tolerance));
        }
    }

    TEST(NvClothSystem, FactoryCooker_RemoveStaticTrianglesEmptyMesh_ReturnsEmptyData)
    {
        AZStd::vector<NvCloth::SimParticleFormat> simplifiedVertices;
        AZStd::vector<NvCloth::SimIndexType> simplifiedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::RemoveStaticTriangles({}, {}, simplifiedVertices, simplifiedIndices, remappedVertices);

        EXPECT_TRUE(simplifiedVertices.empty());
        EXPECT_TRUE(simplifiedIndices.empty());
        EXPECT_TRUE(remappedVertices.empty());
    }

    TEST(NvClothSystem, FactoryCooker_RemoveStaticTrianglesWithOneStaticTriangle_RemovesAllVerticesAndIndices)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 1.0f, 0.0f, 0.0f),
            NvCloth::SimParticleFormat(1.0f, 1.0f, 0.0f, 0.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 0.0f)
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 0, 1, 2 }};

        AZStd::vector<NvCloth::SimParticleFormat> simplifiedVertices;
        AZStd::vector<NvCloth::SimIndexType> simplifiedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::RemoveStaticTriangles(vertices, indices, simplifiedVertices, simplifiedIndices, remappedVertices);

        EXPECT_TRUE(simplifiedVertices.empty());
        EXPECT_TRUE(simplifiedIndices.empty());
        EXPECT_EQ(remappedVertices.size(), vertices.size());
        for (const auto& remappedVertex : remappedVertices)
        {
            EXPECT_LT(remappedVertex, 0); // Remapping must be negative, meaning vertex has been removed.
        }
    }

    TEST(NvClothSystem, FactoryCooker_RemoveStaticTrianglesWithStaticTriangles_StaticTriangleAndVerticesAreRemoved)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 1.0f, 0.0f, 0.0f), // This static vertex will be removed
            NvCloth::SimParticleFormat(1.0f, 1.0f, 0.0f, 0.0f),  // This static vertex will be removed
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 0.0f), // This static vertex will be removed

            NvCloth::SimParticleFormat(1.0f, 1.0f, 1.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f,-1.0f, 1.0f, 0.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 1.0f, 1.0f)
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 0, 1, 2, 3, 4, 5 }}; // First triangle from the triplet 0,1,2 uses all static vertices and will be removed
        const size_t expectedVerticesSizeAfterSimplification = vertices.size() - 3;
        const size_t expectedIndicesSizeAfterSimplification = indices.size() - 3; // 1 triangles less is 3 indices less.

        AZStd::vector<NvCloth::SimParticleFormat> simplifiedVertices;
        AZStd::vector<NvCloth::SimIndexType> simplifiedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::RemoveStaticTriangles(vertices, indices, simplifiedVertices, simplifiedIndices, remappedVertices);

        ASSERT_EQ(simplifiedVertices.size(), expectedVerticesSizeAfterSimplification);
        ASSERT_EQ(simplifiedIndices.size(), expectedIndicesSizeAfterSimplification);
        ASSERT_EQ(remappedVertices.size(), vertices.size());

        for (size_t i = 0; i < remappedVertices.size(); ++i)
        {
            // The first 3 vertices should have been removed, so the remapping should be negative
            if (i < 3)
            {
                EXPECT_LT(remappedVertices[i], 0);
            }
            else
            {
                EXPECT_GE(remappedVertices[i], 0);
                EXPECT_LT(remappedVertices[i], simplifiedVertices.size());
                EXPECT_THAT(simplifiedVertices[remappedVertices[i]], IsCloseTolerance(vertices[i], Tolerance));
            }
        }

        for (size_t i = 0; i < simplifiedIndices.size(); ++i)
        {
            EXPECT_GE(simplifiedIndices[i], 0);
            EXPECT_LT(simplifiedIndices[i], simplifiedVertices.size());
        }

        for (size_t i = 0; i < indices.size(); ++i)
        {
            int remappedVertex = remappedVertices[indices[i]];
            if (remappedVertex >= 0) // If the vertex has not been removed
            {
                EXPECT_THAT(simplifiedVertices[remappedVertex], IsCloseTolerance(vertices[indices[i]], Tolerance));
            }
        }
    }

    TEST(NvClothSystem, FactoryCooker_RemoveStaticTrianglesWithStaticTrianglesSharedVertices_StaticTriangleAndVerticesAreRemoved)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 1.0f, 0.0f, 0.0f), // This static vertex will be removed
            NvCloth::SimParticleFormat(1.0f, 1.0f, 0.0f, 0.0f),  // This static vertex will remain because it's also used in the third triangle too
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 0.0f), // This static vertex will be removed

            NvCloth::SimParticleFormat(1.0f, 1.0f, 1.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f,-1.0f, 1.0f, 0.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 1.0f, 1.0f)
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 3, 4, 5, 0, 1, 2, 3, 1, 5 }}; // Second triangle from the triplet 0,1,2 uses all static vertices and will be removed
        const size_t expectedVerticesSizeAfterSimplification = vertices.size() - 2;
        const size_t expectedIndicesSizeAfterSimplification = indices.size() - 3; // 1 triangles less is 3 indices less.

        AZStd::vector<NvCloth::SimParticleFormat> simplifiedVertices;
        AZStd::vector<NvCloth::SimIndexType> simplifiedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::RemoveStaticTriangles(vertices, indices, simplifiedVertices, simplifiedIndices, remappedVertices);

        ASSERT_EQ(simplifiedVertices.size(), expectedVerticesSizeAfterSimplification);
        ASSERT_EQ(simplifiedIndices.size(), expectedIndicesSizeAfterSimplification);
        ASSERT_EQ(remappedVertices.size(), vertices.size());

        for (size_t i = 0; i < remappedVertices.size(); ++i)
        {
            // The first and third vertex should have been removed, so the remapping should be negative.
            if (i == 0 || i == 2)
            {
                EXPECT_LT(remappedVertices[i], 0);
            }
            else
            {
                EXPECT_GE(remappedVertices[i], 0);
                EXPECT_LT(remappedVertices[i], simplifiedVertices.size());
                EXPECT_THAT(simplifiedVertices[remappedVertices[i]], IsCloseTolerance(vertices[i], Tolerance));
            }
        }

        for (size_t i = 0; i < simplifiedIndices.size(); ++i)
        {
            EXPECT_GE(simplifiedIndices[i], 0);
            EXPECT_LT(simplifiedIndices[i], simplifiedVertices.size());
        }

        for (size_t i = 0; i < indices.size(); ++i)
        {
            int remappedVertex = remappedVertices[indices[i]];
            if (remappedVertex >= 0) // If the vertex has not been removed
            {
                EXPECT_THAT(simplifiedVertices[remappedVertex], IsCloseTolerance(vertices[indices[i]], Tolerance));
            }
        }
    }

    TEST(NvClothSystem, FactoryCooker_RemoveStaticTrianglesWithNonStaticTriangles_ResultIsTheSame)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = {{
            NvCloth::SimParticleFormat(-1.0f, 1.0f, 0.0f, 0.0f),
            NvCloth::SimParticleFormat(1.0f, 1.0f, 0.0f, 1.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 1.0f),

            NvCloth::SimParticleFormat(1.0f, 1.0f, 1.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f,-1.0f, 1.0f, 0.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 1.0f, 1.0f)
        }};
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 0, 1, 2, 3, 4, 5 }};

        AZStd::vector<NvCloth::SimParticleFormat> simplifiedVertices;
        AZStd::vector<NvCloth::SimIndexType> simplifiedIndices;
        AZStd::vector<int> remappedVertices;
        NvCloth::Internal::RemoveStaticTriangles(vertices, indices, simplifiedVertices, simplifiedIndices, remappedVertices);

        // The result after calling RemoveStaticTriangles is expected to have the same size.
        // The vertices will be reordered though due to the processing during simplification.
        ASSERT_EQ(simplifiedVertices.size(), vertices.size());
        ASSERT_EQ(simplifiedIndices.size(), indices.size());
        ASSERT_EQ(remappedVertices.size(), vertices.size());

        for (size_t i = 0; i < remappedVertices.size(); ++i)
        {
            EXPECT_GE(remappedVertices[i], 0);
            EXPECT_LT(remappedVertices[i], simplifiedVertices.size());
            EXPECT_THAT(simplifiedVertices[remappedVertices[i]], IsCloseTolerance(vertices[i], Tolerance));
        }

        for (size_t i = 0; i < simplifiedIndices.size(); ++i)
        {
            EXPECT_GE(simplifiedIndices[i], 0);
            EXPECT_LT(simplifiedIndices[i], simplifiedVertices.size());
            EXPECT_EQ(simplifiedIndices[i], remappedVertices[indices[i]]);
            EXPECT_THAT(simplifiedVertices[simplifiedIndices[i]], IsCloseTolerance(vertices[indices[i]], Tolerance));
        }
    }

    TEST(NvClothSystem, FactoryCooker_SimplifiyMeshWithDuplicatedVerticesAndStaticTriangles_DuplicatedVerticesAndStaticTrianglesAreRemoved)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = { {
            NvCloth::SimParticleFormat(-1.0f, 1.0f, 0.0f, 0.0f), // This static vertex will be removed
            NvCloth::SimParticleFormat(1.0f, 1.0f, 0.0f, 0.0f),  // This static vertex will remain because it's also used in the third triangle too
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 0.0f), // This static vertex will be removed

            NvCloth::SimParticleFormat(1.0f, 1.0f, 1.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f,-1.0f, 1.0f, 0.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 1.0f, 1.0f),

            NvCloth::SimParticleFormat(1.0f, 1.0f, 1.0f, 1.0f), // Duplicated vertex
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 1.0f, 1.0f) // Duplicated vertex
        } };
        const AZStd::vector<NvCloth::SimIndexType> indices = {{ 3, 4, 5, 0, 1, 2, 6, 1, 7 }}; // Second triangle from the triplet 0,1,2 uses all static vertices and will be removed
        const size_t expectedVerticesSizeAfterSimplification = vertices.size() - 4;
        const size_t expectedIndicesSizeAfterSimplification = indices.size() - 3; // 1 triangles less is 3 indices less.
        const bool removeStaticTriangles = true;

        AZStd::vector<NvCloth::SimParticleFormat> simplifiedVertices;
        AZStd::vector<NvCloth::SimIndexType> simplifiedIndices;
        AZStd::vector<int> remappedVertices;
        AZ::Interface<NvCloth::IFabricCooker>::Get()->SimplifyMesh(vertices, indices, simplifiedVertices, simplifiedIndices, remappedVertices, removeStaticTriangles);

        ASSERT_EQ(simplifiedVertices.size(), expectedVerticesSizeAfterSimplification);
        ASSERT_EQ(simplifiedIndices.size(), expectedIndicesSizeAfterSimplification);
        ASSERT_EQ(remappedVertices.size(), vertices.size());

        for (size_t i = 0; i < remappedVertices.size(); ++i)
        {
            // The first and third vertex should have been removed, so the remapping should be negative.
            if (i == 0 || i == 2)
            {
                EXPECT_LT(remappedVertices[i], 0);
            }
            else
            {
                EXPECT_GE(remappedVertices[i], 0);
                EXPECT_LT(remappedVertices[i], simplifiedVertices.size());
                EXPECT_THAT(simplifiedVertices[remappedVertices[i]], IsCloseTolerance(vertices[i], Tolerance));
            }
        }

        for (size_t i = 0; i < simplifiedIndices.size(); ++i)
        {
            EXPECT_GE(simplifiedIndices[i], 0);
            EXPECT_LT(simplifiedIndices[i], simplifiedVertices.size());
        }

        for (size_t i = 0; i < indices.size(); ++i)
        {
            int remappedVertex = remappedVertices[indices[i]];
            if (remappedVertex >= 0) // If the vertex has not been removed
            {
                EXPECT_THAT(simplifiedVertices[remappedVertex], IsCloseTolerance(vertices[indices[i]], Tolerance));
            }
        }
    }

    TEST(NvClothSystem, FactoryCooker_SimplifiyMeshWithoutRemovingStaticTriangles_DuplicatedVerticesRemovedAndStaticTrianglesRemain)
    {
        const AZStd::vector<NvCloth::SimParticleFormat> vertices = { {
            NvCloth::SimParticleFormat(-1.0f, 1.0f, 0.0f, 0.0f), // This static vertex will remain because we won't remove static triangles
            NvCloth::SimParticleFormat(1.0f, 1.0f, 0.0f, 0.0f),  // This static vertex will remain because it's also used in the third triangle too
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 0.0f, 0.0f), // This static vertex will remain because we won't remove static triangles

            NvCloth::SimParticleFormat(1.0f, 1.0f, 1.0f, 1.0f),
            NvCloth::SimParticleFormat(1.0f,-1.0f, 1.0f, 0.0f),
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 1.0f, 1.0f),

            NvCloth::SimParticleFormat(1.0f, 1.0f, 1.0f, 1.0f), // Duplicated vertex
            NvCloth::SimParticleFormat(-1.0f,-1.0f, 1.0f, 1.0f) // Duplicated vertex
        } };
        const AZStd::vector<NvCloth::SimIndexType> indices = { { 3, 4, 5, 0, 1, 2, 6, 1, 7 } }; // Second triangle from the triplet 0,1,2 uses all static vertices and will be removed
        const size_t expectedVerticesSizeAfterSimplification = vertices.size() - 2;
        const size_t expectedIndicesSizeAfterSimplification = indices.size();
        const bool removeStaticTriangles = false;

        AZStd::vector<NvCloth::SimParticleFormat> simplifiedVertices;
        AZStd::vector<NvCloth::SimIndexType> simplifiedIndices;
        AZStd::vector<int> remappedVertices;
        AZ::Interface<NvCloth::IFabricCooker>::Get()->SimplifyMesh(vertices, indices, simplifiedVertices, simplifiedIndices, remappedVertices, removeStaticTriangles);

        ASSERT_EQ(simplifiedVertices.size(), expectedVerticesSizeAfterSimplification);
        ASSERT_EQ(simplifiedIndices.size(), expectedIndicesSizeAfterSimplification);
        ASSERT_EQ(remappedVertices.size(), vertices.size());

        for (size_t i = 0; i < remappedVertices.size(); ++i)
        {
            EXPECT_GE(remappedVertices[i], 0);
            EXPECT_LT(remappedVertices[i], simplifiedVertices.size());
            EXPECT_THAT(simplifiedVertices[remappedVertices[i]], IsCloseTolerance(vertices[i], Tolerance));
        }

        for (size_t i = 0; i < simplifiedIndices.size(); ++i)
        {
            EXPECT_GE(simplifiedIndices[i], 0);
            EXPECT_LT(simplifiedIndices[i], simplifiedVertices.size());
            EXPECT_EQ(simplifiedIndices[i], remappedVertices[indices[i]]);
            EXPECT_THAT(simplifiedVertices[simplifiedIndices[i]], IsCloseTolerance(vertices[indices[i]], Tolerance));
        }
    }
} // namespace UnitTest
