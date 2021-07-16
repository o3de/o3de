/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI/Query.h>

#include <AzCore/std/parallel/lock.h>

namespace AZ
{
    namespace RHI
    {        
        ResultCode QueryPool::Init(Device& device, const QueryPoolDescriptor& descriptor)
        {
            if (Validation::IsEnabled())
            {
                if (!descriptor.m_queriesCount)
                {
                    AZ_Error("RHI", false, "QueryPool size can't be zero");
                    return ResultCode::InvalidArgument;
                }

                if (descriptor.m_type == QueryType::PipelineStatistics && descriptor.m_pipelineStatisticsMask == PipelineStatisticsFlags::None)
                {
                    AZ_Error("RHI", false, "Missing pipeline statistics flags");
                    return ResultCode::InvalidArgument;
                }

                if (descriptor.m_type != QueryType::PipelineStatistics && descriptor.m_pipelineStatisticsMask != PipelineStatisticsFlags::None)
                {
                    AZ_Warning("RHI", false, "Pipeline statistics flags are only valid for PipelineStatistics pools. Ignoring m_pipelineStatisticsMask");
                }
            }

            m_queries.resize(descriptor.m_queriesCount, nullptr);
            m_queryAllocator.Init(descriptor.m_queriesCount);

            return ResourcePool::Init(
                device, descriptor,
                [this, &device, &descriptor]()
            {
                /**
                 * Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                 * for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                 * possibility that users will get garbage values from GetDescriptor().
                 */
                m_descriptor = descriptor;

                return InitInternal(device, descriptor);
            });
        }

        ResultCode QueryPool::InitQuery(Query* query)
        {
            return InitQuery(&query, 1);
        }

        ResultCode QueryPool::InitQuery(Query** queries, uint32_t queryCount)
        {
            AZ_Assert(queries, "Null queries");
            AZStd::vector<QueryPoolSubAllocator::Allocation> allocationIntervals;
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_queriesMutex);
                allocationIntervals = m_queryAllocator.Allocate(queryCount);
            }

            if (allocationIntervals.empty())
            {
                return ResultCode::OutOfMemory;
            }

            uint32_t queryIndex = 0;
            ResultCode result = ResultCode::Fail;
            for (const auto& allocationInterval : allocationIntervals)
            {
                uint32_t end = allocationInterval.m_offset + allocationInterval.m_count;
                for (uint32_t i = allocationInterval.m_offset; i < end; ++i, ++queryIndex)
                {
                    Query& query = *queries[queryIndex];
                    query.m_handle = QueryHandle(i);
                    result = ResourcePool::InitResource(&query, [this, &query]() {return InitQueryInternal(query); });
                    if (result != ResultCode::Success)
                    {
                        return result;
                    }

                    m_queries[i] = queries[queryIndex];
                }               
            }

