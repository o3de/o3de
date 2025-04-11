/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>
#include <Atom/RPI.Public/GpuQuery/QueryPool.h>

namespace AZ
{
    namespace RPI
    {
        //! Timestamp pool context specialization for timestamp queries.
        class ATOM_RPI_PUBLIC_API TimestampQueryPool final
            : public QueryPool
        {
            // Inherit the constructors of RPI::QueryPool.
            using RPI::QueryPool::QueryPool;

        public:
            AZ_RTTI(TimestampQueryPool, "{95A8D7ED-9BAD-4EC4-A201-AF4FD6345D17}", QueryPool);
            AZ_CLASS_ALLOCATOR(TimestampQueryPool, SystemAllocator);

            //! Only use this function to create a new Timestamp QueryPool object. And force using smart pointer to manage pool's life time.
            static QueryPoolPtr CreateTimestampQueryPool(uint32_t queryCount);

        protected:
            // RPI::QueryPool overrides...
            RHI::ResultCode BeginQueryInternal(RHI::Interval rhiQueryIndices, const RHI::FrameGraphExecuteContext& context);
            RHI::ResultCode EndQueryInternal(RHI::Interval rhiQueryIndices, const RHI::FrameGraphExecuteContext& context);

        private:
            TimestampQueryPool(uint32_t queryCapacity, uint32_t queriesPerResult, RHI::QueryType queryType, RHI::PipelineStatisticsFlags statisticsFlags);
        };
    }
}
