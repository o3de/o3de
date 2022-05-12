/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ContextMenuHandlers.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <QAction>
#include <QMenu>

void ContextMenuBottomHandler::Setup()
{
    AzToolsFramework::EditorContextMenuBus::Handler::BusConnect();
}

void ContextMenuBottomHandler::Teardown()
{
    AzToolsFramework::EditorContextMenuBus::Handler::BusDisconnect();
}

int ContextMenuBottomHandler::GetMenuPosition() const
{
    return aznumeric_cast<int>(AzToolsFramework::EditorContextMenuOrdering::BOTTOM);
}

void ContextMenuBottomHandler::PopulateEditorGlobalContextMenu(
    QMenu* menu, [[maybe_unused]] const AZ::Vector2& point, [[maybe_unused]] int flags)
{
    AzToolsFramework::EntityIdList selected;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
        selected, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    QAction* action = nullptr;

    if (selected.size() > 0)
    {
        action = menu->addAction(QObject::tr("Open pinned Inspector"));
        QObject::connect(
            action, &QAction::triggered, action,
            [selected]
            {
                AzToolsFramework::EntityIdSet pinnedEntities(selected.begin(), selected.end());
                AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::OpenPinnedInspector, pinnedEntities);
            }
        );

        menu->addSeparator();
    }
}
