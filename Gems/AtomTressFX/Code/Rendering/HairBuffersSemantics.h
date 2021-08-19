/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
