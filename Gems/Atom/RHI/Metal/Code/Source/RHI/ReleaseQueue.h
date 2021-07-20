/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ObjectCollector.h>
#include <RHI/Memory.h>

namespace AZ
{
    namespace Metal
    {
        // This is a deferred-release queue for Memory instances.
        class ReleaseQueueTraits
            : public RHI::ObjectCollectorTraits
        {
        public:
            using MutexType = AZStd::mutex;
            using ObjectType = Memory;
        };

        using ReleaseQueue = RHI::ObjectCollector<ReleaseQueueTraits>;
    }
}
