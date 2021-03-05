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
