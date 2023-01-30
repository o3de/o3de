/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/IO/Streamer/ReadSplitter.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/FileIOBaseTestTypes.h>
#include <Tests/Streamer/StreamStackEntryConformityTests.h>
#include <Tests/Streamer/StreamStackEntryMock.h>

namespace AZ::IO
{
    class ReadSplitterTestDescription :
        public StreamStackEntryConformityTestsDescriptor<ReadSplitter>
    {
    public:
        ReadSplitter CreateInstance() override
        {
            return ReadSplitter(64_kib, AZCORE_GLOBAL_NEW_ALIGNMENT, 1, 0, false, true);
        }

        bool UsesSlots() const override
        {
            return false;
        }
    };

    class ReadSplitterWithBufferTestDescription :
        public StreamStackEntryConformityTestsDescriptor<ReadSplitter>
    {
    public:
        ReadSplitter CreateInstance() override
        {
            return ReadSplitter(64_kib, 4096, 512, 5_mib, true, true);
        }

        bool UsesSlots() const override
        {
            return true;
        }
    };

    using ReadSplitterTestTypes = ::testing::Types<ReadSplitterTestDescription, ReadSplitterWithBufferTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(Streamer_ReadSplitterConformityTests, StreamStackEntryConformityTests, ReadSplitterTestTypes);

    class Streamer_ReadSplitterTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        static constexpr u64 SplitSize = 1_kib;
        static constexpr size_t MemoryAlignment = 4096;
        static constexpr size_t SizeAlignment = 512;

        Streamer_ReadSplitterTest()
            : m_mock(AZStd::make_shared<StreamStackEntryMock>())
        {
        }

