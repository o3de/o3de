#pragma once

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
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class MeshDataPrimitiveUtils
            {
            public:
                SCENE_DATA_API static std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> CreateBox(
                    const AZ::Vector3& dimensions,
                    unsigned int materialId = AZ::SceneAPI::DataTypes::IMeshData::s_invalidMaterialId
                    );
                SCENE_DATA_API static std::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> CreateBox(
                    float xDimension,
                    float yDimension,
                    float zDimension,
                    unsigned int materialId = AZ::SceneAPI::DataTypes::IMeshData::s_invalidMaterialId
                    );
            };
        }
    }
}

