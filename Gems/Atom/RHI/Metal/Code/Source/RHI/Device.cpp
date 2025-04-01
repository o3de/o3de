/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <Atom/RHI.Reflect/Metal/PlatformLimitsDescriptor.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <RHI/BufferPool.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Metal.h>
#include <RHI/PhysicalDevice.h>


//Symbols related to Obj-c categories are getting stripped out as part of the link step for monolithic builds
//This forces the linker to not strip symbols related to categories without actually referencing the dummy function.
//https://stackoverflow.com/questions/2567498/objective-c-categories-in-static-library
AZ_TRAIT_OS_ATTRIBUTE_MARK_USED static void ImportCategories ()
{
    Import_RHIMetalView();
}

namespace AZ
{
    namespace Metal
    {
        Device::Device()
        {
            RHI::Ptr<PlatformLimitsDescriptor> platformLimitsDescriptor = aznew PlatformLimitsDescriptor();
            platformLimitsDescriptor->LoadPlatformLimitsDescriptor(RHI::Factory::Get().GetName().GetCStr());
            m_descriptor.m_platformLimitsDescriptor = RHI::Ptr<RHI::PlatformLimitsDescriptor>(platformLimitsDescriptor);
        }

        RHI::Ptr<Device> Device::Create()
        {
            return aznew Device();
        }

