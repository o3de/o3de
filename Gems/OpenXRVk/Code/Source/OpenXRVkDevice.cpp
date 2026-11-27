/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVk/OpenXRVkDevice.h>
#include <OpenXRVk/OpenXRVkInstance.h>
#include <OpenXRVk/OpenXRVkSession.h>
#include <OpenXRVk/OpenXRVkSwapChain.h>
#include <OpenXRVk/OpenXRVkSpace.h>
#include <OpenXRVk/OpenXRVkUtils.h>
#include <OpenXRVk/OpenXRVkReferenceSpacesInterface.h>
#include <OpenXRVkCommon.h>
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <AzCore/Casting/numeric_cast.h>

namespace OpenXRVk
{
    XR::Ptr<Device> Device::Create()
    {
        return aznew Device;
    }

    AZ::RHI::ResultCode Device::InitDeviceInternal(AZ::RHI::XRDeviceDescriptor* deviceDescriptor)
    {
        AZ::Vulkan::XRDeviceDescriptor* xrDeviceDescriptor = static_cast<AZ::Vulkan::XRDeviceDescriptor*>(deviceDescriptor);
        m_xrVkDevice = xrDeviceDescriptor->m_inputData.m_xrVkDevice;
        m_xrVkPhysicalDevice = xrDeviceDescriptor->m_inputData.m_xrVkPhysicalDevice;
        m_xrQueueBinding = xrDeviceDescriptor->m_inputData.m_xrQueueBinding;
        return AZ::RHI::ResultCode::Success;
    }

    bool Device::BeginFrameInternal()
    {
        Platform::OpenXRBeginFrameInternal();

        Session* session = static_cast<Session*>(GetSession().get());
        XrSession xrSession = session->GetXrSession();

        m_xrLayers.clear();
        m_projectionLayerViews.clear();
        XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
        XrResult result = xrWaitFrame(xrSession, &frameWaitInfo, &m_frameState);
        WARN_IF_UNSUCCESSFUL(result);

        XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
        result = xrBeginFrame(xrSession, &frameBeginInfo);
        //The XR_FRAME_DISCARDED can sometimes spam harmlessly so filter it out
        if (result != XR_FRAME_DISCARDED)
        {
            WARN_IF_UNSUCCESSFUL(result);
        }

        // Notify the session that a new frame is starting with its new predicted display time.
        // The new predicted display time will be used to calculate XrPoses for existing Reference Spaces.
        session->OnBeginFrame(m_frameState.predictedDisplayTime);

        //Always return true as we want EndFrame to always be called. 
        return true;
    }

