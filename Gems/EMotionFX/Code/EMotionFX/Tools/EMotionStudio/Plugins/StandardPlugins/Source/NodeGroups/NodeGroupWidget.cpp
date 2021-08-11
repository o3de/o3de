/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// inlude required headers
#include "NodeGroupWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <AzCore/std/iterator.h>
#include <AzCore/std/limits.h>
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
        m_nodeTable                  = nullptr;
        m_selectNodesButton          = nullptr;
        m_nodeGroup                  = nullptr;

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
        // create the node groups table
        m_nodeTable = new QTableWidget(0, 1, 0);

        // create the table widget
        m_nodeTable->setCornerButtonEnabled(false);
        m_nodeTable->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_nodeTable->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row selection
        m_nodeTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        // make the table items read only
        m_nodeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // set header items for the table
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem("Nodes");
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        m_nodeTable->setHorizontalHeaderItem(0, nameHeaderItem);

        QHeaderView* horizontalHeader = m_nodeTable->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);

        // create the node selection window
        m_nodeSelectionWindow = new NodeSelectionWindow(this, false);

        // create the selection buttons
        m_selectNodesButton  = new QPushButton();
        m_addNodesButton     = new QPushButton();
        m_removeNodesButton  = new QPushButton();

        EMStudioManager::MakeTransparentButton(m_selectNodesButton, "Images/Icons/Plus.svg",   "Select nodes and replace the current selection");
        EMStudioManager::MakeTransparentButton(m_addNodesButton,    "Images/Icons/Plus.svg",   "Select nodes and add them to the current selection");
        EMStudioManager::MakeTransparentButton(m_removeNodesButton, "Images/Icons/Minus.svg",  "Remove selected nodes from the list");

        // create the buttons layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(m_selectNodesButton);
        buttonLayout->addWidget(m_addNodesButton);
        buttonLayout->addWidget(m_removeNodesButton);

        // create the layouts
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        QVBoxLayout* tableLayout = new QVBoxLayout();
        tableLayout->setSpacing(2);
        tableLayout->setMargin(0);

        tableLayout->addLayout(buttonLayout);
        tableLayout->addWidget(m_nodeTable);

        layout->addLayout(tableLayout);
        //layout->addLayout( editLayout );

        // set the main layout
        setLayout(layout);

        // connect controls to the slots
        connect(m_selectNodesButton, &QPushButton::clicked, this, &NodeGroupWidget::SelectNodesButtonPressed);
        connect(m_addNodesButton, &QPushButton::clicked, this, &NodeGroupWidget::SelectNodesButtonPressed);
        connect(m_removeNodesButton, &QPushButton::clicked, this, &NodeGroupWidget::RemoveNodesButtonPressed);
        connect(m_nodeTable, &QTableWidget::itemSelectionChanged, this, &NodeGroupWidget::OnItemSelectionChanged);
        connect(m_nodeSelectionWindow->GetNodeHierarchyWidget(), &NodeHierarchyWidget::OnSelectionDone, this, &NodeGroupWidget::NodeSelectionFinished);
        connect(m_nodeSelectionWindow->GetNodeHierarchyWidget(), &NodeHierarchyWidget::OnDoubleClicked, this, &NodeGroupWidget::NodeSelectionFinished);
    }


    // used to update the interface
    void NodeGroupWidget::UpdateInterface()
    {
        // clear the table widget
        m_nodeTable->clear();

        // check if the node group is not valid
        if (m_nodeGroup == nullptr)
        {
            // set the column count
            m_nodeTable->setColumnCount(0);

            // disable the widgets
            SetWidgetEnabled(false);

            // stop here
            return;
        }

        // set the column count
        m_nodeTable->setColumnCount(1);

        // enable the widget
        SetWidgetEnabled(true);

        // set the remove nodes button enabled or not based on selection
        m_removeNodesButton->setEnabled((m_nodeTable->rowCount() != 0) && (m_nodeTable->selectedItems().size() != 0));

        // clear the table widget
        m_nodeTable->setRowCount(aznumeric_caster(m_nodeGroup->GetNumNodes()));

        // set header items for the table
        AZStd::string headerText = AZStd::string::format("%s Nodes (%zu / %zu)", ((m_nodeGroup->GetIsEnabledOnDefault()) ? "Enabled" : "Disabled"), m_nodeGroup->GetNumNodes(), m_actor->GetNumNodes());
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem(headerText.c_str());
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignCenter);
        m_nodeTable->setHorizontalHeaderItem(0, nameHeaderItem);

        // fill the table with content
        const size_t numNodes = m_nodeGroup->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            // get the nodegroup
            EMotionFX::Node* node = m_actor->GetSkeleton()->GetNode(m_nodeGroup->GetNode(i));

            // continue if node does not exist
            if (node == nullptr)
            {
                continue;
            }

            // create table items
            QTableWidgetItem* tableItemNodeName = new QTableWidgetItem(node->GetName());
            m_nodeTable->setItem(aznumeric_caster(i), 0, tableItemNodeName);

            // set the row height
            m_nodeTable->setRowHeight(aznumeric_caster(i), 21);
        }

        // resize to contents and adjust header
        QHeaderView* verticalHeader = m_nodeTable->verticalHeader();
        verticalHeader->setVisible(false);
        m_nodeTable->resizeColumnsToContents();
        m_nodeTable->horizontalHeader()->setStretchLastSection(true);

        // set table size
        m_nodeTable->setColumnWidth(0, 37);
        m_nodeTable->setColumnWidth(3, 0);
        m_nodeTable->setColumnHidden(3, true);
        m_nodeTable->sortItems(3);
    }


    // function to set the current nodegroup
    void NodeGroupWidget::SetNodeGroup(EMotionFX::NodeGroup* nodeGroup)
    {
        // check if the actor was set
        if (m_actor == nullptr)
        {
            m_nodeGroup = nullptr;
            return;
        }

        // set the node group
        m_nodeGroup = nodeGroup;

        // update the interface
        UpdateInterface();
    }


    // function to set the current actor
    void NodeGroupWidget::SetActor(EMotionFX::Actor* actor)
    {
        // set the new actor
        m_actor = actor;
        m_nodeGroup = nullptr;

        // update the interface
        UpdateInterface();
    }


    // slot for selecting nodes with the node browser
    void NodeGroupWidget::SelectNodesButtonPressed()
    {
        // check if node group is set
        if (m_nodeGroup == nullptr)
        {
            return;
        }

        // set the action for the selected nodes
        QWidget* senderWidget = (QWidget*)sender();
        if (senderWidget == m_addNodesButton)
        {
            m_nodeAction = CommandSystem::CommandAdjustNodeGroup::NodeAction::Add;
        }
        else
        {
            m_nodeAction = CommandSystem::CommandAdjustNodeGroup::NodeAction::Replace;
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
        m_nodeSelectionList.Clear();
        if (senderWidget == m_selectNodesButton)
        {
            for (const uint16 i : m_nodeGroup->GetNodeArray())
            {
                EMotionFX::Node* node = m_actor->GetSkeleton()->GetNode(i);
                m_nodeSelectionList.AddNode(node);
            }
        }

        // show the node selection window
        m_nodeSelectionWindow->Update(actorInstance->GetID(), &m_nodeSelectionList);
        m_nodeSelectionWindow->show();
    }


    // remove nodes
    void NodeGroupWidget::RemoveNodesButtonPressed()
    {
        if (m_nodeTable->selectedItems().empty())
        {
            return;
        }

        // generate node list string
        AZStd::vector<AZStd::string> nodeList;
        int lowestSelectedRow = AZStd::numeric_limits<int>::max();
        for (const QTableWidgetItem* item : m_nodeTable->selectedItems())
        {
            nodeList.emplace_back(FromQtString(item->text()));
            lowestSelectedRow = AZStd::min(lowestSelectedRow, item->row());
        }

        AZStd::string outResult;
        auto* command = aznew CommandSystem::CommandAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandSystem::CommandAdjustNodeGroup::s_commandName),
            /*actorId=*/ m_actor->GetID(),
            /*name=*/ m_nodeGroup->GetName(),
            /*newName=*/ AZStd::nullopt,
            /*enabledOnDefault=*/ AZStd::nullopt,
            /*nodeNames=*/ AZStd::move(nodeList),
            /*nodeAction=*/ CommandSystem::CommandAdjustNodeGroup::NodeAction::Remove
        );
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        // selected the next row
        if (lowestSelectedRow > (m_nodeTable->rowCount() - 1))
        {
            m_nodeTable->selectRow(lowestSelectedRow - 1);
        }
        else
        {
            m_nodeTable->selectRow(lowestSelectedRow);
        }
    }


    // add / select nodes
    void NodeGroupWidget::NodeSelectionFinished(const AZStd::vector<SelectionItem>& selectionList)
    {
        // return if no nodes are selected
        if (selectionList.empty())
        {
            return;
        }

        // generate node list string
        AZStd::vector<AZStd::string> nodeList;
        nodeList.reserve(selectionList.size());
        AZStd::transform(begin(selectionList), end(selectionList), AZStd::back_inserter(nodeList), [](const auto& item)
        {
            return item.GetNodeName();
        });

        AZStd::string outResult;
        auto* command = aznew CommandSystem::CommandAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandSystem::CommandAdjustNodeGroup::s_commandName),
            /*actorId=*/ m_actor->GetID(),
            /*name=*/ m_nodeGroup->GetName(),
            /*newName=*/ AZStd::nullopt,
            /*enabledOnDefault=*/ AZStd::nullopt,
            /*nodeNames=*/ AZStd::move(nodeList),
            /*nodeAction=*/ m_nodeAction
        );
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }
    }


    // handle item selection changes of the node table
    void NodeGroupWidget::OnItemSelectionChanged()
    {
        m_removeNodesButton->setEnabled((m_nodeTable->rowCount() != 0) && (m_nodeTable->selectedItems().size() != 0));
    }


    // enable/disable the dialog
    void NodeGroupWidget::SetWidgetEnabled(bool enabled)
    {
        m_nodeTable->setDisabled(!enabled);
        m_selectNodesButton->setDisabled(!enabled);
        m_addNodesButton->setDisabled(!enabled);
        m_removeNodesButton->setDisabled(!enabled);
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
