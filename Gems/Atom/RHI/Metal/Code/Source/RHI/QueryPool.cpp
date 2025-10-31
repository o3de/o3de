/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<QueryPool> QueryPool::Create()
        {
            return aznew QueryPool();
        }

        Device& QueryPool::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }
    
        RHI::ResultCode QueryPool::InitInternal(RHI::Device& baseDevice, const RHI::QueryPoolDescriptor& descriptor)
        {
            auto& device = static_cast<Device&>(baseDevice);

#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
            id<MTLDevice> mtlDevice = device.GetMtlDevice();
            NSArray<id<MTLCounterSet>> * counterSets = [mtlDevice counterSets];
            CacheCounterIndices(counterSets);
#endif
            
            switch(descriptor.m_type)
            {
                case RHI::QueryType::Occlusion:
                {
                    RHI::BufferDescriptor bufferDescriptor;
                    bufferDescriptor.m_byteCount = descriptor.m_queriesCount * SizeInBytesPerQuery;
                    bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::Constant;
                    m_visibilityResultBuffer = device.CreateBufferCommitted(bufferDescriptor, RHI::HeapMemoryLevel::Host);
                    break;
                }
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
                case RHI::QueryType::Timestamp:
                {
                    NSUInteger timeStampCounterIndex = [counterSets indexOfObjectPassingTest:^BOOL(id<MTLCounterSet> mtlCounterSet, NSUInteger idx, BOOL *stop)
                    {
                        if ([mtlCounterSet.name isEqualToString:MTLCommonCounterSetTimestamp])
                        {
                            *stop = YES;
                            return YES;
                        }
                        return NO;
                    }];
                    
                    if (timeStampCounterIndex != NSNotFound)
                    {
                        NSError* error = nil;
                        MTLCounterSampleBufferDescriptor* counterSampleBufferDescriptor = [[[MTLCounterSampleBufferDescriptor alloc] init] autorelease];
                        counterSampleBufferDescriptor.label = @"TimeStampCounterSampleBuffer";
                        counterSampleBufferDescriptor.counterSet = counterSets[timeStampCounterIndex];
                        counterSampleBufferDescriptor.sampleCount = descriptor.m_queriesCount;
                        counterSampleBufferDescriptor.storageMode = MTLStorageModeShared;
                        m_timeStampCounterSamplerBuffer = [mtlDevice newCounterSampleBufferWithDescriptor:counterSampleBufferDescriptor
                                                                                                    error:&error];
                        AZ_Assert(!error, "Unable to create samplingBuffer for timestamps");
                    }
                    else
                    {
                        AZ_Assert(false, "TimeStamp related counterset not found");
                    }
                    break;
                }
                case RHI::QueryType::PipelineStatistics:
                {
                    NSError* error = nil;
                    NSUInteger statisticCounterIndex = [counterSets indexOfObjectPassingTest:^BOOL(id<MTLCounterSet> mtlCounterSet, NSUInteger idx, BOOL *stop)
                    {
                        if ([mtlCounterSet.name isEqualToString:MTLCommonCounterSetStatistic])
                        {
                            *stop = YES;
                            return YES;
                        }
                        return NO;
                    }];
                    
                    if (statisticCounterIndex != NSNotFound)
                    {
                        MTLCounterSampleBufferDescriptor* counterSampleBufferDescriptor = [[[MTLCounterSampleBufferDescriptor alloc] init] autorelease];
                        counterSampleBufferDescriptor.label = @"StatisticCounterSamplerBuffer";
                        counterSampleBufferDescriptor.counterSet = counterSets[statisticCounterIndex];
                        counterSampleBufferDescriptor.sampleCount = descriptor.m_queriesCount;
                        counterSampleBufferDescriptor.storageMode = MTLStorageModeShared;
                        m_statisticsCounterSamplerBufferBegin = [mtlDevice newCounterSampleBufferWithDescriptor:counterSampleBufferDescriptor
                                                                                                    error:&error];
                        AZ_Assert(!error, "Unable to create samplingBuffer for pipeline counters");
                        
                        error = 0;
                        m_statisticsCounterSamplerBufferEnd = [mtlDevice newCounterSampleBufferWithDescriptor:counterSampleBufferDescriptor
                                                                                                        error:&error];
                        AZ_Assert(!error, "Unable to create samplingBuffer for pipeline counters");
                    }
                    else
                    {
                        AZ_Assert(false, "PipelineStatistic related counterset not found");
                    }
                    break;
                }
#endif
                default:
                {
                    AZ_Assert(false, "Query type not supported");
                }
            }
            
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode QueryPool::InitQueryInternal(RHI::DeviceQuery& query)
        {
            // Nothing to do
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode QueryPool::GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, RHI::QueryResultFlagBits flags)
        {
            switch(GetDescriptor().m_type)
            {
                case RHI::QueryType::Occlusion:
                {
                    if (RHI::CheckBitsAll(flags, RHI::QueryResultFlagBits::Wait))
                    {
                        //Ensure that the command buffer associated with all the queries are submitted
                        AZStd::unique_lock<AZStd::mutex> lock(m_commandBufferMutex);
                        m_commandBufferCondition.wait(lock, [&]()
                        {
                            bool isCbCommited = true;
                            for (uint32_t i = startIndex; i < startIndex + queryCount; ++i)
                            {
                                auto* query = static_cast<Query*>(GetQuery(RHI::QueryHandle(i)));
                                AZ_Assert(query, "Null Query");
                                isCbCommited &= query->IsCommandBufferCompleted();
                            }
                            return isCbCommited;
                         });
                    }
                    
                    uint8_t* visibilityResultBuffer = static_cast<uint8_t*>(m_visibilityResultBuffer.GetCpuAddress());
                    void* visibilityResultAddress =  visibilityResultBuffer + (startIndex * SizeInBytesPerQuery);
                    memcpy(results, visibilityResultAddress, SizeInBytesPerQuery * queryCount);
                    break;
                }

                case RHI::QueryType::Timestamp:
                {
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
                    NSRange readRange = { startIndex , queryCount };
                    NSData * counterData = [m_timeStampCounterSamplerBuffer resolveCounterRange:readRange];
                    AZ_Assert(counterData, "Resolved samples not present");
                    if(counterData)
                    {
                        AZ_Assert(counterData.length == SizeInBytesPerQuery * queryCount, "Ensure that the resolved data is of corrent length");
                        memcpy(results, [counterData bytes], counterData.length);
                    }
#endif
                    break;
                }
                case RHI::QueryType::PipelineStatistics:
                {
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
                    MTLCounterResultStatistic statisticsDataBegin;
                    MTLCounterResultStatistic statisticsDataEnd;

                    auto mask = GetDescriptor().m_pipelineStatisticsMask;
                    NSRange readRange = { startIndex , queryCount };
                    NSData * counterDataBegin = [m_statisticsCounterSamplerBufferBegin resolveCounterRange:readRange];
                    NSData * counterDataEnd = [m_statisticsCounterSamplerBufferEnd resolveCounterRange:readRange];
                    if(counterDataBegin && counterDataEnd)
                    {
                        uint32_t resultPos = 0;
                        for (uint32_t queryIndex = 0; queryIndex < queryCount; ++queryIndex)
                        {
                            FillStatisticsData(static_cast<const uint64_t*>([counterDataBegin bytes]), queryIndex, statisticsDataBegin);
                            FillStatisticsData(static_cast<const uint64_t*>([counterDataEnd bytes]), queryIndex, statisticsDataEnd);
                            
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::IAVertices))
                            {
                                results[resultPos++] = 0;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::IAPrimitives))
                            {
                                results[resultPos++] = 0;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::VSInvocations))
                            {
                                results[resultPos++] = statisticsDataEnd.vertexInvocations - statisticsDataBegin.vertexInvocations;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::GSInvocations))
                            {
                                results[resultPos++] = 0;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::GSPrimitives))
                            {
                                results[resultPos++] = 0;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CInvocations))
                            {
                               results[resultPos++] = statisticsDataEnd.clipperInvocations - statisticsDataBegin.clipperInvocations;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CPrimitives))
                            {
                                results[resultPos++] = statisticsDataEnd.clipperPrimitivesOut - statisticsDataBegin.clipperPrimitivesOut;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::PSInvocations))
                            {
                                results[resultPos++] = statisticsDataEnd.fragmentInvocations - statisticsDataBegin.fragmentInvocations;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::HSInvocations))
                            {
                                results[resultPos++] = 0;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::DSInvocations))
                            {
                                results[resultPos++] = 0;
                            }
                            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CSInvocations))
                            {
                                results[resultPos++] = statisticsDataEnd.computeKernelInvocations - statisticsDataBegin.computeKernelInvocations;
                            }
                        }
                    }
