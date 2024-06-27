/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceQueryPool.h>
#include <AzCore/std/parallel/conditional_variable.h>

namespace AZ
{
    namespace Metal
    {
        class Query;

        
        class QueryPool final
            : public RHI::DeviceQueryPool
        {
            using Base = RHI::DeviceQueryPool;
        public:
            AZ_RTTI(QueryPool, "{0C03DF09-F4F4-45FB-BE90-4779E44CD4D6}", Base);
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator);
            virtual ~QueryPool() = default;

            static RHI::Ptr<QueryPool> Create();
            Device& GetDevice() const;
           
            const id<MTLBuffer> GetVisibilityBuffer() const;
            void NotifyCommandBufferCommit();
            
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
            const id<MTLCounterSampleBuffer> GetTimeStampCounterSamplerBuffer() const;
            const id<MTLCounterSampleBuffer> GetStatisticsCounterSamplerBufferBegin() const;
            const id<MTLCounterSampleBuffer> GetStatisticsCounterSamplerBufferEnd() const;
#endif
            
        private:
            QueryPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceQueryPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::QueryPoolDescriptor& descriptor) override;
            RHI::ResultCode InitQueryInternal(RHI::DeviceQuery& query) override;
            RHI::ResultCode GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, RHI::QueryResultFlagBits flags) override;
            //////////////////////////////////////////////////////////////////////////
            
            //! Occlusion related data
            MemoryView m_visibilityResultBuffer;
            AZStd::mutex m_commandBufferMutex;
            AZStd::condition_variable m_commandBufferCondition;
            
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
            id<MTLCounterSampleBuffer> m_timeStampCounterSamplerBuffer = nil;
            id<MTLCounterSampleBuffer> m_statisticsCounterSamplerBufferBegin = nil;
            id<MTLCounterSampleBuffer> m_statisticsCounterSamplerBufferEnd = nil;
            AZStd::unordered_map<RHI::PipelineStatisticsFlags, uint32_t> m_pipelineFlagToCounterIndex;
            void CacheCounterIndices(NSArray<id<MTLCounterSet>> * mtlCounterSets);
            void FillStatisticsData(const uint64_t* resolvedSamples, uint32_t queryIndex, MTLCounterResultStatistic& counterStatistic);
#endif            
        };
    }
}
