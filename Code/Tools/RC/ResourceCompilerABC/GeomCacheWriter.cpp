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

#include "ResourceCompilerABC_precompiled.h"
#include "GeomCacheWriter.h"
#include "GeomCacheBlockCompressor.h"
#include <GeomCacheFileFormat.h>

#include <AzCore/std/bind/bind.h>
#include <AzCore/std/chrono/types.h>
#include <AzCore/std/parallel/lock.h>

GeomCacheDiskWriteThread::GeomCacheDiskWriteThread(const string& fileName)
    : m_bExit(false)
{
    m_fileHandle = nullptr; 
    azfopen(&m_fileHandle, fileName, "w+b");
}

GeomCacheDiskWriteThread::~GeomCacheDiskWriteThread()
{
    assert(m_bExit);
    fclose(m_fileHandle);
}

void GeomCacheDiskWriteThread::Write(std::vector<char>& buffer, long offset, int origin)
{
    if (buffer.empty() || m_bExit)
    {
        return;
    }

    fseek(m_fileHandle, offset, origin);

    // Write and clear current read buffer
    const size_t bufferSize = buffer.size();
    m_bytesWritten += bufferSize;
    size_t bytesWritten = fwrite(buffer.data(), 1, bufferSize, m_fileHandle);

    //RCLog("Written %Iu/%Iu bytes with error: %d", bytesWritten, bufferSize, ferror(m_fileHandle));
}

void GeomCacheDiskWriteThread::EndThread()
{
    m_bExit = true;
    RCLog("  Disk write thread exited");
}

uint64 GeomCacheDiskWriteThread::GetCurrentPosition() const
{
#if defined(AZ_PLATFORM_LINUX)
    off_t position;
    position = ftello(m_fileHandle);
    return static_cast<uint64>(position);
#else
    fpos_t position;
    fgetpos(m_fileHandle, &position);
    return static_cast<uint64>(position);
#endif
}

GeomCacheBlockCompressionWriter::GeomCacheBlockCompressionWriter(IGeomCacheBlockCompressor* pBlockCompressor, GeomCacheDiskWriteThread& diskWriteThread)
    : m_pBlockCompressor(pBlockCompressor)
    , m_diskWriteThread(diskWriteThread)
{
}

GeomCacheBlockCompressionWriter::~GeomCacheBlockCompressionWriter()
{
}

void GeomCacheBlockCompressionWriter::PushData(const void* data, size_t size)
{
    size_t writePosition = m_data.size();
    m_data.resize(m_data.size() + size);
    memcpy(&m_data[writePosition], data, size);
}

DataBlockFileInfo GeomCacheBlockCompressionWriter::WriteBlock(bool bCompress, long offset, int origin)
{
    DataBlockFileInfo diskStats;
    diskStats.m_size = m_data.size(); 

    if (m_data.empty())
    {
        return diskStats;
    }

    // Fill job data
    if (bCompress)
    {
        CompressData();
    }

    diskStats.m_position = m_diskWriteThread.GetCurrentPosition();
    diskStats.m_size = m_data.size();

    m_diskWriteThread.Write(m_data, offset, origin);

    m_totalBytesWritten += diskStats.m_size;

    m_data.clear();

    return diskStats;
}

void GeomCacheBlockCompressionWriter::CompressData()
{
    // Remember uncompressed size
    const uint32 uncompressedSize = m_data.size();

    // Compress job data
    std::vector<char> compressedData;
    m_pBlockCompressor->Compress(m_data, compressedData);

    std::vector<char> dataBuffer;
    dataBuffer.reserve(sizeof(GeomCacheFile::SCompressedBlockHeader) + compressedData.size());
    dataBuffer.resize(sizeof(GeomCacheFile::SCompressedBlockHeader));

    GeomCacheFile::SCompressedBlockHeader* pBlockHeader = reinterpret_cast<GeomCacheFile::SCompressedBlockHeader*>(dataBuffer.data());
    pBlockHeader->m_uncompressedSize = uncompressedSize;
    pBlockHeader->m_compressedSize = compressedData.size();

    dataBuffer.insert(dataBuffer.begin() + sizeof(GeomCacheFile::SCompressedBlockHeader), compressedData.begin(), compressedData.end());

    m_data = dataBuffer;
}

