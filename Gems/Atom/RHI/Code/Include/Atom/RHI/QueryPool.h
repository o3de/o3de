/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Interval.h>
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>
#include <Atom/RHI/DeviceQueryPool.h>
#include <Atom/RHI/Query.h>
#include <Atom/RHI/QueryPoolSubAllocator.h>
#include <Atom/RHI/ResourcePool.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace RHI
    {
        class Query;

         //! Query pool provides backing storage and context for query instances. The QueryPoolDescriptor
         //! contains properties defining memory characteristics of query pools. All queries created on a pool
         //! share the same backing and type.
        class QueryPool : public ResourcePool
        {
            using Base = ResourcePool;

        public:
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator, 0);
            AZ_RTTI(QueryPool, "{AB649CDC-1AB3-41ED-BAC5-23460FFBC4AD}", ResourcePool)
            QueryPool() = default;
            virtual ~QueryPool() override = default;

            inline Ptr<DeviceQueryPool> GetDeviceQueryPool(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceQueryPools.find(deviceIndex) != m_deviceQueryPools.end(),
                    "No DeviceQueryPool found for device index %d\n",
                    deviceIndex);
                return m_deviceQueryPools.at(deviceIndex);
            }

            //!  Initialize the QueryPool.
            ResultCode Init(DeviceMask deviceMask, const QueryPoolDescriptor& descriptor);

            //! Initialize a query from the pool. When initializing multiple queries use the other version of InitQuery
            //! because the pool will try to group the queries together.
            //!  @param query Query to initialize.
            ResultCode InitQuery(Query* query);

            //!  Initialize a group of queries from the pool. The initialization will try to allocate
            //!  the queries in a consecutive space. The reason for this is that is more efficient when requesting
            //!  results or copying multiple query results.
            //!  @param queries Pointer to an array of queries to initialize.
            //!  @param queryCount Number of queries.
            ResultCode InitQuery(Query** queries, uint32_t queryCount);

            //!  Get the results from a query in the pool. Results are always returned as uint64_t data.
            //!  The "results" parameter must contain enough space to save the results from the query in the pool.
            //!  For the PipelineStatistics query type, each statistic will be copied to a uint64_t. Because of this
            //!  the "results" array must contain enough space for numPipelineStatistics.
            //!  This function doesn't return partial results. In case of failure no results are returned.
            //!  @param results Pointer to an array where the results will be copied.
            //!  @param resultsCount Number of elements of the "results" array.
            //!  @param QueryResultFlagBits Control how the results will be requested. If QueryResultFlagBits::Wait is used
            //!                            the call will block until the results are done. If QueryResultFlagBits::Wait is not used
            //!                            and the results are not ready, ResultCode::NotReady will be returned
            ResultCode GetResults(Query* query, uint64_t* result, uint32_t resultsCount, QueryResultFlagBits flags);

            //!  Same as GetResults(Query, uint64_t, uint32_t, QueryResultFlagBits) but for a list of queries.
            //!  It's more efficient if the list of queries is sorted by handle in ascending order because there's no need to sort the
            //!  results
            //! before returning.
            ResultCode GetResults(Query** queries, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags);

            //!  Returns the descriptor used to initialize the query pool. Descriptor contents
            //!  are undefined for uninitialized pools.
            const QueryPoolDescriptor& GetDescriptor() const override final;

        private:
            /// Validates that the queries are not null and that they belong to this pool.
            ResultCode ValidateQueries(Query** queries, uint32_t queryCount);

            QueryPoolDescriptor m_descriptor;
            AZStd::unordered_map<int, Ptr<DeviceQueryPool>> m_deviceQueryPools;
        };
    } // namespace RHI
}
