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

namespace AZ::WebGPU
{
    RHI::Ptr<Image> Image::Create()
    {
        return aznew Image();
    }

    wgpu::Texture& Image::GetNativeTexture()
    {
        return m_wgpuTexture;
    }

    const wgpu::Texture& Image::GetNativeTexture() const
    {
        return m_wgpuTexture;
    }

    Image::~Image()
    {
        Invalidate();
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
        wgpuDescriptor.format = ConvertFormat(descriptor.m_format);
        wgpuDescriptor.mipLevelCount = descriptor.m_mipLevels;
        wgpuDescriptor.sampleCount = descriptor.m_multisampleState.m_samples;
        wgpuDescriptor.size = ConvertImageSize(descriptor.m_size);
        wgpuDescriptor.usage = ConvertImageBinding(descriptor.m_bindFlags);

        m_wgpuTexture = device.GetNativeDevice().CreateTexture(&wgpuDescriptor);
        if (!m_wgpuTexture)
        {
            AZ_Assert(m_wgpuTexture, "Failed to create wgpu Image");
            return RHI::ResultCode::Fail;                
        }
        return RHI::ResultCode::Success;
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
        m_wgpuTexture = AZStd::move(texture);
    }
}
