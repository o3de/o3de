/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "Atom_RHI_Metal_precompiled.h"

#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<ImageView> ImageView::Create()
        {
            return aznew ImageView();
        }

        RHI::ResultCode ImageView::InitInternal(RHI::Device& deviceBase, const RHI::Resource& resourceBase)
        {
            const Image& image = static_cast<const Image&>(resourceBase);
            
            RHI::ImageViewDescriptor viewDescriptor = GetDescriptor();
            const RHI::ImageDescriptor& imgDesc = image.GetDescriptor();
            
            BuildImageSubResourceRange(resourceBase);
            const RHI::ImageSubresourceRange& range = GetImageSubresourceRange();
            NSRange levelRange = {range.m_mipSliceMin, static_cast<NSUInteger>(range.m_mipSliceMax - range.m_mipSliceMin + 1)};
            NSRange sliceRange = {range.m_arraySliceMin, static_cast<NSUInteger>(range.m_arraySliceMax - range.m_arraySliceMin + 1)};
            
            id<MTLTexture> mtlTexture = image.GetMemoryView().GetGpuAddress<id<MTLTexture>>();
            MTLPixelFormat textureFormat = ConvertPixelFormat(image.GetDescriptor().m_format);

            id<MTLTexture>  textureView = nil;
            if(viewDescriptor.m_overrideFormat != RHI::Format::Unknown)
            {
                MTLPixelFormat textureViewFormat = ConvertPixelFormat(viewDescriptor.m_overrideFormat);
                
                //Create a unique texture if needed
                if(textureFormat != textureViewFormat)
                {
                    textureView = [mtlTexture newTextureViewWithPixelFormat : textureViewFormat
                                                                  textureType : mtlTexture.textureType
                                                                       levels : levelRange
                                                                       slices : sliceRange];
                }
            }

            if(!textureView)
            {
                textureView = mtlTexture;
                m_memoryView = MemoryView(image.GetMemoryView().GetMemory(),
                           image.GetMemoryView().GetOffset(),
                           image.GetMemoryView().GetSize(),
                           image.GetMemoryView().GetAlignment());
            }
            else
            {
                RHI::Ptr<MetalResource> resc = MetalResource::Create(MetalResourceDescriptor{textureView, ResourceType::MtlTextureType});
                m_memoryView = MemoryView(resc, 0, mtlTexture.buffer.length, 0);
            }

            //textureView is null for the swap chain texture as it is not created until the end of first frame.
            if(textureView)
            {
                m_format = textureView.pixelFormat;
                AZ_Assert(m_format != MTLPixelFormatInvalid, "Invalid pixel format");
            }

            return RHI::ResultCode::Success;
        }

        const MemoryView& ImageView::GetMemoryView() const
        {
            return m_memoryView;
        }
        
        MemoryView& ImageView::GetMemoryView()
        {
            return m_memoryView;
        }
    
        void ImageView::ShutdownInternal()
        {
            auto& device = static_cast<Device&>(GetDevice());
            
            if(m_memoryView.GetMemory())
            {
                device.QueueForRelease(m_memoryView.GetMemory());
            }
            else
            {
                AZ_Assert(GetImage().IsSwapChainTexture(), "Validation check to ensure that only swapchain textures have null imageview as they are special and dont need a view for metal backend");
            }
        }

        RHI::ResultCode ImageView::InvalidateInternal()
        {
            return InitInternal(GetDevice(), GetResource());
        }
        
        const RHI::ImageSubresourceRange& ImageView::GetImageSubresourceRange() const
        {
            return m_imageSubresourceRange;
        }
        
        void ImageView::BuildImageSubResourceRange(const RHI::Resource& resourceBase)
        {
            const Image& image = static_cast<const Image&>(resourceBase);
            const RHI::ImageDescriptor& imageDesc = image.GetDescriptor();
            const RHI::ImageViewDescriptor& descriptor = GetDescriptor();
            
            RHI::ImageSubresourceRange& range = m_imageSubresourceRange;
            range.m_mipSliceMin = descriptor.m_mipSliceMin;
            range.m_mipSliceMax = AZStd::min(descriptor.m_mipSliceMax, static_cast<uint16_t>(imageDesc.m_mipLevels - 1));
            if (imageDesc.m_dimension == RHI::ImageDimension::Image3D)
            {
                range.m_arraySliceMin = 0;
                range.m_arraySliceMax = 0;
            }
            else
            {
                range.m_arraySliceMin = descriptor.m_arraySliceMin;
                range.m_arraySliceMax = AZStd::min(descriptor.m_arraySliceMax, static_cast<uint16_t>(imageDesc.m_arraySize - 1));
            }
        }
    
        const Image& ImageView::GetImage() const
        {
            return static_cast<const Image&>(Base::GetImage());
        }
    }
}
