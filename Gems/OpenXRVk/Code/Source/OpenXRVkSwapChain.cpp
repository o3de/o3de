/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <OpenXRVk/OpenXRVkDevice.h>
#include <OpenXRVk/OpenXRVkInstance.h>
#include <OpenXRVk/OpenXRVkSession.h>
#include <OpenXRVk/OpenXRVkSwapChain.h>
#include <OpenXRVk/OpenXRVkUtils.h>
#include <XR/XRFactory.h>

namespace OpenXRVk
{

    XR::Ptr<SwapChain> SwapChain::Create()
    {
        return aznew SwapChain;
    }

    XR::Ptr<SwapChain::Image> SwapChain::Image::Create()
    {
        return aznew SwapChain::Image;
    }

    XR::Ptr<SwapChain::View> SwapChain::View::Create()
    {
        return aznew SwapChain::View;
    }

    AZ::RHI::ResultCode SwapChain::View::Init(XrSwapchain handle, AZ::u32 width, AZ::u32 height)
    {
        m_handle = handle;
        m_width = width;
        m_height = height;
        return AZ::RHI::ResultCode::Success;
    }

    AZ::u32 SwapChain::View::GetCurrentImageIndex() const
    {
        return m_activeImageIndex;
    }

    AZ::RHI::ResultCode SwapChain::Image::Init(XrSwapchainImageVulkan2KHR swapchainImage)
    {
        m_swapchainImage = swapchainImage;
        return AZ::RHI::ResultCode::Success;
    }

    VkImage SwapChain::Image::GetNativeImage()
    {
        return m_swapchainImage.image;
    }

    XrSwapchain SwapChain::View::GetSwapChainHandle() const
    {
        return m_handle;
    }

    AZ::u32 SwapChain::View::GetWidth() const
    {
        return m_width;
    }

    AZ::u32 SwapChain::View::GetHeight() const
    {
        return m_height;
    }

    void SwapChain::View::Shutdown()
    {
        xrDestroySwapchain(m_handle);
    }

