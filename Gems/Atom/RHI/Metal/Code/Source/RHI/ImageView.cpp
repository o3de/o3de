/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            MTLPixelFormat textureViewFormat = ConvertPixelFormat(viewDescriptor.m_overrideFormat);
            
            id<MTLTexture>  textureView = nil;
            bool isViewFormatDifferent = false;
            //Check if we the viewformat differs from the base texture format
            if(textureViewFormat != MTLPixelFormatInvalid)
            {
                isViewFormatDifferent = textureViewFormat!=textureFormat;
            }
                       
            //Since we divide the array length of a cubemap by NumCubeMapSlices when creating the base texture
            //we have to do reverse of that here
            uint32_t textureLength = mtlTexture.arrayLength;
            if(imgDesc.m_isCubemap)
            {
                textureLength = textureLength * RHI::ImageDescriptor::NumCubeMapSlices;
            }
            
            //Create a new view if the format, mip range or slice range has changed
            if( isViewFormatDifferent ||
                levelRange.length != mtlTexture.mipmapLevelCount ||
                sliceRange.length != textureLength)
            {
                //Protection against creating a view with an invalid format
                //If view format is invalid use the base texture's format
                if(textureViewFormat == MTLPixelFormatInvalid)
                {
                    textureViewFormat = textureFormat;
                }

                textureView = [mtlTexture newTextureViewWithPixelFormat : textureViewFormat
                                                            textureType : mtlTexture.textureType
                                                                 levels : levelRange
                                                                 slices : sliceRange];
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
            const RHI::ImageViewDescriptor& viewDescriptor = GetDescriptor();
            
            RHI::ImageSubresourceRange& range = m_imageSubresourceRange;
            range.m_mipSliceMin = viewDescriptor.m_mipSliceMin;
            range.m_mipSliceMax = AZStd::min(viewDescriptor.m_mipSliceMax, static_cast<uint16_t>(imageDesc.m_mipLevels - 1));
            range.m_arraySliceMin = viewDescriptor.m_arraySliceMin;
            range.m_arraySliceMax = AZStd::min(viewDescriptor.m_arraySliceMax, static_cast<uint16_t>(imageDesc.m_arraySize - 1));

            //The length value of the sliceRange parameter must be a multiple of 6 if the texture
            //is of type MTLTextureTypeCube or MTLTextureTypeCubeArray. Hence we cant really make
            //a subresource view for these types.
            if(imageDesc.m_isCubemap)
            {
                range.m_arraySliceMin = 0;
                range.m_arraySliceMax = static_cast<uint16_t>(imageDesc.m_arraySize - 1);
            }
        }
    
        const Image& ImageView::GetImage() const
        {
            return static_cast<const Image&>(Base::GetImage());
        }
    }
}
