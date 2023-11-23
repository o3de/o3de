/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceBufferPool.h>
#include <Atom/RHI/MultiDeviceFence.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    bool MultiDeviceBufferPool::ValidatePoolDescriptor(const BufferPoolDescriptor& descriptor) const
    {
        if (Validation::IsEnabled())
        {
            if (descriptor.m_heapMemoryLevel == RHI::HeapMemoryLevel::Device &&
                descriptor.m_hostMemoryAccess == RHI::HostMemoryAccess::Read)
            {
                AZ_Error(
                    "MultiDeviceBufferPool",
                    false,
                    "When HeapMemoryLevel::Device is specified, m_hostMemoryAccess must be HostMemoryAccess::Write.");
                return false;
            }
        }
        return true;
    }

    bool MultiDeviceBufferPool::ValidateInitRequest(const MultiDeviceBufferInitRequest& initRequest) const
    {
        if (Validation::IsEnabled())
        {
            const BufferPoolDescriptor& poolDescriptor = GetDescriptor();

            // Bind flags of the buffer must match the pool bind flags.
            if (initRequest.m_descriptor.m_bindFlags != poolDescriptor.m_bindFlags)
            {
                AZ_Error(
                    "MultiDeviceBufferPool",
                    false,
                    "MultiDeviceBuffer bind flags don't match pool bind flags in pool '%s'",
                    GetName().GetCStr());
                return false;
            }

            // Initial data is not allowed for read-only heaps.
            if (initRequest.m_initialData && poolDescriptor.m_hostMemoryAccess == HostMemoryAccess::Read)
            {
                AZ_Error("MultiDeviceBufferPool", false, "Initial data is not allowed with read-only pools.");
                return false;
            }
        }
        return true;
    }

    bool MultiDeviceBufferPool::ValidateIsHostHeap() const
    {
        if (Validation::IsEnabled())
        {
            if (GetDescriptor().m_heapMemoryLevel != HeapMemoryLevel::Host)
            {
                AZ_Error("MultiDeviceBufferPool", false, "This operation is only permitted for pools on the Host heap.");
                return false;
            }
        }
        return true;
    }

    bool MultiDeviceBufferPool::ValidateMapRequest(const MultiDeviceBufferMapRequest& request) const
    {
        if (Validation::IsEnabled())
        {
            if (!request.m_buffer)
            {
                AZ_Error("MultiDeviceBufferPool", false, "Trying to map a null buffer.");
                return false;
            }

            if (request.m_byteCount == 0)
            {
                AZ_Warning(
                    "MultiDeviceBufferPool", false, "Trying to map zero bytes from buffer '%s'.", request.m_buffer->GetName().GetCStr());
                return false;
            }

            if (request.m_byteOffset + request.m_byteCount > request.m_buffer->GetDescriptor().m_byteCount)
            {
                AZ_Error(
                    "MultiDeviceBufferPool",
                    false,
                    "Unable to map buffer '%s', overrunning the size of the buffer.",
                    request.m_buffer->GetName().GetCStr());
                return false;
            }
        }
        return true;
    }

    ResultCode MultiDeviceBufferPool::Init(MultiDevice::DeviceMask deviceMask, const BufferPoolDescriptor& descriptor)
    {
        return MultiDeviceResourcePool::Init(
            deviceMask,
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

    ResultCode MultiDeviceBufferPool::InitBuffer(const MultiDeviceBufferInitRequest& initRequest)
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
                return IterateObjects<SingleDeviceBufferPool>([&initRequest](auto deviceIndex, auto deviceBufferPool)
                {
                    if (!initRequest.m_buffer->m_deviceObjects.contains(deviceIndex))
                    {
                        initRequest.m_buffer->m_deviceObjects[deviceIndex] = Factory::Get().CreateBuffer();
                    }

                    SingleDeviceBufferInitRequest bufferInitRequest(
                        *initRequest.m_buffer->GetDeviceBuffer(deviceIndex), initRequest.m_descriptor, initRequest.m_initialData);
                    return deviceBufferPool->InitBuffer(bufferInitRequest);
                });
            });

        return resultCode;
    }

    ResultCode MultiDeviceBufferPool::OrphanBuffer(MultiDeviceBuffer& buffer)
    {
        if (!ValidateIsInitialized() || !ValidateIsHostHeap() || !ValidateNotDeviceLevel())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateIsRegistered(&buffer))
        {
            return ResultCode::InvalidArgument;
        }

        AZ_PROFILE_SCOPE(RHI, "MultiDeviceBufferPool::OrphanBuffer");

        buffer.Invalidate();
        buffer.Init(GetDeviceMask());

        return ResultCode::Success;
    }

    ResultCode MultiDeviceBufferPool::MapBuffer(const MultiDeviceBufferMapRequest& request, MultiDeviceBufferMapResponse& response)
    {
        AZ_PROFILE_FUNCTION(RHI);

        if (!ValidateIsInitialized() || !ValidateNotDeviceLevel())
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

        SingleDeviceBufferMapRequest deviceMapRequest{};
        deviceMapRequest.m_byteCount = request.m_byteCount;
        deviceMapRequest.m_byteOffset = request.m_byteOffset;

        SingleDeviceBufferMapResponse deviceMapResponse{};

        ResultCode resultCode = IterateObjects<SingleDeviceBufferPool>([&](auto deviceIndex, auto deviceBufferPool)
        {
            deviceMapRequest.m_buffer = request.m_buffer->GetDeviceBuffer(deviceIndex).get();
            auto resultCode = deviceBufferPool->MapBuffer(deviceMapRequest, deviceMapResponse);

            if (resultCode != ResultCode::Success)
            {
                AZ_Error(
                    "MultiDeviceBufferPool",
                    false,
                    "Unable to map buffer '%s'.",
                    request.m_buffer->GetName().GetCStr());
            }
            else
            {
                response.m_data.push_back(deviceMapResponse.m_data);
            }

            return resultCode;
        });

        ValidateBufferMap(*request.m_buffer, !response.m_data.empty());
        return resultCode;
    }

    void MultiDeviceBufferPool::UnmapBuffer(MultiDeviceBuffer& buffer)
    {
        if (ValidateIsInitialized() && ValidateNotDeviceLevel() && ValidateIsRegistered(&buffer))
        {
            IterateObjects<SingleDeviceBufferPool>([&buffer](auto deviceIndex, auto deviceBufferPool)
            {
                deviceBufferPool->UnmapBuffer(*buffer.GetDeviceBuffer(deviceIndex));
            });
        }
    }

    ResultCode MultiDeviceBufferPool::StreamBuffer(const MultiDeviceBufferStreamRequest& request)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateIsRegistered(request.m_buffer))
        {
            return ResultCode::InvalidArgument;
        }

        return IterateObjects<SingleDeviceBufferPool>([&request](auto deviceIndex, auto deviceBufferPool)
        {
            auto* buffer = request.m_buffer->GetDeviceBuffer(deviceIndex).get();

            SingleDeviceBufferStreamRequest bufferStreamRequest{ request.m_fenceToSignal ? request.m_fenceToSignal->GetDeviceFence(deviceIndex).get()
                                                                             : nullptr,
                                                     buffer,
                                                     request.m_byteOffset,
                                                     request.m_byteCount,
                                                     request.m_sourceData };

            return deviceBufferPool->StreamBuffer(bufferStreamRequest);
        });
    }

    const BufferPoolDescriptor& MultiDeviceBufferPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void MultiDeviceBufferPool::ValidateBufferMap([[maybe_unused]] MultiDeviceBuffer& buffer, bool isDataValid)
    {
        if (Validation::IsEnabled())
        {
            // No need for validation for a null rhi
            if (!isDataValid)
            {
                AZ_Error("MultiDeviceBufferPool", false, "Failed to map buffer '%s'.", buffer.GetName().GetCStr());
            }
        }
    }

    bool MultiDeviceBufferPool::ValidateNotDeviceLevel() const
    {
        return GetDescriptor().m_heapMemoryLevel != HeapMemoryLevel::Device;
    }

    void MultiDeviceBufferPool::Shutdown()
    {
        IterateObjects<SingleDeviceBufferPool>([]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
        {
            deviceBufferPool->Shutdown();
        });
        MultiDeviceResourcePool::Shutdown();
    }
} // namespace AZ::RHI