GeomCacheWriter::GeomCacheWriter(const string& filename, GeomCacheFile::EBlockCompressionFormat compressionFormat,
    [[maybe_unused]] const uint numFrames, const bool bPlaybackFromMemory, const bool b32BitIndices)
    : m_compressionFormat(compressionFormat)
    , m_totalUncompressedAnimationSize(0)
{
    m_animationAABB.Reset();

    m_fileHeader.m_flags |= bPlaybackFromMemory ? GeomCacheFile::eFileHeaderFlags_PlaybackFromMemory : 0;
    m_fileHeader.m_flags |= b32BitIndices ? GeomCacheFile::eFileHeaderFlags_32BitIndices : 0;

    m_pDiskWriteThread.reset(new GeomCacheDiskWriteThread(filename));

    switch (compressionFormat)
    {
    case GeomCacheFile::eBlockCompressionFormat_None:
        m_pBlockCompressor.reset(new GeomCacheStoreBlockCompressor);
        break;
    case GeomCacheFile::eBlockCompressionFormat_Deflate:
        m_pBlockCompressor.reset(new GeomCacheDeflateBlockCompressor);
        break;
    case GeomCacheFile::eBlockCompressionFormat_LZ4HC:
        m_pBlockCompressor.reset(new GeomCacheLZ4HCBlockCompressor);
        break;
    case GeomCacheFile::eBlockCompressionFormat_ZSTD:
        m_pBlockCompressor.reset(new GeomCacheZStdBlockCompressor);
        break;
    }

    m_pCompressionWriter.reset(new GeomCacheBlockCompressionWriter(m_pBlockCompressor.get(), *m_pDiskWriteThread));

    for (uint i = 0; i < m_kNumProgressStatusReports; ++i)
    {
        m_bShowedStatus[i] = false;
    }
}

GeomCacheWriterStats GeomCacheWriter::FinishWriting()
{
    GeomCacheWriterStats stats = {0};

    // Write frame offsets
    RCLog("  Writing frame offsets/sizes...");
    if (!WriteFrameInfos())
    {
        m_pDiskWriteThread->EndThread();
        return stats;
    }

    // Last write header with proper signature, AABB & static mesh data offset
    m_fileHeader.m_signature = GeomCacheFile::kFileSignature;
    m_fileHeader.m_totalUncompressedAnimationSize = m_totalUncompressedAnimationSize;
    for (unsigned int i = 0; i < 3; ++i)
    {
        m_fileHeader.m_aabbMin[i] = m_animationAABB.min[i];
        m_fileHeader.m_aabbMax[i] = m_animationAABB.max[i];
    }
    m_pCompressionWriter->PushData((void*)&m_fileHeader, sizeof(GeomCacheFile::SHeader));

    uint64 currentBytesWritten = m_pCompressionWriter->GetTotalBytesWritten();
    m_pCompressionWriter->WriteBlock(false, 0, SEEK_SET);
    uint64 animationBytesWritten = m_pCompressionWriter->GetTotalBytesWritten() - currentBytesWritten;

    // Destroy compression writer and block compressor
    m_pCompressionWriter.reset(nullptr);
    m_pBlockCompressor.reset(nullptr);

    m_pDiskWriteThread->EndThread();

    stats.m_headerDataSize = m_headerWriteSize + m_placeholderForFrameInfos.m_size + m_staticNodeDataSize;
    stats.m_staticDataSize = m_staticMeshDataSize;
    stats.m_animationDataSize = animationBytesWritten;
    stats.m_uncompressedAnimationSize = m_totalUncompressedAnimationSize;

    return stats;
}

