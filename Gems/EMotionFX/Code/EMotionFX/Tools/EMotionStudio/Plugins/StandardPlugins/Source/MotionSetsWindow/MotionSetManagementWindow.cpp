/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioCore.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetManagementWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionListWindow.h>
#include <MCore/Source/FileSystem.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/LogManager.h>
#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QToolBar>

#include <QMessageBox>

namespace EMStudio
{
    MotionSetManagementRemoveMotionsFailedWindow::MotionSetManagementRemoveMotionsFailedWindow(QWidget* parent, const MCore::Array<EMotionFX::Motion*>& motions)
        : QDialog(parent)
    {
        // set the window title
        setWindowTitle("Remove Motions Failed");

        // resize the window
        resize(720, 405);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the top text
        layout->addWidget(new QLabel("The following motions failed to get removed because they are used by another motion set:"));

        // create the table widget
        QTableWidget* tableWidget = new QTableWidget();
        tableWidget->setAlternatingRowColors(true);
        tableWidget->setGridStyle(Qt::SolidLine);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        tableWidget->setCornerButtonEnabled(false);
        tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // set the table widget columns
        tableWidget->setColumnCount(2);
        QStringList headerLabels;
        headerLabels.append("Name");
        headerLabels.append("FileName");
        tableWidget->setHorizontalHeaderLabels(headerLabels);
        tableWidget->horizontalHeader()->setStretchLastSection(true);
        tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        tableWidget->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
        tableWidget->verticalHeader()->setVisible(false);

        // set the number of rows
        const uint32 numMotions = motions.GetLength();
        tableWidget->setRowCount(numMotions);

        // add each motion in the table
        for (uint32 i = 0; i < numMotions; ++i)
        {
            // get the motion
            EMotionFX::Motion* motion = motions[i];

            // create the name table widget item
            QTableWidgetItem* nameTableWidgetItem = new QTableWidgetItem(motion->GetName());
            nameTableWidgetItem->setToolTip(motion->GetName());

            // create the filename table widget item
            QTableWidgetItem* fileNameTableWidgetItem = new QTableWidgetItem(motion->GetFileName());
            fileNameTableWidgetItem->setToolTip(motion->GetFileName());

            // set the text of the row
            tableWidget->setItem(i, 0, nameTableWidgetItem);
            tableWidget->setItem(i, 1, fileNameTableWidgetItem);

            // set the row height
            tableWidget->setRowHeight(i, 21);
        }

        // resize the first column to contents
        tableWidget->resizeColumnToContents(0);

        // add the table widget in the layout
        layout->addWidget(tableWidget);

        // add the button to close the window
        QPushButton* okButton = new QPushButton("OK");
        connect(okButton, &QPushButton::clicked, this, &MotionSetManagementRemoveMotionsFailedWindow::accept);
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setAlignment(Qt::AlignRight);
        buttonLayout->addWidget(okButton);
        layout->addLayout(buttonLayout);

        // set the layout
        setLayout(layout);
    }


    MotionSetManagementRenameWindow::MotionSetManagementRenameWindow(QWidget* parent, EMotionFX::MotionSet* motionSet)
        : QDialog(parent)
    {
        // store the motion set
        mMotionSet = motionSet;

        // set the window title
        setWindowTitle("Enter new motion set name");

        // set the minimum width
        setMinimumWidth(300);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the line edit
        mLineEdit = new QLineEdit();
        connect(mLineEdit, &QLineEdit::textEdited, this, &MotionSetManagementRenameWindow::TextEdited);
        layout->addWidget(mLineEdit);

        // set the current name and select all
        mLineEdit->setText(motionSet->GetName());
        mLineEdit->selectAll();

        // create add the error message
        /*mErrorMsg = new QLabel("<font color='red'>Error: Duplicate name found</font>");
        mErrorMsg->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mErrorMsg->setVisible(false);*/

        // create the button layout
        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        mOKButton                   = new QPushButton("OK");
        QPushButton* cancelButton   = new QPushButton("Cancel");
        //buttonLayout->addWidget(mErrorMsg);
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(cancelButton);

        // Allow pressing the enter key as alternative to pressing the ok button for faster workflow.
        mOKButton->setAutoDefault(true);
        mOKButton->setDefault(true);

        // connect the buttons
        connect(mOKButton, &QPushButton::clicked, this, &MotionSetManagementRenameWindow::Accepted);
        connect(cancelButton, &QPushButton::clicked, this, &MotionSetManagementRenameWindow::reject);

        // set the new layout
        layout->addLayout(buttonLayout);
        setLayout(layout);
    }


