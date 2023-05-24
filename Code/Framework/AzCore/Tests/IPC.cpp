/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IPC/SharedMemory.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>

using namespace AZ;

#if AZ_TRAIT_SUPPORT_IPC

namespace UnitTest
{
    /**
     * Test InterProcessCommunication utils.
     */
    class IPC
        : public LeakDetectionFixture
    {
    };

    TEST_F(IPC, SharedMemoryThrash)
    {
        AZ::SharedMemory sharedMem;
        AZ::SharedMemory::CreateResult result = sharedMem.Create("SharedMemoryUnitTest", sizeof(AZStd::thread_id), true);
        EXPECT_EQ(AZ::SharedMemory::CreatedNew, result);
        bool mapped = sharedMem.Map();
        EXPECT_TRUE(mapped);

        auto threadFunc = [&sharedMem]()
        {
            AZStd::thread_id* lastThreadId = static_cast<AZStd::thread_id*>(sharedMem.Data());

            for (int i = 0; i < 100000; ++i)
            {
                AZStd::lock_guard<decltype(sharedMem)> lock(sharedMem);
                *lastThreadId = AZStd::this_thread::get_id();
            }
        };

        AZStd::thread threads[8];
        for (size_t t = 0; t < AZ_ARRAY_SIZE(threads); ++t)
        {
            threads[t] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        bool unmapped = sharedMem.UnMap();
        EXPECT_TRUE(unmapped);
    }

    TEST_F(IPC, SharedMemoryMutexAbandonment)
    {
        const char* sharedMemKey = "SharedMemoryUnitTest";

        auto threadFunc = [sharedMemKey]()
        {
            void* memBuffer = azmalloc(sizeof(AZ::SharedMemory));
            AZ::SharedMemory* sharedMem = new (memBuffer) AZ::SharedMemory();
            AZ::SharedMemory::CreateResult result = sharedMem->Create(sharedMemKey, sizeof(AZStd::thread_id), true);
            EXPECT_TRUE(result == AZ::SharedMemory::CreatedNew || result == AZ::SharedMemory::CreatedExisting);
            bool mapped = sharedMem->Map();
            EXPECT_TRUE(mapped);

            AZStd::thread_id* lastThreadId = static_cast<AZStd::thread_id*>(sharedMem->Data());

            for (int i = 0; i < 100000; ++i)
            {
                sharedMem->lock();
                *lastThreadId = AZStd::this_thread::get_id();
                if (i == (int)(*lastThreadId).m_id || sharedMem->IsLockAbandoned())
                {
                    // Simulate a crash/dead process by cleaning up the handles but not unlocking
                    HANDLE* mmapHandle = (HANDLE*)(reinterpret_cast<char*>(sharedMem) + sizeof(SharedMemory_Common));
                    HANDLE* mutexHandle = mmapHandle + 1;
                    bool closedMap = CloseHandle(*mmapHandle) != 0;
                    EXPECT_TRUE(closedMap);
                    bool closedMutex = CloseHandle(*mutexHandle) != 0;
                    EXPECT_TRUE(closedMutex); 
                    azfree(sharedMem); // avoid calling dtor
                    return;
                }
                sharedMem->unlock();
            }

            bool unmapped = sharedMem->UnMap();
            EXPECT_TRUE(unmapped);

            sharedMem->~SharedMemory();
            azfree(memBuffer);
        };

        AZStd::thread threads[8];
        for (size_t t = 0; t < AZ_ARRAY_SIZE(threads); ++t)
        {
            threads[t] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }
}

#endif // AZ_TRAIT_SUPPORT_IPC


