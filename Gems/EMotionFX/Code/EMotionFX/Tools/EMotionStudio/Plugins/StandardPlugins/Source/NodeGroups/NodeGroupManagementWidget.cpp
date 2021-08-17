/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_actor = actor;
        m_nodeGroupName = nodeGroupName;

        // set the window title
        setWindowTitle("Rename Node Group");

        // set the minimum width
        setMinimumWidth(300);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the top text
        layout->addWidget(new QLabel("Please enter the new node group name:"));

        // add the line edit
        m_lineEdit = new QLineEdit();
        connect(m_lineEdit, &QLineEdit::textEdited, this, &NodeGroupManagementRenameWindow::TextEdited);
        layout->addWidget(m_lineEdit);

        // set the current name and select all
        m_lineEdit->setText(nodeGroupName.c_str());
        m_lineEdit->selectAll();

        // create the button layout
        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        m_okButton                   = new QPushButton("OK");
        QPushButton* cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(cancelButton);

        // connect the buttons
        connect(m_okButton, &QPushButton::clicked, this, &NodeGroupManagementRenameWindow::Accepted);
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
            m_okButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(m_lineEdit);
        }
        else if (m_nodeGroupName == convertedNewName)
        {
            m_okButton->setEnabled(true);
            m_lineEdit->setStyleSheet("");
        }
        else
        {
            // find duplicate name, it can't be the same name because we already handle this case before
            EMotionFX::NodeGroup* nodeGroup = m_actor->FindNodeGroupByName(convertedNewName.c_str());
            if (nodeGroup)
            {
                m_okButton->setEnabled(false);
                GetManager()->SetWidgetAsInvalidInput(m_lineEdit);
                return;
            }

            // no duplicate name found
            m_okButton->setEnabled(true);
            m_lineEdit->setStyleSheet("");
        }
    }


    void NodeGroupManagementRenameWindow::Accepted()
    {
        const AZStd::string convertedNewName = m_lineEdit->text().toUtf8().data();

        // execute the command
        AZStd::string outResult;
        auto* command = aznew CommandSystem::CommandAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandSystem::CommandAdjustNodeGroup::s_commandName),
            /*actorId=*/ m_actor->GetID(),
            /*name=*/ m_nodeGroupName,
            /*newName=*/ convertedNewName
        );
        if (GetCommandManager()->ExecuteCommand(command, outResult) == false)
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
        m_addButton          = nullptr;
        m_removeButton       = nullptr;
        m_clearButton        = nullptr;
        m_nodeGroupsTable    = nullptr;
        m_selectedRow        = MCORE_INVALIDINDEX32;

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
        m_nodeGroupsTable = new QTableWidget();

        // create the table widget
        m_nodeGroupsTable->setAlternatingRowColors(true);
        m_nodeGroupsTable->setCornerButtonEnabled(false);
        m_nodeGroupsTable->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_nodeGroupsTable->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row single selection
        m_nodeGroupsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_nodeGroupsTable->setSelectionMode(QAbstractItemView::SingleSelection);

        // set the column count
        m_nodeGroupsTable->setColumnCount(3);

        // set header items for the table
        QTableWidgetItem* enabledHeaderItem = new QTableWidgetItem("");
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem("Name");
        QTableWidgetItem* numNodesHeaderItem = new QTableWidgetItem("Num Nodes");
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        numNodesHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_nodeGroupsTable->setHorizontalHeaderItem(0, enabledHeaderItem);
        m_nodeGroupsTable->setHorizontalHeaderItem(1, nameHeaderItem);
        m_nodeGroupsTable->setHorizontalHeaderItem(2, numNodesHeaderItem);

        // set the first column fixed
        QHeaderView* horizontalHeader = m_nodeGroupsTable->horizontalHeader();
        m_nodeGroupsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        m_nodeGroupsTable->setColumnWidth(0, 19);

        // set the name column width
        m_nodeGroupsTable->setColumnWidth(1, 150);

        // set the vertical columns not visible
        QHeaderView* verticalHeader = m_nodeGroupsTable->verticalHeader();
        verticalHeader->setVisible(false);

        // set the last column to take the whole space
        horizontalHeader->setStretchLastSection(true);

        // disable editing
        m_nodeGroupsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // create buttons
        m_addButton      = new QPushButton();
        m_removeButton   = new QPushButton();
        m_clearButton    = new QPushButton();

        EMStudioManager::MakeTransparentButton(m_addButton,     "Images/Icons/Plus.svg",   "Add a new node group");
        EMStudioManager::MakeTransparentButton(m_removeButton,  "Images/Icons/Minus.svg",  "Remove selected node groups");
        EMStudioManager::MakeTransparentButton(m_clearButton,   "Images/Icons/Clear.svg",  "Remove all node groups");

        // add the buttons to the button layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_removeButton);
        buttonLayout->addWidget(m_clearButton);

        // create the layouts
        QVBoxLayout* layout = new QVBoxLayout();

        // set spacing and margin properties
        layout->setMargin(0);
        layout->setSpacing(2);

        // add widgets to the main layout
        layout->addLayout(buttonLayout);
        layout->addWidget(m_nodeGroupsTable);

        // set the main layout
        setLayout(layout);

        // connect controls to the slots
        connect(m_addButton, &QPushButton::clicked, this, &NodeGroupManagementWidget::AddNodeGroup);
        connect(m_removeButton, &QPushButton::clicked, this, &NodeGroupManagementWidget::RemoveSelectedNodeGroup);
        connect(m_clearButton, &QPushButton::clicked, this, &NodeGroupManagementWidget::ClearNodeGroups);
        connect(m_nodeGroupsTable, &QTableWidget::itemSelectionChanged, this, &NodeGroupManagementWidget::UpdateNodeGroupWidget);
    }


    // used to update the interface
    void NodeGroupManagementWidget::UpdateInterface()
    {
        // check if the current actor exists
        if (m_actor == nullptr)
        {
            // remove all rows
            m_nodeGroupsTable->setRowCount(0);

            // disable the controls
            m_addButton->setDisabled(true);
            m_removeButton->setDisabled(true);
            m_clearButton->setDisabled(true);

            // stop here
            return;
        }

        // enable/disable the controls
        m_addButton->setDisabled(false);
        const bool disableButtons = m_actor->GetNumNodeGroups() == 0;
        m_removeButton->setDisabled(disableButtons);
        m_clearButton->setDisabled(disableButtons);

        // set the row count
        m_nodeGroupsTable->setRowCount(aznumeric_caster(m_actor->GetNumNodeGroups()));

        // fill the table with the existing node groups
        const size_t numNodeGroups = m_actor->GetNumNodeGroups();
        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            // get the nodegroup
            EMotionFX::NodeGroup* nodeGroup = m_actor->GetNodeGroup(i);

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
            AZStd::string numGroupString = AZStd::string::format("%zu", nodeGroup->GetNumNodes());
            QTableWidgetItem* tableItemNumNodes = new QTableWidgetItem(numGroupString.c_str());

            // add items to the table
            m_nodeGroupsTable->setCellWidget(aznumeric_caster(i), 0, checkbox);
            m_nodeGroupsTable->setItem(aznumeric_caster(i), 1, tableItemGroupName);
            m_nodeGroupsTable->setItem(aznumeric_caster(i), 2, tableItemNumNodes);

            // set the row height
            m_nodeGroupsTable->setRowHeight(aznumeric_caster(i), 21);
        }

        // set the old selected row if any one
        if (m_selectedRow != MCORE_INVALIDINDEX32)
        {
            m_nodeGroupsTable->setCurrentCell(m_selectedRow, 0);
        }
    }


    // function to set the current actor
    void NodeGroupManagementWidget::SetActor(EMotionFX::Actor* actor)
    {
        // set the new actor
        m_actor = actor;

        // update the interface
        UpdateInterface();
    }


    // function to set the node group widget
    void NodeGroupManagementWidget::SetNodeGroupWidget(NodeGroupWidget* nodeGroupWidget)
    {
        // set the node group widget
        m_nodeGroupWidget = nodeGroupWidget;
    }


    // updates the node group widget
    void NodeGroupManagementWidget::UpdateNodeGroupWidget()
    {
        // return if no node group widget is set
        if (m_nodeGroupWidget == nullptr)
        {
            return;
        }

        // set the node group widget to the actual selection
        m_nodeGroupWidget->SetActor(m_actor);

        // return if the actor is not valid
        if (m_actor == nullptr)
        {
            return;
        }

        // get the current row
        const int currentRow = m_nodeGroupsTable->currentRow();

        // check if the row is valid
        if (currentRow != -1)
        {
            // set the current row
            m_selectedRow = currentRow;

            // set the node group
            EMotionFX::NodeGroup* nodeGroup = m_actor->FindNodeGroupByName(FromQtString(m_nodeGroupsTable->item(m_selectedRow, 1)->text()).c_str());
            m_nodeGroupWidget->SetNodeGroup(nodeGroup);
        }
        else
        {
            m_nodeGroupWidget->SetNodeGroup(nullptr);
            m_selectedRow = MCORE_INVALIDINDEX32;
        }
    }


    // function to add a new node group with the specified name
    void NodeGroupManagementWidget::AddNodeGroup()
    {
        // find the node group index to add
        uint32 groupNumber = 0;
        AZStd::string groupName = AZStd::string::format("UnnamedNodeGroup%i", groupNumber);
        while (SearchTableForString(m_nodeGroupsTable, groupName.c_str(), true) >= 0)
        {
            ++groupNumber;
            groupName = AZStd::string::format("UnnamedNodeGroup%i", groupNumber);
        }

        // call command for adding a new node group
        AZStd::string outResult;
        const AZStd::string command = AZStd::string::format("AddNodeGroup -actorID %i -name \"%s\"", m_actor->GetID(), groupName.c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        // select the new added row
        const int insertPosition = SearchTableForString(m_nodeGroupsTable, groupName.c_str());
        m_nodeGroupsTable->selectRow(insertPosition);
    }


    // function to remove a nodegroup
    void NodeGroupManagementWidget::RemoveSelectedNodeGroup()
    {
        // set the node group of the node group widget to nullptr
        if (m_nodeGroupWidget)
        {
            m_nodeGroupWidget->SetNodeGroup(nullptr);
        }

        // return if there is no entry to delete
        const int currentRow = m_nodeGroupsTable->currentRow();
        if (currentRow < 0)
        {
            return;
        }

        // get the nodegroup
        QTableWidgetItem* item = m_nodeGroupsTable->item(currentRow, 1);
        EMotionFX::NodeGroup* nodeGroup = m_actor->FindNodeGroupByName(FromQtString(item->text()).c_str());

        // call command for removing a nodegroup
        AZStd::string outResult;
        const AZStd::string command = AZStd::string::format("RemoveNodeGroup -actorID %i -name \"%s\"", m_actor->GetID(), nodeGroup->GetName());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        // selected the next row
        if (currentRow > (m_nodeGroupsTable->rowCount() - 1))
        {
            m_nodeGroupsTable->selectRow(currentRow - 1);
        }
        else
        {
            m_nodeGroupsTable->selectRow(currentRow);
        }
    }


    // rename selected node group
    void NodeGroupManagementWidget::RenameSelectedNodeGroup()
    {
        // get the nodegroup
        QTableWidgetItem* item = m_nodeGroupsTable->item(m_nodeGroupsTable->currentRow(), 1);
        EMotionFX::NodeGroup* nodeGroup = m_actor->FindNodeGroupByName(FromQtString(item->text()).c_str());

        // show the rename window
        NodeGroupManagementRenameWindow nodeGroupManagementRenameWindow(this, m_actor, nodeGroup->GetName());
        nodeGroupManagementRenameWindow.exec();
    }


    // function to clear the nodegroups
    void NodeGroupManagementWidget::ClearNodeGroups()
    {
        CommandSystem::ClearNodeGroupsCommand(m_actor);
    }


    // checkbox clicked
    void NodeGroupManagementWidget::checkboxClicked(bool checked)
    {
        // get the sender widget and the node group index
        QCheckBox* senderCheckbox = (QCheckBox*)sender();

        // find the checkbox row
        AZStd::string nodeGroupName;
        const int rowCount = m_nodeGroupsTable->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            QCheckBox* rowChechbox = (QCheckBox*)m_nodeGroupsTable->cellWidget(i, 0);
            if (rowChechbox == senderCheckbox)
            {
                nodeGroupName = m_nodeGroupsTable->item(i, 1)->text().toUtf8().data();
                break;
            }
        }

        // execute the command
        AZStd::string outResult;
        auto* command = aznew CommandSystem::CommandAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandSystem::CommandAdjustNodeGroup::s_commandName),
            /*actorId=*/ m_actor->GetID(),
            /*name=*/ nodeGroupName,
            /*newName=*/ AZStd::nullopt,
            /*enabledOnDefault=*/ checked
        );
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, outResult) == false)
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
        if (m_actor == nullptr)
        {
            return;
        }

        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = m_nodeGroupsTable->selectedItems();

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
