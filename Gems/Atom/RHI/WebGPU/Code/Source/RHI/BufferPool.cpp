/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferPoolResolver.h>
#include <RHI/Device.h>
#include <RHI/Instance.h>

namespace AZ::WebGPU
{
    RHI::Ptr<BufferPool> BufferPool::Create()
    {
        return aznew BufferPool();
    }

    RHI::ResultCode BufferPool::InitInternal(RHI::Device& deviceBase, const RHI::BufferPoolDescriptor& descriptorBase)
    {
        auto& device = static_cast<Device&>(deviceBase);
        SetResolver(AZStd::make_unique<BufferPoolResolver>(device, descriptorBase));
        if (auto* bufferPoolDescriptor = azrtti_cast<const BufferPoolDescriptor*>(&descriptorBase))
        {
            if (bufferPoolDescriptor->m_mappedAtCreation)
            {
                m_extraInitFlags |= Buffer::InitFlags::MappedAtCreation;
            }
        }
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode BufferPool::InitBufferInternal(RHI::DeviceBuffer& bufferBase, const RHI::BufferDescriptor& bufferDescriptor)
    {
        auto& buffer = static_cast<Buffer&>(bufferBase);
        auto& device = static_cast<Device&>(GetDevice());

        const auto& poolDescriptor = GetDescriptor();
        RHI::HeapMemoryLevel heapMemoryLevel = poolDescriptor.m_heapMemoryLevel;
        RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
        if (!heapMemoryUsage.CanAllocate(bufferDescriptor.m_byteCount))
        {
            AZ_Error("WebGPU", false, "Failed to initialize buffer to due memory budget constraints");
            return RHI::ResultCode::OutOfMemory;
        }

        Buffer::InitFlags initFlags = m_extraInitFlags;
        if (heapMemoryLevel == RHI::HeapMemoryLevel::Host)
        {
            // If we create a mappeable buffer for read, we can only use the CopyWrite bindflag
            if (poolDescriptor.m_hostMemoryAccess == RHI::HostMemoryAccess::Read &&
                (bufferDescriptor.m_bindFlags == RHI::BufferBindFlags::CopyWrite ||
                 bufferDescriptor.m_bindFlags == RHI::BufferBindFlags::None))
            {
                initFlags |= Buffer::InitFlags::MapRead;
            }
            // If we create a mappeable buffer for write, we can only use the CopyRead bindflag
            else if (
                poolDescriptor.m_hostMemoryAccess == RHI::HostMemoryAccess::Write &&
                (bufferDescriptor.m_bindFlags == RHI::BufferBindFlags::CopyRead ||
                 bufferDescriptor.m_bindFlags == RHI::BufferBindFlags::None))
            {
                initFlags |= Buffer::InitFlags::MapWrite;
            }
        }

        RHI::BufferDescriptor descriptor = bufferDescriptor;
        if (!RHI::CheckBitsAny(initFlags, Buffer::InitFlags::MapWrite))
        {
            // Add the copy write flag since it's needed for staging copies and clear operations.
            descriptor.m_bindFlags |= RHI::BufferBindFlags::CopyWrite;
        }

        RHI::ResultCode result = buffer.Init(device, descriptor, initFlags);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        heapMemoryUsage.m_usedResidentInBytes += bufferDescriptor.m_byteCount;
        heapMemoryUsage.m_totalResidentInBytes += bufferDescriptor.m_byteCount;
        return result;
    }

    void BufferPool::ShutdownResourceInternal(RHI::DeviceResource& resource)
    {
        auto& buffer = static_cast<Buffer&>(resource);

        // Wait for any pending streaming upload.
        // device.GetAsyncUploadQueue().WaitForUpload(buffer.GetUploadHandle());

        if (auto* resolver = GetResolver())
        {
            ResourcePoolResolver* poolResolver = static_cast<ResourcePoolResolver*>(resolver);
            poolResolver->OnResourceShutdown(resource);
        }

        RHI::HeapMemoryLevel heapMemoryLevel = GetDescriptor().m_heapMemoryLevel;
        RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
        size_t sizeInBytes = buffer.GetDescriptor().m_byteCount;
        heapMemoryUsage.m_usedResidentInBytes -= sizeInBytes;
        heapMemoryUsage.m_totalResidentInBytes -= sizeInBytes;

        // Deallocate the BufferMemory
        buffer.Invalidate();
    }

    RHI::ResultCode BufferPool::OrphanBufferInternal(RHI::DeviceBuffer& bufferBase)
    {
        auto& buffer = static_cast<Buffer&>(bufferBase);

        // Deallocate the BufferMemory
        buffer.Invalidate();

        auto result = InitBufferInternal(bufferBase, bufferBase.GetDescriptor());
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        buffer.InvalidateViews();
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode BufferPool::MapBufferInternal(const RHI::DeviceBufferMapRequest& mapRequest, RHI::DeviceBufferMapResponse& response)
    {
        const RHI::BufferPoolDescriptor& descriptor = GetDescriptor();
        auto* buffer = static_cast<Buffer*>(mapRequest.m_buffer);

        void* mappedData = nullptr;
        auto& instance = Instance::GetInstance().GetNativeInstance();
        if (buffer->CanBeMap())
        {
            size_t byteOffset = RHI::AlignDown(mapRequest.m_byteOffset, MapOffsetAligment);
            size_t byteCount = RHI::AlignUp(mapRequest.m_byteCount, MapSizeAligment);
            instance.WaitAny(
                buffer->GetNativeBuffer().MapAsync(
                    ConvertMapMode(descriptor.m_hostMemoryAccess),
                    byteOffset,
                    byteCount,
                    wgpu::CallbackMode::WaitAnyOnly,
                    [&](wgpu::MapAsyncStatus status, const char* message)
                    {
                        if (status != wgpu::MapAsyncStatus::Success)
                        {
                            AZ_Assert(false, "Failed to map buffer: %s", message);
                            return;
                        }
                        mappedData = buffer->GetNativeBuffer().GetMappedRange(byteOffset, byteCount);
                    }),
                UINT64_MAX);

            if (mappedData)
            {
                mappedData = static_cast<int8_t*>(mappedData) + mapRequest.m_byteOffset - byteOffset;
            }
            else
            {
                return RHI::ResultCode::Fail;
            }
        }
        else
        {
            mappedData = static_cast<BufferPoolResolver*>(GetResolver())->MapBuffer(mapRequest);
            if (mappedData)
            {
                m_memoryUsage.m_transferPull.m_bytesPerFrame += mapRequest.m_byteCount;
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
        auto& buffer = static_cast<Buffer&>(bufferBase);
        if (buffer.CanBeMap())
        {
            buffer.GetNativeBuffer().Unmap();
        }
    }

    void BufferPool::ShutdownInternal()
    {
    }

    void BufferPool::OnFrameEnd()
    {
        Base::OnFrameEnd();
    }
}