        RHI::ResultCode Device::InitInternal(RHI::PhysicalDevice& physicalDeviceBase)
        {
            PhysicalDevice& physicalDevice = static_cast<PhysicalDevice&>(physicalDeviceBase);
            m_metalDevice = physicalDevice.GetNativeDevice();
            AZ_Assert(m_metalDevice, "Native device wasnt created");
            m_eventListener = [[MTLSharedEventListener alloc] init];

            InitFeatures();
            m_clearAttachments.Init(*this);
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Device::InitInternalBindlessSrg(const AZ::RHI::BindlessSrgDescriptor& bindlessSrgDesc)
        {
            return m_bindlessArgumentBuffer.Init(this, bindlessSrgDesc);
        }
    
        RHI::ResultCode Device::InitializeLimits()
        {
            {
                ReleaseQueue::Descriptor releaseQueueDescriptor;
                releaseQueueDescriptor.m_collectLatency = m_descriptor.m_frameCountMax;
                m_releaseQueue.Init(releaseQueueDescriptor);
            }
             
            {
                CommandListAllocator::Descriptor commandListAllocatorDescriptor;
                commandListAllocatorDescriptor.m_frameCountMax = m_descriptor.m_frameCountMax;
                m_commandListAllocator.Init(commandListAllocatorDescriptor, this);
            }

            m_pipelineLayoutCache.Init(*this);
            m_commandQueueContext.Init(*this);

            m_asyncUploadQueue.Init(*this, AsyncUploadQueue::Descriptor(m_descriptor.m_platformLimitsDescriptor->m_platformDefaultValues.m_asyncQueueStagingBufferSizeInBytes));

            BufferMemoryAllocator::Descriptor allocatorDescriptor;
            allocatorDescriptor.m_device = this;
            allocatorDescriptor.m_pageSizeInBytes = DefaultConstantBufferPageSize;
            allocatorDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::Constant;
            allocatorDescriptor.m_getHeapMemoryUsageFunction = [this]() { return &m_argumentBufferConstantsAllocatorMemoryUsage; };
            allocatorDescriptor.m_recycleOnCollect = false;
            m_argumentBufferConstantsAllocator.Init(allocatorDescriptor);
             
            allocatorDescriptor.m_getHeapMemoryUsageFunction = [this]() { return &m_argumentBufferAllocatorMemoryUsage; };
            allocatorDescriptor.m_pageSizeInBytes = DefaultArgumentBufferPageSize;
            m_argumentBufferAllocator.Init(allocatorDescriptor);

            m_nullDescriptorManager.Init(*this);
            
            m_samplerCache = [[NSCache alloc]init];
            [m_samplerCache setName:@"SamplerCache"];

            return RHI::ResultCode::Success;
        }
    
        void Device::PreShutdown()
        {
            //CommandLists hold on to a ptr to the device.
            //They need to be released in order for device's refcount to reach 0 and be released.
            m_commandListAllocator.Shutdown();
            m_commandQueueContext.Shutdown();
            m_asyncUploadQueue.Shutdown();
            m_stagingBufferPool.reset();
            m_nullDescriptorManager.Shutdown();
            m_clearAttachments.Shutdown();
            m_bindlessArgumentBuffer.GarbageCollect();
        }

        void Device::ShutdownInternal()
        {
            m_argumentBufferConstantsAllocator.Shutdown();
            m_argumentBufferAllocator.Shutdown();
            m_releaseQueue.Shutdown();
            m_pipelineLayoutCache.Shutdown();
            
            [m_samplerCache removeAllObjects];
            [m_samplerCache release];
            m_samplerCache = nil;
            
            for (AZ::u32 i = 0; i < CommandEncoderTypeCount; ++i)
            {
                m_commandListPools[i].Shutdown();
            }
        }

        RHI::ResultCode Device::BeginFrameInternal()
        {
            TryCreateAutoreleasePool();
            m_commandQueueContext.Begin();
            return RHI::ResultCode::Success;
        }

        void Device::EndFrameInternal()
        {
            m_bindlessArgumentBuffer.GarbageCollect();
            m_argumentBufferConstantsAllocator.GarbageCollect();
            m_argumentBufferAllocator.GarbageCollect();
            m_commandQueueContext.End();
            m_commandListAllocator.Collect();
            m_releaseQueue.Collect();
            FlushAutoreleasePool();
        }
    
        void Device::WaitForIdleInternal()
        {
            m_commandQueueContext.WaitForIdle();
            m_releaseQueue.Collect(true);
        }
        
        void Device::FlushAutoreleasePool()
        {
            if (m_autoreleasePool)
            {
                [m_autoreleasePool release];
                m_autoreleasePool = nullptr;
            }
        }
        
        void Device::TryCreateAutoreleasePool()
        {
            if (!m_autoreleasePool)
            {
                m_autoreleasePool = [[NSAutoreleasePool alloc] init];
            }
        }

        void Device::QueueDeferredRelease(RHI::Ptr<RHI::Object> object)
        {
        }
    
        MemoryView Device::CreateInternalImageCommitted(MTLTextureDescriptor* mtlTextureDesc)
        {
            id<MTLTexture> mtlTexture = [m_metalDevice newTextureWithDescriptor : mtlTextureDesc];
            AZ_RHI_DUMP_POOL_INFO_ON_FAIL(mtlTexture != nil);
            AZ_Assert(mtlTexture, "Failed to create texture");
            
            RHI::Ptr<MetalResource> resc = MetalResource::Create(MetalResourceDescriptor{mtlTexture, ResourceType::MtlTextureType});
            return MemoryView(resc, 0, mtlTexture.allocatedSize, 0);
        }
    
        // [GFX TODO][ATOM-3888] - This method should go away when the streaming textures also start using private memory.
        MemoryView Device::CreateImageCommitted(const RHI::ImageDescriptor& imageDescriptor, MTLStorageMode overrideStorageMode)
        {
            MTLTextureDescriptor* mtlTextureDesc = ConvertImageDescriptor(imageDescriptor);
            mtlTextureDesc.storageMode = overrideStorageMode;
            return CreateInternalImageCommitted(mtlTextureDesc);
        }
    
        MemoryView Device::CreateImageCommitted(const RHI::ImageDescriptor& imageDescriptor)
        {
            MTLTextureDescriptor* mtlTextureDesc = ConvertImageDescriptor(imageDescriptor);
            return CreateInternalImageCommitted(mtlTextureDesc);
        }

        MemoryView Device::CreateImagePlaced(const RHI::ImageDescriptor& imageDescriptor,
                                             id<MTLHeap> mtlHeap,
                                             size_t heapByteOffset,
                                             MTLSizeAndAlign textureSizeAndAlign,
                                             MTLTextureType overrideTextureType)
        {
            MTLTextureDescriptor* mtlTextureDesc = ConvertImageDescriptor(imageDescriptor);
            mtlTextureDesc.textureType = overrideTextureType;
            return CreateImagePlaced(imageDescriptor, mtlHeap, heapByteOffset, textureSizeAndAlign);
        }
    
        MemoryView Device::CreateImagePlaced(const RHI::ImageDescriptor& imageDescriptor,
                                             id<MTLHeap> mtlHeap,
                                             size_t heapByteOffset,
                                             MTLSizeAndAlign textureSizeAndAlign)
        {
            MTLTextureDescriptor* mtlTextureDesc = ConvertImageDescriptor(imageDescriptor);
            mtlTextureDesc.storageMode = mtlHeap.storageMode;
            mtlTextureDesc.hazardTrackingMode = mtlHeap.hazardTrackingMode;
            id<MTLTexture> mtlTexture = [mtlHeap newTextureWithDescriptor:mtlTextureDesc
                                                                   offset:heapByteOffset];
            AZ_Assert(mtlTexture, "Failed to create texture");
            
            RHI::Ptr<MetalResource> resc = MetalResource::Create(MetalResourceDescriptor{mtlTexture, ResourceType::MtlTextureType});
            AZ_Assert(textureSizeAndAlign.size == mtlTexture.allocatedSize, "Predicted size %tu does not match the actual size %tu", textureSizeAndAlign.size, mtlTexture.allocatedSize);
            return MemoryView(resc, heapByteOffset, mtlTexture.allocatedSize, textureSizeAndAlign.align);
        }
    
        MemoryView Device::CreateBufferPlaced(const RHI::BufferDescriptor& bufferDescriptor,
                                              id<MTLHeap> mtlHeap,
                                              size_t heapByteOffset,
                                              MTLSizeAndAlign bufferSizeAndAlign)
        {
            ResourceDescriptor resourceDesc = ConvertBufferDescriptor(bufferDescriptor);
            MTLResourceOptions resOptions = CovertToResourceOptions(mtlHeap.storageMode, resourceDesc.m_mtlCPUCacheMode, mtlHeap.hazardTrackingMode);
            
            id<MTLBuffer> mtlBuffer = [mtlHeap newBufferWithLength:bufferDescriptor.m_byteCount
                                                             options:resOptions
                                                              offset:heapByteOffset];
            AZ_Assert(mtlBuffer, "Failed to create buffer");
           
            RHI::Ptr<MetalResource> resc = MetalResource::Create(MetalResourceDescriptor{mtlBuffer, ResourceType::MtlBufferType});
            AZ_Assert(bufferSizeAndAlign.size == mtlBuffer.allocatedSize, "Predicted size does not match the actual size");
            return MemoryView(resc, heapByteOffset, mtlBuffer.allocatedSize, bufferSizeAndAlign.align);
        }
      
        MemoryView Device::CreateBufferCommitted(const RHI::BufferDescriptor& bufferDescriptor, RHI::HeapMemoryLevel heapMemoryLevel)
        {
            id<MTLBuffer> mtlBuffer = nullptr;
            ResourceDescriptor resourceDesc = ConvertBufferDescriptor(bufferDescriptor, heapMemoryLevel);
            MTLResourceOptions resOptions = CovertToResourceOptions(resourceDesc.m_mtlStorageMode, resourceDesc.m_mtlCPUCacheMode, resourceDesc.m_mtlHazardTrackingMode);
            mtlBuffer = [m_metalDevice newBufferWithLength:resourceDesc.m_width options:resOptions];
            AZ_RHI_DUMP_POOL_INFO_ON_FAIL(mtlBuffer != nil);
            AZ_Assert(mtlBuffer, "Failed to create the buffer");
            
            RHI::Ptr<MetalResource> resc = MetalResource::Create(MetalResourceDescriptor{mtlBuffer, ResourceType::MtlBufferType});
            return MemoryView(resc, 0, mtlBuffer.length, 0);
        }
                
        void Device::CompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder)
        {
        }
        
