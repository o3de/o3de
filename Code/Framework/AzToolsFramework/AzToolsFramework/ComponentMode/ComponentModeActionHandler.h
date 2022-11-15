/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
    class HotKeyManagerInterface;

    class ComponentModeActionHandler
        : private ActionManagerRegistrationNotificationBus::Handler
        , private ComponentModeFramework::EditorComponentModeNotificationBus::Handler
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
        void ComponentModeStarted(const AZ::Uuid& componentType);
        void ActiveComponentModeChanged(const AZ::Uuid& componentType);
        void ComponentModeEnded();

        ActionManagerInterface* m_actionManagerInterface = nullptr;
        MenuManagerInterface* m_menuManagerInterface = nullptr;
        HotKeyManagerInterface* m_hotKeyManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
