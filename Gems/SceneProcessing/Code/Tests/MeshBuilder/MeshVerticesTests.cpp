/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Generation/Components/MeshOptimizer/MeshBuilder.h>

namespace AZ::MeshBuilder
{
    class CubeMeshVerticesTests
        : public UnitTest::LeakDetectionFixture
    {
    public:
        void SetUpMeshBuilder(size_t vertCount)
        {
            m_meshBuilder = AZStd::make_unique<AZ::MeshBuilder::MeshBuilder>(vertCount);

            // Original vertex numbers
            m_orgVtxLayer = m_meshBuilder->AddLayer<AZ::MeshBuilder::MeshBuilderVertexAttributeLayerUInt32>(
                vertCount,
                false,
                false
            );

            // The positions layer
            m_posLayer = m_meshBuilder->AddLayer<AZ::MeshBuilder::MeshBuilderVertexAttributeLayerVector3>(
                vertCount,
                false,
                true
            );

            // The normals layer
            m_normalsLayer = m_meshBuilder->AddLayer<AZ::MeshBuilder::MeshBuilderVertexAttributeLayerVector3>(
                vertCount,
                false,
                true
            );
        }

        void BuildCube(const bool useSharedNormals)
        {
            constexpr AZStd::array m_cubeVertexIndices = {
                size_t(0),1,2,
                0,2,3,
                1,5,6,
                1,6,2,
                5,4,7,
                5,7,6,
                4,0,3,
                4,3,7,
                1,0,4,
                1,4,5,
                3,2,6,
                3,6,7
            };

            const AZStd::array m_cubeOriginalVertices = {
                AZ::Vector3(-0.5f, -0.5f, -0.5f),
                AZ::Vector3(0.5f, -0.5f, -0.5f),
                AZ::Vector3(0.5f, 0.5f, -0.5f),
                AZ::Vector3(-0.5f, 0.5f, -0.5f),
                AZ::Vector3(-0.5f, -0.5f, 0.5f),
                AZ::Vector3(0.5f, -0.5f, 0.5f),
                AZ::Vector3(0.5f, 0.5f, 0.5f),
                AZ::Vector3(-0.5f, 0.5f, 0.5f)
            };

            // Create the mesh builder and fill in the layers with cube vertices.
            const size_t orgVertCount = m_cubeOriginalVertices.size();
            const size_t triangleCount = m_cubeVertexIndices.size() / 3;
            SetUpMeshBuilder(orgVertCount);

            const size_t materialId = 0;
            for (size_t faceNum = 0; faceNum < triangleCount; ++faceNum)
            {
                const size_t indexA = faceNum * 3;
                const size_t indexB = (faceNum * 3) + 1;
                const size_t indexC = (faceNum * 3) + 2;
                const AZ::Vector3& p1 = m_cubeOriginalVertices[m_cubeVertexIndices[indexA]];
                const AZ::Vector3& p2 = m_cubeOriginalVertices[m_cubeVertexIndices[indexB]];
                const AZ::Vector3& p3 = m_cubeOriginalVertices[m_cubeVertexIndices[indexC]];
                const AZ::Vector3 sharedNormal = (p2 - p1).Cross(p3 - p1).GetNormalized();

                m_meshBuilder->BeginPolygon(materialId);
                for (size_t vertexOfFace = 0; vertexOfFace < 3; ++vertexOfFace)
                {
                    size_t vertexNum = faceNum * 3 + vertexOfFace;
                    const AZ::Vector3& smoothShadedNormal = m_cubeOriginalVertices[m_cubeVertexIndices[vertexNum]].GetNormalized();

                    m_orgVtxLayer->SetCurrentVertexValue(static_cast<AZ::u32>(m_cubeVertexIndices[vertexNum]));
                    m_posLayer->SetCurrentVertexValue(m_cubeOriginalVertices[m_cubeVertexIndices[vertexNum]]);
                    m_normalsLayer->SetCurrentVertexValue(useSharedNormals ? sharedNormal : smoothShadedNormal);
                    m_meshBuilder->AddPolygonVertex(m_cubeVertexIndices[vertexNum]);
                }
                m_meshBuilder->EndPolygon();
            }
        }

    public:
        AZStd::unique_ptr<AZ::MeshBuilder::MeshBuilder> m_meshBuilder;
        AZ::MeshBuilder::MeshBuilderVertexAttributeLayerUInt32* m_orgVtxLayer = nullptr;
        AZ::MeshBuilder::MeshBuilderVertexAttributeLayerVector3* m_posLayer = nullptr;
        AZ::MeshBuilder::MeshBuilderVertexAttributeLayerVector3* m_normalsLayer = nullptr;
    };