    void MotionSetManagementRenameWindow::TextEdited(const QString& text)
    {
        if (text.isEmpty())
        {
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else if (text == mMotionSet->GetName())
        {
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
        else
        {
            // find duplicate name in all motion sets other than this motion set
            const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (uint32 i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (text == motionSet->GetName())
                {
                    //mErrorMsg->setVisible(true);
                    mOKButton->setEnabled(false);
                    GetManager()->SetWidgetAsInvalidInput(mLineEdit);
                    return;
                }
            }

            // no duplicate name found
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
    }


    void MotionSetManagementRenameWindow::Accepted()
    {
        const AZStd::string commandString = AZStd::string::format("AdjustMotionSet -motionSetID %i -newName \"%s\"", mMotionSet->GetID(), mLineEdit->text().toUtf8().data());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        accept();
    }


    // constructor
    MotionSetManagementWindow::MotionSetManagementWindow(MotionSetsWindowPlugin* parentPlugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin = parentPlugin;
    }


    // destructor
    MotionSetManagementWindow::~MotionSetManagementWindow()
    {
    }


    // init after the parent dock window has been created
    bool MotionSetManagementWindow::Init()
    {
        // create the main aidget and put it to the dialog stack
        QVBoxLayout* layout = new QVBoxLayout();
        setLayout(layout);
        layout->setMargin(0);
        layout->setSpacing(2);

        mMotionSetsTree = new QTreeWidget();

        // set the table to row single selection
        mMotionSetsTree->setSelectionBehavior(QAbstractItemView::SelectRows);
        mMotionSetsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // set the minimum size and the resizing policy
        mMotionSetsTree->setMinimumHeight(150);
        mMotionSetsTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mMotionSetsTree->setColumnCount(1);

        mMotionSetsTree->setAlternatingRowColors(true);
        mMotionSetsTree->setExpandsOnDoubleClick(true);
        mMotionSetsTree->setAnimated(true);
        mMotionSetsTree->setObjectName("EMFX.MotionSetManagementWindow.MotionSetsTree");

        connect(mMotionSetsTree, &QTreeWidget::itemSelectionChanged, this, &MotionSetManagementWindow::OnSelectionChanged);

        QStringList headerList;
        headerList.append("Name");
        mMotionSetsTree->setHeaderLabels(headerList);
        mMotionSetsTree->header()->setSortIndicator(0, Qt::AscendingOrder);

        // disable the move of section to have column order fixed
        mMotionSetsTree->header()->setSectionsMovable(false);

        QToolBar* toolBar = new QToolBar(this);
        toolBar->setObjectName("MotionSetManagementWindow.ToolBar");

        m_addAction = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.svg"),
            tr("Add new motion set"),
            this, &MotionSetManagementWindow::OnCreateMotionSet);
        m_addAction->setObjectName("MotionSetManagementWindow.ToolBar.AddNewMotionSet");

        m_openAction = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.svg"),
            tr("Load motion set from a file"),
            this, &MotionSetManagementWindow::OnOpen);

        m_saveMenuAction = toolBar->addAction(
            MysticQt::GetMysticQt()->FindIcon("Images/Icons/Save.svg"),
            tr("Save selected root motion set"));
        {
            QToolButton* toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(m_saveMenuAction));
            AZ_Assert(toolButton, "The action widget must be a tool button.");
            toolButton->setPopupMode(QToolButton::InstantPopup);

            QMenu* contextMenu = new QMenu(toolBar);

            m_saveAction = contextMenu->addAction("Save", this, &MotionSetManagementWindow::OnSave);
            m_saveAsAction = contextMenu->addAction("Save as...", this, &MotionSetManagementWindow::OnSaveAs);

            m_saveMenuAction->setMenu(contextMenu);
        }

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        toolBar->addWidget(spacerWidget);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &MotionSetManagementWindow::OnTextFilterChanged);
        toolBar->addWidget(m_searchWidget);
        
