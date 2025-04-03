/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/Scheduler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Streamer/IStreamerTypesMock.h>
#include <Tests/Streamer/StreamStackEntryMock.h>

namespace AZ::IO
{
    class Streamer_SchedulerTest
        : public UnitTest::LeakDetectionFixture
    {
    protected:
        StreamerContext* m_streamerContext{ nullptr };

    public:
        void SetUp() override
        {
            using ::testing::_;
            using ::testing::AnyNumber;

            UnitTest::LeakDetectionFixture::SetUp();
            // a regular mock warns every time functions are called without being expected
            // a NiceMock only pays attention to calls you tell it to pay attention to.
            m_mock = AZStd::make_shared<testing::NiceMock<StreamStackEntryMock>>();
            ON_CALL(*m_mock, PrepareRequest(_)).WillByDefault([this](FileRequest* request) { m_mock->ForwardPrepareRequest(request); });
            ON_CALL(*m_mock, QueueRequest(_)).WillByDefault([this](FileRequest* request)   { m_mock->ForwardQueueRequest(request); });
            ON_CALL(*m_mock, UpdateStatus(_)).WillByDefault([this](StreamStackEntry::Status& status)
            {
                status.m_numAvailableSlots = 1;
                status.m_isIdle = m_isStackIdle;
            });

            // Expectation needs to be set before the Scheduler thread is started otherwise it may or may not hit before it's set in
            // a test.
            EXPECT_CALL(*m_mock, SetContext(_))
                .Times(1)
                .WillOnce([this](StreamerContext& context)
                    {
                        m_streamerContext = &context;
                        m_mock->ForwardSetContext(context);
                    });
            EXPECT_CALL(*m_mock, UpdateStatus(_)).Times(AnyNumber());

            auto isIdle = m_isStackIdle.load();
            m_isStackIdle = true;
            m_streamer = aznew IO::Streamer(AZStd::thread_desc{}, AZStd::make_unique<Scheduler>(m_mock));
            m_isStackIdle = isIdle;
            Interface<IO::IStreamer>::Register(m_streamer);
        }

        void TearDown() override
        {
            m_isStackIdle = true;
            if (m_streamer)
            {
                Interface<IO::IStreamer>::Unregister(m_streamer);
                delete m_streamer;
                m_streamer = nullptr;
            }
            m_mock.reset();

            UnitTest::LeakDetectionFixture::TearDown();
        }

        void MockForRead()
        {
            using ::testing::_;
            using ::testing::AtLeast;

            EXPECT_CALL(*m_mock, UpdateStatus(_)).Times(AtLeast(1));
            EXPECT_CALL(*m_mock, UpdateCompletionEstimates(_, _, _, _)).Times(AtLeast(1));
            EXPECT_CALL(*m_mock, PrepareRequest(_))
                .Times(1)
                .WillOnce([this](FileRequest* request)
                    {
                        AZ_Assert(m_streamerContext, "AZ::IO::Streamer is not ready to process requests.");
                        auto readData = AZStd::get_if<Requests::ReadRequestData>(&request->GetCommand());
                        AZ_Assert(readData, "Test didn't pass in the correct request.");
                        FileRequest* read = m_streamerContext->GetNewInternalRequest();
                        read->CreateRead(request, readData->m_output, readData->m_outputSize, readData->m_path,
                            readData->m_offset, readData->m_size);
                        m_streamerContext->PushPreparedRequest(read);
                    });
            EXPECT_CALL(*m_mock, ExecuteRequests()).Times(AtLeast(1));
            EXPECT_CALL(*m_mock, QueueRequest(_))
                .Times(1)
                .WillOnce([this](FileRequest* request)
                    {
                        AZ_Assert(m_streamerContext, "AZ::IO::Streamer is not ready to process requests.");
                        auto readData = AZStd::get_if<Requests::ReadData>(&request->GetCommand());
                        AZ_Assert(readData, "Test didn't pass in the correct request.");
                        auto output = reinterpret_cast<uint8_t*>(readData->m_output);
                        AZ_Assert(output != nullptr, "Output buffer has not been set.");
                        for (size_t i = 0; i < readData->m_size; ++i)
                        {
                            output[i] = azlossy_cast<uint8_t>(readData->m_offset + i);
                        }
                        request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                        m_streamerContext->MarkRequestAsCompleted(request);
                    });
        }

        void MockAllocatorForUnclaimedMemory(IStreamerTypes::RequestMemoryAllocatorMock& mock, AZStd::binary_semaphore& sync)
        {
            using ::testing::_;

            EXPECT_CALL(mock, LockAllocator()).Times(1);
            EXPECT_CALL(mock, UnlockAllocator())
                .Times(1)
                .WillOnce([&sync]()
                    {
                        sync.release();
                    });
            EXPECT_CALL(mock, Allocate(_, _, _))
                .Times(1)
                .WillOnce([&mock](size_t minimalSize, size_t recommendedSize, size_t alignment)
                    {
                        return mock.ForwardAllocate(minimalSize, recommendedSize, alignment);
                    });
            EXPECT_CALL(mock, Release(_))
                .Times(1)
                .WillOnce([&mock](void* address)
                    {
                        mock.ForwardRelease(address);
                    });
        }

        void MockAllocatorForClaimedMemory(IStreamerTypes::RequestMemoryAllocatorMock& mock)
        {
            using ::testing::_;

            EXPECT_CALL(mock, LockAllocator()).Times(1);
            EXPECT_CALL(mock, UnlockAllocator()).Times(1);
            EXPECT_CALL(mock, Allocate(_, _, _))
                .Times(1)
                .WillOnce([&mock](size_t minimalSize, size_t recommendedSize, size_t alignment)
                    {
                        return mock.ForwardAllocate(minimalSize, recommendedSize, alignment);
                    });
            EXPECT_CALL(mock, Release(_)).Times(0);
        }

    protected:
        // Using Streamer to interact with the Scheduler as not all functionality
        // is publicly exposed. Since Streamer is mostly the threaded front end for
        // the Scheduler, this is fine.
        Streamer* m_streamer{ nullptr };
        AZStd::shared_ptr<testing::NiceMock<StreamStackEntryMock>> m_mock;
        AZStd::atomic_bool m_isStackIdle = false;
    };

