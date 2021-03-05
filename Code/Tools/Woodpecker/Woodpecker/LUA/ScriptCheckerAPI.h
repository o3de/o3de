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