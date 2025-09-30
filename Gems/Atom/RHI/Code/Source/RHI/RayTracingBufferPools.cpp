/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetShaderTableBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_shaderTableBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetScratchBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_scratchBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetAabbStagingBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_aabbStagingBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetBlasBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_blasBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetDstImplicitBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_dstImplicitBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetDstAddressesArrayBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_dstAddressesArrayBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetDstSizesArrayBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_dstSizesArrayBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetSrcInfosArrayBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_srcInfosArrayBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetSrcInfosCountBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_srcInfosCountBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetTlasInstancesBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_tlasInstancesBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetTlasBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_tlasBufferPool;
    }

    void RayTracingBufferPools::Init(MultiDevice::DeviceMask deviceMask)
    {
        if (m_initialized)
        {
            return;
        }

        MultiDeviceObject::Init(deviceMask);

        // Initialize device ray tracing buffer pools
        IterateDevices(
            [this](int deviceIndex)
            {
                RHI::Ptr<RHI::Device> device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                m_deviceObjects[deviceIndex] = Factory::Get().CreateRayTracingBufferPools();

                GetDeviceRayTracingBufferPools(deviceIndex)->Init(device);
                return true;
            });

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }

        m_shaderTableBufferPool = aznew RHI::BufferPool();
        m_shaderTableBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_shaderTableBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetShaderTableBufferPool().get();
                        m_shaderTableBufferPool->m_descriptor = deviceBufferPool->GetShaderTableBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_scratchBufferPool = aznew RHI::BufferPool();
        m_scratchBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_scratchBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetScratchBufferPool().get();
                        m_scratchBufferPool->m_descriptor = deviceBufferPool->GetScratchBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_aabbStagingBufferPool = aznew RHI::BufferPool();
        m_aabbStagingBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_aabbStagingBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetAabbStagingBufferPool().get();
                        m_aabbStagingBufferPool->m_descriptor = deviceBufferPool->GetAabbStagingBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_blasBufferPool = aznew RHI::BufferPool();
        m_blasBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_blasBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetBlasBufferPool().get();
                        m_blasBufferPool->m_descriptor = deviceBufferPool->GetBlasBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_dstImplicitBufferPool = aznew RHI::BufferPool();
        m_dstImplicitBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_dstImplicitBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetDstImplicitBufferPool().get();
                        m_dstImplicitBufferPool->m_descriptor = deviceBufferPool->GetDstImplicitBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_dstAddressesArrayBufferPool = aznew RHI::BufferPool();
        m_dstAddressesArrayBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_dstAddressesArrayBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetDstAddressesArrayBufferPool().get();
                        m_dstAddressesArrayBufferPool->m_descriptor = deviceBufferPool->GetDstAddressesArrayBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_dstSizesArrayBufferPool = aznew RHI::BufferPool();
        m_dstSizesArrayBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_dstSizesArrayBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetDstSizesArrayBufferPool().get();
                        m_dstSizesArrayBufferPool->m_descriptor = deviceBufferPool->GetDstSizesArrayBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_srcInfosArrayBufferPool = aznew RHI::BufferPool();
        m_srcInfosArrayBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_srcInfosArrayBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetSrcInfosArrayBufferPool().get();
                        m_srcInfosArrayBufferPool->m_descriptor = deviceBufferPool->GetSrcInfosArrayBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_srcInfosCountBufferPool = aznew RHI::BufferPool();
        m_srcInfosCountBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_srcInfosCountBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetSrcInfosCountBufferPool().get();
                        m_srcInfosCountBufferPool->m_descriptor = deviceBufferPool->GetSrcInfosCountBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_tlasInstancesBufferPool = aznew RHI::BufferPool();
        m_tlasInstancesBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_tlasInstancesBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetTlasInstancesBufferPool().get();
                        m_tlasInstancesBufferPool->m_descriptor = deviceBufferPool->GetTlasInstancesBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_tlasBufferPool = aznew RHI::BufferPool();
        m_tlasBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_tlasBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetTlasBufferPool().get();
                        m_tlasBufferPool->m_descriptor = deviceBufferPool->GetTlasBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_initialized = true;
    }
} // namespace AZ::RHI
