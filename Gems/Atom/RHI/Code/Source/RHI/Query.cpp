/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Query.h>
#include <Atom/RHI/QueryPool.h>

namespace AZ::RHI
{
    const QueryPool* Query::GetQueryPool() const
    {
        return static_cast<const QueryPool*>(GetPool());
    }

    QueryPool* Query::GetQueryPool()
    {
        return static_cast<QueryPool*>(GetPool());
    }

    void Query::Shutdown()
    {
        Resource::Shutdown();
    }
} // namespace AZ::RHI
