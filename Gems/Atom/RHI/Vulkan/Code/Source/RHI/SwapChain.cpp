/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Atom_RHI_Vulkan_Platform.h"
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/Queue.h>
#include <RHI/RenderPass.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<SwapChain> SwapChain::Create()
        {
            return aznew SwapChain();
        }

        VkSwapchainKHR SwapChain::GetNativeSwapChain() const
        {
            return m_nativeSwapChain;
        }

        const SwapChain::FrameContext& SwapChain::GetCurrentFrameContext() const
        {
             return m_currentFrameContext;
        }

        const WSISurface& SwapChain::GetSurface() const
        {
            return *m_surface;
        }

        const CommandQueue& SwapChain::GetPresentationQueue() const
        {
            return *m_presentationQueue;
        }

        void SwapChain::QueueBarrier(const VkPipelineStageFlags src, const VkPipelineStageFlags dst, const VkImageMemoryBarrier& imageBarrier)
        {
            m_swapChainBarrier.m_barrier = imageBarrier;
            m_swapChainBarrier.m_srcPipelineStages = src;
            m_swapChainBarrier.m_dstPipelineStages = dst;
            m_swapChainBarrier.m_isValid = true;
        }

        void SwapChain::SetVerticalSyncIntervalInternal(uint32_t previousVsyncInterval)
        {
            uint32_t verticalSyncInterval = GetDescriptor().m_verticalSyncInterval;
            if (verticalSyncInterval == 0 || previousVsyncInterval == 0)
            {
                // The presentation mode may change when transitioning to or from a vsynced presentation mode
                // In this case, the swapchain must be recreated.
                InvalidateNativeSwapChain();
                BuildNativeSwapChain(GetDescriptor().m_dimensions, verticalSyncInterval);
            }
        }

        void SwapChain::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeSwapChain), name.data(), VK_OBJECT_TYPE_SWAPCHAIN_KHR, static_cast<Device&>(GetDevice()));
            }
        }

        RHI::ResultCode SwapChain::InitInternal(RHI::Device& baseDevice, const RHI::SwapChainDescriptor& descriptor, RHI::SwapChainDimensions* nativeDimensions)
        {
            RHI::ResultCode result = RHI::ResultCode::Success;
            RHI::DeviceObject::Init(baseDevice);

            auto& device = static_cast<Device&>(GetDevice());
            RHI::SwapChainDimensions swapchainDimensions = descriptor.m_dimensions;
            result = BuildSurface(descriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            if (!ValidateSurfaceDimensions(swapchainDimensions))
            {
                swapchainDimensions.m_imageHeight = AZStd::clamp(swapchainDimensions.m_imageHeight, m_surfaceCapabilities.minImageExtent.height, m_surfaceCapabilities.maxImageExtent.height);
                swapchainDimensions.m_imageWidth = AZStd::clamp(swapchainDimensions.m_imageWidth, m_surfaceCapabilities.minImageExtent.width, m_surfaceCapabilities.maxImageExtent.width);
                AZ_Printf("Vulkan", "Resizing swapchain from (%d, %d) to (%d, %d).",
                    static_cast<int>(descriptor.m_dimensions.m_imageWidth), static_cast<int>(descriptor.m_dimensions.m_imageHeight),
                    static_cast<int>(swapchainDimensions.m_imageWidth), static_cast<int>(swapchainDimensions.m_imageHeight));
            }

            auto& presentationQueue = device.GetCommandQueueContext().GetOrCreatePresentationCommandQueue(*this);
            m_presentationQueue = &presentationQueue;
            result = BuildNativeSwapChain(swapchainDimensions, descriptor.m_verticalSyncInterval);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            uint32_t imageCount = 0;
            VkResult vkResult = vkGetSwapchainImagesKHR(device.GetNativeDevice(), m_nativeSwapChain, &imageCount, nullptr);
            AssertSuccess(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            m_swapchainNativeImages.resize(imageCount);

            // Retrieve the native images of the swapchain so they are
            // available when we init the Images in InitImageInternal
            vkResult = vkGetSwapchainImagesKHR(device.GetNativeDevice(), m_nativeSwapChain, &imageCount, m_swapchainNativeImages.data());
            AssertSuccess(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            // Acquire the first image
            uint32_t imageIndex = 0;
            result = AcquireNewImage(&imageIndex);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            if (nativeDimensions)
            {
                // Fill out the real swapchain dimensions to return
                nativeDimensions->m_imageCount = imageCount;
                nativeDimensions->m_imageHeight = swapchainDimensions.m_imageHeight;
                nativeDimensions->m_imageWidth = swapchainDimensions.m_imageWidth;
                nativeDimensions->m_imageFormat = ConvertFormat(m_surfaceFormat.format);
            }

            SetName(GetName());
            return result;
        }

        void SwapChain::ShutdownInternal()
        {
            InvalidateNativeSwapChain();
            InvalidateSurface();

            m_presentationQueue = nullptr;

            m_swapchainNativeImages.clear();
            m_currentFrameContext = {};
        }

        RHI::ResultCode SwapChain::InitImageInternal(const RHI::SwapChain::InitImageRequest& request)
        {
            auto& device = static_cast<Device&>(GetDevice());
            Image* image = static_cast<Image*>(request.m_image);
            RHI::ImageDescriptor imageDesc = request.m_descriptor;
            imageDesc.m_format = ConvertFormat(m_surfaceFormat.format);
            RHI::ResultCode result = image->Init(device, m_swapchainNativeImages[request.m_imageIndex], imageDesc);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to initialize swapchain image %d", request.m_imageIndex);
                return result;
            }

            Name name(AZStd::string::format("SwapChainImage_%d", request.m_imageIndex));
            image->SetName(name);

            return result;
        }

        RHI::ResultCode SwapChain::ResizeInternal(const RHI::SwapChainDimensions& dimensions, RHI::SwapChainDimensions* nativeDimensions)
        {
            auto& device = static_cast<Device&>(GetDevice());

            InvalidateNativeSwapChain();
            InvalidateSurface();
            RHI::SwapChainDimensions resizeDimensions = dimensions;
            BuildSurface(GetDescriptor());
            if (!ValidateSurfaceDimensions(dimensions))
            {
                resizeDimensions.m_imageHeight = AZStd::clamp(dimensions.m_imageHeight, m_surfaceCapabilities.minImageExtent.height, m_surfaceCapabilities.maxImageExtent.height);
                resizeDimensions.m_imageWidth = AZStd::clamp(dimensions.m_imageWidth, m_surfaceCapabilities.minImageExtent.width, m_surfaceCapabilities.maxImageExtent.width);
                AZ_Printf("Vulkan", "Resizing swapchain from (%d, %d) to (%d, %d).",
                    static_cast<int>(dimensions.m_imageWidth), static_cast<int>(dimensions.m_imageHeight),
                    static_cast<int>(resizeDimensions.m_imageWidth), static_cast<int>(resizeDimensions.m_imageHeight));
            }

            auto& presentationQueue = device.GetCommandQueueContext().GetOrCreatePresentationCommandQueue(*this);
            m_presentationQueue = &presentationQueue;
            BuildNativeSwapChain(resizeDimensions, GetDescriptor().m_verticalSyncInterval);

            resizeDimensions.m_imageCount = 0;
            VkResult vkResult = vkGetSwapchainImagesKHR(device.GetNativeDevice(), m_nativeSwapChain, &resizeDimensions.m_imageCount, nullptr);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            m_swapchainNativeImages.resize(resizeDimensions.m_imageCount);

            // Retrieve the native images of the swapchain so they are
            // available when we init the Images in InitImageInternal
            vkResult = vkGetSwapchainImagesKHR(device.GetNativeDevice(), m_nativeSwapChain, &resizeDimensions.m_imageCount, m_swapchainNativeImages.data());
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            // Do not recycle the semaphore because they may not ever get signaled and since
            // we can't recycle Vulkan semaphores we just delete them.
            m_currentFrameContext.m_imageAvailableSemaphore->SetRecycleValue(false);
            m_currentFrameContext.m_presentableSemaphore->SetRecycleValue(false);

            // Acquire the first image
            uint32_t imageIndex = 0;
            AcquireNewImage(&imageIndex);

            if (nativeDimensions)
            {
                *nativeDimensions = resizeDimensions;
                // [ATOM-4840] This is a workaround when the windows is minimized (0x0 size).
                // Add proper support to handle this case.
                nativeDimensions->m_imageHeight = AZStd::max(resizeDimensions.m_imageHeight, 1u);
                nativeDimensions->m_imageWidth = AZStd::max(resizeDimensions.m_imageWidth, 1u);
            }

            return RHI::ResultCode::Success;
        }

        uint32_t SwapChain::PresentInternal()
        {
            auto& device = static_cast<Device&>(GetDevice());

            const uint32_t imageIndex = GetCurrentImageIndex();

            auto presentCommand = [this, imageIndex, presentSemaphore = m_currentFrameContext.m_presentableSemaphore, &device](void* queue)
            {
                Queue* vulkanQueue = static_cast<Queue*>(queue);
                VkSemaphore waitSemaphore = presentSemaphore->GetNativeSemaphore();
                if (m_swapChainBarrier.m_isValid)
                {
                    // The presentation and graphic queue belong to different families so
                    // we need to add an ownership transfer to the presentation queue.
                    auto commandList = device.AcquireCommandList(vulkanQueue->GetId().m_familyIndex);
                    commandList->BeginCommandBuffer();
                    vkCmdPipelineBarrier(commandList->GetNativeCommandBuffer(),
                        m_swapChainBarrier.m_srcPipelineStages,
                        m_swapChainBarrier.m_dstPipelineStages,
                        VK_DEPENDENCY_BY_REGION_BIT,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &m_swapChainBarrier.m_barrier);
                    commandList->EndCommandBuffer();

                    // This semaphore will be signaled once the transfer has completed.
                    auto transferSemaphore = device.GetSemaphoreAllocator().Allocate();
                    // We wait until the swapchain image has finished being rendered to initialize the
                    // ownership transfer.
                    vulkanQueue->SubmitCommandBuffers(
                        AZStd::vector<RHI::Ptr<CommandList>>{commandList},
                        AZStd::vector<Semaphore::WaitSemaphore>{AZStd::make_pair(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, presentSemaphore)},
                        AZStd::vector< RHI::Ptr<Semaphore>>{transferSemaphore},
                        nullptr);

                    // The presentation engine must wait until the ownership transfer has completed.
                    waitSemaphore = transferSemaphore->GetNativeSemaphore();
                    transferSemaphore->SignalEvent();
                    // This will not deallocate immediately. It has a collect latency.
                    device.GetSemaphoreAllocator().DeAllocate(transferSemaphore);
                    m_swapChainBarrier.m_isValid = false;
                }

                VkPresentInfoKHR info{};
                info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                info.pNext = nullptr;
                info.waitSemaphoreCount = 1;
                info.pWaitSemaphores = &waitSemaphore;
                info.swapchainCount = 1;
                info.pSwapchains = &m_nativeSwapChain;
                info.pImageIndices = &imageIndex;
                info.pResults = nullptr;

                [[maybe_unused]] const VkResult result = vkQueuePresentKHR(vulkanQueue->GetNativeQueue(), &info);

                // Resizing window cause recreation of SwapChain after calling this method,
                // so VK_SUBOPTIMAL_KHR or VK_ERROR_OUT_OF_DATE_KHR  should not happen at this point.
                AZ_Assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to present swapchain %s", GetName().GetCStr());
                AZ_Warning("Vulkan", result != VK_SUBOPTIMAL_KHR, "Suboptimal presentation of swapchain %s", GetName().GetCStr());
            };

            m_presentationQueue->QueueCommand(AZStd::move(presentCommand));

            uint32_t acquiredImageIndex = GetCurrentImageIndex();
            AcquireNewImage(&acquiredImageIndex);

            return acquiredImageIndex;
        }

        RHI::ResultCode SwapChain::BuildSurface(const RHI::SwapChainDescriptor& descriptor)
        {
            WSISurface::Descriptor surfaceDesc{};
            surfaceDesc.m_windowHandle = descriptor.m_window;
            RHI::Ptr<WSISurface> surface = WSISurface::Create();
            const RHI::ResultCode result = surface->Init(surfaceDesc);
            if (result == RHI::ResultCode::Success)
            {
                m_surface = surface;
                auto& device = static_cast<Device&>(GetDevice());
                const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
                VkResult vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &m_surfaceCapabilities);
                AssertSuccess(vkResult);
                RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));
            }

            return result;
        }

        bool SwapChain::ValidateSurfaceDimensions(const RHI::SwapChainDimensions& dimensions)
        {
            return (m_surfaceCapabilities.minImageExtent.width <= dimensions.m_imageWidth &&
                dimensions.m_imageWidth <= m_surfaceCapabilities.maxImageExtent.width &&
                m_surfaceCapabilities.minImageExtent.height <= dimensions.m_imageHeight &&
                dimensions.m_imageHeight <= m_surfaceCapabilities.maxImageExtent.height);
        }

        VkSurfaceFormatKHR SwapChain::GetSupportedSurfaceFormat(const RHI::Format rhiFormat) const
        {
            AZ_Assert(m_surface, "Surface has not been initialized.");
            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            uint32_t surfaceFormatCount = 0;
            AssertSuccess(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &surfaceFormatCount, nullptr));
            AZ_Assert(surfaceFormatCount > 0, "Surface support no format.");
            AZStd::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            AssertSuccess(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &surfaceFormatCount, surfaceFormats.data()));

            const VkFormat format = ConvertFormat(rhiFormat);
            for (uint32_t index = 0; index < surfaceFormatCount; ++index)
            {
                if (surfaceFormats[index].format == format)
                {
                    return surfaceFormats[index];
                }
            }
            AZ_Warning("Vulkan", false, "Given format is not supported, so it uses a supported format.");
            return surfaceFormats[0];
        }

        VkPresentModeKHR SwapChain::GetSupportedPresentMode(uint32_t verticalSyncInterval) const
        {
            AZ_Assert(m_surface, "Surface has not been initialized.");

            if (verticalSyncInterval > 0)
            {
                // When a non-zero vsync interval is requested, the FIFO presentation mode (always available)
                // is usable without needing to query available presentation modes.
                return VK_PRESENT_MODE_FIFO_KHR;
            }

            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());

            uint32_t modeCount = 0;
            AssertSuccess(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &modeCount, nullptr));
            // At least VK_PRESENT_MODE_FIFO_KHR have to be supported.
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPresentModeKHR.html
            AZ_Assert(modeCount > 0, "no available present mode.");
            AZStd::vector<VkPresentModeKHR> supportedModes(modeCount);
            AssertSuccess(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &modeCount, supportedModes.data()));

            VkPresentModeKHR preferredModes[] = {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR};
            for (VkPresentModeKHR preferredMode : preferredModes)
            {
                for (VkPresentModeKHR supportedMode : supportedModes)
                {
                    if (supportedMode == preferredMode)
                    {
                        return supportedMode;
                    }
                }
            }
            return supportedModes[0];
        }

        VkCompositeAlphaFlagBitsKHR SwapChain::GetSupportedCompositeAlpha() const
        {
            VkFlags supportedModesBits = m_surfaceCapabilities.supportedCompositeAlpha;
            VkCompositeAlphaFlagBitsKHR preferedModes[] = {
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR };

            for (VkCompositeAlphaFlagBitsKHR mode : preferedModes)
            {
                if (supportedModesBits & mode)
                {
                    return mode;
                }
            }

            AZ_Assert(false, "Could not find a supported composite alpha mode for the swapchain");
            return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }

        RHI::ResultCode SwapChain::BuildNativeSwapChain(const RHI::SwapChainDimensions& dimensions, uint32_t verticalSyncInterval)
        {
            AZ_Assert(m_nativeSwapChain == VK_NULL_HANDLE, "Vulkan's native SwapChain has been initialized already.");
            auto& device = static_cast<Device&>(GetDevice());
            auto& queueContext = device.GetCommandQueueContext();
            const VkExtent2D extent = {
                dimensions.m_imageWidth,
                dimensions.m_imageHeight
            };
            AZ_Assert(m_surface, "Surface is null.");

            if (!ValidateSurfaceDimensions(dimensions))
            {
                AZ_Assert(false, "Swapchain dimensions are not supported.");
                return RHI::ResultCode::InvalidArgument;
            }
            m_surfaceFormat = GetSupportedSurfaceFormat(dimensions.m_imageFormat);
            // If the graphic queue is the same as the presentation queue, then we will always acquire
            // 1 image at the same time. If it's another queue, we will have 2 at the same time (while the other queue
            // presents the image)
            auto graphicQueueId = queueContext.GetCommandQueue(RHI::HardwareQueueClass::Graphics).GetId();
            auto presentationQueueId = m_presentationQueue->GetId();
            AZStd::vector<uint32_t> familyIndices{ graphicQueueId.m_familyIndex };
            uint32_t simultaneousAcquiredImages = 1;
            if (graphicQueueId != presentationQueueId)
            {
                simultaneousAcquiredImages = 2;
                if (presentationQueueId.m_familyIndex != graphicQueueId.m_familyIndex)
                {
                    familyIndices.push_back(presentationQueueId.m_familyIndex);
                }
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.flags = 0; // [GFX TODO][ATOM-512] find appropriate flags
            createInfo.surface = m_surface->GetNativeSurface();
            // When acquiring an image the number of images that the application has currently acquired (but not yet presented)
            // need to be less than or equal to the difference between the number of images in swapchain and the value of VkSurfaceCapabilitiesKHR::minImageCount
            createInfo.minImageCount = AZStd::max(dimensions.m_imageCount, simultaneousAcquiredImages + m_surfaceCapabilities.minImageCount);
            createInfo.imageFormat = m_surfaceFormat.format;
            createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1; // non-stereoscopic
            createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = static_cast<uint32_t>(familyIndices.size());
            createInfo.pQueueFamilyIndices = familyIndices.empty() ? nullptr : familyIndices.data();
            createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            createInfo.compositeAlpha = GetSupportedCompositeAlpha();
            createInfo.presentMode = GetSupportedPresentMode(verticalSyncInterval);
            createInfo.clipped = VK_FALSE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            const VkResult result = vkCreateSwapchainKHR(device.GetNativeDevice(), &createInfo, nullptr, &m_nativeSwapChain);
            AssertSuccess(result);

            return ConvertResult(result);
        }

        RHI::ResultCode SwapChain::AcquireNewImage(uint32_t* acquiredImageIndex)
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto& semaphoreAllocator = device.GetSemaphoreAllocator();
            Semaphore* imageAvailableSemaphore = semaphoreAllocator.Allocate();
            VkResult vkResult = vkAcquireNextImageKHR(device.GetNativeDevice(),
                m_nativeSwapChain,
                UINT64_MAX,
                imageAvailableSemaphore->GetNativeSemaphore(),
                VK_NULL_HANDLE,
                acquiredImageIndex);

            // Resizing window cause recreation of SwapChain before calling this method,
            // so VK_SUBOPTIMAL_KHR or VK_ERROR_OUT_OF_DATE_KHR  should not happen.
            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            imageAvailableSemaphore->SignalEvent();
            if (m_currentFrameContext.m_imageAvailableSemaphore)
            {
                semaphoreAllocator.DeAllocate(m_currentFrameContext.m_imageAvailableSemaphore);
            }
            if (m_currentFrameContext.m_presentableSemaphore)
            {
                semaphoreAllocator.DeAllocate(m_currentFrameContext.m_presentableSemaphore);
            }
            m_currentFrameContext.m_imageAvailableSemaphore = imageAvailableSemaphore;
            m_currentFrameContext.m_presentableSemaphore = semaphoreAllocator.Allocate();
            return result;
        }

        void SwapChain::InvalidateSurface()
        {
            m_surface = nullptr;
        }

        void SwapChain::InvalidateNativeSwapChain()
        {
            auto& device = static_cast<Device&>(GetDevice());
            vkDeviceWaitIdle(device.GetNativeDevice());
            if (m_nativeSwapChain != VK_NULL_HANDLE)
            {
                vkDestroySwapchainKHR(device.GetNativeDevice(), m_nativeSwapChain, nullptr);
                m_nativeSwapChain = VK_NULL_HANDLE;
            }
        }
    }
}
