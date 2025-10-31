/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_LUAEditorDebuggerMessages_H
#define LUAEDITOR_LUAEditorDebuggerMessages_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Script/ScriptContextDebug.h>

#pragma once

namespace LUAEditor
{
    // these are messages going from the lua editor TO the debugger.
    // for messages that travel the other way, (from the debugger to the actual lua editor, see Context_DebuggerManagement in LuaEditorContextMessages.h

    struct TargetInfo
    {
        AZStd::string m_displayName; // the name to show the user, like "The Editor" or whatever
        AZ::u32 m_identifier; // CRC that uniquely identifies a target.  This should remain stable across reboots so that we can remember what the last context was.
        // we do not allow debugging of all contexts - sometimes we can execute script but not debug.
        // for example, the "local" in-process editor can not debug or else we deadlock, but if its remote, we can debug.
        bool m_allowDebug;
        TargetInfo(const char* displayName = NULL, AZ::u32 identifier = 0, bool allowDebug = false)
            : m_displayName(displayName)
            , m_identifier(identifier)
            , m_allowDebug(allowDebug) {}
    };

    class LUAEditorDebuggerMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners???
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUAEditorDebuggerMessages> Bus;

        virtual ~LUAEditorDebuggerMessages() {}

        // Request enumeration of available script contexts
        virtual void EnumerateContexts() = 0;

        // Request to be attached to script context
        virtual void AttachDebugger(const char* scriptContextName) = 0;

        // Request to be detached from current context
        virtual void DetachDebugger() = 0;

        // Request enumeration of classes registered in the current context
        virtual void EnumRegisteredClasses(const char* scriptContextName) = 0;

        // Request enumeration of eBuses registered in the current context
        virtual void EnumRegisteredEBuses(const char* scriptContextName) = 0;

        // Request enumeration of global methods and properties registered in the current context
        virtual void EnumRegisteredGlobals(const char* scriptContextName) = 0;

        // create a breakpoint.  The debugName is the name that was given when the script was executed and represents
        // the 'document' (or blob of script) that the breakpoint is for.  The line number is relative to the start of that blob.
        // the combination of line number and debug name uniquely identify a debug breakpoint.
        virtual void CreateBreakpoint(const AZStd::string& debugName, int lineNumber) = 0;

        // Remove a previously set breakpoint from the current context
        virtual void RemoveBreakpoint(const AZStd::string& debugName, int lineNumber) = 0;

        // Set over current line in current context. Can only be called while context is on a breakpoint.
        virtual void DebugRunStepOver() = 0;

        // Set into current line in current context. Can only be called while context is on a breakpoint.
        virtual void DebugRunStepIn() = 0;

        // Set out of current line in current context. Can only be called while context is on a breakpoint.
        virtual void DebugRunStepOut() = 0;

        // Stop execution in current context. Not supported.
        virtual void DebugRunStop() = 0;

        // Continue execution of current context. Can only be called while context is on a breakpoint.
        virtual void DebugRunContinue() = 0;

        // Request enumeration of local variables in current context. Can only be called while context is on a breakpoint
        virtual void EnumLocals() = 0;

        // Get value of a variable in the current context. Can only be called while context is on a breakpoint
        virtual void GetValue(const AZStd::string& varName) = 0;

        // Set value of a variable in the current context. Can only be called while context is on a breakpoint
        // and value should be the structure returned from a previous call to GetValue().
        virtual void SetValue(const AZ::ScriptContextDebug::DebugValue& value) = 0;

        // Request current callstack in the current context. Can only be called while context is on a breakpoint
        virtual void GetCallstack() = 0;
    };

    using LUAEditorDebuggerMessagesRequestBus = AZ::EBus<LUAEditorDebuggerMessages>;

}

#endif//LUAEDITOR_LUAEditorDebuggerMessages_H
