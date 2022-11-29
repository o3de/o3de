/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Generation/Components/MeshOptimizer/MeshBuilder.h>
#include <Generation/Components/MeshOptimizer/MeshBuilderSubMesh.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace AZ::MeshBuilder
{
    struct MeshBuilderFixtureParameter
    {
        size_t m_numRows;
        size_t m_numColumns;
        size_t m_maxSubMeshVertices;
    };

    class MeshBuilderFixture
        : public UnitTest::LeakDetectionFixture
        , public ::testing::WithParamInterface<MeshBuilderFixtureParameter>
    {
    public:
        static void AddVertex(MeshBuilder* meshBuilder, size_t orgVtxNr,
            MeshBuilderVertexAttributeLayerVector3* posLayer, const AZ::Vector3& position,
            MeshBuilderVertexAttributeLayerVector3* normalsLayer, const AZ::Vector3& normal)
        {
            posLayer->SetCurrentVertexValue(position);
            normalsLayer->SetCurrentVertexValue(normal);
            meshBuilder->AddPolygonVertex(orgVtxNr);
        }

        static AZStd::unique_ptr<MeshBuilder> GenerateMesh(size_t numRows, size_t numColumns, size_t maxSubMeshVertices)
        {
            const size_t numOrgVertices = numRows * numColumns;
            auto meshBuilder = AZStd::make_unique<MeshBuilder>(numOrgVertices, /*maxBonesPerSubMesh=*/64, maxSubMeshVertices);

            meshBuilder->AddLayer<MeshBuilderVertexAttributeLayerUInt32>(numOrgVertices, false, false);
            auto* posLayer = meshBuilder->AddLayer<MeshBuilderVertexAttributeLayerVector3>(numOrgVertices, false, true);
            auto* normalsLayer = meshBuilder->AddLayer<MeshBuilderVertexAttributeLayerVector3>(numOrgVertices, false, true);


            size_t materialIndex = 0;

            const AZ::Vector3 normal(0.0f, 0.0f, 1.0f);
            for (size_t row = 0; row < (numRows-1); ++row)
            {
                const auto rowFloat = static_cast<float>(row);
                for (size_t column = 0; column < (numColumns-1); ++column)
                {
                    const auto columnFloat = static_cast<float>(column);

                    // 4 +----------+ 3
                    //   |         /|
                    //   |   T2   / |
                    //   |       /  |
                    //   |      /   |
                    //   |     /    |
                    //   |    /     |
                    //   |   /      |
                    //   |  /   T1  |
                    //   | /        |
                    // 1 +----------+ 2

                    const size_t orgVtxNr1 = column * numRows + row;
                    const size_t orgVtxNr2 = (column + 1) * numRows + row;
                    const size_t orgVtxNr3 = (column + 1) * numRows + (row+1);
                    const size_t orgVtxNr4 = column * numRows + (row+1);

                    const AZ::Vector3 pos1(columnFloat, rowFloat, 0.0f);
                    const AZ::Vector3 pos2(columnFloat+1.0f, rowFloat, 0.0f);
                    const AZ::Vector3 pos3(columnFloat+1.0f, rowFloat+1.0f, 0.0f);
                    const AZ::Vector3 pos4(columnFloat, rowFloat+1.0f, 0.0f);

                    // Triangle 1
                    meshBuilder->BeginPolygon(materialIndex);
                        AddVertex(meshBuilder.get(), orgVtxNr1, posLayer, pos1, normalsLayer, normal);
                        AddVertex(meshBuilder.get(), orgVtxNr2, posLayer, pos2, normalsLayer, normal);
                        AddVertex(meshBuilder.get(), orgVtxNr3, posLayer, pos3, normalsLayer, normal);
                    meshBuilder->EndPolygon();

                    // Triangle 2
                    meshBuilder->BeginPolygon(materialIndex);
                        AddVertex(meshBuilder.get(), orgVtxNr1, posLayer, pos1, normalsLayer, normal);
                        AddVertex(meshBuilder.get(), orgVtxNr3, posLayer, pos3, normalsLayer, normal);
                        AddVertex(meshBuilder.get(), orgVtxNr4, posLayer, pos4, normalsLayer, normal);
                    meshBuilder->EndPolygon();
                }
            }

            EXPECT_EQ(meshBuilder->GetNumOrgVerts(), numRows * numColumns);
            EXPECT_EQ(meshBuilder->GetNumPolygons(), (numRows - 1) * (numColumns - 1) * 2);

            return meshBuilder;
        }

        static void CheckMaxSubMeshVertices(MeshBuilder* meshBuilder, size_t maxSubMeshVertices)
        {
            const size_t numSubMeshes = meshBuilder->GetNumSubMeshes();
            for (size_t i = 0; i < numSubMeshes; ++i)
            {
                const MeshBuilderSubMesh* subMesh = meshBuilder->GetSubMesh(i);
                EXPECT_TRUE(subMesh->GetNumVertices() <= maxSubMeshVertices)
                    << "Sub mesh splitting failed. Sub mesh contains more than the max number of allowed vertices.";
            }
        }

        static void CheckSubMeshSplits(MeshBuilder* meshBuilder, size_t maxSubMeshVertices)
        {
            const size_t numPolygons = meshBuilder->GetNumPolygons();
            const size_t numSubMeshes = meshBuilder->GetNumSubMeshes();

            size_t numAccumulatedPolys = 0;
            size_t numAccumulatedSubMeshVertices = 0;
            for (size_t i = 0; i < numSubMeshes; ++i)
            {
                const MeshBuilderSubMesh* subMesh = meshBuilder->GetSubMesh(i);
                numAccumulatedSubMeshVertices += subMesh->GetNumVertices();

                const size_t numPolysPerSubMesh = subMesh->GetNumPolygons();
                numAccumulatedPolys += numPolysPerSubMesh;
            }

            EXPECT_EQ(numPolygons, numAccumulatedPolys)
                << "Accumulated polygon count for sub meshes does not match total polygon count.";

            if (numAccumulatedSubMeshVertices <= maxSubMeshVertices)
            {
                EXPECT_EQ(numSubMeshes, 1)
                    << "The vertex count is lower than the maximum allowed vertex count per sub mesh. No split needed and expecting a single sub mesh.";
            }
            else
            {
                size_t bestCastNumSubMeshes = numAccumulatedSubMeshVertices / maxSubMeshVertices;
                EXPECT_TRUE(numSubMeshes >= bestCastNumSubMeshes)
                    << "The number of sub meshes is lower than the theoretical best case. One or many splits got missed.";
            }
        }
    };

    TEST_P(MeshBuilderFixture, MeshBuilderTest_MaxSubMeshVertices)
    {
        const MeshBuilderFixtureParameter& param = GetParam();
        AZStd::unique_ptr<MeshBuilder> meshBuilder = GenerateMesh(param.m_numRows, param.m_numColumns, param.m_maxSubMeshVertices);
        CheckMaxSubMeshVertices(meshBuilder.get(), param.m_maxSubMeshVertices);
        CheckSubMeshSplits(meshBuilder.get(), param.m_maxSubMeshVertices);
    }

    static constexpr AZStd::array meshBuilderMaxSubMeshVerticesTestData
    {
        MeshBuilderFixtureParameter {
            /*m_numRows=*/2,
            /*m_numColumns=*/2,
            /*m_maxSubMeshVertices=*/100
        },
        MeshBuilderFixtureParameter {
            4,
            4,
            3
        },
        MeshBuilderFixtureParameter {
            4,
            4,
            15
        },
        MeshBuilderFixtureParameter {
            4,
            32,
            9
        },
        MeshBuilderFixtureParameter {
            64,
            16,
            50
        },
        MeshBuilderFixtureParameter {
            100,
            100,
            64
        },
        MeshBuilderFixtureParameter {
            100,
            100,
            512
        },
        MeshBuilderFixtureParameter {
            100,
            100,
            1000
        },
        MeshBuilderFixtureParameter {
            1000,
            100,
            10000
        }
    };

    INSTANTIATE_TEST_CASE_P(MeshBuilderTest_MaxSubMeshVertices,
        MeshBuilderFixture,
        ::testing::ValuesIn(meshBuilderMaxSubMeshVerticesTestData));
} // namespace AZ::MeshBuilder
