/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
} // namespace AzToolsFramework

namespace AZ::Render
{
    //! Handle menu and action registration for render options
    class AtomRenderOptionsActionHandler
        : private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
        , private AzToolsFramework::EditorEventsBus::Handler
    {
    public:
        void Activate();
        void Deactivate();

    private:
        // ActionManagerRegistrationNotificationBus overrides...
        void OnMenuRegistrationHook() override;
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;

        // EditorEventsBus overrides...
        void NotifyMainWindowInitialized(QMainWindow* mainWindow) override;

        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;

        AZStd::unordered_map<AZ::Name, AZStd::string> m_passToActionNames;
    };
} // namespace AZ::Render
