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

#include <Atom/RHI/MemoryAllocation.h>
#include <RHI/BufferMemory.h>
#include <RHI/MemoryTypeView.h>

namespace AZ
{
    namespace Vulkan
    {
        using BufferMemoryAllocation = RHI::MemoryAllocation<BufferMemory>;

        //! Represents View of a BufferMemory object.
        class BufferMemoryView :
            public MemoryTypeView<BufferMemory>
        {
            using Base = MemoryTypeView<BufferMemory>;

        public:
            BufferMemoryView() = default;

            BufferMemoryView(const Base& memoryView)
                : Base(memoryView)
            {}

            BufferMemoryView(RHI::Ptr<BufferMemory> memory, size_t offset, size_t size, size_t alignment, MemoryAllocationType memoryType)
                : Base(memory, offset, size, alignment, memoryType)
            {}

            //! Returns the BufferMemory object that this view references.
            BufferMemory* GetBufferMemory() const { return GetAllocation().m_memory.get(); }

            //! Returns the Vulkan's Buffer that the this view references.
            VkBuffer GetNativeBuffer() const { return GetBufferMemory()->GetNativeBuffer(); }
        };
    }
}
