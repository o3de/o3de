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
    void* PxAzAllocatorCallback::allocate(size_t size, [[maybe_unused]] const char* typeName, const char* filename, int line)
    {
        // PhysX requires 16-byte alignment
        void* ptr = AZ::AllocatorInstance<PhysXAllocator>::Get().Allocate(size, 16, 0, "PhysX", filename, line);
        AZ_Assert((reinterpret_cast<size_t>(ptr) & 15) == 0, "PhysX requires 16-byte aligned memory allocations.");
        return ptr;
    }

    void PxAzAllocatorCallback::deallocate(void* ptr)
    {
        AZ::AllocatorInstance<PhysXAllocator>::Get().DeAllocate(ptr);
    }
}
