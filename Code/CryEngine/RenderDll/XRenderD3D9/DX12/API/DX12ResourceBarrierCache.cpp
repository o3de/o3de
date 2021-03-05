/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#include "RenderDll_precompiled.h"
#include "DX12ResourceBarrierCache.h"

namespace DX12
{
    const char* BarrierFlagsToString(D3D12_RESOURCE_BARRIER_FLAGS flags)
    {
        if (flags & D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY)
        {
            return "BEGIN_ONLY";
        }
        else if (flags & D3D12_RESOURCE_BARRIER_FLAG_END_ONLY)
        {
            return "END_ONLY";
        }

        return "NONE";
    }

    void ResourceBarrierCache::EnqueueTransition(
        ID3D12GraphicsCommandList* commandList,
        const D3D12_RESOURCE_BARRIER_FLAGS flags,
        const D3D12_RESOURCE_TRANSITION_BARRIER& transition)
    {
        bool bFoundChain = false;

        for (TransitionChainRoot& chain : m_TransitionChains)
        {
            if (chain.resource == transition.pResource)
            {
                TransitionNode& prevNode = m_TransitionNodes[chain.tailIdx];

                // We can collapse split barriers right here if we find them. We know the previous
                // barrier must be a BEGIN_ONLY because we've seen this resource before. Therefore
                // we assert that they match and simply remove the split. This simplifies the flattening
                // process later on.
                if (flags == D3D12_RESOURCE_BARRIER_FLAG_END_ONLY)
                {
                    D3D12_RESOURCE_BARRIER& prevBarrier = m_TransitionBarriers[prevNode.barrierIdx];

                    AZ_Assert(
                        prevBarrier.Flags == D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY &&
                        prevBarrier.Transition.StateBefore == transition.StateBefore &&
                        prevBarrier.Transition.StateAfter == transition.StateAfter,
                        "Split barrier not properly closed");

                    prevBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                    return;
                }

                TransitionNode self;
                self.barrierIdx = AZ::u8(m_TransitionBarriers.size());
                self.selfIdx = AZ::u8(m_TransitionNodes.size());
                self.nextIdx = InvalidNodeIndex;
                prevNode.nextIdx = self.selfIdx;

                m_TransitionNodes.push_back(self);

                // Reset the tail of the list to the new node.
                chain.tailIdx = self.selfIdx;

                bFoundChain = true;
                break;
            }
        }

        if (!bFoundChain)
        {
            TransitionNode self;
            self.barrierIdx = AZ::u8(m_TransitionBarriers.size());
            self.selfIdx = AZ::u8(m_TransitionNodes.size());
            self.nextIdx = InvalidNodeIndex;
            m_TransitionNodes.push_back(self);

            TransitionChainRoot root;
            root.resource = transition.pResource;
            root.tailIdx = self.selfIdx;
            root.headIdx = self.selfIdx;
            m_TransitionChains.push_back(root);
        }

        m_TransitionBarriers.emplace_back();

        D3D12_RESOURCE_BARRIER& barrier = m_TransitionBarriers.back();
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = flags;
        barrier.Transition = transition;

        TryCapacityFlush(commandList);
    }

    void ResourceBarrierCache::EnqueueUAV(ID3D12GraphicsCommandList* commandList, Resource& resource)
    {
        m_Barriers.emplace_back();

        D3D12_RESOURCE_BARRIER& barrierDesc = m_Barriers.back();
        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDesc.UAV.pResource = resource.GetD3D12Resource();

        TryCapacityFlush(commandList);
    }

    void ResourceBarrierCache::Flush(ID3D12GraphicsCommandList* commandList)
    {
        AZ_TRACE_METHOD();
        for (TransitionChainRoot& chain : m_TransitionChains)
        {
            TransitionNode* node = &m_TransitionNodes[chain.headIdx];

            m_Barriers.emplace_back();
            D3D12_RESOURCE_BARRIER* pendingBarrier = &m_Barriers.back();
            *pendingBarrier = m_TransitionBarriers[node->barrierIdx];

            while (node->nextIdx != InvalidNodeIndex)
            {
                node = &m_TransitionNodes[node->nextIdx];
                D3D12_RESOURCE_BARRIER& nextBarrier = m_TransitionBarriers[node->barrierIdx];

                bool bFlattenable =
                    nextBarrier.Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE &&
                    pendingBarrier->Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE;

                // If we can flatten, we don't need to emit a new barrier, we just fix up the pending one.
                if (bFlattenable)
                {
                    pendingBarrier->Transition.StateAfter = nextBarrier.Transition.StateAfter;
                }
                else
                {
                    // Emit current barrier and reset.
                    m_Barriers.emplace_back();
                    pendingBarrier = &m_Barriers.back();
                    *pendingBarrier = nextBarrier;
                }
            }

            // Remove barrier if it ended up as a noop.
            if (pendingBarrier->Transition.StateBefore == pendingBarrier->Transition.StateAfter)
            {
                m_Barriers.pop_back();
            }
        }

        if (m_Barriers.size())
        {
            commandList->ResourceBarrier((UINT)m_Barriers.size(), m_Barriers.data());
            m_Barriers.clear();
        }

        m_TransitionBarriers.clear();
        m_TransitionChains.clear();
        m_TransitionNodes.clear();
    }
}