        void Device::UpdateCpuTimingStatisticsInternal() const
        {
            m_commandQueueContext.UpdateCpuTimingStatistics();
        }

        void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
        {
            for (uint32_t i = 0; i < formatsCapabilities.size(); ++i)
            {
                RHI::Format format = static_cast<RHI::Format>(i);
                RHI::FormatCapabilities& flags = formatsCapabilities[i];
                flags = RHI::FormatCapabilities::None;
                
                //Buffer specific capabilities
                //https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf doesnt have specific information about
                //buffer capabilities, hence enabling it for all formats.
                flags |= RHI::FormatCapabilities::VertexBuffer;
                flags |= RHI::FormatCapabilities::TypedLoadBuffer;
                flags |= RHI::FormatCapabilities::TypedStoreBuffer;
                flags |= RHI::FormatCapabilities::AtomicBuffer;
                
                if (format == RHI::Format::R32_UINT || format == RHI::Format::R16_UINT)
                {
                    flags |= RHI::FormatCapabilities::IndexBuffer;
                }
                
                //Texture specific capabilities
                if(IsDepthStencilSupported(m_metalDevice, format))
                {
                    flags |= RHI::FormatCapabilities::DepthStencil;
                }
                   
                if (IsColorRenderTargetSupported(m_metalDevice, format))
                {
                    flags |= RHI::FormatCapabilities::RenderTarget;
                }
                
                if (IsBlendingSupported(m_metalDevice, format))
                {
                    flags |= RHI::FormatCapabilities::Blend;
                }
                
                //All graphics and compute functions can read or sample from any texture, regardless of its pixel format.
                //For filtering support use IsFilteringSupported
                flags |= RHI::FormatCapabilities::Sample;
            }
        }
        
