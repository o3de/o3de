/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressorLZ4Impl.h"

#include <Compression/CompressionLZ4API.h>

#include <lz4.h>

namespace CompressionLZ4
{
    // Definitions for LZ4 Compressor
    CompressorLZ4::CompressorLZ4() = default;

    Compression::CompressionAlgorithmId CompressorLZ4::GetCompressionAlgorithmId() const
    {
        return GetLZ4CompressionAlgorithmId();
    }

    AZStd::string_view CompressorLZ4::GetCompressionAlgorithmName() const
    {
        return GetLZ4CompressionAlgorithmName();
    }

    [[nodiscard]] size_t CompressorLZ4::CompressBound([[maybe_unused]] size_t uncompressedBufferSize) const
    {
        return LZ4_compressBound(static_cast<int>(uncompressedBufferSize));
    }

    Compression::CompressionResultData CompressorLZ4::CompressBlock(
        AZStd::span<AZStd::byte> compressionBuffer, const AZStd::span<const AZStd::byte>& uncompressedData,
        [[maybe_unused]] const Compression::CompressionOptions& compressionOptions) const
    {
        Compression::CompressionResultData resultData;

        if (const int worstCaseCompressedSize = LZ4_compressBound(static_cast<int>(uncompressedData.size()));
            worstCaseCompressedSize == 0)
        {
            resultData.m_compressionOutcome.m_resultString = Compression::CompressionResultString::format(
                "Input buffer is too large to compress"
                " in a single call. The maximum lz4 input size is %d. The input size is %zu", LZ4_MAX_INPUT_SIZE, uncompressedData.size());
            resultData.m_compressionOutcome.m_result = Compression::CompressionResult::Failed;
            return resultData;
        }
        else if (compressionBuffer.size() < worstCaseCompressedSize)
        {
            resultData.m_compressionOutcome.m_resultString = Compression::CompressionResultString::format(
                "Output buffer capacity is less than the upper bound for worst case."
                " Worst case size is %d; output buffer capacity is %zu\n",
                worstCaseCompressedSize, compressionBuffer.size());
        }

        // Note that this returns a non-negative int so we are narrowing into a size_t here
        int compressedSize = LZ4_compress_default(
            reinterpret_cast<const char*>(uncompressedData.data()),
            reinterpret_cast<char*>(compressionBuffer.data()),
            static_cast<int>(uncompressedData.size()),
            static_cast<int>(compressionBuffer.size()));

        if (compressedSize == 0)
        {
            // LZ4_compress_HC returns a zero value for corrupt data and insufficient buffer
            resultData.m_compressionOutcome.m_resultString += Compression::CompressionResultString::format(
                "lz4_compress_default call has failed. The source buffer size is %zu and the output buffer"
                " has capacity of %zu", uncompressedData.size(), compressionBuffer.size());
            resultData.m_compressionOutcome.m_result = Compression::CompressionResult::Failed;
            return resultData;
        }

        // Update the result buffer span to point at the beginning of the compressed data and
        // the correct compressed size
        resultData.m_compressedBuffer = compressionBuffer.subspan(0, compressedSize);
        resultData.m_compressionOutcome.m_result = Compression::CompressionResult::Complete;
        return resultData;
    }

} // namespace CompressionLZ4
