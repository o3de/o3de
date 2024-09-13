/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImage.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ::WebGPU
{
    class Device;

    class Image final
        : public RHI::DeviceImage
    {
        using Base = RHI::DeviceImage;
        friend class SwapChain;
        friend class AliasedHeap;

    public:
        AZ_CLASS_ALLOCATOR(Image, AZ::ThreadPoolAllocator);
        AZ_RTTI(Image, "{7D518841-59D5-4545-BFD9-9824ECD52664}", Base);
        ~Image();
            
        static RHI::Ptr<Image> Create();

        wgpu::Texture& GetNativeTexture();
        const wgpu::Texture& GetNativeTexture() const;
  
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
            [[maybe_unused]] const RHI::ImageSubresourceRange& subresourceRange,
            [[maybe_unused]] RHI::DeviceImageSubresourceLayout* subresourceLayouts,
            [[maybe_unused]] size_t* totalSizeInBytes) const override
        {
        }
        //////////////////////////////////////////////////////////////////////////

        void SetNativeTexture(wgpu::Texture& texture);

        //! Native texture
        wgpu::Texture m_wgpuTexture = nullptr;
    };
}