void GeomCacheWriter::WriteStaticData(const std::vector<Alembic::Abc::chrono_t>& frameTimes, const std::vector<GeomCache::Mesh*>& meshes, const GeomCache::Node& rootNode)
{
    RCLog("Writing static data to disk...");

    DataBlockFileInfo stats;
    // Write header with 0 signature, to avoid the engine to read incomplete caches.
    // Correct signature will be written in FinishWriting at the end
    m_fileHeader.m_blockCompressionFormat = m_compressionFormat;
    m_fileHeader.m_numFrames = frameTimes.size();
    m_pCompressionWriter->PushData((void*)&m_fileHeader, sizeof(GeomCacheFile::SHeader));
    stats = m_pCompressionWriter->WriteBlock(false);
    m_headerWriteSize = stats.m_size;

    m_diskInfoForFrames.resize(frameTimes.size());

    // Leave space for frame offsets/sizes (don't know them until frames are written)
    // and pass future object to get write position for WriteFrameOffsets later
    std::vector<GeomCacheFile::SFrameInfo> frameInfos(frameTimes.size());
    memset(frameInfos.data(), 0, sizeof(GeomCacheFile::SFrameInfo) * frameInfos.size());
    m_pCompressionWriter->PushData(frameInfos.data(), sizeof(GeomCacheFile::SFrameInfo) * frameInfos.size());
    m_placeholderForFrameInfos = m_pCompressionWriter->WriteBlock(false);

    // Reserve frame type array and store frame times
    m_frameTypes.resize(frameTimes.size(), 0);
    m_frameTimes = frameTimes;

    // Write compressed physics geometries and node data
    RCLog("  Writing node data");
    WriteNodeStaticDataRec(rootNode, meshes);
    stats = m_pCompressionWriter->WriteBlock(true);
    m_staticNodeDataSize = stats.m_size;

    // Write compressed static mesh data
    RCLog("  Writing mesh data (%u meshes)", (uint)meshes.size());
    WriteMeshesStaticData(meshes);
    stats = m_pCompressionWriter->WriteBlock(true);
    m_staticMeshDataSize = stats.m_size;
}

void GeomCacheWriter::WriteFrameTimes(const std::vector<Alembic::Abc::chrono_t>& frameTimes)
{
    RCLog("  Writing frame times");

    const uint32 numFrameTimes = frameTimes.size();
    m_pCompressionWriter->PushData((void*)&numFrameTimes, sizeof(uint32));

    // Convert frame times to float
    std::vector<float> floatFrameTimes;
    floatFrameTimes.reserve(frameTimes.size());
    for (auto iter = frameTimes.begin(); iter != frameTimes.end(); ++iter)
    {
        floatFrameTimes.push_back((float)*iter);
    }

    // Write out time array
    m_pCompressionWriter->PushData(floatFrameTimes.data(), sizeof(float) * floatFrameTimes.size());
}

bool GeomCacheWriter::WriteFrameInfos()
{
    const size_t numFrames = m_diskInfoForFrames.size();

    std::vector<GeomCacheFile::SFrameInfo> frameInfos(numFrames);

    for (size_t i = 0; i < numFrames; ++i)
    {
        frameInfos[i].m_frameType = m_frameTypes[i];
        frameInfos[i].m_frameOffset = m_diskInfoForFrames[i].m_position;
        frameInfos[i].m_frameSize = m_diskInfoForFrames[i].m_size;
        frameInfos[i].m_frameTime = (float)m_frameTimes[i];

        if (frameInfos[i].m_frameOffset == 0 || frameInfos[i].m_frameSize == 0)
        {
            RCLogError("Invalid frame offset or size");
            return false;
        }
    }
    m_pCompressionWriter->PushData(frameInfos.data(), sizeof(GeomCacheFile::SFrameInfo) * frameInfos.size());

    // Overwrite space left free for frame infos
    m_pCompressionWriter->WriteBlock(false, (long)m_placeholderForFrameInfos.m_position, SEEK_SET);

    return true;
}

