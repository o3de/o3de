/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Containers/Scene.h>

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
    bool GenerateTangents(AZ::SceneData::GraphData::BlendShapeData* blendShapeData, size_t uvSetIndex);
} // namespace AZ::TangentGeneration::MikkT
