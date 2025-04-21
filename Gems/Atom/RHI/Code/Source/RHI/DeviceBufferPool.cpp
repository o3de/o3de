/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
namespace AZ::RHI
{
    bool DeviceBufferPool::ValidatePoolDescriptor(const BufferPoolDescriptor& descriptor) const
    {
        if (Validation::IsEnabled())
        {
            if (descriptor.m_heapMemoryLevel == RHI::HeapMemoryLevel::Device &&
                descriptor.m_hostMemoryAccess == RHI::HostMemoryAccess::Read)
            {
                AZ_Error("DeviceBufferPool", false, "When HeapMemoryLevel::Device is specified, m_hostMemoryAccess must be HostMemoryAccess::Write.");
                return false;
            }
        }
        return true;
    }

    bool DeviceBufferPool::ValidateInitRequest(const DeviceBufferInitRequest& initRequest) const
    {
        if (Validation::IsEnabled())
        {
            const BufferPoolDescriptor& poolDescriptor = GetDescriptor();

            // Bind flags of the buffer must match the pool bind flags.
            if (initRequest.m_descriptor.m_bindFlags != poolDescriptor.m_bindFlags)
            {
                AZ_Error("DeviceBufferPool", false, "DeviceBuffer bind flags don't match pool bind flags in pool '%s'", GetName().GetCStr());
                return false;
            }

            // Initial data is not allowed for read-only heaps.
            if (initRequest.m_initialData && poolDescriptor.m_hostMemoryAccess == HostMemoryAccess::Read)
            {
                AZ_Error("DeviceBufferPool", false, "Initial data is not allowed with read-only pools.");
                return false;
            }
        }
        return true;
    }

    bool DeviceBufferPool::ValidateIsHostHeap() const
    {
        if (Validation::IsEnabled())
        {
            if (GetDescriptor().m_heapMemoryLevel != HeapMemoryLevel::Host)
            {
                AZ_Error("DeviceBufferPool", false, "This operation is only permitted for pools on the Host heap.");
                return false;
            }
        }
        return true;
    }

    bool DeviceBufferPool::ValidateMapRequest(const DeviceBufferMapRequest& request) const
    {
        if (Validation::IsEnabled())
        {
            if (!request.m_buffer)
            {
                AZ_Error("DeviceBufferPool", false, "Trying to map a null buffer.");
                return false;
            }

            if (request.m_byteCount == 0)
            {
                AZ_Warning("DeviceBufferPool", false, "Trying to map zero bytes from buffer '%s'.", request.m_buffer->GetName().GetCStr());
                return false;
            }

            if (request.m_byteOffset + request.m_byteCount > request.m_buffer->GetDescriptor().m_byteCount)
            {
                AZ_Error(
                    "DeviceBufferPool", false, "Unable to map buffer '%s', overrunning the size of the buffer.",
                    request.m_buffer->GetName().GetCStr());
                return false;
            }
        }
        return true;
    }

    ResultCode DeviceBufferPool::Init(Device& device, const BufferPoolDescriptor& descriptor)
    {
        return DeviceResourcePool::Init(
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

    ResultCode DeviceBufferPool::InitBuffer(const DeviceBufferInitRequest& initRequest)
    {
        AZ_PROFILE_FUNCTION(RHI);

        if (!ValidateInitRequest(initRequest))
        {
            return ResultCode::InvalidArgument;
        }

        ResultCode resultCode = DeviceBufferPoolBase::InitBuffer(
            initRequest.m_buffer,
            initRequest.m_descriptor,
            [this, &initRequest]() { return InitBufferInternal(*initRequest.m_buffer, initRequest.m_descriptor); });

        if (resultCode == ResultCode::Success && initRequest.m_initialData)
        {
            DeviceBufferMapRequest mapRequest;
            mapRequest.m_buffer = initRequest.m_buffer;
            mapRequest.m_byteCount = initRequest.m_descriptor.m_byteCount;
            mapRequest.m_byteOffset = 0;

            DeviceBufferMapResponse mapResponse;
            resultCode = MapBufferInternal(mapRequest, mapResponse);
            if (resultCode == ResultCode::Success)
            {
                BufferCopy(mapResponse.m_data, initRequest.m_initialData, initRequest.m_descriptor.m_byteCount);
                UnmapBufferInternal(*initRequest.m_buffer);
            }
        }

        return resultCode;
    }

    ResultCode DeviceBufferPool::OrphanBuffer(DeviceBuffer& buffer)
    {
        if (!ValidateIsInitialized() || !ValidateIsHostHeap() || !ValidateNotProcessingFrame())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateIsRegistered(&buffer))
        {
            return ResultCode::InvalidArgument;
        }
            
        AZ_PROFILE_SCOPE(RHI, "DeviceBufferPool::OrphanBuffer");
        return OrphanBufferInternal(buffer);
    }

    ResultCode DeviceBufferPool::MapBuffer(const DeviceBufferMapRequest& request, DeviceBufferMapResponse& response)
    {
        AZ_PROFILE_FUNCTION(RHI);

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

    void DeviceBufferPool::UnmapBuffer(DeviceBuffer& buffer)
    {
        if (ValidateIsInitialized() && ValidateNotProcessingFrame() && ValidateIsRegistered(&buffer) && ValidateBufferUnmap(buffer))
        {
            UnmapBufferInternal(buffer);
        }
    }

    ResultCode DeviceBufferPool::StreamBuffer(const DeviceBufferStreamRequest& request)
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

    const BufferPoolDescriptor& DeviceBufferPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void DeviceBufferPool::BufferCopy(void* destination, const void* source, size_t num)
    {
        memcpy(destination, source, num);
    }

    ResultCode DeviceBufferPool::StreamBufferInternal([[maybe_unused]] const DeviceBufferStreamRequest& request)
    {
        return ResultCode::Unimplemented;
    }

    bool DeviceBufferPool::ValidateNotProcessingFrame() const
    {
        return GetDescriptor().m_heapMemoryLevel != HeapMemoryLevel::Device || DeviceBufferPoolBase::ValidateNotProcessingFrame();
    }

    void DeviceBufferPool::OnFrameBegin()
    {
        if (Validation::IsEnabled())
        {
            AZ_Error(
                "DeviceBufferPool", GetMapRefCount() == 0 || GetDescriptor().m_heapMemoryLevel != HeapMemoryLevel::Device,
                "There are currently buffers mapped on buffer pool '%s'. All buffers must be "
                "unmapped when the frame is processing.", GetName().GetCStr());
        }

        DeviceResourcePool::OnFrameBegin();
    }
}
