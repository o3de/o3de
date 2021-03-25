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

#ifndef LUADEBUGGER_API_H
#define LUADEBUGGER_API_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

#pragma once

namespace LUADebugger
{
    class Messages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners.
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<Messages> Bus;
        typedef Bus::Handler Handler;

        virtual ~Messages() {}
    };
};

#endif//LUADEBUGGER_API_H