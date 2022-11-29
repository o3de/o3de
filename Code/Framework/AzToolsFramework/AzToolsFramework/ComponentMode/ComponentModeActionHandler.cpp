/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentMode/ComponentModeActionHandler.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>

static constexpr AZStd::string_view EditorMainWindowActionContextIdentifier = "o3de.context.editor.mainwindow";

static constexpr AZStd::string_view ComponentModeChangedUpdaterIdentifier = "o3de.updater.onComponentModeChanged";

static constexpr AZStd::string_view EditMenuIdentifier = "o3de.menu.editor.edit";

namespace AzToolsFramework
{
    ComponentModeActionHandler::ComponentModeActionHandler()
    {
        if (IsNewActionManagerEnabled())
        {
            AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            ActionManagerRegistrationNotificationBus::Handler::BusConnect();
            ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusConnect(editorEntityContextId);
            ViewportEditorModeNotificationsBus::Handler::BusConnect(editorEntityContextId);
        }
    }

    ComponentModeActionHandler::~ComponentModeActionHandler()
    {
        if (IsNewActionManagerEnabled())
        {
            ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusDisconnect();
            ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        }
    }

    void ComponentModeActionHandler::OnActionContextModeRegistrationHook()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(
            m_actionManagerInterface,
            "ComponentModeActionHandler - could not get ActionManagerInterface on ComponentModeActionHandler "
            "OnActionUpdaterRegistrationHook.");

        m_actionManagerInterface->RegisterActionContextMode(EditorMainWindowActionContextIdentifier, "AnyComponentMode");
    }

    void ComponentModeActionHandler::OnActionUpdaterRegistrationHook()
    {
        m_actionManagerInterface->RegisterActionUpdater(ComponentModeChangedUpdaterIdentifier);
    }

    void ComponentModeActionHandler::OnActionRegistrationHook()
    {
        m_hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        AZ_Assert(
            m_hotKeyManagerInterface,
            "ComponentModeActionHandler - could not get HotKeyManagerInterface on ComponentModeActionHandler OnActionRegistrationHook.");

        // Add default actions for every Component Mode
        
        // Exit Component Mode
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
            m_actionManagerInterface->AssignModeToAction("SomeMode", actionIdentifier);

            m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Esc");
        }
    }

    void ComponentModeActionHandler::OnMenuBindingHook()
    {
        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        AZ_Assert(
            m_menuManagerInterface,
            "ComponentModeActionHandler - could not get MenuManagerInterface on ComponentModeActionHandler OnMenuBindingHook.");

        m_menuManagerInterface->AddSeparatorToMenu(EditMenuIdentifier, 10000);
        m_menuManagerInterface->AddActionToMenu(EditMenuIdentifier, "o3de.action.componentMode.end", 10001);
    }

    void ComponentModeActionHandler::ActiveComponentModeChanged([[maybe_unused]] const AZ::Uuid& componentType)
    {
        if (m_actionManagerInterface)
        {
            m_actionManagerInterface->SetActiveActionContextMode(EditorMainWindowActionContextIdentifier, "AnyComponentMode");

            // Update Component Mode Changed updater
            m_actionManagerInterface->RegisterActionUpdater(ComponentModeChangedUpdaterIdentifier);
        }
    }

    void ComponentModeActionHandler::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        if (m_actionManagerInterface && mode == ViewportEditorMode::Component)
        {
            m_actionManagerInterface->SetActiveActionContextMode(EditorMainWindowActionContextIdentifier, "AnyComponentMode");

            // Update Component Mode Changed updater
            m_actionManagerInterface->RegisterActionUpdater(ComponentModeChangedUpdaterIdentifier);
        }
    }

    void ComponentModeActionHandler::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        if (m_actionManagerInterface && mode == ViewportEditorMode::Component)
        {
            m_actionManagerInterface->SetActiveActionContextMode(EditorMainWindowActionContextIdentifier, DefaultActionContextModeIdentifier);

            // Update Component Mode Changed updater
            m_actionManagerInterface->RegisterActionUpdater(ComponentModeChangedUpdaterIdentifier);
        }
    }

} // namespace AzToolsFramework
