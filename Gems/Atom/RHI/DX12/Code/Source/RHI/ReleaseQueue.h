/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ObjectCollector.h>

namespace AZ
{
    namespace DX12
    {
        /**
         * This is a deferred-release queue for ID3D12Object instances. Any DX12
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
            using ObjectType = ID3D12Object;
        };

        using ReleaseQueue = RHI::ObjectCollector<ReleaseQueueTraits>;
    }
}
