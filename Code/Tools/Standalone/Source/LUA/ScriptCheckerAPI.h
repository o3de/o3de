/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SCRIPTCHECKER_H
#define SCRIPTCHECKER_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>


namespace LUAEditor
{
    class ScriptCheckerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<ScriptCheckerRequests> Bus;
        typedef Bus::Handler Handler;

        virtual void StartScriptingCheck(const BreakpointMap& uniqueBreakpoints) = 0;
        virtual void BreakpointHit(const Breakpoint& bp) = 0;
        virtual void BreakpointResume() = 0;

        virtual ~ScriptCheckerRequests() {}
    };
}

#pragma once

#endif
