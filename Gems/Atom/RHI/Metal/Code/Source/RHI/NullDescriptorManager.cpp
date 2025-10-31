/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/NullDescriptorManager.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Metal
    {
        const uint32_t NullDescriptorManager::NullDescriptorHeapSize = 500*1024;
    
        void NullDescriptorManager::Init(Device& device)
        {
            DeviceObject::Init(device);
            
            RHI::DeviceObject::Init(device);
            MTLHeapDescriptor* heapDescriptor = [[MTLHeapDescriptor alloc]init];
            heapDescriptor.type = MTLHeapTypePlacement;
            heapDescriptor.storageMode = MTLStorageModePrivate;
            heapDescriptor.size = NullDescriptorHeapSize;
            heapDescriptor.hazardTrackingMode = MTLHazardTrackingModeTracked;

            m_nullDescriptorHeap = [device.GetMtlDevice() newHeapWithDescriptor:heapDescriptor];
            AZ_Assert(m_nullDescriptorHeap, "Null descriptorheap should not be null");
            [heapDescriptor release] ;
            
            [[maybe_unused]] RHI::ResultCode result = CreateImages();
            AZ_Assert(result == RHI::ResultCode::Success, "Image creation was unsuccessfull");
            
            result = CreateBuffer();
            AZ_Assert(result == RHI::ResultCode::Success, "Buffer creation was unsuccessfull");
            
            result = CreateSampler();
            AZ_Assert(result == RHI::ResultCode::Success, "Sampler creation was unsuccessfull");
            
        }

        void NullDescriptorManager::Shutdown()
        {
            m_nullImages.clear();
            m_nullBuffer.m_memoryView = {};
            m_nullMtlSamplerState = nil;
            
            [m_nullDescriptorHeap release];
            m_nullDescriptorHeap = nil;
            
            RHI::DeviceObject::Shutdown();
        }

        RHI::ResultCode NullDescriptorManager::CreateImages()
        {
            Device& device = static_cast<Device&>(GetDevice());

            m_nullImages.resize(static_cast<uint32_t>(ImageTypes::Count));
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly1D)].m_name = "DummyResource_1D";
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly1D)].m_imageDescriptor.m_format = RHI::Format::R8G8B8A8_UNORM;
            
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_name = "DummyResource_2D";
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_imageDescriptor.m_format = RHI::Format::R8G8B8A8_UNORM;

            m_nullImages[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)].m_name = "DummyResource_Msaa_2D";
            m_nullImages[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)].m_imageDescriptor.m_format = RHI::Format::R8G8B8A8_UNORM;
            m_nullImages[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)].m_imageDescriptor.m_multisampleState.m_samples = 4;
            
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)].m_name = "DummyResource_Cube";
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)].m_imageDescriptor.m_format = RHI::Format::R8G8B8A8_UNORM;
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)].m_imageDescriptor.m_isCubemap = 1;
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)].m_imageDescriptor.m_arraySize = 6;
            
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly3D)].m_name = "DummyResource_3D";
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly3D)].m_imageDescriptor.m_format = RHI::Format::R8G8B8A8_UNORM;
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly3D)].m_imageDescriptor.m_arraySize = 1;
            m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly3D)].m_imageDescriptor.m_dimension = RHI::ImageDimension::Image3D;
            
            m_nullImages[static_cast<uint32_t>(ImageTypes::TextureBuffer)].m_name = "DummyResource_TextureBuffer";
            m_nullImages[static_cast<uint32_t>(ImageTypes::TextureBuffer)].m_imageDescriptor.m_format = RHI::Format::R8G8B8A8_UNORM;
            m_nullImages[static_cast<uint32_t>(ImageTypes::TextureBuffer)].m_imageDescriptor.m_size.m_width = 1;
            m_nullImages[static_cast<uint32_t>(ImageTypes::TextureBuffer)].m_imageDescriptor.m_size.m_height = 1;
            
            uint32_t heapSize = 0;
            for (uint32_t imageIndex = static_cast<uint32_t>(NullDescriptorManager::ImageTypes::ReadOnly1D); imageIndex < static_cast<uint32_t>(NullDescriptorManager::ImageTypes::Count); imageIndex++)
            {
                RHI::ResourceMemoryRequirements memoryRequirements = device.GetResourceMemoryRequirements(m_nullImages[imageIndex].m_imageDescriptor);

                MTLSizeAndAlign textureSizeAndAlign;
                textureSizeAndAlign.align = memoryRequirements.m_alignmentInBytes;
                textureSizeAndAlign.size = memoryRequirements.m_sizeInBytes;
                
                const size_t alignedHeapSize = RHI::AlignUp(heapSize, textureSizeAndAlign.align);
                if(imageIndex == static_cast<uint32_t>(NullDescriptorManager::ImageTypes::TextureBuffer))
                {
                    m_nullImages[imageIndex].m_memoryView = device.CreateImagePlaced(m_nullImages[imageIndex].m_imageDescriptor, m_nullDescriptorHeap, alignedHeapSize, textureSizeAndAlign, MTLTextureTypeTextureBuffer);
                }
                else
                {
                    m_nullImages[imageIndex].m_memoryView = device.CreateImagePlaced(m_nullImages[imageIndex].m_imageDescriptor, m_nullDescriptorHeap, alignedHeapSize, textureSizeAndAlign);
                }
                heapSize = static_cast<uint32_t>(alignedHeapSize + textureSizeAndAlign.size);
                if(!m_nullImages[imageIndex].m_memoryView.IsValid())
                {
                    AZ_Assert(false, "Couldnt create a null image for ArgumentTable");
                    return RHI::ResultCode::Fail;
                }
                
                m_nullImages[imageIndex].m_memoryView.SetName(m_nullImages[imageIndex].m_name.c_str());
            }
            
            if(heapSize >= NullDescriptorHeapSize)
            {
                AZ_Assert(false, "Null Descriptor heap is not big enough");
                return RHI::ResultCode::Fail;
            }
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode NullDescriptorManager::CreateBuffer()
        {
            Device& device = static_cast<Device&>(GetDevice());
            
            m_nullBuffer.m_name = "NULL_DESCRIPTOR_BUFFER";
            m_nullBuffer.m_bufferDescriptor.m_byteCount = 1024;
            m_nullBuffer.m_bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderWrite;
            m_nullBuffer.m_memoryView = device.CreateBufferCommitted(m_nullBuffer.m_bufferDescriptor);
            m_nullBuffer.m_memoryView.SetName( m_nullBuffer.m_name.c_str());
            if(!m_nullBuffer.m_memoryView.IsValid())
            {
                AZ_Assert(false, "Couldnt create a null buffer for ArgumentTable");
                return RHI::ResultCode::Fail;
            }
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode NullDescriptorManager::CreateSampler()
        {
            Device& device = static_cast<Device&>(GetDevice());
            MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
            samplerDesc.label = @"NullDummySampler";
            m_nullMtlSamplerState = [device.GetMtlDevice() newSamplerStateWithDescriptor:samplerDesc];
            [samplerDesc release];
            samplerDesc = nil;
            if(m_nullMtlSamplerState == nil)
            {
                AZ_Assert(false, "Couldnt create a null sampler for ArgumentTable");
                return RHI::ResultCode::Fail;
            }
            return RHI::ResultCode::Success;
        }
    
        const MemoryView& NullDescriptorManager::GetNullImage(RHI::ShaderInputImageType imageType, bool isTextureBuffer)
        {
            switch(imageType)
            {
                case RHI::ShaderInputImageType::Image1D:
                case RHI::ShaderInputImageType::Image1DArray:
                {
                    return m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly1D)].m_memoryView;
                }
                case RHI::ShaderInputImageType::Image2D:
                case RHI::ShaderInputImageType::Image2DArray:
                {
                    return m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_memoryView;
                }
                case RHI::ShaderInputImageType::Image2DMultisample:
                {
                    return m_nullImages[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)].m_memoryView;
                }
                case RHI::ShaderInputImageType::ImageCube:
                case RHI::ShaderInputImageType::ImageCubeArray:
                {
                    return m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)].m_memoryView;
                }
                case RHI::ShaderInputImageType::Image3D:
                {
                    return m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly3D)].m_memoryView;
                }                
                default:
                    AZ_Assert(false, "image null descriptor type %d not handled", imageType);                    
            }
           
            return m_nullImages[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_memoryView;
        }
        
        const MemoryView& NullDescriptorManager::GetNullBuffer()
        {
            return m_nullBuffer.m_memoryView;
        }
    
        const MemoryView& NullDescriptorManager::GetNullImageBuffer()
        {
            return m_nullImages[static_cast<uint32_t>(ImageTypes::TextureBuffer)].m_memoryView;
        }
    
        id<MTLSamplerState> NullDescriptorManager::GetNullSampler() const
        {
            return m_nullMtlSamplerState;
        }
    
        id<MTLHeap> NullDescriptorManager::GetNullDescriptorHeap() const
        {
            return m_nullDescriptorHeap;
        }
    }
}
