/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTIMacros.h>
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

    using CompressionResultString = AZStd::fixed_string<256>;

    //! Structure which can be used to supply custom options
    //! to the compression interface CompressBlock function
    //! The expected use is that the derived ICompressionInterface class
    //! implements the CompressionOptions struct and additional fields for specifying compression options
    //! When the derived compression interface classes in their CompressBlock function
    //! would use a call to AZ rtti cast to cast the struct to the correct type derived type
    //! `azrtti_cast<derived-compression-options*>(&compressionOptions);
    struct CompressionOptions
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(CompressionOptions);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        virtual ~CompressionOptions();
    };

    struct CompressionOutcome
    {
        explicit constexpr operator bool() const;
        //! Stores result code of whether the operation succeeded
        CompressionResult m_result{ CompressionResult::PendingStart };
        //! Stores any error messages associated with a failure result
        CompressionResultString m_resultString;
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

        //! Outcome containing result of the compression operation
        CompressionOutcome m_compressionOutcome;
    };

    struct ICompressionInterface
    {
        virtual ~ICompressionInterface() = default;

        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        virtual CompressionAlgorithmId GetCompressionAlgorithmId() const = 0;
        //! Human readable name associated with the compression algorithm
        virtual AZStd::string_view GetCompressionAlgorithmName() const = 0;

        //! Compresses the uncompressed data into the compressed buffer
        //! @param compressedBuffer destination buffer where compressed output will be stored to
        //! @param uncompressedData source buffer containing uncompressed content
        //! @param compressOptions that can be provided to the Compressor
        //! @return a CompressionResultData instance to indicate if compression operation has succeeded
        [[nodiscard]] virtual CompressionResultData CompressBlock(
            AZStd::span<AZStd::byte> compressionBuffer, const AZStd::span<const AZStd::byte>& uncompressedData,
            const CompressionOptions& compressionOptions = {}) const = 0;

        //! Returns the upper bound on compressed size given the uncompressed buffer size
        //! Can be used to allocate a destination buffer that can fit the compressed content
        //! @param uncompressedBufferSize size of uncompressed data
        //! @return worst case(upper bound) size that is needed to store compressed data for a given uncompressed size
        [[nodiscard]] virtual size_t CompressBound(size_t uncompressedBufferSize) const = 0;
    };

    class CompressionRegistrarInterface
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(CompressionRegistrarInterface);
        AZ_RTTI_NO_TYPE_INFO_DECL();
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

        //! Queries the compression interface with the compression algorithm Id
        //! @param compressionAlgorithmId unique Id of compression interface to query
        //! @return pointer to the compression interface or nullptr if not found
        [[nodiscard]] virtual ICompressionInterface* FindCompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const = 0;
        //! Queries the compression interface using the name of the compression algorithm
        //! This is slower than the using the compression algorithm ID.
        //! Furthermore the algorithm name doesn't have to be unique,
        //! so this will return the first compression interface associated with the algorithm name
        //! @param algorithmName Name of the compression algorithm.
        //! NOTE: The compression algorithm name is not checked for uniqueness, unlike the algorithm id
        //! @return pointer to the compression interface or nullptr if not found
        [[nodiscard]] virtual ICompressionInterface* FindCompressionInterface(AZStd::string_view algorithmName) const = 0;


        //! Return true if there is an compression interface registered with the specified id
        //! @param compressionAlgorithmId CompressionAlgorithmId to determine if an compression interface is registered
        //! @return bool indicating if there is an compression interface with the id registered
        [[nodiscard]] virtual bool IsRegistered(CompressionAlgorithmId compressionAlgorithmId) const = 0;
    };

    using CompressionRegistrar = AZ::Interface<CompressionRegistrarInterface>;

} // namespace Compression

// Provides implementations of the CompressionResultData struct
#include "CompressionInterfaceAPI.inl"
