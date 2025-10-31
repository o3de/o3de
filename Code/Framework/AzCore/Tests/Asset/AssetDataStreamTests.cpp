/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>
#include <Tests/Streamer/IStreamerMock.h>

class AssetDataStreamTest
    : public UnitTest::LeakDetectionFixture
{
public:
    void SetUp() override
    {
        using ::testing::_;
        using ::testing::NiceMock;
        using ::testing::Return;

        UnitTest::LeakDetectionFixture::SetUp();
        AZ::Interface<AZ::IO::IStreamer>::Register(&m_mockStreamer);

        // Reroute enough mock streamer calls to this class to let us validate the input parameters and mock
        // out a "functioning" read request.

        ON_CALL(m_mockStreamer, Read(_,::testing::An<IStreamerTypes::RequestMemoryAllocator&>(),_,_,_,_))
            .WillByDefault([this](
                AZStd::string_view relativePath,
                IStreamerTypes::RequestMemoryAllocator& allocator,
                size_t size,
                AZStd::chrono::microseconds deadline,
                IStreamerTypes::Priority priority,
                size_t offset)
                {
                    // Save off all the input parameters to the read request so that we can validate that they match expectations.
                    m_allocator = &allocator;
                    m_relativePath = relativePath;
                    m_size = size;
                    m_deadline = deadline;
                    m_priority = priority;
                    m_offset = offset;
                    return nullptr;
                });

        ON_CALL(m_mockStreamer, SetRequestCompleteCallback(_, _))
            .WillByDefault([this](FileRequestPtr& request, AZ::IO::IStreamer::OnCompleteCallback callback) -> FileRequestPtr&
                {
                    // Save off the callback just so that we can call it when the request is "done"
                    m_callback = callback;
                    return request;
                });

        ON_CALL(m_mockStreamer, QueueRequest(_))
            .WillByDefault([this](const FileRequestPtr& request)
                {
                    // As soon as the request is queued to run, consider it "done" and call the callback
                    if (m_callback)
                    {
                        FileRequestHandle handle(request);
                        m_callback(handle);
                    }
                });

        ON_CALL(m_mockStreamer, GetRequestStatus(_))
            .WillByDefault([this]([[maybe_unused]] FileRequestHandle request)
                {
                    // Return whatever request status has been set in this class
                    return m_requestStatus;
                });

        ON_CALL(m_mockStreamer, GetReadRequestResult(_, _, _, _))
            .WillByDefault([this](
                [[maybe_unused]] FileRequestHandle request,
                void*& buffer,
                AZ::u64& numBytesRead,
                [[maybe_unused]] IStreamerTypes::ClaimMemory claimMemory)
                {
                    auto result = m_allocator->Allocate(m_size, m_size, AZCORE_GLOBAL_NEW_ALIGNMENT);
                    m_buffer = static_cast<AZ::u8*>(result.m_address);
                    memset(m_buffer, m_expectedBufferChar, m_size);
                    numBytesRead = m_size;
                    buffer = m_buffer;
                    return true;
                });
    }

    void TearDown() override
    {
        AZ::Interface<AZ::IO::IStreamer>::Unregister(&m_mockStreamer);
        UnitTest::LeakDetectionFixture::TearDown();
    }


protected:
    ::testing::NiceMock<StreamerMock> m_mockStreamer;

    AZStd::string_view m_relativePath;
    size_t m_size{ 0 };
    AZStd::chrono::microseconds m_deadline{ 0 };
    IStreamerTypes::Priority m_priority{ IStreamerTypes::s_priorityLowest };
    size_t m_offset{ 0 };
    AZ::u8* m_buffer{ nullptr };
    IStreamerTypes::RequestStatus m_requestStatus{ IStreamerTypes::RequestStatus::Completed };
    AZ::IO::IStreamer::OnCompleteCallback m_callback{};
    IStreamerTypes::RequestMemoryAllocator* m_allocator{ nullptr };

    // Define some arbitrary numbers for validating buffer contents
    static inline constexpr AZ::u8 m_expectedBufferChar{ 0xFE };
    static inline constexpr AZ::u8 m_badBufferChar{ 0xFD };

};

TEST_F(AssetDataStreamTest, Init_CreateTrivialInstance_CreationSuccessful)
{
    AZ::Data::AssetDataStream assetDataStream;

    // AssetDataStream is a read-only stream
    EXPECT_TRUE(assetDataStream.CanRead());
    EXPECT_FALSE(assetDataStream.CanWrite());

    // AssetDataStream only supports forward seeking, not arbitrary seeking, so CanSeek() should be false.
    EXPECT_FALSE(assetDataStream.CanSeek());

}

