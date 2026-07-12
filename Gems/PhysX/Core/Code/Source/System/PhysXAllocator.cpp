/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <System/PhysXAllocator.h>


namespace PhysX
{
    void* PxAzAllocatorCallback::allocate(size_t size, [[maybe_unused]] const char* typeName, [[maybe_unused]] const char* filename, [[maybe_unused]] int line)
    {
        // PhysX requires 16-byte alignment
        // void* ptr = AZ::AllocatorInstance<PhysXAllocator>::Get().Allocate(size, 16, 0, "PhysX", filename, line); // TODO: Verify for removal of depracated Allocate function
        void* ptr = AZ::AllocatorInstance<PhysXAllocator>::Get().allocate(size, 16);
        AZ_Assert((reinterpret_cast<size_t>(ptr) & 15) == 0, "PhysX requires 16-byte aligned memory allocations.");
        return ptr;
    }

    void PxAzAllocatorCallback::deallocate(void* ptr)
    {
        // AZ::AllocatorInstance<PhysXAllocator>::Get().DeAllocate(ptr); // TODO: Verify for removal of depracated DeAllocate function
        AZ::AllocatorInstance<PhysXAllocator>::Get().deallocate(ptr);
    }
}
