/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// inlude required headers
#include "AttachmentNodesWindow.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>

// include qt headers
#include <QToolButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QKeyEvent>
#include <QLabel>


namespace EMStudio
{
    // constructor
    AttachmentNodesWindow::AttachmentNodesWindow(QWidget* parent)
        : QWidget(parent)
    {
        mNodeTable          = nullptr;
        mSelectNodesButton  = nullptr;
        mNodeAction         = "";

        // init the widget
        Init();
    }


    // the destructor
    AttachmentNodesWindow::~AttachmentNodesWindow()
    {
    }


    // the init function
    void AttachmentNodesWindow::Init()
    {
        // create the node groups table
        mNodeTable = new QTableWidget(0, 1, 0);

        // create the table widget
        mNodeTable->setMinimumHeight(125);
        mNodeTable->setAlternatingRowColors(true);
        mNodeTable->setCornerButtonEnabled(false);
        mNodeTable->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mNodeTable->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row selection
        mNodeTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        // make the table items read only
        mNodeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // set header items for the table
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem("Nodes");
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        mNodeTable->setHorizontalHeaderItem(0, nameHeaderItem);

        QHeaderView* horizontalHeader = mNodeTable->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);

        // create the node selection window
        mNodeSelectionWindow = new NodeSelectionWindow(this, false);

        // create the selection buttons
        mSelectNodesButton  = new QToolButton();
        mAddNodesButton     = new QToolButton();
        mRemoveNodesButton  = new QToolButton();

        EMStudioManager::MakeTransparentButton(mSelectNodesButton, "Images/Icons/Plus.svg",   "Select nodes and replace the current selection");
        EMStudioManager::MakeTransparentButton(mAddNodesButton,    "Images/Icons/Plus.svg",   "Select nodes and add them to the current selection");
        EMStudioManager::MakeTransparentButton(mRemoveNodesButton, "Images/Icons/Minus.svg",  "Remove selected nodes from the list");

        // create the buttons layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(mSelectNodesButton);
        buttonLayout->addWidget(mAddNodesButton);
        buttonLayout->addWidget(mRemoveNodesButton);

        // create the layouts
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(2);

        layout->addLayout(buttonLayout);
        layout->addWidget(mNodeTable);

        // set the main layout
        setLayout(layout);

