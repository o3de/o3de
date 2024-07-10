/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/SwapChainDescriptor.h>
#include <Atom/RHI/ImagePoolBase.h>
#include <Atom/RHI/DeviceSwapChain.h>
#include <Atom/RHI/XRRenderingInterface.h>

namespace AZ::RHI
{
    //! The platform-independent, multi-device swap chain base class. Swap chains contain a "chain" of images which
    //! map to a platform-specific window, displayed on a physical monitor. The user is allowed
    //! to adjust the swap chain outside of the current FrameScheduler frame. Doing so within a frame scheduler
    //! frame results in undefined behavior.
    //!
    //! The frame scheduler controls presentation of the swap chain. The user may attach a swap chain to a scope
    //! in order to render to the current image.
    //!
    //! Although a multi-device resource class, we still enforce single-device behavior, as a DeviceSwapChain is tied to a
    //! specific window. This is done by initializing it with a device index, which sets the correspondings bit in the
    //! deviceMask. The need for a multi-device class arises from the interoperability within a multi-device context,
    //! especially attachments and the FrameGraph.
    class SwapChain : public ImagePoolBase
    {
    public:
        AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator, 0);
        AZ_RTTI(SwapChain, "{EB2B3AE5-41C0-4833-ABAD-4D964547029C}", Object);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(SwapChain);
        SwapChain() = default;
        virtual ~SwapChain() = default;

        //! Initializes the swap chain, making it ready for attachment.
        //! As the DeviceSwapChain uses multi-device resources on just a single device,
        //! it is explicitly initialized with just a deviceIndex
        ResultCode Init(int deviceIndex, const SwapChainDescriptor& descriptor);

        Ptr<DeviceSwapChain> GetDeviceSwapChain() const;

        //! Presents the swap chain to the display, and rotates the images.
        void Present();

        //! Sets the vertical sync interval for the swap chain.
        //!      0 - No vsync.
        //!      N - Sync to every N vertical refresh.
        //!
        //! A value of 1 syncs to the refresh rate of the monitor.
        void SetVerticalSyncInterval(uint32_t verticalSyncInterval);

        //! Resizes the display resolution of the swap chain. Ideally, this matches the platform window
        //! resolution. Typically, the resize operation will occur in reaction to a platform window size
        //! change. Takes effect immediately and results in a GPU pipeline flush.
        ResultCode Resize(const SwapChainDimensions& dimensions);

        //! Returns the number of images in the swap chain.
        uint32_t GetImageCount() const;

        //! Returns the current image of the swap chain.
        Image* GetCurrentImage() const;

        //! Returns the image associated with the provided index, where the total number of images
        //! is given by GetImageCount().
        Image* GetImage(uint32_t index) const;

        //! Returns the ID used for the SwapChain's attachment
        const AttachmentId& GetAttachmentId() const;

        //! Returns the descriptor provided when initializing the swap chain.
        const RHI::SwapChainDescriptor& GetDescriptor() const override final;

        //! Returns True if the swap chain prefers to use exclusive full screen mode.
        bool IsExclusiveFullScreenPreferred() const;

        //! Returns True if the swap chain prefers exclusive full screen mode and it is currently true, false otherwise.
        bool GetExclusiveFullScreenState() const;

        //! Return True if the swap chain prefers exclusive full screen mode and a transition happened, false otherwise.
        bool SetExclusiveFullScreenState(bool fullScreenState);

        //! Recreate the swapchain if it becomes invalid during presenting. This should happen at the end of the frame
        //! due to images being used as attachments in the frame graph.
        void ProcessRecreation();

        //! Shuts down the pool. This method will shutdown all resources associated with the pool.
        void Shutdown() override final;

    protected:
        struct InitImageRequest
        {
            //! Pointer to the image to initialize.
            Image* m_image = nullptr;

            //! Index of the image in the swap chain.
            uint32_t m_imageIndex = 0;

            //! Descriptor for the image.
            ImageDescriptor m_descriptor;
        };

        //! Shutdown and clear all the images.
        void ShutdownImages();

        //! Initialized all the images.
        ResultCode InitImages();

        //! Return the xr system interface
        RHI::XRRenderingInterface* GetXRSystem() const;

        //! Flag indicating if swapchain recreation is needed at the end of the frame.
        bool m_pendingRecreation = false;

    private:
        bool ValidateDescriptor(const SwapChainDescriptor& descriptor) const;

        SwapChainDescriptor m_descriptor;

        //! Images corresponding to each image in the swap chain.
        AZStd::vector<Ptr<Image>> m_images;

        //! Cache the XR system at initialization time
        RHI::XRRenderingInterface* m_xrSystem = nullptr;
    };
} // namespace AZ::RHI
