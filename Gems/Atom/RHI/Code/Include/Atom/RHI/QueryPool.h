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
#include <Atom/RHI/Query.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/DeviceQueryPool.h>
#include <Atom/RHI/QueryPoolSubAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/sort.h>

namespace AZ::RHI
{
    class Query;

    //! QueryPool manages a map of device-specific QueryPools, which provide backing storage and context for query instances. The
    //! QueryPoolDescriptor contains properties defining memory characteristics of query pools. All queries created on a pool share the same
    //! backing and type.
    class QueryPool : public ResourcePool
    {
        using Base = ResourcePool;

    public:
        AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator, 0);
        AZ_RTTI(QueryPool, "{F46A756D-99F1-4A2A-AE4C-A2A8BE6845CC}", ResourcePool)
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(QueryPool);
        QueryPool() = default;
        virtual ~QueryPool() override = default;

        //!  Initialize the QueryPool by initializing all device-specific QueryPools for each device mentioned in the descriptor's
        //!  deviceMask.
        ResultCode Init(const QueryPoolDescriptor& descriptor);

        //! Initialize a query from the pool (one device-specific query for each DeviceQueryPool).
        //! When initializing multiple queries use the other version of InitQuery
        //! because the pool will try to group the queries together.
        //!  @param query Query to initialize.
        ResultCode InitQuery(Query* query);

        //!  Initialize a group of queries from the pool. The initialization will try to allocate
        //!  the queries in a consecutive space (consecutive per device). The reason for this is that is more efficient when requesting
        //!  results or copying multiple query results.
        //!  @param queries Pointer to an array of queries to initialize.
        //!  @param queryCount Number of queries.
        ResultCode InitQuery(Query** queries, uint32_t queryCount);

        //! Get the number of results that have to be allocated.
        //! The number returned is the number of results per query, multiplied by the number of queries and the number of devices.
        //! The number of devices is queried from the RHISystem.
        //! If the queryCount is omitted or equal to 0, the total number of queries in the pool is used.
        uint32_t CalculateResultsCount(uint32_t queryCount = 0);

        //! Get the results from all queries (from all devices) in the pool, which are returned as uint64_t data.
        //! The parameter "resultsCount" denotes the total number of results requested. It can be determined by calling GetResultsCount().
        //! The "results" parameter must contain enough space to save the results from all queries (from all devices) in the pool,
        //! i.e. resultCount * sizeof(uint64_t), must be pre-allocated.
        //! Results are ordered by device (using the deviceIndex) first and then per query, i.e., all results from a device are
        //! consecutive in memory.
        //! Data will only be written to the results array if the device actually exists, i.e., if its bit in the query's device mask is
        //! set and the device index is lower than the RHISystem's device count.
        //! The function can return partial results. In case of failure of requesting results from a specific device, only results from
        //! lower-indexed devices (which already have successfully returned results) are returned.
        //! For further details related to device-specific query functionality, please check the related header.
        ResultCode GetResults(uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags);

        //! Same as GetResults(uint64_t, uint32_t, QueryResultFlagBits) but for a specific multi-device query.
        ResultCode GetResults(Query* query, uint64_t* result, uint32_t resultsCount, QueryResultFlagBits flags);

        //!  Same as GetResults(Query*, uint64_t, uint32_t, QueryResultFlagBits) but for a list of queries.
        //!  It's more efficient if the list of queries is sorted by handle in ascending order because there's no need to sort the
        //!  results before returning.
        ResultCode GetResults(
            Query** queries, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags);

        //!  Returns the buffer descriptor used to initialize the query pool. Descriptor contents
        //!  are undefined for uninitialized pools.
        const QueryPoolDescriptor& GetDescriptor() const override final;

        //! Forwards the shutdown call to all device-specific QueryPools.
        void Shutdown() override final;

    private:
        //! Get the number of results that have to be allocated per device.
        //! The number returned is the number of results per query, multiplied by the number of queries.
        //! If the queryCount is omitted or equal to 0, the total number of queries in the pool is used.
        uint32_t CalculatePerDeviceResultsCount(uint32_t queryCount = 0);

        //! Validates that the queries are not null and that they belong to this pool.
        ResultCode ValidateQueries(Query** queries, uint32_t queryCount);

        QueryPoolDescriptor m_descriptor;
    };
} // namespace AZ::RHI
