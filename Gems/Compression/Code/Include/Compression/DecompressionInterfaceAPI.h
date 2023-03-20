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

    class DecompressionRegistrarInterface
    {
    public:
        AZ_RTTI(DecompressionRegistrarInterface, "{DB1ACA55-B36F-469B-9704-EC486D9FC810}");
        virtual ~DecompressionRegistrarInterface() = default;

        //! Callback function that is invoked for every registered decompression interface
        //! return true to indicate that visitation of decompression interfaces should continue
        //! returning false halts iteration
        using VisitDecompressionInterfaceCallback = AZStd::function<bool(IDecompressionInterface&)>;
        //! Invokes the supplied visitor for each register decompression interface that is non-nullptr
        virtual void VisitDecompressionInterfaces(const VisitDecompressionInterfaceCallback&) const = 0;


        //! Registers decompression interface and takes ownership of it if registration is successful
        //! @param compressionAlgorithmId Unique id to associate with decompression interface
        //! @param decompressionInterface decompression interface to register
        //! @return Success outcome if the decompression interface was successfully registered
        //! Otherwise, a failure outcome with the decompression interface is forward back to the caller
        virtual AZ::Outcome<void, AZStd::unique_ptr<IDecompressionInterface>> RegisterDecompressionInterface(
            CompressionAlgorithmId compressionAlgorithmId, AZStd::unique_ptr<IDecompressionInterface> decompressionInterface) = 0;


        //! Registers decompression interface, but does not take ownership of it
        //! If a decompression interface with a CompressionAlgorithmId is registered that
        //! matches the input decompression interface, then registration does not occur
        //!
        //! Registers decompression interface, but does not take ownership of it
        //! @param compressionAlgorithmId Unique id to associate with decompression interface
        //! @param decompressionInterface decompression interface to register
        //! @return true if the ICompressionInterface was successfully registered
        virtual bool RegisterDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId, IDecompressionInterface& decompressionInterface) = 0;

        //! Unregisters the decompression interface with the specified id
        //!
        //! @param decompressionAlgorithmId unique Id that identifies the decompression interface
        //! @return true if the unregistration is successful
        virtual bool UnregisterDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId) = 0;

        //! Queries the decompression interface with the decompression algorithmd Id
        //! @param compressionAlgorithmId unique Id of decompression interface to query
        //! @return pointer to the decompression tnterface or nullptr if not found
        [[nodiscard]] virtual IDecompressionInterface* FindDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const = 0;


        //! Return true if there is an decompression interface registered with the specified id
        //! @param compressionAlgorithmId CompressionAlgorithmId to determine if an decompression interface is registered
        //! @return bool indicating if there is an decompression interface with the id registered
        [[nodiscard]] virtual bool IsRegistered(CompressionAlgorithmId compressionAlgorithmId) const = 0;
    };

    using DecompressionRegistrar = AZ::Interface<DecompressionRegistrarInterface>;

} // namespace Compression

// Provides implemenation of the DecompressionResultData struct
#include "DecompressionInterfaceAPI.inl"