TEST_F(AssetDataStreamTest, Open_OpenAndCopyBuffer_BufferCopied)
{
    // Pick an arbitrary buffer size
    constexpr int bufferSize = 100;

    // Create a buffer filled with an expected charater
    AZStd::vector<AZ::u8> buffer(bufferSize, m_expectedBufferChar);

    // Create an assetDataStream and *copy* the buffer into it
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(buffer);

    // Assign the buffer to a different, unexpected character
    buffer.assign(bufferSize, m_badBufferChar);

    // Create a new buffer, fill it with a bad character, and read the data from the assetDataStream into it
    AZStd::vector<AZ::u8> outBuffer(bufferSize, m_badBufferChar);
    AZ::IO::SizeType bytesRead = assetDataStream.Read(outBuffer.size(), outBuffer.data());

    // Validate that we read the correct number of bytes
    EXPECT_EQ(bytesRead, outBuffer.size());
    // Validate that the data read back does *not* match the invalid data.
    // This validates that the buffer got copied and not directly used.
    EXPECT_NE(buffer, outBuffer);
    // Validate that the data read back *does* match the original valid data.
    buffer.assign(bufferSize, m_expectedBufferChar);
    EXPECT_EQ(buffer, outBuffer);

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, Open_OpenAndUseBuffer_BufferUsed)
{
    // Pick an arbitrary buffer size
    constexpr int bufferSize = 100;

    // Create a buffer filled with an expected charater
    AZStd::vector<AZ::u8> buffer(bufferSize, m_expectedBufferChar);

    // Create an assetDataStream and *move* the buffer into it
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(AZStd::move(buffer));

    // The buffer should be moved, so it should no longer contain data.
    EXPECT_TRUE(buffer.empty());

    // Create a new buffer, fill it with a bad character, and read the data from the assetDataStream into it
    AZStd::vector<AZ::u8> outBuffer(bufferSize, m_badBufferChar);
    AZ::IO::SizeType bytesRead = assetDataStream.Read(outBuffer.size(), outBuffer.data());

    // Validate that we read the correct number of bytes
    EXPECT_EQ(bytesRead, outBuffer.size());
    // Validate that the data read back matches the original valid data.
    buffer.assign(bufferSize, m_expectedBufferChar);
    EXPECT_EQ(buffer, outBuffer);

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, Open_OpenAndReadFile_FileReadSuccessfully)
{
    // Choose some non-standard input values to pass in to our Open() request.
    const AZStd::string filePath("path/test");
    const size_t fileOffset = 100;
    const size_t assetSize = 500;
    const AZStd::chrono::milliseconds deadline(1000);
    const AZ::IO::IStreamerTypes::Priority priority(AZ::IO::IStreamerTypes::s_priorityHigh);

    // Keep track of whether or not our callback gets called.
    bool callbackCalled = false;
    AZ::IO::IStreamerTypes::RequestStatus callbackStatus;

    // Create a callback to call on load completion.
    AZ::Data::AssetDataStream::OnCompleteCallback loadCallback =
        [&callbackCalled, &callbackStatus](AZ::IO::IStreamerTypes::RequestStatus status)
    {
        callbackCalled = true;
        callbackStatus = status;
    };

    // Create an AssetDataStream, create a file open request, and wait for it to finish.
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(filePath, fileOffset, assetSize, deadline, priority, loadCallback);
    assetDataStream.BlockUntilLoadComplete();

    // Validate that the AssetDataStream passed our input parameters correctly to the file streamer
    EXPECT_EQ(filePath, m_relativePath);
    EXPECT_EQ(assetSize, m_size);
    EXPECT_EQ(deadline, m_deadline);
    EXPECT_EQ(priority, m_priority);
    EXPECT_EQ(fileOffset, m_offset);

    // Validate that our completion callback got called
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(callbackStatus, m_requestStatus);

    // Create a new buffer, fill it with a bad character, and read the data from the assetDataStream into it
    AZStd::vector<AZ::u8> outBuffer(assetSize, m_badBufferChar);
    AZ::IO::SizeType bytesRead = assetDataStream.Read(outBuffer.size(), outBuffer.data());

    // Validate that we read the correct number of bytes
    EXPECT_EQ(bytesRead, outBuffer.size());

    // Validate that the data read back matches the original valid data.
    EXPECT_EQ(memcmp(m_buffer, outBuffer.data(), bytesRead), 0);

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, IsOpen_OpenAndCloseStream_OnlyTrueWhileOpen)
{
    // Pick an arbitrary buffer size
    constexpr int bufferSize = 100;

    // Create a buffer filled with an expected charater
    AZStd::vector<AZ::u8> buffer(bufferSize, m_expectedBufferChar);

    AZ::Data::AssetDataStream assetDataStream;

    EXPECT_FALSE(assetDataStream.IsOpen());

    assetDataStream.Open(AZStd::move(buffer));

    EXPECT_TRUE(assetDataStream.IsOpen());

    assetDataStream.Close();

    EXPECT_FALSE(assetDataStream.IsOpen());
}

