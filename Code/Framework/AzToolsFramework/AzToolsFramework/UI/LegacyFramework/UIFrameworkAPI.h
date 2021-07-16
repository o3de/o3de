/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef UIFRAMEWORKAPI_H
#define UIFRAMEWORKAPI_H

#include <AzCore/base.h>

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/string/string.h>

#include <QtGui/qwindowdefs.h>

class QMenu;
class QToolBar;
class QAction;
class QWidget;

// this is the API with which ui framework clients talk to the framework
// this file just implements statics and helpers to remove boilerplate from the bus messaging
// specifically used for clients of the UI framework
// clients should only need to include this file to get the API. We can also split this file down even further
// into individual interfaces - but since everything is supposed to be forward declared interfaces, then the compile time
// is not impacted much

namespace AzToolsFramework
{
    class BaseViewData;

    enum SaveChangesDialogResult
    {
        SCDR_Save, // save, and then do what you were going to do.
        SCDR_DiscardAndContinue, // discard any current changes and continue what you're diong (quitting or whagteveR)
        SCDR_CancelOperation, // stop whatever you're doing (quitting or whatever)
    };

    // given an asset name, prompt the user whether they really want to overwrite it.
    // batch mode returns false without prompting.  True is returned if the user accepts the overwrite.
    bool GetOverwritePromptResult(QWidget* pParentWidget, const char* assetNameToOvewrite);

    // ------------------------------------------------------------------------------------------------------------------------
    // ------------------------------------------------ DESCRIPTOR STRUCTS ----------------------------------------------------
    // ------------------------------------------------------------------------------------------------------------------------

    struct HotkeyDescription
    {
        enum Scope
        {
            SCOPE_GLOBAL = 3,
            SCOPE_WINDOW = 2,
            SCOPE_WIDGET = 1
        };
        AZ::u32 m_HotKeyIDCRC;
        AZStd::string m_defaultKey; // a string like "Ctrl+V"
        AZStd::string m_currentKey; // the active key as edited by the end user
        AZStd::string m_description;
        AZStd::string m_category;
        Scope m_scope; // global or GUI-local for this hotkey
        // GroupID to allow similar hotkeys in different focused window contexts
        int m_WindowGroupID;

        HotkeyDescription() {}
        HotkeyDescription(const AZ::u32 hotkeyIDCRC, const AZStd::string& defHotkey, const AZStd::string& desc, const AZStd::string& category, int wgid, const Scope scope)
            : m_HotKeyIDCRC(hotkeyIDCRC)
            , m_defaultKey(defHotkey)
            , m_currentKey(defHotkey)
            , m_description(desc)
            , m_category(category)
            , m_WindowGroupID(wgid)
            , m_scope(scope) {}
    };

    struct MainWindowDescription
    {
        AZStd::string name;
        AZ::Uuid ContextID;
        HotkeyDescription hotkeyDesc;
    };

    // ------------------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------- THE LISTENERS / BUS CLASSES --------------------------------------------
    // ------------------------------------------------------------------------------------------------------------------------

    class FrameworkMessages
        : public AZ::EBusTraits
    {
    public:
        typedef AZ::EBus<FrameworkMessages> Bus;
        typedef Bus::Handler Handler;

        virtual ~FrameworkMessages() {}

        // register a hotkey to make a known hotkey that can be modified by the user
        virtual void RegisterHotkey(const HotkeyDescription& desc) = 0;

        // register an action to belong to a particular registerd hotkey.
        // when you do this, it will automatically change the action to use the new hotkey and also update it when it changes
        virtual void RegisterActionToHotkey(const AZ::u32 m_hotkeyID, QAction* pAction) = 0;

        // note that you don't HAVE to unregister it.  Qt sends us a message when an action is destroyed.  So just delete the action if you want.
        virtual void UnregisterActionFromHotkey(QAction* pAction) = 0;

        // the user would like to quit the application.  All contexts get an opportunity to cancel this operation, offering to save before close, abort, etc.
        virtual void UserWantsToQuit() = 0;

        virtual void AddComponentInfo(MainWindowDescription& desc) = 0;
        virtual void GetComponentsInfo(AZStd::list<MainWindowDescription>& theComponents) = 0;

        virtual void PopulateApplicationMenu(QMenu* theMenu) = 0;

        virtual void RequestMainWindowClose(AZ::Uuid id) = 0;
        virtual void ApplicationCensusReply(bool isOpen) = 0;
    };
}

#endif
