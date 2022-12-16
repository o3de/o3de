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

namespace AZ
{
    namespace RHI
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

        const RHI::Ptr<RHI::BufferPool>& RayTracingBufferPools::GetBlasBufferPool() const
        {
            AZ_Assert(m_initialized, "RayTracingBufferPools was not initialized");
            return m_blasBufferPool;
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

        void RayTracingBufferPools::Init(DeviceMask deviceMask)
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
                    m_deviceRayTracingBufferPools[deviceIndex] = Factory::Get().CreateRayTracingBufferPools();
                    m_deviceRayTracingBufferPools[deviceIndex]->Init(device);
                    return true;
                });

            for (auto& [deviceIndex, pool] : m_deviceRayTracingBufferPools)
            {
                auto platformInitMethod = []()
                {
                    return ResultCode::Success;
                };

                m_shaderTableBufferPool = aznew RHI::BufferPool();
                m_shaderTableBufferPool->Init(deviceMask, pool->GetShaderTableBufferPool()->GetDescriptor(), platformInitMethod);
                m_shaderTableBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetShaderTableBufferPool().get();

                m_scratchBufferPool = aznew RHI::BufferPool();
                m_scratchBufferPool->Init(deviceMask, pool->GetScratchBufferPool()->GetDescriptor(), platformInitMethod);
                m_scratchBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetScratchBufferPool().get();

                m_blasBufferPool = aznew RHI::BufferPool();
                m_blasBufferPool->Init(deviceMask, pool->GetBlasBufferPool()->GetDescriptor(), platformInitMethod);
                m_blasBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetBlasBufferPool().get();

                m_tlasInstancesBufferPool = aznew RHI::BufferPool();
                m_tlasInstancesBufferPool->Init(deviceMask, pool->GetTlasInstancesBufferPool()->GetDescriptor(), platformInitMethod);
                m_tlasInstancesBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetTlasInstancesBufferPool().get();

                m_tlasBufferPool = aznew RHI::BufferPool();
                m_tlasBufferPool->Init(deviceMask, pool->GetTlasBufferPool()->GetDescriptor(), platformInitMethod);
                m_tlasBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetTlasBufferPool().get();
            }

            m_initialized = true;
        }
    } // namespace RHI
} // namespace AZ