TEST_F(AssetDataStreamTest, IsFullyLoaded_OpenStreamFromBuffer_DataIsFullyLoaded)
{
    // Pick an arbitrary buffer size
    constexpr int bufferSize = 100;

    // Create a buffer filled with an expected charater
    AZStd::vector<AZ::u8> buffer(bufferSize, m_expectedBufferChar);

    AZ::Data::AssetDataStream assetDataStream;

    EXPECT_FALSE(assetDataStream.IsFullyLoaded());
    EXPECT_EQ(assetDataStream.GetLoadedSize(), 0);
    EXPECT_EQ(assetDataStream.GetLength(), 0);

    assetDataStream.Open(AZStd::move(buffer));

    EXPECT_TRUE(assetDataStream.IsFullyLoaded());
    EXPECT_EQ(assetDataStream.GetLoadedSize(), bufferSize);
    EXPECT_EQ(assetDataStream.GetLength(), bufferSize);

    assetDataStream.Close();

    EXPECT_FALSE(assetDataStream.IsFullyLoaded());
    EXPECT_EQ(assetDataStream.GetLoadedSize(), 0);
    EXPECT_EQ(assetDataStream.GetLength(), 0);

}

TEST_F(AssetDataStreamTest, IsFullyLoaded_FileDoesNotReadAllData_DataIsNotFullyLoaded)
{
    // Set up some arbitrary file parameters.
    const AZStd::string filePath("path/test");
    const size_t fileOffset = 0;
    const size_t assetSize = 500;

    // Pick a size less than assetSize to represent the amount of data actually loaded
    const size_t incompleteAssetSize = 200;

    using ::testing::_;

    ON_CALL(m_mockStreamer, GetReadRequestResult(_, _, _, _))
        .WillByDefault([this](
            [[maybe_unused]] FileRequestHandle request,
            void*& buffer,
            AZ::u64& numBytesRead,
            [[maybe_unused]] IStreamerTypes::ClaimMemory claimMemory)
            {
                // On the request for the read result, create a size and buffer that's less than the requested amount.
                m_size = incompleteAssetSize;

                auto result = m_allocator->Allocate(m_size, m_size, AZCORE_GLOBAL_NEW_ALIGNMENT);
                m_buffer = static_cast<AZ::u8*>(result.m_address);
                memset(m_buffer, m_expectedBufferChar, m_size);
                numBytesRead = m_size;
                buffer = m_buffer;
                return true;
            });


    // Create an AssetDataStream, create a file open request, and wait for it to finish.
    AZ::Data::AssetDataStream assetDataStream;

    // We expect one error message during the load due to the incomplete file load.
    AZ_TEST_START_TRACE_SUPPRESSION;
    assetDataStream.Open(filePath, fileOffset, assetSize);
    assetDataStream.BlockUntilLoadComplete();
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);

    // Verify that the data reports back the incomplete size for loaded size, and that it is not fully loaded.
    EXPECT_FALSE(assetDataStream.IsFullyLoaded());
    EXPECT_EQ(assetDataStream.GetLoadedSize(), incompleteAssetSize);

    // GetLength should still report back the expected size instead of the loaded size
    EXPECT_EQ(assetDataStream.GetLength(), assetSize);

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, Write_TryWritingToStream_WritingCausesAsserts)
{
    // Create an arbitrary buffer
    constexpr int bufferSize = 100;
    AZStd::vector<AZ::u8> buffer(bufferSize, m_expectedBufferChar);

    // Create a data stream from the buffer
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(buffer);

    // We should get an error when trying to write to the stream.
    AZ_TEST_START_ASSERTTEST;
    assetDataStream.Write(bufferSize, buffer.data());
    AZ_TEST_STOP_ASSERTTEST(1);

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, GetFilename_StreamOpenedWithFile_FileNameMatches)
{
    // Set up some arbitrary file parameters.
    const AZStd::string filePath("path/test");
    const size_t fileOffset = 0;
    const size_t assetSize = 500;

    // Create an AssetDataStream, create a file open request, and wait for it to finish.
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(filePath, fileOffset, assetSize);
    assetDataStream.BlockUntilLoadComplete();

    // Verify that the stream has the expected filename
    EXPECT_EQ(strcmp(assetDataStream.GetFilename(), filePath.c_str()), 0);

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, GetFilename_StreamOpenedWithMemoryBuffer_FileNameIsEmpty)
{
    // Create an arbitrary buffer
    constexpr int bufferSize = 100;
    AZStd::vector<AZ::u8> buffer(bufferSize, m_expectedBufferChar);

    // Create an AssetDataStream from the memory buffer
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(buffer);

    // Verify that the stream has no filename
    EXPECT_EQ(strcmp(assetDataStream.GetFilename(), ""), 0);

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, BlockUntilLoadComplete_BlockWhenOpenedWithMemoryBuffer_BlockReturnsSuccessfully)
{
    // Create an arbitrary buffer
    constexpr int bufferSize = 100;
    AZStd::vector<AZ::u8> buffer(bufferSize, m_expectedBufferChar);

    // Create an AssetDataStream from the memory buffer
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(buffer);

    // Verify that calling BlockUntilLoadComplete doesn't cause problems when used with a memory buffer instead of a file.
    assetDataStream.BlockUntilLoadComplete();

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, Read_ReadDataIncrementally_PartialDataReadSuccessfully)
{
    // Create an arbitrary buffer with different data in every byte
    constexpr int bufferSize = 256;
    AZStd::vector<AZ::u8> buffer(bufferSize);
    for (int offset = 0; offset < bufferSize; offset++)
    {
        // Use the lowest 8 bits of offset to get a repeating pattern of 00 -> FF
        buffer[offset] = offset & 0xFF;
    }

    // Create an AssetDataStream from the memory buffer
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(buffer);

    for (int offset = 0; offset < bufferSize; offset++)
    {
        // Verify that the current position increments correctly on each read request
        EXPECT_EQ(offset, assetDataStream.GetCurPos());

        AZ::u8 byte;
        auto bytesRead = assetDataStream.Read(1, &byte);

        // Verify that when we read one byte at a time, it's incrementing forward through the data set and getting the correct byte.
        EXPECT_EQ(bytesRead, 1);
        EXPECT_EQ(byte, buffer[offset]);
    }

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, Seek_SeekForward_SeekingForwardWorksSuccessfully)
{
    // Create an arbitrary buffer with different data in every byte
    constexpr int bufferSize = 256;
    AZStd::vector<AZ::u8> buffer(bufferSize);
    for (int offset = 0; offset < bufferSize; offset++)
    {
        // Use the lowest 8 bits of offset to get a repeating pattern of 00 -> FF
        buffer[offset] = offset & 0xFF;
    }

    // Create an AssetDataStream from the memory buffer
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(buffer);

    // Pick an arbitrary amount to seek forward on every iteration
    constexpr int skipForwardBytes = 3;
    // The expected offset should increment by the byte read plus the seek on every iteration
    constexpr int offsetIncrement = skipForwardBytes + 1;

    for (int offset = 0; offset < bufferSize; offset+=offsetIncrement)
    {
        // Verify that the current position is correct on each read request, even with the seek
        EXPECT_EQ(offset, assetDataStream.GetCurPos());

        AZ::u8 byte;
        assetDataStream.Read(1, &byte);

        // Verify that we're getting the byte we expected even with the seek
        EXPECT_EQ(byte, buffer[offset]);

        assetDataStream.Seek(skipForwardBytes, AZ::IO::GenericStream::SeekMode::ST_SEEK_CUR);
    }

    assetDataStream.Close();
}

TEST_F(AssetDataStreamTest, Seek_SeekBackward_SeekingBackwardNotAllowed)
{
    // Create an arbitrary buffer
    constexpr int bufferSize = 100;
    AZStd::vector<AZ::u8> buffer(bufferSize, m_expectedBufferChar);

    // Create an AssetDataStream from the memory buffer
    AZ::Data::AssetDataStream assetDataStream;
    assetDataStream.Open(buffer);

    // Moving to the start of the file doesn't move, so this should succeed
    assetDataStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

    // Read a byte
    AZ::u8 byte;
    assetDataStream.Read(1, &byte);

    // Moving to the start of the file now is moving backwards, so this should fail
    AZ_TEST_START_ASSERTTEST;
    assetDataStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
    AZ_TEST_STOP_ASSERTTEST(1);

    assetDataStream.Close();
}

