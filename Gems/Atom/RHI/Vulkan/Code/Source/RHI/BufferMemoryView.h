/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/BufferMemory.h>
#include <RHI/MemoryTypeView.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Represents View of a BufferMemory object.
        class BufferMemoryView :
            public MemoryTypeView<BufferMemory>
        {
            using Base = MemoryTypeView<BufferMemory>;

        public:
            BufferMemoryView() = default;

            BufferMemoryView(RHI::Ptr<BufferMemory> memory)
                : Base(memory)
            {
            }

            //! Returns the Vulkan's Buffer that the this view references.
            const VkBuffer GetNativeBuffer() const
            {
                return GetAllocation()->GetNativeBuffer();
            }

            VkDeviceMemory GetNativeDeviceMemory() const
            {
                return GetAllocation()->GetNativeDeviceMemory();
            }
        };
    }
}
