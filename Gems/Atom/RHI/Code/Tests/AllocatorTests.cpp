/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI/PoolAllocator.h>
#include <Atom/RHI/FreeListAllocator.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/time.h>
#include <AzCore/UnitTest/UnitTest.h>

#define PRINTF(...)  do { UnitTest::ColoredPrintf(UnitTest::COLOR_GREEN, "[          ] "); UnitTest::ColoredPrintf(UnitTest::COLOR_YELLOW, __VA_ARGS__); } while(0)

namespace UnitTest
{
    using namespace AZ;

    class AllocatorTest
        : public RHITestFixture
    {
    public:
        AllocatorTest()
            : RHITestFixture()
        {}

        static size_t GetBytesToBlocks(size_t bytes, size_t alignment)
        {
            return size_t((bytes + alignment - 1) / alignment);
        }

        struct TestDescriptor
        {
            bool m_useVisualizer = false;
            RHI::Allocator* m_allocator = nullptr;
            size_t m_byteCountMax = 0;
            size_t m_byteAlignmentBase = 256;
            size_t m_gcLatency = 3;
            size_t m_iterations = 10000;
            size_t m_allocationSizeMin = 1;
            size_t m_allocationSizeMax = 8 * 1024;
            AZ::u64 m_addressBase = 0;
        };

        void run(const TestDescriptor& descriptor)
        {
            struct Block
            {
                Block() : m_used{}, m_gcIteration{} {}

                AZ::u16 m_used;
                AZ::u16 m_gcIteration;
            };

            struct Allocation
            {
                RHI::VirtualAddress address;
                size_t size;
            };

            const size_t BlockCount = descriptor.m_byteCountMax / descriptor.m_byteAlignmentBase;
            AZStd::vector<Block> m_usedBlocks(BlockCount);
            AZStd::vector<Allocation> currentAllocations;
            AZStd::vector<Allocation> retiredAllocationsCurrent;
            AZStd::vector<Allocation> retiredAllocationsPrevious;

            const size_t AllocationSizeRange = descriptor.m_allocationSizeMax - descriptor.m_allocationSizeMin;

            AZStd::string outputString;
            outputString.resize(BlockCount);

            AZ::SimpleLcgRandom random(AZStd::GetTimeNowMicroSecond());

            /**
             * Does a bunch of random add / remove iterations, tracking garbage collection
             * and block usage. It will assert if the allocator attempts to stomp on another
             * allocation that is marked used.
             */

            for (size_t iteration = 0; iteration < descriptor.m_iterations; ++iteration)
            {
                bool doPrint = false;

                // 51% chance of doing an add. Biased towards adds so we fill up the allocator.
                if ((random.GetRandom() % 100) <= 51)
                {
                    const size_t AllocationSize = (AllocationSizeRange ? (random.GetRandom() % AllocationSizeRange) : 0) + descriptor.m_allocationSizeMin;
                    const size_t AllocationBlockCount = GetBytesToBlocks(AllocationSize, descriptor.m_byteAlignmentBase);

                    RHI::VirtualAddress address = descriptor.m_allocator->Allocate(AllocationSize, descriptor.m_byteAlignmentBase);

                    // Allocator has space. Record the allocation.
                    if (address.IsValid())
                    {
                        ASSERT_TRUE(address.m_ptr % descriptor.m_byteAlignmentBase == 0);

                        size_t addressOffset = address.m_ptr - descriptor.m_addressBase;

                        for (size_t currentBlock = 0; currentBlock < AllocationBlockCount; ++currentBlock)
                        {
                            size_t blockIndex = GetBytesToBlocks(addressOffset, descriptor.m_byteAlignmentBase) + currentBlock;

                            ASSERT_TRUE(m_usedBlocks[blockIndex].m_used == 0);
                            m_usedBlocks[blockIndex].m_used = 1;
                        }

                        currentAllocations.push_back(Allocation{ address, AllocationSize });
                        doPrint = true;
                    }
                    else
                    {
                        ASSERT_TRUE(currentAllocations.size() || retiredAllocationsCurrent.size());
                    }
                }
                else if (currentAllocations.size())
                {
                    // pick a random allocation to remove
                    size_t allocationIndex = random.GetRandom() % currentAllocations.size();

                    Allocation allocation = currentAllocations[allocationIndex];
                    currentAllocations.erase(currentAllocations.begin() + allocationIndex);

                    descriptor.m_allocator->DeAllocate(allocation.address);

                    retiredAllocationsCurrent.push_back(allocation);
                    doPrint = true;
                }

                if ((random.GetRandom() % 4) == 0)
                {
                    descriptor.m_allocator->GarbageCollect();
                    doPrint = true;

                    retiredAllocationsPrevious.clear();
                    for (Allocation retiredAllocation : retiredAllocationsCurrent)
                    {
                        size_t addressOffset = retiredAllocation.address.m_ptr - descriptor.m_addressBase;
                        size_t blockIndex = addressOffset / descriptor.m_byteAlignmentBase;

                        const size_t AllocationBlockCount = GetBytesToBlocks(retiredAllocation.size, descriptor.m_byteAlignmentBase);

                        // Just use the root block as the bookmarked item.
                        ASSERT_TRUE(m_usedBlocks[blockIndex].m_used == 1);
                        m_usedBlocks[blockIndex].m_gcIteration++;

                        if (m_usedBlocks[blockIndex].m_gcIteration > descriptor.m_gcLatency)
                        {
                            for (size_t currentBlock = 0; currentBlock < AllocationBlockCount; ++currentBlock)
                            {
                                size_t subBlockIndex = GetBytesToBlocks(addressOffset, descriptor.m_byteAlignmentBase) + currentBlock;

                                m_usedBlocks[subBlockIndex].m_used = 0;
                                m_usedBlocks[subBlockIndex].m_gcIteration = 0;
                            }
                        }
                        else
                        {
                            retiredAllocationsPrevious.push_back(retiredAllocation);
                        }
                    }

                    retiredAllocationsCurrent.swap(retiredAllocationsPrevious);

                    if (descriptor.m_useVisualizer)
                    {
                        PRINTF("GC...\n");
                    }
                }

                ASSERT_TRUE((retiredAllocationsCurrent.size() + currentAllocations.size()) == descriptor.m_allocator->GetAllocationCount());

                if (doPrint)
                {
                    for (size_t i = 0; i < BlockCount; ++i)
                    {
                        outputString[i] = m_usedBlocks[i].m_used ? 'x' : '-';
                    }

                    if (descriptor.m_useVisualizer)
                    {
                        PRINTF("%s\n", outputString.c_str());
                    }
                }
            }
        }
    };