        void SetUp() override
        {
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);
        }

        void TearDown() override
        {
            if (m_readSplitter)
            {
                delete m_readSplitter;
                m_readSplitter = nullptr;
            }
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        }

        void CreateReadSplitter(u64 maxReadSize, u32 memoryAlignment, u32 sizeAlignment, size_t bufferSize,
            bool adjustOffset, bool splitAlignedRequests)
        {
            using ::testing::_;

            m_readSplitter = new ReadSplitter(maxReadSize, memoryAlignment, sizeAlignment, bufferSize,
                adjustOffset, splitAlignedRequests);

            m_readSplitter->SetNext(m_mock);
            EXPECT_CALL(*m_mock, SetContext(_));
            m_readSplitter->SetContext(m_context);
        }

        void CreateStandardReadSplitter()
        {
            CreateReadSplitter(SplitSize, AZCORE_GLOBAL_NEW_ALIGNMENT, 1, 0, false, true);
        }

        void CreateAlignmentAwareReadSplitter(size_t bufferSize, bool adjustOffset)
        {
            CreateReadSplitter(SplitSize, MemoryAlignment, SizeAlignment, bufferSize, adjustOffset, true);
        }

        void CreatePassThroughReadSplitter()
        {
            // By having no buffer all requests are considered aligned. By turning off splitting
            // splitting aligned request this configuration effectively records the alignment state
            // and passes the request on to the next stack entry.
            CreateReadSplitter(SplitSize, AZCORE_GLOBAL_NEW_ALIGNMENT, 1, 0, false, false);
        }

    protected:
        UnitTest::TestFileIOBase m_fileIO;
        FileIOBase* m_prevFileIO{};
        StreamerContext m_context;
        ReadSplitter* m_readSplitter{ nullptr };
        AZStd::shared_ptr<StreamStackEntryMock> m_mock;
    };

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_LessThanSplitSize_RequestIsForwarded)
    {
        CreateStandardReadSplitter();

        FileRequest* readRequest = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequest->CreateRead(nullptr, nullptr, SplitSize / 2, path, 0, SplitSize / 2);

        EXPECT_CALL(*m_mock, QueueRequest(readRequest)).Times(1);

        m_readSplitter->QueueRequest(readRequest);

        m_context.RecycleRequest(readRequest);
    }

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_TwiceTheSplitSize_TwoSubRequestsCreated)
    {
        using ::testing::_;

        CreateStandardReadSplitter();

        char buffer[SplitSize * 2];
        FileRequest* readRequest = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequest->CreateRead(nullptr, buffer, SplitSize * 2, path, 0, SplitSize * 2);

        AZStd::vector<FileRequest*> subRequests;
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(2)
            .WillRepeatedly([&subRequests](FileRequest* request) {subRequests.push_back(request); });

        m_readSplitter->QueueRequest(readRequest);
        for (size_t i=0; i<subRequests.size(); ++i)
        {
            EXPECT_EQ(subRequests[i]->GetParent(), readRequest);

            Requests::ReadData* data = AZStd::get_if<Requests::ReadData>(&subRequests[i]->GetCommand());
            ASSERT_NE(nullptr, data);
            EXPECT_EQ(SplitSize, data->m_size);
            EXPECT_EQ(SplitSize * i, data->m_offset);
            EXPECT_EQ(buffer + (SplitSize * i), data->m_output);
            EXPECT_EQ(path, data->m_path);

            m_context.MarkRequestAsCompleted(subRequests[i]);
        }
        m_context.FinalizeCompletedRequests();
    }

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_NoSplitOnAlignedEnabled_RequestIsForwardedWithoutChange)
    {
        CreatePassThroughReadSplitter();

        FileRequest* readRequest = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequest->CreateRead(nullptr, nullptr, SplitSize * 2, path, 0, SplitSize * 2);

        EXPECT_CALL(*m_mock, QueueRequest(readRequest)).Times(1);

        m_readSplitter->QueueRequest(readRequest);

        m_context.RecycleRequest(readRequest);
    }

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_MoreSubRequestsThanDepenencies_AdditionalDependenciesAreDelayed)
    {
        using ::testing::_;

        CreateStandardReadSplitter();

        constexpr size_t batchSize = FileRequest::GetMaxNumDependencies();
        constexpr size_t numSubReads = batchSize + 2;
        constexpr size_t size = numSubReads * SplitSize;
        auto buffer = AZStd::unique_ptr<u8[]>(new u8[size]);
        FileRequest* readRequest = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequest->CreateRead(nullptr, buffer.get(), size, path, 0, size);

        AZStd::vector<FileRequest*> subRequests;
        subRequests.reserve(numSubReads);
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(batchSize)
            .WillRepeatedly([&subRequests](FileRequest* request) { subRequests.push_back(request); });

        m_readSplitter->QueueRequest(readRequest);
        ASSERT_EQ(subRequests.size(), batchSize);
        for (size_t i = 0; i < subRequests.size(); ++i)
        {
            EXPECT_EQ(subRequests[i]->GetParent(), readRequest);

            Requests::ReadData* data = AZStd::get_if<Requests::ReadData>(&subRequests[i]->GetCommand());
            ASSERT_NE(nullptr, data);
            EXPECT_EQ(SplitSize, data->m_size);
            EXPECT_EQ(SplitSize * i, data->m_offset);
            EXPECT_EQ(buffer.get() + (SplitSize * i), data->m_output);
            EXPECT_EQ(path, data->m_path);

            m_context.MarkRequestAsCompleted(subRequests[i]);
        }

        subRequests.clear();
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(numSubReads - batchSize)
            .WillRepeatedly([&subRequests](FileRequest* request) { subRequests.push_back(request); });
        m_context.FinalizeCompletedRequests();

        for (size_t i = 0; i < subRequests.size(); ++i)
        {
            EXPECT_EQ(subRequests[i]->GetParent(), readRequest);

            Requests::ReadData* data = AZStd::get_if<Requests::ReadData>(&subRequests[i]->GetCommand());
            ASSERT_NE(nullptr, data);
            EXPECT_EQ(SplitSize, data->m_size);
            EXPECT_EQ(SplitSize * (batchSize + i), data->m_offset);
            EXPECT_EQ(buffer.get() + (SplitSize * (batchSize + i)), data->m_output);
            EXPECT_EQ(path, data->m_path);

            m_context.MarkRequestAsCompleted(subRequests[i]);
        }
        m_context.FinalizeCompletedRequests();
    }

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_UnalignedMemoryAdjusted_BuffersAreUsedToReadTo)
    {
        using ::testing::_;

        CreateAlignmentAwareReadSplitter(1_mib, false);

        constexpr u64 readSize = SplitSize / 2;

        u8* memory = reinterpret_cast<u8*>(azmalloc(readSize + 3, MemoryAlignment));
        u8* buffer = memory + 3; // Adjust the starting address so it doesn't align
        FileRequest* readRequest = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequest->CreateRead(nullptr, buffer, readSize, path, 0, readSize);

        FileRequest* subRequest{ nullptr };
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(1)
            .WillRepeatedly([&subRequest](FileRequest* request) { subRequest = request; });

        m_readSplitter->QueueRequest(readRequest);

        ASSERT_NE(nullptr, subRequest);
        Requests::ReadData* data = AZStd::get_if<Requests::ReadData>(&subRequest->GetCommand());
        EXPECT_NE(buffer, data->m_output);
        EXPECT_EQ(readSize, data->m_size);
        EXPECT_EQ(0, data->m_offset);
        EXPECT_EQ(path, data->m_path);
        u32* subRequestBuffer = reinterpret_cast<u32*>(data->m_output);
        for (u64 i = 0; i < data->m_size / sizeof(u32); ++i)
        {
            subRequestBuffer[i] = aznumeric_caster(i);
        }

        m_context.MarkRequestAsCompleted(subRequest);
        m_context.FinalizeCompletedRequests();

        u32* readBuffer = reinterpret_cast<u32*>(buffer);
        for (u64 i = 0; i < readSize / sizeof(u32); ++i)
        {
            ASSERT_EQ(aznumeric_cast<u32>(i), readBuffer[i]);
        }

        azfree(memory);
    }

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_UnalignedOffsetAdjusted_BuffersAreUsedToReadTo)
    {
        using ::testing::_;

        CreateAlignmentAwareReadSplitter(1_mib, true);

        constexpr u64 offsetAdjustment = sizeof(u32) * 2;
        constexpr u64 readSize = SplitSize / 2;
        ASSERT_GT(MemoryAlignment, offsetAdjustment);
        u8* buffer = reinterpret_cast<u8*>(azmalloc(readSize, MemoryAlignment));
        FileRequest* readRequest = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequest->CreateRead(nullptr, buffer, readSize, path, offsetAdjustment, readSize);

        FileRequest* subRequest{ nullptr };
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(1)
            .WillRepeatedly([&subRequest](FileRequest* request) { subRequest = request; });

        m_readSplitter->QueueRequest(readRequest);

        ASSERT_NE(nullptr, subRequest);
        Requests::ReadData* data = AZStd::get_if<Requests::ReadData>(&subRequest->GetCommand());
        EXPECT_NE(buffer, data->m_output);
        EXPECT_EQ(readSize + offsetAdjustment, data->m_size);
        EXPECT_EQ(0, data->m_offset);
        EXPECT_EQ(path, data->m_path);
        u32* subRequestBuffer = reinterpret_cast<u32*>(data->m_output);
        for (u64 i = 0; i < data->m_size / sizeof(u32); ++i)
        {
            subRequestBuffer[i] = aznumeric_caster(i);
        }

        m_context.MarkRequestAsCompleted(subRequest);
        m_context.FinalizeCompletedRequests();

        u32* readBuffer = reinterpret_cast<u32*>(buffer);
        for (u64 i = 0; i < readSize / sizeof(u32); ++i)
        {
            ASSERT_EQ(aznumeric_cast<u32>(i) + (offsetAdjustment / sizeof(u32)), readBuffer[i]);
        }

        azfree(buffer);
    }

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_ReadMoreThanFitsInTheCache_ReadsAreDelayedAndThenContinued)
    {
        using ::testing::_;

        CreateAlignmentAwareReadSplitter(SplitSize * 4, false);

        constexpr u64 readSize = SplitSize * 6;

        u8* memory = reinterpret_cast<u8*>(azmalloc(readSize + 3, MemoryAlignment));
        u8* buffer = memory + 3; // Adjust the starting address so it doesn't align
        FileRequest* readRequest = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequest->CreateRead(nullptr, buffer, readSize, path, 0, readSize);

        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(4)
            .WillRepeatedly([this](FileRequest* request) { m_context.MarkRequestAsCompleted(request); });

        m_readSplitter->QueueRequest(readRequest);

        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(2)
            .WillRepeatedly([this](FileRequest* request) { m_context.MarkRequestAsCompleted(request); });
        m_context.FinalizeCompletedRequests();

        azfree(memory);
    }

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_AlignedReadAfterDelayedRead_SecondReadIsDelayedAsWellAndBothComplete)
    {
        using ::testing::_;

        CreateAlignmentAwareReadSplitter(SplitSize * 4, false);

        constexpr u64 readSize = SplitSize * 6;

        size_t completedRequests{ 0 };

        u8* memory0 = reinterpret_cast<u8*>(azmalloc(readSize + 3, MemoryAlignment));
        u8* buffer = memory0 + 3; // Adjust the starting address so it doesn't align
        FileRequest* readRequestDelayed = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequestDelayed->CreateRead(nullptr, buffer, readSize, path, 0, readSize);
        readRequestDelayed->SetCompletionCallback([&completedRequests](FileRequestHandle) { ++completedRequests; });

        u8* memory1 = reinterpret_cast<u8*>(azmalloc(readSize, MemoryAlignment));
        FileRequest* readRequestAligned = m_context.GetNewInternalRequest();
        readRequestAligned->CreateRead(nullptr, memory1, readSize, path, 0, readSize);
        readRequestAligned->SetCompletionCallback([&completedRequests](FileRequestHandle) { ++completedRequests; });

        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(4)
            .WillRepeatedly([this, parent = readRequestDelayed](FileRequest* request)
                {
                    EXPECT_EQ(request->GetParent(), parent);
                    m_context.MarkRequestAsCompleted(request);
                });

        m_readSplitter->QueueRequest(readRequestDelayed);
        m_readSplitter->QueueRequest(readRequestAligned);

        auto delayedCallback = [this, parent = readRequestDelayed](FileRequest* request)
        {
            EXPECT_EQ(request->GetParent(), parent);
            m_context.MarkRequestAsCompleted(request);
        };
        auto alignedCallback = [this, parent = readRequestAligned](FileRequest* request)
        {
            EXPECT_EQ(request->GetParent(), parent);
            m_context.MarkRequestAsCompleted(request);
        };
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(2 + 6)
            .WillOnce(delayedCallback)
            .WillOnce(delayedCallback)
            .WillRepeatedly(alignedCallback);
        m_context.FinalizeCompletedRequests();

        EXPECT_EQ(2, completedRequests);

        azfree(memory1);
        azfree(memory0);
    }

    TEST_F(Streamer_ReadSplitterTest, QueueRequest_BufferedReadAfterDelayedRead_SecondReadIsDelayedAsWellAndBothComplete)
    {
        using ::testing::_;

        CreateAlignmentAwareReadSplitter(SplitSize * 4, false);

        constexpr u64 readSize = SplitSize * 6;

        size_t completedRequests{ 0 };

        u8* memory0 = reinterpret_cast<u8*>(azmalloc(readSize + 3, MemoryAlignment));
        u8* buffer0 = memory0 + 3; // Adjust the starting address so it doesn't align
        FileRequest* readRequestDelayed = m_context.GetNewInternalRequest();
        RequestPath path("TestPath");
        readRequestDelayed->CreateRead(nullptr, buffer0, readSize, path, 0, readSize);
        readRequestDelayed->SetCompletionCallback([&completedRequests](FileRequestHandle) { ++completedRequests; });

        u8* memory1 = reinterpret_cast<u8*>(azmalloc(readSize + 3, MemoryAlignment));
        u8* buffer1 = memory1 + 3;
        FileRequest* readRequestBuffered = m_context.GetNewInternalRequest();
        readRequestBuffered->CreateRead(nullptr, buffer1, readSize, path, 0, readSize);
        readRequestBuffered->SetCompletionCallback([&completedRequests](FileRequestHandle) { ++completedRequests; });

        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(4)
            .WillRepeatedly([this, parent = readRequestDelayed](FileRequest* request)
                {
                    EXPECT_EQ(request->GetParent(), parent);
                    m_context.MarkRequestAsCompleted(request);
                });

        m_readSplitter->QueueRequest(readRequestDelayed);
        m_readSplitter->QueueRequest(readRequestBuffered);

        auto delayedCallback = [this, parent = readRequestDelayed](FileRequest* request)
        {
            EXPECT_EQ(request->GetParent(), parent);
            m_context.MarkRequestAsCompleted(request);
        };
        auto alignedCallback = [this, parent = readRequestBuffered](FileRequest* request)
        {
            EXPECT_EQ(request->GetParent(), parent);
            m_context.MarkRequestAsCompleted(request);
        };
        EXPECT_CALL(*m_mock, QueueRequest(_))
            .Times(2 + 6)
            .WillOnce(delayedCallback)
            .WillOnce(delayedCallback)
            .WillRepeatedly(alignedCallback);
        m_context.FinalizeCompletedRequests();

        EXPECT_EQ(2, completedRequests);

        azfree(memory1);
        azfree(memory0);
    }

    TEST_F(Streamer_ReadSplitterTest, CollectStatistics_StatsAreReturned_AfterCallStatsAreAdded)
    {
        using ::testing::_;

        CreateStandardReadSplitter();

        AZStd::vector<Statistic> statistics;
        ASSERT_TRUE(statistics.empty());

        EXPECT_CALL(*m_mock, CollectStatistics(_));

        m_readSplitter->CollectStatistics(statistics);

        EXPECT_GT(statistics.size(), 0);
    }
} // namespace AZ::IO
