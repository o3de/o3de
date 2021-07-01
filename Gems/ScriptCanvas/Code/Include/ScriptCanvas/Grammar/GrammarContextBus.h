/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace ScriptCanvas
{
    namespace Grammar
    {        
        class Context;

        struct RequestTraits : public AZ::EBusTraits
        {
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual Context* GetGrammarContext() = 0;
        };

        using RequestBus = AZ::EBus<RequestTraits>;

        struct EventTraits : public AZ::EBusTraits
        {
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            // add stuff here to speed up parsing across separate graphs
        };

        using EventBus = AZ::EBus<EventTraits>;
    } 

}
