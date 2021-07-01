/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// inlude required headers
#include "NodeGroupManagementWidget.h"
#include <AzCore/std/containers/vector.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/CommandSystem/Source/NodeGroupCommands.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <MCore/Source/StringConversions.h>

// qt headers
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenu>
#include <QCheckBox>


namespace EMStudio
{
    NodeGroupManagementRenameWindow::NodeGroupManagementRenameWindow(QWidget* parent, EMotionFX::Actor* actor, const AZStd::string& nodeGroupName)
        : QDialog(parent)
    {
        // store the values
        mActor = actor;
        mNodeGroupName = nodeGroupName;

        // set the window title
        setWindowTitle("Rename Node Group");

        // set the minimum width
        setMinimumWidth(300);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the top text
        layout->addWidget(new QLabel("Please enter the new node group name:"));

        // add the line edit
        mLineEdit = new QLineEdit();
        connect(mLineEdit, &QLineEdit::textEdited, this, &NodeGroupManagementRenameWindow::TextEdited);
        layout->addWidget(mLineEdit);

        // set the current name and select all
        mLineEdit->setText(nodeGroupName.c_str());
        mLineEdit->selectAll();

        // create the button layout
        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        mOKButton                   = new QPushButton("OK");
        QPushButton* cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(cancelButton);

        // connect the buttons
        connect(mOKButton, &QPushButton::clicked, this, &NodeGroupManagementRenameWindow::Accepted);
        connect(cancelButton, &QPushButton::clicked, this, &NodeGroupManagementRenameWindow::reject);

        // set the new layout
        layout->addLayout(buttonLayout);
        setLayout(layout);
    }


