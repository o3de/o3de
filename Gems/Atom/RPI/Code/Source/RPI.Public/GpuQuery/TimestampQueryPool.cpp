/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/GpuQuery/TimestampQueryPool.h>

namespace AZ
{
    namespace RPI
    {
        TimestampQueryPool::TimestampQueryPool(uint32_t queryCapacity, uint32_t queriesPerResult, RHI::QueryType queryType, RHI::PipelineStatisticsFlags statisticsFlags) :
            QueryPool(queryCapacity, queriesPerResult, queryType, statisticsFlags)
        {
        }

        QueryPoolPtr AZ::RPI::TimestampQueryPool::CreateTimestampQueryPool(uint32_t queryCount)
        {
            // The number of RHI Queries required to calculate a single Timestamp Result.
            const uint32_t RhiQueriesPerTimestampResult = 2u;
            return AZStd::unique_ptr<QueryPool>(aznew TimestampQueryPool(queryCount, RhiQueriesPerTimestampResult, RHI::QueryType::Timestamp, RHI::PipelineStatisticsFlags::None));
        }

        RHI::ResultCode TimestampQueryPool::BeginQueryInternal(RHI::Interval rhiQueryIndices, RHI::CommandList& commandList)
        {
            AZStd::span<const RHI::Ptr<RHI::SingleDeviceQuery>> rhiQueryArray = GetRhiQueryArray();
            AZ::RHI::Ptr<AZ::RHI::SingleDeviceQuery> beginQuery = rhiQueryArray[rhiQueryIndices.m_min];

            return beginQuery->WriteTimestamp(commandList);
        }

        RHI::ResultCode TimestampQueryPool::EndQueryInternal(RHI::Interval rhiQueryIndices, RHI::CommandList& commandList)
        {
            AZStd::span<const RHI::Ptr<RHI::SingleDeviceQuery>> rhiQueryArray = GetRhiQueryArray();
            AZ::RHI::Ptr<AZ::RHI::SingleDeviceQuery> endQuery = rhiQueryArray[rhiQueryIndices.m_max];

            return endQuery->WriteTimestamp(commandList);
        }
    };  // Namespace RPI
};  // Namespace AZ
