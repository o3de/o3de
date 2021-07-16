/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <Atom/RHI.Reflect/Limits.h>

namespace AZ
{
    namespace RPI
    {
        class StreamingImage;

        //! A context owned by a streaming controller which tracks a streaming image asset instance.
        //! The context has shared ownership between the streaming image and the streaming image controller.
        //! The controller is allowed to take a reference on the context, but not on the streaming image itself.
        //! As such, it's necessary to check that the image exists before using.
        class StreamingImageContext
            : public AZStd::intrusive_base
            , public AZStd::intrusive_list_node<StreamingImageContext>
        {
            friend class StreamingImageController;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImageContext, AZ::ThreadPoolAllocator, 0);

            StreamingImageContext() = default;
            virtual ~StreamingImageContext() = default;

            //! Returns the parent image which owns this context. This may be null if the image was destroyed while the context
            //! is held in a work queue. You must test that the image is valid before using in your Update. The pointer is guaranteed
            //! valid for the duration of the StreamingImageController::UpdateInternal() call.
            //! 
            //! NOTE: Do NOT take a strong reference on this image. While doing so in the Update method is safe, allowing
            //! a reference to be held across Update ticks can result in a deadlock when the last reference on an image is released
            //! in a subsequent Update call (the deadlock will occur because the internal mutex on the controller is not recursive.
            //! This is by design). You are guaranteed that the streaming image reference is valid for the duration of the controller
            //! Update tick. So a strong reference is not required anyway.
            StreamingImage* TryGetImage() const;

            //! Returns the target mip level requested for the image.
            uint16_t GetTargetMip() const;

            //! Returns the timestamp of last access.
            size_t GetLastAccessTimestamp() const;

        private:

            // Holds a weak (raw) reference to the parent streaming image.
            StreamingImage* m_streamingImage = nullptr;

            // Tracks whether the context was queued for an expansion update.
            AZStd::atomic_bool m_queuedForMipExpand = {false};

            // Tracks whether the context was queued for a mip target reset.
            AZStd::atomic_bool m_queuedForMipTargetReset = {false};

            // Tracks the requested target mip level.
            AZStd::atomic_uint16_t m_mipLevelTarget = {RHI::Limits::Image::MipCountMax};

            // Tracks the last timestamp the image was requested.
            AZStd::atomic_size_t m_lastAccessTimestamp = {0};
        };

        using StreamingImageContextPtr = AZStd::intrusive_ptr<StreamingImageContext>;
    }
}
