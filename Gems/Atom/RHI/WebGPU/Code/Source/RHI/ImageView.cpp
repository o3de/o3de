/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>

namespace AZ::WebGPU
{
    RHI::Ptr<ImageView> ImageView::Create()
    {
        return aznew ImageView();
    }

    RHI::ResultCode ImageView::InitInternal([[maybe_unused]] RHI::Device& deviceBase, const RHI::DeviceResource& resourceBase)
    {
        const auto& image = static_cast<const Image&>(resourceBase);
        if (!image.GetNativeTexture())
        {
            return RHI::ResultCode::NotReady;
        }

        const RHI::ImageViewDescriptor& viewDescriptor = GetDescriptor();

        RHI::Format viewFormat = viewDescriptor.m_overrideFormat;
        if (viewFormat == RHI::Format::Unknown)
        {
            viewFormat = image.GetDescriptor().m_format;
        }

        RHI::ImageAspectFlags imageAspectFlags = RHI::FilterBits(viewDescriptor.m_aspectFlags, image.GetAspectFlags());
        wgpu::TextureViewDimension imageViewDimension = GetImageViewDimension();
        const RHI::ImageSubresourceRange range = BuildImageSubresourceRange(imageViewDimension, imageAspectFlags);

        wgpu::TextureViewDescriptor wgpuDesc = {};
        wgpuDesc.arrayLayerCount = range.m_arraySliceMax - range.m_arraySliceMin + 1;
        wgpuDesc.aspect = ConvertImageAspectFlags(imageAspectFlags);
        wgpuDesc.baseArrayLayer = range.m_arraySliceMin;
        wgpuDesc.baseMipLevel = range.m_mipSliceMin;
        wgpuDesc.dimension = imageViewDimension;
        wgpuDesc.format = ConvertFormat(viewFormat);
        wgpuDesc.mipLevelCount = range.m_mipSliceMax - range.m_mipSliceMin + 1;

        m_wgpuTextureView = image.GetNativeTexture().CreateView(&wgpuDesc);
        AZ_Assert(m_wgpuTextureView, "Failed to create texture view");
        return m_wgpuTextureView ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    RHI::ResultCode ImageView::InvalidateInternal()
    {
        m_wgpuTextureView = nullptr;
        RHI::ResultCode initResult = InitInternal(GetDevice(), GetResource());
        return initResult;
    }

    void ImageView::ShutdownInternal()
    {
        m_wgpuTextureView = nullptr;
    }

    RHI::ImageSubresourceRange ImageView::BuildImageSubresourceRange(
        wgpu::TextureViewDimension imageViewDimension, RHI::ImageAspectFlags aspectFlags)
    {
        const Image& image = static_cast<const Image&>(GetImage());
        const RHI::ImageDescriptor& imageDesc = image.GetDescriptor();
        const RHI::ImageViewDescriptor& descriptor = GetDescriptor();

        RHI::ImageSubresourceRange range;
        range.m_aspectFlags = aspectFlags;
        range.m_mipSliceMin = descriptor.m_mipSliceMin;
        range.m_mipSliceMax = AZStd::min(descriptor.m_mipSliceMax, static_cast<uint16_t>(imageDesc.m_mipLevels - 1));
        if (imageDesc.m_dimension != RHI::ImageDimension::Image3D)
        {
            range.m_arraySliceMin = descriptor.m_arraySliceMin;
            range.m_arraySliceMax = AZStd::min(descriptor.m_arraySliceMax, static_cast<uint16_t>(imageDesc.m_arraySize - 1));
        }
        else
        {
            switch (imageViewDimension)
            {
            case wgpu::TextureViewDimension::e2D:
            case wgpu::TextureViewDimension::e2DArray:
                {
                    range.m_arraySliceMin = descriptor.m_depthSliceMin;
                    range.m_arraySliceMax =
                        AZStd::GetMin<uint16_t>(descriptor.m_depthSliceMax, static_cast<uint16_t>(imageDesc.m_size.m_depth - 1));
                    break;
                }
            case wgpu::TextureViewDimension::e3D:
                {
                    range.m_arraySliceMin = 0;
                    range.m_arraySliceMax = 0;
                    break;
                }
            default:
                AZ_Assert(false, "Invalid image view type.");
                break;
            }
        }
        return range;
    }

    wgpu::TextureViewDimension ImageView::GetImageViewDimension() const
    {
        const Image& image = static_cast<const Image&>(GetImage());
        const RHI::ImageDescriptor& imgDesc = image.GetDescriptor();
        const RHI::ImageViewDescriptor& imgViewDesc = GetDescriptor();

        [[maybe_unused]] const uint16_t width = static_cast<uint16_t>(imgDesc.m_size.m_width);
        [[maybe_unused]] const uint16_t height = static_cast<uint16_t>(imgDesc.m_size.m_height);
        const uint16_t depth = AZStd::min(
                                    static_cast<uint16_t>(imgViewDesc.m_depthSliceMax - imgViewDesc.m_depthSliceMin),
                                    static_cast<uint16_t>(imgDesc.m_size.m_depth - 1)) +
            1;
        [[maybe_unused]] const uint16_t samples = imgDesc.m_multisampleState.m_samples;
        const uint16_t arrayLayers = AZStd::min(
                                            static_cast<uint16_t>(imgViewDesc.m_arraySliceMax - imgViewDesc.m_arraySliceMin),
                                            static_cast<uint16_t>(imgDesc.m_arraySize - 1)) + 1;
        // We cannot only use the number of layers of the ImageView to determinate if is a texture array. You can have a texture array
        // with only 1 layer and the shader expects an array type instead of a normal image type.
        const bool isArray = arrayLayers > 1 || imgViewDesc.m_isArray;
        AZ_Assert(width * height * depth * samples * arrayLayers != 0, "One of width, height, depth, samples, or arrayLayers = 0.");

        switch (imgDesc.m_dimension)
        {
        case RHI::ImageDimension::Image1D:
            {
                AZ_Assert(height == 1 && depth == 1, "Both height and depth must be 1 for Image1D or Image1DArray.");
                return wgpu::TextureViewDimension::e1D;
            case RHI::ImageDimension::Image2D:
                AZ_Assert(depth == 1, "Depth must be 1 for Image2D or Image2DArray.");
                if (!isArray)
                {
                    return wgpu::TextureViewDimension::e2D;
                }
                else
                {
                    if (imgViewDesc.m_isCubemap)
                    {
                        AZ_Assert(width == height, "Image of Cube or CubeArray form a square.");
                        AZ_Assert(depth == 1, "Depth of Cube or CubeArray = 1.");
                        AZ_Assert(samples == 1, "Sample of Cube or CubeArray = 1.");
                        AZ_Assert(arrayLayers % 6 == 0, "ArrayLayers %% 6 == 0 for Cube or CubeArray.");
                        if (arrayLayers == 6)
                        {
                            return wgpu::TextureViewDimension::Cube;
                        }
                        else
                        {
                            return wgpu::TextureViewDimension::CubeArray;
                        }
                    }
                    else
                    {
                        return wgpu::TextureViewDimension::e2DArray;
                    }
                }
            }
        case RHI::ImageDimension::Image3D:
            {
                return wgpu::TextureViewDimension::e3D;
            }
        default:
            {
                AZ_Assert(false, "Image Dimension is illegal.");
                return wgpu::TextureViewDimension::Undefined;
            }
        }
    }
}
