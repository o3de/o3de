/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/MultiDeviceQuery.h>
#include <Atom/RHI/MultiDeviceQueryPool.h>

namespace AZ::RHI
{
    const MultiDeviceQueryPool* MultiDeviceQuery::GetQueryPool() const
    {
        return static_cast<const MultiDeviceQueryPool*>(GetPool());
    }

    MultiDeviceQueryPool* MultiDeviceQuery::GetQueryPool()
    {
        return static_cast<MultiDeviceQueryPool*>(GetPool());
    }

    void MultiDeviceQuery::Shutdown()
    {
        for (auto& [_, deviceQuery] : m_deviceQueries)
        {
            deviceQuery->Shutdown();
        }

        MultiDeviceResource::Shutdown();
    }

    void MultiDeviceQuery::InvalidateViews()
    {
        for (auto& [_, deviceQuery] : m_deviceQueries)
        {
            deviceQuery->InvalidateViews();
        }
    }
} // namespace AZ::RHI
