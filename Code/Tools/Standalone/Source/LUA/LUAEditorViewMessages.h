/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_VIEWMESSAGES_H
#define LUAEDITOR_VIEWMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

#pragma once

namespace LUAEditor
{
    struct DocumentInfo;

    // these are the messages targeted to the main window of the lua editor
    // they are used to talk to the views and so on.
    class LUAEditorMainWindowMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we have multiple listeners.
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUAEditorMainWindowMessages> Bus;
        typedef Bus::Handler Handler;

        virtual ~LUAEditorMainWindowMessages() {}

        virtual void OnFocusInEvent(const AZStd::string& assetId) = 0;
        virtual void OnFocusOutEvent(const AZStd::string& assetId) = 0;
        virtual void OnRequestCheckOut(const AZStd::string& assetId) = 0;
        virtual void OnConnectedToTarget() = 0;
        virtual void OnDisconnectedFromTarget() = 0;
        virtual void OnConnectedToDebugger() = 0;
        virtual void OnDisconnectedFromDebugger() = 0;
        virtual void OnExecuteScriptResult(bool success) = 0;

        /// Request a main window repaint
        virtual void Repaint() = 0;
    };
}

#endif//LUAEDITOR_VIEWMESSAGES_H
