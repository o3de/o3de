/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceRayTracingCompactionQueryPool.h>

namespace AZ
{
    namespace DX12
    {
        class RayTracingCompactionQueryPool;

        class RayTracingCompactionQuery final : public RHI::DeviceRayTracingCompactionQuery
        {
            using Base = RHI::DeviceRayTracingCompactionQuery;
            friend class RayTracingCompactionQueryPool;

        public:
            AZ_RTTI(RayTracingCompactionQuery, "{50521a53-9740-4e35-b77c-c039016cc44f}", Base);
            AZ_CLASS_ALLOCATOR(RayTracingCompactionQuery, AZ::SystemAllocator);
            ~RayTracingCompactionQuery() = default;
            static RHI::Ptr<RayTracingCompactionQuery> Create();

            int Allocate();

        private:
            RayTracingCompactionQuery() = default;

            void SetResult(uint64_t value);

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingCompactionQuery
            uint64_t GetResult() override;
            RHI::ResultCode InitInternal(RHI::DeviceRayTracingCompactionQueryPool* pool) override;
            //////////////////////////////////////////////////////////////////////////

            AZStd::optional<uint64_t> m_result;
            AZStd::optional<int> m_indexInPool;
        };

        class RayTracingCompactionQueryPool final : public RHI::DeviceRayTracingCompactionQueryPool
        {
            using Base = RHI::DeviceRayTracingCompactionQueryPool;

        public:
            AZ_RTTI(RayTracingCompactionQueryPool, "{7bd9b295-cbb9-4fd4-95fc-0df6ad77e92a}", Base);
            AZ_CLASS_ALLOCATOR(RayTracingCompactionQueryPool, AZ::SystemAllocator);
            ~RayTracingCompactionQueryPool() = default;
            static RHI::Ptr<RayTracingCompactionQueryPool> Create();

            int Allocate(RayTracingCompactionQuery* query);

            RHI::DeviceBuffer* GetCurrentGPUBuffer();
            RHI::DeviceBuffer* GetCurrentCPUBuffer();

        private:
            RayTracingCompactionQueryPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingCompactionQueryPool
            RHI::ResultCode InitInternal(RHI::RayTracingCompactionQueryPoolDescriptor desc) override;
            void BeginFrame(int frame) override;
            //////////////////////////////////////////////////////////////////////////

            struct QueryPoolBuffers
            {
                RHI::Ptr<RHI::DeviceBuffer> m_gpuBuffers;
                RHI::Ptr<RHI::DeviceBuffer> m_cpuBuffers;
                int m_nextFreeIndex = 0;
                int m_enqueuedFrame = -1;

                AZStd::vector<AZStd::pair<RHI::Ptr<RayTracingCompactionQuery>, int>> enqueuedQueries;
            };

            AZStd::array<QueryPoolBuffers, RHI::Limits::Device::FrameCountMax> m_queryPoolBuffers;
            int m_currentBufferIndex = 0;

            int m_currentFrame = -1;
        };
    } // namespace DX12
} // namespace AZ
