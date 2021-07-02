/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

#include "Translation.h"

namespace ScriptCanvas
{
    namespace Translation
    {
        class Context;

        struct RequestTraits : public AZ::EBusTraits
        {
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual Context* GetTranslationContext() = 0;
        };

        using RequestBus = AZ::EBus<RequestTraits>;

        struct EventTraits : public AZ::EBusTraits
        {
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        };

        using EventBus = AZ::EBus<EventTraits>;

    } 

}
