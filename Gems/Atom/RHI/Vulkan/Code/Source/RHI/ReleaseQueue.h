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

namespace AZ
{
    namespace Vulkan
    {
         /**
         * This is a deferred-release queue for Vulkan object instances. Any Vulkan
         * object that needs to be released on the CPU timeline should be queued
         * here to ensure that a reference is held until the GPU has flushed the
         * the last frame using it.
         *
         * Each device has an object queue, and will synchronize its collect latency
         * to match the maximum number of frames allowed on the device.
         */
        class ReleaseQueueTraits
            : public RHI::ObjectCollectorTraits
        {
        public:
            using MutexType = AZStd::mutex;
            using ObjectType = RHI::Object;
        };
        using ReleaseQueue = RHI::ObjectCollector<ReleaseQueueTraits>;
    }
}
