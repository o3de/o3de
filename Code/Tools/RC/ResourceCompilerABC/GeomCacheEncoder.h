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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEENCODER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEENCODER_H
#pragma once


#include "GeomCache.h"
#include "GeomCacheWriter.h"
#include "StealingThreadPool.h"

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/mutex.h>

class GeomCacheEncoder;

struct GeomCacheEncoderFrameInfo
{
    GeomCacheEncoderFrameInfo(GeomCacheEncoder* pEncoder, const uint frameIndex,
        const Alembic::Abc::chrono_t frameTime, const AABB& aabb, const bool bIsLastFrame)
        : m_pEncoder(pEncoder)
        , m_bWritten(false)
        , m_encodeCountdown(0)
        , m_frameIndex(frameIndex)
        , m_frameTime(frameTime)
        , m_frameAABB(aabb)
        , m_bIsLastFrame(bIsLastFrame) {};

    // The encoder that created this structure
    GeomCacheEncoder* m_pEncoder;

    // The ID of this frame
    uint m_frameIndex;

    // Frame type
    GeomCacheFile::EFrameType m_frameType;

    // If frame is the last one
    bool m_bIsLastFrame;

    // The time of this frame
    Alembic::AbcCoreAbstract::chrono_t m_frameTime;

    // If frame was written already
    bool m_bWritten;

    // If this counter reaches 0 the frame is ready to be written
    uint m_encodeCountdown;

    // If this counter reaches 0 the frame can be discarded
    uint m_doneCountdown;

    // AABB of frame
    const AABB m_frameAABB;
};

class GeomCacheEncoder
{
public:
    GeomCacheEncoder(GeomCacheWriter& geomCacheWriter, GeomCache::Node& rootNode,
        const std::vector<GeomCache::Mesh*>& meshes, const bool bUseBFrames, const uint indexFrameDistance);

    void Init();
    void AddFrame(const Alembic::Abc::chrono_t frameTime, const AABB& aabb, const bool bIsLastFrame);

    static bool OptimizeMeshForCompression(GeomCache::Mesh& mesh, const bool bUseMeshPrediction);

private:
    GeomCacheEncoderFrameInfo& GetInfoFromFrameIndex(const uint index);

    void CountNodesRec(GeomCache::Node& currentNode);

    void EncodeFrame(GeomCacheEncoderFrameInfo* pFrame);
    void EncodeAllMeshes(GeomCacheEncoderFrameInfo* pFrame);

    void FrameEncodeFinished(GeomCacheEncoderFrameInfo* pFrame);

    void EncodeNodesRec(GeomCache::Node& currentNode, GeomCacheEncoderFrameInfo* pFrame);
    void EncodeNodeIFrame(const GeomCache::Node& currentNode, const GeomCache::NodeData& rawFrame, std::vector<uint8>& output);

    void EncodeMesh(GeomCache::Mesh* pMesh, GeomCacheEncoderFrameInfo* pFrame);
    void EncodeMeshIFrame(GeomCache::Mesh& mesh, GeomCache::RawMeshFrame& rawMeshFrame, std::vector<uint8>& output);
    void EncodeMeshBFrame(GeomCache::Mesh & mesh,  GeomCache::RawMeshFrame & rawMeshFrame, GeomCache::RawMeshFrame * pPrevFrames[2],
        GeomCache::RawMeshFrame & floorIndexFrame, GeomCache::RawMeshFrame & ceilIndexFrame, std::vector<uint8> &output);

    GeomCacheWriter& m_geomCacheWriter;

    // Set to true if encoder should use bi-directional predicted frames
    bool m_bUseBFrames;
    uint m_indexFrameDistance;

    // Number of animated nodes to compile
    unsigned int m_numNodes;

    // Frame data
    uint m_firstInfoFrameIndex;
    uint m_nextFrameIndex;
    std::deque<std::unique_ptr<GeomCacheEncoderFrameInfo> > m_frames;

    // Global scene structure handles from alembic compiler
    GeomCache::Node& m_rootNode;
    const std::vector<GeomCache::Mesh*>& m_meshes;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEENCODER_H
