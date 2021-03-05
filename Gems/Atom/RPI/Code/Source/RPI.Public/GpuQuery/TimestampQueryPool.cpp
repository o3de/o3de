/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZStd::array_view<RHI::Ptr<RHI::Query>> rhiQueryArray = GetRhiQueryArray();
            AZ::RHI::Ptr<AZ::RHI::Query> beginQuery = rhiQueryArray[rhiQueryIndices.m_min];

            return beginQuery->WriteTimestamp(commandList);
        }

        RHI::ResultCode TimestampQueryPool::EndQueryInternal(RHI::Interval rhiQueryIndices, RHI::CommandList& commandList)
        {
            AZStd::array_view<RHI::Ptr<RHI::Query>> rhiQueryArray = GetRhiQueryArray();
            AZ::RHI::Ptr<AZ::RHI::Query> endQuery = rhiQueryArray[rhiQueryIndices.m_max];

            return endQuery->WriteTimestamp(commandList);
        }
    };  // Namespace RPI
};  // Namespace AZ
