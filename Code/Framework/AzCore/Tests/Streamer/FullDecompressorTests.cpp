/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/FullFileDecompressor.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Tests/Streamer/StreamStackEntryConformityTests.h>
#include <Tests/Streamer/StreamStackEntryMock.h>

namespace AZ::IO
{
    class FullFileDecompressorTestDescription :
        public StreamStackEntryConformityTestsDescriptor<FullFileDecompressor>
    {
    public:
        static constexpr u32 m_arbitrarilyLargeAlignment = 4096;

        FullFileDecompressor CreateInstance() override
        {
            return FullFileDecompressor(2, 2, m_arbitrarilyLargeAlignment);
        }
    };

    INSTANTIATE_TYPED_TEST_CASE_P(
        Streamer_FullFileDecompressorConformityTests, StreamStackEntryConformityTests, FullFileDecompressorTestDescription);

    class Streamer_FullDecompressorTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        enum CompressionState
        {
            Uncompressed,
            Compressed,
            Corrupted
        };

        enum ReadResult
        {
            Success,
            Failed,
            Canceled
        };

        void TearDown() override
        {
            m_decompressor.reset();
            m_mock.reset();

            m_decompressor = nullptr;
            m_mock = nullptr;

            delete[] m_buffer;
            m_buffer = nullptr;

            delete m_context;
            m_context = nullptr;

            UnitTest::LeakDetectionFixture::TearDown();
        }

        void SetupEnvironment(u32 maxNumReads, u32 maxNumJobs)
        {
            m_buffer = new u32[m_fakeFileLength >> 2];

            m_mock = AZStd::make_shared<StreamStackEntryMock>();
            m_decompressor = AZStd::make_shared<FullFileDecompressor>(maxNumReads, maxNumJobs,
                FullFileDecompressorTestDescription::m_arbitrarilyLargeAlignment);

            m_context = new StreamerContext();
            m_decompressor->SetContext(*m_context);
            m_decompressor->SetNext(m_mock);
        }

        void SetupEnvironment()
        {
            SetupEnvironment(1, 1);
        }

        void MockReadCalls(ReadResult mockResult)
        {
            using ::testing::_;
            using ::testing::AnyNumber;
            using ::testing::Return;

            EXPECT_CALL(*m_mock, ExecuteRequests())
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(*m_mock, QueueRequest(_));
            EXPECT_CALL(*m_mock, UpdateStatus(_)).Times(AnyNumber());

            switch (mockResult)
            {
            case ReadResult::Success:
                ON_CALL(*m_mock, QueueRequest(_))
                    .WillByDefault(Invoke(this, &Streamer_FullDecompressorTest::PrepareReadRequest));
                break;
            case ReadResult::Failed:
                ON_CALL(*m_mock, QueueRequest(_))
                    .WillByDefault(Invoke(this, &Streamer_FullDecompressorTest::PrepareFailedReadRequest));
                break;
            case ReadResult::Canceled:
                ON_CALL(*m_mock, QueueRequest(_))
                    .WillByDefault(Invoke(this, &Streamer_FullDecompressorTest::PrepareCanceledReadRequest));
                break;
            default:
                AZ_Assert(false, "Unexpected mock result type.");
            }
        }

        void PrepareReadRequest(FileRequest* request)
        {
            auto data = AZStd::get_if<Requests::ReadData>(&request->GetCommand());
            ASSERT_NE(nullptr, data);

            u64 size = data->m_size >> 2;
            u32* buffer = reinterpret_cast<u32*>(data->m_output);
            for (u64 i = 0; i < size; ++i)
            {
                buffer[i] = aznumeric_caster(data->m_offset + (i << 2));
            }
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
        }

        void PrepareFailedReadRequest(FileRequest* request)
        {
            request->SetStatus(IStreamerTypes::RequestStatus::Failed);
            m_context->MarkRequestAsCompleted(request);
        }

        void PrepareCanceledReadRequest(FileRequest* request)
        {
            request->SetStatus(IStreamerTypes::RequestStatus::Canceled);
            m_context->MarkRequestAsCompleted(request);
        }

