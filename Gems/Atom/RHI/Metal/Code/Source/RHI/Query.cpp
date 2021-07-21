/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Metal_precompiled.h"
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<Query> Query::Create()
        {
            return aznew Query();
        }
    
        Device& Query::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        RHI::ResultCode Query::BeginInternal(RHI::CommandList& baseCommandList, RHI::QueryControlFlags flags)
        {
            auto& commandList = static_cast<CommandList&>(baseCommandList);
            auto* queryPool = static_cast<QueryPool*>(GetQueryPool());
            m_currentControlFlags = flags;
            
            switch(queryPool->GetDescriptor().m_type)
            {
                case RHI::QueryType::Occlusion:
                {
                    size_t queryOffset = GetHandle().GetIndex() * SizeInBytesPerQuery;
                    commandList.SetVisibilityResultMode(ConvertVisibilityResult(m_currentControlFlags), queryOffset);
                    
                    [commandList.GetMtlCommandBuffer() addCompletedHandler:^(id<MTLCommandBuffer> buffer)
                     {
                        m_commandBufferCompleted = true;
                        queryPool->NotifyCommandBufferCommit();
                     }];
                    break;
                }
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
                case RHI::QueryType::PipelineStatistics:
                {
                    commandList.SampleCounters(queryPool->GetStatisticsCounterSamplerBufferBegin(), GetHandle().GetIndex());
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

        RHI::ResultCode Query::EndInternal(RHI::CommandList& baseCommandList)
        {
            auto& commandList = static_cast<CommandList&>(baseCommandList);
            auto* queryPool = static_cast<QueryPool*>(GetQueryPool());
            
            switch(queryPool->GetDescriptor().m_type)
            {
                case RHI::QueryType::Occlusion:
                {
                    size_t queryOffset = GetHandle().GetIndex() * SizeInBytesPerQuery;
                    commandList.SetVisibilityResultMode(MTLVisibilityResultModeDisabled, queryOffset);
                    break;
                }
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
                case RHI::QueryType::PipelineStatistics:
                {
                    commandList.SampleCounters(queryPool->GetStatisticsCounterSamplerBufferEnd(), GetHandle().GetIndex());
                    break;
                }
#endif
                default:
                {
                    AZ_Assert(false, "Query type not supported");
                }
            }
            m_currentControlFlags = RHI::QueryControlFlags::None;
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Query::WriteTimestampInternal(RHI::CommandList& baseCommandList)
        {
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
            auto& commandList = static_cast<CommandList&>(baseCommandList);
            auto* queryPool = static_cast<QueryPool*>(GetQueryPool());
            commandList.SamplePassCounters(queryPool->GetTimeStampCounterSamplerBuffer(), GetHandle().GetIndex());
#endif
            return RHI::ResultCode::Success;
        }
    
        const bool Query::IsCommandBufferCompleted() const
        {
            return m_commandBufferCompleted;
            
        }
    }
}
