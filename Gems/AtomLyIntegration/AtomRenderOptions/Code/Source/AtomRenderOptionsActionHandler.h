/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
    class ToolBarManagerInterface;
} // namespace AzToolsFramework

namespace AZ::Render
{
    //! Handle menu and action registration for render options
    //! @note Should follow the lifetime of the editor as action callbacks are capturing this and are not unregistered
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
        void OnToolBarBindingHook() override;

        // EditorEventsBus overrides...
        void NotifyMainWindowInitialized(QMainWindow* mainWindow) override;

        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
        AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;

        AZStd::optional<bool> m_taaEnabled;
    };
} // namespace AZ::Render