            static bool Decompressor(bool sleep, const void* compressed, size_t compressedSize, void* uncompressed, [[maybe_unused]] size_t uncompressedBufferSize)
        {
            AZ_Assert(compressedSize == uncompressedBufferSize, "Fake decompression algorithm only supports copying data.");
            if (sleep)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(1));
            }
            memcpy(uncompressed, compressed, compressedSize);
            return true;
        }

        static bool CorruptedDecompressor(const CompressionInfo&, const void*, size_t, void*, size_t)
        {
            return false;
        }

        void ProcessCompressedRead(u64 offset, u64 size, CompressionState compressionState, IStreamerTypes::RequestStatus expectedResult)
        {
            CompressionInfo compressionInfo;
            compressionInfo.m_compressedSize = m_fakeFileLength;
            compressionInfo.m_isCompressed = (compressionState == CompressionState::Compressed || compressionState == CompressionState::Corrupted);
            compressionInfo.m_offset = 0;
            compressionInfo.m_uncompressedSize = m_fakeFileLength;
            if (compressionState == CompressionState::Corrupted)
            {
                compressionInfo.m_decompressor = &Streamer_FullDecompressorTest::CorruptedDecompressor;
            }
            else
            {
                compressionInfo.m_decompressor = [](const CompressionInfo&, const void* compressed,
                    size_t compressedSize, void* uncompressed, size_t uncompressedBufferSize) -> bool
                {
                    return Streamer_FullDecompressorTest::Decompressor(false,
                        compressed, compressedSize, uncompressed, uncompressedBufferSize);
                };
            }

            FileRequest* request = m_context->GetNewInternalRequest();
            request->CreateCompressedRead(nullptr, AZStd::move(compressionInfo), m_buffer, offset, size);
            bool result = true;
            auto completed = [&result, expectedResult](const FileRequest& request)
            {
                result = result && request.GetStatus() == expectedResult;
            };
            request->SetCompletionCallback(completed);

            m_decompressor->QueueRequest(request);
            bool hasCompleted = false;
            while (m_decompressor->ExecuteRequests() || !hasCompleted)
            {
                StreamStackEntry::Status status;
                m_decompressor->UpdateStatus(status);
                if (status.m_isIdle)
                {
                    hasCompleted = true;
                }

                m_context->FinalizeCompletedRequests();
            }

            EXPECT_TRUE(result);
        }

        void ProcessMultipleCompressedReads()
        {
            using ::testing::_;
            using ::testing::AnyNumber;
            using ::testing::Return;

            static const constexpr size_t count = 16;

            EXPECT_CALL(*m_mock, ExecuteRequests())
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(*m_mock, QueueRequest(_)).Times(count);
            EXPECT_CALL(*m_mock, UpdateStatus(_)).Times(AnyNumber());

            ON_CALL(*m_mock, QueueRequest(_))
                .WillByDefault(Invoke(this, &Streamer_FullDecompressorTest::PrepareReadRequest));

            CompressionInfo compressionInfo;
            compressionInfo.m_compressedSize = m_fakeFileLength;
            compressionInfo.m_isCompressed = true;
            compressionInfo.m_offset = 0;
            compressionInfo.m_uncompressedSize = m_fakeFileLength;
            compressionInfo.m_decompressor = [](const CompressionInfo&, const void* compressed,
                size_t compressedSize, void* uncompressed, size_t uncompressedBufferSize) -> bool
            {
                return Streamer_FullDecompressorTest::Decompressor(true,
                    compressed, compressedSize, uncompressed, uncompressedBufferSize);
            };

            bool allCompleted = true;
            auto completed = [&allCompleted](const FileRequest& request)
            {
                allCompleted = allCompleted && request.GetStatus() == IStreamerTypes::RequestStatus::Completed;
            };

            FileRequest* requests[count];
            AZStd::unique_ptr<u32[]> buffers[count];
            for (size_t i = 0; i < count; ++i)
            {
                buffers[i] = AZStd::unique_ptr<u32[]>(new u32[m_fakeFileLength >> 2]);
                requests[i] = m_context->GetNewInternalRequest();
                requests[i]->CreateCompressedRead(nullptr, compressionInfo, buffers[i].get(), 0, m_fakeFileLength);
                requests[i]->SetCompletionCallback(completed);
                m_decompressor->QueueRequest(requests[i]);
            }

            bool hasCompleted = false;
            while (m_decompressor->ExecuteRequests() || !hasCompleted)
            {
                StreamStackEntry::Status status;
                m_decompressor->UpdateStatus(status);
                if (status.m_isIdle)
                {
                    hasCompleted = true;
                }

                m_context->FinalizeCompletedRequests();
            }

            EXPECT_TRUE(allCompleted);
        }

        void VerifyReadBuffer(u32* buffer, u64 offset, u64 size)
        {
            size = size >> 2;
            for (u64 i = 0; i < size; ++i)
            {
                // Using assert here because in case of a problem EXPECT would
                // cause a large amount of log noise.
                ASSERT_EQ(buffer[i], offset + (i << 2));
            }
        }

        void VerifyReadBuffer(u64 offset, u64 size)
        {
            VerifyReadBuffer(m_buffer, offset, size);
        }

        u32* m_buffer;
        StreamerContext* m_context;
        AZStd::shared_ptr<FullFileDecompressor> m_decompressor;
        AZStd::shared_ptr<StreamStackEntryMock> m_mock;
        u64 m_fakeFileLength{ 1 * 1024 * 1024 };
    };

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_FullReadAndDecompressData_SuccessfullyReadData)
    {
        SetupEnvironment();
        MockReadCalls(ReadResult::Success);
        ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Compressed, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength);
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_PartialReadAndDecompressData_SuccessfullyReadData)
    {
        SetupEnvironment();
        MockReadCalls(ReadResult::Success);
        ProcessCompressedRead(256, m_fakeFileLength-512, CompressionState::Compressed, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_fakeFileLength-512);
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_FullReadFromArchive_SuccessfullyReadData)
    {
        SetupEnvironment();
        MockReadCalls(ReadResult::Success);
        ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Uncompressed, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength);
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_PartialReadFromArchive_SuccessfullyReadData)
    {
        SetupEnvironment();
        MockReadCalls(ReadResult::Success);
        ProcessCompressedRead(256, m_fakeFileLength - 512, CompressionState::Uncompressed, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_fakeFileLength - 512);
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_FailedRead_FailureIsDetectedAndReported)
    {
        SetupEnvironment();
        MockReadCalls(ReadResult::Failed);
        ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Compressed, IStreamerTypes::RequestStatus::Failed);
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_CanceledRead_CancelIsDetectedAndReported)
    {
        SetupEnvironment();
        MockReadCalls(ReadResult::Canceled);
        ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Compressed, IStreamerTypes::RequestStatus::Canceled);
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_CorruptedArchiveRead_RequestIsCompletedWithFailedState)
    {
        SetupEnvironment();
        MockReadCalls(ReadResult::Success);
        ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Corrupted, IStreamerTypes::RequestStatus::Failed);
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_MultipleRequestsWithSingleJob_AllRequestsComplete)
    {
        SetupEnvironment(4, 1);
        ProcessMultipleCompressedReads();
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_MultipleRequestsWithSingleRead_AllRequestsComplete)
    {
        SetupEnvironment(1, 4);
        ProcessMultipleCompressedReads();
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_MultipleRequestsWithSingleReadAndJob_AllRequestsComplete)
    {
        SetupEnvironment(1, 1);
        ProcessMultipleCompressedReads();
    }

    TEST_F(Streamer_FullDecompressorTest, DecompressedRead_MultipleRequestsWithMultipleReadAndJobs_AllRequestsComplete)
    {
        SetupEnvironment(4, 4);
        ProcessMultipleCompressedReads();
    }
} // namespace AZ::IO
