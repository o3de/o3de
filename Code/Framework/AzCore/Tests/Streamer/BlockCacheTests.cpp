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
#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Tests/FileIOBaseTestTypes.h>
#include <Tests/Streamer/StreamStackEntryConformityTests.h>
#include <Tests/Streamer/StreamStackEntryMock.h>

namespace AZ::IO
{
    class BlockCacheTestDescription :
        public StreamStackEntryConformityTestsDescriptor<BlockCache>
    {
    public:
        BlockCache CreateInstance() override
        {
            return BlockCache(5 * 1024 * 1024, 64 * 1024, AZCORE_GLOBAL_NEW_ALIGNMENT, false);
        }
    };

    INSTANTIATE_TYPED_TEST_CASE_P(Streamer_BlockCacheConformityTests, StreamStackEntryConformityTests, BlockCacheTestDescription);

    class BlockCacheTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);

            m_path = "Test";
            m_context = new StreamerContext();
        }

        void TearDown() override
        {
            delete[] m_buffer;
            m_buffer = nullptr;

            m_cache = nullptr;
            m_mock = nullptr;

            delete m_context;
            m_context = nullptr;

            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        }

        void CreateTestEnvironmentImplementation(bool onlyEpilogWrites)
        {
            using ::testing::_;

            m_cache = AZStd::make_shared<BlockCache>(m_cacheSize, m_blockSize, AZCORE_GLOBAL_NEW_ALIGNMENT, onlyEpilogWrites);
            m_mock = AZStd::make_shared<StreamStackEntryMock>();
            m_cache->SetNext(m_mock);
            EXPECT_CALL(*m_mock, SetContext(_)).Times(1);
            m_cache->SetContext(*m_context);

            m_bufferSize = m_readBufferLength >> 2;
            m_buffer = new u32[m_bufferSize];
        }

        void RedirectReadCalls()
        {
            using ::testing::_;
            using ::testing::AnyNumber;
            using ::testing::Return;

            EXPECT_CALL(*m_mock, ExecuteRequests())
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(*m_mock, QueueRequest(_))
                .WillRepeatedly(Invoke(this, &BlockCacheTest::QueueReadRequest));
        }

        void QueueReadRequest(FileRequest* request)
        {
            auto data = AZStd::get_if<Requests::ReadData>(&request->GetCommand());
            if (data)
            {
                if (m_fakeFileFound)
                {
                    u64 size = data->m_size >> 2;
                    u32* buffer = reinterpret_cast<u32*>(data->m_output);
                    for (u64 i = 0; i < size; ++i)
                    {
                        buffer[i] = aznumeric_caster(data->m_offset + (i << 2));
                    }
                    ReadFile(data->m_output, data->m_path, data->m_offset, data->m_size);

                    request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                }
                else
                {
                    request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                }
                m_context->MarkRequestAsCompleted(request);
            }
            else if (
                AZStd::holds_alternative<Requests::FlushData>(request->GetCommand()) ||
                AZStd::holds_alternative<Requests::FlushAllData>(request->GetCommand()))
            {
                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
            else if (AZStd::holds_alternative<Requests::FileMetaDataRetrievalData>(request->GetCommand()))
            {
                auto& data2 = AZStd::get<Requests::FileMetaDataRetrievalData>(request->GetCommand());
                data2.m_found = m_fakeFileFound;
                data2.m_fileSize = m_fakeFileLength;
                request->SetStatus(m_fakeFileFound ? IStreamerTypes::RequestStatus::Completed : IStreamerTypes::RequestStatus::Failed);
                m_context->MarkRequestAsCompleted(request);
            }
            else
            {
                // While there are more commands that possible, only the above ones are supported in these tests. Add additional
                // commands as needed.
                FAIL();
            }
        }

        void RedirectCanceledReadCalls()
        {
            using ::testing::_;
            using ::testing::Return;

            EXPECT_CALL(*m_mock, ExecuteRequests())
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(*m_mock, QueueRequest(_))
                .WillRepeatedly(Invoke(this, &BlockCacheTest::QueueCanceledReadRequest));
        }

        void QueueCanceledReadRequest(FileRequest* request)
        {
            auto data = AZStd::get_if<Requests::ReadData>(&request->GetCommand());
            if (data)
            {
                ReadFile(data->m_output, data->m_path, data->m_offset, data->m_size);
                request->SetStatus(IStreamerTypes::RequestStatus::Canceled);
                m_context->MarkRequestAsCompleted(request);
            }
            else if (AZStd::holds_alternative<Requests::FileMetaDataRetrievalData>(request->GetCommand()))
            {
                auto& data2 = AZStd::get<Requests::FileMetaDataRetrievalData>(request->GetCommand());
                data2.m_found = true;
                data2.m_fileSize = m_fakeFileLength;
                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
            else
            {
                FAIL();
            }
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

    protected:
        // To make testing easier, this utility mock unpacks the read requests.
        MOCK_METHOD4(ReadFile, bool(void*, const RequestPath&, u64, u64));

        void RunProcessLoop()
        {
            do
            {
                while (m_context->FinalizeCompletedRequests())
                {
                }
            } while (m_cache->ExecuteRequests());
        }

        void RunAndCompleteRequest(FileRequest* request, IStreamerTypes::RequestStatus expectedResult)
        {
            IStreamerTypes::RequestStatus result = IStreamerTypes::RequestStatus::Pending;
            request->SetCompletionCallback([&result](const FileRequest& request)
            {
                // Capture result before internal request is recycled.
                result = request.GetStatus();
            });

            m_cache->QueueRequest(request);
            RunProcessLoop();

            EXPECT_EQ(expectedResult, result);
        }

        void ProcessRead(void* output, const RequestPath& path, u64 offset, u64 size, IStreamerTypes::RequestStatus expectedResult)
        {
            FileRequest* request = m_context->GetNewInternalRequest();
            request->CreateRead(nullptr, output, size, path, offset, size);
            RunAndCompleteRequest(request, expectedResult);
        }

        UnitTest::TestFileIOBase m_fileIO;
        FileIOBase* m_prevFileIO{};
        StreamerContext* m_context;
        AZStd::shared_ptr<BlockCache> m_cache;
        AZStd::shared_ptr<StreamStackEntryMock> m_mock;
        RequestPath m_path;

        u32* m_buffer{ nullptr };
        size_t m_bufferSize{ 0 };
        u64 m_cacheSize{ 5 * 1024 * 1024 };
        u32 m_blockSize{ 64 * 1024 };
        u64 m_fakeFileLength{ 5 * m_blockSize };
        u64 m_readBufferLength{ 10 * 1024 * 1024 };
        bool m_fakeFileFound{ true };
    };

    /////////////////////////////////////////////////////////////
    // Prolog and epilog enabled
    /////////////////////////////////////////////////////////////
    class Streamer_BlockCacheWithPrologAndEpilogTest
        : public BlockCacheTest
    {
    public:
        void CreateTestEnvironment()
        {
            CreateTestEnvironmentImplementation(false);
        }
    };

    // File    |------------------------------------------------|
    // Request |------------------------------------------------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_EntireFileRead_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength);
    }

    // File    |----------------------------------------------|
    // Request |----------------------------------------------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_EntireUnalignedFileRead_FileReadInASingleRead)
    {
        using ::testing::_;

        m_fakeFileLength -= 512;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength);
    }

    // File    |------------------------------------------------|
    // Request      |-------------------------------------------|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_PrologCacheLargeRequest_FileReadInTwoReads)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - m_blockSize));

        ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_fakeFileLength - 256);
    }

    // File    |------------------------------------------------|
    // Request |-------------------------------------------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_EpilogCacheLargeRequest_FileReadInTwoReads)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength - m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength - 256);
    }

    // File    |----------------------------------------------|
    // Request |-------------------------------------------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_UnalignedEpilogCacheLargeRequest_FileReadInTwoReads)
    {
        using ::testing::_;

        m_fakeFileLength -= 512;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, 4 * m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, 4 * m_blockSize, m_fakeFileLength - 4 * m_blockSize));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength - 256);
    }

    // File    |------------------------------------------------|
    // Request     |---------------------------------------|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   v    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_PrologAndEpilogCacheLargeRequest_FileReadInThreeReads)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - 2 * m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 512, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_fakeFileLength - 512);
    }

    // File    |------------------------------------------------|
    // Request     |---------|
    // Cache   [   v    ][    v   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_PrologAndEpilogInTwoBlocks_FileReadInTwoReads)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 256, m_blockSize, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_blockSize);
    }

    // File    |------------------------------------------------|
    // Request |--------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_ExactBlockRead_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

        ProcessRead(m_buffer, m_path, 0, m_blockSize, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_blockSize);
    }

    // File    |------------------------------------------------|
    // Request |------|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_SmallRead_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

        ProcessRead(m_buffer, m_path, 0, m_blockSize - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_blockSize - 256);
    }

    // File    |-----|
    // Request |---|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadUnalignedFile_FileReadInASingleRead)
    {
        using ::testing::_;

        m_fakeFileLength = m_blockSize - 128;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength - 256);
    }

    // File    |------------------------------------------------|
    // Request   |---|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadWithOffset_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

        ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_blockSize - 512);
    }

    // File    |-----|
    // Request  |---|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadWithOffsetInUnaligedFile_FileReadInASingleRead)
    {
        using ::testing::_;

        m_fakeFileLength = m_blockSize - 128;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_blockSize - 512);
    }

    // File    |-----|
    // Request  |----|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadTillEndWithOffsetInUnaligedFile_FileReadInASingleRead)
    {
        using ::testing::_;

        m_fakeFileLength = m_blockSize - 128;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_fakeFileLength - 256);
    }

    // File    |------------------------------------------------|
    // Request              |---|
    // Cache   [   x    ][    v   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadWithOffsetIntoNextBlock_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, m_blockSize + 256, m_blockSize - 512, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(m_blockSize + 256, m_blockSize - 512);
    }

    // File    |------------------------------------------------|
    // Request                                           |------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadWithOffsetIntoLastBlock_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 4 * m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 4 * m_blockSize + 256, m_blockSize - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(4 * m_blockSize + 256, m_blockSize - 256);
    }

    // File     |----------------------------------------------|
    // Request0     |---------------------------------------|
    // Request1                                           |----|
    // Cache    [   v    ][    x   ][   x    ][   x    ][   v    ]
    // This test queues up multiple read that overlap so one in-flight cache block is used in two requests.
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_MultipleReadsOverlapping_BothFilesAreCorrectlyRead)
    {
        using ::testing::_;
        using ::testing::Return;

        CreateTestEnvironment();

        EXPECT_CALL(*m_mock, ExecuteRequests())
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .WillRepeatedly(Invoke(this, &BlockCacheTest::QueueReadRequest));

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - 2 * m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

        constexpr size_t secondReadSize = 512;
        constexpr size_t buffer1Size = secondReadSize / sizeof(u32);
        u32 buffer1[buffer1Size];

        FileRequest* request0 = m_context->GetNewInternalRequest();
        FileRequest* request1 = m_context->GetNewInternalRequest();
        request0->CreateRead(nullptr, m_buffer, m_bufferSize, m_path, 256, m_fakeFileLength - 512);
        request1->CreateRead(nullptr, buffer1, buffer1Size, m_path, m_fakeFileLength-768, secondReadSize);
        bool allRequestsCompleted = true;
        auto completed = [&allRequestsCompleted](const FileRequest& request)
        {
            // Capture result before request is recycled.
            allRequestsCompleted = allRequestsCompleted && request.GetStatus() == IStreamerTypes::RequestStatus::Completed;
        };
        request0->SetCompletionCallback(completed);
        request1->SetCompletionCallback(completed);

        m_cache->QueueRequest(request0);
        m_cache->QueueRequest(request1);
        RunProcessLoop();

        EXPECT_TRUE(allRequestsCompleted);

        VerifyReadBuffer(256, m_fakeFileLength - 512);
        VerifyReadBuffer(buffer1, m_fakeFileLength - 768, secondReadSize);
    }

    // File     |----------------------------------------------|
    // Request0     |---------------------------------------|
    // Request1                                           |----|
    // Cache    [   v    ][    x   ][   x    ][   x    ][   v    ]
    // This test queues up multiple read that overlap so one in-flight cache block is used in two requests. This
    // is the same as the previous version except it finishes the first request so there's no wait-request created.
    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_MultipleReadsOverlappingAfterComplete_BothFilesAreCorrectlyRead)
    {
        using ::testing::_;
        using ::testing::Return;

        CreateTestEnvironment();

        EXPECT_CALL(*m_mock, ExecuteRequests())
            .WillOnce(Return(true))
            .WillOnce(Return(false))
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .WillRepeatedly(Invoke(this, &BlockCacheTest::QueueReadRequest));


        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - 2 * m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

        constexpr size_t secondReadSize = 512;
        constexpr size_t buffer1Size = secondReadSize / sizeof(u32);
        u32 buffer1[buffer1Size];

        FileRequest* request0 = m_context->GetNewInternalRequest();
        FileRequest* request1 = m_context->GetNewInternalRequest();
        request0->CreateRead(nullptr, m_buffer, m_bufferSize, m_path, 256, m_fakeFileLength - 512);
        request1->CreateRead(nullptr, buffer1, buffer1Size, m_path, m_fakeFileLength - 768, secondReadSize);
        bool allRequestsCompleted = true;
        auto completed = [&allRequestsCompleted](const FileRequest& request)
        {
            // Capture result before request is recycled.
            allRequestsCompleted = allRequestsCompleted && request.GetStatus() == IStreamerTypes::RequestStatus::Completed;
        };
        request0->SetCompletionCallback(completed);
        request1->SetCompletionCallback(completed);

        m_cache->QueueRequest(request0);
        RunProcessLoop();
        m_cache->QueueRequest(request1);
        RunProcessLoop();

        EXPECT_TRUE(allRequestsCompleted);

        VerifyReadBuffer(256, m_fakeFileLength - 512);
        VerifyReadBuffer(buffer1, m_fakeFileLength - 768, secondReadSize);
    }

    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_FileLengthNotFound_RequestReturnsFailure)
    {
        using ::testing::_;
        using ::testing::Return;

        CreateTestEnvironment();
        RedirectReadCalls();

        m_fakeFileFound = false;
        m_fakeFileLength = 0;

        ProcessRead(m_buffer, m_path, 0, m_blockSize, IStreamerTypes::RequestStatus::Failed);
    }

    TEST_F(Streamer_BlockCacheWithPrologAndEpilogTest, ReadFile_DelayedRequest_DelayedRequestAlsoFinishes)
    {
        using ::testing::_;
        using ::testing::Invoke;
        using ::testing::Return;

        static const constexpr size_t count = 3;

        m_cacheSize = (count - 1) * m_blockSize;
        CreateTestEnvironment();

        EXPECT_CALL(*m_mock, ExecuteRequests())
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(count * 2) // Once for the file meta file retrieval and a second time for the read.
            .WillRepeatedly(Invoke(this, &BlockCacheTest::QueueReadRequest));
        EXPECT_CALL(*m_mock, UpdateStatus(_))
            .WillRepeatedly(Invoke([](StreamStackEntry::Status& status)
            {
                status.m_numAvailableSlots = 64;
                status.m_isIdle = false;
            }));
        EXPECT_CALL(*this, ReadFile(_, _, _, _)).Times(count);

        constexpr size_t scratchBufferSize = 128_kib;
        using ScratchBuffer = char[scratchBufferSize];
        ScratchBuffer buffers[count];

        bool allRequestsCompleted = true;
        auto completed = [&allRequestsCompleted](const FileRequest& request)
        {
            // Capture result before request is recycled.
            allRequestsCompleted = allRequestsCompleted && request.GetStatus() == IStreamerTypes::RequestStatus::Completed;
        };

        for (size_t i = 0; i < count; ++i)
        {
            StreamStackEntry::Status status;
            m_cache->UpdateStatus(status);
            EXPECT_EQ(i == (count - 1) ? 0 : (count - i - 1), status.m_numAvailableSlots);

            FileRequest* request = m_context->GetNewInternalRequest();
            request->CreateRead(nullptr, buffers[i], scratchBufferSize, m_path, (m_blockSize - 256) * i, m_blockSize - 256);
            request->SetCompletionCallback(completed);
            m_cache->QueueRequest(request);
        }

        RunProcessLoop();
        EXPECT_TRUE(allRequestsCompleted);
    }




    /////////////////////////////////////////////////////////////
    // Prolog and epilog can read, but only the epilog can write.
    /////////////////////////////////////////////////////////////
    class Streamer_BlockCacheWithOnlyEpilogTest
        : public BlockCacheTest
    {
    public:
        void CreateTestEnvironment()
        {
            CreateTestEnvironmentImplementation(true);
        }
    };

    // File    |------------------------------------------------|
    // Request |------------------------------------------------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_EntireFileRead_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength);
    }

    // File    |----------------------------------------------|
    // Request |----------------------------------------------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_EntireUnalignedFileRead_FileReadInASingleRead)
    {
        using ::testing::_;

        m_fakeFileLength -= 512;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength);
    }

    // File    |------------------------------------------------|
    // Request      |-------------------------------------------|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_PrologCacheLargeRequest_FileReadInTwoReads)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 256, m_fakeFileLength - 256));

        ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_fakeFileLength - 256);
    }

    // File    |------------------------------------------------|
    // Request |-------------------------------------------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_EpilogCacheLargeRequest_FileReadInTwoReads)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength - m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength - 256);
    }

    // File    |----------------------------------------------|
    // Request |-------------------------------------------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_UnalignedEpilogCacheLargeRequest_FileReadInTwoReads)
    {
        using ::testing::_;

        m_fakeFileLength -= 512;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, 4 * m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, 4 * m_blockSize, m_fakeFileLength - 4 * m_blockSize));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength - 256);
    }

    // File    |------------------------------------------------|
    // Request     |---------------------------------------|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   v    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_PrologAndEpilogCacheLargeRequest_FileReadInThreeReads)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 256, m_fakeFileLength - m_blockSize - 256));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 512, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_fakeFileLength - 512);
    }

    // File    |------------------------------------------------|
    // Request     |---------|
    // Cache   [   v    ][    v   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_PrologAndEpilogInTwoBlocks_FileReadInTwoReads)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 256, m_blockSize - 256));
        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 256, m_blockSize, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_blockSize);
    }

    // File    |------------------------------------------------|
    // Request |--------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_ExactBlockRead_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

        ProcessRead(m_buffer, m_path, 0, m_blockSize, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_blockSize);
    }

    // File    |------------------------------------------------|
    // Request |------|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_SmallRead_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

        ProcessRead(m_buffer, m_path, 0, m_blockSize - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_blockSize - 256);
    }

    // File    |-----|
    // Request |---|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadUnalignedFile_FileReadInASingleRead)
    {
        using ::testing::_;

        m_fakeFileLength = m_blockSize - 128;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(0, m_fakeFileLength - 256);
    }

    // File    |------------------------------------------------|
    // Request   |---|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadWithOffset_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

        ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_blockSize - 512);
    }

    // File    |-----|
    // Request  |---|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadWithOffsetInUnaligedFile_FileReadInASingleRead)
    {
        using ::testing::_;

        m_fakeFileLength = m_blockSize - 128;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_blockSize - 512);
    }

    // File    |-----|
    // Request  |----|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadTillEndWithOffsetInUnaligedFile_FileReadInASingleRead)
    {
        using ::testing::_;

        m_fakeFileLength = m_blockSize - 128;
        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

        ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(256, m_fakeFileLength - 256);
    }

    // File    |------------------------------------------------|
    // Request              |---|
    // Cache   [   x    ][    v   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadWithOffsetIntoNextBlock_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, m_blockSize + 256, m_blockSize - 512, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(m_blockSize + 256, m_blockSize - 512);
    }

    // File    |------------------------------------------------|
    // Request                                           |------|
    // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadWithOffsetIntoLastBlock_FileReadInASingleRead)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 4 * m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 4 * m_blockSize + 256, m_blockSize - 256, IStreamerTypes::RequestStatus::Completed);
        VerifyReadBuffer(4 * m_blockSize + 256, m_blockSize - 256);
    }

    // File     |----------------------------------------------|
    // Request0     |---------------------------------------|
    // Request1                                           |----|
    // Cache    [   v    ][    x   ][   x    ][   x    ][   v    ]
    // This test queues up multiple reads that overlap so one in-flight cache block is used in two requests.
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_MultipleReadsOverlapping_BothFilesAreCorrectlyRead)
    {
        using ::testing::_;
        using ::testing::Return;

        CreateTestEnvironment();

        EXPECT_CALL(*m_mock, ExecuteRequests())
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .WillRepeatedly(Invoke(this, &BlockCacheTest::QueueReadRequest));

        EXPECT_CALL(*this, ReadFile(_, _, 256, m_fakeFileLength - m_blockSize - 256));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));


        constexpr size_t secondReadSize = 512;
        constexpr size_t buffer1Size = secondReadSize / sizeof(u32);
        u32 buffer1[buffer1Size];

        FileRequest* request0 = m_context->GetNewInternalRequest();
        FileRequest* request1 = m_context->GetNewInternalRequest();
        request0->CreateRead(nullptr, m_buffer, m_bufferSize, m_path, 256, m_fakeFileLength - 512);
        request1->CreateRead(nullptr, buffer1, buffer1Size, m_path, m_fakeFileLength - 768, secondReadSize);
        bool allRequestsCompleted = true;
        auto completed = [&allRequestsCompleted](const FileRequest& request)
        {
            // Capture result before request is recycled.
            allRequestsCompleted = allRequestsCompleted && request.GetStatus() == IStreamerTypes::RequestStatus::Completed;
        };
        request0->SetCompletionCallback(completed);
        request1->SetCompletionCallback(completed);

        m_cache->QueueRequest(request0);
        m_cache->QueueRequest(request1);
        RunProcessLoop();

        EXPECT_TRUE(allRequestsCompleted);

        VerifyReadBuffer(256, m_fakeFileLength - 512);
        VerifyReadBuffer(buffer1, m_fakeFileLength - 768, secondReadSize);
    }

    // File     |----------------------------------------------|
    // Request0     |---------------------------------------|
    // Request1                                           |----|
    // Cache    [   v    ][    x   ][   x    ][   x    ][   v    ]
    // This test queues up multiple reads that overlap so one in-flight cache block is used in two requests. This
    // is the same as the previous version except it finishes the first request so there's no wait-request created.
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_MultipleReadsOverlappingAfterComplete_BothFilesAreCorrectlyRead)
    {
        using ::testing::_;
        using ::testing::Return;

        CreateTestEnvironment();

        EXPECT_CALL(*m_mock, ExecuteRequests())
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .WillRepeatedly(Invoke(this, &BlockCacheTest::QueueReadRequest));

        EXPECT_CALL(*this, ReadFile(_, _, 256, m_fakeFileLength - m_blockSize - 256));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));


        constexpr size_t secondReadSize = 512;
        constexpr size_t buffer1Size = secondReadSize / sizeof(u32);
        u32 buffer1[buffer1Size];

        FileRequest* request0 = m_context->GetNewInternalRequest();
        FileRequest* request1 = m_context->GetNewInternalRequest();
        request0->CreateRead(nullptr, m_buffer, m_bufferSize, m_path, 256, m_fakeFileLength - 512);
        request1->CreateRead(nullptr, buffer1, buffer1Size, m_path, m_fakeFileLength - 768, secondReadSize);
        bool allRequestsCompleted = true;
        auto completed = [&allRequestsCompleted](const FileRequest& request)
        {
            // Capture result before request is recycled.
            allRequestsCompleted = allRequestsCompleted && request.GetStatus() == IStreamerTypes::RequestStatus::Completed;
        };
        request0->SetCompletionCallback(completed);
        request1->SetCompletionCallback(completed);

        m_cache->QueueRequest(request0);
        RunProcessLoop();
        m_cache->QueueRequest(request1);
        m_context->FinalizeCompletedRequests();
        RunProcessLoop();

        EXPECT_TRUE(allRequestsCompleted);

        VerifyReadBuffer(256, m_fakeFileLength - 512);
        VerifyReadBuffer(buffer1, m_fakeFileLength - 768, secondReadSize);
    }

    // File     |----------------------------------------------|
    // Request0     |---------------------|
    // Request1                        |----------------------|
    // Cache    [   v    ][    x   ][   x    ][   x    ][   v    ]
    // This test queues up multiple reads that overlap so one in-flight cache block is used in two requests with the second
    // request using prolog, main and epilog.
    TEST_F(Streamer_BlockCacheWithOnlyEpilogTest, ReadFile_FetchPrologFromCache_BothFilesAreCorrectlyRead)
    {
        using ::testing::_;
        using ::testing::Return;

        CreateTestEnvironment();

        EXPECT_CALL(*m_mock, ExecuteRequests())
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .WillRepeatedly(Invoke(this, &BlockCacheTest::QueueReadRequest));

        size_t firstReadSize = m_fakeFileLength - (2 * m_blockSize) - 512;
        FileRequest* request0 = m_context->GetNewInternalRequest();
        request0->CreateRead(nullptr, m_buffer, m_bufferSize, m_path, 256, firstReadSize);
        EXPECT_CALL(*this, ReadFile(_, _, 256, m_fakeFileLength - (3 * m_blockSize) - 256));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - (3 * m_blockSize), m_blockSize));

        size_t secondReadSize = (m_blockSize - 256) + m_blockSize + (m_blockSize - 256);
        size_t secondReadOffset = m_fakeFileLength - secondReadSize - 256;
        size_t buffer1Size = secondReadSize / sizeof(u32);
        auto buffer1 = AZStd::make_unique<u32[]>(buffer1Size);
        FileRequest* request1 = m_context->GetNewInternalRequest();
        request1->CreateRead(nullptr, buffer1.get(), buffer1Size, m_path, secondReadOffset, secondReadSize);
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - 2 * m_blockSize, m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

        bool allRequestsCompleted = true;
        auto completed = [&allRequestsCompleted](const FileRequest& request)
        {
            // Capture result before request is recycled.
            allRequestsCompleted = allRequestsCompleted && request.GetStatus() == IStreamerTypes::RequestStatus::Completed;
        };
        request0->SetCompletionCallback(completed);
        request1->SetCompletionCallback(completed);

        m_cache->QueueRequest(request0);
        m_cache->QueueRequest(request1);
        RunProcessLoop();

        EXPECT_TRUE(allRequestsCompleted);

        VerifyReadBuffer(256, firstReadSize);
        VerifyReadBuffer(buffer1.get(), secondReadOffset, secondReadSize);
    }





    /////////////////////////////////////////////////////////////
    // Generic block cache test.
    /////////////////////////////////////////////////////////////
    class Streamer_BlockCacheGenericTest
        : public BlockCacheTest
    {
    public:
        void CreateTestEnvironment()
        {
            CreateTestEnvironmentImplementation(false);
        }
    };

    TEST_F(Streamer_BlockCacheGenericTest, Cancel_QueueReadAndCancel_SubRequestPushCanceledThroughCache)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectCanceledReadCalls();

        EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - 2 * m_blockSize));
        EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

        ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 512, IStreamerTypes::RequestStatus::Canceled);
    }

    // File    |------------------------------------------------|
    // Request   |---|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheGenericTest, Flush_FlushPreviouslyReadFile_FileIsReadAgain)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        // Initial read to seed the cache.
        EXPECT_CALL(*this, ReadFile(_, _, _, _)).Times(1);
        ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, IStreamerTypes::RequestStatus::Completed);

        // Flush from the cache to remove the previously cached read.
        FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFlush(m_path);
        RunAndCompleteRequest(request, IStreamerTypes::RequestStatus::Completed);

        // The partial read would normally be serviced from the cache, but now triggers another read.
        EXPECT_CALL(*this, ReadFile(_, _, _, _)).Times(1);
        ProcessRead(m_buffer, m_path, 512, m_blockSize - 1024, IStreamerTypes::RequestStatus::Completed);
    }

    // File    |------------------------------------------------|
    // Request   |---|
    // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
    TEST_F(Streamer_BlockCacheGenericTest, FlushAll_FlushAllPreviouslyReadFiles_FileIsReadAgain)
    {
        using ::testing::_;

        CreateTestEnvironment();
        RedirectReadCalls();

        // Initial read to seed the cache.
        EXPECT_CALL(*this, ReadFile(_, _, _, _)).Times(1);
        ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, IStreamerTypes::RequestStatus::Completed);

        // Flush everything from the cache, including the previously cached read.
        FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFlushAll();
        RunAndCompleteRequest(request, IStreamerTypes::RequestStatus::Completed);

        // The partial read would normally be serviced from the cache, but now triggers another read.
        EXPECT_CALL(*this, ReadFile(_, _, _, _)).Times(1);
        ProcessRead(m_buffer, m_path, 512, m_blockSize - 1024, IStreamerTypes::RequestStatus::Completed);
    }
} // namespace AZ::IO
