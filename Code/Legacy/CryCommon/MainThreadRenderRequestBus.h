/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    /**
     * Used to queue render commands to be safely executed on the main thread
     */
    class MainThreadRenderQueueEvents
        : public AZ::EBusTraits
    {
    public:

        virtual ~MainThreadRenderQueueEvents() {}

        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const bool EnableEventQueue = true;
        typedef AZStd::mutex EventQueueMutexType;
    };

    using MainThreadRenderRequestBus = AZ::EBus<MainThreadRenderQueueEvents>;
}
