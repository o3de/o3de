/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <RHI/BufferPool.h>
#include <RHI/Buffer.h>
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/ResourcePoolResolver.h>
#include <Atom/RHI.Reflect/DX12/BufferPoolDescriptor.h>
#include <AzCore/Casting/lossy_cast.h>

#include <algorithm>

namespace AZ
{
    namespace DX12
    {
        class BufferPoolResolver
            : public ResourcePoolResolver
        {
        public:
            AZ_RTTI(BufferPoolResolver, "{116743AC-5861-4BF8-9ED9-3DDB644AC004}", ResourcePoolResolver);
            AZ_CLASS_ALLOCATOR(BufferPoolResolver, AZ::SystemAllocator);

            BufferPoolResolver(Device& device, const RHI::BufferPoolDescriptor& descriptor)
            {
                m_device = &device;

                if(RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::DynamicInputAssembly))
                {
                    m_readOnlyState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER;
                }
                if (RHI::CheckBitsAll(descriptor.m_bindFlags, RHI::BufferBindFlags::Constant))
                {
                    m_readOnlyState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
                }
                if (RHI::CheckBitsAll(descriptor.m_bindFlags, RHI::BufferBindFlags::ShaderRead))
                {
                    m_readOnlyState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                }
                if (RHI::CheckBitsAll(descriptor.m_bindFlags, RHI::BufferBindFlags::Indirect))
                {
                    m_readOnlyState |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                }
            }

            CpuVirtualAddress MapBuffer(const RHI::DeviceBufferMapRequest& request)
            {
                AZ_PROFILE_FUNCTION(RHI);

                MemoryView stagingMemory = m_device->AcquireStagingMemory(request.m_byteCount, Alignment::Buffer);
                if (!stagingMemory.IsValid())
                {
                    return nullptr;
                }

                BufferUploadPacket uploadRequest;
                
                // Fill the packet with the source and destination regions for copy.
                Buffer* buffer = static_cast<Buffer*>(request.m_buffer);
                buffer->m_pendingResolves++;

                uploadRequest.m_buffer = buffer;
                uploadRequest.m_memory              = buffer->GetMemoryView().GetMemory();
                uploadRequest.m_memoryByteOffset    = buffer->GetMemoryView().GetOffset() + request.m_byteOffset;
                uploadRequest.m_sourceMemory        = stagingMemory;

                auto address = uploadRequest.m_sourceMemory.Map(RHI::HostMemoryAccess::Write);

                // Once the uploadRequest has been processed, add it to the uploadPackets queue.
                m_uploadPacketsLock.lock();
                m_uploadPackets.emplace_back(AZStd::move(uploadRequest));
                m_uploadPacketsLock.unlock();

                return address;
            }

            void Compile(Scope& scope) override
            {
                AZ_UNUSED(scope);
                for (BufferUploadPacket& packet : m_uploadPackets)
                {
                    packet.m_sourceMemory.Unmap(RHI::HostMemoryAccess::Write);

                    if (packet.m_buffer->IsAttachment())
                    {
                        // Informs the graph compiler that this buffer is in the copy destination state.
                        packet.m_buffer->m_initialAttachmentState = D3D12_RESOURCE_STATE_COPY_DEST;
                    }
                    else
                    {
                        // Tracks the union of non-attachment buffers which are transitioned manually.
                        m_nonAttachmentBufferUnion.emplace(packet.m_memory);
                    }
                }
            }

            void Resolve(CommandList& commandList) const override
            {
                for (const BufferUploadPacket& packet : m_uploadPackets)
                {
                    commandList.GetCommandList()->CopyBufferRegion(
                        packet.m_memory,
                        packet.m_memoryByteOffset,
                        packet.m_sourceMemory.GetMemory(),
                        packet.m_sourceMemory.GetOffset(),
                        packet.m_sourceMemory.GetSize());
                }
            }

            void QueueEpilogueTransitionBarriers(CommandList& commandList) const override
            {
                for (ID3D12Resource* resource : m_nonAttachmentBufferUnion)
                {
                    commandList.QueueTransitionBarrier(resource, D3D12_RESOURCE_STATE_COPY_DEST, m_readOnlyState);
                }
            }