        AZStd::chrono::microseconds Device::GpuTimestampToMicroseconds(uint64_t gpuTimestamp, RHI::HardwareQueueClass queueClass) const
        {
            //gpuTimestamp in nanoseconds.
            auto timeInNano = AZStd::chrono::nanoseconds(gpuTimestamp);
            return AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(timeInNano);
        }
    
        RHI::ResourceMemoryRequirements Device::GetResourceMemoryRequirements(const RHI::ImageDescriptor& descriptor)
        {
            MTLTextureDescriptor* mtlTextureDesc = ConvertImageDescriptor(descriptor);
            MTLSizeAndAlign textureSizeAndAlign = [GetMtlDevice() heapTextureSizeAndAlignWithDescriptor:mtlTextureDesc];

            RHI::ResourceMemoryRequirements memoryRequirements;
            memoryRequirements.m_alignmentInBytes = textureSizeAndAlign.align;
            memoryRequirements.m_sizeInBytes = textureSizeAndAlign.size;
            
            return memoryRequirements;
        }

        RHI::ResourceMemoryRequirements Device::GetResourceMemoryRequirements(const RHI::BufferDescriptor& descriptor)
        {
            ResourceDescriptor resourceDesc = ConvertBufferDescriptor(descriptor);
            MTLResourceOptions resOptions = CovertToResourceOptions(resourceDesc.m_mtlStorageMode, resourceDesc.m_mtlCPUCacheMode, resourceDesc.m_mtlHazardTrackingMode);
            MTLSizeAndAlign bufferSizeAndAlign = [GetMtlDevice() heapBufferSizeAndAlignWithLength:descriptor.m_byteCount
                options : resOptions];

            RHI::ResourceMemoryRequirements memoryRequirements;
            memoryRequirements.m_alignmentInBytes = bufferSizeAndAlign.align;
            memoryRequirements.m_sizeInBytes = bufferSizeAndAlign.size;
            return memoryRequirements;
        }

        void Device::ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction)
        {
            m_releaseQueue.Notify(notifyFunction);
        }

