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
#include <Atom/RHI/MultiDeviceQuery.h>
#include <Atom/RHI/MultiDeviceResourcePool.h>
#include <Atom/RHI/QueryPoolSubAllocator.h>
#include <Atom/RHI/QueryPool.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace RHI
    {
        class MultiDeviceQuery;

        //! Query pool provides backing storage and context for query instances. The QueryPoolDescriptor
        //! contains properties defining memory characteristics of query pools. All queries created on a pool
        //! share the same backing and type.
        class MultiDeviceQueryPool : public MultiDeviceResourcePool
        {
            using Base = MultiDeviceResourcePool;

        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceQueryPool, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceQueryPool, "{F46A756D-99F1-4A2A-AE4C-A2A8BE6845CC}", MultiDeviceResourcePool)
            MultiDeviceQueryPool() = default;
            virtual ~MultiDeviceQueryPool() override = default;

            inline Ptr<QueryPool> GetDeviceQueryPool(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDeviceQueryPool",
                    m_deviceQueryPools.find(deviceIndex) != m_deviceQueryPools.end(),
                    "No DeviceQueryPool found for device index %d\n",
                    deviceIndex);

                return m_deviceQueryPools.at(deviceIndex);
            }

            //!  Initialize the MultiDeviceQueryPool.
            ResultCode Init(MultiDevice::DeviceMask deviceMask, const QueryPoolDescriptor& descriptor);

            //! Initialize a query from the pool. When initializing multiple queries use the other version of InitQuery
            //! because the pool will try to group the queries together.
            //!  @param query MultiDeviceQuery to initialize.
            ResultCode InitQuery(MultiDeviceQuery* query);

            //!  Initialize a group of queries from the pool. The initialization will try to allocate
            //!  the queries in a consecutive space. The reason for this is that is more efficient when requesting
            //!  results or copying multiple query results.
            //!  @param queries Pointer to an array of queries to initialize.
            //!  @param queryCount Number of queries.
            ResultCode InitQuery(MultiDeviceQuery** queries, uint32_t queryCount);

            //!  Get the results from all queries in the pool. Results are always returned as uint64_t data.
            //!  The "results" parameter must contain enough space to save the results from all queries in the pool.
            //!  For the PipelineStatistics query type, each statistic will be copied to a uint64_t. Because of this
            //!  the "results" array must contain enough space for numQueries * numPipelineStatistics.
            //!  This function doesn't return partial results. In case of failure no results are returned.
            //!  @param results Pointer to an array where the results will be copied.
            //!  @param resultsCount Number of elements of the "results" array.
            //!  @param QueryResultFlagBits Control how the results will be requested. If QueryResultFlagBits::Wait is used
            //!                            the call will block until the results are done. If QueryResultFlagBits::Wait is not used
            //!                            and the results are not ready, ResultCode::NotReady will be returned
            ResultCode GetResults(uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags);

            //!  Get the results from a query in the pool. Results are always returned as uint64_t data.
            //!  The "results" parameter must contain enough space to save the results from all queries in the pool.
            //!  For the PipelineStatistics query type, each statistic will be copied to a uint64_t. Because of this
            //!  the "results" array must contain enough space for numQueries * numPipelineStatistics.
            //!  This function doesn't return partial results. In case of failure no results are returned.
            //!  @param results Pointer to an array where the results will be copied.
            //!  @param resultsCount Number of elements of the "results" array.
            //!  @param QueryResultFlagBits Control how the results will be requested. If QueryResultFlagBits::Wait is used
            //!                            the call will block until the results are done. If QueryResultFlagBits::Wait is not used
            //!                            and the results are not ready, ResultCode::NotReady will be returned
            ResultCode GetResults(MultiDeviceQuery* query, uint64_t* result, uint32_t resultsCount, QueryResultFlagBits flags);

            //!  Same as GetResults(uint64_t, uint32_t, QueryResultFlagBits) but for a list of queries.
            //!  It's more efficient if the list of queries is sorted by handle in ascending order because there's no need to sort the
            //!  results
            //! before returning.
            ResultCode GetResults(
                MultiDeviceQuery** queries, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags);

            //!  Returns the buffer descriptor used to initialize the query pool. Descriptor contents
            //!  are undefined for uninitialized pools.
            const QueryPoolDescriptor& GetDescriptor() const override final;

            void Shutdown() override final;

        private:
            /// Validates that the queries are not null and that they belong to this pool.
            ResultCode ValidateQueries(MultiDeviceQuery** queries, uint32_t queryCount);

            QueryPoolDescriptor m_descriptor;
            AZStd::unordered_map<int, Ptr<QueryPool>> m_deviceQueryPools;
        };
    } // namespace RHI
} // namespace AZ
