/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/


#include <AWSNativeSDKInit/AWSMemoryInterface.h>

namespace AWSNativeSDKInit
{
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
    const char* MemoryManager::AWS_API_ALLOC_TAG = "AwsApi";

    void MemoryManager::Begin() 
    {
        // We can't guarantee that all uses cases have/will Create a SystemAllocator which is a dependency of 
        // the AWSNativeSDKAllocator in SystemAllocator::Create
        if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();  
            m_systemAllocatorCreated = true;
        }
        AWSNativeSDKAllocator::Descriptor desc;
        m_allocator.Create(desc);
    }

    void MemoryManager::End() 
    {
        m_allocator.Destroy();

        if (m_systemAllocatorCreated)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    }

    void* MemoryManager::AllocateMemory(std::size_t blockSize, std::size_t alignment, const char* allocationTag) 
    {
        return m_allocator->Allocate(blockSize, alignment, 0, allocationTag);
    }

    void MemoryManager::FreeMemory(void* memoryPtr) 
    {
        m_allocator->DeAllocate(memoryPtr);
    }
#endif
}
