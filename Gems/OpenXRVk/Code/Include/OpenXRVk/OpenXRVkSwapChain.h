/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <XR/XRSwapChain.h>
#include <OpenXRVk_Platform.h>

namespace OpenXRVk
{
    //! Class that will help manage native xr swapchains and swapchain images
    class SwapChain final
        : public XR::SwapChain
    {
    public:
        AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator);
        AZ_RTTI(SwapChain, "{3DD88236-8C9F-4864-86F5-018C198BC07E}", XR::SwapChain);

        static XR::Ptr<SwapChain> Create();

        //! This class helps manage the native swapchain image. 
        class Image final
            : public XR::SwapChain::Image
        {
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);
            AZ_RTTI(Image, "{717ABDD4-C050-4FDF-8E93-3784F81FE315}", XR::SwapChain::Image);

            static XR::Ptr<Image> Create();

            AZ::RHI::ResultCode Init(XrSwapchainImageVulkan2KHR swapchainImage);
            VkImage GetNativeImage();
        private:
            XrSwapchainImageVulkan2KHR m_swapchainImage;
        };

        //! This class helps manage the native swapchain for a given view. 
        class View final
            : public XR::SwapChain::View
        {
        public:
            AZ_CLASS_ALLOCATOR(View, AZ::SystemAllocator);
            AZ_RTTI(View, "{F8312427-AC2D-4737-9A8F-A16ADA5319D0}", XR::SwapChain::View);

            static XR::Ptr<View> Create();
            
            AZ::RHI::ResultCode Init(XrSwapchain handle, AZ::u32 width, AZ::u32 height);
            XrSwapchain GetSwapChainHandle() const;
            //! Destroy native swapchain
            void Shutdown() override;

            //! swapchain specific accessor functions
            AZ::u32 GetWidth() const;
            AZ::u32 GetHeight() const;
            AZ::u32 GetCurrentImageIndex() const override;

            //! Native swapChain image data
            AZStd::vector<XrSwapchainImageBaseHeader*> m_swapChainImageHeaders;
            AZStd::vector<XrSwapchainImageVulkan2KHR> m_swapchainImages;
        private:
            XrSwapchain m_handle = XR_NULL_HANDLE;
        };

        // XR::SwapChain overrides...
        AZ::RHI::ResultCode GetSwapChainImage(AZ::RHI::XRSwapChainDescriptor* swapchainDescriptor) const override;
        AZ::u32 GetSwapChainWidth(AZ::u32 viewIndex) const override;
        AZ::u32 GetSwapChainHeight(AZ::u32 viewIndex) const override;
        AZ::RHI::Format GetSwapChainFormat(AZ::u32 viewIndex) const override;

        //! Get the view configurations supported by the drivers
        AZStd::vector<XrViewConfigurationView> GetViewConfigs() const;
        
    private:

        //! Initialize all the native SwapChain and SwapChain images per view.
        AZ::RHI::ResultCode InitInternal() override;

        //! Destroy native objects
        void ShutdownInternal() override;

        //! Return supported swapchain image format
        VkFormat SelectColorSwapChainFormat(const AZStd::vector<int64_t>& runtimeFormats) const;
        
        AZStd::vector<XrViewConfigurationView> m_configViews;
        VkFormat m_colorSwapChainFormat{ VK_FORMAT_UNDEFINED };
        AZ::u32 m_mipCount = 1;
        AZ::u32 m_faceCount = 1;
        AZ::u32 m_arraySize = 1;

        //Todo: Add support up higher sample counts in case on MSAA pipeline
        VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT; 
    };
}
