/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/BestFitExternalMapAllocator.h>
#include <AzCore/Memory/HeapSchema.h>
#include <AzCore/Memory/HphaSchema.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/containers/lock_free_intrusive_stamped_stack.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>

#include <AzCore/std/containers/intrusive_slist.h>
#include <AzCore/std/containers/intrusive_list.h>

#include <AzCore/std/chrono/clocks.h>

#include <AzCore/std/functional.h>



using namespace AZ;
using namespace AZ::Debug;

namespace UnitTest
{
    class MemoryTrackingFixture
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::RECORD_FULL);
            m_drillerManager = Debug::DrillerManager::Create();
            m_drillerManager->Register(aznew MemoryDriller);
        }
        void TearDown() override
        {
            Debug::DrillerManager::Destroy(m_drillerManager);
            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::RECORD_NO_RECORDS);
        }
    protected:
        Debug::DrillerManager*      m_drillerManager = nullptr;
    };

    class SystemAllocatorTest
        : public MemoryTrackingFixture
    {
        static const int            m_threadStackSize = 128 * 1024;
        static const unsigned int   m_maxNumThreads = 10;
        AZStd::thread_desc          m_desc[m_maxNumThreads];

    public:
        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();

            for (unsigned int i = 0; i < m_maxNumThreads; ++i)
            {
                // Set the threads stack size
                m_desc[i].m_stackSize = m_threadStackSize;

                // Allocate stacks for the platforms that can't do it themself.
            }
        }

        void TearDown() override
        {
            // Free allocated stacks.
            MemoryTrackingFixture::TearDown();
        }

        void ThreadFunc()
        {
#ifdef _DEBUG
            static const int numAllocations = 100;
#else
            static const int numAllocations = 10000;
#endif
            void* addresses[numAllocations] = {nullptr};

            IAllocatorAllocate& sysAlloc = AllocatorInstance<SystemAllocator>::Get();

            //////////////////////////////////////////////////////////////////////////
            // Allocate
            AZStd::size_t totalAllocSize = 0;
            for (int i = 0; i < numAllocations; ++i)
            {
                AZStd::size_t size = AZStd::GetMax(rand() % 256, 1);
                // supply all debug info, so we don't need to record the stack.
                addresses[i] = sysAlloc.Allocate(size, 8, 0, "Test Alloc", __FILE__, __LINE__);
                memset(addresses[i], 1, size);
                totalAllocSize += size;
            }
            //////////////////////////////////////////////////////////////////////////

            EXPECT_GE(sysAlloc.NumAllocatedBytes(), totalAllocSize);

            //////////////////////////////////////////////////////////////////////////
            // Deallocate
            for (int i = numAllocations-1; i >=0; --i)
            {
                sysAlloc.DeAllocate(addresses[i]);
            }
            //////////////////////////////////////////////////////////////////////////
        }

        void run()
        {
            void* address[100];
#if defined(AZ_PLATFORM_WINDOWS)
            // On windows we don't require to preallocate memory to function.
            // On most consoles we do!
            {
                AllocatorInstance<SystemAllocator>::Create();

                IAllocatorAllocate& sysAlloc = AllocatorInstance<SystemAllocator>::Get();

                for (int i = 0; i < 100; ++i)
                {
                    address[i] = sysAlloc.Allocate(1000, 32, 0);
                    EXPECT_NE(nullptr, address[i]);
                    EXPECT_EQ(0, ((size_t)address[i] & 31)); // check alignment
                    EXPECT_GE(sysAlloc.AllocationSize(address[i]), 1000); // check allocation size
                }

                EXPECT_GE(sysAlloc.NumAllocatedBytes(), 100000); // we requested 100 * 1000 so we should have at least this much allocated

                for (int i = 0; i < 100; ++i)
                {
                    sysAlloc.DeAllocate(address[i]);
                }

                ////////////////////////////////////////////////////////////////////////
                // Create some threads and simulate concurrent allocation and deallocation
                AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
                {
                    AZStd::thread m_threads[m_maxNumThreads];
                    for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                    {
                        m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&SystemAllocatorTest::ThreadFunc, this));
                        // give some time offset to the threads so we can test alloc and dealloc at the same time.
                        //AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(500));
                    }

                    for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                    {
                        m_threads[i].join();
                    }
                }
                //AZStd::chrono::microseconds exTime = AZStd::chrono::system_clock::now() - startTime;
                //AZ_Printf("UnitTest::SystemAllocatorTest::deafult","Time: %d Ms\n",exTime.count());
                //////////////////////////////////////////////////////////////////////////

                AllocatorInstance<SystemAllocator>::Destroy();
            }
#endif
            memset(address, 0, AZ_ARRAY_SIZE(address) * sizeof(void*));

            SystemAllocator::Descriptor descriptor;
            descriptor.m_stackRecordLevels = 20;
            AllocatorInstance<SystemAllocator>::Create(descriptor);
            IAllocator& sysAllocator = AllocatorInstance<SystemAllocator>::GetAllocator();
            IAllocatorAllocate& sysAlloc = *sysAllocator.GetAllocationSource();

            for (int i = 0; i < 100; ++i)
            {
                address[i] = sysAlloc.Allocate(1000, 32, 0);
                EXPECT_NE(nullptr, address[i]);
                EXPECT_EQ(0, ((size_t)address[i] & 31)); // check alignment
                EXPECT_GE(sysAlloc.AllocationSize(address[i]), 1000); // check allocation size
            }

            EXPECT_TRUE(sysAlloc.NumAllocatedBytes() >= 100000); // we requested 100 * 1000 so we should have at least this much allocated

// If tracking and recording is enabled, we can verify that the alloc info is valid
#if defined(AZ_DEBUG_BUILD)
            sysAllocator.GetRecords()->lock();
            EXPECT_TRUE(sysAllocator.GetRecords());
            const Debug::AllocationRecordsType& records = sysAllocator.GetRecords()->GetMap();
            EXPECT_EQ(100, records.size());
            for (Debug::AllocationRecordsType::const_iterator iter = records.begin(); iter != records.end(); ++iter)
            {
                const Debug::AllocationInfo& ai = iter->second;
                EXPECT_EQ(32, ai.m_alignment);
                EXPECT_EQ(1000, ai.m_byteSize);
                EXPECT_EQ(nullptr, ai.m_fileName); // We did not pass fileName or lineNum to sysAlloc.Allocate()
                EXPECT_EQ(0, ai.m_lineNum);  // -- " --
#   if defined(AZ_PLATFORM_WINDOWS)
                // if our hardware support stack traces make sure we have them, since we did not provide fileName,lineNum
                EXPECT_TRUE(ai.m_stackFrames[0].IsValid());  // We need to have at least one frame

                // For windows we should be able to decode the program counters into readable content.
                // This is possible on deprecated platforms too, but we would need to load the map file manually and so on... it's tricky.
                // Note: depending on where the tests are run from the call stack may differ.
                SymbolStorage::StackLine stackLine[20];
                SymbolStorage::DecodeFrames(ai.m_stackFrames, AZ_ARRAY_SIZE(stackLine), stackLine);
                bool found = false;
                int foundIndex = 0;  // After finding it for the first time, save the index so it can be reused

                for (int idx = foundIndex; idx < AZ_ARRAY_SIZE(stackLine); idx++)
                {
                    if (strstr(stackLine[idx], "SystemAllocatorTest::run"))
                    {
                        found = true;
                        break;
                    }
                    else
                    {
                        foundIndex++;
                    }
                }

                EXPECT_TRUE(found);
#   endif // defined(AZ_PLATFORM_WINDOWS)
            }

            sysAllocator.GetRecords()->unlock();
