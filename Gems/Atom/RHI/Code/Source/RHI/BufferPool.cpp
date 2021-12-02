/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RHI
    {
        BufferInitRequest::BufferInitRequest(
            Buffer& buffer,
            const BufferDescriptor& descriptor,
            const void* initialData)
            : m_buffer{&buffer}
            , m_descriptor{descriptor}
            , m_initialData{initialData}
        {}

        BufferMapRequest::BufferMapRequest(
            Buffer& buffer,
            size_t byteOffset,
            size_t byteCount)
            : m_buffer{&buffer}
            , m_byteOffset{byteOffset}
            , m_byteCount{byteCount}
        {}

        bool BufferPool::ValidatePoolDescriptor(const BufferPoolDescriptor& descriptor) const
        {
            if (Validation::IsEnabled())
            {
                if (descriptor.m_heapMemoryLevel == RHI::HeapMemoryLevel::Device &&
                    descriptor.m_hostMemoryAccess == RHI::HostMemoryAccess::Read)
                {
                    AZ_Error("BufferPool", false, "When HeapMemoryLevel::Device is specified, m_hostMemoryAccess must be HostMemoryAccess::Write.");
                    return false;
                }
            }
            return true;
        }

        bool BufferPool::ValidateInitRequest(const BufferInitRequest& initRequest) const
        {
            if (Validation::IsEnabled())
            {
                const BufferPoolDescriptor& poolDescriptor = GetDescriptor();

                // Bind flags of the buffer must match the pool bind flags.
                if (initRequest.m_descriptor.m_bindFlags != poolDescriptor.m_bindFlags)
                {
                    AZ_Error("BufferPool", false, "Buffer bind flags don't match pool bind flags in pool '%s'", GetName().GetCStr());
                    return false;
                }

                // Initial data is not allowed for read-only heaps.
                if (initRequest.m_initialData && poolDescriptor.m_hostMemoryAccess == HostMemoryAccess::Read)
                {
                    AZ_Error("BufferPool", false, "Initial data is not allowed with read-only pools.");
                    return false;
                }
            }
            return true;
        }

        bool BufferPool::ValidateIsHostHeap() const
        {
            if (Validation::IsEnabled())
            {
                if (GetDescriptor().m_heapMemoryLevel != HeapMemoryLevel::Host)
                {
                    AZ_Error("BufferPool", false, "This operation is only permitted for pools on the Host heap.");
                    return false;
                }
            }
            return true;
        }

        bool BufferPool::ValidateMapRequest(const BufferMapRequest& request) const
        {
            if (Validation::IsEnabled())
            {
                if (request.m_byteCount == 0)
                {
                    AZ_Warning("BufferPool", false, "Trying to map zero bytes from buffer.");
                    return false;
                }
            }
            return true;
        }

        ResultCode BufferPool::Init(Device& device, const BufferPoolDescriptor& descriptor)
        {
            return ResourcePool::Init(
                device, descriptor,
                [this, &device, &descriptor]()
            {
                if (!ValidatePoolDescriptor(descriptor))
                {
                    return ResultCode::InvalidArgument;
                }

                /**
                 * Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                 * for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                 * possibility that users will get garbage values from GetDescriptor().
                 */
                m_descriptor = descriptor;

                return InitInternal(device, descriptor);
            });
        }

        ResultCode BufferPool::InitBuffer(const BufferInitRequest& initRequest)
        {
            AZ_TRACE_METHOD();

            if (!ValidateInitRequest(initRequest))
            {
                return ResultCode::InvalidArgument;
            }

            ResultCode resultCode = BufferPoolBase::InitBuffer(
                initRequest.m_buffer,
                initRequest.m_descriptor,
                [this, &initRequest]() { return InitBufferInternal(*initRequest.m_buffer, initRequest.m_descriptor); });

            if (resultCode == ResultCode::Success && initRequest.m_initialData)
            {
                BufferMapRequest mapRequest;
                mapRequest.m_buffer = initRequest.m_buffer;
                mapRequest.m_byteCount = initRequest.m_descriptor.m_byteCount;
                mapRequest.m_byteOffset = 0;

                BufferMapResponse mapResponse;
                resultCode = MapBufferInternal(mapRequest, mapResponse);
                if (resultCode == ResultCode::Success)
                {
                    BufferCopy(mapResponse.m_data, initRequest.m_initialData, initRequest.m_descriptor.m_byteCount);
                    UnmapBufferInternal(*initRequest.m_buffer);
                }
            }

            return resultCode;
        }

        ResultCode BufferPool::OrphanBuffer(Buffer& buffer)
        {
            if (!ValidateIsInitialized() || !ValidateIsHostHeap() || !ValidateNotProcessingFrame())
            {
                return ResultCode::InvalidOperation;
            }

            if (!ValidateIsRegistered(&buffer))
            {
                return ResultCode::InvalidArgument;
            }
            
            AZ_PROFILE_SCOPE(RHI, "BufferPool::OrphanBuffer");
            return OrphanBufferInternal(buffer);
        }

        ResultCode BufferPool::MapBuffer(const BufferMapRequest& request, BufferMapResponse& response)
        {
            AZ_TRACE_METHOD();

            if (!ValidateIsInitialized() || !ValidateNotProcessingFrame())
            {
                return ResultCode::InvalidOperation;
            }

            if (!ValidateIsRegistered(request.m_buffer))
            {
                return ResultCode::InvalidArgument;
            }

            if (!ValidateMapRequest(request))
            {
                return ResultCode::InvalidArgument;
            }

            ResultCode resultCode = MapBufferInternal(request, response);
            ValidateBufferMap(*request.m_buffer, response.m_data != nullptr);
            return resultCode;
        }

        void BufferPool::UnmapBuffer(Buffer& buffer)
        {
            if (ValidateIsInitialized() && ValidateNotProcessingFrame() && ValidateIsRegistered(&buffer) && ValidateBufferUnmap(buffer))
            {
                UnmapBufferInternal(buffer);
            }
        }

        ResultCode BufferPool::StreamBuffer(const BufferStreamRequest& request)
        {
            if (!ValidateIsInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            if (!ValidateIsRegistered(request.m_buffer))
            {
                return ResultCode::InvalidArgument;
            }

            return StreamBufferInternal(request);
        }

        const BufferPoolDescriptor& BufferPool::GetDescriptor() const
        {
            return m_descriptor;
        }

        void BufferPool::BufferCopy(void* destination, const void* source, size_t num)
        {
            memcpy(destination, source, num);
        }

        ResultCode BufferPool::StreamBufferInternal([[maybe_unused]] const BufferStreamRequest& request)
        {
            return ResultCode::Unimplemented;
        }

        bool BufferPool::ValidateNotProcessingFrame() const
        {
            return GetDescriptor().m_heapMemoryLevel != HeapMemoryLevel::Device || BufferPoolBase::ValidateNotProcessingFrame();
        }

        void BufferPool::OnFrameBegin()
        {
            if (Validation::IsEnabled())
            {
                AZ_Error(
                    "BufferPool", GetMapRefCount() == 0 || GetDescriptor().m_heapMemoryLevel != HeapMemoryLevel::Device,
                    "There are currently buffers mapped on buffer pool '%s'. All buffers must be "
                    "unmapped when the frame is processing.", GetName().GetCStr());
            }

            ResourcePool::OnFrameBegin();
        }
    }
}
