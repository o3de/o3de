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

#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <vulkan/vulkan.h>

namespace AZ
{
    namespace Vulkan
    {
        class ImagePoolDescriptor final
            : public RHI::ImagePoolDescriptor
        {
            using Base = RHI::ImagePoolDescriptor;
        public:
            AZ_RTTI(ImagePoolDescriptor, "12CD3885-F2B7-40FF-87F1-03EF57749328", Base);
            ImagePoolDescriptor();
            static void Reflect(AZ::ReflectContext* context);

            VkDeviceSize m_imagePageSizeInBytes = RHI::DefaultValues::Memory::ImagePoolPageSizeInBytes;

            RHI::HeapMemoryLevel m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
        };
    }
}
