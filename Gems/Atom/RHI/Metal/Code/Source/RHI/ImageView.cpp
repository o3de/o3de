/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <Atom/RHI.Reflect/Format.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<ImageView> ImageView::Create()
        {
            return aznew ImageView();
        }

        RHI::ResultCode ImageView::InitInternal(RHI::Device& deviceBase, const RHI::DeviceResource& resourceBase)
        {
            const Image& image = static_cast<const Image&>(resourceBase);
            auto& device = static_cast<Device&>(deviceBase);
            RHI::ImageViewDescriptor viewDescriptor = GetDescriptor();
            const RHI::ImageDescriptor& imgDesc = image.GetDescriptor();
            
            BuildImageSubResourceRange(resourceBase);
            const RHI::ImageSubresourceRange& range = GetImageSubresourceRange();
            NSRange levelRange = {range.m_mipSliceMin, AZStd::min(static_cast<NSUInteger>(range.m_mipSliceMax - range.m_mipSliceMin + 1), static_cast<NSUInteger>(imgDesc.m_mipLevels))};
            NSRange sliceRange = {range.m_arraySliceMin, AZStd::min(static_cast<NSUInteger>(range.m_arraySliceMax - range.m_arraySliceMin + 1), static_cast<NSUInteger>(imgDesc.m_arraySize))};
            
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
            uint32_t textureLength = static_cast<uint32_t>(mtlTexture.arrayLength);
            if(imgDesc.m_isCubemap)
            {
                textureLength = textureLength * RHI::ImageDescriptor::NumCubeMapSlices;
            }
            
            MTLTextureType imgTextureType = mtlTexture.textureType;
            //Recreate the texture if the existing texture is not flagged as an array but the view is of an array.
            //Essentially this will tag a 2d texture as 2darray texture to ensure that sampling works correctly.
            if(!IsTextureTypeAnArray(imgTextureType) && viewDescriptor.m_isArray)
            {
                isViewFormatDifferent = true;
                imgTextureType = ConvertTextureType(imgDesc.m_dimension, imgDesc.m_arraySize, imgDesc.m_isCubemap, viewDescriptor.m_isArray);
            }
            
            //Create a new view if the format, mip range or slice range has changed
            if( isViewFormatDifferent ||
                levelRange.location != 0 || levelRange.length != mtlTexture.mipmapLevelCount ||
                sliceRange.location != 0 || sliceRange.length != textureLength)
            {
                //Protection against creating a view with an invalid format
                //If view format is invalid use the base texture's format
                if(textureViewFormat == MTLPixelFormatInvalid)
                {
                    textureViewFormat = textureFormat;
                }

                textureView = [mtlTexture newTextureViewWithPixelFormat : textureViewFormat
                                                            textureType : imgTextureType
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
            }
            else
            {
                m_format = textureFormat;
            }
            AZ_Assert(m_format != MTLPixelFormatInvalid, "Invalid pixel format");

            // If a depth stencil image does not have depth or aspect flag set it is probably going to be used as
            // a render target and do not need to be added to the bindless heap
            bool isDSRendertarget = RHI::CheckBitsAny(image.GetDescriptor().m_bindFlags, RHI::ImageBindFlags::DepthStencil) &&
                                    viewDescriptor.m_aspectFlags != RHI::ImageAspectFlags::Depth &&
                                    viewDescriptor.m_aspectFlags != RHI::ImageAspectFlags::Stencil;

            // Cache the read and readwrite index of the view withn the global Bindless Argument buffer
            if (device.GetBindlessArgumentBuffer().IsInitialized() && !viewDescriptor.m_isArray && !isDSRendertarget)
            {
                if (viewDescriptor.m_isCubemap)
                {
                    if (RHI::CheckBitsAll(image.GetDescriptor().m_bindFlags, RHI::ImageBindFlags::ShaderRead))
                    {
                        m_readIndex = device.GetBindlessArgumentBuffer().AttachReadCubeMapImage(*this);
                    }
                }
                else
                {
                    if (RHI::CheckBitsAll(image.GetDescriptor().m_bindFlags, RHI::ImageBindFlags::ShaderRead))
                    {
                        m_readIndex = device.GetBindlessArgumentBuffer().AttachReadImage(*this);
                    }
                    
                    if (RHI::CheckBitsAll(image.GetDescriptor().m_bindFlags, RHI::ImageBindFlags::ShaderWrite))
                    {
                        m_readWriteIndex = device.GetBindlessArgumentBuffer().AttachReadWriteImage(*this);
                    }
                }
            }
            
            m_memoryView.SetName(AZStd::string::format("%s_View_%s", image.GetName().GetCStr(), AZ::RHI::ToString(image.GetDescriptor().m_format)));
            m_hash = TypeHash64(m_imageSubresourceRange.GetHash(), m_hash);
            m_hash = TypeHash64(m_format, m_hash);
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

        void ImageView::ReleaseView()
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (m_memoryView.GetMemory())
            {
                device.QueueForRelease(m_memoryView);
                m_memoryView = {};
            }
            else
            {
                AZ_Assert(GetImage().IsSwapChainTexture(), "Validation check to ensure that only swapchain textures have null ImageView as they are special and don't need a view for metal back-end");
            }
        }

        void ImageView::ReleaseBindlessIndices()
        {
            auto& device = static_cast<Device&>(GetDevice());
            const RHI::ImageViewDescriptor& viewDescriptor = GetDescriptor();
            if (device.GetBindlessArgumentBuffer().IsInitialized())
            {
                if (viewDescriptor.m_isCubemap)
                {
                    if (m_readIndex != ~0u)
                    {
                        device.GetBindlessArgumentBuffer().DetachReadCubeMapImage(m_readIndex);
                    }
                }
                else
                {
                    if (m_readIndex != ~0u)
                    {
                        device.GetBindlessArgumentBuffer().DetachReadImage(m_readIndex);
                    }
                    if (m_readWriteIndex != ~0u)
                    {
                        device.GetBindlessArgumentBuffer().DetachReadWriteImage(m_readWriteIndex);
                    }
                }
            }
        }

        void ImageView::ShutdownInternal()
        {
            ReleaseView();
            ReleaseBindlessIndices();
        }

        RHI::ResultCode ImageView::InvalidateInternal()
        {
            ReleaseView();
            RHI::ResultCode initResult = InitInternal(GetDevice(), GetResource());
            if (initResult != RHI::ResultCode::Success)
            {
                ReleaseBindlessIndices();
            }
            return initResult;
        }
        
        const RHI::ImageSubresourceRange& ImageView::GetImageSubresourceRange() const
        {
            return m_imageSubresourceRange;
        }
        
        void ImageView::BuildImageSubResourceRange(const RHI::DeviceResource& resourceBase)
        {
            const Image& image = static_cast<const Image&>(resourceBase);
            const RHI::ImageDescriptor& imageDesc = image.GetDescriptor();
            const RHI::ImageViewDescriptor& viewDescriptor = GetDescriptor();
            
            RHI::ImageSubresourceRange& range = m_imageSubresourceRange;
            range.m_mipSliceMin = AZStd::max(viewDescriptor.m_mipSliceMin, static_cast<uint16_t>(image.GetStreamedMipLevel()));
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
    
        uint32_t ImageView::GetBindlessReadIndex() const
        {
            return m_readIndex;
        }

        uint32_t ImageView::GetBindlessReadWriteIndex() const
        {
            return m_readWriteIndex;
        }
    }
}
