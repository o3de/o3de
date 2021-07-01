/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/GenericStreamMock.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>

class GenericStreamTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    void SetUp() override
    {
        using ::testing::_;
        using ::testing::NiceMock;
        using ::testing::Return;

        UnitTest::ScopedAllocatorSetupFixture::SetUp();

        // Route our mocked WriteFromStream back to the base class implementation for test validation
        ON_CALL(m_mockGenericStream, WriteFromStream(_, _))
            .WillByDefault([this](AZ::IO::SizeType bytes, AZ::IO::GenericStream* inputStream)
                {
                    return m_mockGenericStream.BaseWriteFromStream(bytes, inputStream);
                });
    }

    void TearDown() override
    {
        UnitTest::ScopedAllocatorSetupFixture::TearDown();
    }

    void CreateTestData(AZStd::vector<AZ::u8>& inBuffer, size_t numBytes)
    {
        inBuffer.resize(numBytes);

        // Fill the buffer with a repeating pattern of 00->FF, FF->00, 00->FF, etc.
        // We flip the pattern ordering to ensure that our buffer copies don't keep copying the first set of bytes.
        for (size_t offset = 0; offset < numBytes; offset++)
        {
            inBuffer[offset] = (offset >> 8) & 0x01 ? ~(offset & 0xFF) : (offset & 0xFF);
        }
    }

    void TestGenericStreamWriteFromStream(size_t numBytesToWrite)
    {
        using ::testing::_;

        // Create an inputStream filled with a repeating pattern of 00-FF, FF-00, 00-FF, etc...
        AZStd::vector<AZ::u8> inBuffer(numBytesToWrite);
        CreateTestData(inBuffer, numBytesToWrite);
        AZ::IO::MemoryStream inputStream(inBuffer.data(), inBuffer.size());

        // We will use mockGenericStream as our output stream for testing, but we'll reroute the Write() call into an
        // output memoryStream to help us easily validate that the GenericStream WriteFromStream implementation works correctly.
        AZStd::vector<AZ::u8> outBuffer(numBytesToWrite);
        AZ::IO::MemoryStream outputStream(outBuffer.data(), outBuffer.size(), 0);;

        // Reroute the mock stream to our output MemoryStream for writing.
        ON_CALL(m_mockGenericStream, Write(_, _))
            .WillByDefault([this, &outputStream](AZ::IO::SizeType bytes, const void* buffer)
                {
                    return outputStream.Write(bytes, buffer);
                });

        // Verify that the default implementation of WriteFromStream works correctly.
        m_mockGenericStream.WriteFromStream(numBytesToWrite, &inputStream);
        EXPECT_EQ(inBuffer, outBuffer);
    }

    void TestMemoryStreamWriteFromStream(size_t numBytesToWrite)
    {
        using ::testing::_;

        // Create an inputStream filled with a repeating pattern of 00-FF, FF-00, 00-FF, etc...
        AZStd::vector<AZ::u8> inBuffer(numBytesToWrite);
        CreateTestData(inBuffer, numBytesToWrite);
        AZ::IO::MemoryStream inputStream(inBuffer.data(), inBuffer.size());

        // Create an output MemoryStream to write into
        AZStd::vector<AZ::u8> outBuffer(numBytesToWrite);
        AZ::IO::MemoryStream outputStream(outBuffer.data(), outBuffer.size(), 0);;

        // Verify that the MemoryStream implementation of WriteFromStream works correctly.
        outputStream.WriteFromStream(numBytesToWrite, &inputStream);
        EXPECT_EQ(inBuffer, outBuffer);
    }

protected:
    ::testing::NiceMock<GenericStreamMock> m_mockGenericStream;
};

TEST_F(GenericStreamTest, WriteFromStream_LessBytesThanTempBuffer_CopyIsSuccessful)
{
    // Test WriteFromStream with a buffer size that's smaller than the copy buffer and not a power of two
    // just to help catch any possible errors.
    TestGenericStreamWriteFromStream(AZ::IO::GenericStream::StreamToStreamCopyBufferSize / 3);
}

TEST_F(GenericStreamTest, WriteFromStream_SameByteSizeAsTempBuffer_CopyIsSuccessful)
{
    // Test WriteFromStream with a buffer size exactly equal to the copy buffer
    TestGenericStreamWriteFromStream(AZ::IO::GenericStream::StreamToStreamCopyBufferSize);
}

TEST_F(GenericStreamTest, WriteFromStream_MoreBytesThanTempBuffer_CopyIsSuccessful)
{
    // Test WriteFromStream with a buffer size that's larger than the copy buffer and not a power of two
    // just to help catch any possible errors.
    TestGenericStreamWriteFromStream(aznumeric_cast<size_t>(AZ::IO::GenericStream::StreamToStreamCopyBufferSize * 2.6f));
}

TEST_F(GenericStreamTest, MemoryStreamWriteFromStream_LessBytesThanTempBuffer_CopyIsSuccessful)
{
    // Test WriteFromStream with a buffer size that's smaller than the GenericStream copy buffer and not a power of two
    // just to help catch any possible errors.
    TestMemoryStreamWriteFromStream(AZ::IO::GenericStream::StreamToStreamCopyBufferSize / 3);
}

TEST_F(GenericStreamTest, MemoryStreamWriteFromStream_SameByteSizeAsTempBuffer_CopyIsSuccessful)
{
    // Test WriteFromStream with a buffer size exactly equal to the GenericStream copy buffer
    TestMemoryStreamWriteFromStream(AZ::IO::GenericStream::StreamToStreamCopyBufferSize);
}

TEST_F(GenericStreamTest, MemoryStreamWriteFromStream_MoreBytesThanTempBuffer_CopyIsSuccessful)
{
    // Test WriteFromStream with a buffer size that's larger than the GenericStream copy buffer and not a power of two
    // just to help catch any possible errors.
    TestMemoryStreamWriteFromStream(aznumeric_cast<size_t>(AZ::IO::GenericStream::StreamToStreamCopyBufferSize * 2.6f));
}
