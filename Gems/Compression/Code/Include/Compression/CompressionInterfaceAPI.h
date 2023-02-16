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
        PendingStart,
        Started,
        Complete,
        Failed,
    };

    struct CompressionResultData
    {
        //! Returns a boolean true if compression has completed
        explicit constexpr operator bool() const;

        //! Retrieves the compressed byte size
        AZ::u64 GetCompressedByteCount() const;

        //! Retrieves the memory address of the compressed data
        AZStd::byte* GetCompressedByteData() const;

        //! Will be set to the memory address of the compressed buffer
        //! supplied to the compression interface CompressBlock command
        //! The size of the span will be set to actual uncompressed size
        AZStd::span<AZStd::byte> m_compressedBuffer;

        //! Stores result code of whether the operation succeeded
        CompressionResult m_result{ CompressionResult::PendingStart };
    };

    struct ICompressionInterface
    {
        virtual ~ICompressionInterface() = default;

        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        virtual CompressionAlgorithmId GetCompressionAlgorithmId() const = 0;
        //! Compresses the uncompressed data into the compressed buffer
        //! @return a CompressionResultData instance to indicate if compression operation has succeeded
        [[nodiscard]] virtual CompressionResultData CompressBlock(
            AZStd::span<AZStd::byte> compressedBuffer, const AZStd::span<const AZStd::byte>& uncompressedData) = 0;
    };

    class CompressionRegistrarInterface
    {
    public:
        AZ_RTTI(CompressionRegistrarInterface, "{92251FE8-9D19-4A23-9A2B-F91D99D9491B}");
        virtual ~CompressionRegistrarInterface() = default;

        //! Callback function that is invoked for every registered compression interface
        //! return true to indicate that visitation of compression interfaces should continue
        //! returning false halts iteration
        using VisitCompressionInterfaceCallback = AZStd::function<bool(ICompressionInterface&)>;
        //! Invokes the supplied visitor for each register compression interface that is non-nullptr
        virtual void VisitCompressionInterfaces(const VisitCompressionInterfaceCallback&) const = 0;

        //! Registers compression interface and takes ownership of it if registration is successful
        //! @param compressionAlgorithmId Unique id to associate with compression interface
        //! @param compressionInterface compression interface to register
        //! @return Success outcome if the compression interface was successfully registered
        //! Otherwise, a failure outcome with the compression interface is forward back to the caller
        virtual AZ::Outcome<void, AZStd::unique_ptr<ICompressionInterface>> RegisterCompressionInterface(
            CompressionAlgorithmId compressionAlgorithmId, AZStd::unique_ptr<ICompressionInterface> compressionInterface) = 0;


        //! Registers compression interface, but does not take ownership of it
        //! If a compression interface with a CompressionAlgorithmId is registered that
        //! matches the input compression interface, then registration does not occur
        //!
        //! Registers compression interface, but does not take ownership of it
        //! @param compressionAlgorithmId Unique id to associate with compression interface
        //! @param compressionInterface compression interface to register
        //! @return true if the ICompressionInterface was successfully registered
        virtual bool RegisterCompressionInterface(CompressionAlgorithmId compressionAlgorithmId, ICompressionInterface& compressionInterface) = 0;

        //! Unregisters the compression interface with the specified id
        //!
        //! @param compressionAlgorithmId unique Id that identifies the compression interface
        //! @return true if the unregistration is successful
        virtual bool UnregisterCompressionInterface(CompressionAlgorithmId compressionAlgorithmId) = 0;

        //! Queries the compression interface with the compression algorithmd Id
        //! @param compressionAlgorithmId unique Id of compression interface to query
        //! @return pointer to the compression interface or nullptr if not found
        [[nodiscard]] virtual ICompressionInterface* FindCompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const = 0;


        //! Return true if there is an compression interface registered with the specified id
        //! @param compressionAlgorithmId CompressionAlgorithmId to determine if an compression interface is registered
        //! @return bool indicating if there is an compression interface with the id registered
        [[nodiscard]] virtual bool IsRegistered(CompressionAlgorithmId compressionAlgorithmId) const = 0;
    };

    using CompressionRegistrar = AZ::Interface<CompressionRegistrarInterface>;

} // namespace Compression

// Provides implementations of the CompressionResultData struct
#include "CompressionInterfaceAPI.inl"
