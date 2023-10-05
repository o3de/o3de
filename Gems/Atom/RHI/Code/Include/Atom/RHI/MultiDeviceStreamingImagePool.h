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

        using MultiDeviceStreamingImageInitRequest = StreamingImageInitRequestTemplate<MultiDeviceImage>;
        using MultiDeviceStreamingImageExpandRequest = StreamingImageExpandRequestTemplate<MultiDeviceImage>;

        class MultiDeviceStreamingImagePool : public MultiDeviceImagePoolBase
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceStreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceStreamingImagePool, "{466B4368-79D6-4363-91DE-3D0001159F7C}", MultiDeviceImagePoolBase);
            AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(StreamingImagePool);
            MultiDeviceStreamingImagePool() = default;
            virtual ~MultiDeviceStreamingImagePool() = default;

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

            //! Frame mutex prevents image update requests from overlapping with frame.
            AZStd::shared_mutex m_frameMutex;
        };
    } // namespace RHI
} // namespace AZ
