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
