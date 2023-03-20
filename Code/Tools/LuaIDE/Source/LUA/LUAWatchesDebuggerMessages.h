/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_LUAWATCHESDEBUGGERMESSAGES_H
#define LUAEDITOR_LUAWATCHESDEBUGGERMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Script/ScriptContextDebug.h>

#pragma once

namespace LUAEditor
{
    // messages going FROM the lua Context TO anyone watching variables

    class LUAWatchesDebuggerMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUAWatchesDebuggerMessages> Bus;
        typedef Bus::Handler Handler;

        virtual void WatchesUpdate(const AZ::ScriptContextDebug::DebugValue& value) = 0;
        virtual void OnDebuggerAttached() = 0;
        virtual void OnDebuggerDetached() = 0;

        virtual ~LUAWatchesDebuggerMessages() {}
    };

    using LUAWatchesDebuggerMessagesRequestBus = AZ::EBus<LUAWatchesDebuggerMessages>;

    // messages going TO the lua Context FROM anyone needing watch info
    class LUAWatchesRequestMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // we only have one listener
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUAWatchesRequestMessages> Bus;
        typedef Bus::Handler Handler;

        virtual void RequestWatchedVariable(const AZStd::string& varName) = 0;

        virtual ~LUAWatchesRequestMessages() {}
    };

    using LUAWatchesRequestMessagesRequestBus = AZ::EBus<LUAWatchesRequestMessages>;
}

#endif//LUAEDITOR_LUAWATCHESDEBUGGERMESSAGES_H
