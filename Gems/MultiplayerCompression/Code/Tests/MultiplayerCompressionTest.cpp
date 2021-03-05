/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <lz4.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <MultiplayerCompression/MultiplayerCompressionBus.h>
#include <LZ4Compressor.h>
#include <AzCore/Compression/Compression.h>
#include <AzTest/AzTest.h>

class MultiplayerCompressionTest
    : public UnitTest::AllocatorsTestFixture
{
protected:

    static const int MAX_BUFFER_SIZE = 512;

    void SetUp() override
    {
        AllocatorsTestFixture::SetUp();
    }

    void TearDown() override
    {
        AllocatorsTestFixture::TearDown();
    }
};

TEST_F(MultiplayerCompressionTest, MultiplayerCompression_CompressTest)
{
    // [SPEC-3870] Revisit this test once higher level layers are implemented to see if it can be repurposed
    /*
    GridMate::WriteBufferDynamic wb(AzNetworking::EndianType::IgnoreEndian);

    // Setup and marshal a highly compressable buffer for LZ4
    AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
    GridMate::Marshaler<int> marshaler;
    for (int i = 0; i < MAX_BUFFER_SIZE; ++i)
    {
        marshaler.Marshal(wb, 1);
    }
    const AZ::u64 marshalTime = (AZStd::chrono::system_clock::now() - startTime).count();

    size_t maxCompressedSize = wb.Size() + 32U;
    size_t compressedSize = -1;
    size_t uncompressedSize = -1;
    size_t consumedSize = -1;
    char* pCompressedBuffer = new char[maxCompressedSize];
    char* pDecompressedBuffer = new char[wb.Size()];

    //Run and test compress
    MultiplayerCompression::LZ4Compressor lz4Compressor;
    startTime = AZStd::chrono::system_clock::now();
    AzNetworking::CompressorError compressStatus = lz4Compressor.Compress(wb.Get(), wb.Size(), pCompressedBuffer, maxCompressedSize, compressedSize);
    const AZ::u64 compressTime = (AZStd::chrono::system_clock::now() - startTime).count();

    ASSERT_TRUE(compressStatus == AzNetworking::CompressorError::Ok);
    EXPECT_TRUE(compressedSize < maxCompressedSize);

    //Run and test decompress
    startTime = AZStd::chrono::system_clock::now();
    AzNetworking::CompressorError decompressStatus = lz4Compressor.Decompress(pCompressedBuffer, compressedSize, pDecompressedBuffer, wb.Size(), consumedSize, uncompressedSize);
    const AZ::u64 decompressTime = (AZStd::chrono::system_clock::now() - startTime).count();

    ASSERT_TRUE(decompressStatus == AzNetworking::CompressorError::Ok);
    EXPECT_TRUE(uncompressedSize = wb.Size());
    EXPECT_TRUE(memcmp(pDecompressedBuffer, wb.Get(), uncompressedSize) == 0);


    GridMate::ReadBuffer rb(GridMate::EndianType::IgnoreEndian, pDecompressedBuffer, wb.Size());

    //Calculate unmarshal time for data's sake
    startTime = AZStd::chrono::system_clock::now();
    for (int i = 0; i < MAX_BUFFER_SIZE; ++i)
    {
        int testVal1;
        marshaler.Unmarshal(testVal1, rb);
        EXPECT_TRUE(testVal1 == 1);
    }
    const AZ::u64 unmarshalTime = (AZStd::chrono::system_clock::now() - startTime).count();

    delete [] pCompressedBuffer;
    delete [] pDecompressedBuffer;

    //Expected [Profile]: Uncompressed Size: 2048 B Compressed Size: 21 B
    AZ_TracePrintf("Multiplayer Compression Test", "Uncompressed Size:(%llu B) Compressed Size:(%llu B) \n", uncompressedSize, compressedSize);
    //Expected [Profile]: Marshal Time : 84 mcs Unmarshal Time : 98 mcs Compress Time : 182 mcs Decompress Time : 7 mcs (times will vary with hardware)
    AZ_TracePrintf("Multiplayer Compression Test", "Marshal Time:(%llu mcs) Unmarshal Time:(%llu mcs) Compress Time:(%llu mcs) Decompress Time:(%llu mcs) \n", marshalTime, unmarshalTime, compressTime, decompressTime);
    */
}

#if AZ_TRAIT_DISABLE_FAILED_MULTIPLAYER_COMPRESSION_TESTS
TEST_F(MultiplayerCompressionTest, DISABLED_MultiplayerCompression_OversizeTest)
#else
TEST_F(MultiplayerCompressionTest, MultiplayerCompression_OversizeTest)
#endif
{
    size_t badInputSize = LZ4_MAX_INPUT_SIZE + 1;
    size_t bufferSize = 4;
    char* badInput = new char[badInputSize];
    char* pBuffer = new char[bufferSize];
    size_t compressedSize = 0;
    size_t consumedSize = 0;
    size_t uncompressedSize = 0;

    MultiplayerCompression::LZ4Compressor lz4Compressor;

    AzNetworking::CompressorError compressStatus = lz4Compressor.Compress(badInput, badInputSize, pBuffer, bufferSize, compressedSize);
    EXPECT_TRUE(compressStatus == AzNetworking::CompressorError::InsufficientBuffer);

    delete [] badInput;
    delete [] pBuffer;
}

#if AZ_TRAIT_DISABLE_FAILED_MULTIPLAYER_COMPRESSION_TESTS
TEST_F(MultiplayerCompressionTest, DISABLED_MultiplayerCompressionTest_UndersizeTest)
#else
TEST_F(MultiplayerCompressionTest, MultiplayerCompressionTest_UndersizeTest)
#endif
{
    size_t badInputSize = LZ4_MAX_INPUT_SIZE + 1;
    size_t bufferSize = 4;
    char* badInput = new char[badInputSize];
    char* pBuffer = new char[bufferSize];
    size_t compressedSize = 0;
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
