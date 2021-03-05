/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ActorsWindow.h"
#include "SceneManagerPlugin.h"
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorInstanceCommands.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/UnitScaleWindow.h"
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <EMotionFX/Source/ActorManager.h>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QMenu>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>


namespace EMStudio
{
    ActorsWindow::ActorsWindow(SceneManagerPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        m_plugin = plugin;

        // create the layouts
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(2);
        vLayout->setAlignment(Qt::AlignTop);

        // create the tree widget
        m_treeWidget = new QTreeWidget();
        m_treeWidget->setObjectName("IsVisibleTreeView");

        // create header items
        m_treeWidget->setColumnCount(1);
        QStringList headerList;
        headerList.append("Name");
        m_treeWidget->setHeaderLabels(headerList);

        // set optical stuff for the tree
        m_treeWidget->setColumnWidth(0, 200);
        m_treeWidget->setColumnWidth(1, 20);
        m_treeWidget->setColumnWidth(1, 100);
        m_treeWidget->setSortingEnabled(false);
        m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_treeWidget->setMinimumWidth(150);
        m_treeWidget->setMinimumHeight(150);
        m_treeWidget->setAlternatingRowColors(true);
        m_treeWidget->setExpandsOnDoubleClick(true);
        m_treeWidget->setAnimated(true);

        // disable the move of section to have column order fixed
        m_treeWidget->header()->setSectionsMovable(false);
        m_treeWidget->setHeaderHidden(true);

        AzQtComponents::CheckBox::setVisibilityMode(m_treeWidget, true);

        QToolBar* toolBar = new QToolBar(this);

        // Open actors
        {
            QAction* menuAction = toolBar->addAction(
                MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Open.svg"),
                tr("Load actor from asset"));

            QToolButton* toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(menuAction));
            AZ_Assert(toolButton, "The action widget must be a tool button.");
            toolButton->setPopupMode(QToolButton::InstantPopup);

            QMenu* contextMenu = new QMenu(toolBar);

            m_loadActorAction = contextMenu->addAction("Load actor", GetMainWindow(), &MainWindow::OnFileOpenActor);
            m_mergeActorAction = contextMenu->addAction("Merge actor", GetMainWindow(), &MainWindow::OnFileMergeActor);

            menuAction->setMenu(contextMenu);
        }

