/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
