/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/XRRenderingInterface.h>

namespace AZ::Vulkan
{
    //! This class is used as a container for transferring instance
    //! data between RHI::Vulkan and XR::Vulkan
    class XRInstanceDescriptor final
        : public RHI::XRInstanceDescriptor
    {
        using Base = RHI::XRInstanceDescriptor;
    public:
        AZ_CLASS_ALLOCATOR(XRInstanceDescriptor, AZ::SystemAllocator, 0);
        AZ_RTTI(XRInstanceDescriptor, "{93DF070E-1423-4BBF-A9F3-136F9E543594}", Base);

        XRInstanceDescriptor() = default;
        ~XRInstanceDescriptor() = default;

        // Provided by the RHI::Vulkan backend
        struct 
        { 
            VkInstanceCreateInfo* m_createInfo = nullptr;
        } m_inputData;

        // Provided by the XR::Vulkan backend
        struct 
        {
            VkInstance m_xrVkInstance = VK_NULL_HANDLE;
        } m_outputData;

    };

    //! This class is used as a container for transferring physical
    //! data between RHI::Vulkan and XR::Vulkan
    class XRPhysicalDeviceDescriptor final
        : public RHI::XRPhysicalDeviceDescriptor
    {
        using Base = RHI::XRPhysicalDeviceDescriptor;

    public:
        AZ_CLASS_ALLOCATOR(XRPhysicalDeviceDescriptor, AZ::SystemAllocator, 0);
        AZ_RTTI(XRPhysicalDeviceDescriptor, "{D1567011-ACA7-430D-9C5D-E639E6E99A85}", Base);

        XRPhysicalDeviceDescriptor() = default;
        ~XRPhysicalDeviceDescriptor() = default;

        // Provided by the XR::Vulkan backend
        struct
        {
            VkPhysicalDevice m_xrVkPhysicalDevice = VK_NULL_HANDLE;
        } m_outputData;
    };

    //! This class is used as a container for transferring device
    //! data between RHI::Vulkan and XR::Vulkan
    class XRDeviceDescriptor final
        : public RHI::XRDeviceDescriptor
    {
        using Base = RHI::XRDeviceDescriptor;

    public:
        AZ_CLASS_ALLOCATOR(XRDeviceDescriptor, AZ::SystemAllocator, 0);
        AZ_RTTI(XRDeviceDescriptor, "{D1567011-ACA7-430D-9C5D-E639E6E99A85}", Base);

        XRDeviceDescriptor() = default;
        ~XRDeviceDescriptor() = default;

        // Provided by the RHI::Vulkan backend
        struct
        {
            VkDeviceCreateInfo* m_deviceCreateInfo = nullptr;
        } m_inputData;

        // Provided by the XR::Vulkan backend
        struct
        {
            VkDevice m_xrVkDevice = VK_NULL_HANDLE;
        } m_outputData;
    };
} // namespace AZ::RHI
