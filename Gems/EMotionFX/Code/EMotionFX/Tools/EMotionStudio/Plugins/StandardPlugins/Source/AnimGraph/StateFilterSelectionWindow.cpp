/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StateFilterSelectionWindow.h"
#include "BlendGraphWidget.h"
#include "AnimGraphPlugin.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>

#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>

#include <AzQtComponents/Utilities/Conversions.h>


namespace EMStudio
{
    StateFilterSelectionWindow::StateFilterSelectionWindow(QWidget* parent)
        : QDialog(parent)
        , m_stateMachine(nullptr)
    {
        setWindowTitle("Select States");

        QVBoxLayout* mainLayout = new QVBoxLayout();
        setLayout(mainLayout);
        mainLayout->setAlignment(Qt::AlignTop);

        mTableWidget = new QTableWidget();
        mTableWidget->setAlternatingRowColors(true);
        mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTableWidget->horizontalHeader()->setStretchLastSection(true);
        mTableWidget->setCornerButtonEnabled(false);
        mTableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        connect(mTableWidget, &QTableWidget::itemSelectionChanged, this, &StateFilterSelectionWindow::OnSelectionChanged);

        mainLayout->addWidget(mTableWidget);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mainLayout->addLayout(buttonLayout);

        QPushButton* okButton = new QPushButton("OK");
        okButton->setDefault(true);
        buttonLayout->addWidget(okButton);
        connect(okButton, &QPushButton::clicked, this, &StateFilterSelectionWindow::accept);

        QPushButton* cancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(cancelButton);
        connect(cancelButton, &QPushButton::clicked, this, &StateFilterSelectionWindow::reject);

        setMinimumSize(400, 800);
    }


    StateFilterSelectionWindow::~StateFilterSelectionWindow()
    {
    }


    // called to init for a new anim graph
    void StateFilterSelectionWindow::ReInit(EMotionFX::AnimGraphStateMachine* stateMachine, const AZStd::vector<EMotionFX::AnimGraphNodeId>& oldNodeSelection, const AZStd::vector<AZStd::string>& oldGroupSelection)
    {
        m_stateMachine          = stateMachine;
        mSelectedGroupNames     = oldGroupSelection;
        m_selectedNodeIds       = oldNodeSelection;

        // clear the table widget
        mWidgetTable.clear();
        mTableWidget->clear();
        mTableWidget->setColumnCount(2);

        // set header items for the table
        QTableWidgetItem* headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(0, headerItem);

        headerItem = new QTableWidgetItem("Type");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(1, headerItem);

        mTableWidget->resizeColumnsToContents();
        QHeaderView* horizontalHeader = mTableWidget->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);

        if (!m_stateMachine)
        {
            return;
        }

        const EMotionFX::AnimGraph* animGraph = m_stateMachine->GetAnimGraph();

        // get the number of nodes inside the active node, the number node groups and set table size and add header items
        const uint32 numNodeGroups  = animGraph->GetNumNodeGroups();
        const uint32 numNodes       = m_stateMachine->GetNumChildNodes();
        const uint32 numRows        = numNodeGroups + numNodes;
        mTableWidget->setRowCount(numRows);