    TEST_F(Streamer_SchedulerTest, QueueNextRequest_QueueUnclaimedFireAndForgetReadWithAllocator_AllocatorCalledAndMemoryFreedAgain)
    {
        using ::testing::_;
        using ::testing::AtLeast;
        using ::testing::Return;

        MockForRead();

        AZStd::binary_semaphore allocatorSync;
        IStreamerTypes::RequestMemoryAllocatorMock allocatorMock;
        MockAllocatorForUnclaimedMemory(allocatorMock, allocatorSync);

        AZStd::binary_semaphore readSync;
        auto wait = [&readSync](FileRequestHandle)
        {
            readSync.release();
        };

        // Scoped to simulate a fire-and-forget request which should cause the request to be deleted after
        // completion and free the allocated memory.
        {
            FileRequestPtr read = m_streamer->Read("TestPath", allocatorMock, 8);
            m_streamer->SetRequestCompleteCallback(read, wait);
            m_streamer->QueueRequest(read);
        }

        ASSERT_TRUE(readSync.try_acquire_for(AZStd::chrono::seconds(5)));
        ASSERT_TRUE(allocatorSync.try_acquire_for(AZStd::chrono::seconds(5)));
    }

    TEST_F(Streamer_SchedulerTest, QueueNextRequest_QueueUnclaimedReadWithAllocator_AllocatorCalledAndMemoryFreedAgain)
    {
        using ::testing::_;
        using ::testing::AtLeast;
        using ::testing::Return;

        MockForRead();

        AZStd::binary_semaphore allocatorSync;
        IStreamerTypes::RequestMemoryAllocatorMock allocatorMock;
        MockAllocatorForUnclaimedMemory(allocatorMock, allocatorSync);

        AZStd::binary_semaphore readSync;
        auto wait = [&readSync](FileRequestHandle)
        {
            readSync.release();
        };

        // Scoped so the request goes out to scope before ending the test. This should trigger the
        // memory release on this thread.
        {
            FileRequestPtr read = m_streamer->Read("TestPath", allocatorMock, 8);
            m_streamer->SetRequestCompleteCallback(read, wait);
            m_streamer->QueueRequest(read);
            ASSERT_TRUE(readSync.try_acquire_for(AZStd::chrono::seconds(5)));
        }

        ASSERT_TRUE(allocatorSync.try_acquire_for(AZStd::chrono::seconds(5)));
    }

