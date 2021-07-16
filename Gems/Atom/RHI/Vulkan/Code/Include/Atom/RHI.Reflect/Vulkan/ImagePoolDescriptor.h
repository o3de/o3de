/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
