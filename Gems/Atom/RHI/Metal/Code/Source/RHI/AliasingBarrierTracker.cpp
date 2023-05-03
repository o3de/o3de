/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/AliasingBarrierTracker.h>
#include <RHI/Device.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Metal
    {
        AliasingBarrierTracker::AliasingBarrierTracker(Device& device)
            : m_device(&device)
        {
        }
    
        void AliasingBarrierTracker::ResetInternal()
        {
            m_resourceFenceData.clear();
            
            //Reset the fence
            for (Fence& fence : m_resourceFences)
            {
                fence.Increment();
            }
        }
        
        void AliasingBarrierTracker::AppendBarrierInternal(
            const RHI::AliasedResource& resourceBefore,
            const RHI::AliasedResource& resourceAfter)
        {           
            m_resourceFenceData.push_back(ResourceFenceData{ static_cast<Scope*>(resourceBefore.m_endScope), static_cast<Scope*>(resourceAfter.m_beginScope)});
        }

        void AliasingBarrierTracker::EndInternal()
        {
            uint32_t numFencesNeeded = static_cast<uint32_t>(m_resourceFenceData.size());
            uint32_t numFencesExist = static_cast<uint32_t>(m_resourceFences.size());
                        
            //Calculate the number of new fences we need to create.
            uint32_t numFencesNeededToCreate = 0;
            if( numFencesNeeded > numFencesExist)
            {
                numFencesNeededToCreate = numFencesNeeded - numFencesExist;
            }
            
            for (uint32_t i = 0; i < numFencesNeededToCreate; i++)
            {
                m_resourceFences.emplace_back();
                Fence& fence = m_resourceFences.back();
                fence.Init(m_device, RHI::FenceState::Reset);
            }
            
            AZ_Assert(m_resourceFenceData.size() <= m_resourceFences.size(), "Need a fence per aliased resource");
            //Queue a fence per aliased resource
            for (uint32_t i = 0; i < m_resourceFenceData.size(); i++)
            {
                ResourceFenceData resourceFenceData = m_resourceFenceData[i];
                resourceFenceData.m_scopeToSignal->QueueResourceFence(Scope::ResourceFenceAction::Signal, m_resourceFences[i]);
                resourceFenceData.m_scopeToWait->QueueResourceFence(Scope::ResourceFenceAction::Wait, m_resourceFences[i]);
            }            
        }
    }
}