            void Deactivate() override
            {
                AZStd::for_each(m_uploadPackets.begin(), m_uploadPackets.end(), [](auto& packet)
                {
                    AZ_Assert(packet.m_buffer->m_pendingResolves, "There's no pending resolves for buffer %s", packet.m_buffer->GetName().GetCStr());
                    packet.m_buffer->m_pendingResolves--;
                });

                m_uploadPackets.clear();
                m_nonAttachmentBufferUnion.clear();
            }

            void OnResourceShutdown(const RHI::DeviceResource& resource) override
            {
                const Buffer& buffer = static_cast<const Buffer&>(resource);
                if (!buffer.m_pendingResolves)
                {
                    return;
                }

                AZStd::lock_guard<AZStd::mutex> lock(m_uploadPacketsLock);
                auto eraseBeginIt = std::stable_partition(
                    m_uploadPackets.begin(),
                    m_uploadPackets.end(),
                    [&buffer](const BufferUploadPacket& packet)
                    {
                        return packet.m_buffer != &buffer;
                    }
                );

                for (auto it = eraseBeginIt; it != m_uploadPackets.end(); ++it)
                {
                    it->m_sourceMemory.Unmap(RHI::HostMemoryAccess::Write);
                }
                m_uploadPackets.resize(AZStd::distance(m_uploadPackets.begin(), eraseBeginIt));
                m_nonAttachmentBufferUnion.erase(buffer.GetMemoryView().GetMemory());
            }

        private:
            struct BufferUploadPacket
            {
                Buffer* m_buffer = nullptr;

                // Buffer properties are held directly to avoid an indirection through Buffer in the inner loops.
                Memory* m_memory = nullptr;
                size_t m_memoryByteOffset = 0;

                MemoryView m_sourceMemory;
            };

            Device* m_device = nullptr;
            D3D12_RESOURCE_STATES m_readOnlyState = D3D12_RESOURCE_STATE_COMMON;
            AZStd::mutex m_uploadPacketsLock;
            AZStd::vector<BufferUploadPacket> m_uploadPackets;
            AZStd::unordered_set<ID3D12Resource*> m_nonAttachmentBufferUnion;
        };

        //////////////////////////////////////////////////////////////////////////
        //
        // BufferPool
        //
        //////////////////////////////////////////////////////////////////////////

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

            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(descriptorBase.m_heapMemoryLevel);

            uint32_t bufferPageSize = static_cast<uint32_t>(RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_bufferPoolPageSizeInBytes);

            // The DX12 descriptor provides an explicit buffer page size override.
            if (const DX12::BufferPoolDescriptor* descriptor = azrtti_cast<const DX12::BufferPoolDescriptor*>(&descriptorBase))
            {
                bufferPageSize = descriptor->m_bufferPoolPageSizeInBytes;
            }

            if (descriptorBase.m_largestPooledAllocationSizeInBytes > 0)
            {
                bufferPageSize = AZStd::max<uint32_t>(bufferPageSize, azlossy_cast<uint32_t>(descriptorBase.m_largestPooledAllocationSizeInBytes));
            }

            BufferMemoryAllocator::Descriptor allocatorDescriptor;
            allocatorDescriptor.m_device = &device;
            allocatorDescriptor.m_pageSizeInBytes = bufferPageSize;
            allocatorDescriptor.m_bindFlags = descriptorBase.m_bindFlags;
            allocatorDescriptor.m_heapMemoryLevel = descriptorBase.m_heapMemoryLevel;
            allocatorDescriptor.m_hostMemoryAccess = descriptorBase.m_hostMemoryAccess;
            allocatorDescriptor.m_getHeapMemoryUsageFunction = [&]() { return &heapMemoryUsage; };
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
            AZ_PROFILE_FUNCTION(RHI);

            // We need respect the buffer's alignment if the buffer is used for SRV or UAV
            bool useBufferAlignment = RHI::CheckBitsAny(bufferDescriptor.m_bindFlags,
                RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::ShaderWrite);

