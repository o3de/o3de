/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace AZ::SceneAPI::DataTypes { class IMeshData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexUVData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexTangentData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexBitangentData; }

namespace AZ::TangentGeneration::Mesh::MikkT
{
    struct MikktCustomData
    {
        const AZ::SceneAPI::DataTypes::IMeshData* m_meshData;
        const AZ::SceneAPI::DataTypes::IMeshVertexUVData* m_uvData;
        AZ::SceneAPI::DataTypes::IMeshVertexTangentData* m_tangentData;
        AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* m_bitangentData;
    };

    bool GenerateTangents(const AZ::SceneAPI::DataTypes::IMeshData* meshData,
        const AZ::SceneAPI::DataTypes::IMeshVertexUVData* uvData,
        AZ::SceneAPI::DataTypes::IMeshVertexTangentData* outTangentData,
        AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* outBitangentData,
        AZ::SceneAPI::DataTypes::MikkTSpaceMethod tSpaceMethod = AZ::SceneAPI::DataTypes::MikkTSpaceMethod::TSpace);
} // namespace AZ::TangentGeneration::MikkT
