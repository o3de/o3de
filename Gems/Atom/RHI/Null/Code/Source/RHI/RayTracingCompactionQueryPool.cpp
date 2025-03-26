/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/RayTracingCompactionQueryPool.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<RayTracingCompactionQuery> RayTracingCompactionQuery::Create()
        {
            return aznew RayTracingCompactionQuery;
        }

        uint64_t RayTracingCompactionQuery::GetResult()
        {
            return 0;
        }

        RHI::ResultCode RayTracingCompactionQuery::InitInternal([[maybe_unused]] RHI::DeviceRayTracingCompactionQueryPool* pool)
        {
            return RHI::ResultCode::Success;
        }

        RHI::Ptr<RayTracingCompactionQueryPool> RayTracingCompactionQueryPool::Create()
        {
            return aznew RayTracingCompactionQueryPool;
        }

        RHI::ResultCode RayTracingCompactionQueryPool::InitInternal([[maybe_unused]] RHI::RayTracingCompactionQueryPoolDescriptor desc)
        {
            return RHI::ResultCode::Success;
        }

        void RayTracingCompactionQueryPool::BeginFrame([[maybe_unused]] int frame)
        {
        }
    } // namespace Null
} // namespace AZ
