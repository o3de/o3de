/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/XRRenderingInterface.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>
#include <XR/XRBase.h>
#include <XR/XRInstance.h>
#include <XR/XRObject.h>
#include <XR/XRSession.h>

namespace XR
{
    class SwapChain;

    //! Base XR device class which will provide access to the back-end concrete object
    class Device
        : public XR::Object
    {
    public:
        AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator);
        AZ_RTTI(Device, "{A31B0DC2-BD54-443E-9350-EB1B10670FF9}");

        Device() = default;
        ~Device() override = default;

        struct Descriptor
        {
            AZ::RHI::ValidationMode m_validationMode = AZ::RHI::ValidationMode::Disabled;
            Ptr<Instance> m_instance;
        };

        //! Create the xr specific native device object and populate the XRDeviceDescriptor with it.
        virtual AZ::RHI::ResultCode InitDeviceInternal(AZ::RHI::XRDeviceDescriptor* instanceDescriptor) = 0;

        //! Returns true if rendering data is valid for the current frame.
        virtual bool ShouldRender() const = 0;

        //! Returns fov data for a give view index.
        virtual AZ::RHI::ResultCode GetViewFov(AZ::u32 viewIndex, AZ::RPI::FovData& outFovData) const = 0;

        //! Returns pose data for a give view index.
        virtual AZ::RHI::ResultCode GetViewPose(AZ::u32 viewIndex, AZ::RPI::PoseData& outPoseData) const = 0;

        //! Init the XR device.
        AZ::RHI::ResultCode Init(Descriptor descriptor);

        //! Signal Begin frame to the underlying back end.
        //! @note This function is called from the thread related to the presentation queue.
        bool BeginFrame();

        //! Signal End frame to the underlying back end.
        //! @note This function is called from the thread related to the presentation queue.
        void EndFrame(Ptr<SwapChain>);

        //! Signal after Endframe has been executed to the underlying back end.
        //! @note This function is called from the main thread.
        void PostFrame();

        //! Signal the back-end to acquire swapchain images.
        bool AcquireSwapChainImage(AZ::u32 viewIndex, SwapChain* swapChain);

        //! Register XR session with the device.
        void RegisterSession(Ptr<Session> session);

        //! UnRegister XR session with the device.
        void UnRegisterSession();

        //! Get the descriptor.
        const Descriptor& GetDescriptor() const;

        //! Get the xr session registered with the device
        Ptr<Session> GetSession() const;

    protected:
        //! Called when the device is being shutdown.
        virtual void ShutdownInternal() = 0;

        //! Called when the device is beginning a frame for processing.
        //! @note This function is called from the thread related to the presentation queue.
        virtual bool BeginFrameInternal() = 0;

        //! Called when the device is ending a frame for processing.
        //! Pass in the active swapchain in order to allow the back end to release the swap chain images
        //! @note This function is called from the thread related to the presentation queue.
        virtual void EndFrameInternal(XR::Ptr<XR::SwapChain>) = 0;

        //! Called after the EndFrame has been executed.
        //! @note This function is called from the main thread.
        virtual void PostFrameInternal() = 0;

        //! Called when the device is beginning a frame for processing.
        virtual bool AcquireSwapChainImageInternal(AZ::u32 viewIndex, XR::SwapChain* baseSwapChain) = 0;

    private:
        ///////////////////////////////////////////////////////////////////
        // XR::Object
        void Shutdown() override;
        ///////////////////////////////////////////////////////////////////

        Ptr<Session> m_session;
        Descriptor m_descriptor;
    };
}
