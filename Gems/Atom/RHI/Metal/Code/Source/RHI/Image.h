/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/AsyncWorkQueue.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImagePool.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/parallel/atomic.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
                
        class Image final
            : public RHI::DeviceImage
        {
            using Base = RHI::DeviceImage;
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::ThreadPoolAllocator);
            AZ_RTTI(Image, "{F9F25704-F885-4CBD-BC96-D8D1E89F95EA}", Base);
            ~Image() = default;
            
            static RHI::Ptr<Image> Create();
            
            // Returns the memory view allocated to this buffer.
            const MemoryView& GetMemoryView() const;
            MemoryView& GetMemoryView();
            
            // Get mip level uploaded to GPU
            uint32_t GetStreamedMipLevel() const;
            void SetStreamedMipLevel(uint32_t streamedMipLevel);

            size_t GetResidentSizeInBytes() const;
            void SetResidentSizeInBytes(size_t sizeInBytes);
            
            void SetUploadHandle(const RHI::AsyncWorkHandle& handle);
            const RHI::AsyncWorkHandle& GetUploadHandle() const;

            // Call when an asynchronous upload has completed.
            void FinalizeAsyncUpload(uint32_t newStreamedMipLevels);
            
            // Returns true if this is a swapchain texture. They are special as they are given to us by the drivers each frame
            bool IsSwapChainTexture() const;
            
        private:
            Image() = default;

            friend class SwapChain;
            friend class ImagePool;
            friend class TransientAttachmentPool;
            friend class StreamingImagePool;
            friend class AliasedHeap;
            friend class ImagePoolResolver;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImage
            void GetSubresourceLayoutsInternal(
                const RHI::ImageSubresourceRange& subresourceRange,
                RHI::DeviceImageSubresourceLayout* subresourceLayouts,
                size_t* totalSizeInBytes) const override;
            //////////////////////////////////////////////////////////////////////////
            
            // Size in bytes in DeviceMemory.  Used from StreamingImagePool.
            size_t m_residentSizeInBytes = 0;
            
            // Tracking the actual mip level data uploaded. It's also used for invalidate image view.
            AZStd::atomic_uint32_t m_streamedMipLevel = 0;
            
            // Handle for uploading mip map data to the image.
            RHI::AsyncWorkHandle m_uploadHandle;
            
            // The memory view allocated to this image.
            MemoryView m_memoryView;
            
            // The number of resolve operations pending for this image.
            AZStd::atomic<uint32_t> m_pendingResolves = 0;

            // Used to mark if this image is a swapchain image
            bool m_isSwapChainImage = false;
        };
    }
}
