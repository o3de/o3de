/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/GpuQuery/GpuQuerySystemInterface.h>
#include <Atom/RHI.Reflect/Interval.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/ScopeId.h>

#include <Atom/RHI/FrameGraphInterface.h>

#include <AzCore/std/smart_ptr/intrusive_refcount.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraphExecuteContext;
    };

    namespace RPI
    {
        class QueryPool;
        class Query;

        //! The RPI Query references multiple RHI Queries by their indices. The amount of referenced RHI Queries depends on the maximum number of GPU frames
        //! that can be buffered before the CPU will stall, and the RPI Query's type (RHI Query indices count = QueryType * Maximum buffered GPU frames).

        //! The RHI Query indices are divided into multiple SubQueries. Each SubQuery is responsible to readback one or more RHI Queries to
        //! calculate a single result, of one frame. The number of RHI Queries referenced by a SubQuery depends on the type of
        //! the RPI Query (e.g Timestamps require two RHI Queries for a single result, while PipelineStatistics only require one RHI
        //! Query for a single result).
        //! When RPI Queries are being recorded (i.e BeginQuery() and EndQuery()), it takes at least one frame for the results of the RPI Query to
        //! be available for readback. The RPI Query utilizes the CPU stall that occurs when the amount of buffered frames reaches its limit to
        //! ensure that the the RHI Queries that were submitted RHI::Limits::Device::FrameCountMax ago are ready for readback (e.g if an RPI Query is used
        //! to record a Timestamp on frame 3, the CPU stall will ensure that the result of the Timestamp will be available on frame 6).
        //! The RPI Query will instantiate multiple SubQueries, one for each buffered frame. The SubQuery that is used for recording, will cache the
        //! FrameIndex in which it was used for recording.
        class ATOM_RPI_PUBLIC_API Query :
            public AZStd::intrusive_base
        {
            friend class QueryPool;

            //! The RHI Query indices get divided into SubQueries. Each SubQuery contains the amount of RHI Query indices necessary 
            //! to calculate a single result from the data that is readback from the RHI Queries.
            struct SubQuery
            {
                static constexpr uint64_t InvalidFrameIndex = static_cast<uint64_t>(-1);

                //! Cache the FrameIndex when this instance was created. The index will be used to identify the lifetime of the SubQuery in frames.
                uint64_t m_poolFrameIndex = InvalidFrameIndex;
                //! The indices of the RHI Queries. The interval stores the first and the last index of the RHI query.
                AZ::RHI::Interval m_rhiQueryIndices;
            };

            // The amount of delay in frames before it reads the data, currently set to 4, which uses the
            // stalling of the triple buffering system to ensure that the last query (4 frames ago), is executed,
            // and that the result is available for polling.
            static constexpr uint32_t BufferedFrames = RHI::Limits::Device::FrameCountMax + 1u;
            static constexpr uint32_t InvalidQueryIndex = static_cast<uint32_t>(-1);

        public:
            AZ_TYPE_INFO(Query, "{DC956F7F-5C9C-40FC-9200-D8C75E238135}");
            AZ_CLASS_ALLOCATOR(Query, AZ::SystemAllocator);

            Query(RPI::QueryPool* queryPool, RHI::Interval rhiQueryIndices, RHI::QueryType queryType, RHI::QueryPoolScopeAttachmentType attachmentType, RHI::ScopeAttachmentAccess attachmentAccess);
            ~Query();

            Query() = delete;
            Query(Query&& other) = delete;
            Query(const Query& other) = delete;
            Query& operator=(const Query& other) = delete;

            //! Returns the QueryType.
            RHI::QueryType GetQueryType() const;

            //! Adds the RHI Query to the scope builder.
            QueryResultCode AddToFrameGraph(RHI::FrameGraphInterface frameGraph);
            //! Begins the RHI Query for recording.
            QueryResultCode BeginQuery(const RHI::FrameGraphExecuteContext& context);
            //! Ends the RHI Query for recording.
            QueryResultCode EndQuery(const RHI::FrameGraphExecuteContext& context);

            //! Returns the earliest possible query result without stalling the thread. Result might be a few frames old.
            //! Note: When trying to retrieve query results without any queries being ready for readback, the system will return
            //! QueryCode::Fail.
            //! @param[out] queryResult The user provided structure where the data is copied to.
            //! @param[in] resultSizeInBytes The size of the data in bytes.
            QueryResultCode GetLatestResult(void* queryResult, uint32_t resultSizeInBytes, int deviceIndex);

            //! Returns the earliest possible query result without stalling the thread. Result might be a few frames old.
            //! Note: When trying to retrieve query results without any queries being ready for readback, the system will return
            //! QueryCode::Fail.
            //! @param[in/out] queryResult The user provided structure where the data is copied to.
            template<typename T>
            QueryResultCode GetLatestResult(T& queryResult, int deviceIndex)
            {
                void* resultData = static_cast<void*>(&queryResult);
                return GetLatestResult(resultData, sizeof(T), deviceIndex);
            }

            //! Returns the result of the earliest possible query. It might stall the calling thread, depending if the query result is available for polling
            //! Note1: When trying to retrieve query results without any queries being ready for readback, the system will return
            //! QueryCode::Fail.
            //! Note2: If this call stalls, it's possible to continue to use the query instance on a separate thread, but it's the user's responsibility
            //! to make sure the pool isn't deleted while the thread is stalling.
            //! @param[out] queryResult The user provided pointer where the data is copied to.
            //! @param[in] resultSizeInBytes The size of the data in bytes.
            QueryResultCode GetLatestResultAndWait(void* queryResult, uint32_t resultSizeInBytes, int deviceIndex);

            //! Returns the result of the earliest possible query. It might stall the calling thread, depending if the query result is available for polling
            //! Note1: When trying to retrieve query results without any queries being ready for readback, the system will return
            //! QueryCode::Fail.
            //! Note2: If this call stalls, it's possible to continue to use the query instance on a separate thread, but it's the user's responsibility
            //! to make sure the pool isn't deleted while the thread is stalling.
            //! @param[in/out] queryResult The user provided pointer where the data is copied to.
            template<typename T>
            QueryResultCode GetLatestResultAndWait(T& queryResult, int deviceIndex)
            {
                void* resultData = static_cast<void*>(&queryResult);
                return GetLatestResultAndWait(resultData, sizeof(T), deviceIndex);
            }

            //! Removes the reference of this instance in the RPI QueryPool where it was created.
            void UnregisterFromPool();

        private:
            // Assigns a new FrameIndex to an available SubQuery, effectively reusing it. This index will be used
            // for the forthcoming AddToFrameGraph(), BeginQuery() and EndQuery().
            bool AssignNewFrameIndexToSubQuery(uint64_t poolFrameIndex);

            // Further Subdivide the RHI Query indices into smaller intervals, one for each SubQuery.
            void SubdivideRhiQueryIndices(RHI::Interval rhiQueryIndices);

            template<typename T>
            uint32_t ReturnSubQueryArrayIndex(T&& comp, uint64_t initialCachedDelta, bool returnOnInvalidIndex) const;
            // Returns the array index of the SubQuery with the shortest lifetime, with a minimum threshold.
            uint32_t GetMostRecentSubQueryArrayIndex(uint64_t threshold) const;
            // Returns the array index of the SubQuery with the longest lifetime or one that is available.
            uint32_t GetOldestOrAvailableSubQueryArrayIndex() const;

            // Gets the RHI Query indices from the SubQuery associated with the current frame.
            AZStd::optional<RHI::Interval> GetRhiQueryIndicesFromCurrentFrame() const;

            RHI::QueryPoolScopeAttachmentType m_attachmentType = RHI::QueryPoolScopeAttachmentType::Global;
            RHI::ScopeAttachmentAccess m_attachmentAccess = RHI::ScopeAttachmentAccess::Read;
            RHI::QueryType m_queryType = RHI::QueryType::Invalid;

            // Cache the index of the most recent added SubQuery.
            uint32_t m_cachedSubQueryArrayIndex = InvalidQueryIndex;

            // Array of SubQueries.
            AZStd::array<SubQuery, BufferedFrames> m_subQueryArray;

            // Cache the RHI Query indices that is passed to the RPI Query instance, prior to further subdividing it.
            RHI::Interval m_rhiQueryIndices;

            // Cache the ScopeId that is passed with BeginQuery(). Used to verify that the provided contexts are the same for BeginQuery()
            // and EndQuery().
            RHI::ScopeId m_cachedScopeId;

            // Pointer to the parent RPI QueryPool that created this RPI Query.
            RPI::QueryPool* m_queryPool = nullptr;
        };

    }; // namespace RPI
}; // namespace AZ
