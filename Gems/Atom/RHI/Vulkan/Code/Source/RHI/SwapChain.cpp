/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Atom_RHI_Vulkan_Platform.h"
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/XRRenderingInterface.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/ILogger.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/Queue.h>
#include <RHI/RenderPass.h>
#include <RHI/SwapChain.h>
#include <RHI/ReleaseContainer.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        static bool IsDefaultSwapChainNeeded()
        {
            auto* xrSystem = RHI::RHISystemInterface::Get()->GetXRSystem();
            return !xrSystem || xrSystem->IsDefaultRenderPipelineNeeded();
        }

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

        bool SwapChain::ProcessRecreation()
        {
            if (m_pendingRecreation)
            {
                VkSwapchainKHR oldSwapchain = m_nativeSwapChain;
                ShutdownImages();
                CreateSwapchain();
                // Destroy the old swapchain AFTER creating the new one (because it's used for building the new one)
                InvalidateNativeSwapChain(oldSwapchain);
                InitImages();

                m_pendingRecreation = false;
                return true;
            }
            return false;
        }

        void SwapChain::SetVerticalSyncIntervalInternal(uint32_t previousVsyncInterval)
        {
            if (GetDescriptor().m_verticalSyncInterval == 0 || previousVsyncInterval == 0)
            {
                // The presentation mode may change when transitioning to or from a vsynced presentation mode
                // In this case, the swapchain must be recreated.
                m_pendingRecreation = true;
            }
        }

        void SwapChain::SetNameInternal([[maybe_unused]] const AZStd::string_view& name)
        {
            // On some GPUs, like the Adreno 740, setting the name of the swapchain causes a crash, so we don't do it.
        }

        RHI::ResultCode SwapChain::InitInternal(RHI::Device& baseDevice, const RHI::SwapChainDescriptor& descriptor, RHI::SwapChainDimensions* nativeDimensions)
        {
            RHI::ResultCode result = RHI::ResultCode::Success;
            RHI::DeviceObject::Init(baseDevice);

            auto& device = static_cast<Device&>(GetDevice());
            m_dimensions = descriptor.m_dimensions;

            if (descriptor.m_isXrSwapChain)
            {
                if (nativeDimensions)
                {
                    *nativeDimensions = m_dimensions;
                }
            }
            else
            {
                result = BuildSurface(descriptor);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);

                auto& presentationQueue = device.GetCommandQueueContext().GetOrCreatePresentationCommandQueue(*this);
                m_presentationQueue = &presentationQueue;

                if (IsDefaultSwapChainNeeded())
                {
                    result = CreateSwapchain();
                    RETURN_RESULT_IF_UNSUCCESSFUL(result);
                }

                if (nativeDimensions)
                {
                    // Fill out the real swapchain dimensions to return
                    *nativeDimensions = m_dimensions;
                    nativeDimensions->m_imageFormat = ConvertFormat(m_surfaceFormat.format);
                }
            }

            SetName(GetName());
            return result;
        }

        void SwapChain::ShutdownInternal()
        {
            //Nothing to clean as all the native objects for xr swapchain is handles by xr modules
            if (GetDescriptor().m_isXrSwapChain)
            {
                return;
            }

            InvalidateNativeSwapChain(m_nativeSwapChain);
            m_nativeSwapChain = VK_NULL_HANDLE;
            InvalidateSurface();
            m_presentationQueue = nullptr;

            m_swapchainNativeImages.clear();
            m_currentFrameContext = {};
        }

        RHI::ResultCode SwapChain::InitImageInternal(const RHI::DeviceSwapChain::InitImageRequest& request)
        {
            auto& device = static_cast<Device&>(GetDevice());
            Image* image = static_cast<Image*>(request.m_image);
            RHI::ImageDescriptor imageDesc = request.m_descriptor;
            RHI::ResultCode result = RHI::ResultCode::Success;

            // XR swapchains will retrieve the native swapchain image from xr system where as non-xr
            // swapchains will use the images created internally (i.e RHI::Vulkan)
            if (GetDescriptor().m_isXrSwapChain)
            {
                XRSwapChainDescriptor xrSwapChainDescriptor;
                xrSwapChainDescriptor.m_inputData.m_swapChainIndex = GetDescriptor().m_xrSwapChainIndex;
                xrSwapChainDescriptor.m_inputData.m_swapChainImageIndex = request.m_imageIndex;

                result = GetXRSystem()->GetSwapChainImage(&xrSwapChainDescriptor);
                AZ_Assert(result == RHI::ResultCode::Success, "Xr Session creation was not successful");

                result = image->Init(device, xrSwapChainDescriptor.m_outputData.m_nativeImage, imageDesc);
            }
            else
            {
                if (IsDefaultSwapChainNeeded())
                {
                    imageDesc.m_format = ConvertFormat(m_surfaceFormat.format);
                    result = image->Init(device, m_swapchainNativeImages[request.m_imageIndex], imageDesc);
                }
            }

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
            VkSwapchainKHR oldSwapchain = m_nativeSwapChain;
            auto& device = static_cast<Device&>(GetDevice());
            m_dimensions = dimensions;

            auto& presentationQueue = device.GetCommandQueueContext().GetOrCreatePresentationCommandQueue(*this);
            m_presentationQueue = &presentationQueue;

            CreateSwapchain();

            if (nativeDimensions)
            {
                *nativeDimensions = m_dimensions;
                // [ATOM-4840] This is a workaround when the windows is minimized (0x0 size).
                // Add proper support to handle this case.
                nativeDimensions->m_imageHeight = AZStd::max(m_dimensions.m_imageHeight, 1u);
                nativeDimensions->m_imageWidth = AZStd::max(m_dimensions.m_imageWidth, 1u);

                nativeDimensions->m_imageFormat = ConvertFormat(m_surfaceFormat.format);
            }

            // Destroy the old swapchain AFTER creating the new one (because it's used for building the new one)
            InvalidateNativeSwapChain(oldSwapchain);

            return RHI::ResultCode::Success;
        }

        uint32_t SwapChain::PresentInternal()
        {
            // No need to present a xr swapchain
            if (GetDescriptor().m_isXrSwapChain)
            {
                return 0;
            }

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
                    device.GetContext().CmdPipelineBarrier(
                        commandList->GetNativeCommandBuffer(),
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
                    auto transferSemaphore = device.GetSwapChainSemaphoreAllocator().Allocate();
                    // We wait until the swapchain image has finished being rendered to initialize the
                    // ownership transfer.
                    vulkanQueue->SubmitCommandBuffers(
                        AZStd::vector<RHI::Ptr<CommandList>>{ commandList },
                        AZStd::vector<Semaphore::WaitSemaphore>{ AZStd::make_pair(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, presentSemaphore) },
                        AZStd::vector<RHI::Ptr<Semaphore>>{ transferSemaphore },
                        {},
                        nullptr);

                    // The presentation engine must wait until the ownership transfer has completed.
                    waitSemaphore = transferSemaphore->GetNativeSemaphore();
                    transferSemaphore->SignalEvent();
                    // This will not deallocate immediately. It has a collect latency.
                    device.GetSwapChainSemaphoreAllocator().DeAllocate(transferSemaphore);
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

                const VkResult result = device.GetContext().QueuePresentKHR(vulkanQueue->GetNativeQueue(), &info);

                // Vulkan's definition of the two types of errors.
                // VK_ERROR_OUT_OF_DATE_KHR: "A surface has changed in such a way that it is no longer compatible with the swapchain,
                //     and further presentation requests using the swapchain will fail. Applications must query the new surface
                //     properties and recreate their swapchain if they wish to continue presenting to the surface."
                // VK_SUBOPTIMAL_KHR: "A swapchain no longer matches the surface properties exactly, but can still be used to
                //     present to the surface successfully."
                //
                // These result values may occur after resizing or some window operation. We should update the surface info and recreate the swapchain.
                // VK_SUBOPTIMAL_KHR is treated as success, but on non-mobile platforms we better update the surface info as well.
                if (result == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    m_pendingRecreation = true;
                }
                else if (result == VK_SUBOPTIMAL_KHR)
                {
                    // On mobile platforms the swapchain won't be recreated on VK_SUBOPTIMAL_KHR.
                    // This is because on mobiles VK_SUBOPTIMAL_KHR is returned when the swapchain's "preTransform"
                    // doesn't match the rotation of the device and that means its render engine internally will
                    // perform the rotation and on certain devices that's not as optimal as being handled by O3DE.
                    // Handling the rotation ourselves is not trivial to achieve, because the viewport dimensions have
                    // to be flipped (which affects UI operations) and view/projection matrices of 3D and 2D systems
                    // have to be manipulated in higher level code, which is very intrusive.
#if AZ_TRAIT_ATOM_VULKAN_RECREATE_SWAPCHAIN_WHEN_SUBOPTIMAL
                    m_pendingRecreation = true;
#endif
                }
                else
                {
                    // Other errors are:
                    // VK_ERROR_OUT_OF_HOST_MEMORY
                    // VK_ERROR_OUT_OF_DEVICE_MEMORY
                    // VK_ERROR_DEVICE_LOST
                    // VK_ERROR_SURFACE_LOST_KHR
                    // VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
                    AZ_Assert(result == VK_SUCCESS, "Unhandled error for swapchain presentation.");
                }
            };

            uint32_t acquiredImageIndex = GetCurrentImageIndex();
            RHI::ResultCode result = AcquireNewImage(&acquiredImageIndex);
            if (result == RHI::ResultCode::Fail)
            {
                m_pendingRecreation = true;
                return 0;
            }
            else
            {
                m_presentationQueue->QueueCommand(AZStd::move(presentCommand));
                return acquiredImageIndex;
            }
        }

        void SwapChain::SetHDRMetaData(float maxOutputNits, float minOutputNits, float maxContentLightLevel,
            float maxFrameAverageLightLevel)
        {
            auto& device = static_cast<Device&>(GetDevice());

            //From DX12 RHI
            struct DisplayChromacities
            {
                float RedX;
                float RedY;
                float GreenX;
                float GreenY;
                float BlueX;
                float BlueY;
                float WhiteX;
                float WhiteY;
            };
            static const DisplayChromacities DisplayChromacityList[] =
            {
                { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Display Gamut Rec709
                { 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Display Gamut Rec2020
            };

            // Select the chromaticity based on HDR format
            int selectedChroma = 0;
            if (m_colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)
            {
                selectedChroma = 0;
            }
            else if (m_colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
            {
                selectedChroma = 1;
            }
            else
            {
                AZ_Assert(false, "Unhandled color space for swapchain.");
            }

            const DisplayChromacities& Chroma = DisplayChromacityList[selectedChroma];
            VkHdrMetadataEXT hdrMetadata;
            hdrMetadata.sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
            //DX12 RHI scales these by 50000 but VKD3D removes that so just use the raw values.
            hdrMetadata.displayPrimaryRed = {Chroma.RedX, Chroma.RedY};
            hdrMetadata.displayPrimaryGreen = {Chroma.GreenX, Chroma.GreenY};
            hdrMetadata.displayPrimaryBlue = {Chroma.BlueX, Chroma.BlueY};
            hdrMetadata.whitePoint = {Chroma.WhiteX, Chroma.WhiteY};
            //Mult. by 10,000 because its what VKD3D does, you can look at convert_max_luminance and convert_min_luminance for reference.
            //Since the DX12 RHI scales these by 10000 we should too but VKD3D downscales by the same amount for minLuminance.
            //https://github.com/HansKristian-Work/vkd3d-proton/blob/8165096180a9019542b693ce1fcb80f44433b1e2/libs/vkd3d/swapchain.c#L866C36-L866C57
            hdrMetadata.maxLuminance = maxOutputNits * 10000.0f;
            hdrMetadata.minLuminance = minOutputNits;
            hdrMetadata.maxContentLightLevel = maxContentLightLevel;
            hdrMetadata.maxFrameAverageLightLevel = maxFrameAverageLightLevel;

            AZ_Assert(device.GetContext().SetHdrMetadataEXT != nullptr, "Calling SetHDRMetaData when HDR isn't supported.");
            device.GetContext().SetHdrMetadataEXT(device.GetNativeDevice(), 1, &m_nativeSwapChain, &hdrMetadata);
        }

        RHI::ResultCode SwapChain::BuildSurface(const RHI::SwapChainDescriptor& descriptor)
        {
            WSISurface::Descriptor surfaceDesc{};
            surfaceDesc.m_windowHandle = descriptor.m_window;
            RHI::Ptr<WSISurface> surface = WSISurface::Create();
            const RHI::ResultCode result = surface->Init(surfaceDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_surface = surface;

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
            AssertSuccess(device.GetContext().GetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &surfaceFormatCount, nullptr));
            AZ_Assert(surfaceFormatCount > 0, "Surface support no format.");
            AZStd::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            AssertSuccess(device.GetContext().GetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &surfaceFormatCount, surfaceFormats.data()));

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

            AZStd::vector<VkPresentModeKHR> preferredModes;
            if (verticalSyncInterval == 0)
            {
                // No vsync, so we try to use the immediate mode.
                preferredModes.push_back(VK_PRESENT_MODE_IMMEDIATE_KHR);
                // If immediate mode is not available we try for mailbox, which technically is still vsync
                // but doesn't block the CPU when queue is full.
                preferredModes.push_back(VK_PRESENT_MODE_MAILBOX_KHR);
            }
            preferredModes.push_back(VK_PRESENT_MODE_FIFO_KHR);
            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());

            uint32_t modeCount = 0;
            AssertSuccess(device.GetContext().GetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &modeCount, nullptr));
            // VK_PRESENT_MODE_FIFO_KHR has to be supported.
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPresentModeKHR.html
            AZ_Assert(modeCount > 0, "no available present mode.");
            AZStd::vector<VkPresentModeKHR> supportedModes(modeCount);
            AssertSuccess(device.GetContext().GetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &modeCount, supportedModes.data()));

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

        VkSurfaceCapabilitiesKHR SwapChain::GetSurfaceCapabilities()
        {
            AZ_Assert(m_surface, "Surface has not been initialized.");

            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());

            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            VkResult vkResult = device.GetContext().GetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &surfaceCapabilities);
            AssertSuccess(vkResult);

            return surfaceCapabilities;
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

        RHI::ResultCode SwapChain::BuildNativeSwapChain(const RHI::SwapChainDimensions& dimensions)
        {
            AZ_Assert(m_surface, "Surface is null.");

            if (!ValidateSurfaceDimensions(dimensions))
            {
                AZ_Assert(false, "Swapchain dimensions are not supported.");
                return RHI::ResultCode::InvalidArgument;
            }

            auto& device = static_cast<Vulkan::Device&>(GetDevice());
            auto& queueContext = device.GetCommandQueueContext();
            const VkExtent2D extent = { dimensions.m_imageWidth, dimensions.m_imageHeight };

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

            VkColorSpaceKHR colorSpace = m_surfaceFormat.colorSpace;
            bool hdrEnabled = false;

            if(device.GetContext().SetHdrMetadataEXT != nullptr &&
                dimensions.m_imageFormat == RHI::Format::R10G10B10A2_UNORM)
            {
                colorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
                hdrEnabled = true;
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
            createInfo.imageColorSpace = colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1; // non-stereoscopic
            createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            createInfo.imageUsage = RHI::FilterBits(createInfo.imageUsage, m_surfaceCapabilities.supportedUsageFlags);
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = aznumeric_cast<uint32_t>(familyIndices.size());
            createInfo.pQueueFamilyIndices = familyIndices.empty() ? nullptr : familyIndices.data();
            createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            createInfo.compositeAlpha = m_compositeAlphaFlagBits;
            createInfo.presentMode = m_presentMode;
            createInfo.clipped = VK_FALSE;
            // Pass the current swapchain as the old one
            createInfo.oldSwapchain = m_nativeSwapChain;

            const VkResult result =
                device.GetContext().CreateSwapchainKHR(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeSwapChain);
            AssertSuccess(result);

            if(hdrEnabled)
            {
                m_colorSpace = createInfo.imageColorSpace;
                float maxOutputNits = 1000.f;
                float minOutputNits = 0.001f;
                float maxContentLightLevelNits = 2000.f;
                float maxFrameAverageLightLevelNits = 500.f;
                SetHDRMetaData(maxOutputNits, minOutputNits, maxContentLightLevelNits, maxFrameAverageLightLevelNits);
            }

            return ConvertResult(result);
        }

        RHI::ResultCode SwapChain::AcquireNewImage(uint32_t* acquiredImageIndex)
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto& semaphoreAllocator = device.GetSwapChainSemaphoreAllocator();
            Semaphore* imageAvailableSemaphore = semaphoreAllocator.Allocate();
            VkResult vkResult = device.GetContext().AcquireNextImageKHR(
                device.GetNativeDevice(),
                m_nativeSwapChain,
                UINT64_MAX,
                imageAvailableSemaphore->GetNativeSemaphore(),
                VK_NULL_HANDLE,
                acquiredImageIndex);

            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

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

        void SwapChain::InvalidateNativeSwapChain(VkSwapchainKHR swapchain)
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto presentCommand = [&device, swapchain]([[maybe_unused]] void* queue)
            {
                device.GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Graphics).WaitForIdle();
                if (swapchain != VK_NULL_HANDLE)
                {
                    device.GetContext().DestroySwapchainKHR(device.GetNativeDevice(), swapchain, VkSystemAllocator::Get());
                }
            };

            m_presentationQueue->QueueCommand(AZStd::move(presentCommand));
            m_presentationQueue->FlushCommands();
        }

        RHI::ResultCode SwapChain::CreateSwapchain()
        {
            auto& device = static_cast<Device&>(GetDevice());

            m_surfaceCapabilities = GetSurfaceCapabilities();
            m_surfaceFormat = GetSupportedSurfaceFormat(m_dimensions.m_imageFormat);
            m_presentMode = GetSupportedPresentMode(GetDescriptor().m_verticalSyncInterval);
            m_compositeAlphaFlagBits = GetSupportedCompositeAlpha();

            if (!ValidateSurfaceDimensions(m_dimensions))
            {
                uint32_t oldHeight = m_dimensions.m_imageHeight;
                uint32_t oldWidth = m_dimensions.m_imageWidth;
                m_dimensions.m_imageHeight = AZStd::clamp(
                    m_dimensions.m_imageHeight,
                    m_surfaceCapabilities.minImageExtent.height,
                    m_surfaceCapabilities.maxImageExtent.height);
                m_dimensions.m_imageWidth = AZStd::clamp(
                    m_dimensions.m_imageWidth,
                    m_surfaceCapabilities.minImageExtent.width,
                    m_surfaceCapabilities.maxImageExtent.width);
                AZLOG_DEBUG("Resizing swapchain from (%u, %u) to (%u, %u).",
                    oldWidth, oldHeight, m_dimensions.m_imageWidth, m_dimensions.m_imageHeight);
            }

            RHI::ResultCode result = BuildNativeSwapChain(m_dimensions);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            AZLOG_DEBUG("Swapchain created. Width: %u, Height: %u.\n", m_dimensions.m_imageWidth, m_dimensions.m_imageHeight);

            // Do not recycle the semaphore because they may not ever get signaled and since
            // we can't recycle Vulkan semaphores we just delete them.
            if (m_currentFrameContext.m_imageAvailableSemaphore)
            {
                m_currentFrameContext.m_imageAvailableSemaphore->SetRecycleValue(false);
            }
            if (m_currentFrameContext.m_presentableSemaphore)
            {
                m_currentFrameContext.m_presentableSemaphore->SetRecycleValue(false);
            }

            m_dimensions.m_imageCount = 0;
            VkResult vkResult =
                device.GetContext().GetSwapchainImagesKHR(device.GetNativeDevice(), m_nativeSwapChain, &m_dimensions.m_imageCount, nullptr);
            AssertSuccess(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            m_swapchainNativeImages.resize(m_dimensions.m_imageCount);

            // Retrieve the native images of the swapchain so they are
            // available when we init the images in InitImageInternal
            vkResult = device.GetContext().GetSwapchainImagesKHR(
                device.GetNativeDevice(), m_nativeSwapChain, &m_dimensions.m_imageCount, m_swapchainNativeImages.data());
            AssertSuccess(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));
            AZLOG_DEBUG("Obtained presentable images.\n");

            // Acquire the first image
            uint32_t imageIndex = 0;
            result = AcquireNewImage(&imageIndex);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            AZLOG_DEBUG("Acquired the first image.\n");

            return RHI::ResultCode::Success;
        }
    }
}
