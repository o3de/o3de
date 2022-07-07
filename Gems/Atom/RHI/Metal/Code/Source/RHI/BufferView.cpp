/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<BufferView> BufferView::Create()
        {
            return aznew BufferView();
        }

        RHI::ResultCode BufferView::InitInternal(RHI::Device& deviceBase, const RHI::Resource& resourceBase)
        {
            const Buffer& buffer = static_cast<const Buffer&>(resourceBase);
            const RHI::BufferViewDescriptor& viewDescriptor = GetDescriptor();
            const RHI::BufferDescriptor& bufferDescriptor = buffer.GetDescriptor();
            
            MemoryView buffMemView = buffer.GetMemoryView();
            size_t buffMemOffset = buffMemView.GetOffset() + viewDescriptor.m_elementOffset * viewDescriptor.m_elementSize;
            m_memoryView = MemoryView(buffMemView.GetMemory(), buffMemOffset, viewDescriptor.m_elementSize * viewDescriptor.m_elementCount, buffMemView.GetAlignment());
            
            bool isRGB32Float = viewDescriptor.m_elementFormat == RHI::Format::R32G32B32_FLOAT;
            
            //Create a texture view needed by typed buffers. In hlsl these variables are declared as Buffer/RWBuffer
            //Unfortunately metal does not support R32G32B32_FLOAT. We will need to ensure we dont have a use case
            //where a texture view is needed for a buffer with this format. Details in ATOM-13279
            if(viewDescriptor.m_elementFormat != RHI::Format::Unknown && !isRGB32Float)
            {
                id<MTLBuffer> mtlBuffer = buffMemView.GetMemory()->GetGpuAddress<id<MTLBuffer>>();
                MTLTextureUsage textureUsage = MTLTextureUsageShaderRead;
                
                if (RHI::CheckBitsAll(bufferDescriptor.m_bindFlags, RHI::BufferBindFlags::ShaderWrite))
                {
                    textureUsage |= MTLTextureUsageShaderWrite;
                }
                
                const uint32_t bytesPerPixel = RHI::GetFormatSize(viewDescriptor.m_elementFormat);                
                MTLTextureDescriptor* mtlTextureDesc =
                      [MTLTextureDescriptor textureBufferDescriptorWithPixelFormat : ConvertPixelFormat(viewDescriptor.m_elementFormat)
                                                                             width : viewDescriptor.m_elementCount
                                                                   resourceOptions : mtlBuffer.resourceOptions
                                                                             usage : textureUsage];
                mtlTextureDesc.textureType = MTLTextureTypeTextureBuffer;
                
                [[maybe_unused]] uint32_t bytesPerRow  = viewDescriptor.m_elementCount * bytesPerPixel;
                AZ_Assert(bytesPerRow == (viewDescriptor.m_elementCount * viewDescriptor.m_elementSize), "Mismatch for bytesPerRow");
                id<MTLTexture> mtlTexture = [mtlBuffer newTextureWithDescriptor : mtlTextureDesc
                                                                         offset : m_memoryView.GetOffset()
                                                                    bytesPerRow : (viewDescriptor.m_elementCount * bytesPerPixel)];
                AZ_Assert(mtlTexture, "Failed to create texture");
                
                RHI::Ptr<MetalResource> textureViewResource = MetalResource::Create(MetalResourceDescriptor{mtlTexture, ResourceType::MtlTextureType});
                m_imageBufferMemoryView = MemoryView(textureViewResource, 0, (viewDescriptor.m_elementCount * bytesPerPixel), 0);
            }
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode BufferView::InvalidateInternal()
        {
            return InitInternal(GetDevice(), GetResource());
        }
        
        void BufferView::ShutdownInternal()
        {
            auto& device = static_cast<Device&>(GetDevice());
            device.QueueForRelease(m_memoryView);
            m_memoryView = {};
        }
    
        const MemoryView& BufferView::GetMemoryView() const
        {
            return m_memoryView;
        }
        
        MemoryView& BufferView::GetMemoryView()
        {
            return m_memoryView;
        }
    
        const MemoryView& BufferView::GetTextureBufferView() const
        {
            return m_imageBufferMemoryView;
        }
    
        MemoryView& BufferView::GetTextureBufferView()
        {
            return m_imageBufferMemoryView;
        }
    }
}