void GeomCacheWriter::WriteMeshesStaticData(const std::vector<GeomCache::Mesh*>& meshes)
{
    const uint32 numMeshes = meshes.size();
    m_pCompressionWriter->PushData((void*)&numMeshes, sizeof(uint32));

    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        const GeomCache::Mesh* pMesh = *iter;

        const std::string& fullName = pMesh->m_abcMesh.getFullName();
        const uint nameLength = fullName.size() + 1;

        GeomCacheFile::SMeshInfo meshInfo;
        meshInfo.m_constantStreams = static_cast<uint8>(pMesh->m_constantStreams);
        meshInfo.m_animatedStreams = static_cast<uint8>(pMesh->m_animatedStreams);
        meshInfo.m_positionPrecision[0] = pMesh->m_positionPrecision[0];
        meshInfo.m_positionPrecision[1] = pMesh->m_positionPrecision[1];
        meshInfo.m_positionPrecision[2] = pMesh->m_positionPrecision[2];
        meshInfo.m_uvMax = pMesh->m_uvMax;
        meshInfo.m_numVertices = static_cast<uint32>(pMesh->m_staticMeshData.m_positions.size());
        meshInfo.m_numMaterials = static_cast<uint32>(pMesh->m_indicesMap.size());
        meshInfo.m_flags = pMesh->m_bUsePredictor ? GeomCacheFile::eMeshIFrameFlags_UsePredictor : 0;
        meshInfo.m_nameLength = nameLength;
        meshInfo.m_hash = pMesh->m_hash;

        for (unsigned int i = 0; i < 3; ++i)
        {
            meshInfo.m_aabbMin[i] = pMesh->m_aabb.min[i];
            meshInfo.m_aabbMax[i] = pMesh->m_aabb.max[i];
        }

        m_pCompressionWriter->PushData((void*)&meshInfo, sizeof(GeomCacheFile::SMeshInfo));
        m_pCompressionWriter->PushData(fullName.c_str(), nameLength);

        // Write out material IDs
        for (auto iter2 = pMesh->m_indicesMap.begin(); iter2 != pMesh->m_indicesMap.end(); ++iter2)
        {
            const uint16 materialId = iter2->first;
            m_pCompressionWriter->PushData((void*)&materialId, sizeof(uint16));
        }
    }

    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        const GeomCache::Mesh& mesh = **iter;
        const GeomCacheFile::EStreams mandatoryStreams = GeomCacheFile::EStreams(GeomCacheFile::eStream_Indices | GeomCacheFile::eStream_Positions
                | GeomCacheFile::eStream_Texcoords | GeomCacheFile::eStream_QTangents);
        assert (((mesh.m_constantStreams | mesh.m_animatedStreams) & mandatoryStreams) == mandatoryStreams);
        WriteMeshStaticData(mesh, mesh.m_constantStreams);
    }
}

void GeomCacheWriter::WriteNodeStaticDataRec(const GeomCache::Node& node, const std::vector<GeomCache::Mesh*>& meshes)
{
    GeomCacheFile::SNodeInfo fileNode;
    fileNode.m_type = static_cast<uint8>(node.m_type);
    fileNode.m_transformType = static_cast<uint16>(node.m_transformType);
    fileNode.m_bVisible = node.m_staticNodeData.m_bVisible ? 1 : 0;
    fileNode.m_meshIndex = std::numeric_limits<uint32>::max();

    if (node.m_type == GeomCacheFile::eNodeType_Mesh)
    {
        auto findIter = std::find(meshes.begin(), meshes.end(), node.m_pMesh.get());
        if (findIter != meshes.end())
        {
            fileNode.m_meshIndex = static_cast<uint32>(findIter - meshes.begin());
        }
    }

    fileNode.m_numChildren = static_cast<uint32>(node.m_children.size());

    const std::string& fullName = (node.m_abcObject != NULL) ? node.m_abcObject.getFullName() : "root";
    const uint nameLength = fullName.size() + 1;
    fileNode.m_nameLength = nameLength;

    // Write out node infos
    m_pCompressionWriter->PushData((void*)&fileNode, sizeof(GeomCacheFile::SNodeInfo));
    m_pCompressionWriter->PushData(fullName.c_str(), nameLength);

    // Store full initial pose. We could optimize this by leaving out animated branches without any physics proxies.
    STATIC_ASSERT(sizeof(QuatTNS) == 10 * sizeof(float), "QuatTNS size should be 40 bytes");
    m_pCompressionWriter->PushData((void*)&node.m_staticNodeData.m_transform, sizeof(QuatTNS));

    if (node.m_type == GeomCacheFile::eNodeType_PhysicsGeometry)
    {
        uint32 geometrySize = node.m_physicsGeometry.size();
        m_pCompressionWriter->PushData((void*)&geometrySize, sizeof(uint32));
        m_pCompressionWriter->PushData((void*)node.m_physicsGeometry.data(), node.m_physicsGeometry.size());
    }

    for (auto iter = node.m_children.begin(); iter != node.m_children.end(); ++iter)
    {
        WriteNodeStaticDataRec(**iter, meshes);
    }
}

