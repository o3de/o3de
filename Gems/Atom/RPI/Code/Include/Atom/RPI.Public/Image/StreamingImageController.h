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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

#include <Atom/RPI.Public/Image/StreamingImageContext.h>
#include <Atom/RPI.Reflect/Image/StreamingImageControllerAsset.h>

#include <AtomCore/Instance/InstanceData.h>

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
            : public Data::InstanceData
        {
        public:
            AZ_INSTANCE_DATA(StreamingImageController, "{D3708719-6955-4D6E-8D4C-1EF782C51EAD}");
            AZ_CLASS_ALLOCATOR(StreamingImageController, SystemAllocator, 0);

            //! Create a StreamingImageController subclass using a StreamingImageControllerAsset subclass. This will
            //! automatically create the appropriate controller type that corresponds to the given asset's type (both 
            //! the asset class and the instance class must be registered with InstanceDatabase<StreamingImageController>).
            //! \param pool  The streaming image pool that the controller should use.
            static Data::Instance<StreamingImageController> Create(const Data::Asset<StreamingImageControllerAsset>& asset, RHI::StreamingImagePool& pool);

            virtual ~StreamingImageController() = default;

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

        protected:
            using StreamingImageContextList = AZStd::intrusive_list<StreamingImageContext, AZStd::list_base_hook<StreamingImageContext>>;

            StreamingImageController() = default;

            //! Wrapped streaming image operations used for derived StreamingImageController classes
            void QueueExpandToMipChainLevel(StreamingImage* image, size_t mipChainIndex);
            void TrimToMipChainLevel(StreamingImage* image, size_t mipChainIndex);

        private:

            ///////////////////////////////////////////////////////////////////
            // Controller Implementation API

            // Called when an image asset is being attached to the controller. The user is expected to return
            // a new instance of a streaming image context. The default implementation creates the basic context.
            virtual StreamingImageContextPtr CreateContextInternal();

            // Called when the controller is updating. The timestamp value is a monotonically increasing counter used
            // to track the time of last access. You are allowed to access the contexts in this method alone. Do NOT hold
            // references to streaming images outside of this method.
            virtual void UpdateInternal(size_t timestamp, const StreamingImageContextList& contexts) = 0;

            ///////////////////////////////////////////////////////////////////

            RHI::StreamingImagePool* m_pool = nullptr;

            // The mutex used to serialize attachment, detachment, and update; as they would otherwise stomp on each other.
            AZStd::mutex m_mutex;
            StreamingImageContextList m_contexts;

            // A work queue for performing StreamingImage::ExpandMipChain calls.
            AZStd::mutex m_mipExpandMutex;
            AZStd::vector<StreamingImageContextPtr> m_mipExpandQueue;

            // A work queue for resetting the minimum target mip level back to the max.
            AZStd::mutex m_mipTargetResetMutex;
            AZStd::vector<StreamingImageContextPtr> m_mipTargetResetQueue;

            // A monotonically increasing counter used to track image mip requests. Useful for sorting contexts by LRU.
            size_t m_timestamp = 0;
        };
    }
}
