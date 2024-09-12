/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/AsyncWorkQueue.h>
#include <Atom/RHI/DeviceImage.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ::WebGPU
{
    class Device;
    class StreamingImagePool;

    class Image final
        : public RHI::DeviceImage
    {
        using Base = RHI::DeviceImage;
        friend class SwapChain;
        friend class AliasedHeap;
        friend class StreamingImagePool;
        friend class ImagePool;

    public:
        AZ_CLASS_ALLOCATOR(Image, AZ::ThreadPoolAllocator);
        AZ_RTTI(Image, "{7D518841-59D5-4545-BFD9-9824ECD52664}", Base);
        ~Image();
            
        static RHI::Ptr<Image> Create();

        wgpu::Texture& GetNativeTexture();
        const wgpu::Texture& GetNativeTexture() const;

        void SetUploadHandle(const RHI::AsyncWorkHandle& handle);
        const RHI::AsyncWorkHandle& GetUploadHandle() const;

        uint16_t GetStreamedMipLevel() const;
        void SetStreamedMipLevel(uint16_t mipLevel);

        void FinalizeAsyncUpload(uint16_t newStreamedMipLevels);
  
    private:
        Image() = default;
        RHI::ResultCode Init(Device& device, wgpu::Texture& image, const RHI::ImageDescriptor& descriptor);
        RHI::ResultCode Init(Device& device, const RHI::ImageDescriptor& descriptor);
        void Invalidate();

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

        void SetNativeTexture(wgpu::Texture& texture);

        // Trim image to specified mip level
        RHI::ResultCode TrimImage(uint16_t targetMipLevel);

        //! Native texture
        wgpu::Texture m_wgpuTexture = nullptr;

        // Handle for uploading mip map data to the image.
        RHI::AsyncWorkHandle m_uploadHandle;

        // Tracking the actual mip level data uploaded. It's also used for invalidate image view.
        uint16_t m_streamedMipLevel = 0;

        // The highest mip level that the image's current bound memory can accommodate
        uint16_t m_highestMipLevel = RHI::Limits::Image::MipCountMax;
    };
}
