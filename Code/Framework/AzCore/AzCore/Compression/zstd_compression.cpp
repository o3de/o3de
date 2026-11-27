/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZCORE_EXCLUDE_ZSTANDARD)

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Casting/lossy_cast.h>
#include <limits>

#include <AzCore/Compression/zstd_compression.h>

using namespace AZ;

ZStd::ZStd(IAllocator* workMemAllocator)
{
    m_workMemoryAllocator = workMemAllocator;
    if (!m_workMemoryAllocator)
    {
        m_workMemoryAllocator = &AllocatorInstance<SystemAllocator>::Get();
    }
    m_streamCompression = nullptr;
    m_streamDecompression = nullptr;
}

ZStd::~ZStd()
{
    if (m_streamCompression)
    {
        StopCompressor();
    }
    if (m_streamDecompression)
    {
        StopDecompressor();
    }
}

void* ZStd::AllocateMem(void* userData, size_t size)
{
    IAllocator* allocator = reinterpret_cast<IAllocator*>(userData);
    return allocator->Allocate(size, 4);
}

void ZStd::FreeMem(void* userData, void* address)
{
    IAllocator* allocator = reinterpret_cast<IAllocator*>(userData);
    allocator->DeAllocate(address);
}

void ZStd::StartCompressor(unsigned int compressionLevel)
{
    AZ_Assert(!m_streamCompression, "Compressor already started!");
    ZSTD_customMem customAlloc;
    customAlloc.customAlloc = reinterpret_cast<ZSTD_allocFunction>(&AllocateMem);
    customAlloc.customFree = &FreeMem;
    customAlloc.opaque = m_workMemoryAllocator;

    m_streamCompression = ZSTD_createCStream_advanced(customAlloc);
    AZ_Assert(m_streamCompression, "ZStandard internal error - failed to create compression stream\n");
    if (m_streamCompression)
    {
        [[maybe_unused]] size_t r = ZSTD_initCStream(m_streamCompression, static_cast<int>(compressionLevel));
        AZ_Assert(!ZSTD_isError(r), "ZStandard internal error - failed to initialize compression stream: %s", ZSTD_getErrorName(r));
    }
}

void ZStd::StopCompressor()
{
    AZ_Assert(m_streamCompression, "Compressor not started!");
    [[maybe_unused]] size_t result = ZSTD_freeCStream(m_streamCompression);
    AZ_Assert(!ZSTD_isError(result), "ZStandard internal error: %s", ZSTD_getErrorName(result));
    m_streamCompression = nullptr;
}

void ZStd::ResetCompressor()
{
    AZ_Assert(m_streamCompression, "Compressor not started!");
    // The change to ZSTD_Ctx_reset api occured first occured in ZStd version 1.3.8
    // https://github.com/facebook/zstd/commit/ff8d37170808931cdca10846da1c23aaf27084fb#diff-e51691a217ef645007ebc5dd6db0539c32d0a531aad5823873adb12d995341b5
#if ZSTD_VERSION_NUMBER >= 10308
    ZSTD_CCtx_reset(m_streamCompression, ZSTD_reset_session_only);
    [[maybe_unused]] size_t r = ZSTD_CCtx_setPledgedSrcSize(m_streamCompression, ZSTD_CONTENTSIZE_UNKNOWN);
#else
    [[maybe_unused]] size_t r = ZSTD_resetCStream(m_streamCompression,0);
#endif
    AZ_Assert(!ZSTD_isError(r), "Can't reset compressor");
}

unsigned int ZStd::Compress(const void* data, unsigned int& dataSize, void* compressedData, unsigned int compressedDataSize, FlushType flushType)
{
    AZ_Assert(m_streamCompression, "Compressor not started!");
    m_inBuffer.src = data;
    m_inBuffer.size = data ? dataSize : 0;
    m_inBuffer.pos = 0;

    m_outBuffer.dst = compressedData;
    m_outBuffer.size = compressedDataSize;
    m_outBuffer.pos = 0;

    size_t remaining = 0;
    if (flushType == FT_FINISH)
    {
        if (m_inBuffer.size > 0)
        {
            remaining = ZSTD_compressStream(m_streamCompression, &m_outBuffer, &m_inBuffer);
            if (!ZSTD_isError(remaining) && m_inBuffer.pos == m_inBuffer.size)
            {
                remaining = ZSTD_endStream(m_streamCompression, &m_outBuffer);
            }
        }
        else
        {
            remaining = ZSTD_endStream(m_streamCompression, &m_outBuffer);
        }
    }
    else if (flushType == FT_FULL_FLUSH || flushType == FT_SYNC_FLUSH || flushType == FT_PARTIAL_FLUSH)
    {
        if (m_inBuffer.size > 0)
        {
            remaining = ZSTD_compressStream(m_streamCompression, &m_outBuffer, &m_inBuffer);
            if (!ZSTD_isError(remaining) && m_inBuffer.pos == m_inBuffer.size)
            {
                remaining = ZSTD_flushStream(m_streamCompression, &m_outBuffer);
            }
        }
        else
        {
            remaining = ZSTD_flushStream(m_streamCompression, &m_outBuffer);
        }
    }
    else
    {
        remaining = ZSTD_compressStream(m_streamCompression, &m_outBuffer, &m_inBuffer);
    }

    if (ZSTD_isError(remaining))
    {
        AZ_Assert(false, "ZStd compression error: %s", ZSTD_getErrorName(remaining));
        dataSize = 0;
        return 0;
    }

    if (data != nullptr)
    {
        dataSize = azlossy_cast<unsigned int>(m_inBuffer.size - m_inBuffer.pos);
    }
    else
    {
        dataSize = azlossy_cast<unsigned int>(remaining);
    }

    return azlossy_cast<unsigned int>(m_outBuffer.pos);
}

