/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Compression/CompressionInterfaceStructs.h>

namespace Compression
{
    enum class DecompressionResult : AZ::u8
    {
        PendingStart,
        Started,
        Complete,
        Failed,
    };

    struct DecompressionResultData
    {
        //! Returns a boolean true if decompression has succeeded
        explicit constexpr operator bool() const;

        //! Retrieves the uncompressed size of the decompression data
        //! from the span
        AZ::u64 GetUncompressedByteCount() const;

        //! Retrieves the memory address where the uncompressed data
        //! is stored
        AZStd::byte* GetUncompressedByteData() const;

        //! Will be set to the memory address of the uncompressed buffer
        //! The size of the span will be set to actual uncompressed size
        AZStd::span<AZStd::byte> m_uncompressedBuffer;

        //! Stores result code of whether the operation succeeded
        DecompressionResult m_result{ DecompressionResult::PendingStart };
    };

    struct IDecompressionInterface
    {
        virtual ~IDecompressionInterface() = default;

        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        virtual CompressionAlgorithmId GetCompressionAlgorithmId() const = 0;
        //! Decompresses the input compressed data into the uncompressed buffer
        //! Both parameters are specified as spans which encapsulates the contiguous buffer and it's size.
        [[nodiscard]] virtual DecompressionResultData DecompressBlock(
            AZStd::span<AZStd::byte> uncompressedBuffer, const AZStd::span<const AZStd::byte>& compressedData) = 0;
    };

    class DecompressionFactoryInterface
    {
    public:
        AZ_RTTI(DecompressionFactoryInterface, "{DB1ACA55-B36F-469B-9704-EC486D9FC810}");
        virtual ~DecompressionFactoryInterface() = default;

        //! Returns a span containing all registered Decompression Interfaces
        //! @return view of registered Decompression Interfaces.
        virtual AZStd::span<IDecompressionInterface* const> GetDecompressionInterfaces() const= 0;

        //! Registers a decompression interface with the decompression factory.
        //! If a decompression interface with a CompressionAlgorithmId is registered that
        //! matches the input decompression interface, then registration does not occur
        //! and the unique_ptr refererence is unmodified and can be re-used by the caller
        //!
        //! @param decompressionInterface unique pointer to decompression interface to register
        //! @return true if the IDecompressionInterface was successfully registered, otherwise false
        virtual bool RegisterDecompressionInterface(AZStd::unique_ptr<IDecompressionInterface>&&) = 0;

        //! Unregisters the Decompression Interface with the specified Id if it is registered with the factory
        //!
        //! @param compressionAlgorithmId unique Id that identifies the Decompression Interface
        //! @return true if the IDecompressionInterface was unregistered, otherwise false
        virtual bool UnregisterCompressionInterface(CompressionAlgorithmId) = 0;

        //! Queries the Decompression Interface with the compression algorithmd Id
        //! @param compressionAlgorithmId unique Id of Decompression Interface to query
        //! @return pointer to the DeCompression Interface or nullptr if not found
        virtual IDecompressionInterface* FindDecompressionInterface(CompressionAlgorithmId) const = 0;
    };

    using DecompressionFactory = AZ::Interface<DecompressionFactoryInterface>;

} // namespace Compression

// Provides implemenation of the DecompressionResultData struct
#include "DecompressionInterfaceAPI.inl"
