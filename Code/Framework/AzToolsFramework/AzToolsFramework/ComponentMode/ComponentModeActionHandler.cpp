/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentMode/ComponentModeActionHandler.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>

static constexpr AZStd::string_view EditorMainWindowActionContextIdentifier = "o3de.context.editor.mainwindow";

namespace AzToolsFramework
{
    void ComponentModeActionHandler::Initialize()
    {
        ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    ComponentModeActionHandler::~ComponentModeActionHandler()
    {
        ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

    void ComponentModeActionHandler::OnActionUpdaterRegistrationHook()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(
            m_actionManagerInterface,
            "ComponentModeActionHandler - could not get ActionManagerInterface on ComponentModeActionHandler OnActionUpdaterRegistrationHook.");

        // TODO - Register Component Mode Change Updater
    }

    void ComponentModeActionHandler::OnActionRegistrationHook()
    {
        m_hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        AZ_Assert(
            m_hotKeyManagerInterface,
            "ComponentModeActionHandler - could not get HotKeyManagerInterface on ComponentModeActionHandler OnActionRegistrationHook.");

        // Add default actions for every Component Mode

        // TODO - Only add these if at least one component mode was added.

        // TODO - Edit Previous
        // TODO - Edit Next

        // Add Back Action to return to default Component Mode

        // New Level
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.componentMode.end";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Done";
            actionProperties.m_description = "Return to normal viewport editing";
            actionProperties.m_category = "Component Modes";

            m_actionManagerInterface->RegisterAction(
                EditorMainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []
                {
                    ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                        &ComponentModeFramework::ComponentModeSystemRequests::EndComponentMode);
                }
            );

            // Only add this to the component modes, not default.
            // TODO - this would be a great helper
            m_actionManagerInterface->AssignModeToAction("SomeMode", actionIdentifier);

            m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Esc");
        }
    }

} // namespace AzToolsFramework
