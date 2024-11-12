/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Image.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/StreamingImagePool.h>

namespace AZ::WebGPU
{
    RHI::Ptr<Image> Image::Create()
    {
        return aznew Image();
    }    

    RHI::ResultCode Image::Init(Device& device, wgpu::Texture& image, const RHI::ImageDescriptor& descriptor)
    {
        Base::Init(device);
        SetDescriptor(descriptor);
        SetNativeTexture(image);
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode Image::Init(Device& device, const RHI::ImageDescriptor& descriptor)
    {
        SetDescriptor(descriptor);
        wgpu::TextureDescriptor wgpuDescriptor = {};
        wgpuDescriptor.dimension = ConvertImageDimension(descriptor.m_dimension);
        wgpuDescriptor.format = ConvertImageFormat(descriptor.m_format);
        wgpuDescriptor.mipLevelCount = descriptor.m_mipLevels;
        wgpuDescriptor.sampleCount = descriptor.m_multisampleState.m_samples;
        wgpuDescriptor.size = ConvertImageSize(descriptor.m_size);
        wgpuDescriptor.usage = ConvertImageBinding(descriptor.m_bindFlags);
        wgpuDescriptor.label = GetName().GetCStr();
        if (descriptor.m_dimension != RHI::ImageDimension::Image3D)
        {
            wgpuDescriptor.size.depthOrArrayLayers = descriptor.m_arraySize;
        }

        m_wgpuTexture = device.GetNativeDevice().CreateTexture(&wgpuDescriptor);
        if (!m_wgpuTexture)
        {
            AZ_Assert(m_wgpuTexture, "Failed to create wgpu Image");
            return RHI::ResultCode::Fail;
        }
        return RHI::ResultCode::Success;
    }

    Image::~Image()
    {
        Invalidate();
    }

    wgpu::Texture& Image::GetNativeTexture()
    {
        return m_wgpuTexture;
    }

    const wgpu::Texture& Image::GetNativeTexture() const
    {
        return m_wgpuTexture;
    }

    void Image::SetUploadFence(RHI::Ptr<Fence> uploadFence)
    {
        m_uploadFence = uploadFence;
    }

    RHI::Ptr<Fence> Image::GetUploadFence() const
    {
        return m_uploadFence;
    }

    uint16_t Image::GetStreamedMipLevel() const
    {
        return m_streamedMipLevel;
    }

    void Image::SetStreamedMipLevel(uint16_t mipLevel)
    {
        m_streamedMipLevel = mipLevel;
    }

    void Image::FinalizeAsyncUpload(uint16_t newStreamedMipLevels)
    {
        AZ_Assert(newStreamedMipLevels <= m_streamedMipLevel, "Expand mip levels can't be more than streamed mip level.");

        if (newStreamedMipLevels < m_streamedMipLevel)
        {
            m_streamedMipLevel = newStreamedMipLevels;
            InvalidateViews();
        }
    }

    void Image::Invalidate()
    {
        if (m_wgpuTexture)
        {
            m_wgpuTexture.Destroy();
            m_wgpuTexture = nullptr;
        }
    }

    void Image::SetNativeTexture(wgpu::Texture& texture)
    {
        if (texture.Get() != m_wgpuTexture.Get())
        {
            m_wgpuTexture = texture;
            InvalidateViews();
        }
    }

    void Image::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuTexture && !name.empty())
        {
            m_wgpuTexture.SetLabel(name.data());
        }
    }

    void Image::GetSubresourceLayoutsInternal(
        const RHI::ImageSubresourceRange& subresourceRange,
        RHI::DeviceImageSubresourceLayout* subresourceLayouts,
        size_t* totalSizeInBytes) const
    {
        const RHI::ImageDescriptor& imageDescriptor = GetDescriptor();
        uint32_t byteOffset = 0;
        const uint32_t offsetAligment = 4;
        for (uint16_t arraySlice = subresourceRange.m_arraySliceMin; arraySlice <= subresourceRange.m_mipSliceMax; ++arraySlice)
        {
            for (uint16_t mipSlice = subresourceRange.m_mipSliceMin; mipSlice <= subresourceRange.m_mipSliceMax; ++mipSlice)
            {
                RHI::ImageSubresource subresource;
                subresource.m_arraySlice = arraySlice;
                subresource.m_mipSlice = mipSlice;
                RHI::DeviceImageSubresourceLayout subresourceLayout = RHI::GetImageSubresourceLayout(imageDescriptor, subresource);

                if (subresourceLayouts)
                {
                    const uint32_t subresourceIndex = RHI::GetImageSubresourceIndex(mipSlice, arraySlice, imageDescriptor.m_mipLevels);
                    RHI::DeviceImageSubresourceLayout& layout = subresourceLayouts[subresourceIndex];
                    layout.m_bytesPerRow = subresourceLayout.m_bytesPerRow;
                    layout.m_bytesPerImage = subresourceLayout.m_rowCount * subresourceLayout.m_bytesPerRow;
                    layout.m_offset = byteOffset;
                    layout.m_rowCount = subresourceLayout.m_rowCount;
                    layout.m_size = subresourceLayout.m_size;
                }

                byteOffset =
                    RHI::AlignUp(byteOffset + subresourceLayout.m_bytesPerImage * subresourceLayout.m_size.m_depth, offsetAligment);
            }
        }

        if (totalSizeInBytes)
        {
            *totalSizeInBytes = byteOffset;
        }
    }

    RHI::ResultCode Image::TrimImage(uint16_t targetMipLevel)
    {
        // Set streamed mip level to target mip level if there are more mips
        if (m_streamedMipLevel < targetMipLevel)
        {
            m_streamedMipLevel = targetMipLevel;
            InvalidateViews();
        }

        AZ_Assert(m_highestMipLevel <= targetMipLevel, "Bound mip level doesn't contain target mip level");
        return RHI::ResultCode::Success;
    }
}
