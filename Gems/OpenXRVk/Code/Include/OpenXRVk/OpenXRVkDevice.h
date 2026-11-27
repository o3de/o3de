/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <XR/XRDevice.h>
#include <XR/XRSwapChain.h>
#include <OpenXRVk_Platform.h>
#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>

namespace OpenXRVk
{
    //! Vulkan specific XR device back-end class that will help manage 
    //! xr specific vulkan native objects related to device.
    class Device final
        : public XR::Device
    {
    public:
        AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator);
        AZ_RTTI(Device, "{81FD9B99-EDA5-4381-90EC-335073554379}", XR::Device);

        static XR::Ptr<Device> Create();

        //////////////////////////////////////////////////////////////////////////
        // XR::Device overrides

        // Create the xr specific native device object and populate the XRDeviceDescriptor with it.
        AZ::RHI::ResultCode InitDeviceInternal(AZ::RHI::XRDeviceDescriptor* instanceDescriptor) override;
        //! Get the Fov data  of the view specified by view index
        AZ::RHI::ResultCode GetViewFov(AZ::u32 viewIndex, AZ::RPI::FovData& outFovData) const override;
        //! Get the Pose data  of the view specified by view index
        AZ::RHI::ResultCode GetViewPose(AZ::u32 viewIndex, AZ::RPI::PoseData& outPoseData) const override;
        //////////////////////////////////////////////////////////////////////////

        //! Returns true if rendering data is valid for the current frame.
        bool ShouldRender() const;

        //! Get the native device
        VkDevice GetNativeDevice() const;

        //! Get the native physical device
        VkPhysicalDevice GetNativePhysicalDevice() const;

        //! Returns the graphic binding for a hardware queue
        const AZ::Vulkan::XRDeviceDescriptor::GraphicsBinding& GetGraphicsBinding(AZ::RHI::HardwareQueueClass queueClass) const;

        //! Get the anticipated display XrTime for the next application-generated frame.
        XrTime GetPredictedDisplayTime() const;

    private:

        //////////////////////////////////////////////////////////////////////////
        // XR::Device overrides
        //! Clean native objects.
        void ShutdownInternal() override;
        //! Inform the drivers that the frame is beginning
        bool BeginFrameInternal() override;
        //! Release the oldest swapchain image and inform the drivers that the frame is ending 
        void EndFrameInternal(XR::Ptr<XR::SwapChain>) override;
        //! Called after EndFrame has been executed.
        void PostFrameInternal() override;
        //! Locate views, acquire swapchain image and synchronize gpu with cpu
        bool AcquireSwapChainImageInternal(AZ::u32 viewIndex, XR::SwapChain* baseSwapChain) override;
        //////////////////////////////////////////////////////////////////////////

        VkDevice m_xrVkDevice = VK_NULL_HANDLE;
        VkPhysicalDevice m_xrVkPhysicalDevice = VK_NULL_HANDLE;
        AZStd::array<AZ::Vulkan::XRDeviceDescriptor::GraphicsBinding, AZ::RHI::HardwareQueueClassCount> m_xrQueueBinding;
        XrFrameState m_frameState{ XR_TYPE_FRAME_STATE };
        AZStd::vector<XrCompositionLayerBaseHeader*> m_xrLayers;
        XrCompositionLayerProjection m_xrLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        AZStd::vector<XrCompositionLayerProjectionView> m_projectionLayerViews;
    };
}