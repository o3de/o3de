/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ContextMenuHandlers.h>

#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <QAction>
#include <QMenu>

void EditorContextMenuHandler::Setup()
{
    AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
}

void EditorContextMenuHandler::Teardown()
{
    AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
}

void EditorContextMenuHandler::OnMenuBindingHook()
{
    auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
    if (!menuManagerInterface)
    {
        return;
    }

    // Entity Outliner Context Menu
    menuManagerInterface->AddActionToMenu(
        EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.entity.openPinnedInspector", 50100);

    // Viewport Context Menu
    menuManagerInterface->AddActionToMenu(
        EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.entity.openPinnedInspector", 50100);
}

void EditorContextMenuHandler::OnActionRegistrationHook()
{
    auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
    if (!actionManagerInterface)
    {
        return;
    }

    // Open Pinned Inspector
    {
        const AZStd::string_view actionIdentifier = "o3de.action.entity.openPinnedInspector";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Open Pinned Inspector";
        actionProperties.m_description = "Open a new instance of the Entity Inspector for the current selection.";
        actionProperties.m_category = "Edit";

        actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []()
            {
                AzToolsFramework::EntityIdList selectedEntities;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                AzToolsFramework::EntityIdSet pinnedEntities(selectedEntities.begin(), selectedEntities.end());
                AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::OpenPinnedInspector, pinnedEntities);
            }
        );

        actionManagerInterface->InstallEnabledStateCallback(
            actionIdentifier,
            []() -> bool
            {
                int selectedEntitiesCount;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntitiesCount, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntitiesCount);

                return selectedEntitiesCount > 0;
            }
        );

        // Trigger update whenever entity selection changes.
        actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }
}
