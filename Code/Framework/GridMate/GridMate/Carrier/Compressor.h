/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_COMPRESSOR_INTERFACE_H
#define GM_COMPRESSOR_INTERFACE_H

#include <AzCore/PlatformDef.h>
#include <AzCore/std/functional.h>

namespace GridMate
{
    /**
    * Collection of compression related error codes
    */
    enum class CompressorError
    {
        Ok,                   ///< No error, operation finished successfully
        InsufficientBuffer,   ///< Buffer size is insufficient for the operation to complete, increase the size and try again
        CorruptData           ///< Malformed or hacked packet, potentially security issue
    };

    /*
    * Unique identifier of a given compressor
    */
    using CompressorType = AZ::u32;

    /**
     * Packet data compressor interface
     */
    class Compressor
    {
    public:
        virtual ~Compressor() = default;

        /*
        * Initialize compressor
        */
        virtual bool Init() = 0;

        /*
        * Unique identifier of a given compressor
        */
        virtual CompressorType GetType() const = 0;

        /*
        * Returns max possible size of uncompressed data chunk needed to fit compressed data in maxCompSize bytes
        */
        virtual size_t GetMaxChunkSize(size_t maxCompSize) const = 0;

        /*
        * Returns size of compressed buffer needed to uncompress uncompSize of bytes
        */
        virtual size_t GetMaxCompressedBufferSize(size_t uncompSize) const = 0;

        /*
        * Finalizes the stream, and returns composed packet
        * \param uncompData - buffer to compress
        * \param uncompSize - length of data to compress from uncompData
        * \param compData - should be able to fit at least GetMaxCompressedBufferSize(uncompSize) bytes
        * \param compDataSize - size of compData buffer
        * \param compSize - length of compressed data written into compData
        *
        * Chunk based compressors should loop internally in Compress() to compress all chunks of uncompData.
        */
        virtual CompressorError Compress(const void* uncompData, size_t uncompSize, void* compData, size_t compDataSize, size_t& compSize) = 0;

        /*
        * Decompress packet
        * \param compData - buffer to decompress
        * \param compSize - length of data to deccompress from compData
        * \param uncompData - should be able to fit at least GetDecompressedBufferSize(compressedDataSize)
        * \param uncompDataSize - size of uncompData buffer.
        * \param consumedSize - the number of bytes processed out of compData. (previously named chunkSize)
        * \param uncompSize - length of decompressed data written into uncompData
        *
        * Chunk based decompressors should loop internally in Decompress() to decompress all chunks of compData.
        */
        virtual CompressorError Decompress(const void* compData, size_t compDataSize, void* uncompData, size_t uncompDataSize, size_t& consumedSize, size_t& uncompSize) = 0;
    };

    /**
    * Abstract factory to instantiate compressors
    * Used by carrier to create compressor
    */
    class CompressionFactory
    {
    public:
        virtual ~CompressionFactory() = default;

        /*
        * Instantiate new compressor
        */
        virtual AZStd::shared_ptr<Compressor> CreateCompressor() = 0;
    };
}

#endif // GM_COMPRESSOR_INTERFACE_H

