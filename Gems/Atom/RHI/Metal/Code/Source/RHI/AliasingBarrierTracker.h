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
#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RHI/AliasingBarrierTracker.h>
#include <RHI/Fence.h>
#include <RHI/Memory.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
        class Resource;
        class Scope;

        class AliasingBarrierTracker
            : public RHI::AliasingBarrierTracker
        {
            using Base = RHI::AliasingBarrierTracker;
        public:
            AZ_CLASS_ALLOCATOR(AliasingBarrierTracker, AZ::SystemAllocator, 0);
            AZ_RTTI(AliasingBarrierTracker, "{38A96291-D9D1-4C9B-8894-AC381D284F29}", Base);

            AliasingBarrierTracker(Device& device);
            
        private:
            //////////////////////////////////////////////////////////////////////////
           // RHI::AliasingBarrierTracker
            void AppendBarrierInternal(const RHI::AliasedResource& resourceBefore, const RHI::AliasedResource& resourceAfter) override;
            void EndInternal() override;
            void ResetInternal() override;
            //////////////////////////////////////////////////////////////////////////            

            struct ResourceFenceData
            {
                Scope* m_scopeToSignal = nullptr;
                Scope* m_scopeToWait = nullptr;
            };

            AZStd::vector<ResourceFenceData> m_resourceFenceData;
            AZStd::vector<Fence> m_resourceFences;
            RHI::Ptr<Device> m_device;
        };
    }
}
