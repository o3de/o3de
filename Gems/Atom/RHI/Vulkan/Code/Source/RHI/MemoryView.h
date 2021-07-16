/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
