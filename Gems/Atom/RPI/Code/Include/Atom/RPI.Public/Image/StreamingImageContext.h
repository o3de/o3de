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
#include <AzCore/std/containers/intrusive_list.h>

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RPI.Public/Configuration.h>

namespace AZ
{
    namespace RPI
    {
        class StreamingImage;
        class StreamingImageController;

        //! A context owned by a streaming controller which tracks a streaming image asset instance.
        //! The context has shared ownership between the streaming image and the streaming image controller.
        //! The controller is allowed to take a reference on the context, but not on the streaming image itself.
        //! As such, it's necessary to check that the image exists before using.
        class ATOM_RPI_PUBLIC_API StreamingImageContext
            : public AZStd::intrusive_base
            , public AZStd::intrusive_list_node<StreamingImageContext>
        {
            friend class StreamingImageController;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImageContext, AZ::ThreadPoolAllocator);

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

            //! Calculate some mip stats which are used for determinate their expansion or eviction orders
            //! The stats include: m_mipLevelTargetAdjusted, m_residentMip, m_evictableMips, m_missingMips, m_residentMipSize
            //! This function need to be called every time after a mip is expanded or evicted or when the global mip bias is changed
            void UpdateMipStats();

        private:

            // Holds a weak (raw) reference to the parent streaming image.
            StreamingImage* m_streamingImage = nullptr;

            // Tracks whether the context was queued for an expansion update.
            AZStd::atomic_bool m_queuedForMipExpand = {false};

            // Tracks the desired target mip level. Default to 0 which is the mip level with highest detail.
            // User may use StreamingImage::SetTargetMip() function to set the target mip level for the image
            AZStd::atomic_uint16_t m_mipLevelTarget = {0};

            // Tracks the last timestamp the image was requested.
            AZStd::atomic_size_t m_lastAccessTimestamp = {0};
                        
            // The target mip level which applied global mip bias
            uint16_t m_mipLevelTargetAdjusted = 0;
            // The most detailed mip which is resident
            uint16_t m_residentMip = 0;
            // The number of mips which can be evicted
            uint16_t m_evictableMips = 0;
            // The mips which are missing to reach m_mipLevelTargetAdjusted 
            uint16_t m_missingMips = 0;
            // The size of the most detailed mip
            uint32_t m_residentMipSize = 1;
        };

        using StreamingImageContextPtr = AZStd::intrusive_ptr<StreamingImageContext>;
    }
}
