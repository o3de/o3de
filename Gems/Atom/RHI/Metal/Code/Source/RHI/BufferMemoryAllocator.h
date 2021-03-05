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
#pragma once

#include <RHI/BufferMemoryView.h>
#include <RHI/MemorySubAllocator.h>

namespace AZ
{
    namespace Metal
    {
        class Device;

        class BufferMemoryAllocator
        {
        public:
            BufferMemoryAllocator() = default;
            BufferMemoryAllocator(const BufferMemoryAllocator&) = delete;

            using Descriptor = MemoryPageAllocator::Descriptor;

            void Init(const Descriptor& descriptor);

            void Shutdown();

            void GarbageCollect();

            BufferMemoryView Allocate(size_t sizeInBytes);
            void DeAllocate(const BufferMemoryView& memory);

        private:
            BufferMemoryView AllocateUnique(const RHI::BufferDescriptor& bufferDescriptor);
            void DeAllocateUnique(const BufferMemoryView& memoryView);

            Descriptor m_descriptor;
            MemoryPageAllocator m_pageAllocator;
            bool m_usePageAllocator = true;

            AZStd::mutex m_allocatorMutex;
            MemoryFreeListSubAllocator m_subAllocator;
            size_t m_subAllocationAlignment = Alignment::Buffer;
        };
    }
}
