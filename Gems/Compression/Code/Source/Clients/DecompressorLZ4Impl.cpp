/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DecompressorLZ4Impl.h"

#include <Compression/CompressionLZ4API.h>

#include <lz4.h>

namespace CompressionLZ4
{
    // Definitions for LZ4 Compressor
    DecompressorLZ4::DecompressorLZ4() = default;

    Compression::CompressionAlgorithmId DecompressorLZ4::GetCompressionAlgorithmId() const
    {
        return GetLZ4CompressionAlgorithmId();
    }

    AZStd::string_view DecompressorLZ4::GetCompressionAlgorithmName() const
    {
        return GetLZ4CompressionAlgorithmName();
    }

    Compression::DecompressionResultData DecompressorLZ4::DecompressBlock(
        AZStd::span<AZStd::byte> decompressionBuffer, const AZStd::span<const AZStd::byte>& compressedData,
        [[maybe_unused]] const Compression::DecompressionOptions& decompressionOptions) const
    {
        Compression::DecompressionResultData resultData;

        if (decompressionBuffer.empty())
        {
            resultData.m_decompressionOutcome.m_resultString = Compression::DecompressionResultString(
                "Decompression buffer is empty, uncompressed content cannot be stored in it\n");
            // Do not return, but hold on to result string in case an error occurs in decompression
        }


        // Note that this returns a non-negative int so we are narrowing into a size_t here
        int decompressedSize = LZ4_decompress_safe(
            reinterpret_cast<const char*>(compressedData.data()),
            reinterpret_cast<char*>(decompressionBuffer.data()),
            static_cast<int>(compressedData.size()),
            static_cast<int>(decompressionBuffer.size()));

        if (decompressedSize < 0)
        {
            // LZ4_compress_HC returns a zero value for corrupt data and insufficient buffer
            resultData.m_decompressionOutcome.m_resultString += Compression::DecompressionResultString::format(
                "LZ4_decompress_safe call has failed. Either the decompression buffer cannot fit all decompressed content "
                "or the source stream is malformed. Dest buffer capacity: %zu, source stream size: %zu",
                decompressionBuffer.size(), compressedData.size());
            resultData.m_decompressionOutcome.m_result = Compression::DecompressionResult::Failed;
            return resultData;
        }

        // Update the result buffer span to point at the beginning of the compressed data and
        // the correct compressed size
        resultData.m_uncompressedBuffer = decompressionBuffer.subspan(0, decompressedSize);
        resultData.m_decompressionOutcome.m_result = Compression::DecompressionResult::Complete;
        return resultData;
    }

} // namespace CompressionLZ4