    AZ::RHI::ResultCode SwapChain::InitInternal()
    {
        Instance* xrVkInstance = static_cast<Instance*>(GetDescriptor().m_instance.get());
        Session* xrVkSession = static_cast<Session*>(GetDescriptor().m_session.get());
        XrInstance xrInstance = xrVkInstance->GetXRInstance();
        XrSystemId xrSystemId = xrVkInstance->GetXRSystemId();
        XrSession xrSession = xrVkSession->GetXrSession();

        // Read graphics properties for preferred swapchain length and logging.
        XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
        XrResult result = xrGetSystemProperties(xrInstance, xrSystemId, &systemProperties);
        WARN_IF_UNSUCCESSFUL(result);

        if(GetDescriptor().m_validationMode == AZ::RHI::ValidationMode::Enabled)
        {
            // Log system properties.
            AZ_Printf("OpenXRVk", "System Properties: Name=%s VendorId=%d\n", systemProperties.systemName, systemProperties.vendorId);
            AZ_Printf("OpenXRVk",
                "System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d\n",
                    systemProperties.graphicsProperties.maxSwapchainImageWidth, systemProperties.graphicsProperties.maxSwapchainImageHeight,
                    systemProperties.graphicsProperties.maxLayerCount);
            AZ_Printf("OpenXRVk",
                "System Tracking Properties: OrientationTracking=%s PositionTracking=%s\n",
                    systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "True" : "False",
                    systemProperties.trackingProperties.positionTracking == XR_TRUE ? "True" : "False");
        }

        m_numViews = xrVkInstance->GetViewCount();
        XrViewConfigurationType viewConfigType = xrVkInstance->GetViewConfigType();
        m_configViews.resize(m_numViews, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
        result = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId,
                                                   viewConfigType, m_numViews, &m_numViews, m_configViews.data());
        WARN_IF_UNSUCCESSFUL(result);

        // Create the swapchain and get the images.
        if (m_numViews > 0)
        {
            // Select a swapchain format.
            uint32_t swapchainFormatCount = 0;
            result = xrEnumerateSwapchainFormats(xrSession, 0, &swapchainFormatCount, nullptr);
            AZStd::vector<int64_t> swapChainFormats(swapchainFormatCount);
            result = xrEnumerateSwapchainFormats(xrSession, aznumeric_cast<uint32_t>(swapChainFormats.size()),
                                                 &swapchainFormatCount, swapChainFormats.data());
            WARN_IF_UNSUCCESSFUL(result);
            AZ_Assert(swapchainFormatCount == swapChainFormats.size(), "Size mismatch swapchainFormatCount %i swapChainFormats size %i", swapchainFormatCount, swapChainFormats.size());

            m_colorSwapChainFormat = SelectColorSwapChainFormat(swapChainFormats);

            // Print swapchain formats and the selected one.
            if (GetDescriptor().m_validationMode == AZ::RHI::ValidationMode::Enabled)
            {
                AZStd::string swapchainFormatsString;
                for (int64_t format : swapChainFormats)
                {
                    const bool selected = format == static_cast<int64_t>(m_colorSwapChainFormat);
                    swapchainFormatsString += " ";
                    if (selected)
                    {
                        swapchainFormatsString += "[";
                    }
                    swapchainFormatsString += AZStd::string::format("%" PRId64, format);
                    if (selected)
                    {
                        swapchainFormatsString += "]";
                    }
                }
                AZ_Printf("OpenXRVk", "Swapchain Formats: %s\n", swapchainFormatsString.c_str());
            }

            double xrViewResolutionScale = 1.0;
            if (auto* settingsRegistry = AZ::SettingsRegistry::Get();
                settingsRegistry != nullptr)
            {
                AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, xrViewResolutionScale,
                    "/O3DE/Atom/OpenXR", "ViewResolutionScale");
            }

            // Create a swapchain for each view.
            for (uint32_t i = 0; i < m_numViews; i++)
            {
                XrViewConfigurationView& configView = m_configViews[i];

                configView.recommendedImageRectWidth = static_cast<uint32_t>(static_cast<double>(configView.recommendedImageRectWidth) * xrViewResolutionScale);
                configView.recommendedImageRectHeight = static_cast<uint32_t>(static_cast<double>(configView.recommendedImageRectHeight) * xrViewResolutionScale);

                if (GetDescriptor().m_validationMode == AZ::RHI::ValidationMode::Enabled)
                {
                    AZ_Printf("OpenXRVk",
                          "Creating swapchain for view %d with dimensions Width=%d Height=%d SampleCount=%d\n", i,
                        configView.recommendedImageRectWidth, configView.recommendedImageRectHeight, configView.recommendedSwapchainSampleCount);
                }

                XR::Ptr<XR::SwapChain::View> baseViewSwapChain = XR::Factory::Get().CreateSwapChainView();
                SwapChain::View* viewSwapChain = static_cast<SwapChain::View*>(baseViewSwapChain.get());
                if (viewSwapChain)
                {
                    // Create the xr swapchain.
                    XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
                    swapchainCreateInfo.arraySize = m_arraySize;
                    swapchainCreateInfo.format = static_cast<int64_t>(m_colorSwapChainFormat);
                    swapchainCreateInfo.width = configView.recommendedImageRectWidth;
                    swapchainCreateInfo.height = configView.recommendedImageRectHeight;
                    swapchainCreateInfo.mipCount = m_mipCount;
                    swapchainCreateInfo.faceCount = m_faceCount;
                    swapchainCreateInfo.sampleCount = m_sampleCount;
                    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                                                     XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                                                     XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT |
                                                     XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT |
                                                     XR_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT_KHR
                                                     ;

                    XrSwapchain handle = XR_NULL_HANDLE;
                    result = xrCreateSwapchain(xrSession, &swapchainCreateInfo, &handle);
                    WARN_IF_UNSUCCESSFUL(result);

                    AZ::RHI::ResultCode resultCode = viewSwapChain->Init(handle, swapchainCreateInfo.width, swapchainCreateInfo.height);
                    if(resultCode == AZ::RHI::ResultCode::Success)
                    {
                        m_viewSwapchains.push_back(viewSwapChain);
                    }
                }

                result = xrEnumerateSwapchainImages(viewSwapChain->GetSwapChainHandle(), 0, &viewSwapChain->m_numImages, nullptr);
                WARN_IF_UNSUCCESSFUL(result);

                viewSwapChain->m_swapChainImageHeaders.resize(viewSwapChain->m_numImages);
                viewSwapChain->m_swapchainImages.resize(viewSwapChain->m_numImages);
                for (AZ::u32 j = 0; j < viewSwapChain->m_numImages; ++j)
                {
                    viewSwapChain->m_swapchainImages[j] = { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR };
                    viewSwapChain->m_swapChainImageHeaders[j] = reinterpret_cast<XrSwapchainImageBaseHeader*>(&viewSwapChain->m_swapchainImages[j]);
                }

                result = xrEnumerateSwapchainImages(viewSwapChain->GetSwapChainHandle(), viewSwapChain->m_numImages, &viewSwapChain->m_numImages, viewSwapChain->m_swapChainImageHeaders[0]);
                WARN_IF_UNSUCCESSFUL(result);
                for (uint32_t j = 0; j < viewSwapChain->m_numImages; ++j)
                {
                    XR::Ptr<XR::SwapChain::Image> baseViewSwapChainImage = XR::Factory::Get().CreateSwapChainImage();
                    SwapChain::Image* viewSwapChainImage = static_cast<SwapChain::Image*>(baseViewSwapChainImage.get());
                    AZ::RHI::ResultCode resultCode = viewSwapChainImage->Init(viewSwapChain->m_swapchainImages[j]);
                    if (resultCode == AZ::RHI::ResultCode::Success)
                    {
                        viewSwapChain->m_images.push_back(baseViewSwapChainImage);
                    }
                }
            }
        }

