/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Script/ScriptContextDebug.h>
#include <AzFramework/AzFrameworkAPI.h>

namespace AzFramework
{
    /*
     * The script debug agent, if available, will listen on this bus.
     */
    class ScriptDebugAgentEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~ScriptDebugAgentEvents() {}

        virtual void RegisterContext(AZ::ScriptContext* sc, const char* name) = 0;  // Tells the agent that a new script context is available for debugging.
        virtual void UnregisterContext(AZ::ScriptContext* sc) = 0;  // Remove a script context from the agent's list.
    };
    typedef AZ::EBus<ScriptDebugAgentEvents> ScriptDebugAgentBus;
}   // namespace AzFramework

AZ_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AZF_API, AzFramework::ScriptDebugAgentEvents)
