/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_LUASTACKTRACKERMESSAGES_H
#define LUAEDITOR_LUASTACKTRACKERMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#pragma once

namespace LUAEditor
{
    // combined, name+line is a unique stack entry
    // this data definition is used by anyone tracking stacks at execution break

    struct StackEntry
    {
    public:
        AZ_CLASS_ALLOCATOR(StackEntry, AZ::SystemAllocator);

        AZStd::string m_blob; // the name of the debug blob
        int m_blobLine; // the line relative to the start of that blob
    };
    typedef AZStd::list<StackEntry> StackList;

    // messages going FROM the lua Context TO anyone interested in stack updates (aka the stack panel)

    class LUAStackTrackerMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUAStackTrackerMessages> Bus;
        typedef Bus::Handler Handler;

        virtual void StackUpdate(const StackList& stackList) = 0;
        virtual void StackClear() = 0;

        virtual ~LUAStackTrackerMessages() {}
    };

    // messages going TO the lua Context FROM anyone interested in retrieving breakpoint info

    class LUAStackRequestMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // we only have one listener
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUAStackRequestMessages> Bus;
        typedef Bus::Handler Handler;

        virtual void RequestStackClicked(const AZStd::string& blobName, int lineNumber) = 0;

        virtual ~LUAStackRequestMessages() {}
    };
}

#endif//LUAEDITOR_LUASTACKTRACKERMESSAGES_H
