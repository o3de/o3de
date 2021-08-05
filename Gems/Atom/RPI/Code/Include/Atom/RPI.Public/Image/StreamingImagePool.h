/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Image/StreamingImageController.h>

#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>

#include <Atom/RHI/StreamingImagePool.h>

#include <AtomCore/Instance/InstanceData.h>

namespace AZ
{
    namespace RHI
    {
        class Device;
    }

    namespace RPI
    {
        //! A pool of streaming images and a context for controlling streaming operations on those images.
        //! 
        //! StreamingImagePool contains both an RHI pool and a streaming controller. Streaming images associate
        //! directly to a single pool, which is used as the allocation and streaming context. The relationship
        //! is many-to-one.
        //! 
        //! Users won't need to interact directly with pools. If a streaming image is instantiated which references
        //! the pool (by asset id), the pool is automatically brought up to accommodate the request. Likewise, if a pool
        //! is no longer referenced by a streaming image, it is shut down.
        class StreamingImagePool final
            : public Data::InstanceData
        {
            friend class StreamingImage;
            friend class ImageSystem;
        public:
            AZ_INSTANCE_DATA(StreamingImagePool, "{2F87FE52-4D20-49CA-9418-E544FB450B81}");
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);

            //! Instantiates or returns an existing runtime streaming image pool using its paired asset.
            //! @param streamingImagePoolAsset The asset used to instantiate an instance of the streaming image pool.
            static Data::Instance<StreamingImagePool> FindOrCreate(const Data::Asset<StreamingImagePoolAsset>& streamingImagePoolAsset);

            RHI::StreamingImagePool* GetRHIPool();

            const RHI::StreamingImagePool* GetRHIPool() const;

        private:
            StreamingImagePool() = default;

            // Standard init path from asset data.
            static Data::Instance<StreamingImagePool> CreateInternal(RHI::Device& device, StreamingImagePoolAsset& streamingImagePoolAsset);
            RHI::ResultCode Init(RHI::Device& device, StreamingImagePoolAsset& poolAsset);

            // Updates the streaming controller (ticked by the system component).
            void Update();

            ///////////////////////////////////////////////////////////////////
            // Private API for StreamingImage
            void AttachImage(StreamingImage* image);
            void DetachImage(StreamingImage* image);
            ///////////////////////////////////////////////////////////////////

            // The RHI streaming image pool instance.
            RHI::Ptr<RHI::StreamingImagePool> m_pool;

            // The controller used to manage streaming events on the pool.
            Data::Instance<StreamingImageController> m_controller;
        };
    }
}
