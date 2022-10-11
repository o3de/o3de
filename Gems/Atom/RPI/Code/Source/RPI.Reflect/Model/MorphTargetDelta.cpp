/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/MorphTargetDelta.h>

namespace AZ::RPI
{
    static_assert(sizeof(PackedCompressedMorphTargetDelta) == 32, "The morph target compute shader expects a structured buffer that is exactly 32 bytes per element. If you change MorphTargetDelta, be sure to update MorphTargetSRG.azsli");

    PackedCompressedMorphTargetDelta PackMorphTargetDelta(const CompressedMorphTargetDelta& compressedDelta)
    {
        PackedCompressedMorphTargetDelta packedDelta{ 0,0,0,0,0, {0,0,0} };
        packedDelta.m_morphedVertexIndex = compressedDelta.m_morphedVertexIndex;

        // Position x is in the most significant 16 bits, y is in the least significant 16 bits
        packedDelta.m_positionXY |= (static_cast<uint32_t>(compressedDelta.m_positionX) << 16);
        packedDelta.m_positionXY |=  static_cast<uint32_t>(compressedDelta.m_positionY);

        // Position z is in the most significant 16 bits
        packedDelta.m_positionZNormalXY |= static_cast<uint32_t>(compressedDelta.m_positionZ) << 16;

        // Normal x and y are in the least significant 16 bits (8 bits per channel)
        packedDelta.m_positionZNormalXY |= static_cast<uint32_t>(compressedDelta.m_normalX) << 8;
        packedDelta.m_positionZNormalXY |= static_cast<uint32_t>(compressedDelta.m_normalY);

        // Normal z is in the most significant 8 bits
        packedDelta.m_normalZTangentXYZ |= static_cast<uint32_t>(compressedDelta.m_normalZ) << 24;

        // Tangent x, y, and z are in the least significant 24 bits (8 bits per channel)
        packedDelta.m_normalZTangentXYZ |= static_cast<uint32_t>(compressedDelta.m_tangentX) << 16;
        packedDelta.m_normalZTangentXYZ |= static_cast<uint32_t>(compressedDelta.m_tangentY) << 8;
        packedDelta.m_normalZTangentXYZ |= static_cast<uint32_t>(compressedDelta.m_tangentZ);

        // Bitangents are in the least significant 24 bits (8 bits per channel)
        packedDelta.m_padBitangentXYZ |= static_cast<uint32_t>(compressedDelta.m_bitangentX) << 16;
        packedDelta.m_padBitangentXYZ |= static_cast<uint32_t>(compressedDelta.m_bitangentY) << 8;
        packedDelta.m_padBitangentXYZ |= static_cast<uint32_t>(compressedDelta.m_bitangentZ);

        return packedDelta;
    }

    CompressedMorphTargetDelta UnpackMorphTargetDelta(const PackedCompressedMorphTargetDelta& packedDelta)
    {
        CompressedMorphTargetDelta compressedDelta;
        compressedDelta.m_morphedVertexIndex = packedDelta.m_morphedVertexIndex;

        // Position x is in the most significant 16 bits, y is in the least significant 16 bits
        compressedDelta.m_positionX = packedDelta.m_positionXY >> 16;
        compressedDelta.m_positionY = packedDelta.m_positionXY & 0x0000FFFF;

        // Position z is in the most significant 16 bits
        compressedDelta.m_positionZ = packedDelta.m_positionZNormalXY >> 16;

        // Normal x and y are in the least significant 16 bits (8 bits per channel)
        compressedDelta.m_normalX = (packedDelta.m_positionZNormalXY >> 8) & 0x000000FF;
        compressedDelta.m_normalY =  packedDelta.m_positionZNormalXY       & 0x000000FF;

        // Normal z is in the most significant 8 bits
        compressedDelta.m_normalZ = packedDelta.m_normalZTangentXYZ >> 24;

        // Tangent x, y, and z are in the least significant 24 bits (8 bits per channel)
        compressedDelta.m_tangentX = (packedDelta.m_normalZTangentXYZ >> 16) & 0x000000FF;
        compressedDelta.m_tangentY = (packedDelta.m_normalZTangentXYZ >> 8 ) & 0x000000FF;
        compressedDelta.m_tangentZ =  packedDelta.m_normalZTangentXYZ        & 0x000000FF;

        // Bitangents are in the least significant 24 bits (8 bits per channel)
        compressedDelta.m_bitangentX = (packedDelta.m_padBitangentXYZ >> 16) & 0x000000FF;
        compressedDelta.m_bitangentY = (packedDelta.m_padBitangentXYZ >> 8 ) & 0x000000FF;
        compressedDelta.m_bitangentZ =  packedDelta.m_padBitangentXYZ        & 0x000000FF;

        return compressedDelta;
    }
} // namespace AZ::RPI