    TEST_F(Streamer_SchedulerTest, QueueNextRequest_QueueClaimedReadWithAllocator_AllocatorCalledAndMemoryFreedAgain)
    {
        using ::testing::_;
        using ::testing::AtLeast;
        using ::testing::Return;

        MockForRead();

        IStreamerTypes::RequestMemoryAllocatorMock allocatorMock;
        MockAllocatorForClaimedMemory(allocatorMock);

        AZStd::binary_semaphore readSync;
        auto wait = [&readSync](FileRequestHandle)
        {
            readSync.release();
        };

        FileRequestPtr read = m_streamer->Read("TestPath", allocatorMock, 8);
        m_streamer->SetRequestCompleteCallback(read, wait);
        m_streamer->QueueRequest(read);
        ASSERT_TRUE(readSync.try_acquire_for(AZStd::chrono::seconds(5)));

        void* buffer = nullptr;
        u64 readSize = 0;
        EXPECT_TRUE(m_streamer->GetReadRequestResult(read, buffer, readSize, IStreamerTypes::ClaimMemory::Yes));
        ASSERT_NE(nullptr, buffer);

        allocatorMock.ForwardRelease(buffer);
    }

    TEST_F(Streamer_SchedulerTest, ProcessCancelRequest_CancelReadRequest_MockDoesNotReceiveReadRequest)
    {
        using ::testing::_;
        using ::testing::AtLeast;
        using ::testing::Return;

        EXPECT_CALL(*m_mock, UpdateStatus(_)).Times(AtLeast(1));
        EXPECT_CALL(*m_mock, UpdateCompletionEstimates(_, _, _, _)).Times(AtLeast(1));
        EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(AtLeast(1));
        EXPECT_CALL(*m_mock, ExecuteRequests()).Times(AtLeast(1));
        EXPECT_CALL(*m_mock, QueueRequest(_)).Times(1);

        AZStd::atomic_int counter = 2;
        AZStd::binary_semaphore sync;
        auto wait = [&sync, &counter](FileRequestHandle)
        {
            if (--counter == 0)
            {
                sync.release();
            }
        };

        char fakeBuffer[8];
        FileRequestPtr read = m_streamer->Read("TestPath", fakeBuffer, sizeof(fakeBuffer), 8);
        FileRequestPtr cancel = m_streamer->Cancel(read);

        m_streamer->SetRequestCompleteCallback(read, wait);
        m_streamer->SetRequestCompleteCallback(cancel, wait);

        m_streamer->SuspendProcessing();
        m_streamer->QueueRequest(read);
        m_streamer->QueueRequest(cancel);
        m_streamer->ResumeProcessing();

        ASSERT_TRUE(sync.try_acquire_for(AZStd::chrono::seconds(5)));
        EXPECT_EQ(IStreamerTypes::RequestStatus::Completed, m_streamer->GetRequestStatus(cancel));
        EXPECT_EQ(IStreamerTypes::RequestStatus::Canceled, m_streamer->GetRequestStatus(read));
    }

    TEST_F(Streamer_SchedulerTest, Reschedule_SetNewDeadlineAndPriority_ReadRequestInMockHasUpdatedTime)
    {
        using ::testing::_;
        using ::testing::AtLeast;
        using ::testing::Invoke;
        using ::testing::Return;

        EXPECT_CALL(*m_mock, UpdateStatus(_)).Times(AtLeast(1));
        EXPECT_CALL(*m_mock, UpdateCompletionEstimates(_, _, _, _)).Times(AtLeast(1));
        EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(AtLeast(1));
        EXPECT_CALL(*m_mock, ExecuteRequests()).Times(AtLeast(1));
        EXPECT_CALL(*m_mock, QueueRequest(_)).Times(1)
            .WillOnce(Invoke([this](FileRequest* request)
            {
                auto* read = request->GetCommandFromChain<Requests::ReadRequestData>();
                ASSERT_NE(nullptr, read);
                EXPECT_LT(read->m_deadline, FileRequest::s_noDeadlineTime);
                EXPECT_EQ(read->m_priority, IStreamerTypes::s_priorityHighest);
                m_mock->ForwardQueueRequest(request);
            }));

        AZStd::atomic_int counter = 2;
        AZStd::binary_semaphore sync;
        auto wait = [&sync, &counter](FileRequestHandle)
        {
            if (--counter == 0)
            {
                sync.release();
            }
        };

        char fakeBuffer[8];
        FileRequestPtr read = m_streamer->Read("TestPath", fakeBuffer, sizeof(fakeBuffer), 8,
            IStreamerTypes::s_noDeadline, IStreamerTypes::s_priorityMedium);
        FileRequestPtr reschedule = m_streamer->RescheduleRequest(read, IStreamerTypes::s_deadlineNow, IStreamerTypes::s_priorityHighest);

        m_streamer->SetRequestCompleteCallback(read, wait);
        m_streamer->SetRequestCompleteCallback(reschedule, wait);

        m_streamer->SuspendProcessing();
        m_streamer->QueueRequest(read);
        m_streamer->QueueRequest(reschedule);
        m_streamer->ResumeProcessing();

        ASSERT_TRUE(sync.try_acquire_for(AZStd::chrono::seconds(5)));
        EXPECT_EQ(IStreamerTypes::RequestStatus::Completed, m_streamer->GetRequestStatus(reschedule));
    }

