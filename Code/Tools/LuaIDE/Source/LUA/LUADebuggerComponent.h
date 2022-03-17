/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUADEBUGGER_COMPONENT_H
#define LUADEBUGGER_COMPONENT_H

#include "LUAEditorDebuggerMessages.h"
#include <AzFramework/TargetManagement/TargetManagementAPI.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#pragma once

namespace LUADebugger
{
    // the perforce component's job is to manage perforce connectivity and execute perforce commands
    // it parses the status of perforce commands and returns results.
    // it has helpers to determine what needs to be done with a file in order to remove it or to add it.
    // for example, if a file is checked out and needs to be deleted, it knows that it will need to both revert the file and mark it for delete
    // in order to get to where we want it to be, from where we are now.
    // it does not keep track of individual files or watch directories or anything
    class Component
        : public AZ::Component
        , public LUAEditor::LUAEditorDebuggerMessages::Bus::Handler
        , public AzFramework::TargetManagerClient::Bus::Handler
        , public AzFramework::TmMsgBus::Handler
    {
    public:
        AZ_COMPONENT(Component, "{7854C9F4-D7E5-4420-A14E-FA5B19822F39}");

        Component();
        virtual ~Component();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init();
        virtual void Activate();
        virtual void Deactivate();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //Debugger Messages, from the LUAEditor::LUAEditorDebuggerMessages::Bus
        // Enumerate script contexts on the target
        // Request enumeration of available script contexts
        virtual void EnumerateContexts();
        // Request to be attached to script context
        virtual void AttachDebugger(const char* scriptContextName);
        // Request to be detached from current context
        virtual void DetachDebugger();
        // Request enumeration of classes registered in the current context
        virtual void EnumRegisteredClasses(const char* scriptContextName);
        // Request enumeration of eBusses registered in the current context
        virtual void EnumRegisteredEBuses(const char* scriptContextName);
        // Request enumeration of global methods and properties registered in the current context
        virtual void EnumRegisteredGlobals(const char* scriptContextName);
        // create a breakpoint.  The debugName is the name that was given when the script was executed and represents
        // the 'document' (or blob of script) that the breakpoint is for.  The line number is relative to the start of that blob.
        // the combination of line number and debug name uniquely identify a debug breakpoint.
        virtual void CreateBreakpoint(const AZStd::string& debugName, int lineNumber);
        // Remove a previously set breakpoint from the current context
        virtual void RemoveBreakpoint(const AZStd::string& debugName, int lineNumber);
        // Set over current line in current context. Can only be called while context is on a breakpoint.
        virtual void DebugRunStepOver();
        // Set into current line in current context. Can only be called while context is on a breakpoint.
        virtual void DebugRunStepIn();
        // Set out of current line in current context. Can only be called while context is on a breakpoint.
        virtual void DebugRunStepOut();
        // Stop execution in current context. Not supported.
        virtual void DebugRunStop();
        // Continue execution of current context. Can only be called while context is on a breakpoint.
        virtual void DebugRunContinue();
        // Request enumeration of local variables in current context. Can only be called while context is on a breakpoint
        virtual void EnumLocals();
        // Get value of a variable in the current context. Can only be called while context is on a breakpoint
        virtual void GetValue(const AZStd::string& varName);
        // Set value of a variable in the current context. Can only be called while context is on a breakpoint
        // and value should be the structure returned from a previous call to GetValue().
        virtual void SetValue(const AZ::ScriptContextDebug::DebugValue& value);
        // Request current callstack in the current context. Can only be called while context is on a breakpoint
        virtual void GetCallstack();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TargetManagerClient
        virtual void DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TmMsgBus
        virtual void OnReceivedMsg(AzFramework::TmMsgPtr msg);
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection);
    };
};

#endif
