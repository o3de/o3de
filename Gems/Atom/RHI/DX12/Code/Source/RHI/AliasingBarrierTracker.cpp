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
#include "RHI/Atom_RHI_DX12_precompiled.h"
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
            static_cast<Scope*>(resourceAfter.m_beginScope)->QueueAliasingBarrier(barrier);
        }
    }
}
