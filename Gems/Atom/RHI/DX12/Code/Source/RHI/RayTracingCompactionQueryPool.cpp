/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/Factory.h>
#include <RHI/RayTracingCompactionQueryPool.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<RayTracingCompactionQuery> RayTracingCompactionQuery::Create()
        {
            return aznew RayTracingCompactionQuery;
        }

        int RayTracingCompactionQuery::Allocate()
        {
            AZ_Assert(!m_indexInPool, "RayTracingCompactionQuery::Allocate: Trying to allocate a query twice");
            auto dx12Pool = static_cast<RayTracingCompactionQueryPool*>(m_pool);
            m_indexInPool = dx12Pool->Allocate(this);
            return m_indexInPool.value();
        }

        void RayTracingCompactionQuery::SetResult(uint64_t value)
        {
            m_result = value;
        }

        uint64_t RayTracingCompactionQuery::GetResult()
        {
            AZ_Assert(m_result, "RayTracingCompactionQuery::GetResult: Result not ready");
            return m_result.value();
        }

        RHI::ResultCode RayTracingCompactionQuery::InitInternal(RHI::DeviceRayTracingCompactionQueryPool* pool)
        {
            m_pool = pool;
            return RHI::ResultCode::Success;
        }

        RHI::Ptr<RayTracingCompactionQueryPool> RayTracingCompactionQueryPool::Create()
        {
            return aznew RayTracingCompactionQueryPool;
        }

        int RayTracingCompactionQueryPool::Allocate(RayTracingCompactionQuery* query)
        {
            auto& buffers = m_queryPoolBuffers[m_currentBufferIndex];
            AZ_Assert(buffers.m_nextFreeIndex < m_desc.m_budget, "RayTracingCompactionQueryPool: Pool is full");
            buffers.enqueuedQueries.emplace_back(query, buffers.m_nextFreeIndex);
            buffers.m_nextFreeIndex++;
            return buffers.enqueuedQueries.back().second;
        }

        RHI::DeviceBuffer* RayTracingCompactionQueryPool::GetCurrentGPUBuffer()
        {
            return m_queryPoolBuffers[m_currentBufferIndex].m_gpuBuffers.get();
        }

        RHI::DeviceBuffer* RayTracingCompactionQueryPool::GetCurrentCPUBuffer()
        {
            return m_queryPoolBuffers[m_currentBufferIndex].m_cpuBuffers.get();
        }

        RHI::ResultCode RayTracingCompactionQueryPool::InitInternal(RHI::RayTracingCompactionQueryPoolDescriptor desc)
        {
            for (auto& buffers : m_queryPoolBuffers)
            {
                {
                    buffers.m_cpuBuffers = RHI::Factory::Get().CreateBuffer();

                    RHI::BufferDescriptor bufferDesc;
                    bufferDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
                    bufferDesc.m_byteCount = desc.m_budget * sizeof(uint64_t);
                    bufferDesc.m_bindFlags = RHI::BufferBindFlags::CopyWrite;
                    bufferDesc.m_alignment = sizeof(uint64_t);

                    RHI::DeviceBufferInitRequest request;
                    request.m_descriptor = bufferDesc;
                    request.m_buffer = buffers.m_cpuBuffers.get();
                    desc.m_readbackBufferPool->GetDeviceBufferPool(GetDevice().GetDeviceIndex())->InitBuffer(request);
                }
                {
                    buffers.m_gpuBuffers = RHI::Factory::Get().CreateBuffer();

                    RHI::BufferDescriptor bufferDesc;
                    bufferDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
                    bufferDesc.m_byteCount = desc.m_budget * sizeof(uint64_t);
                    bufferDesc.m_bindFlags = desc.m_copyBufferPool->GetDescriptor().m_bindFlags;
                    bufferDesc.m_alignment = sizeof(uint64_t);

                    RHI::DeviceBufferInitRequest request;
                    request.m_descriptor = bufferDesc;
                    request.m_buffer = buffers.m_gpuBuffers.get();
                    desc.m_copyBufferPool->GetDeviceBufferPool(GetDevice().GetDeviceIndex())->InitBuffer(request);
                }
            }
            return RHI::ResultCode::Success;
        }

        void RayTracingCompactionQueryPool::BeginFrame(int frame)
        {
            m_currentFrame = frame;
            for (auto& buffers : m_queryPoolBuffers)
            {
                if (buffers.m_enqueuedFrame == m_currentFrame - static_cast<int>(RHI::Limits::Device::FrameCountMax))
                {
                    auto pool = azrtti_cast<RHI::DeviceBufferPool*>(buffers.m_cpuBuffers->GetPool());
                    RHI::DeviceBufferMapRequest request;
                    request.m_buffer = buffers.m_cpuBuffers.get();
                    request.m_byteCount = buffers.m_cpuBuffers->GetDescriptor().m_byteCount;
                    request.m_byteOffset = 0;
                    RHI::DeviceBufferMapResponse response;
                    pool->MapBuffer(request, response);
                    AZ_Assert(response.m_data, "RayTracingCompactionQueryPool::BeginFrameInternal: Mapping result buffer failed");
                    auto mappedMemory = static_cast<uint64_t*>(response.m_data);

                    for (auto& [query, indexInBuffer] : buffers.enqueuedQueries)
                    {
                        query->SetResult(mappedMemory[indexInBuffer]);
                    }

                    pool->UnmapBuffer(*buffers.m_cpuBuffers);

                    buffers.enqueuedQueries.clear();
                    buffers.m_enqueuedFrame = -1;
                    buffers.m_nextFreeIndex = -1;
                }
            }

            m_currentBufferIndex = (m_currentBufferIndex + 1) % m_queryPoolBuffers.size();
            m_queryPoolBuffers[m_currentBufferIndex].m_nextFreeIndex = 0;
            m_queryPoolBuffers[m_currentBufferIndex].m_enqueuedFrame = m_currentFrame;
        }
    } // namespace DX12
} // namespace AZ
