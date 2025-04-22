/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(AliasingBarrierTracker, AZ::SystemAllocator);
            AZ_RTTI(AliasingBarrierTracker, "{58BB64EA-B087-4008-9940-539486EEE71A}", Base);

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::AliasingBarrierTracker
            void AppendBarrierInternal(const RHI::AliasedResource& resourceBefore, const RHI::AliasedResource& resourceAfter) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