        void Device::InitFeatures()
        {
            m_features.m_geometryShader = false;
            m_features.m_computeShader = true;
            m_features.m_independentBlend = true;
            m_features.m_dualSourceBlending = true;
            m_features.m_customSamplePositions = m_metalDevice.programmableSamplePositionsSupported;
            m_features.m_indirectDrawSupport = false;
            
            //Metal drivers save and load serialized PipelineLibrary internally
            m_features.m_isPsoCacheFileOperationsNeeded = false; 
            
            RHI::QueryTypeFlags counterSamplingFlags = RHI::QueryTypeFlags::None;

            bool supportsInterDrawTimestamps = true;
#if defined(__IPHONE_14_0) || defined(__MAC_11_0) || defined(__TVOS_14_0)
            if (@available(macOS 11.0, iOS 14, tvOS 14, *))
            {
                supportsInterDrawTimestamps = [m_metalDevice supportsCounterSampling:MTLCounterSamplingPointAtDrawBoundary];
            }
            else
#endif
            {
                supportsInterDrawTimestamps = ![m_metalDevice.name containsString:@"Apple"]; // Apple GPU's don't support inter draw timestamps at the M1/A14 generation
            }
            
            if (supportsInterDrawTimestamps)
            {
                counterSamplingFlags |= (RHI::QueryTypeFlags::Timestamp | RHI::QueryTypeFlags::PipelineStatistics);
                m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Copy)] = RHI::QueryTypeFlags::Timestamp;
            }

            m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Graphics)] = RHI::QueryTypeFlags::Occlusion | counterSamplingFlags;
            //Compute queue can do gfx work
            m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Compute)] = RHI::QueryTypeFlags::Occlusion | counterSamplingFlags;            
            m_features.m_occlusionQueryPrecise = true;
            
            m_features.m_unboundedArrays = m_metalDevice.argumentBuffersSupport == MTLArgumentBuffersTier2;
            m_features.m_unboundedArrays = false; //Remove this when unbounded array support is added to spirv-cross
            
            // Metal backend is able to simulate unbounded arrays for Bindless srg. However it is disabled by default as it uses up memory and we dont have
            // any features that is using Bindless at the moment.
            m_features.m_simulateBindlessUA = false;

            //Values taken from https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
            m_limits.m_maxImageDimension1D = 8192;
            m_limits.m_maxImageDimension2D = 8192;
            m_limits.m_maxImageDimension3D = 2048;
            m_limits.m_maxImageDimensionCube = 8192;
            m_limits.m_maxImageArraySize = 2048;
            m_limits.m_minConstantBufferViewOffset = Alignment::Constant;
            m_limits.m_maxConstantBufferSize = m_metalDevice.maxBufferLength;
            m_limits.m_maxBufferSize = m_metalDevice.maxBufferLength;
 
            m_features.m_swapchainScalingFlags = RHI::ScalingFlags::Stretch;
            AZ_Assert(m_metalDevice.argumentBuffersSupport >= MTLArgumentBuffersTier1, "Atom needs Argument buffer support to run");
            
            m_features.m_subpassInputSupport = RHI::SubpassInputSupportType::Color;
        }

        CommandList* Device::AcquireCommandList(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return m_commandListAllocator.Allocate(hardwareQueueClass);
        }
        
        RHI::ConstPtr<PipelineLayout> Device::AcquirePipelineLayout(const RHI::PipelineLayoutDescriptor& descriptor)
        {
            return m_pipelineLayoutCache.Allocate(descriptor);
        }
        
        void Device::QueueForRelease(const MemoryView& memoryView)
        {
            m_releaseQueue.QueueForCollect(memoryView.GetMemory());
        }
        
        void Device::QueueForRelease(Memory* memory)
        {
            m_releaseQueue.QueueForCollect(memory, 1);
        }
    
        RHI::Ptr<Buffer> Device::AcquireStagingBuffer(AZStd::size_t byteCount, RHI::BufferBindFlags bufferBindFlags)
        {
            if (!m_stagingBufferPool)
            {
                m_stagingBufferPool = BufferPool::Create();
                RHI::BufferPoolDescriptor poolDesc;
                poolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
                poolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                poolDesc.m_bindFlags = RHI::BufferBindFlags::CopyRead;
                poolDesc.m_budgetInBytes = RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_stagingBufferBudgetInBytes;
                RHI::ResultCode result = m_stagingBufferPool->Init(*this, poolDesc);
                if (result != RHI::ResultCode::Success)
                {
                    AZ_Assert(false, "Staging buffer pool was not created.");
                }
            }
            
            RHI::Ptr<Buffer> stagingBuffer = Buffer::Create();
            RHI::BufferDescriptor bufferDesc(bufferBindFlags, byteCount);
            RHI::DeviceBufferInitRequest initRequest(*stagingBuffer, bufferDesc);
            const RHI::ResultCode result = m_stagingBufferPool->InitBuffer(initRequest);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Staging buffer is not initialized.");
                return nullptr;
            }

            return stagingBuffer;
        }
    
        NullDescriptorManager& Device::GetNullDescriptorManager()
        {
            return m_nullDescriptorManager;
        }
        
        AZStd::vector<RHI::Format> Device::GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const
        {
            return AZStd::vector<RHI::Format>{RHI::Format::B8G8R8A8_UNORM};
        }
    
        BindlessArgumentBuffer& Device::GetBindlessArgumentBuffer()
        {
            return m_bindlessArgumentBuffer;
        }
    
        RHI::ResultCode Device::ClearRenderAttachments(CommandList& commandList, MTLRenderPassDescriptor* renderpassDesc, const AZStd::vector<ClearAttachments::ClearData>& clearAttachmentData)
        {
            return m_clearAttachments.Clear(commandList, renderpassDesc, clearAttachmentData);
        }
    }
}
