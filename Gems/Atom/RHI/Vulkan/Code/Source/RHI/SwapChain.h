/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            //////////////////////////////////////////////////////////////////////

            RHI::ResultCode BuildSurface(const RHI::SwapChainDescriptor& descriptor);
            bool ValidateSurfaceDimensions(const RHI::SwapChainDimensions& dimensions);
            VkSurfaceFormatKHR GetSupportedSurfaceFormat(const RHI::Format format) const;
            VkPresentModeKHR GetSupportedPresentMode() const;
            VkCompositeAlphaFlagBitsKHR GetSupportedCompositeAlpha() const;
            RHI::ResultCode BuildNativeSwapChain(const RHI::SwapChainDimensions& dimensions);
            RHI::ResultCode AcquireNewImage(uint32_t* acquiredImageIndex);

            void InvalidateSurface();
            void InvalidateNativeSwapChain();

            VkSwapchainKHR m_nativeSwapChain = VK_NULL_HANDLE;
            RHI::Ptr<WSISurface> m_surface;
            CommandQueue* m_presentationQueue = nullptr;
            VkSurfaceFormatKHR m_surfaceFormat = {};
            VkSurfaceCapabilitiesKHR m_surfaceCapabilities;

            FrameContext m_currentFrameContext;

            struct SwapChainBarrier
            {
                VkPipelineStageFlags m_srcPipelineStages = 0;
                VkPipelineStageFlags m_dstPipelineStages = 0;
                VkImageMemoryBarrier m_barrier = {};
                bool m_isValid = false;
            } m_swapChainBarrier;

            AZStd::vector<VkImage> m_swapchainNativeImages;
        };
    }
}
