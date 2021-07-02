/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
