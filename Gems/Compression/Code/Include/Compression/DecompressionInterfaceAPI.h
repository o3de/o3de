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
    enum class DecompressionResult : AZ::u8
    {
        PendingStart,
        Started,
        Complete,
        Failed,
    };

    using DecompressionResultString = AZStd::fixed_string<512>;

    //! Structure which can be used to supply custom options
    //! to the decompression interface DeompressBlock function
    //! The expected use is that the derived IDeCompressionInterface class
    //! implements the Decompression struct and additional fields for specifying decompression options
    //! When the derived compressed interface classes in their DecompressBlock function
    //! would use a call to AZ rtti cast to cast the struct to the correct type derived type
    //! `azrtti_cast<derived-decompression-options*>(&decompressionOptions);
    struct DecompressionOptions
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(DecompressionOptions);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        virtual ~DecompressionOptions();
    };

    struct DecompressionOutcome
    {
        explicit constexpr operator bool() const;
        //! Stores result code of whether the operation succeeded
        DecompressionResult m_result{ DecompressionResult::PendingStart };
        //! Stores any error messages associated with a failure result
        DecompressionResultString m_resultString;
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

        //! Outcome containing result of the decompression operation
        DecompressionOutcome m_decompressionOutcome;
    };

    struct IDecompressionInterface
    {
        virtual ~IDecompressionInterface() = default;

        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        virtual CompressionAlgorithmId GetCompressionAlgorithmId() const = 0;
        //! Human readable name associated with the compression algorithm
        virtual AZStd::string_view GetCompressionAlgorithmName() const = 0;

        //! Decompresses the input compressed data into the uncompressed buffer
        //! Both parameters are specified as spans which encapsulates the contiguous buffer and it's size.
        //! @param uncompressedBuffer destination buffer where uncompress output will be stored
        //! @param compressedData source buffer containing compressed data to decompress
        //! @param decompressionOptions that can be provided to the Compressor
        //! @return a DecompressionResultData instance to indicate if compression operation has succeeded
        [[nodiscard]] virtual DecompressionResultData DecompressBlock(
            AZStd::span<AZStd::byte> decompressionBuffer, const AZStd::span<const AZStd::byte>& compressedData,
            const DecompressionOptions& decompressionOptions = {}) const = 0;
    };

    class DecompressionRegistrarInterface
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(DecompressionRegistrarInterface);
        AZ_RTTI_NO_TYPE_INFO_DECL();
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

        //! Queries the decompression interface with the decompression algorithm Id
        //! @param compressionAlgorithmId unique Id of decompression interface to query
        //! @return pointer to the decompression interface or nullptr if not found
        [[nodiscard]] virtual IDecompressionInterface* FindDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const = 0;
        //! Queries the decompression interface using the name of the compression algorithm
        //! This is slower than the using the compression algorithm ID.
        //! Furthermore the algorithm name doesn't have to be unique,
        //! so this will return the first compression interface associated with the algorithm name
        //! @param algorithmName Name of the compression algorithm.
        //! NOTE: The compression algorithm name is not checked for uniqueness, unlike the algorithm id
        //! @return pointer to the decompression interface or nullptr if not found
        [[nodiscard]] virtual IDecompressionInterface* FindDecompressionInterface(AZStd::string_view algorithmName) const = 0;


        //! Return true if there is an decompression interface registered with the specified id
        //! @param compressionAlgorithmId CompressionAlgorithmId to determine if an decompression interface is registered
        //! @return bool indicating if there is an decompression interface with the id registered
        [[nodiscard]] virtual bool IsRegistered(CompressionAlgorithmId compressionAlgorithmId) const = 0;
    };

    using DecompressionRegistrar = AZ::Interface<DecompressionRegistrarInterface>;

} // namespace Compression

// Provides implementation of the DecompressionResultData struct
#include "DecompressionInterfaceAPI.inl"
