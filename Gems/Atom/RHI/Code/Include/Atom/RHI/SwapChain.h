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

namespace AZ
{
    namespace RHI
    {
         //! The platform-independent swap chain base class. Swap chains contain a "chain" of images which
         //! map to a platform-specific window, displayed on a physical monitor. The user is allowed
         //! to adjust the swap chain outside of the current FrameScheduler frame. Doing so within a frame scheduler
         //! frame results in undefined behavior.
         //!
         //! The frame scheduler controls presentation of the swap chain. The user may attach a swap chain to a scope
         //! in order to render to the current image.
        class SwapChain
            : public ImagePoolBase
        {
        public:
            AZ_RTTI(SwapChain, "{888B64A5-D956-406F-9C33-CF6A54FC41B0}", Object);

            virtual ~SwapChain();

            //! Initializes the swap chain, making it ready for attachment.
            ResultCode Init(RHI::Device& device, const SwapChainDescriptor& descriptor);

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

            //! Returns the current image index of the swap chain.
            uint32_t GetCurrentImageIndex() const;

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
            virtual bool IsExclusiveFullScreenPreferred() const { return false; }

            //! Returns True if the swap chain prefers exclusive full screen mode and it is currently true, false otherwise.
            virtual bool GetExclusiveFullScreenState() const { return false; }

            //! Return True if the swap chain prefers exclusive full screen mode and a transition happened, false otherwise.
            virtual bool SetExclusiveFullScreenState([[maybe_unused]]bool fullScreenState) { return false; }

        protected:
            SwapChain();

            struct InitImageRequest
            {
                //! Pointer to the image to initialize.
                Image* m_image = nullptr;

                //! Index of the image in the swap chain.
                uint32_t m_imageIndex = 0;

                //! Descriptor for the image.
                ImageDescriptor m_descriptor;
            };

            //////////////////////////////////////////////////////////////////////////
            // ResourcePool Overrides

            //! Called when the pool is shutting down.
            void ShutdownInternal() override;

            //////////////////////////////////////////////////////////////////////////

        private:

            bool ValidateDescriptor(const SwapChainDescriptor& descriptor) const;

            //////////////////////////////////////////////////////////////////////////
            // Platform API

            //! Called when the swap chain is initializing.
            virtual ResultCode InitInternal(RHI::Device& device, const SwapChainDescriptor& descriptor, SwapChainDimensions* nativeDimensions) = 0;

            //! called when the swap chain is initializing an image.
            virtual ResultCode InitImageInternal(const InitImageRequest& request) = 0;

            //! Called when the swap chain is resizing.
            virtual ResultCode ResizeInternal(const SwapChainDimensions& dimensions, SwapChainDimensions* nativeDimensions) = 0;

            //! Called when the swap chain is presenting the currently swap image.
            //! Returns the index of the current image after the swap.
            virtual uint32_t PresentInternal() = 0;

            virtual void SetVerticalSyncIntervalInternal([[maybe_unused]]uint32_t previousVerticalSyncInterval) {}

            //////////////////////////////////////////////////////////////////////////

            SwapChainDescriptor m_descriptor;

            //! Images corresponding to each image in the swap chain.
            AZStd::vector<Ptr<Image>> m_images;

            //! The current image index.
            uint32_t m_currentImageIndex = 0;
        };
    }
}
