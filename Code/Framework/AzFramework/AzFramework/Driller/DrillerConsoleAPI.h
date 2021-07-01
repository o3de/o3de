/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Driller/Driller.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    /*
     * Descriptors for drillers available on the target machine.
     */
    struct DrillerInfo final
    {
        AZ_RTTI(DrillerInfo, "{197AC318-B65C-4B36-A109-BD25422BF7D0}");
        AZ::u32 m_id;
        AZStd::string m_groupName;
        AZStd::string m_name;
        AZStd::string m_description;
    };

    typedef AZStd::vector<DrillerInfo> DrillerInfoListType;
    typedef AZStd::vector<AZ::u32> DrillerListType;

    /**
     * Driller clients interested in receiving notification events from the
     * console should implement this interface.
     */
    class DrillerConsoleEvents
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        typedef AZ::OSStdAllocator  AllocatorType;
        //////////////////////////////////////////////////////////////////////////

        virtual ~DrillerConsoleEvents() {}

        // A list of available drillers has been received from the target machine.
        virtual void OnDrillersEnumerated(const DrillerInfoListType& availableDrillers) = 0;
        virtual void OnDrillerSessionStarted(AZ::u64 sessionId) = 0;
        virtual void OnDrillerSessionStopped(AZ::u64 sessionId) = 0;
    };
    typedef AZ::EBus<DrillerConsoleEvents> DrillerConsoleEventBus;

    /**
     * Commands can be sent to the driller through this interface.
     */
    class DrillerConsoleCommands
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        typedef AZ::OSStdAllocator  AllocatorType;

        // there's only one driller console instance allowed
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~DrillerConsoleCommands() {}

        // Request an enumeration of available drillers from the target machine
        virtual void EnumerateAvailableDrillers() = 0;
        // Start a drilling session. This function is normally called internally by DrillerRemoteSession
        virtual void StartDrillerSession(const AZ::Debug::DrillerManager::DrillerListType& requestedDrillers, AZ::u64 sessionId) = 0;
        // Stop a drilling session. This function is normally called internally by DrillerRemoteSession
        virtual void StopDrillerSession(AZ::u64 sessionId) = 0;
    };
    typedef AZ::EBus<DrillerConsoleCommands> DrillerConsoleCommandBus;
}   // namespace AzFramework
