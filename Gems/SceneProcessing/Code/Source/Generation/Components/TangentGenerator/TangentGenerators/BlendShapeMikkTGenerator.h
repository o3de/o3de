/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>

namespace AZ::SceneData::GraphData
{
    class BlendShapeData;
}

namespace AZ::TangentGeneration::BlendShape::MikkT
{
    struct MikktCustomData
    {
        AZ::SceneData::GraphData::BlendShapeData* m_blendShapeData;
        size_t m_uvSetIndex;
    };

    // The main generation method.
    bool GenerateTangents(AZ::SceneData::GraphData::BlendShapeData* blendShapeData,
        size_t uvSetIndex,
        AZ::SceneAPI::DataTypes::MikkTSpaceMethod tSpaceMethod = AZ::SceneAPI::DataTypes::MikkTSpaceMethod::TSpace);
} // namespace AZ::TangentGeneration::MikkT
