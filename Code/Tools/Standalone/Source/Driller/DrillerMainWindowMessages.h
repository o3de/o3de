/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_DRILLERMAINWINDOWMESSAGES_H
#define DRILLER_DRILLERMAINWINDOWMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

#include <Source/Driller/DrillerDataTypes.h>
#include <AzCore/std/string/string.h>

#pragma once

namespace Driller
{
    // messages going FROM the Driller Main Window Context TO anyone interested in frame scrubber control
    class DrillerMainWindowMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // components have an actual ID that they report back on
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef int BusIdType;
        typedef AZ::EBus<DrillerMainWindowMessages> Bus;
        typedef Bus::Handler Handler;

        virtual void FrameChanged(FrameNumberType frame) = 0;
        virtual void PlaybackLoopBeginChanged(FrameNumberType frame){(void)frame; }
        virtual void PlaybackLoopEndChanged(FrameNumberType frame){(void)frame; }
        /// Important: eventIndex is the event index for the aggregator, NOT global event id.
        virtual void EventChanged(EventNumberType eventIndex) = 0;

        virtual ~DrillerMainWindowMessages() {}
    };

    // messages going FROM the main window TO (data viewers) anyone interested in event actions
    class DrillerEventWindowMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // components have an actual ID that they report back on
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef int BusIdType;
        typedef AZ::EBus<DrillerEventWindowMessages> Bus;
        typedef Bus::Handler Handler;

        virtual void EventFocusChanged(EventNumberType eventIdx) = 0;

        virtual ~DrillerEventWindowMessages() {}
    };

    // messages going FROM the main window TO (aggregators and their data viewers) anyone using Driller Workspace files
    class WorkspaceSettingsProvider;
    class DrillerWorkspaceWindowMessages
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // components have an actual ID that they report back on
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef int BusIdType;
        typedef AZ::EBus<DrillerWorkspaceWindowMessages> Bus;
        typedef Bus::Handler Handler;

        // overlay anything you want from the provider into your internal state.
        virtual void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*) = 0;

        // now open windows / etc that are specified in your internal saved state.
        virtual void ActivateWorkspaceSettings(WorkspaceSettingsProvider*) = 0;
        virtual void SaveSettingsToWorkspace(WorkspaceSettingsProvider*) = 0;

        virtual ~DrillerWorkspaceWindowMessages() {}
    };

    // messages going FROM any data viewers TO the global window to request action
    class DrillerDataViewMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<DrillerDataViewMessages> Bus;
        typedef Bus::Handler Handler;

        //virtual void EventRequestEventFocus(AZ::s64 eventIdx) = 0;
        virtual void EventRequestOpenFile(AZStd::string fileName) = 0;
        virtual void EventRequestOpenWorkspace(AZStd::string fileName) = 0;

        virtual ~DrillerDataViewMessages() {}
    };
    
    // messages going FROM any data viewers TO the capture window
    class DrillerCaptureWindowInterface : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // components have an actual ID that they report back on
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef int BusIdType;
        
        virtual void ScrubToFrameRequest(FrameNumberType frameType) = 0;
    };
    
    typedef AZ::EBus<DrillerCaptureWindowInterface> DrillerCaptureWindowRequestBus;

}

#endif//DRILLER_DRILLERMAINWINDOWMESSAGES_H
