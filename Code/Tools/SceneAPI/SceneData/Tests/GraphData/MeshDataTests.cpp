/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <memory>
#include <AzTest/AzTest.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>

namespace AZ
{
    namespace SceneData
    {
        // ===================================================
        // == MeshData Construction                         ==
        // ===================================================

        TEST(MeshData_Construction, Constructor_DefaultConstruction_PositionCountEqualsZero)
        {
            AZ::SceneData::GraphData::MeshData meshData;

            EXPECT_EQ(0, meshData.GetVertexCount());
        }

        TEST(MeshData_Construction, Constructor_DefaultConstruction_HasNoNormalData)
        {
            AZ::SceneData::GraphData::MeshData meshData;

            EXPECT_FALSE(meshData.HasNormalData());
        }

        TEST(MeshData_Construction, AddPosition_AddVector3_GetVertexCountEqualsOne)
        {
            AZ::SceneData::GraphData::MeshData meshData;
            AZ::Vector3 position(1, 0, 0);
            meshData.AddPosition(position);

            EXPECT_EQ(1, meshData.GetVertexCount());
        }

        TEST(MeshData_Construction, AddPosition_AddVector3_GetPositionEqual)
        {
            AZ::SceneData::GraphData::MeshData meshData;
            AZ::Vector3 position(0.1f, 0.2f, 0.3f);
            meshData.AddPosition(position);
            const AZ::Vector3& storedPosition = meshData.GetPosition(0);
            EXPECT_FLOAT_EQ(position.GetX(), storedPosition.GetX());
            EXPECT_FLOAT_EQ(position.GetY(), storedPosition.GetY());
            EXPECT_FLOAT_EQ(position.GetZ(), storedPosition.GetZ());
        }

        TEST(MeshData_Construction, AddNormal_AddVector3_HasNormalData)
        {
            AZ::SceneData::GraphData::MeshData meshData;
            AZ::Vector3 normal(1, 0, 0);
            meshData.AddNormal(normal);

            EXPECT_TRUE(meshData.HasNormalData());
        }

        TEST(MeshData_Construction, AddNormal_AddVector3_GetNormalEqual)
        {
            AZ::SceneData::GraphData::MeshData meshData;
            AZ::Vector3 normal(0.1f, 0.2f, 0.3f);
            meshData.AddNormal(normal);
            const AZ::Vector3& storedNormal = meshData.GetNormal(0);
            EXPECT_FLOAT_EQ(normal.GetX(), storedNormal.GetX());
            EXPECT_FLOAT_EQ(normal.GetY(), storedNormal.GetY());
            EXPECT_FLOAT_EQ(normal.GetZ(), storedNormal.GetZ());
        }

        TEST(MeshData_Construction, AddFace_AddValidFace_GetFaceEqual)
        {
            AZ::SceneData::GraphData::MeshData meshData;
            AZ::SceneAPI::DataTypes::IMeshData::Face face;
            face.vertexIndex[0] = 0;
            face.vertexIndex[1] = 1;
            face.vertexIndex[2] = 2;

            meshData.AddFace(face);

            EXPECT_EQ(1, meshData.GetFaceCount());

            const AZ::SceneAPI::DataTypes::IMeshData::Face& testValue = meshData.GetFaceInfo(0);
            EXPECT_EQ(testValue.vertexIndex[0], face.vertexIndex[0]);
            EXPECT_EQ(testValue.vertexIndex[1], face.vertexIndex[1]);
            EXPECT_EQ(testValue.vertexIndex[2], face.vertexIndex[2]);
        }

        TEST(MeshData_Construction, AddFace_AddValidFaceIndexes_GetFaceEqual)
        {
            AZ::SceneData::GraphData::MeshData meshData;

            meshData.AddFace(0, 1, 2);

            EXPECT_EQ(1, meshData.GetFaceCount());

            const AZ::SceneAPI::DataTypes::IMeshData::Face& testValue = meshData.GetFaceInfo(0);
            EXPECT_EQ(testValue.vertexIndex[0], 0);
            EXPECT_EQ(testValue.vertexIndex[1], 1);
            EXPECT_EQ(testValue.vertexIndex[2], 2);
        }

        TEST(MeshData_Construction, AddFace_AddValidFace_GetFaceMaterialEqual)
        {
            AZ::SceneData::GraphData::MeshData meshData;
            AZ::SceneAPI::DataTypes::IMeshData::Face face;
            face.vertexIndex[0] = 0;
            face.vertexIndex[1] = 1;
            face.vertexIndex[2] = 2;

            meshData.AddFace(face);

            EXPECT_EQ(0, meshData.GetFaceMaterialId(0));
        }
    }
}
