/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

#include <Compression/CompressionLZ4API.h>
#include <Clients/DecompressorLZ4Impl.h>

namespace CompressionLZ4Test
{
    class DecompressionLZ4Fixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        DecompressionLZ4Fixture() = default;

        ~DecompressionLZ4Fixture() = default;

    protected:
    };

    TEST_F(DecompressionLZ4Fixture, LZ4Decompressor_DecompressBlock_Succeeds)
    {
        auto compressionAlgorithmId = CompressionLZ4::GetLZ4CompressionAlgorithmId();
        auto decompressorLz4 = AZStd::make_unique<CompressionLZ4::DecompressorLZ4>();

        EXPECT_EQ(compressionAlgorithmId, decompressorLz4->GetCompressionAlgorithmId());

        // The compressed data for Hello World has a 0xb0 before it
        constexpr AZStd::string_view dataToUncompress = "\xb0" R"(Hello World)";

        AZStd::vector<AZStd::byte> decompressionBuffer;
        // Use a buffer size of at least 10x to fit the decompressed data
        constexpr size_t DecompressBufferSize = dataToUncompress.size() * 10;
        // Resize the decompression output buffer to store the decompressed content
        decompressionBuffer.resize_no_construct(DecompressBufferSize);

        AZStd::span compressedData(reinterpret_cast<const AZStd::byte*>(dataToUncompress.data()), dataToUncompress.size());

        Compression::DecompressionResultData decompressionResultData = decompressorLz4->DecompressBlock(
            decompressionBuffer, compressedData);

        EXPECT_TRUE(static_cast<bool>(decompressionResultData));
        EXPECT_TRUE(static_cast<bool>(decompressionResultData.m_decompressionOutcome));
        EXPECT_GT(decompressionResultData.GetUncompressedByteCount(), 0);
        EXPECT_NE(nullptr, decompressionResultData.GetUncompressedByteData());

        // Resize the decompression buffer to the exact size
        decompressionBuffer.resize_no_construct(decompressionResultData.GetUncompressedByteCount());
        AZStd::string_view uncompressedString(reinterpret_cast<char*>(decompressionResultData.GetUncompressedByteData()),
            decompressionResultData.GetUncompressedByteCount());

        EXPECT_EQ("Hello World", uncompressedString);
    }

    TEST_F(DecompressionLZ4Fixture, LZ4Compressor_CompressBlock_WithBufferTooSmall_Fails)
    {
        auto compressionAlgorithmId = CompressionLZ4::GetLZ4CompressionAlgorithmId();
        auto decompressorLz4 = AZStd::make_unique<CompressionLZ4::DecompressorLZ4>();

        EXPECT_EQ(compressionAlgorithmId, decompressorLz4->GetCompressionAlgorithmId());

        constexpr AZStd::string_view dataToUncompress = "\xb0" R"(Hello World)";
        AZStd::span compressedData(reinterpret_cast<const AZStd::byte*>(dataToUncompress.data()), dataToUncompress.size());

        // The decompression output buffer has a size of zero, so decompression should fail
        AZStd::vector<AZStd::byte> decompressionBuffer;

        Compression::DecompressionResultData decompressionResultData = decompressorLz4->DecompressBlock(
            decompressionBuffer, compressedData);

        EXPECT_FALSE(static_cast<bool>(decompressionResultData));
        EXPECT_FALSE(static_cast<bool>(decompressionResultData.m_decompressionOutcome));
        EXPECT_EQ(0, decompressionResultData.GetUncompressedByteCount());
        EXPECT_EQ(nullptr, decompressionResultData.GetUncompressedByteData());
    }
}