        layout->addWidget(toolBar);
        layout->addWidget(mMotionSetsTree);

        ReInit();
        UpdateInterface();

        return true;
    }


    void MotionSetManagementWindow::RecursivelyAddSets(QTreeWidgetItem* parent, EMotionFX::MotionSet* motionSet, const AZStd::vector<uint32>& selectedSetIDs)
    {
        // Add the given motion set to the tree widget.
        QTreeWidgetItem* item = new QTreeWidgetItem(parent);
        item->setText(0, motionSet->GetName());
        item->setData(0, Qt::UserRole, motionSet->GetID());
        item->setIcon(0, QIcon(QStringLiteral(":/EMotionFX/MotionSet.svg")));

        // Store the motion set id in the tree widget itm.
        AZStd::string idString;
        AZStd::to_string(idString, motionSet->GetID());
        item->setWhatsThis(0, idString.c_str());

        item->setExpanded(true);
        parent->addChild(item);

        // Check if the motion set is selected and select the item in that case.
        if (AZStd::find(selectedSetIDs.begin(), selectedSetIDs.end(), motionSet->GetID()) != selectedSetIDs.end())
        {
            item->setSelected(true);
        }

        // Hide in case the search field text is not part of the motion set name.
        if (item->text(0).contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive))
        {
            item->setHidden(false);

            // Propagate visibility flag up the hierarchy.
            QTreeWidgetItem* parentItem = item->parent();
            while (parentItem)
            {
                parentItem->setHidden(false);
                parentItem = parentItem->parent();
            }
        }
        else
        {
            item->setHidden(true);
        }

        // Recursively add all child sets.
        const uint32 numChildSets = motionSet->GetNumChildSets();
        for (uint32 i = 0; i < numChildSets; ++i)
        {
            EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
            RecursivelyAddSets(item, childSet, selectedSetIDs);
        }
    }


    void MotionSetManagementWindow::ReInit()
    {
        // Get the selected items in the motion set tree widget..
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // Create and fill an array containing ids of all selected motion sets.
        AZStd::vector<uint32> selectedMotionSetIDs;
        selectedMotionSetIDs.resize(numSelectedItems);
        for (int32 i = 0; i < numSelectedItems; ++i)
        {
            const int motionSetId = AzFramework::StringFunc::ToInt(selectedItems[i]->whatsThis(0).toUtf8().data());
            selectedMotionSetIDs[i] = motionSetId;
        }

        // Set the sorting disabled to avoid index issues.
        mMotionSetsTree->setSortingEnabled(false);

        // Clear all old items.
        mMotionSetsTree->blockSignals(true);
        mMotionSetsTree->clear();

        // Iterate through root motion sets and fill in the table recursively.
        AZStd::string tempString;
        const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            // Only process root motion sets.
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
            if (motionSet->GetParentSet())
            {
                continue;
            }

            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            // add the top level item
            QTreeWidgetItem* item = new QTreeWidgetItem(mMotionSetsTree);
            item->setText(0, motionSet->GetName());
            item->setData(0, Qt::UserRole, motionSet->GetID());
            item->setIcon(0, QIcon(QStringLiteral(":/EMotionFX/MotionSet.svg")));
            item->setExpanded(true);

            AZStd::to_string(tempString, motionSet->GetID());
            item->setWhatsThis(0, tempString.c_str());

            mMotionSetsTree->addTopLevelItem(item);

            // Should the motion set be selected?
            if (AZStd::find(selectedMotionSetIDs.begin(), selectedMotionSetIDs.end(), motionSet->GetID()) != selectedMotionSetIDs.end())
            {
                item->setSelected(true);
            }

            // check if the current item contains the find text
            if (item->text(0).contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive))
            {
                // set the item not hidden
                item->setHidden(false);

                // set each parent not hidden
                QTreeWidgetItem* parent = item->parent();
                while (parent)
                {
                    parent->setHidden(false);
                    parent = parent->parent();
                }
            }
            else
            {
                // set the item hidden
                item->setHidden(true);
            }

            // get the number of children and iterate through them
            const uint32 numChildSets = motionSet->GetNumChildSets();
            for (uint32 j = 0; j < numChildSets; ++j)
            {
                // get the child set
                EMotionFX::MotionSet* childSet = motionSet->GetChildSet(j);

                // recursively go through all sets
                RecursivelyAddSets(item, childSet, selectedMotionSetIDs);
            }
        }

        // enable the tree signals
        mMotionSetsTree->blockSignals(false);

        // enable the sorting
        mMotionSetsTree->setSortingEnabled(true);
    }


    void MotionSetManagementWindow::OnSelectionChanged()
    {
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const uint32 numSelected = selectedItems.count();
        if (numSelected != 1)
        {
            mPlugin->SetSelectedSet(nullptr);
        }
        else
        {
            const uint32 motionSetId = selectedItems[0]->data(0, Qt::UserRole).toUInt();
            EMotionFX::MotionSet* selectedSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetId);

            if (selectedSet)
            {
                mPlugin->SetSelectedSet(selectedSet);
            }
        }
    }


    void MotionSetManagementWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        // create the context menu
        QMenu* menu = new QMenu(this);
        menu->setObjectName("EMFX.MotionSetManagementWindow.ContextMenu");

        // add motion set is always enabled
        QAction* addAction = menu->addAction("Add Motion Set");
        connect(addAction, &QAction::triggered, this, &MotionSetManagementWindow::OnCreateMotionSet);

        // get the selected items
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // add remove if at least one item selected
        if (numSelectedItems > 0)
        {
            QAction* removeAction = menu->addAction("Remove selected");
            removeAction->setObjectName("EMFX.MotionSetManagementWindow.ContextMenu.RemoveSelected");
            connect(removeAction, &QAction::triggered, this, &MotionSetManagementWindow::OnRemoveSelectedMotionSets);

            QAction* removeAllAction = menu->addAction("Remove all");
            connect(removeAllAction, &QAction::triggered, this, &MotionSetManagementWindow::OnClearMotionSets);
        }

        // add rename if only one item selected
        if (numSelectedItems == 1)
        {
            QAction* renameAction = menu->addAction("Rename Selected Motion Set");
            connect(renameAction, &QAction::triggered, this, &MotionSetManagementWindow::OnRenameSelectedMotionSet);
        }

        // add the save menu if at least one item selected
        if (numSelectedItems > 0)
        {
            menu->addSeparator();

            // add the save menu
            QAction* saveAction = menu->addAction("Save Selected Root Motion Set");
            connect(saveAction, &QAction::triggered, this, &MotionSetManagementWindow::OnSave);
        }

        // show the menu at the given position
        menu->popup(event->globalPos());
        connect(menu, &QMenu::triggered, menu, &QMenu::deleteLater);
    }


    void MotionSetManagementWindow::OnCreateMotionSet()
    {
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // only add the motion set as child if at least one item selected
        // if nothing is selected, add the new motion set as root
        if (numSelectedItems == 0)
        {
            // generate the unique name
            const AZStd::string uniqueMotionSetName = MCore::GenerateUniqueString("MotionSet",   
                [&](const AZStd::string& value)
                {
                    return (EMotionFX::GetMotionManager().FindMotionSetIndexByName(value.c_str()) == MCORE_INVALIDINDEX32);
                });

            // Construct the command string.
            const AZStd::string commandString = AZStd::string::format("CreateMotionSet -name \"%s\"", uniqueMotionSetName.c_str());

            // Execute the command.
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommand(commandString, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }

            // Select the new motion set
            mMotionSetsTree->clearSelection();
            const EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByName(uniqueMotionSetName.c_str());
            if (motionSet)
            {
                SelectItemsById(motionSet->GetID());
            }
        }
        else
        {
            // create the command group
            MCore::CommandGroup commandGroup("Create motion sets");

            // add each command
            AZStd::string commandString;
            AZStd::unordered_map<AZStd::string, EMotionFX::MotionSet*> parentMotionSetByName;
            AZStd::string uniqueMotionSetName;
            uint32 selectedMotionSetId = MCORE_INVALIDINDEX32;

            for (int i = 0; i < numSelectedItems; ++i)
            {
                // generate the unique name
                uniqueMotionSetName = MCore::GenerateUniqueString("MotionSet",   
                    [&](const AZStd::string& value)
                    {
                        return (EMotionFX::GetMotionManager().FindMotionSetIndexByName(value.c_str()) == MCORE_INVALIDINDEX32) &&
                            (parentMotionSetByName.find(value) == parentMotionSetByName.end());
                    });

                // Find the selected motion set.
                selectedMotionSetId = selectedItems[i]->data(0, Qt::UserRole).toUInt();
                EMotionFX::MotionSet* selectedSet = EMotionFX::GetMotionManager().FindMotionSetByID(selectedMotionSetId);
                if (selectedSet)
                {
                    commandString = AZStd::string::format("CreateMotionSet -name \"%s\" -parentSetID %i", uniqueMotionSetName.c_str(), selectedSet->GetID());
                    commandGroup.AddCommandString(commandString);

                    // Add the name in the array.
                    // It's needed to generate the unique name
                    parentMotionSetByName.emplace(AZStd::move(uniqueMotionSetName), selectedSet);
                }
            }

            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }

            // Select the new motion sets.
            mMotionSetsTree->clearSelection();
            for (const AZStd::pair<AZStd::string, EMotionFX::MotionSet*>& nameAndParentMotionSet : parentMotionSetByName)
            {
                EMotionFX::MotionSet* motionSet = nameAndParentMotionSet.second->RecursiveFindMotionSetByName(nameAndParentMotionSet.first);
                SelectItemsById(motionSet->GetID());
            }
        }
    }


    void MotionSetManagementWindow::SelectItemsById(uint32 motionSetId)
    {
        bool selectionChanged = false;
        disconnect(mMotionSetsTree, &QTreeWidget::itemSelectionChanged, this, &MotionSetManagementWindow::OnSelectionChanged);
        QTreeWidgetItemIterator it(mMotionSetsTree);
        while (*it)
        {
            if ((*it)->data(0, Qt::UserRole).toUInt() == motionSetId)
            {
                if (!(*it)->isSelected())
                {
                    selectionChanged = true;
                }
                (*it)->setSelected(true);
                break;
            }
            ++it;
        }
        connect(mMotionSetsTree, &QTreeWidget::itemSelectionChanged, this, &MotionSetManagementWindow::OnSelectionChanged);
        if (selectionChanged)
        {
            OnSelectionChanged(); 
        }
    }

    void MotionSetManagementWindow::GetSelectedMotionSets(AZStd::vector<EMotionFX::MotionSet*>& outSelectedMotionSets) const
    {
        // Get the selected items from the motion set tree widget.
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const int numSelectedItems = selectedItems.count();

        outSelectedMotionSets.resize(numSelectedItems);

        // Find the corresponding motion sets and add them to the array.
        for (int32 i = 0; i < numSelectedItems; ++i)
        {
            const int motionSetId = AzFramework::StringFunc::ToInt(selectedItems[i]->whatsThis(0).toUtf8().data());
            outSelectedMotionSets[i] = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetId);
        }
    }


    void MotionSetManagementWindow::RecursiveIncreaseMotionsReferenceCount(EMotionFX::MotionSet* motionSet)
    {
        // Increase the reference counter if needed for each motion.
        const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            // Check the reference counter if only one reference registered.
            // Two is needed because the remove motion command has to be called to have the undo/redo possible
            // without it the motion list is also not updated because the remove motion callback is not called.
            // Also avoid the issue to remove from set but not the motion list, to keep it in the motion list.
            if (motionEntry->GetMotion() && motionEntry->GetMotion()->GetReferenceCount() == 1)
            {
                motionEntry->GetMotion()->IncreaseReferenceCount();
            }
        }

        // Do the same for all child motion sets recursively.
        const uint32 numChildSets = motionSet->GetNumChildSets();
        for (uint32 i = 0; i < numChildSets; ++i)
        {
            EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
            RecursiveIncreaseMotionsReferenceCount(childSet);
        }
    }


    void MotionSetManagementWindow::RecursiveRemoveMotionsFromSet(EMotionFX::MotionSet* motionSet, MCore::CommandGroup& commandGroup, AZStd::vector<EMotionFX::Motion*>& failedRemoveMotions)
    {
        // Recursively remove motions from the all entries in the child motion sets.
        const uint32 numChildSets = motionSet->GetNumChildSets();
        for (uint32 i = 0; i < numChildSets; ++i)
        {
            EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
            RecursiveRemoveMotionsFromSet(childSet, commandGroup, failedRemoveMotions);
        }

        // Iterate through the entries and add the corresponding remove motion command to the command group.
        AZStd::string motionFilename, commandString;
        const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            // Get the motion entry and check if the assigned motion is loaded.
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;
            if (!motionEntry->GetMotion())
            {
                continue;
            }

            motionFilename = motionSet->ConstructMotionFilename(motionEntry);
            commandString = AZStd::string::format("RemoveMotion -filename \"%s\"", motionFilename.c_str());
            commandGroup.AddCommandString(commandString);
        }
    }


    void MotionSetManagementWindow::OnRemoveSelectedMotionSets()
    {
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const uint32 numSelected = selectedItems.count();
        if (numSelected <= 0)
        {
            return;
        }

        // ask to remove motions
        bool removeMotions;
        if (QMessageBox::question(this, "Remove Motions From Project?", "Remove the motions from the project entirely? This would also remove them from the motion list. Pressing no will remove them from the motion set but keep them inside the motion list inside the motions window.", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            removeMotions = true;
        }
        else
        {
            removeMotions = false;
        }

        // create our command group
        MCore::CommandGroup commandGroup("Remove motion sets");

        // Create the failed remove motions array.
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;

        // get the number of selected motion sets and iterate through them
        AZStd::set<AZ::u32> toBeRemoved;
        for (int32 i = numSelected - 1; i >= 0; --i)
        {
            // get the motion set ID
            const uint32 motionSetID = AzFramework::StringFunc::ToInt(FromQtString(selectedItems[i]->whatsThis(0)).c_str());

            // get the current motion set and only process the root sets
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);

            // in case we modified the motion set ask if the user wants to save changes it before removing it
            mPlugin->SaveDirtyMotionSet(motionSet, nullptr, true, false);

            // recursively increase motions reference count
            RecursiveIncreaseMotionsReferenceCount(motionSet);

            // recursively remove motion sets (post order traversing)
            CommandSystem::RecursivelyRemoveMotionSets(motionSet, commandGroup, toBeRemoved);

            // recursively remove motions from motion sets
            if (removeMotions)
            {
                RecursiveRemoveMotionsFromSet(motionSet, commandGroup, failedRemoveMotions);
            }
        }

        // Execute the group command.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // show the window if at least one failed remove motion
        if (!failedRemoveMotions.empty())
        {
            MotionSetRemoveMotionsFailedWindow removeMotionsFailedWindow(this, failedRemoveMotions);
            removeMotionsFailedWindow.exec();
        }
    }


    void MotionSetManagementWindow::OnRenameSelectedMotionSet()
    {
        MotionSetManagementRenameWindow motionSetManagementRenameWindow(this, mPlugin->GetSelectedSet());
        motionSetManagementRenameWindow.exec();
    }

    void MotionSetManagementWindow::OnClearMotionSets()
    {
        // show the save dirty files window before
        if (mPlugin->OnSaveDirtyMotionSets() == DirtyFileManager::CANCELED)
        {
            return;
        }

        // ask to remove motions
        bool removeMotions;
        if (QMessageBox::question(this, "Remove Motions From Project?", "Remove the motions from the project entirely? This would also remove them from the motion list. Pressing no will remove them from the motion set but keep them inside the motion list inside the motions window.", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            removeMotions = true;
        }
        else
        {
            removeMotions = false;
        }

        // Create the command group.
        MCore::CommandGroup commandGroup("Clear motion sets");

        // Increase the reference counter if needed for each motion.
        AZStd::string commandString;
        const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
            for (const auto& item : motionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

                // Check the reference counter if only one reference registered.
                // Two is needed because the remove motion command has to be called to have the undo/redo possible
                // without it the motion list is also not updated because the remove motion callback is not called.
                // Also avoid the issue to remove from set but not the motion list, to keep it in the motion list.
                if (motionEntry->GetMotion() && motionEntry->GetMotion()->GetReferenceCount() == 1)
                {
                    motionEntry->GetMotion()->IncreaseReferenceCount();
                }
            }
        }

        // Clear all motions sets.
        CommandSystem::ClearMotionSetsCommand(&commandGroup);

        // Remove all motions.
        if (removeMotions)
        {
            AZStd::string motionFileName;
            for (uint32 i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
                for (const auto& item : motionEntries)
                {
                    const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

                    if (motionEntry->GetMotion())
                    {
                        motionFileName = motionSet->ConstructMotionFilename(motionEntry);
                        commandString = AZStd::string::format("RemoveMotion -filename \"%s\"", motionFileName.c_str());
                        commandGroup.AddCommandString(commandString);
                    }
                }
            }
        }

        // Execute command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void MotionSetManagementWindow::UpdateInterface()
    {
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // remove and save buttons are valid if at least one item is selected
        const bool atLeastOneItemSelected = numSelectedItems > 0;
        m_saveAction->setEnabled(atLeastOneItemSelected);

        // Filter to only keep the root motion sets from the selected items.
        AZStd::vector<EMotionFX::MotionSet*> selectedRootMotionSets;
        selectedRootMotionSets.reserve(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* rootItem = selectedItems[i];
            while (rootItem->parent())
            {
                rootItem = rootItem->parent();
            }

            const uint32 motionSetID = AzFramework::StringFunc::ToInt(FromQtString(rootItem->whatsThis(0)).c_str());
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
            if (AZStd::find(selectedRootMotionSets.begin(), selectedRootMotionSets.end(), motionSet) == selectedRootMotionSets.end())
            {
                selectedRootMotionSets.push_back(motionSet);
            }
        }

        const bool oneRootSetSelected = selectedRootMotionSets.size() == 1;
        m_saveAsAction->setEnabled(oneRootSetSelected);
    }


    void MotionSetManagementWindow::OnOpen()
    {
        AZStd::string filename = GetMainWindow()->GetFileManager()->LoadMotionSetFileDialog(this);
        GetMainWindow()->activateWindow();
        mPlugin->LoadMotionSet(filename);
    }


    void MotionSetManagementWindow::OnSave()
    {
        // get the selected items and the number of selected items
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // at leat one item must be selected
        if (numSelectedItems == 0)
        {
            return;
        }

        // Filter to only keep the root motion sets from the selected items.
        AZStd::vector<EMotionFX::MotionSet*> selectedRootMotionSets;
        selectedRootMotionSets.reserve(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            // find the root item
            QTreeWidgetItem* rootItem = selectedItems[i];
            while (rootItem->parent())
            {
                rootItem = rootItem->parent();
            }

            // Add the root motion set in the array if not already added.
            const uint32 motionSetID = AzFramework::StringFunc::ToInt(FromQtString(rootItem->whatsThis(0)).c_str());
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
            if (AZStd::find(selectedRootMotionSets.begin(), selectedRootMotionSets.end(), motionSet) == selectedRootMotionSets.end())
            {
                selectedRootMotionSets.push_back(motionSet);
            }
        } 

        // create the command group.
        MCore::CommandGroup commandGroup("Save selected motion sets");
        commandGroup.SetReturnFalseAfterError(true);

        // Add each command.
        const size_t numSelectedRootMotionSets = selectedRootMotionSets.size();
        for (size_t i = 0; i < numSelectedRootMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = selectedRootMotionSets[i];

            // Show a file dialog in case the motion set hasn't been saved yet.
            AZStd::string filename = motionSet->GetFilename();
            if (filename.empty())
            {
                filename = GetMainWindow()->GetFileManager()->SaveMotionSetFileDialog(this);
                if (filename.empty())
                {
                    continue;
                }
            }

            // add the save command
            GetMainWindow()->GetFileManager()->SaveMotionSet(filename.c_str(), motionSet, &commandGroup);
        }

        // execute the command group
        // check the number of commands is needed to avoid notification if nothing needs save
        if (commandGroup.GetNumCommands() > 0)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, 
                    AZStd::string::format("MotionSet <font color=red>failed</font> to save<br/><br/>%s", result.c_str()).c_str());
            }
            else
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, 
                    "MotionSet <font color=green>successfully</font> saved");
            }
        }
    }


    void MotionSetManagementWindow::OnSaveAs()
    {
        // get the selected items and the number of selected items
        const QList<QTreeWidgetItem*> selectedItems = mMotionSetsTree->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // filter to only keep the root motion sets from the selected items
        AZStd::vector<EMotionFX::MotionSet*> selectedRootMotionSets;
        selectedRootMotionSets.reserve(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            // find the root item
            QTreeWidgetItem* rootItem = selectedItems[i];
            while (rootItem->parent())
            {
                rootItem = rootItem->parent();
            }

            // Add the root motion set in the array if not already added.
            const uint32 motionSetID = AzFramework::StringFunc::ToInt(FromQtString(rootItem->whatsThis(0)).c_str());
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
            if (AZStd::find(selectedRootMotionSets.begin(), selectedRootMotionSets.end(), motionSet) == selectedRootMotionSets.end())
            {
                selectedRootMotionSets.push_back(motionSet);
            }
        }

        // only one root motion set must be found
        if (selectedRootMotionSets.size() != 1)
        {
            return;
        }

        // if the motion set doesn't have a filename, the file dialog is shown
        AZStd::string filename = GetMainWindow()->GetFileManager()->SaveMotionSetFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        // save the motion set
        GetMainWindow()->GetFileManager()->SaveMotionSet(filename.c_str(), selectedRootMotionSets[0]);
    }


    void MotionSetManagementWindow::OnTextFilterChanged(const QString& text)
    {
        FromQtString(text, &m_searchWidgetText);
        ReInit();
    }


    void MotionSetManagementWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            OnRemoveSelectedMotionSets();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void MotionSetManagementWindow::keyReleaseEvent(QKeyEvent* event)
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
} // namespace EMStudio