    TEST_F(CubeMeshVerticesTests, SmoothShadedCubeMeshVertexDedup)
    {
        BuildCube(/* useSharedNormals= */false);
        const float vertexDupeRatio = m_meshBuilder->CalcNumVertices() / aznumeric_cast<float>(m_meshBuilder->GetNumOrgVerts());
        EXPECT_EQ(vertexDupeRatio, 1.0f) << "No duplicated vertex should be created.";
    }

    TEST_F(CubeMeshVerticesTests, FlatShadedCubeMeshVertexDedup)
    {
        BuildCube(/* useSharedNormals= */true);
        const float vertexDupeRatio = m_meshBuilder->CalcNumVertices() / aznumeric_cast<float>(m_meshBuilder->GetNumOrgVerts());
        EXPECT_EQ(vertexDupeRatio, 3.0f) << "Vertex ratio for flat shaded cube should be 24/8(Unique Normals/originalVertices).";
    }

    class TriangleFanMeshVerticesTestsFixture
        : public ::testing::WithParamInterface<size_t>,
        public CubeMeshVerticesTests
    {
    public:
        void SetUp() override
        {
            CubeMeshVerticesTests::SetUp();
            /*
             * 1 triangle fan:
             *      / *
             *     *__ *
             *
             *  3 triangles fan:
             *    *  *
             *    \ | / *
             *     \|/__ *
             *
             *  6 triangles fan:
             *        *  *
             *     *  \ | / *
             *    *  __\|/__ *
             *    *    /|
             *     *  / |
             *        * *
             *
             *  etc.
             */

             // Individual vertices + shared/center vertex for triangle fan.
            m_numOrgVertices = (GetParam() * 2) + 1;
        }

        void BuildTriangleFan(const bool useSameNormal)
        {
            SetUpMeshBuilder(m_numOrgVertices);
            const size_t faceCount = (m_numOrgVertices - 1) / 2;
            const int materialId = 0;

            AZ::Vector3 centerVertex(0.0f, 0.0f, 0.0f);
            size_t vertexNum = 1;
            for (size_t faceNum = 0; faceNum < faceCount; ++faceNum)
            {
                m_meshBuilder->BeginPolygon(materialId);
                for (size_t vertexOfFace = 0; vertexOfFace < 3; ++vertexOfFace)
                {
                    AZ::Vector3 normal(0.0f, 0.0f, 1.0f + (useSameNormal ? 0.0f : vertexNum));

                    if (vertexOfFace == 0)
                    {
                        m_posLayer->SetCurrentVertexValue(centerVertex);
                        m_orgVtxLayer->SetCurrentVertexValue(static_cast<AZ::u32>(vertexOfFace));
                        m_normalsLayer->SetCurrentVertexValue(normal);
                        m_meshBuilder->AddPolygonVertex(vertexOfFace);
                    }
                    else
                    {
                        const float angle = vertexNum * (360.0f / m_numOrgVertices);
                        const float x = AZ::Cos(angle);
                        const float y = AZ::Sin(angle);
                        AZ::Vector3 point(x, y, 0);
                        m_posLayer->SetCurrentVertexValue(point);
                        m_orgVtxLayer->SetCurrentVertexValue(static_cast<AZ::u32>(vertexNum));
                        m_normalsLayer->SetCurrentVertexValue(normal);
                        m_meshBuilder->AddPolygonVertex(vertexNum);
                        vertexNum++;
                    }
                }
                m_meshBuilder->EndPolygon();
            }
        }

    public:
        size_t m_numOrgVertices = 0;
    };

    TEST_P(TriangleFanMeshVerticesTestsFixture, SameNormalTriangleFanVertexDedup)
    {
        BuildTriangleFan(/* useSameNormal= */true);
        const float vertexDupeRatio = m_meshBuilder->CalcNumVertices() / aznumeric_cast<float>(m_meshBuilder->GetNumOrgVerts());
        EXPECT_EQ(vertexDupeRatio, 1.0f) << "No duplicated vertex should be created.";
    }

    TEST_P(TriangleFanMeshVerticesTestsFixture, DifferentNormalTriangleFanVertexDedup)
    {
        BuildTriangleFan(/* useSameNormal= */false);
        const size_t faceCount = (m_numOrgVertices - 1) / 2;
        const float vertexDupeRatio = m_meshBuilder->CalcNumVertices() / aznumeric_cast<float>(m_meshBuilder->GetNumOrgVerts());
        const float expectedRatio = (faceCount * 3) / aznumeric_cast<float>(m_numOrgVertices);
        EXPECT_EQ(vertexDupeRatio, expectedRatio) << "Duplicated vertex ratio does not match expected ratio.";
    }

    static constexpr AZStd::array meshVerticesTestData = {
        size_t(1) /* number of triangles in a triangle fan*/,
        3,
        6,
        9
    };

    INSTANTIATE_TEST_CASE_P(TriangleFanZVertexDedupTests,
        TriangleFanMeshVerticesTestsFixture,
        ::testing::ValuesIn(meshVerticesTestData)
    );
} // namespace AZ::MeshBuilder
