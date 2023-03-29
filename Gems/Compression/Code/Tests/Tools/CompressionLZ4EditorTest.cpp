/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/UnitTest/TestTypes.h>

#include <Compression/CompressionLZ4API.h>
#include <Tools/CompressorLZ4Impl.h>

namespace CompressionLZ4Test
{
    class CompressionLZ4Fixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        CompressionLZ4Fixture() = default;

        ~CompressionLZ4Fixture() = default;

    protected:
    };

    TEST_F(CompressionLZ4Fixture, LZ4Compressor_CompressBlock_Succeeds)
    {
        auto compressionAlgorithmId = CompressionLZ4::GetLZ4CompressionAlgorithmId();
        auto compressorLz4 = AZStd::make_unique<CompressionLZ4::CompressorLZ4>();

        EXPECT_EQ(compressionAlgorithmId, compressorLz4->GetCompressionAlgorithmId());

        constexpr AZStd::string_view dataToCompress = R"(Hello World)";
        size_t compressBufferUpperBound = compressorLz4->CompressBound(dataToCompress.size());
        EXPECT_GT(compressBufferUpperBound, 0);

        AZStd::vector<AZStd::byte> compressionBuffer;
        // Resize the compression output buffer to be able to fit the largest amount of compressed data
        // for the given dataToCompress size
        compressionBuffer.resize_no_construct(compressBufferUpperBound);

        AZStd::span uncompressedData(reinterpret_cast<const AZStd::byte*>(dataToCompress.data()), dataToCompress.size());

        Compression::CompressionResultData compressionResultData = compressorLz4->CompressBlock(
            compressionBuffer, uncompressedData);

        EXPECT_TRUE(static_cast<bool>(compressionResultData));
        EXPECT_TRUE(static_cast<bool>(compressionResultData.m_compressionOutcome));
        EXPECT_GT(compressionResultData.GetCompressedByteCount(), 0);
        EXPECT_NE(nullptr, compressionResultData.GetCompressedByteData());

        // Resize the compression buffer to the exact size
        compressionBuffer.resize_no_construct(compressionResultData.GetCompressedByteCount());
    }

    TEST_F(CompressionLZ4Fixture, LZ4Compressor_CompressBlock_WithBufferTooSmall_Fails)
    {
        auto compressionAlgorithmId = CompressionLZ4::GetLZ4CompressionAlgorithmId();
        auto compressorLz4 = AZStd::make_unique<CompressionLZ4::CompressorLZ4>();

        EXPECT_EQ(compressionAlgorithmId, compressorLz4->GetCompressionAlgorithmId());

        constexpr AZStd::string_view dataToCompress = R"(Hello World)";
        AZStd::span uncompressedData(reinterpret_cast<const AZStd::byte*>(dataToCompress.data()), dataToCompress.size());

        // The compression output buffer has a size of zero, so compression should fail
        AZStd::vector<AZStd::byte> compressionBuffer;

        Compression::CompressionResultData compressionResultData = compressorLz4->CompressBlock(
            compressionBuffer, uncompressedData);

        EXPECT_FALSE(static_cast<bool>(compressionResultData));
        EXPECT_FALSE(static_cast<bool>(compressionResultData.m_compressionOutcome));
        EXPECT_EQ(0, compressionResultData.GetCompressedByteCount());
        EXPECT_EQ(nullptr, compressionResultData.GetCompressedByteData());

    }
}
