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
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
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

        EnumerateComponentModes(
            [&](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
            {
                AZStd::string modeIdentifier = AZStd::string::format("o3de.context.mode.%s", classData->m_name);
                m_actionManagerInterface->RegisterActionContextMode(EditorMainWindowActionContextIdentifier, modeIdentifier);
                m_componentModeToActionContextModeMap.emplace(classData->m_typeId, AZStd::move(modeIdentifier));

                return true;
            }
        );
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

        // Register default actions common to every Component Mode
        RegisterCommonComponentModeActions();
    }

    void ComponentModeActionHandler::RegisterCommonComponentModeActions()
    {
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

        // Add these Actions to all Component Modes
        EnumerateComponentModes(
            [&](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
            {
                // Add default actions to all component modes
                AZStd::string modeIdentifier = AZStd::string::format("o3de.context.mode.%s", classData->m_name);
                m_actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.componentMode.end");

                return true;
            }
        );
    }

    void ComponentModeActionHandler::OnMenuBindingHook()
    {
        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        AZ_Assert(
            m_menuManagerInterface,
            "ComponentModeActionHandler - could not get MenuManagerInterface on ComponentModeActionHandler OnMenuBindingHook.");

        m_menuManagerInterface->AddSeparatorToMenu(EditMenuIdentifier, 3000);
        // Component Mode Actions should be located between these two separators.
        m_menuManagerInterface->AddSeparatorToMenu(EditMenuIdentifier, 10000);
        m_menuManagerInterface->AddActionToMenu(EditMenuIdentifier, "o3de.action.componentMode.end", 10001);
    }

    void ComponentModeActionHandler::ActiveComponentModeChanged(const AZ::Uuid& componentType)
    {
        // This function triggers when the Editor changes from one Component Mode to another.
        // This syncs up the Action Context Mode to the Component Mode, and triggers the updater
        // for actions whose enabled state depends on which Component Mode is enabled.
        if (m_actionManagerInterface)
        {
            if (auto componentModeTypeIter = m_componentModeToActionContextModeMap.find(componentType);
                componentModeTypeIter != m_componentModeToActionContextModeMap.end())
            {
                ChangeToMode(componentModeTypeIter->second);
            }
        }
    }

    void ComponentModeActionHandler::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        // This function triggers when the Editor changes from regular editing to a Component Mode.
        // This syncs up the Action Context Mode to the Component Mode, and triggers the updater
        // for actions whose enabled state depends on which Component Mode is enabled.
        if (m_actionManagerInterface && mode == ViewportEditorMode::Component)
        {
            AZ::Uuid componentModeTypeId;
            ComponentModeFramework::ComponentModeRequestBus::BroadcastResult(
                componentModeTypeId, &ComponentModeFramework::ComponentModeRequests::GetComponentModeType);

            if (auto componentModeTypeIter = m_componentModeToActionContextModeMap.find(componentModeTypeId);
                componentModeTypeIter != m_componentModeToActionContextModeMap.end())
            {
                ChangeToMode(componentModeTypeIter->second);
            }
        }
    }

    void ComponentModeActionHandler::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        // This function triggers when the Editor changes from a Component Mode to regular editing.
        // This sets the Action Context Mode back to the default value, and triggers the updater
        // for actions whose enabled state depends on which Component Mode is enabled.
        if (m_actionManagerInterface && mode == ViewportEditorMode::Component)
        {
            ChangeToMode(DefaultActionContextModeIdentifier);
        }
    }

    void ComponentModeActionHandler::EnumerateComponentModes(
        const AZStd::function<bool(const AZ::SerializeContext::ClassData*, const AZ::Uuid&)>& handler)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        serializeContext->EnumerateDerived<ComponentModeFramework::EditorBaseComponentMode>(handler);
    }

    void ComponentModeActionHandler::ChangeToMode(const AZStd::string& modeIdentifier)
    {
        m_actionManagerInterface->SetActiveActionContextMode(EditorMainWindowActionContextIdentifier, modeIdentifier);
        m_actionManagerInterface->TriggerActionUpdater(ComponentModeChangedUpdaterIdentifier);
    }

} // namespace AzToolsFramework
