/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_CONTEXTMESSAGES_H
#define LUAEDITOR_CONTEXTMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Script/ScriptContextDebug.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Script/ScriptRemoteDebugging.h>
#include <AzFramework/TargetManagement/TargetManagementAPI.h>

#pragma once

namespace LUAEditor
{
    //split these into different files later

    class Context_DocumentManagement
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners.
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<Context_DocumentManagement> Bus;
        typedef Bus::Handler Handler;

        virtual ~Context_DocumentManagement() {}

        virtual void OnNewDocument(const AZStd::string& assetId) = 0;
        virtual void OnLoadDocument(const AZStd::string& assetId, bool errorOnNotFound) = 0;
        virtual void OnCloseDocument(const AZStd::string& assetId) = 0;
        virtual void OnSaveDocument(const AZStd::string& assetId, bool bCloseAfterSave, bool bSaveAs) = 0;
        virtual bool OnSaveDocumentAs(const AZStd::string& assetId, bool bCloseAfterSave) = 0;
        virtual void OnReloadDocument(const AZStd::string assetId) = 0;

        virtual void DocumentCheckOutRequested(const AZStd::string& assetId) = 0;
        virtual void RefreshAllDocumentPerforceStat() = 0;

        virtual void UpdateDocumentData(const AZStd::string& assetId, const char* dataPtr, const AZStd::size_t dataLength) = 0;
        virtual void GetDocumentData(const AZStd::string& assetId, const char** dataPtr, AZStd::size_t& dataLength) = 0;

        virtual void NotifyDocumentModified(const AZStd::string& assetId, bool modified) = 0;
    };

    // These are messages that the context eats
    class Context_DebuggerManagement
        : public AZ::EBusTraits
    {
    public:
        virtual ~Context_DebuggerManagement() {}

        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners.
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<Context_DebuggerManagement> Bus;
        typedef Bus::Handler Handler;

        // execute the document as a script blob.  Locally = true means that the document should be executed in the editor itself.
        virtual void ExecuteScriptBlob(const AZStd::string& fromAssetId, bool executeLocally) = 0;

        // Add or remove a breakpoint
        virtual void SynchronizeBreakpoints() = 0;
        virtual void CreateBreakpoint(const AZStd::string& fromAssetId, int lineNumber) = 0;
        virtual void MoveBreakpoint(const AZ::Uuid& breakpointUID, int lineNumber) = 0;
        virtual void DeleteBreakpoint(const AZ::Uuid& breakpointUID) = 0;
        virtual void CleanUpBreakpoints() = 0;


        // Debugger connection
        virtual void OnDebuggerAttached() = 0;
        virtual void OnDebuggerRefused() = 0;
        virtual void OnDebuggerDetached() = 0;

        // breakpoint replies
        virtual void OnBreakpointHit(const AZStd::string& assetIdString, int lineNumber) = 0;
        virtual void OnBreakpointAdded(const AZStd::string& assetIdString, int lineNumber) = 0;
        virtual void OnBreakpointRemoved(const AZStd::string& assetIdString, int lineNumber) = 0;

        // Replies to queries
        virtual void OnReceivedAvailableContexts(const AZStd::vector<AZStd::string>& contexts) = 0;
        virtual void OnReceivedRegisteredClasses(const AzFramework::ScriptUserClassList& classes) = 0;
        virtual void OnReceivedRegisteredEBuses(const AzFramework::ScriptUserEBusList& ebuses) = 0;
        virtual void OnReceivedRegisteredGlobals(const AzFramework::ScriptUserMethodList& methods, const AzFramework::ScriptUserPropertyList& properties) = 0;
        virtual void OnReceivedLocalVariables(const AZStd::vector<AZStd::string>& vars) = 0;
        virtual void OnReceivedCallstack(const AZStd::vector<AZStd::string>& callstack) = 0;

        // Replies to value operations
        virtual void OnReceivedValueState(const AZ::ScriptContextDebug::DebugValue& value) = 0;
        virtual void OnSetValueResult(const AZStd::string& name, bool success) = 0;

        // if the execution is resumed ('run') then this happens:
        virtual void OnExecutionResumed() = 0; // removes line number from the current line number and lets us know that execution has resumed

        // Script execution results
        virtual void OnExecuteScriptResult(bool success) = 0;

        // requested actions from subordinate systems
        virtual void RequestDetachDebugger() = 0;
        virtual void RequestAttachDebugger() = 0;
    };
}

#endif//LUAEDITOR_CONTEXTMESSAGES_H
