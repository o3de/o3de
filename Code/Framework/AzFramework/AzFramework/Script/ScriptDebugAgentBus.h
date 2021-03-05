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

#ifndef SCRIPT_DEBUG_AGENT_BUS_H
#define SCRIPT_DEBUG_AGENT_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Script/ScriptContextDebug.h>

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
}   // namespace HExFramework

#endif
#pragma once