        m_createInstanceAction = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Plus.svg"),
            tr("Create a new instance of the selected actors"),
            this, &ActorsWindow::OnCreateInstanceButtonClicked);

        m_saveAction = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Save.svg"),
            tr("Save selected actors"),
            GetMainWindow(), &MainWindow::OnFileSaveSelectedActors);

        vLayout->addWidget(toolBar);
        vLayout->addWidget(m_treeWidget);

        connect(m_treeWidget, &QTreeWidget::itemChanged, this, &ActorsWindow::OnVisibleChanged);
        connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &ActorsWindow::OnSelectionChanged);

        setLayout(vLayout);
    }


    void ActorsWindow::ReInit()
    {
        // disable signals and clear the tree widget
        m_treeWidget->blockSignals(true);
        m_treeWidget->clear();

        // iterate trough all actors and add them to the tree including their instances
        const uint32 numActors = EMotionFX::GetActorManager().GetNumActors();
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActors; ++i)
        {
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            // ignore visualization actors
            if (actor->GetIsUsedForVisualization())
            {
                continue;
            }

            // ignore engine actors
            if (actor->GetIsOwnedByRuntime())
            {
                continue;
            }

            // create a tree item for the new attachment
            QTreeWidgetItem* newItem = new QTreeWidgetItem(m_treeWidget);

            // add checkboxes to the treeitem
            newItem->setFlags(newItem->flags() | Qt::ItemIsUserCheckable);
            newItem->setCheckState(0, Qt::Checked);

            // adjust text of the treeitem
            AzFramework::StringFunc::Path::GetFileName(actor->GetFileName(), m_tempString);
            m_tempString += AZStd::string::format(" (ID: %i)", actor->GetID());
            newItem->setText(0, m_tempString.c_str());
            newItem->setData(0, Qt::UserRole, actor->GetID());
            newItem->setExpanded(true);

            if (actor->GetDirtyFlag())
            {
                QFont font = newItem->font(0);
                font.setItalic(true);
                newItem->setFont(0, font);
            }

            // add as top level item
            m_treeWidget->addTopLevelItem(newItem);

            for (uint32 k = 0; k < numActorInstances; ++k)
            {
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(k);
                if (actorInstance->GetActor() == actor && !actorInstance->GetIsOwnedByRuntime())
                {
                    // create a tree item for the new attachment
                    QTreeWidgetItem* newChildItem = new QTreeWidgetItem(newItem);

                    // add checkboxes to the treeitem
                    newChildItem->setFlags(newChildItem->flags() | Qt::ItemIsUserCheckable);

                    // adjust text of the treeitem
                    newChildItem->setText(0, tr("Instance (ID: %1)").arg(actorInstance->GetID()));
                    newChildItem->setData(0, Qt::UserRole, actorInstance->GetID());
                    newChildItem->setExpanded(true);

                    // add as top level item
                    newItem->addChild(newChildItem);
                }
            }
        }

        // enable signals
        m_treeWidget->blockSignals(false);
    }


    void ActorsWindow::UpdateInterface()
    {
        // get the current selection
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        // disable signals
        m_treeWidget->blockSignals(true);

        const uint32 numTopLevelItems = m_treeWidget->topLevelItemCount();
        for (uint32 i = 0; i < numTopLevelItems; ++i)
        {
            bool                atLeastOneInstanceVisible   = false;
            QTreeWidgetItem*    item                        = m_treeWidget->topLevelItem(i);
            const uint32        actorID                     = GetIDFromTreeItem(item);
            EMotionFX::Actor*   actor                       = EMotionFX::GetActorManager().FindActorByID(actorID);
            const bool          actorSelected               = selection.CheckIfHasActor(actor);

            item->setSelected(actorSelected);

            const uint32 numChildren = item->childCount();
            for (uint32 j = 0; j < numChildren; ++j)
            {
                QTreeWidgetItem* child = item->child(j);
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(GetIDFromTreeItem(child));
                if (actorInstance)
                {
                    const bool actorInstSelected  = selection.CheckIfHasActorInstance(actorInstance);
                    const bool actorInstVisible   = (actorInstance->GetRender());
                    child->setSelected(actorInstSelected);
                    child->setCheckState(0, actorInstVisible ? Qt::Checked : Qt::Unchecked);

                    if (actorInstVisible)
                    {
                        atLeastOneInstanceVisible = true;
                    }
                }
            }

            item->setCheckState(0, atLeastOneInstanceVisible ? Qt::Checked : Qt::Unchecked);
        }

        // enable signals
        m_treeWidget->blockSignals(false);

        // toggle enabled state of the add/remove buttons
        SetControlsEnabled();
    }


    void ActorsWindow::OnRemoveButtonClicked()
    {
        MCore::CommandGroup commandGroup("Remove Actors/ActorInstances");

        AZStd::vector<EMotionFX::Actor*> toBeRemovedActors;

        const QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();
        const uint32 numItems = items.length();
        for (uint32 i = 0; i < numItems; ++i)
        {
            QTreeWidgetItem* item = items[i];
            if (!item)
            {
                continue;
            }

            // check if the parent is not valid, on this case it's not an instance
            QTreeWidgetItem* parent = item->parent();
            if (!parent)
            {
                const uint32 actorId = GetIDFromTreeItem(item);
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorId);
                if (actor)
                {
                    // remove actor instances
                    const uint32 numChildren = item->childCount();
                    for (uint32 j = 0; j < numChildren; ++j)
                    {
                        QTreeWidgetItem* child = item->child(j);

                        m_tempString = AZStd::string::format("RemoveActorInstance -actorInstanceID %i", GetIDFromTreeItem(child));
                        commandGroup.AddCommandString(m_tempString);
                    }

                    // remove the actor
                    m_tempString = AZStd::string::format("RemoveActor -actorID %i", actorId);
                    commandGroup.AddCommandString(m_tempString);
                
                    toBeRemovedActors.emplace_back(actor);
                }
            }
            else
            {
                // remove the actor instance
                if (!parent->isSelected())
                {
                    m_tempString = AZStd::string::format("RemoveActorInstance -actorInstanceID %i", GetIDFromTreeItem(item));
                    commandGroup.AddCommandString(m_tempString);
                }
            }
        }

        // Ask the user if he wants to save the actor in case it got modified and is about to be removed.
        for (EMotionFX::Actor* actor : toBeRemovedActors)
        {
            m_plugin->SaveDirtyActor(actor, &commandGroup, true, false);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        m_plugin->ReInit();
    }


    void ActorsWindow::OnClearButtonClicked()
    {
        // ask the user if he really wants to remove all actors and their instances
        if (QMessageBox::question(this, "Remove All Actors And Actor Instances?", "Are you sure you want to remove all actors and all actor instances?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        // show the save dirty files window before
        if (GetMainWindow()->GetDirtyFileManager()->SaveDirtyFiles(SaveDirtyActorFilesCallback::TYPE_ID) == DirtyFileManager::CANCELED)
        {
            return;
        }

        // clear the scene
        CommandSystem::ClearScene(true, true);
    }


    void ActorsWindow::OnCreateInstanceButtonClicked()
    {
        // create the command group
        MCore::CommandGroup commandGroup("Create actor instances");

        // the actor ID array
        AZStd::vector<uint32> actorIDs;

        // filter the list to keep the actor items only
        const QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();
        const uint32 numItems = items.length();
        for (uint32 i = 0; i < numItems; ++i)
        {
            // get the item and check if the item is valid
            QTreeWidgetItem* item = items[i];
            if (item == nullptr)
            {
                continue;
            }

            // check if the item is a root
            if (item->parent() == nullptr)
            {
                const uint32 id = GetIDFromTreeItem(item);
                if (id != MCORE_INVALIDINDEX32)
                {
                    actorIDs.push_back(id);
                }
            }
        }

        // clear the current selection
        commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");

        // create instances for all selected actors
        AZStd::string command;
        const size_t numIds = actorIDs.size();
        for (size_t i = 0; i < numIds; ++i)
        {
            command = AZStd::string::format("CreateActorInstance -actorID %i", actorIDs[i]);
            commandGroup.AddCommandString(command);
        }

        // execute the group command
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // update the interface
        m_plugin->UpdateInterface();
    }


    void ActorsWindow::OnVisibleChanged(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(column);

        if (!item)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Adjust actor instances");

        if (!item->parent())
        {
            const uint32 numChildren = item->childCount();
            for (uint32 i = 0; i < numChildren; ++i)
            {
                QTreeWidgetItem* child = item->child(i);

                m_tempString = AZStd::string::format("AdjustActorInstance -actorInstanceId %i -doRender %s", GetIDFromTreeItem(child), AZStd::to_string(item->checkState(0) == Qt::Checked).c_str());
                commandGroup.AddCommandString(m_tempString);
            }
        }
        else
        {
            m_tempString = AZStd::string::format("AdjustActorInstance -actorInstanceId %i -doRender %s", GetIDFromTreeItem(item), AZStd::to_string(item->checkState(0) == Qt::Checked).c_str());
            commandGroup.AddCommandString(m_tempString);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        m_plugin->UpdateInterface();
    }


    void ActorsWindow::OnSelectionChanged()
    {
        // create the command group
        MCore::CommandGroup commandGroup("Select actors/actor instances");

        // make sure selection is unlocked
        bool selectionLocked = GetCommandManager()->GetLockSelection();
        if (selectionLocked)
        {
            commandGroup.AddCommandString("ToggleLockSelection");
        }

        // get the selected items
        const uint32 numTopItems = m_treeWidget->topLevelItemCount();
        for (uint32 i = 0; i < numTopItems; ++i)
        {
            // selection of the topLevelItems
            QTreeWidgetItem* topLevelItem = m_treeWidget->topLevelItem(i);
            if (topLevelItem->isSelected())
            {
                m_tempString = AZStd::string::format("Select -actorID %i", GetIDFromTreeItem(topLevelItem));
                commandGroup.AddCommandString(m_tempString);
            }
            else
            {
                m_tempString = AZStd::string::format("Unselect -actorID %i", GetIDFromTreeItem(topLevelItem));
                commandGroup.AddCommandString(m_tempString);
            }

            // loop trough the children and adjust selection there
            uint32 numChilds = topLevelItem->childCount();
            for (uint32 j = 0; j < numChilds; ++j)
            {
                QTreeWidgetItem* child = topLevelItem->child(j);
                if (child->isSelected())
                {
                    m_tempString = AZStd::string::format("Select -actorInstanceID %i", GetIDFromTreeItem(child));
                    commandGroup.AddCommandString(m_tempString);
                }
                else
                {
                    m_tempString = AZStd::string::format("Unselect -actorInstanceID %i", GetIDFromTreeItem(child));
                    commandGroup.AddCommandString(m_tempString);
                }
            }
        }

        // execute the command group
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // update enabled flag for the add/remove instance buttons
        SetControlsEnabled();
    }


    void ActorsWindow::OnResetTransformationOfSelectedActorInstances()
    {
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand("ResetToBindPose", result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void ActorsWindow::OnCloneSelected()
    {
        CommandSystem::CloneSelectedActorInstances();
    }


    void ActorsWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        const QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();

        // get number of selected items and top level items
        const uint32 numSelected = items.size();
        const uint32 numTopItems = m_treeWidget->topLevelItemCount();

        // create the context menu
        QMenu menu(this);
        menu.setToolTipsVisible(true);

        bool actorSelected = false;
        for (uint32 i = 0; i < numSelected; ++i)
        {
            QTreeWidgetItem* item = items[i];
            if (item->parent() == nullptr)
            {
                actorSelected = true;
                break;
            }
        }

        bool instanceSelected = false;
        const int selectedItemCount = items.count();
        for (int i = 0; i < selectedItemCount; ++i)
        {
            if (items[i]->parent())
            {
                instanceSelected = true;
                break;
            }
        }

        if (numSelected > 0)
        {
            if (instanceSelected)
            {
                QAction* resetTransformationAction = menu.addAction("Reset transforms");
                connect(resetTransformationAction, &QAction::triggered, this, &ActorsWindow::OnResetTransformationOfSelectedActorInstances);

                menu.addSeparator();

                QAction* hideAction = menu.addAction("Hide selected instance");
                connect(hideAction, &QAction::triggered, this, &ActorsWindow::OnHideSelected);

                QAction* unhideAction = menu.addAction("Show selected instance");
                connect(unhideAction, &QAction::triggered, this, &ActorsWindow::OnUnhideSelected);

                menu.addSeparator();
            }

            if (instanceSelected)
            {
                QAction* cloneAction = menu.addAction("Copy selected");
                connect(cloneAction, &QAction::triggered, this, &ActorsWindow::OnCloneSelected);
            }

            QAction* removeAction = menu.addAction("Remove selected");
            connect(removeAction, &QAction::triggered, this, &ActorsWindow::OnRemoveButtonClicked);
        }

        if (numTopItems > 0)
        {
            QAction* clearAction = menu.addAction("Remove all");
            connect(clearAction, &QAction::triggered, this, &ActorsWindow::OnClearButtonClicked);
        }

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    void ActorsWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            OnRemoveButtonClicked();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void ActorsWindow::keyReleaseEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            event->accept();
            return;
        }

        // base class
        QWidget::keyReleaseEvent(event);
    }


    void ActorsWindow::SetControlsEnabled()
    {
        // check if table widget was set
        if (m_treeWidget == nullptr)
        {
            return;
        }

        // get number of selected items and top level items
        const QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();
        const uint32 numSelected = items.size();
        const uint32 numTopItems = m_treeWidget->topLevelItemCount();

        // check if at least one actor selected
        bool actorSelected = false;
        for (uint32 i = 0; i < numSelected; ++i)
        {
            QTreeWidgetItem* item = items[i];
            if (item->parent() == nullptr)
            {
                actorSelected = true;
                break;
            }
        }

        // set the enabled state of the buttons
        m_createInstanceAction->setEnabled(actorSelected);
        m_saveAction->setEnabled(numSelected != 0);
    }


    void ActorsWindow::SetVisibilityFlags(bool isVisible)
    {
        MCore::CommandGroup commandGroup(isVisible ? "Unhide actor instances" : "Hide actor instances");

        // create the instances of the selected actors
        const QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();
        const uint32 numItems = items.length();
        for (uint32 i = 0; i < numItems; ++i)
        {
            // check if parent or child item
            QTreeWidgetItem* item = items[i];
            if (item == nullptr || item->parent() == nullptr)
            {
                continue;
            }

            // extract the id from the given item
            const uint32 id = GetIDFromTreeItem(item);

            m_tempString = AZStd::string::format("AdjustActorInstance -actorInstanceId %i -doRender %s", id, AZStd::to_string(isVisible).c_str());
            commandGroup.AddCommandString(m_tempString);
        }

        // execute the command group
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // update the interface
        m_plugin->UpdateInterface();
    }


    uint32 ActorsWindow::GetIDFromTreeItem(QTreeWidgetItem* item)
    {
        if (item == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }
        return item->data(0, Qt::UserRole).toInt();
    }
} // namespace EMStudio
