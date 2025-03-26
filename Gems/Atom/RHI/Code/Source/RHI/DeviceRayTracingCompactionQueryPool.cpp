/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceRayTracingCompactionQueryPool.h>

namespace AZ::RHI
{
    ResultCode DeviceRayTracingCompactionQuery::Init(Device& device, DeviceRayTracingCompactionQueryPool* pool)
    {
        DeviceObject::Init(device);
        m_pool = pool;
        return InitInternal(pool);
    }

    DeviceRayTracingCompactionQueryPool* DeviceRayTracingCompactionQuery::GetPool()
    {
        return m_pool;
    }

    ResultCode DeviceRayTracingCompactionQueryPool::Init(Device& device, RayTracingCompactionQueryPoolDescriptor desc)
    {
        DeviceObject::Init(device);
        m_desc = desc;
        return InitInternal(desc);
    }

    const RayTracingCompactionQueryPoolDescriptor& DeviceRayTracingCompactionQueryPool::GetDescriptor()
    {
        return m_desc;
    }
} // namespace AZ::RHI
