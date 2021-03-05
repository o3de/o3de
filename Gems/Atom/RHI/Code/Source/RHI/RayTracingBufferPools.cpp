/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Device.h>

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

        void RayTracingBufferPools::Init(RHI::Ptr<RHI::Device>& device)
        {
            if (m_initialized)
            {
                return;
            }

            // create shader table buffer pool
            {
                RHI::BufferPoolDescriptor bufferPoolDesc;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::ShaderRead;

                m_shaderTableBufferPool = RHI::Factory::Get().CreateBufferPool();
                m_shaderTableBufferPool->SetName(Name("RayTracingShaderTableBufferPool"));
                RHI::ResultCode resultCode = m_shaderTableBufferPool->Init(*device, bufferPoolDesc);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing shader table buffer pool");
            }

            // create scratch buffer pool
            {
                RHI::BufferPoolDescriptor bufferPoolDesc;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;

                m_scratchBufferPool = RHI::Factory::Get().CreateBufferPool();
                m_scratchBufferPool->SetName(Name("RayTracingScratchBufferPool"));
                RHI::ResultCode resultCode = m_scratchBufferPool->Init(*device, bufferPoolDesc);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing scratch buffer pool");
            }

            // create BLAS buffer pool
            {
                RHI::BufferPoolDescriptor bufferPoolDesc;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;

                m_blasBufferPool = RHI::Factory::Get().CreateBufferPool();
                m_blasBufferPool->SetName(Name("RayTracingBlasBufferPool"));
                RHI::ResultCode resultCode = m_blasBufferPool->Init(*device, bufferPoolDesc);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing BLAS buffer pool");
            }

            // create TLAS Instances buffer pool
            {
                RHI::BufferPoolDescriptor bufferPoolDesc;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::ShaderRead;

                m_tlasInstancesBufferPool = RHI::Factory::Get().CreateBufferPool();
                m_tlasInstancesBufferPool->SetName(Name("RayTracingTlasInstancesBufferPool"));
                RHI::ResultCode resultCode = m_tlasInstancesBufferPool->Init(*device, bufferPoolDesc);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing TLAS instances buffer pool");
            }

            // create TLAS buffer pool
            {
                RHI::BufferPoolDescriptor bufferPoolDesc;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::RayTracingAccelerationStructure;

                m_tlasBufferPool = RHI::Factory::Get().CreateBufferPool();
                m_tlasBufferPool->SetName(Name("RayTracingTLASBufferPool"));
                RHI::ResultCode resultCode = m_tlasBufferPool->Init(*device, bufferPoolDesc);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to initialize ray tracing TLAS buffer pool");
            }

            m_initialized = true;
        }
    }
}
