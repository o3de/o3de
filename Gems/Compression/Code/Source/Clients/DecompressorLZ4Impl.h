/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <Compression/DecompressionInterfaceAPI.h>

namespace CompressionLZ4
{
    class DecompressorLZ4
        : public Compression::IDecompressionInterface
    {
    public:
        DecompressorLZ4();
        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        Compression::CompressionAlgorithmId GetCompressionAlgorithmId() const override;
        //! Retrieves the human readable associated with the LZ4 compressor
        AZStd::string_view GetCompressionAlgorithmName() const override;
        //! Compresses the uncompressed data into the compressed buffer
        //! @return a CompressionResultData instance to indicate if compression operation has succeeded
        [[nodiscard]] Compression::DecompressionResultData DecompressBlock(
            AZStd::span<AZStd::byte> decompressionBuffer, const AZStd::span<const AZStd::byte>& compressedData,
            const Compression::DecompressionOptions& decompressionOptions = {}) const override;
    };
} // namespace CompressionLZ4
