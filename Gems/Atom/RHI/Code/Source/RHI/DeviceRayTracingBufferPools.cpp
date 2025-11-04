/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Device.h>

namespace AZ::RHI
{
    RHI::Ptr<RHI::DeviceRayTracingBufferPools> DeviceRayTracingBufferPools::CreateRHIRayTracingBufferPools()
    {
        RHI::Ptr<RHI::DeviceRayTracingBufferPools> rayTracingBufferPools = RHI::Factory::Get().CreateRayTracingBufferPools();
        AZ_Error("DeviceRayTracingBufferPools", rayTracingBufferPools.get(), "Failed to create RHI::DeviceRayTracingBufferPools");
        return rayTracingBufferPools;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetShaderTableBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_shaderTableBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetScratchBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_scratchBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetAabbStagingBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_aabbStagingBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetBlasBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_blasBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetDstImplicitBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_dstImplicitBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetDstAddressesArrayBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_dstAddressesArrayBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetDstSizesArrayBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_dstSizesArrayBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetSrcInfosArrayBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_srcInfosArrayBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetSrcInfosCountBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_srcInfosCountBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetTlasInstancesBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_tlasInstancesBufferPool;
    }

    const RHI::Ptr<RHI::DeviceBufferPool>& DeviceRayTracingBufferPools::GetTlasBufferPool() const
    {
        AZ_Assert(m_initialized, "DeviceRayTracingBufferPools was not initialized");
        return m_tlasBufferPool;
    }

    void DeviceRayTracingBufferPools::Init(RHI::Ptr<RHI::Device>& device)
    {
        if (m_initialized)
        {
            return;
        }

        // create shader table buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
            bufferPoolDesc.m_bindFlags = GetShaderTableBufferBindFlags();

            m_shaderTableBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_shaderTableBufferPool->SetName(Name("RayTracingShaderTableBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_shaderTableBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing shader table buffer pool");
        }

        // create scratch buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetScratchBufferBindFlags();

            m_scratchBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_scratchBufferPool->SetName(Name("RayTracingScratchBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_scratchBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing scratch buffer pool");
        }

        // create AABB buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetAabbStagingBufferBindFlags();

            m_aabbStagingBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_aabbStagingBufferPool->SetName(Name("RayTracingAabbStagingBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_aabbStagingBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing AABB staging buffer pool");
        }

        // create BLAS buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetBlasBufferBindFlags();

            m_blasBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_blasBufferPool->SetName(Name("RayTracingBlasBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_blasBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing BLAS buffer pool");
        }

        // create destination implicit buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetDstImplicitBufferBindFlags();

            m_dstImplicitBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_dstImplicitBufferPool->SetName(Name("RayTracingDstImplicitBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_dstImplicitBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing destination implicit buffer pool");
        }

        // create destination addresses array buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetDstAddressesArrayBufferBindFlags();

            m_dstAddressesArrayBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_dstAddressesArrayBufferPool->SetName(Name("RayTracingDstAddressesArrayBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_dstAddressesArrayBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing destination addresses array buffer pool");
        }

        // create destination sizes array buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetDstSizesArrayBufferBindFlags();

            m_dstSizesArrayBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_dstSizesArrayBufferPool->SetName(Name("RayTracingDstSizesArrayBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_dstSizesArrayBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing destination sizes array buffer pool");
        }

        // create source infos array buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetSrcInfosArrayBufferBindFlags();

            m_srcInfosArrayBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_srcInfosArrayBufferPool->SetName(Name("RayTracingSrcInfosArrayBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_srcInfosArrayBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing source infos array buffer pool");
        }

        // create source infos count buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetSrcInfosCountBufferBindFlags();

            m_srcInfosCountBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_srcInfosCountBufferPool->SetName(Name("RayTracingSrcInfosCountBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_srcInfosCountBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing source infos count buffer pool");
        }

        // create TLAS Instances buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetTlasInstancesBufferBindFlags();

            m_tlasInstancesBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_tlasInstancesBufferPool->SetName(Name("RayTracingTlasInstancesBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_tlasInstancesBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing TLAS instances buffer pool");
        }

        // create TLAS buffer pool
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_bindFlags = GetTlasBufferBindFlags();

            m_tlasBufferPool = RHI::Factory::Get().CreateBufferPool();
            m_tlasBufferPool->SetName(Name("RayTracingTLASBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_tlasBufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing TLAS buffer pool");
        }

        m_initialized = true;
    }
}
