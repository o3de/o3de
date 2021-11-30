/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// inlude required headers
#include "AttachmentNodesWindow.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "AzCore/std/limits.h"
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
        m_nodeTable          = nullptr;
        m_selectNodesButton  = nullptr;
        m_nodeAction         = "";

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
        m_nodeTable = new QTableWidget(0, 1, 0);

        // create the table widget
        m_nodeTable->setMinimumHeight(125);
        m_nodeTable->setAlternatingRowColors(true);
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
        m_selectNodesButton  = new QToolButton();
        m_addNodesButton     = new QToolButton();
        m_removeNodesButton  = new QToolButton();

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
        layout->setSpacing(2);

        layout->addLayout(buttonLayout);
        layout->addWidget(m_nodeTable);

        // set the main layout
        setLayout(layout);

        // connect controls to the slots
        connect(m_selectNodesButton, &QToolButton::clicked, this, &AttachmentNodesWindow::SelectNodesButtonPressed);
        connect(m_addNodesButton, &QToolButton::clicked, this, &AttachmentNodesWindow::SelectNodesButtonPressed);
        connect(m_removeNodesButton, &QToolButton::clicked, this, &AttachmentNodesWindow::RemoveNodesButtonPressed);
        connect(m_nodeTable, &QTableWidget::itemSelectionChanged, this, &AttachmentNodesWindow::OnItemSelectionChanged);
        connect(m_nodeSelectionWindow->GetNodeHierarchyWidget(), &NodeHierarchyWidget::OnSelectionDone, this, &AttachmentNodesWindow::NodeSelectionFinished);
        connect(m_nodeSelectionWindow->GetNodeHierarchyWidget(), &NodeHierarchyWidget::OnDoubleClicked, this, &AttachmentNodesWindow::NodeSelectionFinished);
    }


    // used to update the interface
    void AttachmentNodesWindow::UpdateInterface()
    {
        // clear the table widget
        m_nodeTable->clear();

        // check if the current actor exists
        if (m_actor == nullptr)
        {
            // set the column count
            m_nodeTable->setColumnCount(0);

            // disable the widgets
            SetWidgetDisabled(true);

            // stop here
            return;
        }

        // set the column count
        m_nodeTable->setColumnCount(1);

        // enable the widget
        SetWidgetDisabled(false);

        // set the remove nodes button enabled or not based on selection
        m_removeNodesButton->setEnabled((m_nodeTable->rowCount() != 0) && (m_nodeTable->selectedItems().size() != 0));

        // counter for attachment nodes
        int numAttachmentNodes = 0;

        // set the row count
        const int numNodes = aznumeric_caster(m_actor->GetNumNodes());
        for (int i = 0; i < numNodes; ++i)
        {
            // get the nodegroup
            EMotionFX::Node* node = m_actor->GetSkeleton()->GetNode(i);
            if (node->GetIsAttachmentNode())
            {
                numAttachmentNodes++;
            }
        }

        // set the row count
        m_nodeTable->setRowCount(numAttachmentNodes);

        // set header items for the table
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem(AZStd::string::format("Attachment Nodes (%d / %zu)", numAttachmentNodes, m_actor->GetNumNodes()).c_str());
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignCenter);
        m_nodeTable->setHorizontalHeaderItem(0, nameHeaderItem);

        // fill the table with content
        uint16 currentRow = 0;
        for (uint16 i = 0; i < numNodes; ++i)
        {
            // get the nodegroup
            EMotionFX::Node* node = m_actor->GetSkeleton()->GetNode(i);

            // continue if node does not exist
            if (node == nullptr || node->GetIsAttachmentNode() == false)
            {
                continue;
            }

            // create table items
            QTableWidgetItem* tableItemNodeName = new QTableWidgetItem(node->GetName());
            m_nodeTable->setItem(currentRow, 0, tableItemNodeName);

            // set the row height and increase row counter
            m_nodeTable->setRowHeight(currentRow, 21);
            ++currentRow;
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

        // toggle enabled state of the remove button
        OnItemSelectionChanged();
    }


    // function to set the current actor
    void AttachmentNodesWindow::SetActor(EMotionFX::Actor* actor)
    {
        // set the new actor
        m_actor = actor;

        // update the interface
        UpdateInterface();
    }


    // slot for selecting nodes with the node browser
    void AttachmentNodesWindow::SelectNodesButtonPressed()
    {
        // check if actor is set
        if (m_actor == nullptr)
        {
            return;
        }

        // set the action for the selected nodes
        QWidget* senderWidget = (QWidget*)sender();
        if (senderWidget == m_addNodesButton)
        {
            m_nodeAction = "add";
        }
        else
        {
            m_nodeAction = "select";
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
            const size_t numNodes = m_actor->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = m_actor->GetSkeleton()->GetNode(i);
                if (node->GetIsAttachmentNode())
                {
                    m_nodeSelectionList.AddNode(node);
                }
            }
        }

        // show the node selection window
        m_nodeSelectionWindow->Update(actorInstance->GetID(), &m_nodeSelectionList);
        m_nodeSelectionWindow->show();
    }


    // remove node buttons
    void AttachmentNodesWindow::RemoveNodesButtonPressed()
    {
        // generate node list string
        AZStd::string nodeList;
        int lowestSelectedRow = AZStd::numeric_limits<int>::max();
        const int numTableRows = m_nodeTable->rowCount();
        for (int i = 0; i < numTableRows; ++i)
        {
            // get the current table item
            QTableWidgetItem* item = m_nodeTable->item(i, 0);
            if (item == nullptr)
            {
                continue;
            }

            // add the item to remove list, if it's selected
            if (item->isSelected())
            {
                nodeList += AZStd::string::format("%s;", FromQtString(item->text()).c_str());
                if (item->row() < lowestSelectedRow)
                {
                    lowestSelectedRow = item->row();
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
        command = AZStd::string::format("AdjustActor -actorID %i -nodeAction \"remove\" -attachmentNodes \"%s\"", m_actor->GetID(), nodeList.c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }

        // selected the next row
        if (lowestSelectedRow > m_nodeTable->rowCount() - 1)
        {
            m_nodeTable->selectRow(lowestSelectedRow - 1);
        }
        else
        {
            m_nodeTable->selectRow(lowestSelectedRow);
        }
    }


    // add / select nodes
    void AttachmentNodesWindow::NodeSelectionFinished(AZStd::vector<SelectionItem> selectionList)
    {
        if (selectionList.empty())
        {
            return;
        }

        // generate node list string
        AZStd::string nodeList;
        nodeList.reserve(16384);
        for (const SelectionItem& i : selectionList)
        {
            nodeList += AZStd::string::format("%s;", i.GetNodeName());
        }
        AzFramework::StringFunc::Strip(nodeList, MCore::CharacterConstants::semiColon, true /* case sensitive */, false /* beginning */, true /* ending */);

        // call command for adjusting disable on default flag
        AZStd::string outResult;
        AZStd::string command;
        command = AZStd::string::format("AdjustActor -actorID %i -nodeAction \"%s\" -attachmentNodes \"%s\"", m_actor->GetID(), m_nodeAction.c_str(), nodeList.c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // enable/disable the dialog
    void AttachmentNodesWindow::SetWidgetDisabled(bool disabled)
    {
        m_nodeTable->setDisabled(disabled);
        m_selectNodesButton->setDisabled(disabled);
        m_addNodesButton->setDisabled(disabled);
        m_removeNodesButton->setDisabled(disabled);
    }


    // handle item selection changes of the node table
    void AttachmentNodesWindow::OnItemSelectionChanged()
    {
        m_removeNodesButton->setEnabled((m_nodeTable->rowCount() != 0) && (!m_nodeTable->selectedItems().empty()));
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
