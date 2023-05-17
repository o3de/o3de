/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceRayTracingBufferPools.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        const RHI::Ptr<RHI::MultiDeviceBufferPool>& MultiDeviceRayTracingBufferPools::GetShaderTableBufferPool() const
        {
            AZ_Assert(m_initialized, "MultiDeviceRayTracingBufferPools was not initialized");
            return m_shaderTableBufferPool;
        }

        const RHI::Ptr<RHI::MultiDeviceBufferPool>& MultiDeviceRayTracingBufferPools::GetScratchBufferPool() const
        {
            AZ_Assert(m_initialized, "MultiDeviceRayTracingBufferPools was not initialized");
            return m_scratchBufferPool;
        }

        const RHI::Ptr<RHI::MultiDeviceBufferPool>& MultiDeviceRayTracingBufferPools::GetBlasBufferPool() const
        {
            AZ_Assert(m_initialized, "MultiDeviceRayTracingBufferPools was not initialized");
            return m_blasBufferPool;
        }

        const RHI::Ptr<RHI::MultiDeviceBufferPool>& MultiDeviceRayTracingBufferPools::GetTlasInstancesBufferPool() const
        {
            AZ_Assert(m_initialized, "MultiDeviceRayTracingBufferPools was not initialized");
            return m_tlasInstancesBufferPool;
        }

        const RHI::Ptr<RHI::MultiDeviceBufferPool>& MultiDeviceRayTracingBufferPools::GetTlasBufferPool() const
        {
            AZ_Assert(m_initialized, "MultiDeviceRayTracingBufferPools was not initialized");
            return m_tlasBufferPool;
        }

        void MultiDeviceRayTracingBufferPools::Init(MultiDevice::DeviceMask deviceMask)
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

            m_shaderTableBufferPool = aznew RHI::MultiDeviceBufferPool();
            m_shaderTableBufferPool->Init(deviceMask, [this](){
                for (auto& [deviceIndex, pool] : m_deviceRayTracingBufferPools)
                {
                    m_shaderTableBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetShaderTableBufferPool().get();
                    m_shaderTableBufferPool->m_descriptor = pool->GetShaderTableBufferPool()->GetDescriptor();
                }
                return ResultCode::Success;
            });

            m_scratchBufferPool = aznew RHI::MultiDeviceBufferPool();
            m_scratchBufferPool->Init(deviceMask, [this](){
                for (auto& [deviceIndex, pool] : m_deviceRayTracingBufferPools)
                {
                    m_scratchBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetScratchBufferPool().get();
                    m_scratchBufferPool->m_descriptor = pool->GetScratchBufferPool()->GetDescriptor();
                }
                return ResultCode::Success;
            });

            m_blasBufferPool = aznew RHI::MultiDeviceBufferPool();
            m_blasBufferPool->Init(deviceMask, [this](){
                for (auto& [deviceIndex, pool] : m_deviceRayTracingBufferPools)
                {
                    m_blasBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetBlasBufferPool().get();
                    m_blasBufferPool->m_descriptor = pool->GetBlasBufferPool()->GetDescriptor();
                }
                return ResultCode::Success;
            });

            m_tlasInstancesBufferPool = aznew RHI::MultiDeviceBufferPool();
            m_tlasInstancesBufferPool->Init(deviceMask, [this](){
                for (auto& [deviceIndex, pool] : m_deviceRayTracingBufferPools)
                {
                    m_tlasInstancesBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetTlasInstancesBufferPool().get();
                    m_tlasInstancesBufferPool->m_descriptor = pool->GetTlasInstancesBufferPool()->GetDescriptor();
                }
                return ResultCode::Success;
            });

            m_tlasBufferPool = aznew RHI::MultiDeviceBufferPool();
            m_tlasBufferPool->Init(deviceMask, [this](){
                for (auto& [deviceIndex, pool] : m_deviceRayTracingBufferPools)
                {
                    m_tlasBufferPool->m_deviceBufferPools[deviceIndex] = pool->GetTlasBufferPool().get();
                    m_tlasBufferPool->m_descriptor = pool->GetTlasBufferPool()->GetDescriptor();
                }
                return ResultCode::Success;
            });

            m_initialized = true;
        }
    } // namespace RHI
} // namespace AZ
