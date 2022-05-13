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
    enum class DecompressionResult : u8
    {
        Started,
        Complete,
        NotStarted,
        Failed,
    };

    struct DecompressionResultData
    {
        //! Returns a boolean true if decompression has succeeded
        explicit constexpr operator bool() const
        {
            return m_result == DecompressionResult::Complete;
        }

        //! Retrieves the uncompressed size of the decompression data
        //! from the span
        AZ::u64 GetBytesUncompressed() const
        {
            return m_uncompressedBuffer.size();
        }

        //! Retrieves the memory address where the uncompressed data
        //! is stored
        AZStd::byte* GetAddress() const
        {
            return m_uncompressedBuffer.data();
        }

        //! Will be set to the memory address of the uncompressed buffer
        //! supplied to DecompressBlock
        //! The size of the span will be set to actual uncompressed size
        AZStd::span<AZStd::byte> m_uncompressedBuffer;

        //! Stores result code of whether the operation succeeded
        DecompressionResult m_result{ DecompressionResult::NotStarted };
    };

    struct IDecompressionInterface
    {
        virtual ~IDecompressionInterface() = default;

        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        virtual CompressionAlgorithmId GetCompressionAlgorithmId() const = 0;
        //! Decompresses the input compressed data into the uncompressed buffer
        //! Both parameters are specified as spans which encapsulates the contiguous buffer and it's size.
        virtual [[nodiscard]] DecompressionResultData DecompressBlock(
            AZStd::span<AZStd::byte> uncompressedBuffer, const AZStd::span<const AZStd::byte>& compressedData) = 0;
    };

    class DecompressionFactoryInterface
    {
    public:
        AZ_RTTI(DecompressionFactoryInterface, "{DB1ACA55-B36F-469B-9704-EC486D9FC810}");
        virtual ~DecompressionFactoryInterface() = default;

        virtual void GetRegisteredDeompressionInterfaces(AZStd::span<IDecompressionInterface* const>) = 0;
        virtual bool RegisterDecompressionInterface(AZStd::unique_ptr<IDecompressionInterface>) = 0;
        virtual IDecompressionInterface* FindDecompressionInterface(CompressionAlgorithmId) const = 0;
    };

    using DecompressionFactory = AZ::Interface<DecompressionFactoryInterface>;

} // namespace Compression
