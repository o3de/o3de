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

        class StreamingImageController
        {
            friend class StreamingImagePool;
            friend class StreamingImage;

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

            // For all the streaming images, evict mipmaps which are not needed (mips which have higher details than target mip level)
            void EvictUnusedMips();

            // Evict mipmaps for specific image
            // Return true if any mipmaps were evicted
            bool EvictUnusedMips(StreamingImage* image);

            // Called when the RHI::StreamingImagePool is low on memory for new allocation
            bool OnHandleLowMemory();

            // Get gpu memory usage of the streaming image pool
            size_t GetPoolMemoryUsage();

            // Update image's priority
            void UpdateImagePriority(StreamingImage* image);

            // Calculate image's streaming priority value
            StreamingImage::Priority CalculateImagePriority(StreamingImage* image) const;

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

            // A sorted list of streaming images which mips can be streamed in/out.
            // The images are sorted by their priority key, last access timestamp and their address (so there won't be same key from different images)
            struct ImagePriorityComparator
            {
                bool operator()(const StreamingImage* lhs, const StreamingImage* rhs) const;
            };
            AZStd::set<StreamingImage*, ImagePriorityComparator> m_streamableImages;
            // mutex for access the m_streamableImages
            AZStd::mutex m_imageListAccessMutex;

            // The images which are expanding will be added to this list and removed from m_streamableImages list.
            // Once their expanding are finished, they would be removed from this list and added back to m_streamableImages list
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
