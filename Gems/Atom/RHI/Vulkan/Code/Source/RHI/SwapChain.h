/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SwapChain.h>
#include <Atom/RHI.Reflect/SwapChainDescriptor.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>
#include <RHI/Semaphore.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/Framebuffer.h>
#include <RHI/RenderPass.h>
#include <RHI/WSISurface.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class CommandQueue;

        class SwapChain final
            : public RHI::SwapChain
        {
            using Base = RHI::SwapChain;

        public:
            AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator, 0);
            AZ_RTTI(SwapChain, "4AE7AE82-BB25-4665-BF79-85F407255B26", Base);

            struct FrameContext
            {
                Semaphore* m_imageAvailableSemaphore = nullptr;
                Semaphore* m_presentableSemaphore = nullptr;
            };

            static RHI::Ptr<SwapChain> Create();
            ~SwapChain() = default;

            VkSwapchainKHR GetNativeSwapChain() const;
            const FrameContext& GetCurrentFrameContext() const;
            const WSISurface& GetSurface() const;
            const CommandQueue& GetPresentationQueue() const;

            void QueueBarrier(const VkPipelineStageFlags src, const VkPipelineStageFlags dst, const VkImageMemoryBarrier& imageBarrier);

        private:
            SwapChain() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////
            // RHI::SwapChain
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::SwapChainDescriptor& descriptor, RHI::SwapChainDimensions* nativeDimensions) override;
            void ShutdownInternal() override;
            RHI::ResultCode InitImageInternal(const RHI::SwapChain::InitImageRequest& request) override;
            RHI::ResultCode ResizeInternal(const RHI::SwapChainDimensions& dimensions, RHI::SwapChainDimensions* nativeDimensions) override;
            uint32_t PresentInternal() override;
            void SetVerticalSyncIntervalInternal(uint32_t previousVsyncInterval) override;
            //////////////////////////////////////////////////////////////////////

            RHI::ResultCode BuildSurface(const RHI::SwapChainDescriptor& descriptor);

            //! Returns true is the swapchain dimensions are supported by the current surface.
            bool ValidateSurfaceDimensions(const RHI::SwapChainDimensions& dimensions);
            //! Returns the corresponding Vulkan format that is supported by the surface.
            //! If such format is not found, return the first supported format from the surface.
            VkSurfaceFormatKHR GetSupportedSurfaceFormat(const RHI::Format format) const;
            //! Returns the correct presentation mode.
            //! If verticalSyncInterval is non-zero, returns VK_PRESENT_MODE_FIFO_KHR.
            //! Otherwise, choose preferred mode if they are supported.
            //! If not, the first supported present mode is returned.
            VkPresentModeKHR GetSupportedPresentMode(uint32_t verticalSyncInterval) const;
            //! Returns the preferred alpha compositing modes if they are supported.
            //! If not, error will be reported.
            VkCompositeAlphaFlagBitsKHR GetSupportedCompositeAlpha() const;
            //! Returns the current surface capabilities.
            VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();
            //! Create the swapchain when initializing, or
            //! swapchain is no longer compatible or is sub-optimal with the surface.
            RHI::ResultCode CreateSwapchain();
            //! Build underlying Vulkan swapchain.
            RHI::ResultCode BuildNativeSwapChain(const RHI::SwapChainDimensions& dimensions);
            //! Retrieve the index of the next available presentable image.
            RHI::ResultCode AcquireNewImage(uint32_t* acquiredImageIndex);

            //! Destroy the surface.
            void InvalidateSurface();
            //! Destroy the old swapchain.
            void InvalidateNativeSwapChain();

            RHI::Ptr<WSISurface> m_surface;
            VkSwapchainKHR m_nativeSwapChain = VK_NULL_HANDLE;
            CommandQueue* m_presentationQueue = nullptr;
            FrameContext m_currentFrameContext;

            //! Swapchain data
            VkSurfaceFormatKHR m_surfaceFormat = {};
            VkSurfaceCapabilitiesKHR m_surfaceCapabilities = {};
            VkPresentModeKHR m_presentMode = {};
            VkCompositeAlphaFlagBitsKHR m_compositeAlphaFlagBits = {}; 
            AZStd::vector<VkImage> m_swapchainNativeImages;
            RHI::SwapChainDimensions m_dimensions;

            struct SwapChainBarrier
            {
                VkPipelineStageFlags m_srcPipelineStages = 0;
                VkPipelineStageFlags m_dstPipelineStages = 0;
                VkImageMemoryBarrier m_barrier = {};
                bool m_isValid = false;
            } m_swapChainBarrier;
        };
    }
}
