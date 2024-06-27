/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/StreamingImagePoolDescriptor.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImagePoolBase.h>
#include <Atom/RHI/DeviceStreamingImagePool.h>

#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RHI
    {
        using CompleteCallback = AZStd::function<void()>;

        using StreamingImageInitRequest = StreamingImageInitRequestTemplate<Image>;
        using StreamingImageExpandRequest = StreamingImageExpandRequestTemplate<Image>;

        class StreamingImagePool : public ImagePoolBase
        {
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePool, "{466B4368-79D6-4363-91DE-3D0001159F7C}", ImagePoolBase);
            AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(StreamingImagePool);
            StreamingImagePool() = default;
            virtual ~StreamingImagePool() = default;

            //! Initializes the pool. The pool must be initialized before images can be registered with it.
            ResultCode Init(MultiDevice::DeviceMask deviceMask, const StreamingImagePoolDescriptor& descriptor);

            //! Initializes the backing resources of an image.
            ResultCode InitImage(const StreamingImageInitRequest& request);

            //! Expands a streaming image with new mip chain data. The expansion can be performed
            //! asynchronously or synchronously depends on @m_waitForUpload in @StreamingImageExpandRequest.
            //! Upon completion, the views will be invalidated and map to the newly
            //! streamed mip levels.
            ResultCode ExpandImage(const StreamingImageExpandRequest& request);

            //! Trims a streaming image down to (and including) the target mip level. This occurs
            //! immediately. The newly evicted mip levels are no longer accessible by image views
            //! and the contents are considered undefined.
            ResultCode TrimImage(Image& image, uint32_t targetMipLevel);

            const StreamingImagePoolDescriptor& GetDescriptor() const override final;

            //! Set a callback function that is called when the pool is out of memory for new allocations
            //! User could provide such a callback function which releases some resources from the pool
            //! If some resources are released, the function may return true.
            //! If nothing is released, the function should return false.
            using LowMemoryCallback = DeviceStreamingImagePool::LowMemoryCallback;
            void SetLowMemoryCallback(LowMemoryCallback callback);

            //! Set memory budget for all device pools
            //! Return true if the pool was set to new memory budget successfully
            bool SetMemoryBudget(size_t newBudget);

            //! Returns the maximum memory used by one of its pools for a specific heap type.
            const HeapMemoryUsage& GetHeapMemoryUsage(HeapMemoryLevel heapMemoryLevel) const;

            //! Return if it supports tiled image feature
            bool SupportTiledImage() const;

            void Shutdown() override final;

        private:
            using ImagePoolBase::InitImage;
            using ResourcePool::Init;

            bool ValidateInitRequest(const StreamingImageInitRequest& initRequest) const;
            bool ValidateExpandRequest(const StreamingImageExpandRequest& expandRequest) const;

            StreamingImagePoolDescriptor m_descriptor;

            //! Frame mutex prevents image update requests from overlapping with frame.
            AZStd::shared_mutex m_frameMutex;
        };
    } // namespace RHI
} // namespace AZ