            size_t overrideAlignment = useBufferAlignment? bufferDescriptor.m_alignment : 0;

            BufferMemoryView memoryView = m_allocator.Allocate(bufferDescriptor.m_byteCount, overrideAlignment);
            if (memoryView.IsValid())
            {
                // Unique memoryView can inherit the name of the buffer.
                if (memoryView.GetType() == BufferMemoryType::Unique && !bufferBase.GetName().IsEmpty())
                {
                    memoryView.SetName(bufferBase.GetName().GetStringView());
                }

                Buffer& buffer = static_cast<Buffer&>(bufferBase);
                buffer.m_memoryView = AZStd::move(memoryView);
                buffer.m_initialAttachmentState = ConvertInitialResourceState(GetDescriptor().m_heapMemoryLevel, GetDescriptor().m_hostMemoryAccess);
                return RHI::ResultCode::Success;
            }
            return RHI::ResultCode::OutOfMemory;
        }

        void BufferPool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            if (auto* resolver = GetResolver())
            {
                resolver->OnResourceShutdown(resourceBase);
            }

            Buffer& buffer = static_cast<Buffer&>(resourceBase);
            m_allocator.DeAllocate(buffer.m_memoryView);
            buffer.m_memoryView = {};
            buffer.m_initialAttachmentState = D3D12_RESOURCE_STATE_COMMON;            
            buffer.m_pendingResolves = 0;
        }

        RHI::ResultCode BufferPool::OrphanBufferInternal(RHI::DeviceBuffer& bufferBase)
        {
            Buffer& buffer = static_cast<Buffer&>(bufferBase);

            BufferMemoryView newMemoryView = m_allocator.Allocate(bufferBase.GetDescriptor().m_byteCount);
            if (newMemoryView.IsValid())
            {
                if (newMemoryView.GetType() == BufferMemoryType::Unique && !bufferBase.GetName().IsEmpty())
                {
                    newMemoryView.SetName(bufferBase.GetName().GetStringView());
                }
                m_allocator.DeAllocate(buffer.m_memoryView);
                buffer.m_memoryView = AZStd::move(newMemoryView);
                buffer.InvalidateViews();
                return RHI::ResultCode::Success;
            }
            return RHI::ResultCode::OutOfMemory;
        }

        RHI::ResultCode BufferPool::MapBufferInternal(const RHI::DeviceBufferMapRequest& request, RHI::DeviceBufferMapResponse& response)
        {
            AZ_PROFILE_FUNCTION(RHI);

            const RHI::BufferPoolDescriptor& poolDescriptor = GetDescriptor();
            Buffer& buffer = *static_cast<Buffer*>(request.m_buffer);
            CpuVirtualAddress mappedData = nullptr;

            if (poolDescriptor.m_heapMemoryLevel == RHI::HeapMemoryLevel::Host)
            {
                mappedData = buffer.GetMemoryView().Map(poolDescriptor.m_hostMemoryAccess);

                if (!mappedData)
                {
                    return RHI::ResultCode::Fail;
                }
                mappedData += request.m_byteOffset;
                
            }
            else
            {
                mappedData = GetResolver()->MapBuffer(request);
                if (mappedData)
                {
                    m_memoryUsage.m_transferPull.m_bytesPerFrame += RHI::AlignUp(request.m_byteCount, Alignment::Buffer);
                }
                else
                {
                    return RHI::ResultCode::OutOfMemory;
                }
            }

            response.m_data = mappedData;
            return RHI::ResultCode::Success;
        }

        void BufferPool::UnmapBufferInternal(RHI::DeviceBuffer& bufferBase)
        {
            const RHI::BufferPoolDescriptor& poolDescriptor = GetDescriptor();
            Buffer& buffer = static_cast<Buffer&>(bufferBase);

            if (poolDescriptor.m_heapMemoryLevel == RHI::HeapMemoryLevel::Host)
            {
                buffer.GetMemoryView().Unmap(poolDescriptor.m_hostMemoryAccess);
            }
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
