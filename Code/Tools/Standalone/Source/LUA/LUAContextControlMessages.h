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

#ifndef LUACONTEXTCONTROL_MESSAGES_H
#define LUACONTEXTCONTROL_MESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#pragma once

namespace LUAEditor
{
    //split these into different files later

    class Context_ControlManagement
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners.
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<Context_ControlManagement> Bus;
        typedef Bus::Handler Handler;

        // Debugger connection maintenance
        virtual void OnDebuggerAttached() = 0;
        virtual void OnDebuggerRefused() = 0;
        virtual void OnDebuggerDetached() = 0;
        virtual void OnTargetConnected() = 0;
        virtual void OnTargetDisconnected() = 0;
        virtual void OnTargetContextPrepared(AZStd::string& contextName) = 0;

        virtual ~Context_ControlManagement() {}
    };
}

#endif//LUACONTEXTCONTROL_MESSAGES_H
