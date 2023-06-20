/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <Compression/CompressionInterfaceAPI.h>

namespace CompressionLZ4
{
    class CompressorLZ4
        : public Compression::ICompressionInterface
    {
    public:
        CompressorLZ4();
        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        Compression::CompressionAlgorithmId GetCompressionAlgorithmId() const override;
        //! Retrieves the human readable associated with the LZ4 compressor
        AZStd::string_view GetCompressionAlgorithmName() const override;
        //! Compresses the uncompressed data into the compressed buffer
        //! @return a CompressionResultData instance to indicate if compression operation has succeeded
        [[nodiscard]] Compression::CompressionResultData CompressBlock(
            AZStd::span<AZStd::byte> compressionBuffer, const AZStd::span<const AZStd::byte>& uncompressedData,
            const Compression::CompressionOptions& compressionOptions = {}) const override;

        [[nodiscard]] size_t CompressBound([[maybe_unused]] size_t uncompressedBufferSize) const override;
    };
} // namespace CompressionLZ4
