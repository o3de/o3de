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
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshDataPrimitiveUtils.h>
#include <algorithm>


namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            namespace DataTypes = AZ::SceneAPI::DataTypes;

            std::unique_ptr<DataTypes::IMeshData > MeshDataPrimitiveUtils::CreateBox(const AZ::Vector3& dimensions, unsigned int materialId)
            {
                return CreateBox(dimensions.GetX(), dimensions.GetY(), dimensions.GetZ(), materialId);
            }

            std::unique_ptr<DataTypes::IMeshData> MeshDataPrimitiveUtils::CreateBox(float xDimension, float yDimension, float zDimension, unsigned int materialId)
            {
                const float c_minDimension = 0.00001f;

                xDimension = std::max(xDimension, c_minDimension);
                yDimension = std::max(yDimension, c_minDimension);
                zDimension = std::max(zDimension, c_minDimension);

                xDimension /= 2.0f;
                yDimension /= 2.0f;
                zDimension /= 2.0f;

                //assume box is centered
                //assume clockwise rotation for faces

                MeshData* meshData = new MeshData();

                //clockwise looking from neg x
                meshData->AddPosition(Vector3(-xDimension, -yDimension, -zDimension));
                meshData->AddPosition(Vector3(-xDimension, -yDimension, zDimension));
                meshData->AddPosition(Vector3(-xDimension, yDimension, zDimension));
                meshData->AddPosition(Vector3(-xDimension, yDimension, -zDimension));

                //clockwise looking from pos x
                meshData->AddPosition(Vector3(xDimension, -yDimension, -zDimension));
                meshData->AddPosition(Vector3(xDimension, yDimension, -zDimension));
                meshData->AddPosition(Vector3(xDimension, yDimension, zDimension));
                meshData->AddPosition(Vector3(xDimension, -yDimension, zDimension));

                //negx
                meshData->AddFace(0, 1, 2, materialId);
                meshData->AddFace(0, 2, 3, materialId);
                //x
                meshData->AddFace(4, 5, 6, materialId);
                meshData->AddFace(4, 6, 7, materialId);
                //negy
                meshData->AddFace(0, 4, 7, materialId);
                meshData->AddFace(0, 7, 1, materialId);
                //y
                meshData->AddFace(3, 2, 6, materialId);
                meshData->AddFace(3, 6, 5, materialId);
                //negz
                meshData->AddFace(0, 3, 4, materialId);
                meshData->AddFace(4, 3, 5, materialId);
                //z
                meshData->AddFace(7, 6, 2, materialId);
                meshData->AddFace(7, 2, 1, materialId);

                return std::unique_ptr<DataTypes::IMeshData>(meshData);
            }
        }
    }
}

