/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <lz4.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <LZ4Compressor.h>

#include <AzCore/Compression/Compression.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzTest/AzTest.h>

class MultiplayerCompressionTest
    : public UnitTest::LeakDetectionFixture
{
};

TEST_F(MultiplayerCompressionTest, MultiplayerCompression_CompressTest)
{
    AzNetworking::UdpPacketEncodingBuffer buffer;
    buffer.Resize(buffer.GetCapacity());

    // Setup and marshal a highly compressable buffer for LZ4
    AZStd::chrono::steady_clock::time_point startTime = AZStd::chrono::steady_clock::now();
    memset(buffer.GetBuffer(), 255, buffer.GetCapacity());

    size_t maxCompressedSize = buffer.GetSize() + 32U;
    size_t compressedSize = std::numeric_limits<size_t>::max();
    size_t uncompressedSize = std::numeric_limits<size_t>::max();
    size_t consumedSize = std::numeric_limits<size_t>::max();
    char* pCompressedBuffer = new char[maxCompressedSize];
    char* pDecompressedBuffer = new char[buffer.GetSize()];

    //Run and test compress
    MultiplayerCompression::LZ4Compressor lz4Compressor;
    startTime = AZStd::chrono::steady_clock::now();
    AzNetworking::CompressorError compressStatus = lz4Compressor.Compress(buffer.GetBuffer(), buffer.GetSize(), pCompressedBuffer, maxCompressedSize, compressedSize);
    [[maybe_unused]] const AZ::u64 compressTime = (AZStd::chrono::steady_clock::now() - startTime).count();

    ASSERT_TRUE(compressStatus == AzNetworking::CompressorError::Ok);
    EXPECT_TRUE(compressedSize < maxCompressedSize);

    //Run and test decompress
    startTime = AZStd::chrono::steady_clock::now();
    AzNetworking::CompressorError decompressStatus = lz4Compressor.Decompress(pCompressedBuffer, compressedSize, pDecompressedBuffer, buffer.GetSize(), consumedSize, uncompressedSize);
    [[maybe_unused]] const AZ::u64 decompressTime = (AZStd::chrono::steady_clock::now() - startTime).count();

    ASSERT_TRUE(decompressStatus == AzNetworking::CompressorError::Ok);
    EXPECT_TRUE(uncompressedSize = buffer.GetSize());
    EXPECT_TRUE(memcmp(pDecompressedBuffer, buffer.GetBuffer(), uncompressedSize) == 0);

    delete [] pCompressedBuffer;
    delete [] pDecompressedBuffer;

    //Expected [Profile]: Uncompressed Size: 2048 B Compressed Size: 21 B
    AZ_TracePrintf("Multiplayer Compression Test", "Uncompressed Size:(%llu B) Compressed Size:(%llu B) \n", uncompressedSize, compressedSize);
    //Expected [Profile]: Marshal Time : 84 mcs Unmarshal Time : 98 mcs Compress Time : 182 mcs Decompress Time : 7 mcs (times will vary with hardware)
    AZ_TracePrintf("Multiplayer Compression Test", "Compress Time:(%llu mcs) Decompress Time:(%llu mcs) \n", compressTime, decompressTime);
}

TEST_F(MultiplayerCompressionTest, MultiplayerCompression_OversizeTest)
{
    size_t badInputSize = LZ4_MAX_INPUT_SIZE + 1;
    size_t bufferSize = 4;
    char* badInput = new char[badInputSize];
    char* pBuffer = new char[bufferSize];
    size_t compressedSize = 0;

    MultiplayerCompression::LZ4Compressor lz4Compressor;

    AzNetworking::CompressorError compressStatus = lz4Compressor.Compress(badInput, badInputSize, pBuffer, bufferSize, compressedSize);
    EXPECT_TRUE(compressStatus == AzNetworking::CompressorError::InsufficientBuffer);

    delete [] badInput;
    delete [] pBuffer;
}

TEST_F(MultiplayerCompressionTest, MultiplayerCompressionTest_UndersizeTest)
{
    size_t badInputSize = LZ4_MAX_INPUT_SIZE + 1;
    size_t bufferSize = 4;
    char* badInput = new char[badInputSize];
    char* pBuffer = new char[bufferSize];
    size_t consumedSize = 0;
    size_t uncompressedSize = 0;

    MultiplayerCompression::LZ4Compressor lz4Compressor;

    AzNetworking::CompressorError decompressStatus = lz4Compressor.Decompress(badInput, badInputSize, pBuffer, bufferSize, consumedSize, uncompressedSize);
    EXPECT_TRUE(decompressStatus == AzNetworking::CompressorError::CorruptData);

    delete [] badInput;
    delete [] pBuffer;
}

TEST_F(MultiplayerCompressionTest, MultiplayerCompressionTest_NullTest)
{
    size_t compressedSize = 0;
    size_t consumedSize = 0;
    size_t uncompressedSize = 0;

    MultiplayerCompression::LZ4Compressor lz4Compressor;

    AzNetworking::CompressorError compressStatus = lz4Compressor.Compress(nullptr, 4, nullptr, 4, compressedSize);
    EXPECT_TRUE(compressStatus == AzNetworking::CompressorError::Uninitialized);

    AzNetworking::CompressorError decompressStatus = lz4Compressor.Decompress(nullptr, 4, nullptr, 4, consumedSize, uncompressedSize);
    EXPECT_TRUE(decompressStatus == AzNetworking::CompressorError::Uninitialized);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