void GeomCacheWriter::WriteFrame(const uint frameIndex, const AABB& frameAABB, const GeomCacheFile::EFrameType frameType,
    const std::vector<GeomCache::Mesh*>& meshes, GeomCache::Node& rootNode)
{
    m_animationAABB.Add(frameAABB);

    std::vector<uint32> meshOffsets;
    GeomCacheFile::SFrameHeader frameHeader;
    STATIC_ASSERT((sizeof(GeomCacheFile::SFrameHeader) % 16) == 0, "GeomCacheFile::SFrameHeader size must be a multiple of 16");

    m_frameTypes[frameIndex] = frameType;

    for (unsigned int i = 0; i < 3; ++i)
    {
        frameHeader.m_frameAABBMin[i] = frameAABB.min[i];
        frameHeader.m_frameAABBMax[i] = frameAABB.max[i];
    }

    GetFrameData(frameHeader, meshes);
    m_pCompressionWriter->PushData((void*)&frameHeader, sizeof(GeomCacheFile::SFrameHeader));

    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        GeomCache::Mesh* mesh = *iter;
        WriteMeshFrameData(*mesh);
    }

    uint32 bytesWritten = 0;
    WriteNodeFrameRec(rootNode, meshes, bytesWritten);

    // Pad node data to 16 bytes
    std::vector<uint8> paddingData(((bytesWritten + 15) & ~15) - bytesWritten, 0);
    if (paddingData.size())
    {
        m_pCompressionWriter->PushData(paddingData.data(), paddingData.size());
    }

    m_totalUncompressedAnimationSize += m_pCompressionWriter->GetCurrentDataSize();
    m_diskInfoForFrames[frameIndex] = m_pCompressionWriter->WriteBlock(true);

    const uint fraction = uint((float)m_kNumProgressStatusReports * ((float)(frameIndex + 1) / (float)m_fileHeader.m_numFrames));
    if (fraction > 0 && !m_bShowedStatus[fraction - 1])
    {
        const uint percent = (100 * fraction) / (m_kNumProgressStatusReports);
        RCLog("  %u%% processed", percent);
        m_bShowedStatus[fraction - 1] = true;
    }
}

void GeomCacheWriter::GetFrameData(GeomCacheFile::SFrameHeader& header, const std::vector<GeomCache::Mesh*>& meshes)
{
    uint32 dataOffset = 0;

    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        const GeomCache::Mesh& mesh = **iter;

        if (mesh.m_animatedStreams != 0)
        {
            AZStd::lock_guard<AZStd::mutex> jobLock(mesh.m_encodedFramesCS);
            dataOffset += mesh.m_encodedFrames.front().size();
        }
    }

    header.m_nodeDataOffset = dataOffset;
}

void GeomCacheWriter::WriteNodeFrameRec(GeomCache::Node& node, const std::vector<GeomCache::Mesh*>& meshes, uint32& bytesWritten)
{
    AZStd::lock_guard<AZStd::mutex> jobLock(node.m_encodedFramesCS);
    m_pCompressionWriter->PushData(node.m_encodedFrames.front().data(),
        node.m_encodedFrames.front().size());
    bytesWritten += node.m_encodedFrames.front().size();
    node.m_encodedFrames.pop_front();

    for (auto iter = node.m_children.begin(); iter != node.m_children.end(); ++iter)
    {
        WriteNodeFrameRec(**iter, meshes, bytesWritten);
    }
}

