/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        m_tableWidget = new QTableWidget();
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tableWidget->horizontalHeader()->setStretchLastSection(true);
        m_tableWidget->setCornerButtonEnabled(false);
        m_tableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &StateFilterSelectionWindow::OnSelectionChanged);

        mainLayout->addWidget(m_tableWidget);

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
        m_selectedGroupNames     = oldGroupSelection;
        m_selectedNodeIds       = oldNodeSelection;

        // clear the table widget
        m_widgetTable.clear();
        m_tableWidget->clear();
        m_tableWidget->setColumnCount(2);

        // set header items for the table
        QTableWidgetItem* headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(0, headerItem);

        headerItem = new QTableWidgetItem("Type");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(1, headerItem);

        m_tableWidget->resizeColumnsToContents();
        QHeaderView* horizontalHeader = m_tableWidget->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);

        if (!m_stateMachine)
        {
            return;
        }

        const EMotionFX::AnimGraph* animGraph = m_stateMachine->GetAnimGraph();

        // get the number of nodes inside the active node, the number node groups and set table size and add header items
        const size_t numNodeGroups  = animGraph->GetNumNodeGroups();
        const size_t numNodes       = m_stateMachine->GetNumChildNodes();
        const int numRows           = aznumeric_caster(numNodeGroups + numNodes);
        m_tableWidget->setRowCount(numRows);

        // Block signals for the table widget to not reach OnSelectionChanged() when adding rows as that
        // clears m_selectedNodeIds and thus breaks the 'is node selected' check in the following loop.
        {
            QSignalBlocker signalBlocker(m_tableWidget);

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
        QHeaderView* verticalHeader = m_tableWidget->verticalHeader();
        verticalHeader->setVisible(false);
        m_tableWidget->resizeColumnsToContents();
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
        m_tableWidget->setItem(rowIndex, 0, nameItem);

        // add a lookup
        m_widgetTable.emplace_back(WidgetLookup(nameItem, name, isGroup));

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
        m_tableWidget->setItem(rowIndex, 1, typeItem);

        // add a lookup
        m_widgetTable.emplace_back(WidgetLookup(typeItem, name, isGroup));

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
        m_tableWidget->setRowHeight(rowIndex, 21);
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
        const size_t numWidgets = m_widgetTable.size();
        for (size_t i = 0; i < numWidgets; ++i)
        {
            if (m_widgetTable[i].m_isGroup && m_widgetTable[i].m_widget == widget)
            {
                return animGraph->FindNodeGroupByName(m_widgetTable[i].m_name.c_str());
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
        const size_t numWidgets = m_widgetTable.size();
        for (size_t i = 0; i < numWidgets; ++i)
        {
            if (m_widgetTable[i].m_isGroup == false && m_widgetTable[i].m_widget == widget)
            {
                return animGraph->RecursiveFindNodeByName(m_widgetTable[i].m_name.c_str());
            }
        }

        // the widget is not in the list, return failure
        return nullptr;
    }


    // called when the selection got changed
    void StateFilterSelectionWindow::OnSelectionChanged()
    {
        // reset the selection arrays
        m_selectedGroupNames.clear();
        m_selectedNodeIds.clear();

        // get the selected items and the number of them
        QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
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
                if (AZStd::find(m_selectedGroupNames.begin(), m_selectedGroupNames.end(), nodeGroup->GetName()) == m_selectedGroupNames.end())
                {
                    m_selectedGroupNames.emplace_back(nodeGroup->GetName());
                }
            }
        }
    }
} // namespace EMStudio
