/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceQuery.h>
#include <Atom/RHI/Resource.h>

namespace AZ
{
    namespace RHI
    {
        class CommandList;
        class QueryPool;
        using QueryHandle = RHI::Handle<uint32_t>;

        /**
        * Query resource for recording gpu data like occlusion, timestamp or pipeline statistics.
        * Queries belong to a QueryPool and their types are determinated by the pool.
        */
        class Query : public Resource
        {
            friend class QueryPool;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::SystemAllocator, 0);
            AZ_RTTI(Query, "{055C873F-490F-4469-9FD6-76997D1FDC7E}", Resource);

            Query() = default;
            virtual ~Query() override = default;

            inline Ptr<DeviceQuery> GetDeviceQuery(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceQueries.find(deviceIndex) != m_deviceQueries.end(), "No DeviceQuery found for device index %d\n", deviceIndex);
                return m_deviceQueries.at(deviceIndex);
            }

            /// Returns the query pool that this query belongs to.
            const QueryPool* GetQueryPool() const;
            QueryPool* GetQueryPool();

        private:
            AZStd::unordered_map<int, Ptr<DeviceQuery>> m_deviceQueries;
        };
    }
}
