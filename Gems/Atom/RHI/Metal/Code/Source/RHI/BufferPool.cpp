/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI.Reflect/Metal/BufferPoolDescriptor.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferPoolResolver.h>
#include <RHI/Device.h>

namespace Platform
{
    AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::DeviceBufferMapRequest& request, AZ::RHI::DeviceBufferMapResponse& response);
    void UnMapBufferInternal(AZ::RHI::DeviceBuffer& bufferBase);
}

namespace AZ
{
    namespace Metal
    {

        RHI::Ptr<BufferPool> BufferPool::Create()
        {
            return aznew BufferPool();
        }
        
        BufferPoolResolver* BufferPool::GetResolver()
        {
            return static_cast<BufferPoolResolver*>(Base::GetResolver());
        }
    
        Device& BufferPool::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        RHI::ResultCode BufferPool::InitInternal(RHI::Device& deviceBase, const RHI::BufferPoolDescriptor& descriptorBase)
        {
            Device& device = static_cast<Device&>(deviceBase);
            
            RHI::HeapMemoryUsage* heapMemoryUsage = &m_memoryUsage.GetHeapMemoryUsage(descriptorBase.m_heapMemoryLevel);
            uint32_t bufferPageSize = static_cast<uint32_t>(RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_bufferPoolPageSizeInBytes);
            
            // The descriptor provides an explicit buffer page size override.
            if (const Metal::BufferPoolDescriptor* descriptor = azrtti_cast<const Metal::BufferPoolDescriptor*>(&descriptorBase))
            {
                bufferPageSize = descriptor->m_bufferPoolPageSizeInBytes;
            }
            
            BufferMemoryAllocator::Descriptor allocatorDescriptor;
            allocatorDescriptor.m_device = &device;
            allocatorDescriptor.m_pageSizeInBytes = bufferPageSize;
            allocatorDescriptor.m_bindFlags = descriptorBase.m_bindFlags;
            allocatorDescriptor.m_heapMemoryLevel = descriptorBase.m_heapMemoryLevel;
            allocatorDescriptor.m_hostMemoryAccess = descriptorBase.m_hostMemoryAccess;
            allocatorDescriptor.m_getHeapMemoryUsageFunction = [=]() { return heapMemoryUsage; };
            allocatorDescriptor.m_recycleOnCollect = false;
            m_allocator.Init(allocatorDescriptor);

            if (descriptorBase.m_heapMemoryLevel == RHI::HeapMemoryLevel::Device)
            {
                SetResolver(AZStd::make_unique<BufferPoolResolver>(device, descriptorBase));
            }
            
            return RHI::ResultCode::Success;
        }

        void BufferPool::ShutdownInternal()
        {
            m_allocator.Shutdown();
        }

        void BufferPool::OnFrameEnd()
        {
            m_allocator.GarbageCollect();
            Base::OnFrameEnd();
        }

        RHI::ResultCode BufferPool::InitBufferInternal(RHI::DeviceBuffer& bufferBase, const RHI::BufferDescriptor& bufferDescriptor)
        {
            BufferMemoryView memoryView = m_allocator.Allocate(bufferDescriptor.m_byteCount);
            if (memoryView.IsValid())
            {
                if (memoryView.GetType() == BufferMemoryType::Unique && !bufferBase.GetName().IsEmpty())
                {
                    memoryView.SetName(bufferBase.GetName().GetStringView());
                }
                
                Buffer& buffer = static_cast<Buffer&>(bufferBase);
                buffer.m_memoryView = AZStd::move(memoryView);
                return RHI::ResultCode::Success;
            }
            return RHI::ResultCode::OutOfMemory;
        }

        void BufferPool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            Buffer& buffer = static_cast<Buffer&>(resourceBase);

            if (auto* resolver = GetResolver())
            {
                resolver->OnResourceShutdown(resourceBase);
            }

            m_allocator.DeAllocate(buffer.m_memoryView);
            buffer.m_memoryView = {};
            buffer.m_pendingResolves = 0;
        }

        RHI::ResultCode BufferPool::OrphanBufferInternal(RHI::DeviceBuffer& bufferBase)
        {
            Buffer& buffer = static_cast<Buffer&>(bufferBase);
            
            BufferMemoryView newMemoryView = m_allocator.Allocate(bufferBase.GetDescriptor().m_byteCount);
            if (newMemoryView.IsValid() && buffer.m_memoryView.IsValid())
            {
                m_allocator.DeAllocate(buffer.m_memoryView);
                buffer.m_memoryView = AZStd::move(newMemoryView);
                buffer.InvalidateViews();
                return RHI::ResultCode::Success;
            }
            return RHI::ResultCode::OutOfMemory;
        }
        
        RHI::ResultCode BufferPool::MapBufferInternal(const RHI::DeviceBufferMapRequest& request, RHI::DeviceBufferMapResponse& response)
        {
            Buffer& buffer = *static_cast<Buffer*>(request.m_buffer);            
            MTLStorageMode mtlStorageMode = buffer.GetMemoryView().GetStorageMode();
            
            switch(mtlStorageMode)
            {
                case MTLStorageModeShared:
                {
                    void* systemAddress = buffer.GetMemoryView().GetCpuAddress();
                    CpuVirtualAddress mappedData = static_cast<CpuVirtualAddress>(systemAddress);
                    
                    if (!mappedData)
                    {
                        return RHI::ResultCode::Fail;
                    }
                    mappedData += request.m_byteOffset;
                    
                    response.m_data = mappedData;
                    break;
                }
                case MTLStorageModePrivate:
                {
                    void* mappedData = GetResolver()->MapBuffer(request);
                    if (mappedData)
                    {
                        m_memoryUsage.m_transferPull.m_bytesPerFrame += request.m_byteCount;
                    }
                    else
                    {
                        return RHI::ResultCode::OutOfMemory;
                    }
                    response.m_data = mappedData;
                    break;
                }
                default:
                {
                    return Platform::MapBufferInternal(request, response);
                }
            }
                     
            return RHI::ResultCode::Success;
        }

        void BufferPool::UnmapBufferInternal(RHI::DeviceBuffer& bufferBase)
        {
            //MTLStorageModeShared - We need to do nothing here as the memory is shared
            //MTLStorageModePrivate - Do nothing because the Resolver will take care of this via a staging buffer
            //MTLStorageModeManaged - The method below should address synchronization needed for MTLStorageModeManaged memory for mac
            Platform::UnMapBufferInternal(bufferBase);
        }
        
        RHI::ResultCode BufferPool::StreamBufferInternal(const RHI::DeviceBufferStreamRequest& request)
        {
            GetDevice().GetAsyncUploadQueue().QueueUpload(request);
            return RHI::ResultCode::Success;
        }

        void BufferPool::ComputeFragmentation() const
        {
            float fragmentation = m_allocator.ComputeFragmentation();

            const RHI::BufferPoolDescriptor& descriptor = GetDescriptor();
            m_memoryUsage.GetHeapMemoryUsage(descriptor.m_heapMemoryLevel).m_fragmentation = fragmentation;
        }
    }
}
