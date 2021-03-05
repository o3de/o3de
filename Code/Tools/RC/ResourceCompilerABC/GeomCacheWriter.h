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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEWRITER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEWRITER_H
#pragma once


#include "GeomCache.h"
#include "GeomCacheBlockCompressor.h"
#include "StealingThreadPool.h"

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>

// This contains the position and size of a disk write
struct DataBlockFileInfo
{
    DataBlockFileInfo()
        : m_position(0)
        , m_size(0) 
    {
    }

    DataBlockFileInfo(uint64 position, uint32 size)
        : m_position(position)
        , m_size(size) 
    {
    }

    uint64 m_position;
    uint32 m_size;
};

// This writes the results from the compression job to disk and will
// not run in the thread pool, because it requires almost no CPU.
class GeomCacheDiskWriteThread
{
public:
    GeomCacheDiskWriteThread(const string& fileName);
    ~GeomCacheDiskWriteThread();

    // Flushes the buffers to disk and exits the thread
    void EndThread();

    // Write with FIFO buffering. This will acquire ownership of the buffer.
    void Write(std::vector<char>& buffer, long offset = 0, int origin = SEEK_CUR);

    size_t GetBytesWritten() { return m_bytesWritten; }

    uint64 GetCurrentPosition() const;

private:
    FILE* m_fileHandle;

    bool m_bExit;

    // Stats
    size_t m_bytesWritten;
};

// This class will receive the data from the GeomCacheWriter.
class GeomCacheBlockCompressionWriter
{
public:
    GeomCacheBlockCompressionWriter(IGeomCacheBlockCompressor* pBlockCompressor, GeomCacheDiskWriteThread& diskWriteThread);
    ~GeomCacheBlockCompressionWriter();

    // This adds data to the current buffer.
    void PushData(const void* data, size_t size);

    // Compresses data in the current buffer and writes it to disk. Returns information about the disk write
    DataBlockFileInfo WriteBlock(bool bCompress, long offset = 0, int origin = SEEK_CUR);

    // Returns the total number of bytes written to disk. This can be less than 
    // the data pushed to the writer because the data written to disk may have
    // been compressed
    uint64 GetTotalBytesWritten() const { return m_totalBytesWritten; }
    size_t GetCurrentDataSize() const { return m_data.size(); }

private:

    void CompressData();

    std::vector<char> m_data;

    GeomCacheDiskWriteThread& m_diskWriteThread;
    IGeomCacheBlockCompressor* m_pBlockCompressor;

    uint64 m_totalBytesWritten;
};

struct GeomCacheWriterStats
{
    uint64 m_headerDataSize;
    uint64 m_staticDataSize;
    uint64 m_animationDataSize;
    uint64 m_uncompressedAnimationSize;
};

class GeomCacheWriter
{
public:
    GeomCacheWriter(const string& filename, GeomCacheFile::EBlockCompressionFormat compressionFormat,
        const uint numFrames, const bool bPlaybackFromMemory,
        const bool b32BitIndices);

    void WriteStaticData(const std::vector<Alembic::Abc::chrono_t>& frameTimes, const std::vector<GeomCache::Mesh*>& meshes, const GeomCache::Node& rootNode);

    void WriteFrame(const uint frameIndex, const AABB& frameAABB, const GeomCacheFile::EFrameType frameType,
        const std::vector<GeomCache::Mesh*>& meshes, GeomCache::Node& rootNode);

    // Flush all buffers, write frame offsets and return the size of the animation stream
    GeomCacheWriterStats FinishWriting();

private:
    void WriteFrameTimes(const std::vector<Alembic::Abc::chrono_t>& frameTimes);
    bool WriteFrameInfos();
    void WriteMeshesStaticData(const std::vector<GeomCache::Mesh*>& meshes);
    void WriteNodeStaticDataRec(const GeomCache::Node& node, const std::vector<GeomCache::Mesh*>& meshes);

    void GetFrameData(GeomCacheFile::SFrameHeader& header, const std::vector<GeomCache::Mesh*>& meshes);
    void WriteNodeFrameRec(GeomCache::Node& node, const std::vector<GeomCache::Mesh*>& meshes, uint32& bytesWritten);

    void WriteMeshFrameData(GeomCache::Mesh& meshData);
    void WriteMeshStaticData(const GeomCache::Mesh& meshData, GeomCacheFile::EStreams streamMask);

    std::vector<DataBlockFileInfo> m_diskInfoForFrames;
    std::vector<Alembic::Abc::chrono_t> m_frameTimes;
    std::vector<uint> m_frameTypes;

    GeomCacheFile::EBlockCompressionFormat m_compressionFormat;

    GeomCacheFile::SHeader m_fileHeader;
    AABB m_animationAABB;
    static const uint m_kNumProgressStatusReports = 10;
    bool m_bShowedStatus[m_kNumProgressStatusReports];

    std::unique_ptr<IGeomCacheBlockCompressor> m_pBlockCompressor;
    std::unique_ptr<GeomCacheDiskWriteThread> m_pDiskWriteThread;
    std::unique_ptr<GeomCacheBlockCompressionWriter> m_pCompressionWriter;

    DataBlockFileInfo m_placeholderForFrameInfos;

    uint64 m_headerWriteSize;
    uint64 m_staticNodeDataSize;
    uint64 m_staticMeshDataSize;
    uint64 m_totalUncompressedAnimationSize;
};
#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEWRITER_H
