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
#include <Atom/RHI/Scope.h>

#include <Atom/RPI.Public/GpuQuery/GpuQuerySystem.h>
#include <Atom/RPI.Public/GpuQuery/Query.h>
#include <Atom/RPI.Public/GpuQuery/QueryPool.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    namespace RPI
    {
        Query::Query(RPI::QueryPool* queryPool, RHI::Interval rhiQueryIndices, RHI::QueryType queryType, RHI::QueryPoolScopeAttachmentType attachmentType, RHI::ScopeAttachmentAccess attachmentAccess)
        {
            if (!queryPool)
            {
                AZ_Error("RPI::Query", false, "Query must be initialized with valid queryPool");
                return;
            }

            m_queryPool = queryPool;

            m_rhiQueryIndices = rhiQueryIndices;
            m_attachmentType = attachmentType;
            m_attachmentAccess = attachmentAccess;
            m_queryType = queryType;

            // Split the queryIndices into multiple intervals. One for each buffered frame.
            SubdivideRhiQueryIndices(rhiQueryIndices);
        }

        Query::~Query()
        {
            UnregisterFromPool();
        }

        void Query::UnregisterFromPool()
        {
            if (m_queryPool)
            {
                m_queryPool->UnregisterQuery(this);
            }
        }

        RHI::QueryType Query::GetQueryType() const
        {
            return m_queryType;
        }

        QueryResultCode Query::AddToFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
            // Assign the FrameIndex of the query pool.
            if (!AssignNewFrameIndexToSubQuery(m_queryPool->GetPoolFrameIndex()))
            {
                return QueryResultCode::Fail;
            }

            // Get the RHI Query indices from the current FrameIndex.
            const auto rhiQueryIndices = GetRhiQueryIndicesFromCurrentFrame();
            if (!rhiQueryIndices)
            {
                return QueryResultCode::Fail;
            }

            // Tell the FrameGraph which RHI QueryPool, and which RHI Queries need to be used.
            [[maybe_unused]] RHI::ResultCode resultCode = frameGraph.UseQueryPool(m_queryPool->m_rhiQueryPool, rhiQueryIndices.value(), m_attachmentType, m_attachmentAccess);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to add the queries to the scope builder");

            // Invalidate the ScopeId.
            m_cachedScopeId = RHI::ScopeId();

            return QueryResultCode::Success;
        }

        QueryResultCode Query::BeginQuery(const RHI::FrameGraphExecuteContext& context)
        {
            // Return fail if the query wasn't added to frame graph
            if (m_cachedSubQueryArrayIndex == InvalidQueryIndex)
            {
                return QueryResultCode::Fail;
            }

            const auto rhiQueryIndices = GetRhiQueryIndicesFromCurrentFrame();
            if (!rhiQueryIndices)
            {
                return QueryResultCode::Fail;
            }

            [[maybe_unused]] RHI::ResultCode resultCode = m_queryPool->BeginQueryInternal(rhiQueryIndices.value(), context);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to begin recording the query");

            m_cachedScopeId = context.GetScopeId();

            return QueryResultCode::Success;
        }

        QueryResultCode Query::EndQuery(const RHI::FrameGraphExecuteContext& context)
        {
            // return fail if the query wasn't added to frame graph
            if (m_cachedSubQueryArrayIndex == InvalidQueryIndex)
            {
                return QueryResultCode::Fail;
            }

            // Validate that the queries are recorded for the same scope.
            // Note: the timestamp query skips this check because its start and end may be added in randome order since they are added in different FrameGraphExecuteContext.
            // The order doesn't matter because timestamp's begin or end are just inserting a timestamp to the command list.
            // Also, the command list's execution order will still follow the order of start query and end query.
            if (m_cachedScopeId != context.GetScopeId() && GetQueryType() != RHI::QueryType::Timestamp)
            {
                AZ_Warning("RPI::Query", false,
                    "The FrameGraphExecuteContext's Scope that is used for RPI::Query::BeginQuery is not the same for RPI::Query::EndQuery.");
                return QueryResultCode::Fail;
            }

            const auto rhiQueryIndices = GetRhiQueryIndicesFromCurrentFrame();
            if (!rhiQueryIndices)
            {
                return QueryResultCode::Fail;
            }

            [[maybe_unused]] RHI::ResultCode resultCode = m_queryPool->EndQueryInternal(rhiQueryIndices.value(), context);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to end recording the query");

            return QueryResultCode::Success;
        }

        QueryResultCode Query::GetLatestResultAndWait(void* queryResult, uint32_t resultSizeInBytes, int deviceIndex)
        {
            if (resultSizeInBytes < m_queryPool->GetQueryResultSize())
            {
                AZ_Warning("RPI::Query", false, "Not enough space to copy the query result into the provided data container.");
                return QueryResultCode::Fail;
            }

            // Get the most recent query index that has been submitted at least 1 frame ago.
            const uint64_t frameOffset = 1u;
            const uint32_t recentSubQueryIndex = GetMostRecentSubQueryArrayIndex(frameOffset);
            if (recentSubQueryIndex == InvalidQueryIndex)
            {
                return QueryResultCode::Fail;
            }

            const SubQuery& recentSubQuery = m_subQueryArray[recentSubQueryIndex];

            // This may stall the calling thread; depending if the query result is already available for polling.
            return m_queryPool->GetQueryResultFromIndices(static_cast<uint64_t*>(queryResult), recentSubQuery.m_rhiQueryIndices, RHI::QueryResultFlagBits::Wait, deviceIndex);
        }

        QueryResultCode Query::GetLatestResult(void* queryResult, uint32_t resultSizeInBytes, int deviceIndex)
        {
            if (resultSizeInBytes < m_queryPool->GetQueryResultSize())
            {
                AZ_Warning("RPI::Query", false, "Not enough space to copy the query result into the provided data container.");
                return QueryResultCode::Fail;
            }

            // Get the most recent query index that has been submitted at least (BufferedFrames-1) frames ago.
            const uint32_t frameOffset = BufferedFrames - 1u;
            const uint32_t latestQueryIndex = GetMostRecentSubQueryArrayIndex(frameOffset);
            if (latestQueryIndex == InvalidQueryIndex)
            {
                return QueryResultCode::Fail;
            }

            SubQuery& subQuery = m_subQueryArray[latestQueryIndex];
            return m_queryPool->GetQueryResultFromIndices(static_cast<uint64_t*>(queryResult), subQuery.m_rhiQueryIndices, RHI::QueryResultFlagBits::None, deviceIndex);
        }

        bool Query::AssignNewFrameIndexToSubQuery(uint64_t poolFrameIndex)
        {
            if (m_cachedSubQueryArrayIndex != InvalidQueryIndex &&
                m_subQueryArray[m_cachedSubQueryArrayIndex].m_poolFrameIndex == poolFrameIndex)
            {
                // It might run multiple times if a pass has multiple scopes run on multiple devices
                return true;
            }

            // Get the oldest query array index.
            const uint32_t availableQueryIndex = GetOldestOrAvailableSubQueryArrayIndex();

            // Cache the index of the most recently added SubQuery.
            m_cachedSubQueryArrayIndex = availableQueryIndex;

            // Reuse the oldest query by setting the current FrameIndex.
            m_subQueryArray[availableQueryIndex].m_poolFrameIndex = poolFrameIndex;

            return true;
        }

        void Query::SubdivideRhiQueryIndices(RHI::Interval rhiQueryIndices)
        {
            // Calculate the amount of RHI Queries used for this RPI Query.
            const uint32_t queryIndicesCount = rhiQueryIndices.m_max - rhiQueryIndices.m_min + 1u;
            AZ_Assert((queryIndicesCount % m_queryPool->GetQueriesPerResult()) == 0u,
                "The amount of RHI::Query indices used for the RPI::Query is not a multiple of the number of RHI::Queries required to calculate a single result.");

            // Calculate the number of query groups.
            const uint32_t subQueryIndexCount = queryIndicesCount / m_queryPool->GetQueriesPerResult();
            AZ_Assert(subQueryIndexCount == BufferedFrames,
                "The amount of QueryGroups needs to be equal to the defined BufferedFrames");

            // Divide the RHI Query indices equally among the SubQueries.
            for (uint32_t queryGroupIndex = 0u; queryGroupIndex < subQueryIndexCount; queryGroupIndex++)
            {
                // Calculate the range of the interval.
                const uint32_t groupOffset = queryGroupIndex * m_queryPool->GetQueriesPerResult();
                const uint32_t min = groupOffset + rhiQueryIndices.m_min;
                const uint32_t max = groupOffset + rhiQueryIndices.m_min + m_queryPool->GetQueriesPerResult() - 1u;

                m_subQueryArray[queryGroupIndex].m_rhiQueryIndices = RHI::Interval(min, max);
            }
        }

        template<typename T>
        uint32_t Query::ReturnSubQueryArrayIndex(T&& comp, uint64_t initialCachedDelta, bool returnOnInvalidIndex) const
        {
            uint32_t cachedQueryIndex = InvalidQueryIndex;
            uint64_t cachedFrameDelta = initialCachedDelta;
            for (uint32_t i = 0u; i < m_subQueryArray.size(); i++)
            {
                const SubQuery& subQuery = m_subQueryArray[i];

                // Return or ignore the index of reused SubQueries.
                if (subQuery.m_poolFrameIndex == SubQuery::InvalidFrameIndex)
                {
                    if (returnOnInvalidIndex)
                    {
                        return i;
                    }
                    else
                    {
                        continue;
                    }
                }

                // Calculate the delta between the RPI QueryPool's FrameIndex and the SubQuery's cached FrameIndex.
                const uint64_t poolFrameIndex = m_queryPool->GetPoolFrameIndex();
                AZ_Assert(poolFrameIndex >= subQuery.m_poolFrameIndex, "The SubQuery's cached FrameIndex is older than the RPI QueryPool's FrameIndex");
                const uint64_t frameDelta = poolFrameIndex - subQuery.m_poolFrameIndex;

                if (comp(frameDelta, cachedFrameDelta))
                {
                    cachedQueryIndex = i;
                    cachedFrameDelta = frameDelta;
                }
            }

            return cachedQueryIndex;
        }

        uint32_t Query::GetMostRecentSubQueryArrayIndex(uint64_t threshold) const
        {
            return ReturnSubQueryArrayIndex([threshold](uint64_t frameDelta, uint64_t cachedFrameDelta)
            {
                return frameDelta < cachedFrameDelta && frameDelta >= threshold;
            }, SubQuery::InvalidFrameIndex, false);
        }

        uint32_t Query::GetOldestOrAvailableSubQueryArrayIndex() const
        {
            return ReturnSubQueryArrayIndex([](uint64_t frameDelta, uint64_t cachedFrameDelta)
            {
                return frameDelta > cachedFrameDelta;
            }, 0u, true);
        }

        AZStd::optional<RHI::Interval> Query::GetRhiQueryIndicesFromCurrentFrame() const
        {
            if (m_cachedSubQueryArrayIndex == InvalidQueryIndex)
            {
                return AZStd::nullopt;
            }

            const SubQuery& subQuery = m_subQueryArray[m_cachedSubQueryArrayIndex];
            const uint64_t poolFrameIndex = m_queryPool->GetPoolFrameIndex();

            if (poolFrameIndex != subQuery.m_poolFrameIndex)
            {
                AZ_Warning("RPI::Query", false, "FrameIndex doesn't match the one from the query. The recording of a query needs to happen within one frame");
                return AZStd::nullopt;
            }

            return subQuery.m_rhiQueryIndices;
        }

    };  // Namespace RPI
};  // Namespace AZ
