#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

