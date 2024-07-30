/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/Query.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/Scope.h>

#include <Atom/RPI.Public/GpuQuery/GpuQuerySystem.h>
#include <Atom/RPI.Public/GpuQuery/QueryPool.h>

namespace AZ
{
    namespace RPI
    {
        static const char* GetQueryTypeString(RHI::QueryType queryType)
        {
            switch (queryType)
            {
            case RHI::QueryType::Occlusion:
            {
                return "Occlusion";
            }
            case RHI::QueryType::Timestamp:
            {
                return "Timestamp";
            }
            case RHI::QueryType::PipelineStatistics:
            {
                return "PipelineStatistics";
            }
            default:
            {
                AZ_Assert(false, "Unknown QueryType supplied");
                return "UnknownQueryType";
            }
            };
        }
        QueryPoolPtr QueryPool::CreateQueryPool(uint32_t queryCount, uint32_t rhiQueriesPerResult, RHI::QueryType queryType, RHI::PipelineStatisticsFlags pipelineStatisticsFlags)
        {
            return AZStd::unique_ptr<QueryPool>(aznew QueryPool(queryCount, rhiQueriesPerResult, queryType, pipelineStatisticsFlags));
        }

        QueryPool::QueryPool(uint32_t queryCapacity, uint32_t queriesPerResult, RHI::QueryType queryType, RHI::PipelineStatisticsFlags statisticsFlags)
        {
            m_queryCapacity = queryCapacity;
            m_queriesPerResult = queriesPerResult;
            m_statisticsFlags = statisticsFlags;
            m_queryType = queryType;

            // Calculate the total amount of RHI queries the RPI QueryPool needs to initialize.
            m_rhiQueryCapacity = m_queryCapacity * m_queriesPerResult * RPI::Query::BufferedFrames;
            m_rhiQueryArray.resize(m_rhiQueryCapacity);

            m_availableIntervalArray.reserve(queryCapacity);

            // Calculate the query result size.
            CalculateResultSize();

            // Populate the array with available RHI Query intervals.
            CreateRhiQueryIntervals();

            // Setup the query pool.
            {
                RHI::QueryPoolDescriptor queryPoolDesc;
                queryPoolDesc.m_queriesCount = m_rhiQueryCapacity;
                queryPoolDesc.m_type = m_queryType;
                queryPoolDesc.m_pipelineStatisticsMask = m_statisticsFlags;

                m_rhiQueryPool = aznew RHI::QueryPool();
                AZStd::string poolName = AZStd::string::format("%sQueryPool", GetQueryTypeString(queryType));
                m_rhiQueryPool->SetName(AZ::Name(poolName));
                [[maybe_unused]] auto result = m_rhiQueryPool->Init(queryPoolDesc);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to create the query pool");
            }

            // Create the RHI queries.
            {
                AZStd::vector<RHI::Query*> rawQueryArray;
                rawQueryArray.reserve(m_rhiQueryArray.size());

                for (RHI::Ptr<RHI::Query>& query : m_rhiQueryArray)
                {
                    query = aznew RHI::Query();
                    rawQueryArray.emplace_back(query.get());
                }

                m_rhiQueryPool->InitQuery(rawQueryArray.data(), static_cast<uint32_t>(rawQueryArray.size()));
            }
        }

        QueryPool::~QueryPool()
        {
            // Unregister the queries first
            for (auto& query : m_queryRegistry)
            {
                query->UnregisterFromPool();
            }
            AZ_Assert(m_queryRegistry.empty(), "The QueryRegistry should be empty.");

            m_availableIntervalArray.clear();
            m_rhiQueryArray.clear();
            m_rhiQueryPool = nullptr;
        }

        void QueryPool::Update()
        {
            // Increment the QueryPool's FrameIndex.
            m_poolFrameIndex++;
        }

        RHI::Ptr<RPI::Query> QueryPool::CreateQuery(RHI::QueryPoolScopeAttachmentType attachmentType, RHI::ScopeAttachmentAccess attachmentAccess)
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_queryRegistryMutex);

            // Get an available RHI Query interval.
            if (m_availableIntervalArray.empty())
            {
                AZ_WarningOnce("Gpu QueryPool", false,
                    "There are no more available query indices left. This will result in Query data not being available for certain passes. \
                    Initialize the RPI::QueryPool with a bigger capacity.");
                return nullptr;
            }

            RHI::Interval rhiQueryIndices = m_availableIntervalArray.back();
            m_availableIntervalArray.pop_back();

            // Create the RPI Query, and add it to the registry.
            auto* query = aznew RPI::Query(this, rhiQueryIndices, m_queryType, attachmentType, attachmentAccess);
            m_queryRegistry.emplace(query);