#endif
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Incorrect query type");
                }
            }
            return RHI::ResultCode::Success;
        }

       
        
        const id<MTLBuffer> QueryPool::GetVisibilityBuffer() const
        {
            return m_visibilityResultBuffer.GetGpuAddress<id<MTLBuffer>>();
        }
 
        void QueryPool::NotifyCommandBufferCommit()
        {
            m_commandBufferCondition.notify_all();
        }
    
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
        const id<MTLCounterSampleBuffer> QueryPool::GetTimeStampCounterSamplerBuffer() const
        {
            return m_timeStampCounterSamplerBuffer;
        }
    
        const id<MTLCounterSampleBuffer> QueryPool::GetStatisticsCounterSamplerBufferBegin() const
        {
            return m_statisticsCounterSamplerBufferBegin;
        }
    
        const id<MTLCounterSampleBuffer> QueryPool::GetStatisticsCounterSamplerBufferEnd() const
        {
            return m_statisticsCounterSamplerBufferEnd;
        }

        void QueryPool::FillStatisticsData(const uint64_t* resolvedSamples, uint32_t queryIndex, MTLCounterResultStatistic& counterStatistic)
        {            
            uint32_t sampleIndex = queryIndex * sizeof(MTLCounterResultStatistic);
            counterStatistic.vertexInvocations = resolvedSamples[sampleIndex + m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::VSInvocations]];
            counterStatistic.clipperInvocations = resolvedSamples[sampleIndex + m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::CInvocations]];
            counterStatistic.clipperPrimitivesOut = resolvedSamples[sampleIndex + m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::CPrimitives]];
            counterStatistic.fragmentInvocations = resolvedSamples[sampleIndex +  m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::PSInvocations]];
            counterStatistic.computeKernelInvocations = resolvedSamples[sampleIndex + m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::CSInvocations]];
        }
    
        void QueryPool::CacheCounterIndices(NSArray<id<MTLCounterSet>> * mtlCounterSets)
        {
            uint32_t counterIndex = 0;
            for(id<MTLCounterSet> mtlCounterSet in mtlCounterSets)
            {
                NSArray<id<MTLCounter>>* mtlCounters = mtlCounterSet.counters;
                counterIndex = 0;
                for(id<MTLCounter> mtlCounter in mtlCounters)
                {
                    if ([mtlCounter.name isEqualToString:MTLCommonCounterVertexInvocations])
                    {
                        m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::VSInvocations] = counterIndex;
                    }
                    else if ([mtlCounter.name isEqualToString:MTLCommonCounterFragmentInvocations])
                    {
                        m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::PSInvocations] = counterIndex;
                    }
                    else if ([mtlCounter.name isEqualToString:MTLCommonCounterComputeKernelInvocations])
                    {
                        m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::CSInvocations] = counterIndex;
                    }
                    else if ([mtlCounter.name isEqualToString:MTLCommonCounterClipperInvocations])
                    {
                        m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::CInvocations] = counterIndex;
                    }
                    else if ([mtlCounter.name isEqualToString:MTLCommonCounterClipperPrimitivesOut])
                    {
                        m_pipelineFlagToCounterIndex[RHI::PipelineStatisticsFlags::CPrimitives] = counterIndex;
                    }
                    
                    counterIndex++;
                }
            }
        }
#endif
    }
}
