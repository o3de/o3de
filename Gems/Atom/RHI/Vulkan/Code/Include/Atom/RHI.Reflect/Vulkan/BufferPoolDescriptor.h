/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <vulkan/vulkan.h>

namespace AZ
{
    namespace Vulkan
    {
        class BufferPoolDescriptor final
            : public RHI::BufferPoolDescriptor
        {
            using Base = RHI::BufferPoolDescriptor;
        public:
            AZ_RTTI(BufferPoolDescriptor, "728C4498-2FEC-43F5-9E88-410B93E7CAD7", Base);
            BufferPoolDescriptor();
            static void Reflect(AZ::ReflectContext* context);

            VkDeviceSize m_bufferPoolPageSizeInBytes = RHI::DefaultValues::Memory::BufferPoolPageSizeInBytes;
            VkMemoryPropertyFlags m_additionalMemoryPropertyFlags = 0;
        };
    }
}
