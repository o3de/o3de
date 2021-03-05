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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Decodes geom cache data


#ifndef CRYINCLUDE_CRY3DENGINE_GEOMCACHEDECODER_H
#define CRYINCLUDE_CRY3DENGINE_GEOMCACHEDECODER_H
#pragma once

#if defined(USE_GEOM_CACHES)

#include "GeomCacheFileFormat.h"

class CGeomCache;
struct SGeomCacheRenderMeshUpdateContext;
struct SGeomCacheStaticMeshData;

struct SGeomCacheFrameHeader
{
    enum EFrameHeaderState
    {
        eFHS_Uninitialized = 0,
        eFHS_Undecoded = 1,
        eFHS_Decoded = 2
    };

    EFrameHeaderState m_state;
    uint32 m_offset;
};

namespace GeomCacheDecoder
{
    // Decodes an index frame
    void DecodeIFrame(const CGeomCache* pGeomCache, char* pData);

    // Decodes a bi-directional predicted frame
    void DecodeBFrame(const CGeomCache * pGeomCache, char* pData, char* pPrevFramesData[2],
        char* pFloorIndexFrameData, char* pCeilIndexFrameData);

    bool PrepareFillMeshData(SGeomCacheRenderMeshUpdateContext& updateContext, const SGeomCacheStaticMeshData& staticMeshData,
        const char*& pFloorFrameMeshData, const char*& pCeilFrameMeshData, size_t& offsetToNextMesh, float& lerpFactor);

    void FillMeshDataFromDecodedFrame(const bool bMotionBlur, SGeomCacheRenderMeshUpdateContext& updateContext,
        const SGeomCacheStaticMeshData& staticMeshData, const char* pFloorFrameMeshData,
        const char* pCeilFrameMeshData, float lerpFactor);

    // Gets total needed space for uncompressing successive blocks
    uint32 GetDecompressBufferSize(const char* const pStartBlock,  const uint numFrames);

    // Decompresses one block of compressed data with header for input
    bool DecompressBlock(const GeomCacheFile::EBlockCompressionFormat compressionFormat, char* const pDest, const char* const pSource);

    // Decompresses blocks of compressed data with headers for input and output
    bool DecompressBlocks(const GeomCacheFile::EBlockCompressionFormat compressionFormat, char* const pDest,
        const char* const pSource, const uint blockOffset, const uint numBlocks, const uint numHandleFrames);

    Vec3 DecodePosition(const Vec3& aabbMin, const Vec3& aabbSize, const GeomCacheFile::Position& inPosition, const Vec3& convertFactor);
    Vec2 DecodeTexcoord(const GeomCacheFile::Texcoords& inTexcoords, float uvMax);
    Quat DecodeQTangent(const GeomCacheFile::QTangent& inQTangent);

    void TransformAndConvertToTangentAndBitangent(const Quat& rotation, const Quat& inQTangent, SPipTangents& outTangents);
    void ConvertToTangentAndBitangent(const Quat& inQTangent, SPipTangents& outTangents);
};

#endif
#endif // CRYINCLUDE_CRY3DENGINE_GEOMCACHEDECODER_H
