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
    enum class CompressionResult : AZ::u8
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
        [[nodiscard]] virtual CompressionResultData CompressBlock(
            AZStd::span<AZStd::byte> compressedBuffer, const AZStd::span<const AZStd::byte>& uncompressedData) = 0;
    };

    class CompressionFactoryInterface
    {
    public:
        AZ_RTTI(CompressionFactoryInterface, "{92251FE8-9D19-4A23-9A2B-F91D99D9491B}");
        virtual ~CompressionFactoryInterface() = default;

        //! Returns a span containing all registered Compression Interfaces
        //! @return span off registered Compression Interfaces.
        virtual AZStd::span<ICompressionInterface* const> GetCompressionInterfaces() = 0;

        //! Registers a compression interface with the compression factory.
        //! If a compression interface with a CompressionAlgorithmId is registered that
        //! matches the input compression interface, then registration does not occur
        //! and the unique_ptr refererence is unmodified and can be re-used by the caller
        //!
        //! @param compressionInterface unique pointer to compression interface to register
        //! @return true if the ICompressionInterface was successfully registered, otherwise false
        virtual bool RegisterCompressionInterface(AZStd::unique_ptr<ICompressionInterface>&&) = 0;

        //! Unregisters the Compression Interface with the specified Id if it is registered with the factory
        //!
        //! @param compressionAlgorithmId unique Id that identifies the Compression Interface
        //! @return true if the ICompressionInterface was unregistered, otherwise false
        virtual bool UnregisterCompressionInterface(CompressionAlgorithmId) = 0;

        //! Queries the Compression Interface with the compression algorithmd Id
        //! @param compressionAlgorithmId unique Id of Compression Interface to query
        //! @return pointer to the Compression Interface or nullptr if not found
        virtual ICompressionInterface* FindCompressionInterface(CompressionAlgorithmId) const = 0;
    };

    using CompressionFactory = AZ::Interface<CompressionFactoryInterface>;

} // namespace Compression
