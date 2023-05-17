/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/StreamingImagePoolDescriptor.h>
#include <Atom/RHI/MultiDeviceImage.h>
#include <Atom/RHI/MultiDeviceImagePoolBase.h>
#include <Atom/RHI/StreamingImagePool.h>

#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RHI
    {
        using CompleteCallback = AZStd::function<void()>;

        /**
         * A structure used as an argument to MultiDeviceStreamingImagePool::InitImage.
         */
        struct MultiDeviceStreamingImageInitRequest
        {
            MultiDeviceStreamingImageInitRequest() = default;

            MultiDeviceStreamingImageInitRequest(
                MultiDeviceImage& image, const ImageDescriptor& descriptor, AZStd::span<const StreamingImageMipSlice> tailMipSlices);

            /// The image to initialize.
            MultiDeviceImage* m_image = nullptr;

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
         * A structure used as an argument to MultiDeviceStreamingImagePool::ExpandImage.
         */
        struct MultiDeviceStreamingImageExpandRequest
        {
            MultiDeviceStreamingImageExpandRequest() = default;

            /// The image with which to expand its mip chain.
            MultiDeviceImage* m_image = nullptr;

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

        class MultiDeviceStreamingImagePool : public MultiDeviceImagePoolBase
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceStreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceStreamingImagePool, "{466B4368-79D6-4363-91DE-3D0001159F7C}", MultiDeviceImagePoolBase);
            MultiDeviceStreamingImagePool() = default;
            virtual ~MultiDeviceStreamingImagePool() = default;

            const RHI::Ptr<StreamingImagePool>& GetDeviceStreamingImagePool(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDeviceStreamingImagePool",
                    m_deviceStreamingImagePools.find(deviceIndex) != m_deviceStreamingImagePools.end(),
                    "No DeviceStreamingImagePool found for device index %d\n",
                    deviceIndex);

                return m_deviceStreamingImagePools.at(deviceIndex);
            }

            //! Initializes the pool. The pool must be initialized before images can be registered with it.
            ResultCode Init(MultiDevice::DeviceMask deviceMask, const StreamingImagePoolDescriptor& descriptor);

            //! Initializes the backing resources of an image.
            ResultCode InitImage(const MultiDeviceStreamingImageInitRequest& request);

            //! Expands a streaming image with new mip chain data. The expansion can be performed
            //! asynchronously or synchronously depends on @m_waitForUpload in @MultiDeviceStreamingImageExpandRequest.
            //! Upon completion, the views will be invalidated and map to the newly
            //! streamed mip levels.
            ResultCode ExpandImage(const MultiDeviceStreamingImageExpandRequest& request);

            //! Trims a streaming image down to (and including) the target mip level. This occurs
            //! immediately. The newly evicted mip levels are no longer accessible by image views
            //! and the contents are considered undefined.
            ResultCode TrimImage(MultiDeviceImage& image, uint32_t targetMipLevel);

            const StreamingImagePoolDescriptor& GetDescriptor() const override final;

        private:
            using MultiDeviceImagePoolBase::InitImage;
            using MultiDeviceResourcePool::Init;

            bool ValidateInitRequest(const MultiDeviceStreamingImageInitRequest& initRequest) const;
            bool ValidateExpandRequest(const MultiDeviceStreamingImageExpandRequest& expandRequest) const;

            StreamingImagePoolDescriptor m_descriptor;

            // Frame mutex prevents image update requests from overlapping with frame.
            AZStd::shared_mutex m_frameMutex;

            AZStd::unordered_map<int, Ptr<StreamingImagePool>> m_deviceStreamingImagePools;
        };
    } // namespace RHI
} // namespace AZ
