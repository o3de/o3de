/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/EBusSharedDispatchTraits.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/AZTestShared/Utils/Utils.h>

#include <gtest/gtest.h>

namespace UnitTest
{
    // Test EBus that uses the EBusSharedDispatchMutex.
    class SharedDispatchRequests : public AZ::EBusSharedDispatchTraits<SharedDispatchRequests>
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        // Custom disconnect policy is used here to verify that disconnects do not occur while dispatches are in progress.
        template<class Bus>
        struct ConnectionPolicy : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Disconnect(
                typename Bus::Context& context,
                typename Bus::HandlerNode& handler,
                typename Bus::BusPtr& busPtr)
            {
                EXPECT_EQ(m_totalRecursiveQueriesInProgress, 0);
                AZ::EBusConnectionPolicy<Bus>::Disconnect(context, handler, busPtr);
            }
        };

        // Provide a test EBus call that can be run in parallel.
        virtual void RecursiveQuery(int32_t numRecursions = 5) = 0;

        // These are static and defined on the EBus so that we can check the values from Disconnect.
        static AZStd::atomic_int m_totalRecursiveQueriesInProgress;
        static AZStd::atomic_int m_totalRecursiveQueriesCompleted;
    };
    using SharedDispatchRequestBus = AZ::EBus<SharedDispatchRequests>;

    AZStd::atomic_int SharedDispatchRequests::m_totalRecursiveQueriesInProgress = 0;
    AZStd::atomic_int SharedDispatchRequests::m_totalRecursiveQueriesCompleted = 0;

    // Test EBus handler that provides recursion and synchronization to test out the features of the EBusSharedDispatchMutex.
    class SharedDispatchRequestHandler : public SharedDispatchRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SharedDispatchRequestHandler, AZ::SystemAllocator);

        AZStd::semaphore m_querySemaphore; 
        AZStd::semaphore m_syncSemaphore;
        AZStd::semaphore m_disconnectSemaphore;

        AZStd::atomic_int m_numDisconnects = 0;

        SharedDispatchRequestHandler()
        {
            // Reinitialize these for every test.
            m_totalRecursiveQueriesInProgress = 0;
            m_totalRecursiveQueriesCompleted = 0;
        }

        ~SharedDispatchRequestHandler() override
        {
            SharedDispatchRequestBus::Handler::BusDisconnect();
        }

        void Connect()
        {
            SharedDispatchRequestBus::Handler::BusConnect();
        }

        void Disconnect()
        {
            // Signal that the thread is running and has at least made it this far.
            m_disconnectSemaphore.release();

            SharedDispatchRequestBus::Handler::BusDisconnect();
            m_numDisconnects++;
        }

        void RecursiveQuery(int32_t numRecursions = 5) override
        {
            if (numRecursions <= 0)
            {
                // At the end of the recursion, signal the syncSemaphore that we've reached the end of the recursion.
                // We'll use this as a way to guarantee that all our threads have reached this point at the same time.
                m_syncSemaphore.release();

                // Block on the querySemaphore. This won't get released until every thread has released the syncSemaphore.
                m_querySemaphore.acquire();

                // Track that we've completed the query successfully.
                m_totalRecursiveQueriesCompleted++;
                return;
            }

            // Recursively call the EBus a fixed number of times, and keep track of how many times we've successfully recursed.
            m_totalRecursiveQueriesInProgress++;
            SharedDispatchRequestBus::Broadcast(&SharedDispatchRequestBus::Events::RecursiveQuery, numRecursions - 1);
            m_totalRecursiveQueriesInProgress--;
        }
    };

    class EBusSharedDispatchMutexTestFixture
        : public LeakDetectionFixture
    {
    public:

        EBusSharedDispatchMutexTestFixture()
        {
            SharedDispatchRequestBus::GetOrCreateContext();
        }
    };

    TEST_F(EBusSharedDispatchMutexTestFixture, RecursiveBusCallsOnSingleThreadWorks)
    {
        // Verify that multiple nested bus calls to the same bus on the same thread works without deadlocks.

        constexpr int32_t TotalRecursiveQueries = 10;
        SharedDispatchRequestHandler handler;
        handler.Connect();

        // This is a single-threaded test, so we don't need the recursive query to block before returning.
        handler.m_querySemaphore.release();

        SharedDispatchRequestBus::Broadcast(&SharedDispatchRequestBus::Events::RecursiveQuery, TotalRecursiveQueries);
        EXPECT_EQ(handler.m_totalRecursiveQueriesInProgress, 0);
        EXPECT_EQ(handler.m_totalRecursiveQueriesCompleted, 1);

        // Not strictly needed, but since we're doing a release() in RecursiveQuery, this keeps the semaphore acquire/release calls
        // balanced for the test.
        handler.m_syncSemaphore.acquire();

        handler.Disconnect();
    }

    TEST_F(EBusSharedDispatchMutexTestFixture, RecursiveBusCallsOnMultipleThreadsWork)
    {
        // Verify that multiple dispatched events run in parallel without deadlocks, even if each thread has recursively called
        // events on the same bus.

        const int32_t TotalRecursiveQueries = 10;
        SharedDispatchRequestHandler handler;
        handler.Connect();

        constexpr size_t ThreadCount = 4;
        AZStd::thread threads[ThreadCount];

        // Each thread will trigger the RecursiveQuery call. This call has semaphores in it so that we can guarantee that
        // every thread has reached the same state at the same time.
        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(
                [TotalRecursiveQueries]()
                {
                    SharedDispatchRequestBus::Broadcast(&SharedDispatchRequestBus::Events::RecursiveQuery, TotalRecursiveQueries);
                });
        }

        // Wait for all the threads to reach the point where they're blocking. This will occur once they've each successfully called
        // down through the RecursiveQuery multiple times and are ready to finish.
        for (size_t threadNum = 0; threadNum < ThreadCount; threadNum++)
        {
            handler.m_syncSemaphore.acquire();
        }

        // Before unblocking the threads, verify that we've got the total number of expected recursions in progress
        // and that none of the calls have completed.
        EXPECT_EQ(handler.m_totalRecursiveQueriesInProgress, TotalRecursiveQueries * ThreadCount);
        EXPECT_EQ(handler.m_totalRecursiveQueriesCompleted, 0);

        // Unblock all the threads.
        for (size_t threadNum = 0; threadNum < ThreadCount; threadNum++)
        {
            handler.m_querySemaphore.release();
        }

        // Wait for the threads to finish.
        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }

        // Verify that we ended up with the correct number of completed recursive calls and that none are still in progress.
        EXPECT_EQ(handler.m_totalRecursiveQueriesInProgress, 0);
        EXPECT_EQ(handler.m_totalRecursiveQueriesCompleted, ThreadCount);

        handler.Disconnect();
    }

    TEST_F(EBusSharedDispatchMutexTestFixture, DispatchCallsBlockDisconnectFromRunning)
    {
        // Verify that BusConnect / BusDisconnect cannot run in parallel with event dispatches.
        // We can't easily test BusConnect running in parallel, because by definition no dispatches can successfully occur before
        // the handler is connected. However, we can test Disconnect by doing the following:
        // - Run multiple dispatches in parallel and block them mid-dispatch
        // - Run Disconnect() on a thread
        // - Unblock the dispatches
        // - Wait for the dispatches and disconnect to complete.
        // The Disconnect() logic will verify that the number of running dispatches is 0. If the dispatches successfully blocked the
        // disconnect, the Disconnect() won't be able to execute until all the dispatches have completed. If they don't block the
        // disconnect, then there will be dispatches running at the same time and the verification will fail.

        const int32_t TotalRecursiveQueries = 5;
        SharedDispatchRequestHandler handler;
        handler.Connect();

        constexpr size_t ThreadCount = 4;
        AZStd::thread threads[ThreadCount];
        AZStd::thread disconnectThread;

        // Each thread will trigger the RecursiveQuery call. This call has semaphores in it so that we can guarantee that
        // every thread has reached the same state at the same time.
        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(
                [TotalRecursiveQueries]()
                {
                    SharedDispatchRequestBus::Broadcast(&SharedDispatchRequestBus::Events::RecursiveQuery, TotalRecursiveQueries);
                });
        }

        // Wait for all the threads to reach the point where they're blocking. This will occur once they've each successfully called
        // down through the RecursiveQuery multiple times and are ready to finish.
        for (size_t threadNum = 0; threadNum < ThreadCount; threadNum++)
        {
            handler.m_syncSemaphore.acquire();
        }

        disconnectThread = AZStd::thread(
            [&handler]()
            {
                handler.Disconnect();
            }
        );

        // Wait for the disconnect thread to start running. At this point, no disconnects should have occurred, because it's blocked
        // waiting on the dispatches to finish.
        handler.m_disconnectSemaphore.acquire();
        EXPECT_EQ(handler.m_numDisconnects, 0);

        // Unblock all the dispatch threads.
        for (size_t threadNum = 0; threadNum < ThreadCount; threadNum++)
        {
            handler.m_querySemaphore.release();
        }

        // Wait for the dispatch threads to finish.
        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }

        // Wait for the disconnect thread to finish.
        disconnectThread.join();

        // Verify that the disconnect finished. Our disconnect logic will verify that no dispatches were running during the disconnect.
        EXPECT_EQ(handler.m_numDisconnects, 1);
    }

} // namespace UnitTest