void GeomCacheWriter::WriteMeshFrameData(GeomCache::Mesh& mesh)
{
    if (mesh.m_animatedStreams != 0)
    {
        AZStd::lock_guard<AZStd::mutex> jobLock(mesh.m_encodedFramesCS);
        m_pCompressionWriter->PushData(mesh.m_encodedFrames.front().data(), mesh.m_encodedFrames.front().size());
        mesh.m_encodedFrames.pop_front();
    }
}

void GeomCacheWriter::WriteMeshStaticData(const GeomCache::Mesh& mesh, GeomCacheFile::EStreams streamMask)
{
    if (streamMask & GeomCacheFile::eStream_Indices)
    {
        for (auto iter = mesh.m_indicesMap.begin(); iter != mesh.m_indicesMap.end(); ++iter)
        {
            const std::vector<uint32>& indices = iter->second;
            const uint32 numIndices = indices.size();
            m_pCompressionWriter->PushData(&numIndices, sizeof(uint32));

            if ((m_fileHeader.m_flags & GeomCacheFile::eFileHeaderFlags_32BitIndices) != 0)
            {
                m_pCompressionWriter->PushData(indices.data(), sizeof(uint32) * numIndices);
            }
            else
            {
                std::vector<uint16> indices16Bit(indices.size());
                std::transform(indices.begin(), indices.end(), indices16Bit.begin(), [](uint32 index) { return static_cast<uint16>(index); });
                m_pCompressionWriter->PushData(indices16Bit.data(), sizeof(uint16) * numIndices);
            }
        }
    }

    bool bWroteStream = false;
    unsigned int numElements = 0;

    const GeomCache::MeshData& meshData = mesh.m_staticMeshData;

    if (streamMask & GeomCacheFile::eStream_Positions)
    {
        m_pCompressionWriter->PushData(meshData.m_positions.data(), sizeof(GeomCacheFile::Position) * meshData.m_positions.size());
        bWroteStream = true;
        numElements = meshData.m_positions.size();
    }

    if (streamMask & GeomCacheFile::eStream_Texcoords)
    {
        assert(!bWroteStream || meshData.m_texcoords.size() == numElements);
        m_pCompressionWriter->PushData(meshData.m_texcoords.data(), sizeof(GeomCacheFile::Texcoords) * meshData.m_texcoords.size());
        bWroteStream = true;
        numElements = meshData.m_texcoords.size();
    }

    if (streamMask & GeomCacheFile::eStream_QTangents)
    {
        assert(!bWroteStream || meshData.m_qTangents.size() == numElements);
        m_pCompressionWriter->PushData(meshData.m_qTangents.data(), sizeof(GeomCacheFile::QTangent) * meshData.m_qTangents.size());
        bWroteStream = true;
        numElements = meshData.m_qTangents.size();
    }

    if (streamMask & GeomCacheFile::eStream_Colors)
    {
        assert(!bWroteStream || meshData.m_reds.size() == numElements);
        assert(!bWroteStream || meshData.m_greens.size() == numElements);
        assert(!bWroteStream || meshData.m_blues.size() == numElements);
        assert(!bWroteStream || meshData.m_alphas.size() == numElements);
        m_pCompressionWriter->PushData(meshData.m_reds.data(), sizeof(GeomCacheFile::Color) * meshData.m_reds.size());
        m_pCompressionWriter->PushData(meshData.m_greens.data(), sizeof(GeomCacheFile::Color) * meshData.m_greens.size());
        m_pCompressionWriter->PushData(meshData.m_blues.data(), sizeof(GeomCacheFile::Color) * meshData.m_blues.size());
        m_pCompressionWriter->PushData(meshData.m_alphas.data(), sizeof(GeomCacheFile::Color) * meshData.m_alphas.size());
        bWroteStream = true;
        numElements = meshData.m_reds.size();
    }

    if (mesh.m_bUsePredictor)
    {
        const uint32 predictorDataSize = mesh.m_predictorData.size();
        m_pCompressionWriter->PushData(&predictorDataSize, sizeof(uint32));
        m_pCompressionWriter->PushData(mesh.m_predictorData.data(), sizeof(uint16) * mesh.m_predictorData.size());
    }
}
