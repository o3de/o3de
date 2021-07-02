/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mCallback   = nullptr;
        mList       = nullptr;
    }


    ActionHistoryPlugin::~ActionHistoryPlugin()
    {
        if (mCallback)
        {
            EMStudio::GetCommandManager()->RemoveCallback(mCallback, false);
            delete mCallback;
        }

        delete mList;
    }


    const char* ActionHistoryPlugin::GetCompileDate() const
    {
        return MCORE_DATE;
    }


    const char* ActionHistoryPlugin::GetName() const
    {
        return "Action History";
    }


    uint32 ActionHistoryPlugin::GetClassID() const
    {
        return ActionHistoryPlugin::CLASS_ID;
    }


    const char* ActionHistoryPlugin::GetCreatorName() const
    {
        return "O3DE";
    }


    float ActionHistoryPlugin::GetVersion() const
    {
        return 1.0f;
    }


    EMStudioPlugin* ActionHistoryPlugin::Clone()
    {
        ActionHistoryPlugin* newPlugin = new ActionHistoryPlugin();
        return newPlugin;
    }


    // Init after the parent dock window has been created.
    bool ActionHistoryPlugin::Init()
    {
        mList = new QListWidget(mDock);

        mList->setFlow(QListView::TopToBottom);
        mList->setMovement(QListView::Static);
        mList->setViewMode(QListView::ListMode);
        mList->setSelectionRectVisible(true);
        mList->setSelectionBehavior(QAbstractItemView::SelectRows);
        mList->setSelectionMode(QAbstractItemView::SingleSelection);
        mDock->setWidget(mList);

        // Detect item selection changes.
        connect(mList, &QListWidget::itemSelectionChanged, this, &ActionHistoryPlugin::OnSelectedItemChanged);

        // Register the callback.
        mCallback = new ActionHistoryCallback(mList);
        EMStudio::GetCommandManager()->RegisterCallback(mCallback);

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

            historyItemString = MCore::CommandManager::CommandHistoryEntry::ToString(historyItem.mCommandGroup, historyItem.mExecutedCommand, historyItem.m_historyItemNr);
            mList->addItem(new QListWidgetItem(historyItemString.c_str(), mList));
        }

        // Set the current history index in case the user called undo.
        mList->setCurrentRow(commandManager->GetHistoryIndex());
    }


    // Called when the selection changed.
    void ActionHistoryPlugin::OnSelectedItemChanged()
    {
        // Get the list of selected items and make sure exactly one is selected.
        QList<QListWidgetItem*> selected = mList->selectedItems();
        if (selected.count() != 1)
        {
            return;
        }

        // Get the selected item and its index (row number in the list).
        const uint32 index = mList->row(selected.at(0));

        // Change the command index.
        mCallback->OnSetCurrentCommand(index);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/ActionHistory/moc_ActionHistoryPlugin.cpp>
