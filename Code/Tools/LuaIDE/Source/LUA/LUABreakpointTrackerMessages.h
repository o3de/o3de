/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_LUABREAKPOINTTRACKERMESSAGES_H
#define LUAEDITOR_LUABREAKPOINTTRACKERMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>

#pragma once

namespace AZ { class ReflectContext; }

namespace LUAEditor
{
    // combined, name+line is a unique breakpoint
    // this data definition is used by anyone tracking breakpoints
    // which currently includes the main context, editor and breakpoint control panel

    class Breakpoint
    {
    public:
        AZ_RTTI(Breakpoint, "{6E203CB5-C09B-433D-BA31-177762F574B8}");
        AZ_CLASS_ALLOCATOR(Breakpoint, AZ::SystemAllocator);

        virtual ~Breakpoint() = default;

        AZ::Uuid m_breakpointId; // a globally unique ID for every breakpoint.
        AZStd::string m_assetId; // the assetId of the document that the breakpoint was created for;
        int m_documentLine; // the line in the document that the breakpoint was set on.
        AZStd::string m_assetName;

        void RepurposeToNewOwner(const AZStd::string& newAssetName, const AZStd::string& newAssetId);
        static void Reflect(AZ::ReflectContext* reflection);
    };

    typedef AZStd::unordered_map<AZ::Uuid, Breakpoint> BreakpointMap;

    // messages going FROM the lua Context TO anyone interested in breakpoints

    class LUABreakpointTrackerMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUABreakpointTrackerMessages> Bus;
        typedef Bus::Handler Handler;

        virtual void BreakpointsUpdate(const BreakpointMap& uniqueBreakpoints) = 0;
        virtual void BreakpointHit(const Breakpoint& bp) = 0;
        virtual void BreakpointResume() = 0;

        virtual ~LUABreakpointTrackerMessages() {}
    };

    // messages going TO the lua Context FROM anyone interested in retrieving breakpoint info

    class LUABreakpointRequestMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // we only have one listener
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUABreakpointRequestMessages> Bus;
        typedef Bus::Handler Handler;

        virtual const BreakpointMap* RequestBreakpoints() = 0;
        virtual void RequestEditorFocus(const AZStd::string& assetIdString, int lineNumber) = 0;
        virtual void RequestDeleteBreakpoint(const AZStd::string& assetIdString, int lineNumber) = 0;

        virtual ~LUABreakpointRequestMessages() {}
    };
}

#endif//LUAEDITOR_LUABREAKPOINTTRACKERMESSAGES_H
