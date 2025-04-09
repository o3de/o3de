/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/Fence.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    bool BufferPool::ValidatePoolDescriptor(const BufferPoolDescriptor& descriptor) const
    {
        if (Validation::IsEnabled())
        {
            if (descriptor.m_heapMemoryLevel == RHI::HeapMemoryLevel::Device &&
                descriptor.m_hostMemoryAccess == RHI::HostMemoryAccess::Read)
            {
                AZ_Error(
                    "BufferPool",
                    false,
                    "When HeapMemoryLevel::Device is specified, m_hostMemoryAccess must be HostMemoryAccess::Write.");
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
                AZ_Error(
                    "BufferPool",
                    false,
                    "Buffer bind flags don't match pool bind flags in pool '%s'",
                    GetName().GetCStr());
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
            if (!request.m_buffer)
            {
                AZ_Error("BufferPool", false, "Trying to map a null buffer.");
                return false;
            }

            if (request.m_byteCount == 0)
            {
                AZ_Warning(
                    "BufferPool", false, "Trying to map zero bytes from buffer '%s'.", request.m_buffer->GetName().GetCStr());
                return false;
            }

            if (request.m_byteOffset + request.m_byteCount > request.m_buffer->GetDescriptor().m_byteCount)
            {
                AZ_Error(
                    "BufferPool",
                    false,
                    "Unable to map buffer '%s', overrunning the size of the buffer.",
                    request.m_buffer->GetName().GetCStr());
                return false;
            }
        }
        return true;
    }

    ResultCode BufferPool::Init(const BufferPoolDescriptor& descriptor)
    {
        return ResourcePool::Init(
            descriptor.m_deviceMask,
            [this, &descriptor]()
            {
                if (!ValidatePoolDescriptor(descriptor))
                {
                    return ResultCode::InvalidArgument;
                }

                // Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                // for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                // possibility that users will get garbage values from GetDescriptor().
                m_descriptor = descriptor;

                ResultCode result = ResultCode::Success;

                IterateDevices(
                    [this, &descriptor, &result](int deviceIndex)
                    {
                        auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                        m_deviceObjects[deviceIndex] = Factory::Get().CreateBufferPool();

                        result = GetDeviceBufferPool(deviceIndex)->Init(*device, descriptor);

                        return result == ResultCode::Success;
                    });

                if (result != ResultCode::Success)
                {
                    // Reset already initialized device-specific BufferPools and set deviceMask to 0
                    m_deviceObjects.clear();
                    MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
                }

                return result;
            });
    }

    ResultCode BufferPool::InitBuffer(const BufferInitRequest& initRequest)
    {
        AZ_PROFILE_FUNCTION(RHI);

        if (!ValidateInitRequest(initRequest))
        {
            return ResultCode::InvalidArgument;
        }

        ResultCode resultCode = InitBuffer(
            initRequest.m_buffer,
            initRequest.m_descriptor,
            [this, &initRequest]()
            {
                initRequest.m_buffer->Init(GetDeviceMask() & initRequest.m_deviceMask);

                return IterateObjects<DeviceBufferPool>(
                    [&initRequest](auto deviceIndex, auto deviceBufferPool)
                    {
                        if (CheckBit(initRequest.m_buffer->GetDeviceMask(), deviceIndex))
                        {
                            if (!initRequest.m_buffer->m_deviceObjects.contains(deviceIndex))
                            {
                                initRequest.m_buffer->m_deviceObjects[deviceIndex] = Factory::Get().CreateBuffer();
                            }

                            DeviceBufferInitRequest bufferInitRequest(
                                *initRequest.m_buffer->GetDeviceBuffer(deviceIndex), initRequest.m_descriptor, initRequest.m_initialData);
                            return deviceBufferPool->InitBuffer(bufferInitRequest);
                        }
                        else
                        {
                            initRequest.m_buffer->m_deviceObjects.erase(deviceIndex);
                        }

                        return ResultCode::Success;
                    });
            });

        return resultCode;
    }

    ResultCode BufferPool::UpdateBufferDeviceMask(const BufferDeviceMaskRequest& request)
    {
        AZ_PROFILE_FUNCTION(RHI);

        return IterateObjects<DeviceBufferPool>(
            [&request](auto deviceIndex, auto deviceBufferPool)
            {
                if (CheckBit(request.m_deviceMask, deviceIndex))
                {
                    if (!request.m_buffer->m_deviceObjects.contains(deviceIndex))
                    {
                        request.m_buffer->m_deviceObjects[deviceIndex] = Factory::Get().CreateBuffer();

                        DeviceBufferInitRequest bufferInitRequest(
                            *request.m_buffer->GetDeviceBuffer(deviceIndex), request.m_buffer->m_descriptor, request.m_initialData);
                        auto result = deviceBufferPool->InitBuffer(bufferInitRequest);

                        if (result == ResultCode::Success)
                        {
                            request.m_buffer->Init(SetBit(request.m_buffer->GetDeviceMask(), deviceIndex));
                        }

                        return result;
                    }
                }
                else
                {
                    request.m_buffer->Init(ResetBit(request.m_buffer->GetDeviceMask(), deviceIndex));
                    request.m_buffer->m_deviceObjects.erase(deviceIndex);
                }

                return ResultCode::Success;
            });
    }

    ResultCode BufferPool::OrphanBuffer(Buffer& buffer)
    {
        if (!ValidateIsInitialized() || !ValidateIsHostHeap())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateIsRegistered(&buffer))
        {
            return ResultCode::InvalidArgument;
        }

        AZ_PROFILE_SCOPE(RHI, "BufferPool::OrphanBuffer");

        buffer.Invalidate();
        buffer.Init(MultiDevice::NoDevices);

        return ResultCode::Success;
    }

    ResultCode BufferPool::MapBuffer(const BufferMapRequest& request, BufferMapResponse& response)
    {
        AZ_PROFILE_FUNCTION(RHI);

        if (!ValidateIsInitialized())
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

        DeviceBufferMapRequest deviceMapRequest{};
        deviceMapRequest.m_byteCount = request.m_byteCount;
        deviceMapRequest.m_byteOffset = request.m_byteOffset;

        DeviceBufferMapResponse deviceMapResponse{};

        ResultCode resultCode = IterateObjects<DeviceBufferPool>([&](auto deviceIndex, auto deviceBufferPool)
        {
            deviceMapRequest.m_buffer = request.m_buffer->GetDeviceBuffer(deviceIndex).get();
            auto resultCode = deviceBufferPool->MapBuffer(deviceMapRequest, deviceMapResponse);

            if (resultCode != ResultCode::Success)
            {
                AZ_Error(
                    "BufferPool",
                    false,
                    "Unable to map buffer '%s'.",
                    request.m_buffer->GetName().GetCStr());
            }
            else
            {
                response.m_data[deviceIndex] = deviceMapResponse.m_data;
            }

            return resultCode;
        });

        ValidateBufferMap(*request.m_buffer, !response.m_data.empty());
        return resultCode;
    }

    void BufferPool::UnmapBuffer(Buffer& buffer)
    {
        if (ValidateIsInitialized() && ValidateIsRegistered(&buffer))
        {
            IterateObjects<DeviceBufferPool>([&buffer](auto deviceIndex, auto deviceBufferPool)
            {
                deviceBufferPool->UnmapBuffer(*buffer.GetDeviceBuffer(deviceIndex));
            });
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

        return IterateObjects<DeviceBufferPool>([&request](auto deviceIndex, auto deviceBufferPool)
        {
            auto* buffer = request.m_buffer->GetDeviceBuffer(deviceIndex).get();

            DeviceBufferStreamRequest bufferStreamRequest{ request.m_fenceToSignal ? request.m_fenceToSignal->GetDeviceFence(deviceIndex).get()
                                                                             : nullptr,
                                                     buffer,
                                                     request.m_byteOffset,
                                                     request.m_byteCount,
                                                     request.m_sourceData };

            return deviceBufferPool->StreamBuffer(bufferStreamRequest);
        });
    }

    const BufferPoolDescriptor& BufferPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void BufferPool::ValidateBufferMap([[maybe_unused]] Buffer& buffer, bool isDataValid)
    {
        if (Validation::IsEnabled())
        {
            // No need for validation for a null rhi
            if (!isDataValid)
            {
                AZ_Error("BufferPool", false, "Failed to map buffer '%s'.", buffer.GetName().GetCStr());
            }
        }
    }

    void BufferPool::Shutdown()
    {
        ResourcePool::Shutdown();
    }
} // namespace AZ::RHI