#endif //#if defined(AZ_DEBUG_BUILD)

            // Free all memory
            for (int i = 0; i < 100; ++i)
            {
                sysAlloc.DeAllocate(address[i]);
            }

            sysAlloc.GarbageCollect();
            EXPECT_LT(sysAlloc.NumAllocatedBytes(), 1024); // We freed everything from a memspace, we should have only a very minor chunk of data

            //////////////////////////////////////////////////////////////////////////
            // realloc test
            address[0] = nullptr;
            static const unsigned int checkValue = 0x0badbabe;
            // create tree (non pool) allocation (we usually pool < 256 bytes)
            address[0] = sysAlloc.Allocate(2048, 16);
            *(unsigned*)(address[0]) = checkValue; // set check value
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 2048, 16);
            address[0] = sysAlloc.ReAllocate(address[0], 1024, 16); // test tree big -> tree small
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 1024, 16);
            address[0] = sysAlloc.ReAllocate(address[0], 4096, 16); // test tree small -> tree big
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 4096, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = sysAlloc.ReAllocate(address[0], 128, 16); // test tree -> pool,
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 128, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = sysAlloc.ReAllocate(address[0], 64, 16); // pool big -> pool small
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 64, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = sysAlloc.ReAllocate(address[0], 64, 16); // pool sanity check
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 64, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = sysAlloc.ReAllocate(address[0], 192, 16); // pool small -> pool big
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 192, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = sysAlloc.ReAllocate(address[0], 2048, 16); // pool -> tree
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 2048, 16);
            ;
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = sysAlloc.ReAllocate(address[0], 2048, 16); // tree sanity check
            AZ_TEST_ASSERT_CLOSE(sysAlloc.AllocationSize(address[0]), 2048, 16);
            ;
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            sysAlloc.DeAllocate(address[0], 2048, 16);
            // TODO realloc with different alignment tests
            //////////////////////////////////////////////////////////////////////////

            // run some thread allocations.
            //////////////////////////////////////////////////////////////////////////
            // Create some threads and simulate concurrent allocation and deallocation
            //AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
            {
                AZStd::thread m_threads[m_maxNumThreads];
                for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                {
                    m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&SystemAllocatorTest::ThreadFunc, this));
                    // give some time offset to the threads so we can test alloc and dealloc at the same time.
                    AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(500));
                }

                for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                {
                    m_threads[i].join();
                }
            }
            //AZStd::chrono::microseconds exTime = AZStd::chrono::system_clock::now() - startTime;
            //AZ_Printf("UnitTest::SystemAllocatorTest::mspaces","Time: %d Ms\n",exTime.count());
            //////////////////////////////////////////////////////////////////////////

            AllocatorInstance<SystemAllocator>::Destroy();
        }
    };

    TEST_F(SystemAllocatorTest, Test)
    {
        run();
    }

    class PoolAllocatorTest
        : public MemoryTrackingFixture
    {
    protected:
        SystemAllocator::Descriptor m_sysAllocDesc;
        bool m_isDynamic;
        int m_numStaticPages;
    public:
        PoolAllocatorTest(bool isDynamic = true, int numStaticPages = 0)
            : m_isDynamic(isDynamic)
            , m_numStaticPages(numStaticPages)
        {
        }

        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();

            m_sysAllocDesc.m_allocationRecords = true;
            AllocatorInstance<SystemAllocator>::Create(m_sysAllocDesc);
            PoolAllocator::Descriptor poolDesc;
            poolDesc.m_allocationRecords = true;
            poolDesc.m_isDynamic = m_isDynamic;
            poolDesc.m_numStaticPages = m_numStaticPages;
            AllocatorInstance<PoolAllocator>::Create(poolDesc);
        }

        void TearDown() override
        {
            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<SystemAllocator>::Destroy();
            MemoryTrackingFixture::TearDown();
        }

        void run()
        {
            IAllocatorAllocate& poolAlloc = AllocatorInstance<PoolAllocator>::Get();
            IAllocator& poolAllocator = AllocatorInstance<PoolAllocator>::GetAllocator();
            // 64 should be the max number of different pool sizes we can allocate.
            void* address[64];
            //////////////////////////////////////////////////////////////////////////
            // Allocate different pool sizes
            memset(address, 0, AZ_ARRAY_SIZE(address)*sizeof(void*));

            // try any size from 8 to 256 (which are supported pool sizes)
            int i = 0;
            for (int size = 8; size <= 256; ++i, size += 8)
            {
                address[i] = poolAlloc.Allocate(size, 8);
                EXPECT_GE(poolAlloc.AllocationSize(address[i]), (AZStd::size_t)size);
                memset(address[i], 1, size);
            }

            EXPECT_GE(poolAlloc.NumAllocatedBytes(), 4126);

            if (poolAllocator.GetRecords())
            {
                poolAllocator.GetRecords()->lock();
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_EQ(32, records.size());
                poolAllocator.GetRecords()->unlock();
            }

            for (i = 0; address[i] != nullptr; ++i)
            {
                poolAlloc.DeAllocate(address[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            EXPECT_EQ(0, poolAlloc.NumAllocatedBytes());

            if (poolAllocator.GetRecords())
            {
                poolAllocator.GetRecords()->lock();
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_EQ(0, records.size());
                poolAllocator.GetRecords()->unlock();
            }

            //////////////////////////////////////////////////////////////////////////
            // Allocate many elements from the same size

            //          AllocatorManager::MemoryBreak mb;
            //          mb.fileName = "This File";
            //          AllocatorManager::Instance().SetMemoryBreak(0,mb);

            memset(address, 0, AZ_ARRAY_SIZE(address)*sizeof(void*));
            for (unsigned int j = 0; j < AZ_ARRAY_SIZE(address); ++j)
            {
                address[j] = poolAlloc.Allocate(256, 8, 0, "Pool Alloc", "This File", 123);
                EXPECT_GE(poolAlloc.AllocationSize(address[j]), 256);
                memset(address[j], 1, 256);
            }
            //          AllocatorManager::Instance().ResetMemoryBreak(0);

            EXPECT_GE(poolAlloc.NumAllocatedBytes(), AZ_ARRAY_SIZE(address)*256);

            if (poolAllocator.GetRecords())
            {
                poolAllocator.GetRecords()->lock();
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_EQ(AZ_ARRAY_SIZE(address), records.size());
                poolAllocator.GetRecords()->unlock();
            }

            for (unsigned int j = 0; j < AZ_ARRAY_SIZE(address); ++j)
            {
                poolAlloc.DeAllocate(address[j]);
            }
            //////////////////////////////////////////////////////////////////////////

            EXPECT_EQ(0, poolAlloc.NumAllocatedBytes());

            if (poolAllocator.GetRecords())
            {
                EXPECT_NE(nullptr, poolAllocator.GetRecords());
                poolAllocator.GetRecords()->lock();
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_EQ(0, records.size());
                poolAllocator.GetRecords()->unlock();
            }
        }
    };

    TEST_F(PoolAllocatorTest, Test)
    {
        run();
    }

    class PoolAllocatorDynamicWithStaticPagesTest
        : public PoolAllocatorTest
    {
    public:
        PoolAllocatorDynamicWithStaticPagesTest()
            : PoolAllocatorTest(true, 10) {}                                      // just create 10 pages we will allocate more than
    };

    TEST_F(PoolAllocatorDynamicWithStaticPagesTest, Test)
    {
        run();
    }

    class PoolAllocatorStaticPagesTest
        : public PoolAllocatorTest
    {
    public:
        PoolAllocatorStaticPagesTest()
            : PoolAllocatorTest(false, 50) {}
    };

    TEST_F(PoolAllocatorStaticPagesTest, Test)
    {
        run();
    }

    /**
     * Tests ThreadPoolAllocator
     */
    class ThreadPoolAllocatorTest
        : public MemoryTrackingFixture
    {
        static const int            m_threadStackSize = 128 * 1024;
        static const unsigned int   m_maxNumThreads = 10;
        AZStd::thread_desc          m_desc[m_maxNumThreads];

        //\todo lock free
        struct AllocClass
            : public AZStd::intrusive_slist_node<AllocClass>
        {
            int m_data;
        };
        typedef AZStd::intrusive_slist<AllocClass, AZStd::slist_base_hook<AllocClass> > SharedAllocStackType;
        AZStd::mutex    m_mutex;

        SharedAllocStackType    m_sharedAlloc;
#ifdef _DEBUG
        static const int        m_numSharedAlloc = 100; ///< Number of shared alloc free.
#else
        static const int        m_numSharedAlloc = 10000; ///< Number of shared alloc free.
#endif
#if (ATOMIC_ADDRESS_LOCK_FREE==2)  // or we can use locked atomics
        AZStd::atomic_bool      m_doneSharedAlloc;
#else
        volatile bool           m_doneSharedAlloc;
#endif

        SystemAllocator::Descriptor m_sysAllocDesc;
        bool m_isDynamic;
        int m_numStaticPages;

    public:
        ThreadPoolAllocatorTest(bool isDynamic = true, int numStaticPages = 0)
            : m_isDynamic(isDynamic)
            , m_numStaticPages(numStaticPages)
        {
        }

        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();

            m_doneSharedAlloc = false;
#if defined(AZ_PLATFORM_WINDOWS)
            SetCriticalSectionSpinCount(m_mutex.native_handle(), 4000);
#endif

            for (unsigned int i = 0; i < m_maxNumThreads; ++i)
            {
                m_desc[i].m_stackSize = m_threadStackSize;
            }

            m_sysAllocDesc.m_allocationRecords = true;
            AllocatorInstance<SystemAllocator>::Create(m_sysAllocDesc);
            ThreadPoolAllocator::Descriptor poolDesc;
            poolDesc.m_allocationRecords = true;
            poolDesc.m_isDynamic = m_isDynamic;
            poolDesc.m_numStaticPages = m_numStaticPages;
            AllocatorInstance<ThreadPoolAllocator>::Create(poolDesc);
        }

        void TearDown() override
        {
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<SystemAllocator>::Destroy();

            MemoryTrackingFixture::TearDown();
        }

        void AllocDeallocFunc()
        {
#ifdef _DEBUG
            static const int numAllocations = 100;
#else
            static const int numAllocations = 10000;
#endif
            void* addresses[numAllocations] = {nullptr};

            IAllocatorAllocate& poolAlloc = AllocatorInstance<ThreadPoolAllocator>::Get();

            //////////////////////////////////////////////////////////////////////////
            // Allocate
            for (int i = 0; i < numAllocations; ++i)
            {
                AZStd::size_t size = AZStd::GetMax(1, ((i + 1) * 2) % 256);
                addresses[i] = poolAlloc.Allocate(size, 8, 0, "Test Alloc", __FILE__, __LINE__);
                EXPECT_NE(addresses[i], nullptr);
                memset(addresses[i], 1, size);
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Deallocate
            for (int i = numAllocations-1; i >=0; --i)
            {
                poolAlloc.DeAllocate(addresses[i]);
            }
            //////////////////////////////////////////////////////////////////////////
        }

        /**
         * Function that does allocations and pushes them on a lock free stack
         */
        void SharedAlloc()
        {
            IAllocatorAllocate& poolAlloc = AllocatorInstance<ThreadPoolAllocator>::Get();
            for (int i = 0; i < m_numSharedAlloc; ++i)
            {
                AZStd::size_t minSize = sizeof(AllocClass);
                AZStd::size_t size = AZStd::GetMax((AZStd::size_t)(rand() % 256), minSize);
                AllocClass* ac = reinterpret_cast<AllocClass*>(poolAlloc.Allocate(size, AZStd::alignment_of<AllocClass>::value, 0, "Shared Alloc", __FILE__, __LINE__));
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                m_sharedAlloc.push_back(*ac);
            }
        }

        /**
        * Function that does deallocations from the lock free stack
        */
        void SharedDeAlloc()
        {
            IAllocatorAllocate& poolAlloc = AllocatorInstance<ThreadPoolAllocator>::Get();
            AllocClass* ac;
            int isDone = 0;
            while (isDone!=2)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                while (!m_sharedAlloc.empty())
                {
                    ac = &m_sharedAlloc.front();
                    m_sharedAlloc.pop_front();
                    poolAlloc.DeAllocate(ac);
                }

                if (m_doneSharedAlloc) // once we know we don't add more elements, make one last check and exit.
                {
                    ++isDone;
                }
            }
        }

        class MyThreadPoolAllocator
            : public ThreadPoolBase<MyThreadPoolAllocator>
        {
        public:
            AZ_CLASS_ALLOCATOR(MyThreadPoolAllocator, SystemAllocator, 0);
            AZ_TYPE_INFO(MyThreadPoolAllocator, "{28D80F96-19B1-4465-8278-B53989C44CF1}");

            struct Descriptor
                : public ThreadPoolBase<MyThreadPoolAllocator>::Descriptor
            {
                Descriptor()
                    : ThreadPoolBase<MyThreadPoolAllocator>::Descriptor()
                {
                    m_pageSize = 64 * 1024;
                    m_minAllocationSize = 1024;
                    m_maxAllocationSize = 1024;
                }
            };

            using Base = ThreadPoolBase<MyThreadPoolAllocator>;

            MyThreadPoolAllocator()
                : Base("MyThreadPoolAllocator", "Fast thread 1024 byte allocator")
            {
            }
        };

        void run()
        {
            IAllocatorAllocate& poolAlloc = AllocatorInstance<ThreadPoolAllocator>::Get();
            IAllocator& poolAllocator = AllocatorInstance<ThreadPoolAllocator>::GetAllocator();
            // 64 should be the max number of different pool sizes we can allocate.
            void* address[64];
            //////////////////////////////////////////////////////////////////////////
            // Allocate different pool sizes
            memset(address, 0, AZ_ARRAY_SIZE(address)*sizeof(void*));

            // try any size from 8 to 256 (which are supported pool sizes)
            int j = 0;
            for (int size = 8; size <= 256; ++j, size += 8)
            {
                address[j] = poolAlloc.Allocate(size, 8);
                EXPECT_GE(poolAlloc.AllocationSize(address[j]), (AZStd::size_t)size);
                memset(address[j], 1, size);
            }

            EXPECT_GE(poolAlloc.NumAllocatedBytes(), 4126);

            if (poolAllocator.GetRecords())
            {
                poolAllocator.GetRecords()->lock();
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_EQ(32, records.size());
                poolAllocator.GetRecords()->unlock();
            }

            for (int i = 0; address[i] != nullptr; ++i)
            {
                poolAlloc.DeAllocate(address[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            EXPECT_EQ(0, poolAlloc.NumAllocatedBytes());

            if (poolAllocator.GetRecords())
            {
                poolAllocator.GetRecords()->lock();
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_EQ(0, records.size());
                poolAllocator.GetRecords()->unlock();
            }

            //////////////////////////////////////////////////////////////////////////
            // Allocate many elements from the same size
            memset(address, 0, AZ_ARRAY_SIZE(address)*sizeof(void*));
            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(address); ++i)
            {
                address[i] = poolAlloc.Allocate(256, 8);
                EXPECT_GE(poolAlloc.AllocationSize(address[i]), 256);
                memset(address[i], 1, 256);
            }

            EXPECT_GE(poolAlloc.NumAllocatedBytes(), AZ_ARRAY_SIZE(address)*256);

            if (poolAllocator.GetRecords())
            {
                poolAllocator.GetRecords()->lock();
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_EQ(AZ_ARRAY_SIZE(address), records.size());
                poolAllocator.GetRecords()->unlock();
            }

            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(address); ++i)
            {
                poolAlloc.DeAllocate(address[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            EXPECT_EQ(0, poolAlloc.NumAllocatedBytes());

            if (poolAllocator.GetRecords())
            {
                poolAllocator.GetRecords()->lock();
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_EQ(0, records.size());
                poolAllocator.GetRecords()->unlock();
            }

            //////////////////////////////////////////////////////////////////////////
            // Create some threads and simulate concurrent allocation and deallocation
            //AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
            {
                AZStd::thread m_threads[m_maxNumThreads];
                for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                {
                    m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&ThreadPoolAllocatorTest::AllocDeallocFunc, this));
                }

                for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                {
                    m_threads[i].join();
                }
            }
            //////////////////////////////////////////////////////////////////////////
            //AZStd::chrono::microseconds exTime = AZStd::chrono::system_clock::now() - startTime;
            //AZ_Printf("UnitTest","Time: %d Ms\n",exTime.count());

            //////////////////////////////////////////////////////////////////////////
            // Spawn some threads that allocate and some that deallocate together
            {
                AZStd::thread m_threads[m_maxNumThreads];

                for (unsigned int i = m_maxNumThreads/2; i <m_maxNumThreads; ++i)
                {
                    m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&ThreadPoolAllocatorTest::SharedDeAlloc, this));
                }

                for (unsigned int i = 0; i < m_maxNumThreads/2; ++i)
                {
                    m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&ThreadPoolAllocatorTest::SharedAlloc, this));
                }

                for (unsigned int i = 0; i < m_maxNumThreads/2; ++i)
                {
                    m_threads[i].join();
                }

                m_doneSharedAlloc = true;

                for (unsigned int i = m_maxNumThreads/2; i <m_maxNumThreads; ++i)
                {
                    m_threads[i].join();
                }
            }
            //////////////////////////////////////////////////////////////////////////

            // Our pools will support only 1024 byte allocations
            AZ::AllocatorInstance<MyThreadPoolAllocator>::Create();

            void* pooled1024 = AZ::AllocatorInstance<MyThreadPoolAllocator>::Get().Allocate(1024, 1024, 0);
            AZ::AllocatorInstance<MyThreadPoolAllocator>::Get().DeAllocate(pooled1024);

            AZ_TEST_START_TRACE_SUPPRESSION;
            void* pooled2048 = AZ::AllocatorInstance<MyThreadPoolAllocator>::Get().Allocate(2048, 2048, 0);
            (void)pooled2048;
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            AZ::AllocatorInstance<MyThreadPoolAllocator>::Destroy();
        }
    };

    TEST_F(ThreadPoolAllocatorTest, Test)
    {
        run();
    }

    class ThreadPoolAllocatorDynamicWithStaticPagesTest
        : public ThreadPoolAllocatorTest
    {
    public:
        ThreadPoolAllocatorDynamicWithStaticPagesTest()
            : ThreadPoolAllocatorTest(true, 10) {}                                            // just create 10 pages we will allocate more than
    };

    TEST_F(ThreadPoolAllocatorDynamicWithStaticPagesTest, Test)
    {
        run();
    }

    class ThreadPoolAllocatorStaticPagesTest
        : public ThreadPoolAllocatorTest
    {
    public:
        ThreadPoolAllocatorStaticPagesTest()
            : ThreadPoolAllocatorTest(false, 10000) {}
    };

    TEST_F(ThreadPoolAllocatorStaticPagesTest, Test)
    {
        run();
    }

    TEST(BestFitExternalMap, Test)
    {
        SystemAllocator::Descriptor sysDesc;
        sysDesc.m_heap.m_numFixedMemoryBlocks = 1;
        sysDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = 5 * 1024 * 1024;
        sysDesc.m_heap.m_fixedMemoryBlocks[0] = AZ_OS_MALLOC(sysDesc.m_heap.m_fixedMemoryBlocksByteSize[0], sysDesc.m_heap.m_memoryBlockAlignment);
        AllocatorInstance<SystemAllocator>::Create(sysDesc);

        BestFitExternalMapAllocator::Descriptor desc;
        desc.m_mapAllocator = nullptr; // use the system allocator
        desc.m_memoryBlockByteSize = 4 * 1024 * 1024;
        desc.m_memoryBlock = azmalloc(desc.m_memoryBlockByteSize, desc.m_memoryBlockAlignment);

        AllocatorInstance<BestFitExternalMapAllocator>::Create(desc);
        IAllocatorAllocate& bfAlloc = AllocatorInstance<BestFitExternalMapAllocator>::Get();

        EXPECT_EQ( desc.m_memoryBlockByteSize, bfAlloc.Capacity() );
        EXPECT_EQ( 0, bfAlloc.NumAllocatedBytes() );
        EXPECT_EQ( desc.m_memoryBlockByteSize, bfAlloc.GetMaxAllocationSize() );

        void* addr = bfAlloc.Allocate(desc.m_memoryBlockByteSize, 16, 0);

        EXPECT_NE(nullptr, addr);
        EXPECT_EQ( desc.m_memoryBlockByteSize, bfAlloc.Capacity() );
        EXPECT_EQ( desc.m_memoryBlockByteSize, bfAlloc.NumAllocatedBytes() );
        EXPECT_EQ( 0, bfAlloc.GetMaxAllocationSize() );

        // enable assert mode
        //void* addr2 = bfAlloc.Allocate(1,1,0);
        //EXPECT_EQ(NULL, addr2);

        bfAlloc.DeAllocate(addr);
        EXPECT_EQ( desc.m_memoryBlockByteSize, bfAlloc.Capacity() );
        EXPECT_EQ( 0, bfAlloc.NumAllocatedBytes() );
        EXPECT_EQ( desc.m_memoryBlockByteSize, bfAlloc.GetMaxAllocationSize() );

        //bfAlloc.DeAllocate(addr2);

        // add more tests !!!

        AllocatorInstance<BestFitExternalMapAllocator>::Destroy();

        azfree(desc.m_memoryBlock);

        AllocatorInstance<SystemAllocator>::Destroy();
    }

    /**
     * Tests azmalloc,azmallocex/azfree.
     */
    class AZMallocTest
        : public MemoryTrackingFixture
    {
        SystemAllocator::Descriptor m_sysAllocDesc;
    public:
        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();

            m_sysAllocDesc.m_allocationRecords = true;

            AllocatorInstance<SystemAllocator>::Create(m_sysAllocDesc);
            PoolAllocator::Descriptor poolDesc;
            poolDesc.m_allocationRecords = true;
            AllocatorInstance<PoolAllocator>::Create(poolDesc);
        }

        void TearDown() override
        {
            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<SystemAllocator>::Destroy();
            MemoryTrackingFixture::TearDown();
        }

        void run()
        {
            IAllocator& sysAllocator = AllocatorInstance<SystemAllocator>::GetAllocator();
            IAllocator& poolAllocator = AllocatorInstance<PoolAllocator>::GetAllocator();

            void* ptr = azmalloc(16*1024, 32, SystemAllocator);
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            if (sysAllocator.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*sysAllocator.GetRecords());
                const Debug::AllocationRecordsType& records = sysAllocator.GetRecords()->GetMap();
                Debug::AllocationRecordsType::const_iterator iter = records.find(ptr);
                EXPECT_TRUE(iter!=records.end());  // our allocation is in the list
            }
            azfree(ptr, SystemAllocator);
            if (sysAllocator.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*sysAllocator.GetRecords());
                const Debug::AllocationRecordsType& records = sysAllocator.GetRecords()->GetMap();
                EXPECT_TRUE(records.find(ptr)==records.end());  // our allocation is NOT in the list
            }
            const char allocName[] = "BlaBla";
            ptr = azmalloc(16*1024, 32, SystemAllocator, allocName);
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            if (sysAllocator.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*sysAllocator.GetRecords());
                const Debug::AllocationRecordsType& records = sysAllocator.GetRecords()->GetMap();
                Debug::AllocationRecordsType::const_iterator iter = records.find(ptr);
                EXPECT_TRUE(iter!=records.end());  // our allocation is in the list
                //printf("%s",iter->second.m_name);
                EXPECT_STREQ(iter->second.m_name, allocName);
            }
            azfree(ptr, SystemAllocator);
            if (sysAllocator.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*sysAllocator.GetRecords());
                const Debug::AllocationRecordsType& records = sysAllocator.GetRecords()->GetMap();
                EXPECT_TRUE(records.find(ptr)==records.end());  // our allocation is NOT in the list
            }

            ptr = azmalloc(16, 32, PoolAllocator);
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            if (poolAllocator.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*poolAllocator.GetRecords());
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                Debug::AllocationRecordsType::const_iterator iter = records.find(ptr);
                EXPECT_TRUE(iter!=records.end());  // our allocation is in the list
            }
            azfree(ptr, PoolAllocator);
            if (poolAllocator.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*poolAllocator.GetRecords());
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_TRUE(records.find(ptr)==records.end());  // our allocation is NOT in the list
            }
            ptr = azmalloc(16, 32, PoolAllocator, "BlaBlaPool");
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            if (poolAllocator.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*poolAllocator.GetRecords());
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                Debug::AllocationRecordsType::const_iterator iter = records.find(ptr);
                EXPECT_TRUE(iter!=records.end());  // our allocation is in the list
                EXPECT_STREQ(iter->second.m_name, "BlaBlaPool");
            }
            azfree(ptr, PoolAllocator);
            if (poolAllocator.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*poolAllocator.GetRecords());
                const Debug::AllocationRecordsType& records = poolAllocator.GetRecords()->GetMap();
                EXPECT_TRUE(records.find(ptr)==records.end());  // our allocation is NOT in the list
            }
        }
    };

    TEST_F(AZMallocTest, Test)
    {
        run();
    }

    /**
     * Tests aznew/delete, azcreate/azdestroy
     */
    class AZNewCreateDestroyTest
        : public MemoryTrackingFixture
    {
        class MyClass
        {
        public:
            AZ_CLASS_ALLOCATOR(MyClass, PoolAllocator, 0);

            MyClass(int data = 303)
                : m_data(data) {}
            ~MyClass() {}

            alignas(32) int m_data;
        };
        // Explicitly doesn't have AZ_CLASS_ALLOCATOR
        class MyDerivedClass
            : public MyClass
        {
        public:
            MyDerivedClass() = default;
        };
        SystemAllocator::Descriptor m_sysAllocDesc;
    public:
        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();

            m_sysAllocDesc.m_allocationRecords = true;

            AllocatorInstance<SystemAllocator>::Create(m_sysAllocDesc);
            PoolAllocator::Descriptor poolDesc;
            poolDesc.m_allocationRecords = true;
            AllocatorInstance<PoolAllocator>::Create(poolDesc);
        }

        void TearDown() override
        {
            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<SystemAllocator>::Destroy();

            MemoryTrackingFixture::TearDown();
        }

        void run()
        {
            IAllocator& sysAlloc = AllocatorInstance<SystemAllocator>::GetAllocator();
            IAllocator& poolAlloc = AllocatorInstance<PoolAllocator>::GetAllocator();

            MyClass* ptr = aznew MyClass(202);  /// this should allocate memory from the pool allocator
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            EXPECT_EQ(202, ptr->m_data);               // check value

            if (poolAlloc.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*poolAlloc.GetRecords());
                const Debug::AllocationRecordsType& records = poolAlloc.GetRecords()->GetMap();
                Debug::AllocationRecordsType::const_iterator iter = records.find(ptr);
                EXPECT_TRUE(iter!=records.end());  // our allocation is in the list
                EXPECT_STREQ(iter->second.m_name, "MyClass");
            }
            delete ptr;

            if (poolAlloc.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*poolAlloc.GetRecords());
                const Debug::AllocationRecordsType& records = poolAlloc.GetRecords()->GetMap();
                EXPECT_TRUE(records.find(ptr)==records.end());  // our allocation is NOT in the list
            }

            // now use the azcreate to allocate the object wherever we want
            ptr = azcreate(MyClass, (101), SystemAllocator);
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            EXPECT_EQ(101, ptr->m_data);               // check value
            if (sysAlloc.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*sysAlloc.GetRecords());
                const Debug::AllocationRecordsType& records = sysAlloc.GetRecords()->GetMap();
                Debug::AllocationRecordsType::const_iterator iter = records.find(ptr);
                EXPECT_TRUE(iter!=records.end());  // our allocation is in the list
                EXPECT_STREQ(iter->second.m_name, "MyClass");
            }
            azdestroy(ptr, SystemAllocator);
            if (sysAlloc.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*sysAlloc.GetRecords());
                const Debug::AllocationRecordsType& records = sysAlloc.GetRecords()->GetMap();
                EXPECT_TRUE(records.find(ptr)==records.end());  // our allocation is NOT in the list
            }

            // azcreate
            // now use the azcreate to allocate the object wherever we want
            ptr = azcreate(MyClass, (505), SystemAllocator, "MyClassNamed");
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            EXPECT_EQ(505, ptr->m_data);               // check value
            if (sysAlloc.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*sysAlloc.GetRecords());
                const Debug::AllocationRecordsType& records = sysAlloc.GetRecords()->GetMap();
                Debug::AllocationRecordsType::const_iterator iter = records.find(ptr);
                EXPECT_TRUE(iter!=records.end());  // our allocation is in the list
                EXPECT_STREQ(iter->second.m_name, "MyClassNamed");
            }
            azdestroy(ptr);  // imply SystemAllocator
            if (sysAlloc.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*sysAlloc.GetRecords());
                const Debug::AllocationRecordsType& records = sysAlloc.GetRecords()->GetMap();
                EXPECT_TRUE(records.find(ptr)==records.end());  // our allocation is NOT in the list
            }

            // Test creation of derived classes
            ptr = aznew MyDerivedClass();       /// this should allocate memory from the pool allocator
            EXPECT_EQ(0, ((size_t)ptr & 31));   // check alignment
            EXPECT_EQ(303, ptr->m_data);        // check value

            if (poolAlloc.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*poolAlloc.GetRecords());
                const Debug::AllocationRecordsType& records = poolAlloc.GetRecords()->GetMap();
                Debug::AllocationRecordsType::const_iterator iter = records.find(ptr);
                EXPECT_TRUE(iter != records.end());  // our allocation is in the list
                EXPECT_STREQ(iter->second.m_name, "MyClass");
            }
            delete ptr;

            if (poolAlloc.GetRecords())
            {
                AZStd::lock_guard<AZ::Debug::AllocationRecords> lock(*poolAlloc.GetRecords());
                const Debug::AllocationRecordsType& records = poolAlloc.GetRecords()->GetMap();
                EXPECT_TRUE(records.find(ptr) == records.end());  // our allocation is NOT in the list
            }
        }
    };

    TEST_F(AZNewCreateDestroyTest, Test)
    {
        run();
    }

