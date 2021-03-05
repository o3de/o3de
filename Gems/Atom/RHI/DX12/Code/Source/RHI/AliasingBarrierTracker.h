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

#include <Atom/RHI/AliasingBarrierTracker.h>

namespace AZ
{
    namespace DX12
    {
        class Resource;
        class Scope;

        class AliasingBarrierTracker :
            public RHI::AliasingBarrierTracker
        {
            using Base = RHI::AliasingBarrierTracker;
        public:
            AZ_CLASS_ALLOCATOR(AliasingBarrierTracker, AZ::SystemAllocator, 0);
            AZ_RTTI(AliasingBarrierTracker, "{58BB64EA-B087-4008-9940-539486EEE71A}", Base);

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::AliasingBarrierTracker
            void AppendBarrierInternal(const RHI::AliasedResource& resourceBefore, const RHI::AliasedResource& resourceAfter) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