        return AZ::RHI::ResultCode::Success;
    }

    VkFormat SwapChain::SelectColorSwapChainFormat(const AZStd::vector<int64_t>& runtimeFormats) const
    {
        // List of supported color swapchain formats.
        constexpr int64_t SupportedColorSwapchainFormats[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB };

        auto swapchainFormatIt =
            AZStd::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), AZStd::begin(SupportedColorSwapchainFormats),
                AZStd::end(SupportedColorSwapchainFormats));
        if (swapchainFormatIt == runtimeFormats.end())
        {
            AZ_Error("OpenXRVk", false, "No runtime swapchain format supported for color swapchain");
            return VK_FORMAT_UNDEFINED;
        }

        return static_cast<VkFormat>(*swapchainFormatIt);
    }

    AZStd::vector<XrViewConfigurationView> SwapChain::GetViewConfigs() const
    {
        return m_configViews;
    }

    AZ::RHI::ResultCode SwapChain::GetSwapChainImage(AZ::RHI::XRSwapChainDescriptor* swapchainDescriptor) const
    {
        AZ::Vulkan::XRSwapChainDescriptor* xrSwapChainDescriptor = static_cast<AZ::Vulkan::XRSwapChainDescriptor*>(swapchainDescriptor);
        uint32_t swapChainIndex = xrSwapChainDescriptor->m_inputData.m_swapChainIndex;
        uint32_t swapChainImageIndex = xrSwapChainDescriptor->m_inputData.m_swapChainImageIndex;

        XR::SwapChain::View* viewSwapChain = GetView(swapChainIndex);
        SwapChain::Image* swapchainImage = static_cast<SwapChain::Image*>(viewSwapChain->m_images[swapChainImageIndex].get());
        xrSwapChainDescriptor->m_outputData.m_nativeImage = swapchainImage->GetNativeImage();
        return AZ::RHI::ResultCode::Success;
    }

    AZ::u32 SwapChain::GetSwapChainWidth(AZ::u32 viewIndex) const
    {
        return m_configViews[viewIndex].recommendedImageRectWidth;
    }

    AZ::u32 SwapChain::GetSwapChainHeight(AZ::u32 viewIndex) const
    {
        return m_configViews[viewIndex].recommendedImageRectHeight;
    }

    AZ::RHI::Format SwapChain::GetSwapChainFormat([[maybe_unused]] AZ::u32 viewIndex) const
    {
        return AZ::Vulkan::ConvertFormat(m_colorSwapChainFormat);
    }

    void SwapChain::ShutdownInternal()
    {
        for(XR::Ptr<XR::SwapChain::View> viewSwapChain : m_viewSwapchains)
        {
            viewSwapChain->Shutdown();
        }
    }
}
