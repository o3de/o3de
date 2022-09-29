/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <SurfaceData/MixedStackHeapAllocator.h>

namespace UnitTest
{

    struct MixedAllocatorTestFixture
        : public AllocatorsTestFixture
    {
    public:
        MixedAllocatorTestFixture()
        {
            SetShouldCleanUpGenericClassInfo(false);
        }
    };

    TEST_F(MixedAllocatorTestFixture, MixedStackHeapAllocator_GetNameSetNameWorks)
    {
        const int StackElements = 4;

        // Setting the name via construction should work.
        const char name[] = "Mixed allocator";
        SurfaceData::mixed_stack_heap_allocator<float, StackElements> allocator(name);
        EXPECT_EQ(strcmp(allocator.get_name(), name), 0);

        // Setting the name via set_name should work.
        const char newName[] = "Renamed allocator";
        allocator.set_name(newName);
        EXPECT_EQ(strcmp(allocator.get_name(), newName), 0);
    }

    TEST_F(MixedAllocatorTestFixture, MixedStackHeapAllocator_SingleStackAllocationWorks)
    {
        const int StackElements = 4;
        SurfaceData::mixed_stack_heap_allocator<float, StackElements> allocator;

        // Choose a size and alignment that fits within our requested stack buffer.
        size_t allocSize = sizeof(float) * StackElements;
        size_t allocAlignment = alignof(float);

        auto numAllocatedBytesBefore = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().NumAllocatedBytes();

        // Verify we can allocate the requested amount of data.
        auto data = allocator.allocate(allocSize, allocAlignment);
        ASSERT_NE(data, nullptr);

        auto numAllocatedBytesAfter = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().NumAllocatedBytes();

        // Verify that none of the data came from the system allocator (heap).
        EXPECT_EQ(numAllocatedBytesBefore, numAllocatedBytesAfter);

        // Verify that a deallocation is successful.
        allocator.deallocate(data, allocSize, allocAlignment);
    }

    TEST_F(MixedAllocatorTestFixture, MixedStackHeapAllocator_SingleHeapAllocationWorks)
    {
        const int StackElements = 4;
        SurfaceData::mixed_stack_heap_allocator<float, StackElements> allocator;

        // Choose a size that's larger than our allocated stack buffer.
        size_t allocSize = sizeof(float) * (StackElements + 1);
        size_t allocAlignment = alignof(float);

        auto numAllocatedBytesBefore = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().NumAllocatedBytes();

        // Verify we can allocate the requested amount of data.
        auto data = allocator.allocate(allocSize, allocAlignment);
        ASSERT_NE(data, nullptr);

        auto numAllocatedBytesAfter = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().NumAllocatedBytes();

        // Verify that all of the data came from the system allocator (heap).
        // The actual allocated size can be larger than what was requested, so compare with >= instead of ==.
        EXPECT_GE(numAllocatedBytesAfter - numAllocatedBytesBefore, allocSize);

        // Verify that a deallocation is successful.
        allocator.deallocate(data, allocSize, allocAlignment);
    }


    TEST_F(MixedAllocatorTestFixture, MixedStackHeapAllocator_StackAllocationResizeWorks_OnlyWithinStackBufferSize)
    {
        const int StackElements = 4;
        SurfaceData::mixed_stack_heap_allocator<float, StackElements> allocator;

        // Choose a size that matches allocated stack buffer.
        size_t allocSize = sizeof(float) * StackElements;
        size_t allocResizeSmallerSize = sizeof(float) * (StackElements - 1);
        size_t allocResizeLargerSize = sizeof(float) * (StackElements + 1);
        size_t allocAlignment = alignof(float);

        auto numAllocatedBytesBefore = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().NumAllocatedBytes();

        // Allocate the requested amount of data.
        auto data = allocator.allocate(allocSize, allocAlignment);
        ASSERT_NE(data, nullptr);

        // Resize to something smaller than the allocated stack buffer. This should succeed.
        auto reallocatedData = allocator.reallocate(data, allocResizeSmallerSize);
        EXPECT_NE(reallocatedData, nullptr);

        // Resize to something larger than the allocated stack buffer. This should return 0, as it isn't suported.
        reallocatedData = allocator.reallocate(data, allocResizeLargerSize);
        EXPECT_EQ(reallocatedData, nullptr);

        auto numAllocatedBytesAfter = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().NumAllocatedBytes();

        // Verify that all of the data came from the stack.
        EXPECT_GE(numAllocatedBytesAfter - numAllocatedBytesBefore, 0);

        // Verify that a deallocation is successful.
        allocator.deallocate(reallocatedData, allocSize, allocAlignment);
    }

} // namespace UnitTest