    TEST_F(Streamer_SchedulerTest, ProcessTillIdle_ShutDownIsDelayedUntilIdle_SchedulerThreadDoesNotImmediatelyShutDown)
    {
        using::testing::_;
        using ::testing::AnyNumber;
        using ::testing::Invoke;

        constexpr static size_t Iterations = 16;

        AZStd::atomic_int counter{ 0 };

        EXPECT_CALL(*m_mock, UpdateStatus(_)).Times(AnyNumber());
        EXPECT_CALL(*m_mock, UpdateCompletionEstimates(_, _, _, _)).Times(AnyNumber());

        // Pretend to be busy [Iterations] times, then set the status to idle so the Scheduler thread can exit.
        EXPECT_CALL(*m_mock, ExecuteRequests())
            .Times(Iterations + 1)
            .WillRepeatedly(Invoke([this, &counter]()
            {
                if (counter++ >= Iterations)
                {
                    m_isStackIdle = true;
                    return false;
                }
                else
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
                    return true;
                }
            }));

        if (m_streamer)
        {
            Interface<IO::IStreamer>::Unregister(m_streamer);
            delete m_streamer;
            m_streamer = nullptr;
        }

        EXPECT_EQ(Iterations + 1, counter);
    }

    TEST_F(Streamer_SchedulerTest, RequestSorting)
    {
        //////////////////////////////////////////////////////////////
        // Test equal priority requests that are past their deadlines (aka panic)
        //////////////////////////////////////////////////////////////
        IStreamerTypes::Deadline panicDeadline(IStreamerTypes::Deadline::min());
        auto estimatedCompleteTime = AZStd::chrono::steady_clock::now();
        char fakeBuffer[8];
        FileRequestPtr panicRequest = m_streamer->Read("PanicRequest", fakeBuffer, sizeof(fakeBuffer), 8, panicDeadline);
        panicRequest->m_request.SetEstimatedCompletion(estimatedCompleteTime);

        // Passed deadline, same object (same pointer)
        EXPECT_EQ(
            m_streamer->m_streamStack->Thread_PrioritizeRequests(&panicRequest->m_request, &panicRequest->m_request),
            Scheduler::Order::Equal);

        // Passed deadline, different object
        FileRequestPtr panicRequest2 = m_streamer->Read("PanicRequest2", fakeBuffer, sizeof(fakeBuffer), 8, panicDeadline);
        panicRequest2->m_request.SetEstimatedCompletion(estimatedCompleteTime);
        EXPECT_EQ(
            m_streamer->m_streamStack->Thread_PrioritizeRequests(&panicRequest->m_request, &panicRequest2->m_request),
            Scheduler::Order::Equal);


        //////////////////////////////////////////////////////////////
        // Test equal priority requests that are both reading the same file
        //////////////////////////////////////////////////////////////
        RequestPath emptyPath;
        FileRequestPtr readRequest = m_streamer->Read("SameFile", fakeBuffer, sizeof(fakeBuffer), 8, panicDeadline);
        FileRequestPtr sameFileRequest = m_streamer->CreateRequest();
        sameFileRequest->m_request.CreateRead(&sameFileRequest->m_request, fakeBuffer, 8, emptyPath, 0, 8);
        sameFileRequest->m_request.m_parent = &readRequest->m_request;
        sameFileRequest->m_request.m_dependencies = 0;

        // Same file read, same object (same pointer)
        EXPECT_EQ(
            m_streamer->m_streamStack->Thread_PrioritizeRequests(&sameFileRequest->m_request, &sameFileRequest->m_request),
            Scheduler::Order::Equal);

        FileRequestPtr readRequest2 = m_streamer->Read("SameFile2", fakeBuffer, sizeof(fakeBuffer), 8, panicDeadline);
        FileRequestPtr sameFileRequest2 = m_streamer->CreateRequest();
        sameFileRequest2->m_request.CreateRead(&sameFileRequest2->m_request, fakeBuffer, 8, emptyPath, 0, 8);
        sameFileRequest2->m_request.m_parent = &readRequest2->m_request;
        sameFileRequest2->m_request.m_dependencies = 0;

        // Same file read, different objects
        EXPECT_EQ(
            m_streamer->m_streamStack->Thread_PrioritizeRequests(&sameFileRequest->m_request, &sameFileRequest2->m_request),
            Scheduler::Order::Equal);
    }
} // namespace AZ::IO
