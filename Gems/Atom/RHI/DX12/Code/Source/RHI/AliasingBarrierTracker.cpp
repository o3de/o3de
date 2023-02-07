/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/AliasingBarrierTracker.h>
#include <RHI/Image.h>
#include <RHI/Buffer.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace DX12
    {
        ID3D12Resource* GetD3D12Resource(const RHI::AliasedResource& aliasedResource)
        {
            switch (aliasedResource.m_type)
            {
            case RHI::AliasedResourceType::Buffer:
                return static_cast<Buffer*>(aliasedResource.m_resource)->GetMemoryView().GetMemory();
            case RHI::AliasedResourceType::Image:
                return static_cast<Image*>(aliasedResource.m_resource)->GetMemoryView().GetMemory();
            default:
                AZ_Assert(false, "Invalid aliased resource type %d", aliasedResource.m_type);
                return nullptr;
            }
        }

        void AliasingBarrierTracker::AppendBarrierInternal(const RHI::AliasedResource& resourceBefore, const RHI::AliasedResource& resourceAfter)
        {
            D3D12_RESOURCE_ALIASING_BARRIER barrier;
            barrier.pResourceBefore = GetD3D12Resource(resourceBefore);
            barrier.pResourceAfter = GetD3D12Resource(resourceAfter);

            const BarrierOp::CommandListState* state = nullptr;
            switch (resourceBefore.m_type)
            {
            case RHI::AliasedResourceType::Image:
                {
                    // Need to set the proper sample positions (or reset them) before emitting the barrier
                    const auto& descriptor = static_cast<Image*>(resourceBefore.m_resource)->GetDescriptor();
                    if (RHI::CheckBitsAll(descriptor.m_bindFlags, RHI::ImageBindFlags::Depth))
                    {
                        state = &descriptor.m_multisampleState;
                    }
                }
                break;
            default:
                break;
            }
            static_cast<Scope*>(resourceAfter.m_beginScope)->QueueAliasingBarrier(barrier, state);
        }
    }
}
