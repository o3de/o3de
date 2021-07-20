/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// inlude required headers
#include "NodeGroupWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/Source/NodeGroup.h>
#include <MCore/Source/StringConversions.h>

// include qt headers
#include <QPushButton>
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
    NodeGroupWidget::NodeGroupWidget(QWidget* parent)
        : QWidget(parent)
    {
        //mEnabledOnDefaultCheckbox = nullptr;
        mNodeTable                  = nullptr;
        mSelectNodesButton          = nullptr;
        mNodeGroup                  = nullptr;
        mNodeAction                 = "";

        // init the widget
        Init();
    }


    // the destructor
    NodeGroupWidget::~NodeGroupWidget()
    {
    }


    // the init function
    void NodeGroupWidget::Init()
    {
        // create the disable checkbox
        //mEnabledOnDefaultCheckbox = new QCheckBox( "Enabled On Default" );

        // edit field for the group name
        //mNodeGroupNameEdit = new QLineEdit();

        // create the node groups table
        mNodeTable = new QTableWidget(0, 1, 0);

        // create the table widget
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
        mSelectNodesButton  = new QPushButton();
        mAddNodesButton     = new QPushButton();
        mRemoveNodesButton  = new QPushButton();

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

        // create layout for the edit field
        /*QHBoxLayout* editLayout = new QHBoxLayout();
        editLayout->addWidget( new QLabel("Name:") );
        editLayout->addWidget( mNodeGroupNameEdit );
        editLayout->addWidget( mEnabledOnDefaultCheckbox );*/

        // create the layouts
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        QVBoxLayout* tableLayout = new QVBoxLayout();
        tableLayout->setSpacing(2);
        tableLayout->setMargin(0);

        tableLayout->addLayout(buttonLayout);
        tableLayout->addWidget(mNodeTable);

        layout->addLayout(tableLayout);
        //layout->addLayout( editLayout );

        // set the main layout
        setLayout(layout);

        // connect controls to the slots
        connect(mSelectNodesButton, &QPushButton::clicked, this, &NodeGroupWidget::SelectNodesButtonPressed);
        connect(mAddNodesButton, &QPushButton::clicked, this, &NodeGroupWidget::SelectNodesButtonPressed);
        connect(mRemoveNodesButton, &QPushButton::clicked, this, &NodeGroupWidget::RemoveNodesButtonPressed);
        connect(mNodeTable, &QTableWidget::itemSelectionChanged, this, &NodeGroupWidget::OnItemSelectionChanged);
        //connect( mEnabledOnDefaultCheckbox, SIGNAL(clicked()), this, SLOT(EnabledOnDefaultChanged()) );
        //connect( mNodeGroupNameEdit, SIGNAL(editingFinished()), this, SLOT(NodeGroupNameEditingFinished()) );
        //connect( mNodeGroupNameEdit, SIGNAL(textChanged(QString)), this, SLOT(NodeGroupNameEditChanged(QString)) );
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), static_cast<void (NodeHierarchyWidget::*)(MCore::Array<SelectionItem>)>(&NodeHierarchyWidget::OnSelectionDone), this, &NodeGroupWidget::NodeSelectionFinished);
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), static_cast<void (NodeHierarchyWidget::*)(MCore::Array<SelectionItem>)>(&NodeHierarchyWidget::OnDoubleClicked), this, &NodeGroupWidget::NodeSelectionFinished);
    }


    // used to update the interface
    void NodeGroupWidget::UpdateInterface()
    {
        // clear the table widget
        mNodeTable->clear();

        // check if the node group is not valid
        if (mNodeGroup == nullptr)
        {
            // set the column count
            mNodeTable->setColumnCount(0);

            // disable the widgets
            SetWidgetEnabled(false);

            // clear the edit field
            //mNodeGroupNameEdit->setText( "" );

            // stop here
            return;
        }

        // set the column count
        mNodeTable->setColumnCount(1);

        // enable the widget
        SetWidgetEnabled(true);

        // set the remove nodes button enabled or not based on selection
        mRemoveNodesButton->setEnabled((mNodeTable->rowCount() != 0) && (mNodeTable->selectedItems().size() != 0));

        // clear the table widget
        mNodeTable->setRowCount(mNodeGroup->GetNumNodes());

        // set header items for the table
        AZStd::string headerText = AZStd::string::format("%s Nodes (%i / %i)", ((mNodeGroup->GetIsEnabledOnDefault()) ? "Enabled" : "Disabled"), mNodeGroup->GetNumNodes(), mActor->GetNumNodes());
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem(headerText.c_str());
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignCenter);
        mNodeTable->setHorizontalHeaderItem(0, nameHeaderItem);

        // fill the table with content
        const uint16 numNodes = mNodeGroup->GetNumNodes();
        for (uint16 i = 0; i < numNodes; ++i)
        {
            // get the nodegroup
            EMotionFX::Node* node = mActor->GetSkeleton()->GetNode(mNodeGroup->GetNode(i));

            // continue if node does not exist
            if (node == nullptr)
            {
                continue;
            }

            // create table items
            QTableWidgetItem* tableItemNodeName = new QTableWidgetItem(node->GetName());
            mNodeTable->setItem(i, 0, tableItemNodeName);

            // set the row height
            mNodeTable->setRowHeight(i, 21);
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
    }


    // function to set the current nodegroup
    void NodeGroupWidget::SetNodeGroup(EMotionFX::NodeGroup* nodeGroup)
    {
        // check if the actor was set
        if (mActor == nullptr)
        {
            mNodeGroup = nullptr;
            return;
        }

        // set the node group
        mNodeGroup = nodeGroup;

        // update the interface
        UpdateInterface();
    }


    // function to set the current actor
    void NodeGroupWidget::SetActor(EMotionFX::Actor* actor)
    {
        // set the new actor
        mActor = actor;
        mNodeGroup = nullptr;

        // update the interface
        UpdateInterface();
    }


    // slot for selecting nodes with the node browser
    void NodeGroupWidget::SelectNodesButtonPressed()
    {
        // check if node group is set
        if (mNodeGroup == nullptr)
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
            MCore::SmallArray<uint16>&  nodes       = mNodeGroup->GetNodeArray();
            const uint16                numNodes    = nodes.GetLength();
            for (uint16 i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = mActor->GetSkeleton()->GetNode(nodes[i]);
                mNodeSelectionList.AddNode(node);
            }
        }

        // show the node selection window
        mNodeSelectionWindow->Update(actorInstance->GetID(), &mNodeSelectionList);
        mNodeSelectionWindow->show();
    }


    // remove nodes
    void NodeGroupWidget::RemoveNodesButtonPressed()
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
                nodeList += AZStd::string::format("%s;", item->text().toUtf8().data());
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
        AZStd::string command = AZStd::string::format("AdjustNodeGroup -actorID %i -name \"%s\" -nodeAction \"remove\" -nodeNames \"%s\"", mActor->GetID(), mNodeGroup->GetName(), nodeList.c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
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
    void NodeGroupWidget::NodeSelectionFinished(MCore::Array<SelectionItem> selectionList)
    {
        // return if no nodes are selected
        if (selectionList.GetLength() == 0)
        {
            return;
        }

        // generate node list string
        AZStd::string nodeList;
        nodeList.reserve(16448);
        const uint32 numSelectedNodes = selectionList.GetLength();
        for (uint32 i = 0; i < numSelectedNodes; ++i)
        {
            nodeList += selectionList[i].GetNodeName();
            nodeList += ";";
        }
        AzFramework::StringFunc::Strip(nodeList, MCore::CharacterConstants::semiColon, true /* case sensitive */, false /* beginning */, true /* ending */);

        // call command for adjusting disable on default flag
        AZStd::string outResult;
        AZStd::string command = AZStd::string::format("AdjustNodeGroup -actorID %i -name \"%s\" -nodeAction \"%s\" -nodeNames \"%s\"", mActor->GetID(), mNodeGroup->GetName(), mNodeAction.c_str(), nodeList.c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }
    }


    // handle item selection changes of the node table
    void NodeGroupWidget::OnItemSelectionChanged()
    {
        mRemoveNodesButton->setEnabled((mNodeTable->rowCount() != 0) && (mNodeTable->selectedItems().size() != 0));
    }


    // enable/disable the dialog
    void NodeGroupWidget::SetWidgetEnabled(bool enabled)
    {
        //mNodeGroupNameEdit->setDisabled( !enabled );
        //mEnabledOnDefaultCheckbox->setDisabled( !enabled );
        mNodeTable->setDisabled(!enabled);
        mSelectNodesButton->setDisabled(!enabled);
        mAddNodesButton->setDisabled(!enabled);
        mRemoveNodesButton->setDisabled(!enabled);
    }


    // key press event
    void NodeGroupWidget::keyPressEvent(QKeyEvent* event)
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
    void NodeGroupWidget::keyReleaseEvent(QKeyEvent* event)
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

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/NodeGroups/moc_NodeGroupWidget.cpp>
