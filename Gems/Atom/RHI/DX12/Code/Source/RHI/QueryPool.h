/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceQueryPool.h>

namespace AZ
{
    namespace DX12
    {
        class Query;

        /**
        *   QueryPool implementation for DX12.
        *   It uses a ID3D12QueryHeap object as the backing native object.
        *   It uses an internal buffer to resolve queries into a buffer.
        */
        class QueryPool final
            : public RHI::DeviceQueryPool
        {
            using Base = RHI::DeviceQueryPool;
        public:
            AZ_RTTI(QueryPool, "{158BB61D-8867-4939-B6B3-07C6280AD5DC}", Base);
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator);
            virtual ~QueryPool() = default;

            static RHI::Ptr<QueryPool> Create();

            ID3D12QueryHeap* GetHeap() const;

            /// Notify that a query has ended and the results must be resolved.
            void OnQueryEnd(Query& query, D3D12_QUERY_TYPE type);

        private:
            QueryPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceQueryPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::QueryPoolDescriptor& descriptor) override;
            RHI::ResultCode InitQueryInternal(RHI::DeviceQuery& query) override;
            RHI::ResultCode GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, RHI::QueryResultFlagBits flags) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            size_t GetQueryResultSize() const;
            void CopyPipelineStatisticsResult(uint64_t* destination, D3D12_QUERY_DATA_PIPELINE_STATISTICS* source, uint32_t queryCount);
            uint8_t* MapReadBackBuffer(D3D12_RANGE readRange);
            void UnmapReadBackBuffer();

            RHI::Ptr<ID3D12QueryHeap> m_queryHeap;
            RHI::Ptr<ID3D12Resource> m_readBackBuffer; ///< Internal buffer used for resolving query results.

            static const uint32_t numQueryType = 8;
            mutable AZStd::mutex m_queriesToResolveMutex;
            AZStd::array<AZStd::vector<Query*>, numQueryType> m_queriesToResolve; ///< List of queries to resolve.
        };
    }
}
