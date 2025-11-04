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
    namespace Null
    {
        class RayTracingCompactionQueryPool;

        class RayTracingCompactionQuery final : public RHI::DeviceRayTracingCompactionQuery
        {
            using Base = RHI::DeviceRayTracingCompactionQuery;

        public:
            AZ_RTTI(RayTracingCompactionQuery, "{fbd10ff1-d508-4dee-a628-2d0db051b7f3}", Base);
            AZ_CLASS_ALLOCATOR(RayTracingCompactionQuery, AZ::SystemAllocator);
            ~RayTracingCompactionQuery() = default;
            static RHI::Ptr<RayTracingCompactionQuery> Create();

        private:
            RayTracingCompactionQuery() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingCompactionQuery
            uint64_t GetResult() override;
            RHI::ResultCode InitInternal(RHI::DeviceRayTracingCompactionQueryPool* pool) override;
            //////////////////////////////////////////////////////////////////////////
        };

        class RayTracingCompactionQueryPool final : public RHI::DeviceRayTracingCompactionQueryPool
        {
            using Base = RHI::DeviceRayTracingCompactionQueryPool;

        public:
            AZ_RTTI(RayTracingCompactionQueryPool, "{8048f0f9-7628-42a8-999a-c746be169eb7}", Base);
            AZ_CLASS_ALLOCATOR(RayTracingCompactionQueryPool, AZ::SystemAllocator);
            ~RayTracingCompactionQueryPool() = default;
            static RHI::Ptr<RayTracingCompactionQueryPool> Create();

        private:
            RayTracingCompactionQueryPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingCompactionQueryPool
            RHI::ResultCode InitInternal(RHI::RayTracingCompactionQueryPoolDescriptor desc) override;
            void BeginFrame(int frame) override;
            //////////////////////////////////////////////////////////////////////////
        };
    } // namespace Null
} // namespace AZ
