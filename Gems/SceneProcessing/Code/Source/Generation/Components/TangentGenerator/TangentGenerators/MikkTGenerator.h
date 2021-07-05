/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace AZ::SceneAPI::DataTypes { class IMeshData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexUVData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexTangentData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexBitangentData; }

namespace AZ::TangentGeneration::Mesh::MikkT
{
    struct MikktCustomData
    {
        AZ::SceneAPI::DataTypes::IMeshData*                 m_meshData;
        AZ::SceneAPI::DataTypes::IMeshVertexUVData*         m_uvData;
        AZ::SceneAPI::DataTypes::IMeshVertexTangentData*    m_tangentData;
        AZ::SceneAPI::DataTypes::IMeshVertexBitangentData*  m_bitangentData;
    };

    // The main generation method.
    bool GenerateTangents(AZ::SceneAPI::Containers::SceneManifest& manifest, AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::SceneAPI::DataTypes::IMeshData* meshData, size_t uvSet);
} // namespace AZ::TangentGeneration::MikkT
