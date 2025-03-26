/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceRayTracingCompactionQueryPool.h>
#include <Atom/RHI/Query.h>
#include <vulkan/vulkan.h>

namespace AZ
{
    namespace Vulkan
    {
        class CommandList;

        class RayTracingCompactionQuery final : public RHI::DeviceRayTracingCompactionQuery
        {
            using Base = RHI::DeviceRayTracingCompactionQuery;

        public:
            AZ_RTTI(RayTracingCompactionQuery, "{50521a53-9740-4e35-b77c-c039016cc44f}", Base);
            AZ_CLASS_ALLOCATOR(RayTracingCompactionQuery, AZ::SystemAllocator);
            ~RayTracingCompactionQuery();
            static RHI::Ptr<RayTracingCompactionQuery> Create();

            void Allocate();
            int GetIndexInPool();

        private:
            RayTracingCompactionQuery() = default;
            RayTracingCompactionQuery(const RayTracingCompactionQuery&) = delete;
            RayTracingCompactionQuery(RayTracingCompactionQuery&&) = delete;
            void operator=(const RayTracingCompactionQuery&) = delete;
            void operator=(RayTracingCompactionQuery&&) = delete;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingCompactionQuery
            uint64_t GetResult() override;
            RHI::ResultCode InitInternal(RHI::DeviceRayTracingCompactionQueryPool* pool) override;
            //////////////////////////////////////////////////////////////////////////

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

            int Allocate();
            void Deallocate(int index);

            uint64_t GetResult(int index);

            VkQueryPool GetNativeQueryPool();
            void ResetFreedQueries(CommandList* commandList);

        private:
            RayTracingCompactionQueryPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingCompactionQueryPool
            RHI::ResultCode InitInternal(RHI::RayTracingCompactionQueryPoolDescriptor desc) override;
            //////////////////////////////////////////////////////////////////////////

            VkQueryPool m_nativeQueryPool = VK_NULL_HANDLE;
            AZStd::vector<int> m_freeList;
            AZStd::vector<int> m_queriesEnqueuedForReset;
        };
    } // namespace Vulkan
} // namespace AZ
