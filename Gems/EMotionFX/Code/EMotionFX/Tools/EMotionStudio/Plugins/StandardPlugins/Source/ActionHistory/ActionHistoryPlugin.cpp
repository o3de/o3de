/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ActionHistoryPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <QDockWidget>
#include <QListWidget>
#include <EMotionFX/CommandSystem/Source/MiscCommands.h>


namespace EMStudio
{
    ActionHistoryPlugin::ActionHistoryPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        m_callback   = nullptr;
        m_list       = nullptr;
    }


    ActionHistoryPlugin::~ActionHistoryPlugin()
    {
        if (m_callback)
        {
            EMStudio::GetCommandManager()->RemoveCallback(m_callback, false);
            delete m_callback;
        }

        delete m_list;
    }


    const char* ActionHistoryPlugin::GetName() const
    {
        return "Action History";
    }


    uint32 ActionHistoryPlugin::GetClassID() const
    {
        return ActionHistoryPlugin::CLASS_ID;
    }


    // Init after the parent dock window has been created.
    bool ActionHistoryPlugin::Init()
    {
        m_list = new QListWidget(m_dock);

        m_list->setFlow(QListView::TopToBottom);
        m_list->setMovement(QListView::Static);
        m_list->setViewMode(QListView::ListMode);
        m_list->setSelectionRectVisible(true);
        m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_list->setSelectionMode(QAbstractItemView::SingleSelection);
        m_dock->setWidget(m_list);

        // Detect item selection changes.
        connect(m_list, &QListWidget::itemSelectionChanged, this, &ActionHistoryPlugin::OnSelectedItemChanged);

        // Register the callback.
        m_callback = new ActionHistoryCallback(m_list);
        EMStudio::GetCommandManager()->RegisterCallback(m_callback);

        // Sync the interface with the actual command history.
        ReInit();
        return true;
    }


    void ActionHistoryPlugin::ReInit()
    {
        CommandSystem::CommandManager* commandManager = EMStudio::GetCommandManager();
        AZStd::string historyItemString;

        // Add all history items to the action history list.
        const size_t numHistoryItems = commandManager->GetNumHistoryItems();
        for (size_t i = 0; i < numHistoryItems; ++i)
        {
            const MCore::CommandManager::CommandHistoryEntry& historyItem = commandManager->GetHistoryItem(i);

            historyItemString = MCore::CommandManager::CommandHistoryEntry::ToString(historyItem.m_commandGroup, historyItem.m_executedCommand, historyItem.m_historyItemNr);
            m_list->addItem(new QListWidgetItem(historyItemString.c_str(), m_list));
        }

        // Set the current history index in case the user called undo.
        m_list->setCurrentRow(static_cast<int>(commandManager->GetHistoryIndex()));
    }


    // Called when the selection changed.
    void ActionHistoryPlugin::OnSelectedItemChanged()
    {
        // Get the list of selected items and make sure exactly one is selected.
        QList<QListWidgetItem*> selected = m_list->selectedItems();
        if (selected.count() != 1)
        {
            return;
        }

        // Get the selected item and its index (row number in the list).
        const uint32 index = m_list->row(selected.at(0));

        // Change the command index.
        m_callback->OnSetCurrentCommand(index);
    }
} // namespace EMStudio
