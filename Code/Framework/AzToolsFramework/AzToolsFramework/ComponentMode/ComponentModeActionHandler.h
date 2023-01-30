/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
    class HotKeyManagerInterface;

    //! Handler class to sync up Component Mode customized actions to the Action Manager.
    //! Ensures each Component Mode is mapped to an ActionContextMode and that the two systems
    //! are correctly synchronized when the state changes.
    class ComponentModeActionHandler
        : private ActionManagerRegistrationNotificationBus::Handler
        , private ComponentModeFramework::EditorComponentModeNotificationBus::Handler
        , private ViewportEditorModeNotificationsBus::Handler
    {
    public:
        ComponentModeActionHandler();
        ~ComponentModeActionHandler();

    private:
        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionContextModeRegistrationHook() override;
        void OnActionUpdaterRegistrationHook() override;
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;

        // EditorComponentModeNotificationBus overrides ...
        void ActiveComponentModeChanged(const AZ::Uuid& componentType) override;

        // ViewportEditorModeNotificationsBus overrides ...
        void OnEditorModeActivated(const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode) override;
        void OnEditorModeDeactivated(const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode) override;

        void RegisterCommonComponentModeActions();

        // Enumerates all classes derived from the Component Mode Base class.
        void EnumerateComponentModes(const AZStd::function<bool(const AZ::SerializeContext::ClassData*, const AZ::Uuid&)>& handler);

        void ChangeToMode(const AZStd::string& modeIdentifier);

        AZStd::unordered_map<AZ::Uuid, AZStd::string> m_componentModeToActionContextModeMap;

        ActionManagerInterface* m_actionManagerInterface = nullptr;
        MenuManagerInterface* m_menuManagerInterface = nullptr;
        HotKeyManagerInterface* m_hotKeyManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