    void NodeGroupManagementRenameWindow::TextEdited(const QString& text)
    {
        const AZStd::string convertedNewName = text.toUtf8().data();
        if (text.isEmpty())
        {
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else if (mNodeGroupName == convertedNewName)
        {
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
        else
        {
            // find duplicate name, it can't be the same name because we already handle this case before
            EMotionFX::NodeGroup* nodeGroup = mActor->FindNodeGroupByName(convertedNewName.c_str());
            if (nodeGroup)
            {
                mOKButton->setEnabled(false);
                GetManager()->SetWidgetAsInvalidInput(mLineEdit);
                return;
            }

            // no duplicate name found
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
    }


    void NodeGroupManagementRenameWindow::Accepted()
    {
        const AZStd::string convertedNewName = mLineEdit->text().toUtf8().data();

        // execute the command
        AZStd::string outResult;
        AZStd::string command = AZStd::string::format("AdjustNodeGroup -actorID %i -name \"%s\" -newName \"%s\"", mActor->GetID(), mNodeGroupName.c_str(), convertedNewName.c_str());
        if (GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        // accept
        accept();
    }


    // constructor
    NodeGroupManagementWidget::NodeGroupManagementWidget(NodeGroupWidget* nodeGroupWidget, QWidget* parent)
        : QWidget(parent)
    {
        // init the button variables to nullptr
        mAddButton          = nullptr;
        mRemoveButton       = nullptr;
        mClearButton        = nullptr;
        mNodeGroupsTable    = nullptr;
        mSelectedRow        = MCORE_INVALIDINDEX32;

        // set the node group widget
        SetNodeGroupWidget(nodeGroupWidget);

        // init the widget
        Init();
    }


    // the destructor
    NodeGroupManagementWidget::~NodeGroupManagementWidget()
    {
    }


    // init function
    void NodeGroupManagementWidget::Init()
    {
        // create the node groups table
        mNodeGroupsTable = new QTableWidget();

        // create the table widget
        mNodeGroupsTable->setAlternatingRowColors(true);
        mNodeGroupsTable->setCornerButtonEnabled(false);
        mNodeGroupsTable->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mNodeGroupsTable->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row single selection
        mNodeGroupsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        mNodeGroupsTable->setSelectionMode(QAbstractItemView::SingleSelection);

        // set the column count
        mNodeGroupsTable->setColumnCount(3);

        // set header items for the table
        QTableWidgetItem* enabledHeaderItem = new QTableWidgetItem("");
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem("Name");
        QTableWidgetItem* numNodesHeaderItem = new QTableWidgetItem("Num Nodes");
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        numNodesHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mNodeGroupsTable->setHorizontalHeaderItem(0, enabledHeaderItem);
        mNodeGroupsTable->setHorizontalHeaderItem(1, nameHeaderItem);
        mNodeGroupsTable->setHorizontalHeaderItem(2, numNodesHeaderItem);

        // set the first column fixed
        QHeaderView* horizontalHeader = mNodeGroupsTable->horizontalHeader();
        mNodeGroupsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        mNodeGroupsTable->setColumnWidth(0, 19);

        // set the name column width
        mNodeGroupsTable->setColumnWidth(1, 150);

        // set the vertical columns not visible
        QHeaderView* verticalHeader = mNodeGroupsTable->verticalHeader();
        verticalHeader->setVisible(false);

        // set the last column to take the whole space
        horizontalHeader->setStretchLastSection(true);

        // disable editing
        mNodeGroupsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // create buttons
        mAddButton      = new QPushButton();
        mRemoveButton   = new QPushButton();
        mClearButton    = new QPushButton();

        EMStudioManager::MakeTransparentButton(mAddButton,     "Images/Icons/Plus.svg",   "Add a new node group");
        EMStudioManager::MakeTransparentButton(mRemoveButton,  "Images/Icons/Minus.svg",  "Remove selected node groups");
        EMStudioManager::MakeTransparentButton(mClearButton,   "Images/Icons/Clear.svg",  "Remove all node groups");

        // add the buttons to the button layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(mAddButton);
        buttonLayout->addWidget(mRemoveButton);
        buttonLayout->addWidget(mClearButton);

        // create the layouts
        QVBoxLayout* layout = new QVBoxLayout();

        // set spacing and margin properties
        layout->setMargin(0);
        layout->setSpacing(2);

        // add widgets to the main layout
        layout->addLayout(buttonLayout);
        layout->addWidget(mNodeGroupsTable);

        // set the main layout
        setLayout(layout);

        // connect controls to the slots
        connect(mAddButton, &QPushButton::clicked, this, &NodeGroupManagementWidget::AddNodeGroup);
        connect(mRemoveButton, &QPushButton::clicked, this, &NodeGroupManagementWidget::RemoveSelectedNodeGroup);
        connect(mClearButton, &QPushButton::clicked, this, &NodeGroupManagementWidget::ClearNodeGroups);
        connect(mNodeGroupsTable, &QTableWidget::itemSelectionChanged, this, &NodeGroupManagementWidget::UpdateNodeGroupWidget);
        //connect( mNodeGroupsTable, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)), this, SLOT(UpdateNodeGroupWidget(QTableWidgetItem*, QTableWidgetItem*)) );
        //connect( mNodeGroupsTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(NodeGroupeNameDoubleClicked(QTableWidgetItem*)) );
    }


    // used to update the interface
    void NodeGroupManagementWidget::UpdateInterface()
    {
        // check if the current actor exists
        if (mActor == nullptr)
        {
            // remove all rows
            mNodeGroupsTable->setRowCount(0);

            // disable the controls
            mAddButton->setDisabled(true);
            mRemoveButton->setDisabled(true);
            mClearButton->setDisabled(true);

            // stop here
            return;
        }

        // enable/disable the controls
        mAddButton->setDisabled(false);
        const bool disableButtons = mActor->GetNumNodeGroups() == 0;
        mRemoveButton->setDisabled(disableButtons);
        mClearButton->setDisabled(disableButtons);

        // set the row count
        mNodeGroupsTable->setRowCount(mActor->GetNumNodeGroups());

        // fill the table with the existing node groups
        const uint32 numNodeGroups = mActor->GetNumNodeGroups();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // get the nodegroup
            EMotionFX::NodeGroup* nodeGroup = mActor->GetNodeGroup(i);

            // continue if node group does not exist
            if (nodeGroup == nullptr)
            {
                continue;
            }

            // create the checkbox
            QCheckBox* checkbox = new QCheckBox("");
            checkbox->setChecked(nodeGroup->GetIsEnabledOnDefault());
            checkbox->setStyleSheet("background: transparent; padding-left: 3px; max-width: 13px;");

            // connect the checkbox
            connect(checkbox, &QCheckBox::clicked, this, &NodeGroupManagementWidget::checkboxClicked);

            // create table items
            QTableWidgetItem* tableItemGroupName = new QTableWidgetItem(nodeGroup->GetName());
            AZStd::string numGroupString = AZStd::string::format("%i", nodeGroup->GetNumNodes());
            QTableWidgetItem* tableItemNumNodes = new QTableWidgetItem(numGroupString.c_str());

            // add items to the table
            mNodeGroupsTable->setCellWidget(i, 0, checkbox);
            mNodeGroupsTable->setItem(i, 1, tableItemGroupName);
            mNodeGroupsTable->setItem(i, 2, tableItemNumNodes);

            // set the row height
            mNodeGroupsTable->setRowHeight(i, 21);
        }

        // set the old selected row if any one
        if (mSelectedRow != MCORE_INVALIDINDEX32)
        {
            mNodeGroupsTable->setCurrentCell(mSelectedRow, 0);
        }
    }


    // function to set the current actor
    void NodeGroupManagementWidget::SetActor(EMotionFX::Actor* actor)
    {
        // set the new actor
        mActor = actor;

        // update the interface
        UpdateInterface();
    }


    // function to set the node group widget
    void NodeGroupManagementWidget::SetNodeGroupWidget(NodeGroupWidget* nodeGroupWidget)
    {
        // set the node group widget
        mNodeGroupWidget = nodeGroupWidget;
    }


    // updates the node group widget
    void NodeGroupManagementWidget::UpdateNodeGroupWidget()
    {
        // return if no node group widget is set
        if (mNodeGroupWidget == nullptr)
        {
            return;
        }

        // set the node group widget to the actual selection
        mNodeGroupWidget->SetActor(mActor);

        // return if the actor is not valid
        if (mActor == nullptr)
        {
            return;
        }

        // get the current row
        const int currentRow = mNodeGroupsTable->currentRow();

        // check if the row is valid
        if (currentRow != -1)
        {
            // set the current row
            mSelectedRow = currentRow;

            // set the node group
            EMotionFX::NodeGroup* nodeGroup = mActor->FindNodeGroupByName(FromQtString(mNodeGroupsTable->item(mSelectedRow, 1)->text()).c_str());
            mNodeGroupWidget->SetNodeGroup(nodeGroup);
        }
        else
        {
            mNodeGroupWidget->SetNodeGroup(nullptr);
            mSelectedRow = MCORE_INVALIDINDEX32;
        }
    }
    /*void NodeGroupManagementWidget::UpdateNodeGroupWidget(QTableWidgetItem* current, QTableWidgetItem* previous)
    {
        MCORE_UNUSED(previous);

        // return if no node group widget is set
        if (mNodeGroupWidget == nullptr)
            return;

        // set the node group widget to the actual selection
        mNodeGroupWidget->SetActor( mActor );

        if (current)
        {
            // set the current row
            mSelectedRow = current->row();

            // set the node group
            NodeGroup* nodeGroup = mActor->FindNodeGroupByName( FromQtString(mNodeGroupsTable->item(current->row(), 1)->text()).c_str() );
            mNodeGroupWidget->SetNodeGroup( nodeGroup );
        }
        else
        {
            mNodeGroupWidget->SetNodeGroup( nullptr );
            mSelectedRow = MCORE_INVALIDINDEX32;
        }
    }*/


    // called whenever a cell is changed
    /*void NodeGroupManagementWidget::NodeGroupNamesChanged(const QString& text)
    {
        // get the sender widget
        QWidget* senderWidget = (QWidget*)sender();

        // check for duplicates
        const int duplicateFound = SearchTableForString( mNodeGroupsTable, text );

        // mark edit field in red, if entry already exists
        if (duplicateFound >= 0)
            GetManager()->SetWidgetAsInvalidInput( senderWidget );
        else
            senderWidget->setStyleSheet("");
    }*/


    // starts editing
    /*void NodeGroupManagementWidget::NodeGroupeNameDoubleClicked(QTableWidgetItem* item)
    {
        // add new line edit for the selected widget
        QLineEdit* lineEdit = new QLineEdit( mNodeGroupsTable->item(item->row(), 0)->text() );
        mNodeGroupsTable->setCellWidget( item->row(), 0, lineEdit );

        // jump into the edit field
        lineEdit->selectAll();
        lineEdit->setFocus();
        mNodeGroupsTable->setCurrentCell( item->row(), 0 );

        // connect slots for edit finishing and text change
        connect( lineEdit, SIGNAL(editingFinished()), this, SLOT(NodeGroupNameEditingFinished()) );
        connect( lineEdit, SIGNAL(textChanged(QString)), this, SLOT(NodeGroupNamesChanged(QString)) );
    }*/


    // called when editing is finished
    /*void NodeGroupManagementWidget::NodeGroupNameEditingFinished()
    {
        // get the current item
        QTableWidgetItem* item = mNodeGroupsTable->currentItem();

        // get the sender widget
        QLineEdit* senderWidget = (QLineEdit*)sender();

        // return if one of the widgets does not exist
        if (item == nullptr || senderWidget == nullptr)
            return;

        // call commands for name change if name does not exist yet
        if (senderWidget->styleSheet() == "")
        {
            // call command for adding a new node group
            String outResult;
            String command;
            command.Format( "AdjustNodeGroup -actorID %i -name \"%s\" -newName \"%s\"", mActor->GetID(), FromQtString(item->text()).c_str(), FromQtString(senderWidget->text()).c_str() );
            if (EMStudio::GetCommandManager()->ExecuteCommand( command.c_str(), outResult ) == false)
                LogError( outResult.c_str() );
        }
        else
        {
            // delete the line edit
            mNodeGroupsTable->setCellWidget(item->row(), item->column(), nullptr);
        }
    }*/


    // function to add a new node group with the specified name
    void NodeGroupManagementWidget::AddNodeGroup()
    {
        // find the node group index to add
        uint32 groupNumber = 0;
        AZStd::string groupName = AZStd::string::format("UnnamedNodeGroup%i", groupNumber);
        while (SearchTableForString(mNodeGroupsTable, groupName.c_str(), true) >= 0)
        {
            ++groupNumber;
            groupName = AZStd::string::format("UnnamedNodeGroup%i", groupNumber);
        }

        // call command for adding a new node group
        AZStd::string outResult;
        const AZStd::string command = AZStd::string::format("AddNodeGroup -actorID %i -name \"%s\"", mActor->GetID(), groupName.c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        // select the new added row
        const int insertPosition = SearchTableForString(mNodeGroupsTable, groupName.c_str());
        mNodeGroupsTable->selectRow(insertPosition);

        // find insert position
        /*int insertPosition = SearchTableForString( mNodeGroupsTable, groupName.c_str() );
        if (insertPosition >= 0)
            NodeGroupeNameDoubleClicked( mNodeGroupsTable->item(insertPosition, 0) );*/
    }


    // function to remove a nodegroup
    void NodeGroupManagementWidget::RemoveSelectedNodeGroup()
    {
        // set the node group of the node group widget to nullptr
        if (mNodeGroupWidget)
        {
            mNodeGroupWidget->SetNodeGroup(nullptr);
        }

        // return if there is no entry to delete
        const int currentRow = mNodeGroupsTable->currentRow();
        if (currentRow < 0)
        {
            return;
        }

        // get the nodegroup
        QTableWidgetItem* item = mNodeGroupsTable->item(currentRow, 1);
        EMotionFX::NodeGroup* nodeGroup = mActor->FindNodeGroupByName(FromQtString(item->text()).c_str());

        // call command for removing a nodegroup
        AZStd::string outResult;
        const AZStd::string command = AZStd::string::format("RemoveNodeGroup -actorID %i -name \"%s\"", mActor->GetID(), nodeGroup->GetName());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        // selected the next row
        if (currentRow > (mNodeGroupsTable->rowCount() - 1))
        {
            mNodeGroupsTable->selectRow(currentRow - 1);
        }
        else
        {
            mNodeGroupsTable->selectRow(currentRow);
        }
    }


    // rename selected node group
    void NodeGroupManagementWidget::RenameSelectedNodeGroup()
    {
        // get the nodegroup
        QTableWidgetItem* item = mNodeGroupsTable->item(mNodeGroupsTable->currentRow(), 1);
        EMotionFX::NodeGroup* nodeGroup = mActor->FindNodeGroupByName(FromQtString(item->text()).c_str());

        // show the rename window
        NodeGroupManagementRenameWindow nodeGroupManagementRenameWindow(this, mActor, nodeGroup->GetName());
        nodeGroupManagementRenameWindow.exec();
    }


    // function to clear the nodegroups
    void NodeGroupManagementWidget::ClearNodeGroups()
    {
        CommandSystem::ClearNodeGroupsCommand(mActor);
    }


    // checkbox clicked
    void NodeGroupManagementWidget::checkboxClicked(bool checked)
    {
        // get the sender widget and the node group index
        QCheckBox* senderCheckbox = (QCheckBox*)sender();

        // find the checkbox row
        AZStd::string nodeGroupName;
        const int rowCount = mNodeGroupsTable->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            QCheckBox* rowChechbox = (QCheckBox*)mNodeGroupsTable->cellWidget(i, 0);
            if (rowChechbox == senderCheckbox)
            {
                nodeGroupName = mNodeGroupsTable->item(i, 1)->text().toUtf8().data();
            }
        }

        // execute the command
        AZStd::string outResult;
        const AZStd::string command = AZStd::string::format("AdjustNodeGroup -actorID %i -name \"%s\" -enabledOnDefault \"%s\"", 
            mActor->GetID(), 
            nodeGroupName.c_str(), 
            AZStd::to_string(checked).c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }
    }


    // searches the table for the given string
    int NodeGroupManagementWidget::SearchTableForString(QTableWidget* tableWidget, const QString& text, bool ignoreCurrentSelection)
    {
        // get the  table dimensions
        const uint32 numRows = tableWidget->rowCount();
        //const uint32 numCols = tableWidget->columnCount();

        // loop trough the table and check for duplicate
        for (uint32 i = 0; i < numRows; ++i)
        {
            QTableWidgetItem* item = tableWidget->item(i, 1);
            if (item == nullptr)
            {
                continue;
            }
            if (item == tableWidget->currentItem() && ignoreCurrentSelection == false)
            {
                continue;
            }
            if (item->text().compare(text, Qt::CaseInsensitive) == 0)
            {
                return i;
            }
        }

        // not found
        return -1;
    }


    // key press event
    void NodeGroupManagementWidget::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            RemoveSelectedNodeGroup();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    // key release event
    void NodeGroupManagementWidget::keyReleaseEvent(QKeyEvent* event)
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


    // context menu
    void NodeGroupManagementWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        // if the actor is not valid, the node group management is disabled
        if (mActor == nullptr)
        {
            return;
        }

        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mNodeGroupsTable->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();

        // filter the items
        AZStd::vector<uint32> rowIndices;
        rowIndices.reserve(numSelectedItems);
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            const uint32 rowIndex = selectedItems[i]->row();
            if (std::find(rowIndices.begin(), rowIndices.end(), rowIndex) == rowIndices.end())
            {
                rowIndices.push_back(rowIndex);
            }
        }

        // create the context menu
        QMenu menu(this);

        // add node group is always enabled
        QAction* addAction = menu.addAction("Add Node Group");
        connect(addAction, &QAction::triggered, this, &NodeGroupManagementWidget::AddNodeGroup);

        // add remove action
        if (rowIndices.size() > 0)
        {
            QAction* removeAction = menu.addAction("Remove Selected Node Group");
            connect(removeAction, &QAction::triggered, this, &NodeGroupManagementWidget::RemoveSelectedNodeGroup);
        }

        // add rename action if only one selected
        if (rowIndices.size() == 1)
        {
            QAction* renameAction = menu.addAction("Rename Selected Node Group");
            connect(renameAction, &QAction::triggered, this, &NodeGroupManagementWidget::RenameSelectedNodeGroup);
        }

        // show the menu at the given position
        menu.exec(event->globalPos());
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/NodeGroups/moc_NodeGroupManagementWidget.cpp>
