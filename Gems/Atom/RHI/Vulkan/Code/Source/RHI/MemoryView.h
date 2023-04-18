/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/MemoryAllocation.h>
#include <RHI/MemoryTypeView.h>

namespace AZ
{
    namespace Vulkan
    {
        class MemoryView :
            public MemoryTypeView<MemoryAllocation>
        {
            using Base = MemoryTypeView<MemoryAllocation>;

        public:
            MemoryView() = default;

            MemoryView(const Base& memoryView)
                : Base(memoryView)
            {}

            MemoryView(RHI::Ptr<MemoryAllocation> memory, size_t offset, size_t size)
                : Base(memory, offset, size)
            {}

            size_t GetVKMemoryOffset() const
            {
                return GetAllocation()->GetOffset() + GetOffset();
            }
        };
    }
}