    TEST_F(AllocatorTest, PoolAllocator)
    {
        RHI::PoolAllocator::Descriptor descriptor;
        descriptor.m_elementSize = 128;
        descriptor.m_capacityInBytes = 128 * descriptor.m_elementSize;
        descriptor.m_garbageCollectLatency = 2;
        descriptor.m_addressBase = RHI::VirtualAddress::CreateFromPointer((void*)0xffffffffeeee0100);

        RHI::PoolAllocator allocator;
        allocator.Init(descriptor);

        AllocatorTest::TestDescriptor testDescriptor;
        testDescriptor.m_byteCountMax = descriptor.m_capacityInBytes;
        testDescriptor.m_byteAlignmentBase = 128;
        testDescriptor.m_gcLatency = descriptor.m_garbageCollectLatency;
        testDescriptor.m_allocator = &allocator;
        testDescriptor.m_useVisualizer = false;
        testDescriptor.m_iterations = 10000;
        testDescriptor.m_allocationSizeMin = descriptor.m_elementSize;
        testDescriptor.m_allocationSizeMax = descriptor.m_elementSize;
        testDescriptor.m_addressBase = descriptor.m_addressBase.m_ptr;

        run(testDescriptor);
    }

    TEST_F(AllocatorTest, FirstFitAllocator)
    {
        RHI::FreeListAllocator::Descriptor descriptor;
        descriptor.m_capacityInBytes = 64 * 1024;
        descriptor.m_alignmentInBytes = 256;
        descriptor.m_garbageCollectLatency = 2;
        descriptor.m_addressBase = RHI::VirtualAddress::CreateFromPointer((void*)0xffffffffeeee0100);
        descriptor.m_policy = RHI::FreeListAllocatorPolicy::FirstFit;

        RHI::FreeListAllocator allocator;
        allocator.Init(descriptor);

        AllocatorTest::TestDescriptor testDescriptor;
        testDescriptor.m_byteCountMax = descriptor.m_capacityInBytes;
        testDescriptor.m_byteAlignmentBase = descriptor.m_alignmentInBytes;
        testDescriptor.m_gcLatency = descriptor.m_garbageCollectLatency;
        testDescriptor.m_allocator = &allocator;
        testDescriptor.m_useVisualizer = false;
        testDescriptor.m_iterations = 10000;
        testDescriptor.m_addressBase = descriptor.m_addressBase.m_ptr;
        run(testDescriptor);

        descriptor.m_garbageCollectLatency = 0;
        descriptor.m_addressBase = RHI::VirtualAddress::CreateFromOffset(1024);
        allocator.Init(descriptor);

        testDescriptor.m_gcLatency = descriptor.m_garbageCollectLatency;
        testDescriptor.m_addressBase = descriptor.m_addressBase.m_ptr;
        run(testDescriptor);
    }

