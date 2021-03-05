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
#pragma once

#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>
#include <Atom/RPI.Public/GpuQuery/QueryPool.h>

namespace AZ
{
    namespace RPI
    {
        //! Timestamp pool context specialization for timestamp queries.
        class TimestampQueryPool final
            : public QueryPool
        {
            // Inherit the constructors of RPI::QueryPool.
            using RPI::QueryPool::QueryPool;

        public:
            AZ_RTTI(TimestampQueryPool, "{95A8D7ED-9BAD-4EC4-A201-AF4FD6345D17}", QueryPool);
            AZ_CLASS_ALLOCATOR(TimestampQueryPool, SystemAllocator, 0);

            //! Only use this function to create a new Timestamp QueryPool object. And force using smart pointer to manage pool's life time.
            static QueryPoolPtr CreateTimestampQueryPool(uint32_t queryCount);

        protected:
            // RPI::QueryPool overrides...
            RHI::ResultCode BeginQueryInternal(RHI::Interval rhiQueryIndices, RHI::CommandList& commandList);
            RHI::ResultCode EndQueryInternal(RHI::Interval rhiQueryIndices, RHI::CommandList& commandList);

        private:
            TimestampQueryPool(uint32_t queryCapacity, uint32_t queriesPerResult, RHI::QueryType queryType, RHI::PipelineStatisticsFlags statisticsFlags);
        };
    }
}
