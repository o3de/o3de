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

namespace AZ
{
    namespace RHI
    {
        MultiDeviceBufferInitRequest::MultiDeviceBufferInitRequest(
            MultiDeviceBuffer& buffer, const BufferDescriptor& descriptor, const void* initialData)
            : m_buffer{ &buffer }
            , m_descriptor{ descriptor }
            , m_initialData{ initialData }
        {
        }

        MultiDeviceBufferMapRequest::MultiDeviceBufferMapRequest(MultiDeviceBuffer& buffer, size_t byteOffset, size_t byteCount)
            : m_buffer{ &buffer }
            , m_byteOffset{ byteOffset }
            , m_byteCount{ byteCount }
        {
        }

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
                        "MultiDeviceBufferPool",
                        false,
                        "Trying to map zero bytes from buffer '%s'.",
                        request.m_buffer->GetName().GetCStr());
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

                    /**
                     * Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                     * for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                     * possibility that users will get garbage values from GetDescriptor().
                     */
                    m_descriptor = descriptor;

                    ResultCode result = ResultCode::Success;

                    IterateDevices(
                        [this, &descriptor, &result](int deviceIndex)
                        {
                            auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                            m_deviceBufferPools[deviceIndex] = Factory::Get().CreateBufferPool();
                            result = m_deviceBufferPools[deviceIndex]->Init(*device, descriptor);

                            return result == ResultCode::Success;
                        });

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
                    ResultCode result = ResultCode::Success;

                    for (auto& [deviceIndex, deviceBufferPool] : m_deviceBufferPools)
                    {
                        if (!initRequest.m_buffer->m_deviceBuffers.contains(deviceIndex))
                            initRequest.m_buffer->m_deviceBuffers[deviceIndex] = Factory::Get().CreateBuffer();
                        BufferInitRequest bufferInitRequest(
                            *initRequest.m_buffer->m_deviceBuffers[deviceIndex], initRequest.m_descriptor, initRequest.m_initialData);
                        result = deviceBufferPool->InitBuffer(bufferInitRequest);

                        if (result != ResultCode::Success)
                            break;
                    }

                    return result;
                });

            if (resultCode == ResultCode::Success && initRequest.m_initialData)
            {
                MultiDeviceBufferMapRequest mapRequest;
                mapRequest.m_buffer = initRequest.m_buffer;
                mapRequest.m_byteCount = initRequest.m_descriptor.m_byteCount;
                mapRequest.m_byteOffset = 0;

                MultiDeviceBufferMapResponse mapResponse;
                resultCode = MapBuffer(mapRequest, mapResponse);
                if (resultCode == ResultCode::Success)
                {
                    for (auto data : mapResponse.m_data)
                    {
                        if (data)
                        {
                            memcpy(data, initRequest.m_initialData, initRequest.m_descriptor.m_byteCount);
                        }
                    }

                    UnmapBuffer(*initRequest.m_buffer);
                }
            }

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

            ResultCode resultCode = ResultCode::Success;

            BufferMapRequest deviceMapRequest{};
            deviceMapRequest.m_byteCount = request.m_byteCount;
            deviceMapRequest.m_byteOffset = request.m_byteOffset;

            BufferMapResponse deviceMapResponse{};

            for (auto& [deviceIndex, deviceBufferPool] : m_deviceBufferPools)
            {
                deviceMapRequest.m_buffer = request.m_buffer->m_deviceBuffers[deviceIndex].get();
                resultCode = deviceBufferPool->MapBuffer(deviceMapRequest, deviceMapResponse);

                if (resultCode != ResultCode::Success)
                    break;

                response.m_data.push_back(deviceMapResponse.m_data);
            }

            ValidateBufferMap(*request.m_buffer, !response.m_data.empty());
            return resultCode;
        }

        void MultiDeviceBufferPool::UnmapBuffer(MultiDeviceBuffer& buffer)
        {
            if (ValidateIsInitialized() && ValidateNotDeviceLevel() && ValidateIsRegistered(&buffer))
            {
                for (auto& [deviceIndex, deviceBufferPool] : m_deviceBufferPools)
                {
                    deviceBufferPool->UnmapBuffer(*buffer.m_deviceBuffers[deviceIndex]);
                }
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

            ResultCode result = ResultCode::Success;

            for (auto& [deviceIndex, deviceBufferPool] : m_deviceBufferPools)
            {
                auto* buffer = request.m_buffer->m_deviceBuffers[deviceIndex].get();

                BufferStreamRequest bufferStreamRequest{ request.m_fenceToSignal
                                                                         ? request.m_fenceToSignal->GetDeviceFence(deviceIndex).get()
                                                                         : nullptr,
                                                                     buffer,
                                                                     request.m_byteOffset,
                                                                     request.m_byteCount,
                                                                     request.m_sourceData };

                result = deviceBufferPool->StreamBuffer(bufferStreamRequest);

                if (result != ResultCode::Success)
                    break;
            }

            return result;
        }

        const BufferPoolDescriptor& MultiDeviceBufferPool::GetDescriptor() const
        {
            return m_descriptor;
        }

        ResultCode MultiDeviceBufferPool::InitBuffer(
            MultiDeviceBuffer* buffer, const BufferDescriptor& descriptor, PlatformMethod platformInitResourceMethod)
        {
            // The descriptor is assigned regardless of whether initialization succeeds. Descriptors are considered
            // undefined for uninitialized resources. This makes the buffer descriptor well defined at initialization
            // time rather than leftover data from the previous usage.
            buffer->SetDescriptor(descriptor);

            return MultiDeviceResourcePool::InitResource(buffer, platformInitResourceMethod);
        }

        void MultiDeviceBufferPool::ValidateBufferMap(MultiDeviceBuffer& buffer, bool isDataValid)
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
            for (auto [_, pool] : m_deviceBufferPools)
                pool->Shutdown();
            MultiDeviceResourcePool::Shutdown();
        }
    } // namespace RHI
} // namespace AZ
