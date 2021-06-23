/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AudioAllocators.h>
#include <AudioLogger.h>
#include <SoundCVars.h>

namespace Audio::Platform
{
    void InitializeAudioAllocators()
    {
        // Create audio system memory pool
        if (!AZ::AllocatorInstance<AudioSystemAllocator>::IsReady())
        {
            const size_t heapSize = Audio::CVars::s_ATLMemorySize << 10;

            AudioSystemAllocator::Descriptor allocDesc;

            // Generic Allocator:
            allocDesc.m_allocationRecords = true;
            allocDesc.m_heap.m_numFixedMemoryBlocks = 1;
            allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = heapSize;

            allocDesc.m_heap.m_fixedMemoryBlocks[0] = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(
                allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0],
                allocDesc.m_heap.m_memoryBlockAlignment
            );

            AZ::AllocatorInstance<AudioSystemAllocator>::Create(allocDesc);
        }

        // Create the Bank allocator...
        if (!AZ::AllocatorInstance<AudioBankAllocator>::IsReady())
        {
            const size_t heapSize = Audio::CVars::s_FileCacheManagerMemorySize << 10;

            AudioBankAllocator::Descriptor allocDesc;
            allocDesc.m_allocationRecords = true;
            allocDesc.m_heap.m_numFixedMemoryBlocks = 1;
            allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = heapSize;
            allocDesc.m_heap.m_fixedMemoryBlocks[0] = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(
                allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0],
                allocDesc.m_heap.m_memoryBlockAlignment
            );

            AZ::AllocatorInstance<AudioBankAllocator>::Create(allocDesc);
        }
    }

    void ShutdownAudioAllocators()
    {
        if (AZ::AllocatorInstance<Audio::AudioBankAllocator>::IsReady())
        {
            AZ::AllocatorInstance<Audio::AudioBankAllocator>::Destroy();
        }

        if (AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
        {
            AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Destroy();
        }
    }

} // namespace Audio::Platform