unsigned int ZStd::GetMinCompressedBufferSize(unsigned int sourceDataSize)
{
    return azlossy_cast<unsigned int>(ZSTD_compressBound(sourceDataSize));
}

void ZStd::StartDecompressor()
{
    AZ_Assert(!m_streamDecompression, "Decompressor already started!");
    ZSTD_customMem customAlloc;
    customAlloc.customAlloc = reinterpret_cast<ZSTD_allocFunction>(&ZStd::AllocateMem);
    customAlloc.customFree = &ZStd::FreeMem;
    customAlloc.opaque = m_workMemoryAllocator;

    m_streamDecompression = ZSTD_createDStream_advanced(customAlloc);
    m_nextBlockSize = ZSTD_initDStream(m_streamDecompression);
    AZ_Assert(!ZSTD_isError(m_nextBlockSize), "ZStandard internal error: %s", ZSTD_getErrorName(m_nextBlockSize));

    //pointers will be set later....
    m_inBuffer.pos = 0;
    m_inBuffer.size = 0;
    m_outBuffer.size = 0;
    m_outBuffer.pos = 0;
    m_compressedBufferIndex = 0;
}

void ZStd::StopDecompressor()
{
    AZ_Assert(m_streamDecompression, "Decompressor not started!");
    size_t result = ZSTD_freeDStream(m_streamDecompression);
    AZ_Verify(!ZSTD_isError(result), "ZStandard internal error: %s", ZSTD_getErrorName(result));
    m_streamDecompression = nullptr;
}

void ZStd::ResetDecompressor(Header* header)
{
    AZ_UNUSED(header);
    AZ_Assert(m_streamDecompression, "Decompressor not started!");
#if ZSTD_VERSION_NUMBER >= 10308
    m_nextBlockSize = ZSTD_DCtx_reset(m_streamDecompression, ZSTD_reset_session_only);
#else
    m_nextBlockSize = ZSTD_resetDStream(m_streamDecompression);
#endif
    m_compressedBufferIndex = 0;
}

void ZStd::SetupDecompressHeader(Header header)
{
    AZ_UNUSED(header);
}

unsigned int ZStd::Decompress(const void* compressedData, unsigned int compressedDataSize, void* outputData, unsigned int& outputDataSize, size_t* sizeOfNextBlock)
{
    AZ_Assert(m_streamDecompression, "Decompressor not started!");
    m_inBuffer.src = compressedData;
    m_inBuffer.size = (m_nextBlockSize > 0 && m_nextBlockSize < compressedDataSize) ? m_nextBlockSize : compressedDataSize;
    m_inBuffer.pos = 0;

    m_outBuffer.dst = outputData;
    m_outBuffer.size = outputDataSize;
    m_outBuffer.pos = 0;

    m_nextBlockSize = ZSTD_decompressStream(m_streamDecompression, &m_outBuffer, &m_inBuffer);
    if (ZSTD_isError(m_nextBlockSize))
    {
        AZ_Assert(false, "ZStd streaming decompression error: %s", ZSTD_getErrorName(m_nextBlockSize));
        return std::numeric_limits<unsigned int>::max();
    }

    if (m_nextBlockSize == 0)
    {
        // The ZSTD_DCtx_reset function was updated in 1.3.8 to support the reset directive enum
        // https://github.com/facebook/zstd/commit/5c68639186e80469e9a426553a578864902133c4
#if ZSTD_VERSION_NUMBER >= 10308
        m_nextBlockSize = ZSTD_DCtx_reset(m_streamDecompression, ZSTD_reset_session_only);
#else
        m_nextBlockSize = ZSTD_resetDStream(m_streamDecompression);
#endif
    }

    outputDataSize = azlossy_cast<unsigned int>(m_outBuffer.size - m_outBuffer.pos);
    m_compressedBufferIndex += azlossy_cast<unsigned int>(m_inBuffer.pos);

    if (sizeOfNextBlock)
    {
        *sizeOfNextBlock = m_nextBlockSize;
    }

    return azlossy_cast<unsigned int>(m_inBuffer.pos);
}

bool ZStd::IsCompressorStarted() const
{
    return m_streamCompression != nullptr;
}

bool ZStd::IsDecompressorStarted() const
{
    return m_streamDecompression != nullptr;
}

//////////////////////////////////////////////////////////////////////////

#endif // #if !defined(AZCORE_EXCLUDE_ZSTANDARD)
