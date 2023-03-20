/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformDef.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace AzNetworking
{
    //! Collection of compression related error codes
    enum class CompressorError
    {
        Ok,                   //!< No error, operation finished successfully
        InsufficientBuffer,   //!< Buffer size is insufficient for the operation to complete, increase the size and try again
        CorruptData,          //!< Malformed or hacked packet, potentially security issue
        Uninitialized         //!< Compressor or supplied buffers are uninitialized
    };

    //! Unique identifier of a given compressor
    AZ_TYPE_SAFE_INTEGRAL(CompressorType, uint32_t);

    //! @class ICompressor
    //! @brief Packet data compressor interface.
    //!
    //! ICompressor is an abstract compression interface meant for user provided GEMs to implement (such as the [Multiplayer
    //! Compression Gem](http://o3de.org/docs/user-guide/gems/reference/multiplayer/multiplayer-compression)).
    //! Compression is supported for both TCP and UDP connections.  Instantiation of a compressor is controlled by the
    //! `net_UdpCompressor` or `net_TcpCompressor` cvar for their respective protocols.  

    class ICompressor
    {
    public:

        virtual ~ICompressor() = default;

        //! Initialize compressor.
        virtual bool Init() = 0;

        //! Unique identifier of a given compressor.
        virtual CompressorType GetType() const = 0;

        //! Returns max possible size of uncompressed data chunk needed to fit compressed data in maxCompSize bytes.
        virtual AZStd::size_t GetMaxChunkSize(AZStd::size_t maxCompSize) const = 0;

        //! Returns size of compressed buffer needed to uncompress uncompSize of bytes.
        virtual AZStd::size_t GetMaxCompressedBufferSize(AZStd::size_t uncompSize) const = 0;

        //! Finalizes the stream, and returns composed packet.
        //! Chunk based compressors should loop internally in Compress() to compress all chunks of uncompData.
        //! @param uncompData   buffer to compress
        //! @param uncompSize   length of data to compress from uncompData
        //! @param compData     should be able to fit at least GetMaxCompressedBufferSize(uncompSize) bytes
        //! @param compDataSize size of compData buffer
        //! @param compSize     length of compressed data written into compData
        virtual CompressorError Compress
        (
            const void* uncompData,
            AZStd::size_t uncompSize,
            void* compData,
            AZStd::size_t compDataSize,
            AZStd::size_t& compSize
        ) = 0;

        //! Decompress packet.
        //! Chunk based decompressors should loop internally in Decompress() to decompress all chunks of compData.
        //! @param compData       buffer to decompress
        //! @param compSize       length of data to decompress from compData
        //! @param uncompData     should be able to fit at least GetDecompressedBufferSize(compressedDataSize)
        //! @param uncompDataSize size of uncompData buffer.
        //! @param consumedSize   the number of bytes processed out of compData. (previously named chunkSize)
        //! @param uncompSize     length of decompressed data written into uncompData
        virtual CompressorError Decompress
        (
            const void* compData,
            AZStd::size_t compDataSize,
            void* uncompData,
            AZStd::size_t uncompDataSize,
            AZStd::size_t& consumedSize,
            AZStd::size_t& uncompSize
        ) = 0;
    };

    //! @class ICompressorFactory
    //! @brief Abstract factory to instantiate compressors.
    //!
    //! ICompressorFactory is an abstract compression interface meant for user provided GEMs to implement. ICompressorFactory
    //! implementations can be registered to classes implementing INetworking. Registered factories can then be used to create
    //! ICompressor implementations on demand. The [Multiplayer Compression
    //! Gem](http://o3de.org/docs/user-guide/gems/reference/multiplayer/multiplayer-compression) is an example of an ICompressorFactory
    //! for an LZ4 Compressor. In it, MultiplayerCompressionSystemComponent registers its ICompressorFactory with
    //! NetworkingSystemComponent, which is an implementation of INetworking. Registered factories are keyed by an AZ::Crc32
    //! of their string name accessed through the factory's GetFactoryName method.
    class ICompressorFactory
    {
    public:
        virtual ~ICompressorFactory() = default;

        //! Instantiate a new compressor
        //! @return A unique_ptr to a new Compressor
        virtual AZStd::unique_ptr<ICompressor> Create() = 0;

        //! Gets the string name of this compressor factory
        //! @return the string name of this compressor factory
        virtual const AZStd::string_view GetFactoryName() const = 0;
    };
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AzNetworking::CompressorType);
