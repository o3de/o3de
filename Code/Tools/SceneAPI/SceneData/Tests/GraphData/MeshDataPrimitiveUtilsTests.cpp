/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <memory>
#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshDataPrimitiveUtils.h>

#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <float.h>

namespace AZ
{
    namespace SceneData
    {
        // ===================================================
        // == MeshDataPrimitiveUtilTests                    ==
        // ===================================================

        bool FaceWindingPointsDirection(const AZ::SceneAPI::DataTypes::IMeshData::Face& face, const std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData>& mesh, const AZ::Vector3 expectedNormal)
        {
            AZ::Vector3 v0 = mesh->GetPosition(face.vertexIndex[0]);
            AZ::Vector3 v1 = mesh->GetPosition(face.vertexIndex[1]);
            AZ::Vector3 v2 = mesh->GetPosition(face.vertexIndex[2]);

            AZ::Vector3 dir1 = v1 - v0;
            AZ::Vector3 dir2 = v2 - v0;

            AZ::Vector3 normal = dir1.Cross(dir2);

            return normal.Dot(expectedNormal) >= (1.0f - FLT_EPSILON);
        }

        bool PointsMatch(const AZ::Vector3& orig, const AZ::Vector3& testPosition)
        {
            if (orig.GetX() != testPosition.GetX())
            {
                return false;
            }
            if (orig.GetY() != testPosition.GetY())
            {
                return false;
            }
            if (orig.GetZ() != testPosition.GetZ())
            {
                return false;
            }
            return true;
        }

        TEST(MeshDataPrimitiveUtils, CreateBox_BasicValues_BoxHasCorrectTopology)
        {
            std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> mesh
                = AZ::SceneData::GraphData::MeshDataPrimitiveUtils::CreateBox(1.0f, 2.0f, 3.0f);

            ASSERT_NE(nullptr, mesh);
            EXPECT_EQ(8, mesh->GetVertexCount());
            EXPECT_EQ(12, mesh->GetFaceCount());
        }

        TEST(MeshDataPrimitiveUtils, CreateBox_BasicVectorValues_BoxHasCorrectTopology)
        {
            AZ::Vector3 dims(1.0f, 2.0f, 3.0f);
            std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> mesh
                = AZ::SceneData::GraphData::MeshDataPrimitiveUtils::CreateBox(dims);

            ASSERT_NE(nullptr, mesh);
            EXPECT_EQ(8, mesh->GetVertexCount());
            EXPECT_EQ(12, mesh->GetFaceCount());
        }

        TEST(MeshDataPrimitiveUtils, CreateBox_BasicValues_XFacesPointCorrectDirection)
        {
            std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> mesh
                = AZ::SceneData::GraphData::MeshDataPrimitiveUtils::CreateBox(1.0f, 2.0f, 3.0f);

            AZ::SceneAPI::DataTypes::IMeshData::Face face = mesh->GetFaceInfo(0);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(-1.0f, 0.0f, 0.0f)));

            face = mesh->GetFaceInfo(1);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(-1.0f, 0.0f, 0.0f)));

            face = mesh->GetFaceInfo(2);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(1.0f, 0.0f, 0.0f)));

            face = mesh->GetFaceInfo(3);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(1.0f, 0.0f, 0.0f)));
        }

        TEST(MeshDataPrimitiveUtils, CreateBox_BasicValues_YFacesPointCorrectDirection)
        {
            std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> mesh
                = AZ::SceneData::GraphData::MeshDataPrimitiveUtils::CreateBox(1.0f, 2.0f, 3.0f);

            AZ::SceneAPI::DataTypes::IMeshData::Face face = mesh->GetFaceInfo(4);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(0.0f, -1.0f, 0.0f)));

            face = mesh->GetFaceInfo(5);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(0.0f, -1.0f, 0.0f)));

            face = mesh->GetFaceInfo(6);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(0.0f, 1.0f, 0.0f)));

            face = mesh->GetFaceInfo(7);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(0.0f, 1.0f, 0.0f)));
        }

        TEST(MeshDataPrimitiveUtils, CreateBox_BasicValues_ZFacesPointCorrectDirection)
        {
            std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> mesh
                = AZ::SceneData::GraphData::MeshDataPrimitiveUtils::CreateBox(1.0f, 2.0f, 3.0f);

            AZ::SceneAPI::DataTypes::IMeshData::Face face = mesh->GetFaceInfo(8);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(0.0f, 0.0f, -1.0f)));

            face = mesh->GetFaceInfo(9);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(0.0f, 0.0f, -1.0f)));

            face = mesh->GetFaceInfo(10);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(0.0f, 0.0f, 1.0f)));

            face = mesh->GetFaceInfo(11);
            EXPECT_TRUE(FaceWindingPointsDirection(face, mesh, AZ::Vector3(0.0f, 0.0f, 1.0f)));
        }

        TEST(MeshDataPrimitiveUtils, CreateBox_BasicValues_VertexPositionsValid)
        {
            AZ::Vector3 dims(1.0f, 2.0f, 3.0f);
            std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> mesh
                = AZ::SceneData::GraphData::MeshDataPrimitiveUtils::CreateBox(dims);

            dims /= 2.0f;

            AZ::Vector3 position = mesh->GetPosition(0);
            EXPECT_TRUE(PointsMatch(Vector3(-dims.GetX(), -dims.GetY(), -dims.GetZ()), position));
            position = mesh->GetPosition(1);
            EXPECT_TRUE(PointsMatch(Vector3(-dims.GetX(), -dims.GetY(), dims.GetZ()), position));
            position = mesh->GetPosition(2);
            EXPECT_TRUE(PointsMatch(Vector3(-dims.GetX(), dims.GetY(), dims.GetZ()), position));
            position = mesh->GetPosition(3);
            EXPECT_TRUE(PointsMatch(Vector3(-dims.GetX(), dims.GetY(), -dims.GetZ()), position));
            position = mesh->GetPosition(4);
            EXPECT_TRUE(PointsMatch(Vector3(dims.GetX(), -dims.GetY(), -dims.GetZ()), position));
            position = mesh->GetPosition(5);
            EXPECT_TRUE(PointsMatch(Vector3(dims.GetX(), dims.GetY(), -dims.GetZ()), position));
            position = mesh->GetPosition(6);
            EXPECT_TRUE(PointsMatch(Vector3(dims.GetX(), dims.GetY(), dims.GetZ()), position));
            position = mesh->GetPosition(7);
            EXPECT_TRUE(PointsMatch(Vector3(dims.GetX(), -dims.GetY(), dims.GetZ()), position));
        }
    }
}