        // Block signals for the table widget to not reach OnSelectionChanged() when adding rows as that
        // clears m_selectedNodeIds and thus breaks the 'is node selected' check in the following loop.
        {
            QSignalBlocker signalBlocker(mTableWidget);

            // iterate the nodes and add them all
            uint32 currentRowIndex = 0;
            for (uint32 i = 0; i < numNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = m_stateMachine->GetChildNode(i);

                // check if we need to select the node
                bool isSelected = false;
                if (AZStd::find(m_selectedNodeIds.begin(), m_selectedNodeIds.end(), childNode->GetId()) != m_selectedNodeIds.end())
                {
                    isSelected = true;
                }

                AddRow(currentRowIndex, childNode->GetName(), false, isSelected);
                currentRowIndex++;
            }

            // iterate the node groups and add them all
            for (uint32 i = 0; i < numNodeGroups; ++i)
            {
                // get a pointer to the node group
                EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

                // check if any of the nodes from the node group actually is visible in the current state machine
                bool addGroup = false;
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    EMotionFX::AnimGraphNode* childNode = m_stateMachine->GetChildNode(n);
                    if (nodeGroup->Contains(childNode->GetId()))
                    {
                        addGroup = true;
                        break;
                    }
                }

                if (addGroup)
                {
                    // check if we need to select the node group
                    bool isSelected = false;
                    if (AZStd::find(oldGroupSelection.begin(), oldGroupSelection.end(), nodeGroup->GetName()) != oldGroupSelection.end())
                    {
                        isSelected = true;
                    }

                    AZ::Color color;
                    color.FromU32(nodeGroup->GetColor());
                    AddRow(currentRowIndex, nodeGroup->GetName(), true, isSelected, AzQtComponents::ToQColor(color));
                    currentRowIndex++;
                }
            }
        }

        // resize to contents and adjust header
        QHeaderView* verticalHeader = mTableWidget->verticalHeader();
        verticalHeader->setVisible(false);
        mTableWidget->resizeColumnsToContents();
        horizontalHeader->setStretchLastSection(true);
    }


    void StateFilterSelectionWindow::AddRow(uint32 rowIndex, const char* name, bool isGroup, bool isSelected, const QColor& color)
    {
        // create the name item
        QTableWidgetItem* nameItem = nullptr;
        if (isGroup)
        {
            nameItem = new QTableWidgetItem("[" + QString(name) + "]");
        }
        else
        {
            nameItem = new QTableWidgetItem(name);
        }

        // set the name item params
        nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        // add the name item in the table
        mTableWidget->setItem(rowIndex, 0, nameItem);

        // add a lookup
        mWidgetTable.emplace_back(WidgetLookup(nameItem, name, isGroup));

        // create the type item
        QTableWidgetItem* typeItem = nullptr;
        if (isGroup)
        {
            typeItem = new QTableWidgetItem("Node Group");
        }
        else
        {
            typeItem = new QTableWidgetItem("Node");
        }

        // set the type item params
        typeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        // add the type item in the table
        mTableWidget->setItem(rowIndex, 1, typeItem);

        // add a lookup
        mWidgetTable.emplace_back(WidgetLookup(typeItem, name, isGroup));

        // set backgroundcolor of the row
        if (isGroup)
        {
            QColor backgroundColor = color;
            backgroundColor.setAlpha(50);
            nameItem->setBackground(backgroundColor);
            typeItem->setBackground(backgroundColor);
        }

        // handle selection
        if (isSelected)
        {
            nameItem->setSelected(true);
            typeItem->setSelected(true);
        }

        // set the row height
        mTableWidget->setRowHeight(rowIndex, 21);
    }


    // find the index for the given widget
    EMotionFX::AnimGraphNodeGroup* StateFilterSelectionWindow::FindGroupByWidget(QTableWidgetItem* widget) const
    {
        if (!m_stateMachine)
        {
            return nullptr;
        }

        const EMotionFX::AnimGraph* animGraph = m_stateMachine->GetAnimGraph();

        // for all table entries
        const size_t numWidgets = mWidgetTable.size();
        for (size_t i = 0; i < numWidgets; ++i)
        {
            if (mWidgetTable[i].mIsGroup && mWidgetTable[i].mWidget == widget)
            {
                return animGraph->FindNodeGroupByName(mWidgetTable[i].mName.c_str());
            }
        }

        // the widget is not in the list, return failure
        return nullptr;
    }


    // find the node for the given widget
    EMotionFX::AnimGraphNode* StateFilterSelectionWindow::FindNodeByWidget(QTableWidgetItem* widget) const
    {
        if (!m_stateMachine)
        {
            return nullptr;
        }

        const EMotionFX::AnimGraph* animGraph = m_stateMachine->GetAnimGraph();

        // for all table entries
        const size_t numWidgets = mWidgetTable.size();
        for (size_t i = 0; i < numWidgets; ++i)
        {
            if (mWidgetTable[i].mIsGroup == false && mWidgetTable[i].mWidget == widget)
            {
                return animGraph->RecursiveFindNodeByName(mWidgetTable[i].mName.c_str());
            }
        }

        // the widget is not in the list, return failure
        return nullptr;
    }


    // called when the selection got changed
    void StateFilterSelectionWindow::OnSelectionChanged()
    {
        // reset the selection arrays
        mSelectedGroupNames.clear();
        m_selectedNodeIds.clear();

        // get the selected items and the number of them
        QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // iterate through the selected items
        for (int32 i = 0; i < numSelectedItems; ++i)
        {
            // handle nodes
            EMotionFX::AnimGraphNode* node = FindNodeByWidget(selectedItems[i]);
            if (node)
            {
                // add the node name in case it is not in yet
                if (AZStd::find(m_selectedNodeIds.begin(), m_selectedNodeIds.end(), node->GetId()) == m_selectedNodeIds.end())
                {
                    m_selectedNodeIds.emplace_back(node->GetId());
                }
            }

            // handle node groups
            EMotionFX::AnimGraphNodeGroup* nodeGroup = FindGroupByWidget(selectedItems[i]);
            if (nodeGroup)
            {
                // add the node group name in case it is not in yet
                if (AZStd::find(mSelectedGroupNames.begin(), mSelectedGroupNames.end(), nodeGroup->GetName()) == mSelectedGroupNames.end())
                {
                    mSelectedGroupNames.emplace_back(nodeGroup->GetName());
                }
            }
        }
    }
} // namespace EMStudio
