
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ObjectCollector.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/SwapChain.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/BufferMemoryAllocator.h>
#include <RHI/CommandListPool.h>
#include <RHI/Conversions.h>
#include <RHI/MemoryView.h>
#include <RHI/Metal.h>
#include <RHI/PipelineState.h>
#include <RHI/PipelineLayout.h>
#include <RHI/ReleaseQueue.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/MetalView.h>
#include <RHI/MetalViewController.h>
#include <RHI/MetalView_Platform.h>
#include <RHI/NullDescriptorManager.h>

namespace AZ
{
    namespace Metal
    {
        class Device final
            : public RHI::Device
        {
            using Base = RHI::Device;
        public:
            AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator, 0);
            AZ_RTTI(Device, "{04DA2F69-1E6F-42A1-B4F0-7ADC127A5AAB}", Base);

            static RHI::Ptr<Device> Create();

            MemoryView CreateInternalImageCommitted(MTLTextureDescriptor* mtlTextureDesc);
            MemoryView CreateImageCommitted(const RHI::ImageDescriptor& imageDescriptor);
            MemoryView CreateImageCommitted(const RHI::ImageDescriptor& imageDescriptor, MTLStorageMode overrideStorageMode);
                        
            MemoryView CreateImagePlaced(const RHI::ImageDescriptor& imageDescriptor, id<MTLHeap> mtlHeap, size_t heapByteOffset, MTLSizeAndAlign textureSizeAndAlign);
            MemoryView CreateImagePlaced(const RHI::ImageDescriptor& imageDescriptor, id<MTLHeap> mtlHeap, size_t heapByteOffset, MTLSizeAndAlign textureSizeAndAlign, MTLTextureType overrideTextureType);
            
            MemoryView CreateBufferCommitted(const RHI::BufferDescriptor& bufferDescriptor, RHI::HeapMemoryLevel heapMemoryLevel = RHI::HeapMemoryLevel::Device);
            MemoryView CreateBufferPlaced(const RHI::BufferDescriptor& bufferDescriptor, id<MTLHeap> mtlHeap, size_t heapByteOffset, MTLSizeAndAlign bufferSizeAndAlign);
            
            MemoryView CreateResourceCommitted(ResourceDescriptor resourceDesc);

            inline id<MTLDevice> GetMtlDevice()
            {
                return m_metalDevice;
            }

            inline MTLSharedEventListener* GetSharedEventListener()
            {
                return m_eventListener;
            }

            //! Releases an object through the garbage collector, which will defer release
            //! until the current GPU frame has been flushed.
            void QueueDeferredRelease(RHI::Ptr<RHI::Object> object);

            inline CommandListPool& GetCommandListPool(RHI::HardwareQueueClass hardwareQueueClass)
            {
                return m_commandListPools[static_cast<AZ::u32>(hardwareQueueClass)];
            }

            CommandQueue& GetGraphicsCommandQueue()
            {
                return m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Graphics);
            }

            const CommandQueue& GetGraphicsCommandQueue() const
            {
                return m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Graphics);
            }

            CommandQueue& GetCopyCommandQueue()
            {
                return m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Copy);
            }     

            const CommandQueue& GetCopyCommandQueue() const
            {
                return m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Copy);
            }
            
            CommandQueue& GetComputeCommandQueue()
            {
                return m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Compute);
            }
            
            const CommandQueue& GetComputeCommandQueue() const
            {
                return m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Compute);
            }
            
            //! Acquires a new command list for the frame given the hardware queue class. The command list is
            //! automatically reclaimed after the current frame has flushed through the GPU.
            CommandList* AcquireCommandList(RHI::HardwareQueueClass hardwareQueueClass);

            
             //! Queues the backing Memory instance of a MemoryView for release (by taking a reference) after the
             //! current frame has flushed through the GPU. The reference on the MemoryView itself is not released.             
            void QueueForRelease(const MemoryView& memoryView);
            void QueueForRelease(Memory* memory);
            
            //! Acquires a pipeline layout from the internal cache.
            RHI::ConstPtr<PipelineLayout> AcquirePipelineLayout(const RHI::PipelineLayoutDescriptor& descriptor);
            
            RHI::Ptr<Buffer> AcquireStagingBuffer(AZStd::size_t byteCount, RHI::BufferBindFlags bufferBindFlags);
            
            CommandQueueContext& GetCommandQueueContext()
            {
                return m_commandQueueContext;
            }

            const CommandQueueContext& GetCommandQueueContext() const
            {
                return m_commandQueueContext;
            }

            AsyncUploadQueue& GetAsyncUploadQueue()
            {
                return m_asyncUploadQueue;
            }

            const AsyncUploadQueue& GetAsyncUploadQueue() const
            {
                return m_asyncUploadQueue;
            }
            
            const NSCache* GetSamplerCache() const
            {
                return m_samplerCache;
            }
            
            BufferMemoryAllocator& GetArgBufferConstantBufferAllocator() { return m_argumentBufferConstantsAllocator;}
            BufferMemoryAllocator& GetArgumentBufferAllocator() { return m_argumentBufferAllocator;}
            
            NullDescriptorManager& GetNullDescriptorManager();
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements(const RHI::ImageDescriptor & descriptor) override;
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements(const RHI::BufferDescriptor & descriptor) override;
            void ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction) override;

        private:
            Device();
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::Device
            RHI::ResultCode InitInternal(RHI::PhysicalDevice& physicalDevice) override;
            void ShutdownInternal() override;
            void CompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder) override;
            void UpdateCpuTimingStatisticsInternal() const override;
            void BeginFrameInternal() override;
            void EndFrameInternal() override;
            void WaitForIdleInternal() override;
            AZStd::chrono::microseconds GpuTimestampToMicroseconds(uint64_t gpuTimestamp, RHI::HardwareQueueClass queueClass) const override;
            void FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities) override;
            RHI::ResultCode InitializeLimits() override;
            void PreShutdown() override;
            AZStd::vector<RHI::Format> GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const override;
            //////////////////////////////////////////////////////////////////////////

            void InitFeatures();            
            void FlushAutoreleasePool();
            void TryCreateAutoreleasePool();
            
            CommandListAllocator m_commandListAllocator;
            
            AZStd::array<CommandListPool, Metal::CommandEncoderTypeCount> m_commandListPools;

            id<MTLDevice> m_metalDevice = nil;
            MTLSharedEventListener* m_eventListener = nil;
            NSAutoreleasePool* m_autoreleasePool = nullptr;

            CommandQueueContext m_commandQueueContext;
            PipelineLayoutCache m_pipelineLayoutCache;
            ReleaseQueue m_releaseQueue;
            AsyncUploadQueue m_asyncUploadQueue;
            RHI::Ptr<BufferPool> m_stagingBufferPool;
            
            BufferMemoryAllocator m_argumentBufferConstantsAllocator;
            BufferMemoryAllocator m_argumentBufferAllocator;
            RHI::HeapMemoryUsage m_argumentBufferConstantsAllocatorMemoryUsage;
            RHI::HeapMemoryUsage m_argumentBufferAllocatorMemoryUsage;
            
            NullDescriptorManager m_nullDescriptorManager;
            NSCache* m_samplerCache;
        };
    }
}


