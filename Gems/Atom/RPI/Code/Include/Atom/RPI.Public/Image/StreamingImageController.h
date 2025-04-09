/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/StreamingImageContext.h>
#include <Atom/RPI.Reflect/Image/StreamingImageControllerAsset.h>

namespace AZ
{
    namespace RHI
    {
        class StreamingImagePool;
    }

    namespace RPI
    {
        class StreamingImage;

        class ATOM_RPI_PUBLIC_API StreamingImageController
        {
            friend class StreamingImagePool;
            friend class StreamingImage;
            friend class StreamingImageContext;

        public:
            //! Create a StreamingImageController
            static AZStd::unique_ptr<StreamingImageController> Create(RHI::StreamingImagePool& pool);

            StreamingImageController() = default;
            ~StreamingImageController() = default;

        protected:

            //! Attaches an instance of an image streaming asset to the controller.
            void AttachImage(StreamingImage* image);

            //! Detaches an instance of an image streaming asset from the controller.
            void DetachImage(StreamingImage* image);

            //! Performs an update tick of the streaming controller.
            void Update();

            //! Returns a monotonically increasing counter used to track usage of images for streaming.
            size_t GetTimestamp() const;

            //! Called by the streaming image when events occur.
            void OnSetTargetMip(StreamingImage* image, uint16_t targetMipLevel);
            void OnMipChainAssetReady(StreamingImage* image);

            //! Returns the number of images which are expanding their mipmaps
            uint32_t GetExpandingImageCount() const;

            //! Returns the number of streamable images attached to this controller
            uint32_t GetStreamableImageCount() const;

            //! Set mipmap bias to streaming images's streaming target
            //! For example, if the streaming image's streaming target is 0, and mipmap bias is 1, then the actual streaming target is 0+1
            //! The mipBias can be a negative value.
            //! The streaming target will be clamped to [0, imageLowestMip]
            void SetMipBias(int16_t mipBias);

            int16_t GetMipBias() const;
            
            //! Returns a streaming image's target mip level (with the global mip bias)
            uint16_t GetImageTargetMip(const StreamingImage* image) const;

            //! Return whether the available memory of the streaming image pool is low
            bool IsMemoryLow() const;

        protected:
            using StreamingImageContextList = AZStd::intrusive_list<StreamingImageContext, AZStd::list_base_hook<StreamingImageContext>>;


            // Evict one mip chain for the streaming image with lowest priority
            bool EvictOneMipChain();

            // Stream in one mip chain for the streaming image with highest priority
            bool ExpandOneMipChain();

            // Evict mipmaps for specific image
            // Return true if any mipmaps were evicted
            bool EvictUnusedMips(StreamingImage* image);

            // A callback function to release memory until the StreamingImagePool's device memory usage is same or less than the input number
            bool ReleaseMemory(size_t targetMemoryUsage);

            // Get gpu memory usage of the streaming image pool
            size_t GetPoolMemoryUsage();

            // Insert image to expandable and evictable lists
            void ReinsertImageToLists(StreamingImage* image);

            // Called when the expanding of an image is finished or canceled
            void EndExpandImage(StreamingImage* image);

            // Return whether an image need to expand its mipmap
            bool NeedExpand(const StreamingImage* image) const;

            // Reset the cached variables related to last memory value when the controller receives low memory notification
            void ResetLowMemoryState();

        private:

            // Called when an image asset is being attached to the controller. The user is expected to return
            // a new instance of a streaming image context. The default implementation creates the basic context.
            StreamingImageContextPtr CreateContext();

            ///////////////////////////////////////////////////////////////////

            RHI::StreamingImagePool* m_pool = nullptr;

            // The mutex used to serialize attachment, detachment, and update; as they would otherwise stomp on each other.
            AZStd::mutex m_contextAccessMutex;
            StreamingImageContextList m_contexts;

            // A work queue for performing StreamingImage::ExpandMipChain calls.
            AZStd::mutex m_mipExpandMutex;
            AZStd::queue<StreamingImageContextPtr> m_mipExpandQueue;

            // All the images which are managed by StreamingImageController
            AZStd::set<StreamingImage*> m_streamableImages;

            // All the images which are managed by StreamingImageController can be in one or few of the following lists
            // m_expandableImages, m_evictableImages or m_expandingImages
            // - When an image hasn't reached its target mipmap level, it will be in the m_expandableImages list
            // - When an image has mipmaps that can be evicted, it's in m_evictableImages list
            // - When an image is in the process of expanding, it will be removed from m_expandableImages and added to m_expandingImages list
            // - It's possible for an image to be in both m_expandableImages and m_evictableImages list.
            // - Expanding will be canceled when memory is low
            // - When an image is done expanding or evicted, they will get re-inserted back into m_expandableImages and m_evictableImages list
            
            // A list of expandable images which are sorted by their expanding priority
            struct ExpandPriorityComparator
            {
                bool operator()(const StreamingImage* lhs, const StreamingImage* rhs) const;
            };
            AZStd::set<StreamingImage*, ExpandPriorityComparator> m_expandableImages;
            // A list of images which have evictable mipmap. They are sorted by their evicting priority.
            struct EvictPriorityComparator
            {
                bool operator()(const StreamingImage* lhs, const StreamingImage* rhs) const;
            };

            AZStd::set<StreamingImage*, EvictPriorityComparator> m_evictableImages;
            // mutex for access the image lists
            AZStd::recursive_mutex m_imageListAccessMutex;

            // The images which are expanding will be added to this list and removed from m_streamableImages list.
            // Once their expansion is finished, they would be removed from this list and added back to m_evictableImages or/and m_expandableImages list
            AZStd::unordered_set<StreamingImage*> m_expandingImages;

            // A monotonically increasing counter used to track image mip requests. Useful for sorting contexts by LRU.
            size_t m_timestamp = 0;

            // The last memory when the controller receives low memory notification
            size_t m_lastLowMemory = 0 ;

            // a global option to add a bias to all the streaming images' target mip level
            int16_t m_globalMipBias = 0;
        };
    }
}
