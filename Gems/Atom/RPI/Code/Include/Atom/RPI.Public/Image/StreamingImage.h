/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Public/Image/StreamingImageContext.h>

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

// Enable streaming image hot reloading
#define AZ_RPI_STREAMING_IMAGE_HOT_RELOADING

namespace AZ
{
    namespace RPI
    {
        class StreamingImagePool;
        class StreamingImageController;

        //! A runtime streaming image, containing GPU data and streaming state.
        //! StreamingImage is the runtime instance of a StreamingImageAsset. Both are immutable (on
        //! GPU and CPU, respectively), and thus should remain 1-to-1.
        //! StreamingImage connects to its parent pool and parent streaming controller. The pool provides
        //! the allocation context for the RHI image. The controller provides the logic for streaming events
        //! based on priority and budget.
        //! 
        //! USAGE: To use a StreamingImage instance, you must declare usage of a desired target mip level.
        //! This is done *every* frame by calling SetTargetMip. This call is made when evaluating visibility
        //! of a parent surface in a feature processor, so that the proper mip level is chosen. Not calling
        //! SetTargetMip effectively tells the controller that the image was not used this cycle, making it
        //! a candidate for eviction.
        //! 
        //! STREAMING CONTROLLER USAGE: StreamingImage exposes an internal API to the streaming controller. Its the *sole*
        //! responsibility of the controller to fetch and evict mip chains from the streaming image, as this is the only
        //! system with enough context to budget properly.
        //! 
        //! Streaming works like a cache hierarchy. The GPU is the final 'L0' cache, the CPU is the 'L1' cache, and
        //! the disk is 'L2'. The GPU image allocation grows or shrinks to fit a target mip level. When expanding the
        //! image, the controller fetches mips from disk, using 'QueueExpandToMipChainLevel'. This establishes a connection
        //! with the asset system, which begins asynchronously streaming content from disk. When content arrives in CPU memory,
        //! the image queues itself on the controller for expansion. The expansion operation is done at a specific time in
        //! the streaming phase of the controller, in order to make uploads deterministic.
        //! 
        //! A trim operation will immediately trim the GPU image down and cancel any in-flight mip chain fetches.
        //! 
        class StreamingImage final
            : public Image
            , public Data::AssetBus::MultiHandler
        {
            static_assert(RHI::Limits::Image::MipCountMax < 16, "StreamingImageAsset is optimized to support a maximum of 16 mip levels.");

            friend class ImageSystem;
            friend class StreamingImageController;
        public:
            AZ_INSTANCE_DATA(StreamingImage, "{E48A7FF0-3065-42C6-9673-4FE7C8905629}", Image);
            AZ_CLASS_ALLOCATOR(StreamingImage, SystemAllocator, 0);

            //! Instantiates or returns an existing streaming image instance using its paired asset.
            static Data::Instance<StreamingImage> FindOrCreate(const Data::Asset<StreamingImageAsset>& streamingImageAsset);

            //! Helper method to instantiate a single-mip, single array streaming image from CPU data.
            static Data::Instance<StreamingImage> CreateFromCpuData(
                const StreamingImagePool& streamingImagePool,
                RHI::ImageDimension imageDimension,
                RHI::Size imageSize,
                RHI::Format imageFormat,
                const void* imageData,
                size_t imageDataSize,
                Uuid id = Uuid::CreateRandom());

            ~StreamingImage() override;

            //! Requests that image mips be made available for use by the streaming controller. This method should be
            //! called each frame with updated information based on visibility, etc. *Not* calling this method within
            //! a frame effectively tells the streaming controller that the image is not being used, and is a candidate
            //! for eviction. The streaming controller will prioritize attempt to service the request as quickly as
            //! possible. It is safe to call this method multiple times, and from multiple threads. The controller will track
            //! the most detailed request.
            //! 
            //! A value of 0 is the most detailed mip level. The value is clamped to the last mip in the chain.
            void SetTargetMip(uint16_t targetMipLevel);
            
            const Data::Instance<StreamingImagePool>& GetPool() const;

            //! Returns whether the streaming image is allowed to evict or expand mip chains.
            bool IsStreamable() const;

            ///////////////////////////////////////////////////////////////////
            // Streaming Controller API

            //! Trims the image to (and including) the requested mip chain index. Mip chains of higher detail
            //! than the requested mip chain are evicted from the GPU and any in-flight fetch requests are
            //! aborted.
            //! @param mipChainLevel The index of the mip chain (where 0 is most detailed) to target.
            RHI::ResultCode TrimToMipChainLevel(size_t mipChainLevel);

            //! Queues an expansion operation which fetches mip chain assets from disk. Each time a contiguous range
            //! of mip chains is ready, an expansion is queued on the parent controller.
            void QueueExpandToMipChainLevel(size_t mipChainLevel);
            
            //! Queues an expansion to the mip chain that is one level higher than the resident mip chain.
            void QueueExpandToNextMipChainLevel();

            //! Performs the GPU mip chain expansion for any contiguous range of ready mip chain assets. Returns
            //! the result of the RHI pool residency update. If no new mip chains are available, this will no-op
            //! and return success.
            RHI::ResultCode ExpandMipChain();

            //! Returns the most detailed mip level currently resident in memory, where a value of 0 is the highest detailed mip.
            uint16_t GetResidentMipLevel();

            //! Returns the average color of this image (alpha-weighted in case of 4-component images).
            Color GetAverageColor() const;

        private:
            StreamingImage() = default;

            static Data::Instance<StreamingImage> CreateInternal(StreamingImageAsset& streamingImageAsset);
            RHI::ResultCode Init(StreamingImageAsset& imageAsset);
            void Shutdown();

            ///////////////////////////////////////////////////////////////////
            // AssetBus::Handler
            /// Used to accept image mip chain asset events.
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            ///////////////////////////////////////////////////////////////////
            
            /**
             * Evicts the mip chain asset associated with the provided index from the CPU. Does *NOT*
             * affect the GPU image content.
             */
            void EvictMipChainAsset(size_t mipChainIndex);

            /**
             * Fetches the mip chain asset associated with the provided index. This will invoke a
             * streaming request from the asset system, which will take time. Fires an event to the
             * streaming controller when the mip is ready.
             */
            void FetchMipChainAsset(size_t mipChainIndex);
            
            /// Returns whether the mip chain is loaded.
            bool IsMipChainAssetReady(size_t mipChainIndex) const;

            /**
             * Called when a mip chain asset is ready.
             */
            void OnMipChainAssetReady(size_t mipChainIndex);

            /**
             * Uploads the mip chain content from the asset to the GPU
             */
            RHI::ResultCode UploadMipChain(size_t mipChainIndex);
            
            struct MipChainState
            {
                static const uint16_t InvalidMipChain = (uint16_t)-1;

                // Tracks the target mip chain asset for CPU residency through the asset system.
                uint16_t m_streamingTarget = InvalidMipChain;

                // Tracks the target mip chain asset for GPU residency.
                uint16_t m_residencyTarget = InvalidMipChain;

                // Tracks which mip chain assets are active (loading or ready).
                uint16_t m_maskActive = 0;

                // Tracks which mip chain assets are ready.
                uint16_t m_maskReady = 0;

                // Tracks which mip chain assets are evictable.
                uint16_t m_maskEvictable = (uint16_t)-1;
            };

            // Runtime state used to track streaming state. Only valid while initialized.
            MipChainState m_state;

            /**
             * Streaming image holds local Asset<> references to mip chains. This is because we are not
             * allowed to mutate the references held by the streaming image asset (it violates the immutability)
             * of the asset. Instead, the instance maintains its own list and fetch / evict events will populate
             * the local references. This also has the benefit of allowing the streaming image asset to hold its
             * own references which are never evicted, which is key for runtime-generated assets with no backing
             * representation on disk.
             */

            // A vector of local mip chain asset handles; used to control fetching / eviction.
            AZStd::fixed_vector<Data::Asset<ImageMipChainAsset>, RHI::Limits::Image::MipCountMax> m_mipChains;

            // The controller interface and local context used to control streaming of the image.
            StreamingImageController* m_streamingController = nullptr;
            StreamingImageContextPtr m_streamingContext;

            // The pool used to initialize the asset.
            Data::Instance<StreamingImagePool> m_pool;

            // RHI pool reference cached at init time from the parent pool asset.
            RHI::StreamingImagePool* m_rhiPool = nullptr;

            // The image asset associated with this image instance.
            Data::Asset<StreamingImageAsset> m_imageAsset;
        };
    }
}