            return query;
        }

        void QueryPool::UnregisterQuery(RPI::Query* query)
        {
            AZ_Assert(query, "The RPI::Query has to be valid");

            AZStd::unique_lock<AZStd::mutex> lock(m_queryRegistryMutex);

            // Push the RHI Query indices back into the array of available indices for reuse.
            m_availableIntervalArray.emplace_back(query->m_rhiQueryIndices);

            // Invalidate the RPI Query's QueryPool.
            query->m_queryPool = nullptr;

            // Remove the RPI Query from the registry.
            m_queryRegistry.erase(query);
        }

        RHI::ResultCode QueryPool::BeginQueryInternal(RHI::Interval rhiQueryIndices, const RHI::FrameGraphExecuteContext& context)
        {
            auto rhiQueryArray = GetRhiQueryArray();
            RHI::Ptr<RHI::Query> beginQuery = rhiQueryArray[rhiQueryIndices.m_min];

            return beginQuery->GetDeviceQuery(context.GetDeviceIndex())->Begin(*context.GetCommandList());
        }

        RHI::ResultCode QueryPool::EndQueryInternal(RHI::Interval rhiQueryIndices, const RHI::FrameGraphExecuteContext& context)
        {
            auto rhiQueryArray = GetRhiQueryArray();
            RHI::Ptr<RHI::Query> endQuery = rhiQueryArray[rhiQueryIndices.m_max];

            return endQuery->GetDeviceQuery(context.GetDeviceIndex())->End(*context.GetCommandList());
        }

        AZStd::span<const RHI::Ptr<RHI::Query>> RPI::QueryPool::GetRhiQueryArray() const
        {
            return m_rhiQueryArray;
        }

        QueryResultCode QueryPool::GetQueryResultFromIndices(uint64_t* result, RHI::Interval rhiQueryIndices, RHI::QueryResultFlagBits queryResultFlag, int deviceIndex)
        {
            // Get the raw RHI Query pointers.
            AZStd::vector<RHI::DeviceQuery*> queryArray = GetRawRhiQueriesFromInterval(rhiQueryIndices, deviceIndex);

            // RHI Query results are readback with values that are a multiple of uint64_t.
            const uint32_t resultCount = m_queryResultSize / sizeof(uint64_t);
            const RHI::ResultCode resultCode = m_rhiQueryPool->GetDeviceQueryPool(deviceIndex)->GetResults(queryArray.data(), m_queriesPerResult, result, resultCount, queryResultFlag);

            return resultCode == RHI::ResultCode::Success ? QueryResultCode::Success : QueryResultCode::Fail;
        }

        uint32_t QueryPool::GetQueryResultSize() const
        {
            return m_queryResultSize;
        }

        void QueryPool::CalculateResultSize()
        {
            using namespace RHI;

            // Query result element count per QueryType.
            const uint32_t TimestampResultCount = 2u;
            const uint32_t OcclusionResultCount = 1u;

            uint32_t resultCount = 0u;

            // Determine the result size in uint64 by the QueryType.
            switch (m_queryType)
            {
            case QueryType::PipelineStatistics:
                // Each bit set, is translated to an additional result.
                resultCount = CountBitsSet(static_cast<uint64_t>(m_statisticsFlags));
                break;
            case QueryType::Timestamp:
                // A single timestamp result consists of two values.
                resultCount = TimestampResultCount;
                break;
            case QueryType::Occlusion:
                // A single occlusion result consists of one value.
                resultCount = OcclusionResultCount;
                break;
            default:
                AZ_Assert(false, "Unsupported QueryType");
                break;
            }

            m_queryResultSize = resultCount * sizeof(uint64_t);
        }

        void QueryPool::CreateRhiQueryIntervals()
        {
            // Calculates the RHI Query indices that are associated with the RPI Query.
            const auto getRhiQuriesFromRpiQueryIndex = [this](uint32_t rpiQueryIndex)
            {
                // The amount of RHI Queries that are required for a single RPI Query.
                const uint32_t queryIntervalSize = m_queriesPerResult * RPI::Query::BufferedFrames;
                const uint32_t queryIntervalOffset = rpiQueryIndex * queryIntervalSize;

                return RHI::Interval(queryIntervalOffset, queryIntervalOffset + queryIntervalSize - 1u);
            };

            for (uint32_t i = 0u; i < m_queryCapacity; i++)
            {
                m_availableIntervalArray.emplace_back(getRhiQuriesFromRpiQueryIndex(i));
            }
        }

        uint64_t QueryPool::GetPoolFrameIndex() const
        {
            return m_poolFrameIndex;
        }

        uint32_t QueryPool::GetQueriesPerResult() const
        {
            return m_queriesPerResult;
        }

        AZStd::span<const RHI::Ptr<RHI::Query>> QueryPool::GetRhiQueriesFromInterval(const RHI::Interval& rhiQueryIndices) const
        {
            const uint32_t queryCount = rhiQueryIndices.m_max - rhiQueryIndices.m_min + 1u;
            AZ_Assert(rhiQueryIndices.m_max < m_rhiQueryCapacity, "Query array index is going over the limit");

            return AZStd::span<const RHI::Ptr<RHI::Query>>(m_rhiQueryArray.begin() + rhiQueryIndices.m_min, queryCount);
        }

        AZStd::vector<RHI::DeviceQuery*> QueryPool::GetRawRhiQueriesFromInterval(const RHI::Interval& rhiQueryIndices, int deviceIndex) const
        {
            auto rhiQueries = GetRhiQueriesFromInterval(rhiQueryIndices);

            AZStd::vector<RHI::DeviceQuery*> queryArray;
            queryArray.reserve(rhiQueries.size());
            for (RHI::Ptr<RHI::Query> rhiQuery : rhiQueries)
            {
                queryArray.emplace_back(rhiQuery->GetDeviceQuery(deviceIndex).get());
            }

            return queryArray;
        }

    };  // Namespace RPI
};  // Namespace AZ