    TEST_F(AllocatorTest, BestFitAllocator)
    {
        RHI::FreeListAllocator::Descriptor descriptor;
        descriptor.m_capacityInBytes = 64 * 1024;
        descriptor.m_alignmentInBytes = 256;
        descriptor.m_garbageCollectLatency = 2;
        descriptor.m_addressBase = RHI::VirtualAddress::CreateFromPointer((void*)0xffffffffeeee0100);
        descriptor.m_policy = RHI::FreeListAllocatorPolicy::BestFit;

        RHI::FreeListAllocator allocator;
        allocator.Init(descriptor);

        AllocatorTest::TestDescriptor testDescriptor;
        testDescriptor.m_byteCountMax = descriptor.m_capacityInBytes;
        testDescriptor.m_byteAlignmentBase = descriptor.m_alignmentInBytes;
        testDescriptor.m_gcLatency = descriptor.m_garbageCollectLatency;
        testDescriptor.m_allocator = &allocator;
        testDescriptor.m_useVisualizer = false;
        testDescriptor.m_iterations = 10000;
        testDescriptor.m_addressBase = descriptor.m_addressBase.m_ptr;
        run(testDescriptor);

        descriptor.m_garbageCollectLatency = 0;
        descriptor.m_addressBase = RHI::VirtualAddress::CreateFromOffset(1024);
        allocator.Init(descriptor);

        testDescriptor.m_gcLatency = descriptor.m_garbageCollectLatency;
        testDescriptor.m_addressBase = descriptor.m_addressBase.m_ptr;
        run(testDescriptor);
    }

    TEST_F(AllocatorTest, FreeListFragmentation)
    {
        // There are several ways to measure fragmentation, with varying degrees of accuracy (at the
        // expense of cost). The free list fragmentation computation uses a relatively simple scheme
        // that relates the available capacity with the largest blocksize (1 minus this ratio).

        // Create an allocator featuring 4 contiguous 256 byte blocks
        RHI::FreeListAllocator::Descriptor descriptor;
        descriptor.m_capacityInBytes = 1024;
        descriptor.m_alignmentInBytes = 256;
        descriptor.m_policy = RHI::FreeListAllocatorPolicy::FirstFit;

        RHI::FreeListAllocator allocator;
        allocator.Init(descriptor);

        // An allocator without any allocations reports 0 fragmentation
        ASSERT_EQ(allocator.ComputeFragmentation(), 0.f);

        auto address0 = allocator.Allocate(256, 0);
        // After allocating a single block as above, the remaining memory in the allocator remains
        // contiguous, so fragmentation remains 0
        ASSERT_EQ(allocator.ComputeFragmentation(), 0.f);

        allocator.Allocate(256, 0);
        // Same after the second allocation. The free memory is one large block at the end
        ASSERT_EQ(allocator.ComputeFragmentation(), 0.f);

        allocator.DeAllocate(address0);
        allocator.GarbageCollect();

        // Now, we have two free blocks. The large block represents 2/3rds of the available free space,
        // so we expect 1/3 to be the reported fragmentation.
        ASSERT_NEAR(allocator.ComputeFragmentation(), 1.f / 3, 1e-6f);

        allocator.Allocate(512, 0);

        // We've now occupied the last two blocks, so we once again expect 0 fragmentation
        ASSERT_EQ(allocator.ComputeFragmentation(), 0.f);
    }
}
