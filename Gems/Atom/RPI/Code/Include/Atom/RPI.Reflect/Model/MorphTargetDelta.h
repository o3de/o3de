/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <stdint.h>

namespace AZ::RPI
{
    namespace MorphTargetDeltaConstants
    {
        // Min/max values for compression should correspond to what is used in MorphTargetCompression.azsli

        // A tangent/bitangent/normal will be between -1.0 and 1.0 on any given axis
        // The largest a delta needs to be is 2.0 in either positive or negative direction
        // to modify a value from 1 to -1 or from -1 to 1
        constexpr float s_tangentSpaceDeltaMin = -2.0f;
        constexpr float s_tangentSpaceDeltaMax = 2.0f;
    }

    //! This class represents the data that is passed to the morph target compute shader for an individual delta
    //! See MorphTargetSRG.azsli for the corresponding shader struct
    //! It is 16-byte aligned to work with structured buffers
    //! Because AZSL does not support 16 or 8 bit integer types, the data is packed into 32 bit unsigned ints
    struct PackedCompressedMorphTargetDelta
    {
        // The index of the vertex being modified by this delta
        uint32_t m_morphedVertexIndex;
        // 16 bits per component for position deltas
        uint32_t m_positionXY;
        // Position z plus 8 bits per component for normal deltas
        uint32_t m_positionZNormalXY;
        // Normal z plus 8 bits per component for tangent deltas
        uint32_t m_normalZTangentXYZ;
        // 8 bit padding plus 8 bits per component for bitangent deltas
        uint32_t m_padBitangentXYZ;
        // Explicit padding so the struct is 16 byte aligned for structured buffers
        uint32_t m_pad[3];
    };

    //! A morph target delta that is compressed, but split into individual components
    //! It is not intended to be sent to the GPU, so it is not explicitly padded for alignment
    struct CompressedMorphTargetDelta
    {
        // 32 bit indices
        uint32_t m_morphedVertexIndex;

        // 16 bits per component for position deltas
        uint16_t m_positionX;
        uint16_t m_positionY;
        uint16_t m_positionZ;

        // 8 bits per component for normal, tangent, and bitangent deltas
        uint8_t m_normalX;
        uint8_t m_normalY;
        uint8_t m_normalZ;

        uint8_t m_tangentX;
        uint8_t m_tangentY;
        uint8_t m_tangentZ;

        uint8_t m_bitangentX;
        uint8_t m_bitangentY;
        uint8_t m_bitangentZ;
    };

    ATOM_RPI_REFLECT_API PackedCompressedMorphTargetDelta PackMorphTargetDelta(const CompressedMorphTargetDelta& compressedDelta);
    ATOM_RPI_REFLECT_API CompressedMorphTargetDelta UnpackMorphTargetDelta(const PackedCompressedMorphTargetDelta& packedDelta);
} // namespace AZ::RPI
