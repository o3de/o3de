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
