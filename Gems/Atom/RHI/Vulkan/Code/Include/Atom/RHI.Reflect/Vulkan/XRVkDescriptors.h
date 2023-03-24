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
        AZ_CLASS_ALLOCATOR(XRInstanceDescriptor, AZ::SystemAllocator);
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
            GladVulkanContext m_context;
        } m_outputData;

    };

    //! This class is used as a container for transferring physical
    //! data between RHI::Vulkan and XR::Vulkan
    class XRPhysicalDeviceDescriptor final
        : public RHI::XRPhysicalDeviceDescriptor
    {
        using Base = RHI::XRPhysicalDeviceDescriptor;

    public:
        AZ_CLASS_ALLOCATOR(XRPhysicalDeviceDescriptor, AZ::SystemAllocator);
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
        AZ_CLASS_ALLOCATOR(XRDeviceDescriptor, AZ::SystemAllocator);
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
            GladVulkanContext m_context;
        } m_outputData;
    };


    class XRSessionDescriptor final : public RHI::XRSessionDescriptor
    {
        using Base = RHI::XRSessionDescriptor;

    public:
        AZ_CLASS_ALLOCATOR(XRSessionDescriptor, AZ::SystemAllocator);
        AZ_RTTI(XRSessionDescriptor, "{2859EB21-D013-496B-B922-7C903BC377CF}", Base);

        XRSessionDescriptor() = default;
        ~XRSessionDescriptor() = default;

        struct GraphicsBinding
        {
            uint32_t m_queueFamilyIndex = 0;
            uint32_t m_queueIndex = 0;
        };

        // Provided by the RHI::Vulkan backend
        struct
        {
            GraphicsBinding m_graphicsBinding; 
        } m_inputData;
    };


    class XRSwapChainDescriptor final
        : public RHI::XRSwapChainDescriptor
    {
        using Base = RHI::XRSwapChainDescriptor;

    public:
        AZ_CLASS_ALLOCATOR(XRSwapChainDescriptor, AZ::SystemAllocator);
        AZ_RTTI(XRSwapChainDescriptor, "{358AA524-8EAF-40A9-A2B3-5D6916B72DA1}", Base);

        XRSwapChainDescriptor() = default;
        ~XRSwapChainDescriptor() = default;

        // Provided by the XR::Vulkan backend
        struct
        {
            uint32_t m_swapChainIndex = 0;
            uint32_t m_swapChainImageIndex = 0;
        } m_inputData;

        struct
        {
            VkImage m_nativeImage = VK_NULL_HANDLE;
        } m_outputData;
    };
} // namespace AZ::RHI