            return result;
        }

        ResultCode QueryPool::ValidateQueries(Query** queries, uint32_t queryCount)
        {
            if (!queryCount)
            {
                AZ_Error("RHI", false, "Query count is 0");
                return RHI::ResultCode::InvalidArgument;
            }

            for (uint32_t i = 0; i < queryCount; ++i)
            {
                auto* query = queries[i];
                if (!query)
                {
                    AZ_Error("RHI", false, "Null query %i", i);
                    return RHI::ResultCode::InvalidArgument;
                }

                if (query->GetQueryPool() != this)
                {
                    AZ_Error("RHI", false, "Query does not belong to this pool");
                    return RHI::ResultCode::InvalidArgument;
                }

                auto queryIndex = query->GetHandle().GetIndex();
                if (queryIndex >= static_cast<uint32_t>(m_queries.size()) || queryIndex >= GetResourceCount())
                {
                    AZ_Error("RHI", false, "Invalid query handle for query %d", i);
                    return RHI::ResultCode::InvalidArgument;
                }

                if (GetQuery(query->GetHandle()) != query)
                {
                    AZ_Error("RHI", false, "Invalid query");
                    return RHI::ResultCode::InvalidArgument;
                }
            }
            return ResultCode::Success;
        }

        ResultCode QueryPool::GetResults(Query* query, uint64_t* result, uint32_t resultsCount, QueryResultFlagBits flags)
        {
            return GetResults(&query, 1, result, resultsCount, flags);
        }

        ResultCode QueryPool::GetResults(Query** queries, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags)
        {
            AZ_Assert(queries && queryCount, "Null queries");
            AZ_Assert(results && resultsCount, "Null results");
            uint32_t perResultSize = m_descriptor.m_type == QueryType::PipelineStatistics ? CountBitsSet(static_cast<uint64_t>(m_descriptor.m_pipelineStatisticsMask)) : 1;

            if (Validation::IsEnabled())
            {
                auto validationResult = ValidateQueries(queries, queryCount);
                if (validationResult != ResultCode::Success)
                {
                    return validationResult;
                }

                if (perResultSize * queryCount > resultsCount)
                {
                    AZ_Error("RHI", false, "Results count is too small. Needed at least %d", perResultSize * queryCount);
                    return RHI::ResultCode::InvalidArgument;
                }
            }

            AZStd::vector<uint32_t> resultsOrder;
            // Get the group of consecutive queries from the provided list.
            AZStd::vector<Query*> sortedQueries(queries, queries + queryCount);
            SortQueries(sortedQueries);

            AZStd::vector<Interval> intervals = GetQueryIntervalsSorted(sortedQueries);
            uint64_t* resultsPointer = results;
            // Call the platform implementation with each of this group of consecutive queries.
            for (auto const& interval : intervals)
            {
                uint32_t intervalSize = interval.m_max - interval.m_min + 1;
                ResultCode result = GetResultsInternal(
                    interval.m_min,
                    intervalSize,
                    resultsPointer,
                    intervalSize * perResultSize,
                    flags);

                if (result != ResultCode::Success)
                {
                    return result;
                }
                resultsPointer += intervalSize * perResultSize;
            }

            // Sort the results using the order of the query list.
            AZStd::unordered_map<uint32_t, uint32_t> queryToSlotMap;
            for (uint32_t i = 0; i < queryCount; ++i)
            {
                queryToSlotMap[queries[i]->GetHandle().GetIndex()] = i;
            }

            AZStd::vector<uint64_t> tempResult(perResultSize);
            for (size_t i = 0; i < sortedQueries.size();)
            {
                Query* sortedQuery = sortedQueries[i];
                uint32_t slot = queryToSlotMap[sortedQuery->GetHandle().GetIndex()];
                if (i == slot)
                {
                    ++i;
                    continue;
                }

                // Swap contents
                ::memcpy(tempResult.data(), results + perResultSize * slot, perResultSize * sizeof(uint64_t));
                ::memcpy(results + perResultSize * slot, results + perResultSize * i, perResultSize * sizeof(uint64_t));
                ::memcpy(results + perResultSize * i, tempResult.data(), perResultSize * sizeof(uint64_t));
                AZStd::swap(sortedQueries[i], sortedQueries[slot]);
            }

            return ResultCode::Success;
        }

        ResultCode QueryPool::GetResults(uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags)
        {
            AZ_Assert(resultsCount <= m_queries.size(), "Invalid size for writing the query results");
            AZStd::vector<Query*> queries = GetQueries();
            return GetResults(queries.data(), static_cast<uint32_t>(queries.size()), results, resultsCount, flags);
        }

        const QueryPoolDescriptor& QueryPool::GetDescriptor() const
        {
            return m_descriptor;
        }

        const Query* QueryPool::GetQuery(QueryHandle handle) const
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_queriesMutex);
            return m_queries[handle.GetIndex()];
        }

        Query* QueryPool::GetQuery(QueryHandle handle)
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_queriesMutex);
            return m_queries[handle.GetIndex()];
        }

        void QueryPool::ShutdownInternal()
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_queriesMutex);
            m_queries.clear();
        }

        void QueryPool::ShutdownResourceInternal(Resource& resource)
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_queriesMutex);
            auto& query = static_cast<Query&>(resource);
            m_queries[query.m_handle.GetIndex()] = nullptr;
            m_queryAllocator.DeAllocate(query.m_handle.GetIndex());
        }

        AZStd::vector<Query*> QueryPool::GetQueries()
        {
            AZStd::vector<Query*> queries;
            AZStd::unique_lock<AZStd::mutex> lock(m_queriesMutex);
            AZStd::for_each(m_queries.begin(), m_queries.end(), [&queries](Query* query)
            {
                if (query)
                {
                    queries.push_back(query);
                }
            });
            return queries;
        }
    }
}
