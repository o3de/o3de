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
        return m_ShaderTableBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetScratchBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_ScratchBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetBlasBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_BlasBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetTlasInstancesBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_TlasInstancesBufferPool;
    }

    const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetTlasBufferPool() const
    {
        AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
        return m_TlasBufferPool;
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

        m_ShaderTableBufferPool = aznew RHI::BufferPool();
        m_ShaderTableBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_ShaderTableBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetShaderTableBufferPool().get();
                        m_ShaderTableBufferPool->m_descriptor = deviceBufferPool->GetShaderTableBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_ScratchBufferPool = aznew RHI::BufferPool();
        m_ScratchBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_ScratchBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetScratchBufferPool().get();
                        m_ScratchBufferPool->m_descriptor = deviceBufferPool->GetScratchBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_BlasBufferPool = aznew RHI::BufferPool();
        m_BlasBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_BlasBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetBlasBufferPool().get();
                        m_BlasBufferPool->m_descriptor = deviceBufferPool->GetBlasBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_TlasInstancesBufferPool = aznew RHI::BufferPool();
        m_TlasInstancesBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_TlasInstancesBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetTlasInstancesBufferPool().get();
                        m_TlasInstancesBufferPool->m_descriptor = deviceBufferPool->GetTlasInstancesBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_TlasBufferPool = aznew RHI::BufferPool();
        m_TlasBufferPool->Init(
            deviceMask,
            [this]()
            {
                IterateObjects<DeviceRayTracingBufferPools>(
                    [this]([[maybe_unused]] auto deviceIndex, auto deviceBufferPool)
                    {
                        m_TlasBufferPool->m_deviceObjects[deviceIndex] = deviceBufferPool->GetTlasBufferPool().get();
                        m_TlasBufferPool->m_descriptor = deviceBufferPool->GetTlasBufferPool()->GetDescriptor();
                    });
                return ResultCode::Success;
            });

        m_initialized = true;
    }
} // namespace AZ::RHI