    void Device::EndFrameInternal(XR::Ptr<XR::SwapChain> baseSwapChain)
    {
        Platform::OpenXREndFrameInternal();

        Session* session = static_cast<Session*>(GetSession().get());
        Instance* instance = static_cast<Instance*>(GetDescriptor().m_instance.get());
        SwapChain* swapChain = static_cast<SwapChain*>(baseSwapChain.get());
        XrSession xrSession = session->GetXrSession();

        for (uint32_t i = 0; i < swapChain->GetNumViews(); i++)
        {
            XR::SwapChain::View* baseSwapChainView = baseSwapChain->GetView(i);
            SwapChain::View* viewSwapChain = static_cast<SwapChain::View*>(baseSwapChainView);

            if (baseSwapChainView->m_isImageAcquired)
            {
                XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
                XrResult result = xrReleaseSwapchainImage(viewSwapChain->GetSwapChainHandle(), &releaseInfo);
                ASSERT_IF_UNSUCCESSFUL(result);

                baseSwapChainView->m_isImageAcquired = false;
            }
        }

        m_xrLayer.space = session->GetViewSpaceXrSpace();
        m_xrLayer.viewCount = aznumeric_cast<uint32_t>(m_projectionLayerViews.size());
        m_xrLayer.views = m_projectionLayerViews.data();

        m_xrLayers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&m_xrLayer));

        XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
        frameEndInfo.displayTime = m_frameState.predictedDisplayTime;

        frameEndInfo.environmentBlendMode = instance->GetEnvironmentBlendMode();
        frameEndInfo.layerCount = aznumeric_cast<uint32_t>(m_xrLayers.size());
        frameEndInfo.layers = m_xrLayers.data();
        XrResult result = xrEndFrame(xrSession, &frameEndInfo);

        //The XR_ERROR_VALIDATION_FAILURE can sometimes spam harmlessly so filter it out.
        //It usually happens when xrBeginFrame yields XR_FRAME_DISCARDED 
        if (result != XR_ERROR_VALIDATION_FAILURE)
        {
            WARN_IF_UNSUCCESSFUL(result);
        }
    }

    void Device::PostFrameInternal()
    {
        Platform::OpenXRPostFrameInternal();
    }

    bool Device::AcquireSwapChainImageInternal(AZ::u32 viewIndex, XR::SwapChain* baseSwapChain)
    {
        XR::SwapChain::View* baseSwapChainView = baseSwapChain->GetView(viewIndex);
        SwapChain::View* swapChainView = static_cast<SwapChain::View*>(baseSwapChainView);
        XrSwapchain swapChainHandle = swapChainView->GetSwapChainHandle();
        Session* session = static_cast<Session*>(GetSession().get());

        XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        auto result = xrAcquireSwapchainImage(swapChainHandle, &acquireInfo, &baseSwapChainView->m_activeImageIndex);
        baseSwapChainView->m_isImageAcquired = (result == XR_SUCCESS);
        WARN_IF_UNSUCCESSFUL(result);

        XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        waitInfo.timeout = XR_INFINITE_DURATION;
        result = xrWaitSwapchainImage(swapChainHandle, &waitInfo);
        ASSERT_IF_UNSUCCESSFUL(result);

        // REMARK: The data in m_views was updated during BeginFrameInternal(), which
        // calls session->OnBeginFrame(...).
        const auto& xrViews = session->GetXrViews();
        m_projectionLayerViews.resize(xrViews.size());
        m_projectionLayerViews[viewIndex] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
        m_projectionLayerViews[viewIndex].pose = xrViews[viewIndex].pose;
        m_projectionLayerViews[viewIndex].fov = xrViews[viewIndex].fov;
        m_projectionLayerViews[viewIndex].subImage.swapchain = swapChainHandle;
        m_projectionLayerViews[viewIndex].subImage.imageRect.offset = { 0, 0 };
        m_projectionLayerViews[viewIndex].subImage.imageRect.extent = { static_cast<int>(swapChainView->GetWidth()),
                                                                        static_cast<int>(swapChainView->GetHeight()) };
        return true;
    }

    bool Device::ShouldRender() const
    {
        return m_frameState.shouldRender == XR_TRUE;
    }

    VkDevice Device::GetNativeDevice() const
    {
        return m_xrVkDevice;
    }

    VkPhysicalDevice Device::GetNativePhysicalDevice() const
    {
        return m_xrVkPhysicalDevice;
    }

    const AZ::Vulkan::XRDeviceDescriptor::GraphicsBinding& Device::GetGraphicsBinding(AZ::RHI::HardwareQueueClass queueClass) const
    {
        return m_xrQueueBinding[static_cast<uint32_t>(queueClass)];
    }

    AZ::RHI::ResultCode Device::GetViewFov(AZ::u32 viewIndex, AZ::RPI::FovData& outFovData) const
    {
        if (auto spacesIface = OpenXRReferenceSpacesInterface::Get();
            spacesIface != nullptr)
        {
            outFovData = spacesIface->GetViewFovData(viewIndex);
            return AZ::RHI::ResultCode::Success;
        }
        return AZ::RHI::ResultCode::Fail;
    }

    AZ::RHI::ResultCode Device::GetViewPose([[maybe_unused]] AZ::u32 viewIndex, [[maybe_unused]] AZ::RPI::PoseData& outPoseData) const
    {
        AZ_Assert(false, "Don't call this function!");
        return AZ::RHI::ResultCode::Fail;
    }

    XrTime Device::GetPredictedDisplayTime() const
    {
        return m_frameState.predictedDisplayTime;
    }

    void Device::ShutdownInternal()
    {
        m_projectionLayerViews.clear();
        m_xrLayers.clear();
        m_xrVkDevice = VK_NULL_HANDLE;
        m_xrVkPhysicalDevice = VK_NULL_HANDLE;
    }

}