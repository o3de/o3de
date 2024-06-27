/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/sort.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>
#include <Atom/RHI/DeviceResourcePool.h>
#include <Atom/RHI.Reflect/Interval.h>
#include <Atom/RHI/DeviceQuery.h>
#include <Atom/RHI/QueryPoolSubAllocator.h>

namespace AZ::RHI
{
    class DeviceBuffer;
    class DeviceQuery;

    //! Controls how the results of a query are retrieved.
    enum class QueryResultFlagBits : uint32_t
    {
        None = 0,
        Wait = AZ_BIT(1) ///< The request will block waiting for the queries to finish.
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::QueryResultFlagBits)

    //! Query pool provides backing storage and context for query instances. The QueryPoolDescriptor
    //! contains properties defining memory characteristics of query pools. All queries created on a pool
    //! share the same backing and type.
    class DeviceQueryPool
        : public DeviceResourcePool
    {
        using Base = DeviceResourcePool;
    public:
        AZ_RTTI(DeviceQueryPool, "{D6744249-953F-45B6-AD90-B98B35E74521}", DeviceResourcePool)
        virtual ~DeviceQueryPool() override = default;

        //!  Initialize the DeviceQueryPool.
        ResultCode Init(Device& device, const QueryPoolDescriptor& descriptor);

        //! Initialize a query from the pool. When initializing multiple queries use the other version of InitQuery
        //! because the pool will try to group the queries together.
        //!  @param query DeviceQuery to initialize.
        ResultCode InitQuery(DeviceQuery* query);

        //!  Initialize a group of queries from the pool. The initialization will try to allocate
        //!  the queries in a consecutive space. The reason for this is that is more efficient when requesting
        //!  results or copying multiple query results.
        //!  @param queries Pointer to an array of queries to initialize.
        //!  @param queryCount Number of queries.
        ResultCode InitQuery(DeviceQuery** queries, uint32_t queryCount);

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

        //!  Same as GetResults(uint64_t, uint32_t, QueryResultFlagBits) but for one specific DeviceQuery
        ResultCode GetResults(DeviceQuery* query, uint64_t* result, uint32_t resultsCount, QueryResultFlagBits flags);

        //!  Same as GetResults(uint64_t, uint32_t, QueryResultFlagBits) but for a list of queries.
        //!  It's more efficient if the list of queries is sorted by handle in ascending order because there's no need to sort the results
        //! before returning.
        ResultCode GetResults(DeviceQuery** queries, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags);

        //!  Returns the buffer descriptor used to initialize the query pool. Descriptor contents
        //!  are undefined for uninitialized pools.
        const QueryPoolDescriptor& GetDescriptor() const override final;

        //!  Returns the query by their handle.
        const DeviceQuery* GetQuery(QueryHandle handle) const;

    protected:
        DeviceQueryPool() = default; 

        DeviceQuery* GetQuery(QueryHandle handle);

        //!  Find groups of consecutive QueryHandle values from a list of unsorted queries.
        //!  @param queries The list of queries to search.
        template<class T>
        AZStd::vector<Interval> GetQueryIntervals(const AZStd::vector<T*>& queries);

        //! Find groups of consecutive QueryHandle values from a list of sorted queries.
        //!  @param queries The list of sorted queries to search.
        template<class T>
        AZStd::vector<Interval> GetQueryIntervalsSorted(const AZStd::vector<T*>& queries);

        template<class T>
        void SortQueries(AZStd::vector<T*>& queries);

        //////////////////////////////////////////////////////////////////////////
        // DeviceResourcePool overrides
        void ShutdownInternal() override;
        void ShutdownResourceInternal(DeviceResource& resource) override;
        void ComputeFragmentation() const override {}
        //////////////////////////////////////////////////////////////////////////

    private:
        //////////////////////////////////////////////////////////////////////////
        // Interfaces that the platform implementation overrides.

        /// Called when the pool is being initialized.
        virtual ResultCode InitInternal(Device& device, const QueryPoolDescriptor& descriptor) = 0;

        /// Called when a query is being initialized onto the pool.
        virtual ResultCode InitQueryInternal(DeviceQuery& query) = 0;

        /// Gets the results for a group of consecutive queries.
        virtual ResultCode GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags) = 0;
        //////////////////////////////////////////////////////////////////////////


        /// Validates that the queries are not null and that they belong to this pool.
        ResultCode ValidateQueries(DeviceQuery** queries, uint32_t queryCount);

        /// Returns a vector with all queries of the pool. This function is thread safe.
        AZStd::vector<DeviceQuery*> GetQueries();

        QueryPoolDescriptor m_descriptor;
        mutable AZStd::mutex m_queriesMutex;
        QueryPoolSubAllocator m_queryAllocator; ///< Used to allocate which index a query will use.
        AZStd::vector<DeviceQuery*> m_queries; ///< The list with all the queries of the pool ordered by their handle index.
    };

    template<class T>
    void DeviceQueryPool::SortQueries(AZStd::vector<T*>& queries)
    {
        AZStd::sort(queries.begin(), queries.end(), [](const auto& lhs, const auto& rhs)
        {
            return lhs->GetHandle().GetIndex() < rhs->GetHandle().GetIndex();
        });
    }

    template<class T>
    AZStd::vector<Interval> DeviceQueryPool::GetQueryIntervals(const AZStd::vector<T*>& queries)
    {
        AZStd::vector<T*> sortedQueries(queries);
        // Sort them by index ...
        SortQueries(sortedQueries);
        return GetQueryIntervalsSorted(sortedQueries);
    }

    template<class T>
    AZStd::vector<Interval> DeviceQueryPool::GetQueryIntervalsSorted(const AZStd::vector<T*>& sortedQueries)
    {
        if (sortedQueries.empty())
        {
            return AZStd::vector<Interval>();
        }

        AZStd::vector<Interval> intervals;
        size_t lastQueryIndex = 0;
        for (size_t i = 0; i < sortedQueries.size(); ++i)
        {
            QueryHandle queryHandle = sortedQueries[i]->GetHandle();
            // Check that the current handle value is consecutive with the previous one. If not,
            // we need to insert a new interval.
            if ((queryHandle.GetIndex() - sortedQueries[lastQueryIndex]->GetHandle().GetIndex()) != (i - lastQueryIndex))
            {
                intervals.emplace_back(sortedQueries[lastQueryIndex]->GetHandle().GetIndex(), sortedQueries[i - 1]->GetHandle().GetIndex());
                lastQueryIndex = i;
            }
        }

        intervals.emplace_back(sortedQueries[lastQueryIndex]->GetHandle().GetIndex(), sortedQueries[sortedQueries.size() - 1]->GetHandle().GetIndex());
        return intervals;
    }
}
