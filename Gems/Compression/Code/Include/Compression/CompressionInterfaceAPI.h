/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/span.h>
#include <Compression/CompressionInterfaceStructs.h>

namespace Compression
{
    enum class CompressionResult : u8
    {
        Started,
        Complete,
        NotStarted,
        Failed,
    };

    struct CompressionResultData
    {
        //! Returns a boolean true if decompression has completed
        explicit constexpr operator bool() const
        {
            return m_result == CompressionResult::Complete;
        }

        //! Retrieves the compressed data size
        AZ::u64 GetBytesCompressed() const
        {
            return m_compressedBuffer.size();
        }

        //! Retrieves the memory address of the compressed data
        AZStd::byte* GetAddress() const
        {
            return m_compressedBuffer.data();
        }

        //! Will be set to the memory address of the compressed buffer
        //! supplied to the compression interface CompressBlock command
        //! The size of the span will be set to actual uncompressed size
        AZStd::span<AZStd::byte> m_compressedBuffer;

        //! Stores result code of whether the operation succeeded
        CompressionResult m_result{ CompressionResult::NotStarted };
    };

    struct ICompressionInterface
    {
        virtual ~ICompressionInterface() = default;

        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        virtual CompressionAlgorithmId GetCompressionAlgorithmId() const = 0;
        //! Compresses the input uncompressed data into the compressed buffer supplied as both spans
        //! @return a CompressionResultData instance to indicate if compression operation has succeeded
        virtual CompressionResultData CompressBlock(
            AZStd::span<AZStd::byte> compressedBuffer, const AZStd::span<const AZStd::byte>& uncompressedData) = 0;
    };

    class CompressionFactoryInterface
    {
    public:
        AZ_RTTI(CompressionFactoryInterface, "{92251FE8-9D19-4A23-9A2B-F91D99D9491B}");
        virtual ~CompressionFactoryInterface() = default;

        virtual void GetRegisteredCompressionInterfaces(AZStd::span<ICompressionInterface* const>) = 0;
        virtual bool RegisterCompressionInterface(AZStd::unique_ptr<ICompressionInterface>) = 0;
        virtual ICompressionInterface* FindCompressionInterface(CompressionAlgorithmId) const = 0;
    };

    using CompressionFactory = AZ::Interface<CompressionFactoryInterface>;

} // namespace Compression
