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
#include <RHI/Memory.h>
#include <RHI/MemoryTypeView.h>

namespace AZ
{
    namespace Vulkan
    {
        using MemoryAllocation = RHI::MemoryAllocation<Memory>;

        class MemoryView :
            public MemoryTypeView<Memory>
        {
            using Base = MemoryTypeView<Memory>;

        public:
            MemoryView() = default;

            MemoryView(const Base& memoryView)
                : Base(memoryView)
            {}

            MemoryView(RHI::Ptr<Memory> memory, size_t offset, size_t size, size_t alignment, MemoryAllocationType memoryType)
                : Base(memory, offset, size, alignment, memoryType)
            {}

            Memory* GetMemory() const { return GetAllocation().m_memory.get(); }
        };
    }
}
