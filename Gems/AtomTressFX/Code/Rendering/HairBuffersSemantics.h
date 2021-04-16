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

#pragma once


namespace AZ
{
    namespace RPI
    {
        class BufferAsset;
        class Buffer;
    }

    namespace Render
    {
        namespace Hair
        {
            enum class HairDynamicBuffersSemantics : uint8_t
            {
                Position = 0,
                PositionsPrev,
                PositionsPrevPrev,
                Tangent,
                StrandLevelData,
                NumBufferStreams
            };

            enum class HairGenerationBuffersSemantics : uint8_t
            {
                InitialHairPositions = 0,
                HairRestLengthSRV,
                HairStrandType,
                FollowHairRootOffset,
                BoneSkinningData,
                TressFXSimulationConstantBuffer,
                NumBufferStreams
            };

            enum class HairRenderBuffersSemantics : uint8_t
            {
                HairVertexRenderParams = 0,
                HairTexCoords,
                BaseAlbedo,
                StrandAlbedo,
                RenderCB,
                StrandCB,
                NumBufferStreams
            };

        } // namespace Hair
    } // namespace Render
} // namespace AZ
