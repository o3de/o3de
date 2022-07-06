/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/StreamingImagePoolDescriptor.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImagePoolBase.h>

#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * Represents a single subresource in an image. Image sub-resources are a 2D
         * grid [MipLevelCount, ArraySize] where mip slice is an axis, and array slice
         * is an axis.
         */
        struct StreamingImageSubresourceData
        {
            /// Data to upload for this subresource. Format must match format of the image including block / row size.
            const void* m_data = nullptr;
        };

        /**
         * A list of sub-resources in this mip slice, one for each array slice in the array.
         */
        struct StreamingImageMipSlice
        {
            /// An array of subresource datas. The size of this array must match the array size of the image.
            AZStd::span<const StreamingImageSubresourceData> m_subresources;

            /// The layout of each image in the array.
            ImageSubresourceLayout m_subresourceLayout;
        };
        
        using CompleteCallback = AZStd::function<void()>;

        /**
         * A structure used as an argument to StreamingImagePool::InitImage.
         */
        struct DeviceStreamingImageInitRequest
        {
            DeviceStreamingImageInitRequest() = default;

            DeviceStreamingImageInitRequest(
                DeviceImage& image,
                const ImageDescriptor& descriptor,
                AZStd::span<const StreamingImageMipSlice> tailMipSlices);

            /// The image to initialize.
            DeviceImage* m_image = nullptr;

            /// The descriptor used to to initialize the image.
            ImageDescriptor m_descriptor;

            /**
             * An array of tail mip slices to upload. This must not be empty or the call will fail.
             * This should only include the baseline set of mips necessary to render the image at
             * its lowest resolution. The uploads is performed synchronously.
             */
            AZStd::span<const StreamingImageMipSlice> m_tailMipSlices;
        };

        /**
         * A structure used as an argument to StreamingImagePool::ExpandImage.
         */
        struct DeviceStreamingImageExpandRequest
        {
            DeviceStreamingImageExpandRequest() = default;

            /// The image with which to expand its mip chain.
            DeviceImage* m_image = nullptr;

            /**
             * A list of image mip slices used to expand the contents. The data *must*
             * remain valid for the duration of the upload (until m_completeCallback
             * is triggered).
             */
            AZStd::span<const StreamingImageMipSlice> m_mipSlices;

            /// Whether the function need to wait until the upload is finished.
            bool m_waitForUpload = false;
            
            /// A function to call when the upload is complete. It will be called instantly if m_waitForUpload was set to true.
            CompleteCallback m_completeCallback;
        };

        class DeviceStreamingImagePool : public DeviceImagePoolBase
        {
        public:
            AZ_RTTI(DeviceStreamingImagePool, "{C9F1E40E-D852-4515-ADCC-E2D3AB4B56AB}", DeviceImagePoolBase);
            virtual ~DeviceStreamingImagePool() = default;

            /// Initializes the pool. The pool must be initialized before images can be registered with it.
            ResultCode Init(Device& device, const StreamingImagePoolDescriptor& descriptor);

            /// Initializes the backing resources of an image.
            ResultCode InitImage(const DeviceStreamingImageInitRequest& request);

            /**
             * Expands a streaming image with new mip chain data. The expansion can be performed
             * asynchronously or synchronously depends on @m_waitForUpload in @StreamingImageExpandRequest. 
             * Upon completion, the views will be invalidated and map to the newly
             * streamed mip levels.
             */
            ResultCode ExpandImage(const DeviceStreamingImageExpandRequest& request);

            /**
             * Trims a streaming image down to (and including) the target mip level. This occurs
             * immediately. The newly evicted mip levels are no longer accessible by image views
             * and the contents are considered undefined.
             */
            ResultCode TrimImage(DeviceImage& image, uint32_t targetMipLevel);

            const StreamingImagePoolDescriptor& GetDescriptor() const override final;

        protected:
            DeviceStreamingImagePool() = default;

        private:
            using DeviceResourcePool::Init;
            using DeviceImagePoolBase::InitImage;

            bool ValidateInitRequest(const DeviceStreamingImageInitRequest& initRequest) const;
            bool ValidateExpandRequest(const DeviceStreamingImageExpandRequest& expandRequest) const;

            //////////////////////////////////////////////////////////////////////////
            // Platform API

            /// Called when the pool is being initialized.
            virtual ResultCode InitInternal(Device& device, const StreamingImagePoolDescriptor& descriptor);

            /// Called when an image is being initialized on the pool.
            virtual ResultCode InitImageInternal(const DeviceStreamingImageInitRequest& request);

            /// Called when an image mips are being expanded.
            virtual ResultCode ExpandImageInternal(const DeviceStreamingImageExpandRequest& request);

            /// Called when an image mips are being trimmed.
            virtual ResultCode TrimImageInternal(DeviceImage& image, uint32_t targetMipLevel);

            //////////////////////////////////////////////////////////////////////////

            StreamingImagePoolDescriptor m_descriptor;

            /// Frame mutex prevents image update requests from overlapping with frame.
            AZStd::shared_mutex m_frameMutex;
        };
    }
}