        // connect controls to the slots
        connect(mSelectNodesButton, &QToolButton::clicked, this, &AttachmentNodesWindow::SelectNodesButtonPressed);
        connect(mAddNodesButton, &QToolButton::clicked, this, &AttachmentNodesWindow::SelectNodesButtonPressed);
        connect(mRemoveNodesButton, &QToolButton::clicked, this, &AttachmentNodesWindow::RemoveNodesButtonPressed);
        connect(mNodeTable, &QTableWidget::itemSelectionChanged, this, &AttachmentNodesWindow::OnItemSelectionChanged);
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), static_cast<void (NodeHierarchyWidget::*)(MCore::Array<SelectionItem>)>(&NodeHierarchyWidget::OnSelectionDone), this, &AttachmentNodesWindow::NodeSelectionFinished);
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), static_cast<void (NodeHierarchyWidget::*)(MCore::Array<SelectionItem>)>(&NodeHierarchyWidget::OnDoubleClicked), this, &AttachmentNodesWindow::NodeSelectionFinished);
    }


    // used to update the interface
    void AttachmentNodesWindow::UpdateInterface()
    {
        // clear the table widget
        mNodeTable->clear();

        // check if the current actor exists
        if (mActor == nullptr)
        {
            // set the column count
            mNodeTable->setColumnCount(0);

            // disable the widgets
            SetWidgetDisabled(true);

            // stop here
            return;
        }

        // set the column count
        mNodeTable->setColumnCount(1);

        // enable the widget
        SetWidgetDisabled(false);

        // set the remove nodes button enabled or not based on selection
        mRemoveNodesButton->setEnabled((mNodeTable->rowCount() != 0) && (mNodeTable->selectedItems().size() != 0));

        // counter for attachment nodes
        uint16 numAttachmentNodes = 0;

        // set the row count
        const uint16 numNodes = mActor->GetNumNodes();
        for (uint16 i = 0; i < numNodes; ++i)
        {
            // get the nodegroup
            EMotionFX::Node* node = mActor->GetSkeleton()->GetNode(i);
            if (node->GetIsAttachmentNode())
            {
                numAttachmentNodes++;
            }
        }

        // set the row count
        mNodeTable->setRowCount(numAttachmentNodes);

        // set header items for the table
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem(AZStd::string::format("Attachment Nodes (%i / %i)", numAttachmentNodes, mActor->GetNumNodes()).c_str());
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignCenter);
        mNodeTable->setHorizontalHeaderItem(0, nameHeaderItem);

        // fill the table with content
        uint16 currentRow = 0;
        for (uint16 i = 0; i < numNodes; ++i)
        {
            // get the nodegroup
            EMotionFX::Node* node = mActor->GetSkeleton()->GetNode(i);

            // continue if node does not exist
            if (node == nullptr || node->GetIsAttachmentNode() == false)
            {
                continue;
            }

            // create table items
            QTableWidgetItem* tableItemNodeName = new QTableWidgetItem(node->GetName());
            mNodeTable->setItem(currentRow, 0, tableItemNodeName);

            // set the row height and increase row counter
            mNodeTable->setRowHeight(currentRow, 21);
            ++currentRow;
        }

        // resize to contents and adjust header
        QHeaderView* verticalHeader = mNodeTable->verticalHeader();
        verticalHeader->setVisible(false);
        mNodeTable->resizeColumnsToContents();
        mNodeTable->horizontalHeader()->setStretchLastSection(true);

        // set table size
        mNodeTable->setColumnWidth(0, 37);
        mNodeTable->setColumnWidth(3, 0);
        mNodeTable->setColumnHidden(3, true);
        mNodeTable->sortItems(3);

        // toggle enabled state of the remove button
        OnItemSelectionChanged();
    }


    // function to set the current actor
    void AttachmentNodesWindow::SetActor(EMotionFX::Actor* actor)
    {
        // set the new actor
        mActor = actor;

        // update the interface
        UpdateInterface();
    }


    // slot for selecting nodes with the node browser
    void AttachmentNodesWindow::SelectNodesButtonPressed()
    {
        // check if actor is set
        if (mActor == nullptr)
        {
            return;
        }

        // set the action for the selected nodes
        QWidget* senderWidget = (QWidget*)sender();
        if (senderWidget == mAddNodesButton)
        {
            mNodeAction = "add";
        }
        else
        {
            mNodeAction = "select";
        }

        // get the selected actorinstance
        const CommandSystem::SelectionList& selection       = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance*           actorInstance   = selection.GetSingleActorInstance();

        // show hint if no/multiple actor instances is/are selected
        if (actorInstance == nullptr)
        {
            return;
        }

        // create selection list for the current nodes within the group
        mNodeSelectionList.Clear();
        if (senderWidget == mSelectNodesButton)
        {
            const uint32 numNodes = mActor->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = mActor->GetSkeleton()->GetNode(i);
                if (node->GetIsAttachmentNode())
                {
                    mNodeSelectionList.AddNode(node);
                }
            }
        }

        // show the node selection window
        mNodeSelectionWindow->Update(actorInstance->GetID(), &mNodeSelectionList);
        mNodeSelectionWindow->show();
    }


    // remove node buttons
    void AttachmentNodesWindow::RemoveNodesButtonPressed()
    {
        // generate node list string
        AZStd::string nodeList;
        uint32 lowestSelectedRow = MCORE_INVALIDINDEX32;
        const uint32 numTableRows = mNodeTable->rowCount();
        for (uint32 i = 0; i < numTableRows; ++i)
        {
            // get the current table item
            QTableWidgetItem* item = mNodeTable->item(i, 0);
            if (item == nullptr)
            {
                continue;
            }

            // add the item to remove list, if it's selected
            if (item->isSelected())
            {
                nodeList += AZStd::string::format("%s;", FromQtString(item->text()).c_str());
                if ((uint32)item->row() < lowestSelectedRow)
                {
                    lowestSelectedRow = (uint32)item->row();
                }
            }
        }

        // stop here if nothing selected
        if (nodeList.empty())
        {
            return;
        }

        // call command for adjusting disable on default flag
        AZStd::string outResult;
        AZStd::string command;
        command = AZStd::string::format("AdjustActor -actorID %i -nodeAction \"remove\" -attachmentNodes \"%s\"", mActor->GetID(), nodeList.c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }

        // selected the next row
        if (lowestSelectedRow > ((uint32)mNodeTable->rowCount() - 1))
        {
            mNodeTable->selectRow(lowestSelectedRow - 1);
        }
        else
        {
            mNodeTable->selectRow(lowestSelectedRow);
        }
    }


    // add / select nodes
    void AttachmentNodesWindow::NodeSelectionFinished(MCore::Array<SelectionItem> selectionList)
    {
        // return if no nodes are selected
        if (selectionList.GetLength() == 0)
        {
            return;
        }

        // generate node list string
        AZStd::string nodeList;
        nodeList.reserve(16384);
        const uint32 numSelectedNodes = selectionList.GetLength();
        for (uint32 i = 0; i < numSelectedNodes; ++i)
        {
            nodeList += AZStd::string::format("%s;", selectionList[i].GetNodeName());
        }
        AzFramework::StringFunc::Strip(nodeList, MCore::CharacterConstants::semiColon, true /* case sensitive */, false /* beginning */, true /* ending */);

        // call command for adjusting disable on default flag
        AZStd::string outResult;
        AZStd::string command;
        command = AZStd::string::format("AdjustActor -actorID %i -nodeAction \"%s\" -attachmentNodes \"%s\"", mActor->GetID(), mNodeAction.c_str(), nodeList.c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // enable/disable the dialog
    void AttachmentNodesWindow::SetWidgetDisabled(bool disabled)
    {
        mNodeTable->setDisabled(disabled);
        mSelectNodesButton->setDisabled(disabled);
        mAddNodesButton->setDisabled(disabled);
        mRemoveNodesButton->setDisabled(disabled);
    }


    // handle item selection changes of the node table
    void AttachmentNodesWindow::OnItemSelectionChanged()
    {
        mRemoveNodesButton->setEnabled((mNodeTable->rowCount() != 0) && (mNodeTable->selectedItems().size() != 0));
    }


    // key press event
    void AttachmentNodesWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            RemoveNodesButtonPressed();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    // key release event
    void AttachmentNodesWindow::keyReleaseEvent(QKeyEvent* event)
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

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Attachments/moc_AttachmentNodesWindow.cpp>