#if AZ_TRAIT_PERF_MEMORYBENCHMARK_IS_AVAILABLE
    class PERF_MemoryBenchmark
        : public ::testing::Test
    {
#if defined(_DEBUG)
        static const unsigned N = 1024 * 128;
#else
        static const unsigned N = 1024 * 512;
#endif
        static const size_t DefaultAlignment = 1;
        static const size_t MIN_SIZE = 2;
        size_t MAX_SIZE;
        static const size_t MAX_ALIGNMENT_LOG2 = 7;
        static const size_t MAX_ALIGNMENT = 1 << MAX_ALIGNMENT_LOG2;

        struct test_record
        {
            void* ptr;
            size_t size;
            size_t alignment;
            size_t _padding;
        }* tr /*[N]*/;


        size_t rand_size()
        {
            float r = float(rand()) / RAND_MAX;
            return MIN_SIZE + (size_t)((MAX_SIZE - MIN_SIZE) * powf(r, 8.0f));
        }

        size_t rand_alignment()
        {
            float r = float(rand()) / RAND_MAX;
            return (size_t)1 << (size_t)(MAX_ALIGNMENT_LOG2 * r);
        }

        class DebugSysAlloc
            : public AZ::IAllocatorAllocate
        {
            pointer_type    Allocate(size_type byteSize, size_type alignment, int flags, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
            {
                (void)flags;
                (void)name;
                (void)fileName;
                (void)lineNum;
                (void)suppressStackRecord;
                return AZ_OS_MALLOC(byteSize, alignment);
            }
            void                    DeAllocate(pointer_type ptr, size_type, size_type) override
            {
                AZ_OS_FREE(ptr);
            }
            pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
            {
                (void)ptr;
                (void)newSize;
                (void)newAlignment;
                AZ_Assert(false, "Not supported!");
                return NULL;
            }
            /// Resize an allocated memory block. Returns the new expanded size (up to newSize) or AllocationSize(ptr) or 0 (if you don't support resize at all)
            size_type               Resize(pointer_type ptr, size_type newSize) override  { (void)ptr; (void)newSize; return 0; }
            /// Returns allocation size for given address. 0 if the address doesn't belong to the allocator.
            size_type               AllocationSize(pointer_type ptr) override             { (void)ptr; return 0; }

            size_type               NumAllocatedBytes() const override                    { return 0; }
            /// Returns the capacity of the Allocator in bytes. If the return value is 0 the Capacity is undefined (usually depends on another allocator)
            size_type               Capacity() const override                             { return 1 * 1024 * 1024 * 1024; }
            /// Returns max allocation size if possible. If not returned value is 0
            size_type               GetMaxAllocationSize() const override                 { return 1 * 1024 * 1024 * 1024; }
            /// Returns max allocation size of a single contiguous allocation
            size_type               GetMaxContiguousAllocationSize() const override       { return 1 * 1024 * 1024 * 1024; }
            /// Returns a pointer to a sub-allocator or NULL.
            IAllocatorAllocate*     GetSubAllocator() override                            { return NULL; }
        };
    public:
        void SetUp() override
        {
            SystemAllocator::Descriptor desc;
            AllocatorInstance<SystemAllocator>::Create(desc);
            tr = (test_record*)AZ_OS_MALLOC(sizeof(test_record)*N, 8);
            MAX_SIZE = 4096;
        }
        
        void TearDown() override
        {
            AZ_OS_FREE(tr);
            AllocatorInstance<SystemAllocator>::Destroy();
        }

        //////////////////////////////////////////////////////////////////////////
        // default allocator
        void defAllocate(unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = AZ_OS_MALLOC(size, DefaultAlignment);
            }
        }
        void defFree(unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                AZ_OS_FREE(tr[j].ptr);
                tr[j].ptr = tr[i].ptr;
            }
        }
        void defAllocateSize(unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = AZ_OS_MALLOC(size, DefaultAlignment);
                tr[i].size = size;
            }
        }
        void defFreeSize(unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                AZ_OS_FREE(tr[j].ptr /*, tr[j].size*/);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
            }
        }

        void defAllocateAlignment(unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = AZ_OS_MALLOC(size, alignment);
            }
        }
        void defFreeAlignment(unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                AZ_OS_FREE(tr[j].ptr);
                tr[j].ptr = tr[i].ptr;
            }
        }

        void defAllocateAlignmentSize(unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = AZ_OS_MALLOC(size, alignment);
                tr[i].size = size;
                tr[i].alignment = alignment;
            }
        }
        void defFreeAlignmentSize(unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                AZ_OS_FREE(tr[j].ptr /*, tr[j].size, tr[j].alignment*/);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
                tr[j].alignment = tr[i].alignment;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // HPHA
        void hphaAllocate(HphaSchema& hpha, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = hpha.Allocate(size, DefaultAlignment, 0);
            }
        }
        void hphaFree(HphaSchema& hpha, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                hpha.DeAllocate(tr[j].ptr);
                tr[j].ptr = tr[i].ptr;
            }
        }
        void hphaAllocateSize(HphaSchema& hpha, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = hpha.Allocate(size, DefaultAlignment, 0);
                tr[i].size = size;
            }
        }
        void hphaFreeSize(HphaSchema& hpha, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                hpha.DeAllocate(tr[j].ptr, tr[j].size);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
            }
        }

        void hphaAllocateAlignment(HphaSchema& hpha, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = hpha.Allocate(size, alignment, 0);
            }
        }
        void hphaFreeAlignment(HphaSchema& hpha, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                hpha.DeAllocate(tr[j].ptr);
                tr[j].ptr = tr[i].ptr;
            }
        }

        void hphaAllocateAlignmentSize(HphaSchema& hpha, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = hpha.Allocate(size, alignment, 0);
                tr[i].size = size;
                tr[i].alignment = alignment;
            }
        }
        void hphaFreeAlignmentSize(HphaSchema& hpha, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                hpha.DeAllocate(tr[j].ptr, tr[j].size, tr[j].alignment);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
                tr[j].alignment = tr[i].alignment;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DLMalloc
        void dlAllocate(HeapSchema& dl, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = dl.Allocate(size, DefaultAlignment, 0);
            }
        }
        void dlFree(HeapSchema& dl, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                dl.DeAllocate(tr[j].ptr);
                tr[j].ptr = tr[i].ptr;
            }
        }
        void dlAllocateSize(HeapSchema& dl, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = dl.Allocate(size, DefaultAlignment, 0);
                tr[i].size = size;
            }
        }
        void dlFreeSize(HeapSchema& dl, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                dl.DeAllocate(tr[j].ptr /*, tr[j].size*/);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
            }
        }

        void dlAllocateAlignment(HeapSchema& dl, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = dl.Allocate(size, alignment, 0);
            }
        }
        void dlFreeAlignment(HeapSchema& dl, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                dl.DeAllocate(tr[j].ptr);
                tr[j].ptr = tr[i].ptr;
            }
        }

        void dlAllocateAlignmentSize(HeapSchema& dl, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = dl.Allocate(size, alignment, 0);
                tr[i].size = size;
                tr[i].alignment = alignment;
            }
        }
        void dlFreeAlignmentSize(HeapSchema& dl, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                dl.DeAllocate(tr[j].ptr /*, tr[j].size, tr[j].alignment*/);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
                tr[j].alignment = tr[i].alignment;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // POOL
        void poolAllocate(PoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = pool.Allocate(size, DefaultAlignment, 0, nullptr, nullptr, 0, 0);
            }
        }
        void poolFree(PoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                pool.DeAllocate(tr[j].ptr, 0, 0);
                tr[j].ptr = tr[i].ptr;
            }
        }
        void poolAllocateSize(PoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = pool.Allocate(size, DefaultAlignment, 0, nullptr, nullptr, 0, 0);
                tr[i].size = size;
            }
        }
        void poolFreeSize(PoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                pool.DeAllocate(tr[j].ptr, tr[j].size, 0);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
            }
        }

        void poolAllocateAlignment(PoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = pool.Allocate(size, alignment, 0, nullptr, nullptr, 0, 0);
            }
        }
        void poolFreeAlignment(PoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                pool.DeAllocate(tr[j].ptr, 0, 0);
                tr[j].ptr = tr[i].ptr;
            }
        }

        void poolAllocateAlignmentSize(PoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = pool.Allocate(size, alignment, 0, nullptr, nullptr, 0, 0);
                tr[i].size = size;
                tr[i].alignment = alignment;
            }
        }
        void poolFreeAlignmentSize(PoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                pool.DeAllocate(tr[j].ptr, tr[j].size, tr[j].alignment);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
                tr[j].alignment = tr[i].alignment;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // THREAD POOL
        void thPoolAllocate(ThreadPoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = pool.Allocate(size, DefaultAlignment, 0, nullptr, nullptr, 0, 0);
            }
        }
        void thPoolFree(ThreadPoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                pool.DeAllocate(tr[j].ptr, 0, 0);
                tr[j].ptr = tr[i].ptr;
            }
        }
        void thPoolAllocateSize(ThreadPoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                tr[i].ptr = pool.Allocate(size, DefaultAlignment, 0, nullptr, nullptr, 0, 0);
                tr[i].size = size;
            }
        }
        void thPoolFreeSize(ThreadPoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                pool.DeAllocate(tr[j].ptr, tr[j].size, 0);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
            }
        }

        void thPoolAllocateAlignment(ThreadPoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = pool.Allocate(size, alignment, 0, nullptr, nullptr, 0, 0);
            }
        }
        void thPoolFreeAlignment(ThreadPoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                pool.DeAllocate(tr[j].ptr, 0, 0);
                tr[j].ptr = tr[i].ptr;
            }
        }

        void thPoolAllocateAlignmentSize(ThreadPoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                size_t size = rand_size();
                size_t alignment = rand_alignment();
                tr[i].ptr = pool.Allocate(size, alignment, 0, nullptr, nullptr, 0, 0);
                tr[i].size = size;
                tr[i].alignment = alignment;
            }
        }
        void thPoolFreeAlignmentSize(ThreadPoolSchema& pool, unsigned iStart, unsigned iEnd)
        {
            for (unsigned i = iStart; i < iEnd; i++)
            {
                unsigned j = i + rand() % (iEnd - i);
                pool.DeAllocate(tr[j].ptr, tr[j].size, tr[j].alignment);
                tr[j].ptr = tr[i].ptr;
                tr[j].size = tr[i].size;
                tr[j].alignment = tr[i].alignment;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        void allocdealloc(HphaSchema& hpha, HeapSchema& dl, PoolSchema& pool, bool isHpha, bool isDL, bool isDefault, bool isPool)
        {
            AZStd::chrono::system_clock::time_point start;
            AZStd::chrono::duration<float> elapsed;
            printf("MinSize %u MaxSize %u MaxAlignment %u\n", static_cast<unsigned int>(MIN_SIZE), static_cast<unsigned int>(MAX_SIZE), static_cast<unsigned int>(MAX_ALIGNMENT));
            printf("\t\t\t\tHPHA\t\tDL\t\tDEFAULT\t\tPOOL\n");

            printf("ALLOC/FREE:");
            if (isHpha)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                hphaAllocate(hpha, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t\t\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                hphaFree(hpha, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t\t\t(skip/skip)");
            }

            if (isDL)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                dlAllocate(dl, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                dlFree(dl, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }


            if (isDefault)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                defAllocate(0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                defFree(0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }

            if (MAX_SIZE<=256)
            {
                if (isPool)
                {
                    srand(1234);
                    start = AZStd::chrono::system_clock::now();
                    poolAllocate(pool, 0, N);
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    start = AZStd::chrono::system_clock::now();
                    poolFree(pool, 0, N);
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)\n", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)\n");
                }
            }
            else
            {
                printf("\n");
            }

            printf("ALLOC/FREE(size):");
            if (isHpha)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                hphaAllocateSize(hpha, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                hphaFreeSize(hpha, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t\t(skip/skip)");
            }

            if (isDL)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                dlAllocateSize(dl, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                dlFreeSize(dl, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }

            if (isDefault)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                defAllocateSize(0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                defFreeSize(0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }

            if (MAX_SIZE<=256)
            {
                if (isPool)
                {
                    srand(1234);
                    start = AZStd::chrono::system_clock::now();
                    poolAllocateSize(pool, 0, N);
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    start = AZStd::chrono::system_clock::now();
                    poolFreeSize(pool, 0, N);
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)\n", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)\n");
                }
            }
            else
            {
                printf("\n");
            }

            printf("ALLOC(align)/FREE:");
            if (isHpha)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                hphaAllocateAlignment(hpha, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                hphaFreeAlignment(hpha, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t\t(skip/skip)");
            }

            if (isDL)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                dlAllocateAlignment(dl, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                dlFreeAlignment(dl, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }

            if (isDefault)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                defAllocateAlignment(0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                defFreeAlignment(0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }

            if (MAX_SIZE<=256)
            {
                if (isPool)
                {
                    srand(1234);
                    start = AZStd::chrono::system_clock::now();
                    poolAllocateAlignment(pool, 0, N);
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    start = AZStd::chrono::system_clock::now();
                    poolFreeAlignment(pool, 0, N);
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)\n", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)\n");
                }
            }
            else
            {
                printf("\n");
            }

            printf("ALLOC(align)/FREE(size,align):");
            if (isHpha)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                hphaAllocateAlignmentSize(hpha, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                hphaFreeAlignmentSize(hpha, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }

            if (isDL)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                dlAllocateAlignmentSize(dl, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                dlFreeAlignmentSize(dl, 0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }

            if (isDefault)
            {
                srand(1234);
                start = AZStd::chrono::system_clock::now();
                defAllocateAlignmentSize(0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("\t(%.3f/", elapsed.count());
                start = AZStd::chrono::system_clock::now();
                defFreeAlignmentSize(0, N);
                elapsed = AZStd::chrono::system_clock::now() - start;
                printf("%.3f)", elapsed.count());
            }
            else
            {
                printf("\t(skip/skip)");
            }

            if (MAX_SIZE<=256)
            {
                if (isPool)
                {
                    srand(1234);
                    start = AZStd::chrono::system_clock::now();
                    poolAllocateAlignmentSize(pool, 0, N);
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    start = AZStd::chrono::system_clock::now();
                    poolFreeAlignmentSize(pool, 0, N);
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)\n", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)\n");
                }
            }
            else
            {
                printf("\n");
            }

            printf("TEST REALLOC:");
            srand(1234);
            start = AZStd::chrono::system_clock::now();
            for (unsigned i = 0; i < N; i++) {
            size_t size = rand_size();
            tr[i].ptr = hpha.ReAllocate(nullptr, size, 1);
            }
            for (unsigned i = 0; i < N; i++) {
            size_t size = rand_size();
            tr[i].ptr = hpha.ReAllocate(tr[i].ptr, size, 1);
            }
            for (unsigned i = 0; i < N; i++) {
            unsigned j = i + rand() % (N - i);
            hpha.ReAllocate(tr[j].ptr, 0, 0);
            tr[j].ptr = tr[i].ptr;
            }
            elapsed = AZStd::chrono::system_clock::now() - start;
            printf("\t\t\t\t\t(%.3f)", elapsed.count());

            srand(1234);
            start = AZStd::chrono::system_clock::now();
            for (unsigned i = 0; i < N; i++) {
            size_t size = rand_size();
            tr[i].ptr = realloc(NULL, size);
            }
            for (unsigned i = 0; i < N; i++) {
            size_t size = rand_size();
            tr[i].ptr = realloc(tr[i].ptr, size);
            }
            for (unsigned i = 0; i < N; i++) {
            unsigned j = i + rand() % (N - i);
            realloc(tr[j].ptr, 0);
            tr[j].ptr = tr[i].ptr;
            }
            elapsed = AZStd::chrono::system_clock::now() - start;
            printf("\t(%.3f)\n", elapsed.count());

            printf("TEST REALLOC with ALIGNMENT:");
            srand(1234);
            start = AZStd::chrono::system_clock::now();
            for (unsigned i = 0; i < N; i++) {
            size_t size = rand_size();
            size_t alignment = rand_alignment();
            tr[i].ptr = hpha.ReAllocate(NULL, size, alignment);
            }
            for (unsigned i = 0; i < N; i++) {
            size_t size = rand_size();
            size_t alignment = rand_alignment();
            tr[i].ptr = hpha.ReAllocate(tr[i].ptr, size, alignment);
            }
            for (unsigned i = 0; i < N; i++) {
            unsigned j = i + rand() % (N - i);
            hpha.ReAllocate(tr[j].ptr, 0, 0);
            tr[j].ptr = tr[i].ptr;
            }
            elapsed = AZStd::chrono::system_clock::now() - start;
            printf("\t\t\t(%.3f)", elapsed.count());

            srand(1234);
            start = AZStd::chrono::system_clock::now();
#ifdef AZ_PLATFORM_WINDOWS // Windows test only
            for (unsigned i = 0; i < N; i++) {
            size_t size = rand_size();
            size_t alignment = rand_alignment();
            tr[i].ptr = _aligned_realloc(NULL, size, alignment);
            }
            for (unsigned i = 0; i < N; i++) {
            size_t size = rand_size();
            size_t alignment = rand_alignment();
            tr[i].ptr = _aligned_realloc(tr[i].ptr, size, alignment);
            }
            for (unsigned i = 0; i < N; i++) {
            unsigned j = i + rand() % (N - i);
            realloc(tr[j].ptr, 0);
            tr[j].ptr = tr[i].ptr;
            }
#endif // AZ_PLATFORM_WINDOWS
            elapsed = AZStd::chrono::system_clock::now() - start;
            printf("\t(%.3f)\n", elapsed.count());
        }

        void allocdeallocThread(HphaSchema& hpha, HeapSchema& dl, ThreadPoolSchema& thPool, bool isHpha, bool isDL, bool isDefault, bool isPool)
        {
            AZStd::chrono::system_clock::time_point start;
            AZStd::chrono::duration<float> elapsed;
            printf("MinSize %u MaxSize %u MaxAlignment %u\n", static_cast<unsigned int>(MIN_SIZE), static_cast<unsigned int>(MAX_SIZE), static_cast<unsigned int>(MAX_ALIGNMENT));
            printf("\t\t\t\tHPHA\t\tDL\t\tDEFAULT\t\tPOOL\n");

            static const int TN = N / 4;

            {
                printf("ALLOC/FREE:");
                if (isHpha)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocate, this, AZStd::ref(hpha), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocate, this, AZStd::ref(hpha), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocate, this, AZStd::ref(hpha), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocate, this, AZStd::ref(hpha), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t\t\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::hphaFree, this, AZStd::ref(hpha), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::hphaFree, this, AZStd::ref(hpha), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::hphaFree, this, AZStd::ref(hpha), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::hphaFree, this, AZStd::ref(hpha), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t\t\t(skip/skip)");
                }
            }

            {
                if (isDL)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::dlAllocate, this, AZStd::ref(dl), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::dlAllocate, this, AZStd::ref(dl), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::dlAllocate, this, AZStd::ref(dl), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::dlAllocate, this, AZStd::ref(dl), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::dlFree, this, AZStd::ref(dl), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::dlFree, this, AZStd::ref(dl), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::dlFree, this, AZStd::ref(dl), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::dlFree, this, AZStd::ref(dl), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }

            {
                if (isDefault)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::defAllocate, this, 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::defAllocate, this, 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::defAllocate, this, 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::defAllocate, this, 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::defFree, this, 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::defFree, this, 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::defFree, this, 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::defFree, this, 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }

            if (MAX_SIZE<=256)
            {
                if (isPool)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocate, this, AZStd::ref(thPool), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocate, this, AZStd::ref(thPool), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocate, this, AZStd::ref(thPool), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocate, this, AZStd::ref(thPool), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::thPoolFree, this, AZStd::ref(thPool), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::thPoolFree, this, AZStd::ref(thPool), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::thPoolFree, this, AZStd::ref(thPool), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::thPoolFree, this, AZStd::ref(thPool), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)\n", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)\n");
                }
            }
            else
            {
                printf("\n");
            }

            {
                printf("ALLOC/FREE(size):");
                if (isHpha)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateSize, this, AZStd::ref(hpha), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateSize, this, AZStd::ref(hpha), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateSize, this, AZStd::ref(hpha), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateSize, this, AZStd::ref(hpha), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeSize, this, AZStd::ref(hpha), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeSize, this, AZStd::ref(hpha), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeSize, this, AZStd::ref(hpha), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeSize, this, AZStd::ref(hpha), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("/%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t\t(skip/skip)");
                }
            }

            {
                if (isDL)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateSize, this, AZStd::ref(dl), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateSize, this, AZStd::ref(dl), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateSize, this, AZStd::ref(dl), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateSize, this, AZStd::ref(dl), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::dlFreeSize, this, AZStd::ref(dl), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::dlFreeSize, this, AZStd::ref(dl), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::dlFreeSize, this, AZStd::ref(dl), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::dlFreeSize, this, AZStd::ref(dl), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }


            {
                if (isDefault)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::defAllocateSize, this, 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::defAllocateSize, this, 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::defAllocateSize, this, 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::defAllocateSize, this, 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::defFreeSize, this, 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::defFreeSize, this, 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::defFreeSize, this, 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::defFreeSize, this, 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }

            if (MAX_SIZE<=256)
            {
                if (isPool)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateSize, this, AZStd::ref(thPool), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateSize, this, AZStd::ref(thPool), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateSize, this, AZStd::ref(thPool), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateSize, this, AZStd::ref(thPool), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeSize, this, AZStd::ref(thPool), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeSize, this, AZStd::ref(thPool), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeSize, this, AZStd::ref(thPool), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeSize, this, AZStd::ref(thPool), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)\n", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)\n");
                }
            }
            else
            {
                printf("\n");
            }

            {
                printf("ALLOC(align)/FREE:");
                if (isHpha)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateAlignment, this, AZStd::ref(hpha), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateAlignment, this, AZStd::ref(hpha), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateAlignment, this, AZStd::ref(hpha), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateAlignment, this, AZStd::ref(hpha), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeAlignment, this, AZStd::ref(hpha), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeAlignment, this, AZStd::ref(hpha), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeAlignment, this, AZStd::ref(hpha), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeAlignment, this, AZStd::ref(hpha), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t\t(skip/skip)");
                }
            }

            {
                if (isDL)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateAlignment, this, AZStd::ref(dl), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateAlignment, this, AZStd::ref(dl), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateAlignment, this, AZStd::ref(dl), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateAlignment, this, AZStd::ref(dl), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::dlFreeAlignment, this, AZStd::ref(dl), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::dlFreeAlignment, this, AZStd::ref(dl), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::dlFreeAlignment, this, AZStd::ref(dl), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::dlFreeAlignment, this, AZStd::ref(dl), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }

            {
                if (isDefault)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::defAllocateAlignment, this, 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::defAllocateAlignment, this, 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::defAllocateAlignment, this, 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::defAllocateAlignment, this, 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::defFreeAlignment, this, 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::defFreeAlignment, this, 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::defFreeAlignment, this, 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::defFreeAlignment, this, 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }

            if (MAX_SIZE<=256)
            {
                if (isPool)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateAlignment, this, AZStd::ref(thPool), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateAlignment, this, AZStd::ref(thPool), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateAlignment, this, AZStd::ref(thPool), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateAlignment, this, AZStd::ref(thPool), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeAlignment, this, AZStd::ref(thPool), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeAlignment, this, AZStd::ref(thPool), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeAlignment, this, AZStd::ref(thPool), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeAlignment, this, AZStd::ref(thPool), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)\n", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)\n");
                }
            }
            else
            {
                printf("\n");
            }

            {
                printf("ALLOC(align)/FREE(size,align):");
                if (isHpha)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateAlignmentSize, this, AZStd::ref(hpha), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateAlignmentSize, this, AZStd::ref(hpha), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateAlignmentSize, this, AZStd::ref(hpha), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::hphaAllocateAlignmentSize, this, AZStd::ref(hpha), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeAlignmentSize, this, AZStd::ref(hpha), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeAlignmentSize, this, AZStd::ref(hpha), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeAlignmentSize, this, AZStd::ref(hpha), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::hphaFreeAlignmentSize, this, AZStd::ref(hpha), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }

            {
                if (isDL)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateAlignmentSize, this, AZStd::ref(dl), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateAlignmentSize, this, AZStd::ref(dl), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateAlignmentSize, this, AZStd::ref(dl), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::dlAllocateAlignmentSize, this, AZStd::ref(dl), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::dlFreeAlignmentSize, this, AZStd::ref(dl), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::dlFreeAlignmentSize, this, AZStd::ref(dl), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::dlFreeAlignmentSize, this, AZStd::ref(dl), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::dlFreeAlignmentSize, this, AZStd::ref(dl), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }

            {
                if (isDefault)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::defAllocateAlignmentSize, this, 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::defAllocateAlignmentSize, this, 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::defAllocateAlignmentSize, this, 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::defAllocateAlignmentSize, this, 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::defFreeAlignmentSize, this, 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::defFreeAlignmentSize, this, 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::defFreeAlignmentSize, this, 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::defFreeAlignmentSize, this, 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)");
                }
            }

            if (MAX_SIZE<=256)
            {
                if (isPool)
                {
                    srand(1234);
                    AZStd::thread tr1(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateAlignmentSize, this, AZStd::ref(thPool), 0 * TN, 1 * TN));
                    AZStd::thread tr2(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateAlignmentSize, this, AZStd::ref(thPool), 1 * TN, 2 * TN));
                    AZStd::thread tr3(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateAlignmentSize, this, AZStd::ref(thPool), 2 * TN, 3 * TN));
                    AZStd::thread tr4(AZStd::bind(&PERF_MemoryBenchmark::thPoolAllocateAlignmentSize, this, AZStd::ref(thPool), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr1.join();
                    tr2.join();
                    tr3.join();
                    tr4.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("\t(%.3f/", elapsed.count());
                    AZStd::thread tr5(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeAlignmentSize, this, AZStd::ref(thPool), 0 * TN, 1 * TN));
                    AZStd::thread tr6(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeAlignmentSize, this, AZStd::ref(thPool), 1 * TN, 2 * TN));
                    AZStd::thread tr7(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeAlignmentSize, this, AZStd::ref(thPool), 2 * TN, 3 * TN));
                    AZStd::thread tr8(AZStd::bind(&PERF_MemoryBenchmark::thPoolFreeAlignmentSize, this, AZStd::ref(thPool), 3 * TN, 4 * TN));
                    start = AZStd::chrono::system_clock::now();
                    tr5.join();
                    tr6.join();
                    tr7.join();
                    tr8.join();
                    elapsed = AZStd::chrono::system_clock::now() - start;
                    printf("%.3f)\n", elapsed.count());
                }
                else
                {
                    printf("\t(skip/skip)\n");
                }
            }
            else
            {
                printf("\n");
            }
        }

        void run()
        {
            printf("\n\t\t\t=======================\n");
            printf("\t\t\tSchemas Benchmark Test!\n");
            printf("\t\t\t=======================\n");
            DebugSysAlloc da;
            {
                HphaSchema::Descriptor hphaDesc;
                hphaDesc.m_fixedMemoryBlockByteSize = AZ_TRAIT_OS_HPHA_MEMORYBLOCKBYTESIZE;
                hphaDesc.m_fixedMemoryBlock = AZ_OS_MALLOC(hphaDesc.m_fixedMemoryBlockByteSize, hphaDesc.m_fixedMemoryBlockAlignment);
                HeapSchema::Descriptor dlMallocDesc;
                dlMallocDesc.m_numMemoryBlocks = 1;
                dlMallocDesc.m_memoryBlocksByteSize[0] = hphaDesc.m_fixedMemoryBlockByteSize;
                dlMallocDesc.m_memoryBlocks[0] = AZ_OS_MALLOC(dlMallocDesc.m_memoryBlocksByteSize[0], hphaDesc.m_fixedMemoryBlockAlignment);
                PoolSchema::Descriptor poolDesc;
                poolDesc.m_pageAllocator = &da;
                poolDesc.m_numStaticPages = 100;
                printf("\t\t\tPrealocated memory %.2f MB!\n", (float)hphaDesc.m_fixedMemoryBlockByteSize / (1024.f*1024.f));
                {
                    HphaSchema hpha(hphaDesc);
                    HeapSchema dl(dlMallocDesc);
                    PoolSchema pool;
                    pool.Create(poolDesc);
                    ThreadPoolSchemaHelper<nullptr_t> threadPool;
                    threadPool.Create(poolDesc);

                    printf("---- Single Thread ----\n");
                    // any allocations
                    MAX_SIZE = 4096;
                    allocdealloc(hpha, dl, pool, false, false, false, true);
                    printf("\n");
                    // pool allocations
                    MAX_SIZE = 256;
                    allocdealloc(hpha, dl, pool, true, false, false, true);

                    // threads
                    printf("\n---- 4 Threads ----\n");
                    // any allocations
                    MAX_SIZE = 4096;
                    //allocdeallocThread(hpha,dl,threadPool,true,true,true,true);
                    printf("\n");
                    // pool allocations
                    MAX_SIZE = 256;
                    //allocdeallocThread(hpha,dl,threadPool,true,true,true,true);

                    pool.Destroy();
                    threadPool.Destroy();
                }
                AZ_OS_FREE(hphaDesc.m_fixedMemoryBlock);
                AZ_OS_FREE(dlMallocDesc.m_memoryBlocks[0]);
            }

            #if AZ_TRAIT_UNITTEST_NON_PREALLOCATED_HPHA_TEST
                      printf("\n\t\t\tNO prealocated memory!\n");
                      {
                          HphaSchema::Descriptor hphaDesc;
                          HeapSchema::Descriptor dlMallocDesc;
                          PoolSchema::Descriptor poolDesc;
                          poolDesc.m_pageAllocator = &da;
            
                          HphaSchema hpha(hphaDesc);
                          HeapSchema dl(dlMallocDesc);
                          PoolSchema pool;
                          pool.Create(poolDesc);
                          ThreadPoolSchemaHelper<nullptr_t> threadPool;
                          threadPool.Create(poolDesc);
            
                          printf("---- Single Thread ----\n");
                          // any allocations
                          MAX_SIZE = 4096;
                          allocdealloc(hpha,dl,pool,true,true,true,true);
                          printf("\n");
                          // pool allocations
                          MAX_SIZE = 256;
                          allocdealloc(hpha,dl,pool,true,true,true,true);
            
                          // threads
                          printf("\n---- 4 Threads ----\n");
                          // any allocations
                          MAX_SIZE = 4096;
                          allocdeallocThread(hpha,dl,threadPool,true,true,true,true);
                          printf("\n");
                          // pool allocations
                          MAX_SIZE = 256;
                          allocdeallocThread(hpha,dl,threadPool,true,true,true,true);
                          pool.Destroy();
                          threadPool.Destroy();
                      }
            #endif
        }
    };

#ifdef ENABLE_PERFORMANCE_TEST
    TEST_F(PERF_MemoryBenchmark, Test)
    {
        run();
    }
#endif // ENABLE_PERFORMANCE_TEST
#endif // AZ_TRAIT_PERF_MEMORYBENCHMARK_IS_AVAILABLE
}

// GlobalNewDeleteTest-End
