/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/sort.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Widgets/ColorLabel.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include "MCore/Source/Config.h"
#include "NodeGroupWindow.h"
#include "AnimGraphPlugin.h"
#include "GraphNode.h"
#include "NodeGraph.h"
#include "BlendTreeVisualNode.h"
#include "BlendGraphWidget.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QMenu>
#include <QToolBar>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeGroupCommands.h>

#include <AzQtComponents/Utilities/Conversions.h>


namespace EMStudio
{
    NodeGroupRenameWindow::NodeGroupRenameWindow(QWidget* parent, EMotionFX::AnimGraph* animGraph, const AZStd::string& nodeGroup)
        : QDialog(parent)
    {
        // Store the values
        m_animGraph = animGraph;
        m_nodeGroup = nodeGroup;

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
        connect(m_lineEdit, &QLineEdit::textEdited, this, &NodeGroupRenameWindow::TextEdited);
        layout->addWidget(m_lineEdit);

        // set the current name and select all
        m_lineEdit->setText(nodeGroup.c_str());
        m_lineEdit->selectAll();

        // create the button layout
        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        m_okButton                   = new QPushButton("OK");
        QPushButton* cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(cancelButton);

        // connect the buttons
        connect(m_okButton, &QPushButton::clicked, this, &NodeGroupRenameWindow::Accepted);
        connect(cancelButton, &QPushButton::clicked, this, &NodeGroupRenameWindow::reject);

        // set the new layout
        layout->addLayout(buttonLayout);
        setLayout(layout);
    }


    void NodeGroupRenameWindow::TextEdited(const QString& text)
    {
        const AZStd::string convertedNewName = FromQtString(text);
        if (text.isEmpty())
        {
            m_okButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(m_lineEdit);
        }
        else if (m_nodeGroup == convertedNewName)
        {
            m_okButton->setEnabled(true);
            m_lineEdit->setStyleSheet("");
        }
        else
        {
            // find duplicate name in the anim graph other than this node group
            const size_t numNodeGroups = m_animGraph->GetNumNodeGroups();
            for (size_t i = 0; i < numNodeGroups; ++i)
            {
                EMotionFX::AnimGraphNodeGroup* nodeGroup = m_animGraph->GetNodeGroup(i);
                if (nodeGroup->GetNameString() == convertedNewName)
                {
                    m_okButton->setEnabled(false);
                    GetManager()->SetWidgetAsInvalidInput(m_lineEdit);
                    return;
                }
            }

            // no duplicate name found
            m_okButton->setEnabled(true);
            m_lineEdit->setStyleSheet("");
        }
    }


    void NodeGroupRenameWindow::Accepted()
    {
        // Execute the command
        AZStd::string outResult;
        const AZStd::string convertedNewName = FromQtString(m_lineEdit->text());
        auto* command = aznew CommandSystem::CommandAnimGraphAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandSystem::CommandAnimGraphAdjustNodeGroup::s_commandName),
            m_animGraph->GetID(),
            /*name = */ m_nodeGroup,
            /*visible = */ AZStd::nullopt,
            /*newName = */ convertedNewName
        );
        if (!GetCommandManager()->ExecuteCommand(command, outResult))
        {
            MCore::LogError(outResult.c_str());
        }

        // accept
        accept();
    }

    // constructor
    NodeGroupWindow::NodeGroupWindow(AnimGraphPlugin* plugin)
        : QWidget()
    {
        m_plugin             = plugin;
        m_tableWidget        = nullptr;
        m_addAction          = nullptr;

        // create and register the command callbacks
        m_createCallback     = new CommandAnimGraphAddNodeGroupCallback(false);
        m_removeCallback     = new CommandAnimGraphRemoveNodeGroupCallback(false);
        m_adjustCallback     = new CommandAnimGraphAdjustNodeGroupCallback(false);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddNodeGroup", m_createCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNodeGroup", m_removeCallback);
        GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandAnimGraphAdjustNodeGroup::s_commandName.data(), m_adjustCallback);

        // add the add button
        m_addAction = new QAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.svg"), tr("Add new node group"), this);
        connect(m_addAction, &QAction::triggered, this, &NodeGroupWindow::OnAddNodeGroup);

        // add the buttons to add, remove and clear the motions
        QToolBar* toolBar = new QToolBar();
        toolBar->addAction(m_addAction);

        toolBar->addSeparator();

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        m_searchWidget->setEnabledFiltersVisible(false);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &NodeGroupWindow::OnTextFilterChanged);
        toolBar->addWidget(m_searchWidget);

        // create the table widget
        m_tableWidget = new QTableWidget();
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setCornerButtonEnabled(false);
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &NodeGroupWindow::UpdateInterface);

        // set the column count
        m_tableWidget->setColumnCount(3);

        // set header items for the table
        QTableWidgetItem* headerItem = new QTableWidgetItem("Vis");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("Color");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(2, headerItem);

        // set the column params
        m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        m_tableWidget->setColumnWidth(0, 25);
        m_tableWidget->setColumnWidth(1, 41);
        m_tableWidget->horizontalHeader()->setVisible(false);

        m_tableWidget->setShowGrid(false);

        AzQtComponents::CheckBox::setVisibilityMode(m_tableWidget, true);
        connect(m_tableWidget, &QTableWidget::itemChanged, this, &NodeGroupWindow::OnItemChanged);

        // ser the horizontal header params
        QHeaderView* horizontalHeader = m_tableWidget->horizontalHeader();
        horizontalHeader->setSortIndicator(2, Qt::AscendingOrder);
        horizontalHeader->setStretchLastSection(true);

        // hide the vertical header
        QHeaderView* verticalHeader = m_tableWidget->verticalHeader();
        verticalHeader->setVisible(false);

        // create the vertical layout
        m_verticalLayout = new QVBoxLayout();
        m_verticalLayout->setSpacing(2);
        m_verticalLayout->setMargin(3);
        m_verticalLayout->setAlignment(Qt::AlignTop);
        m_verticalLayout->addWidget(toolBar);
        m_verticalLayout->addWidget(m_tableWidget);

        // set the object name
        setObjectName("StyledWidget");

        // create the fake widget and layout
        QWidget* fakeWidget = new QWidget();
        fakeWidget->setObjectName("StyledWidget");
        fakeWidget->setLayout(m_verticalLayout);

        QVBoxLayout* fakeLayout = new QVBoxLayout();
        fakeLayout->setMargin(0);
        fakeLayout->setSpacing(0);
        fakeLayout->setObjectName("StyledWidget");
        fakeLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
        fakeLayout->addWidget(fakeWidget);

        // set the layout
        setLayout(fakeLayout);

        // init
        Init();
    }


    // destructor
    NodeGroupWindow::~NodeGroupWindow()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(m_createCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_adjustCallback, false);
        delete m_createCallback;
        delete m_removeCallback;
        delete m_adjustCallback;
    }


    // create the list of parameters
    void NodeGroupWindow::Init()
    {
        // selected node groups array
        AZStd::vector<AZStd::string> selectedNodeGroups;

        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();

        // get the number of selected items
        const int numSelectedItems = selectedItems.count();

        // filter the items
        selectedNodeGroups.reserve(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            const int rowIndex = selectedItems[i]->row();
            const AZStd::string nodeGroupName = FromQtString(m_tableWidget->item(rowIndex, 2)->text());
            if (AZStd::find(begin(selectedNodeGroups), end(selectedNodeGroups), nodeGroupName) == end(selectedNodeGroups))
            {
                selectedNodeGroups.emplace_back(nodeGroupName);
            }
        }

        // clear the lookup array
        m_widgetTable.clear();

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            m_tableWidget->setRowCount(0);
            UpdateInterface();
            return;
        }

        // disable signals
        m_tableWidget->blockSignals(true);

        // get the number of node groups
        const int numNodeGroups = aznumeric_caster(animGraph->GetNumNodeGroups());

        // set table size and add header items
        m_tableWidget->setRowCount(numNodeGroups);

        // disable the sorting
        m_tableWidget->setSortingEnabled(false);

        // add each node group
        for (int i = 0; i < numNodeGroups; ++i)
        {
            // get a pointer to the node group
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            // check if the node group is selected
            const bool itemSelected = AZStd::find(begin(selectedNodeGroups), end(selectedNodeGroups), nodeGroup->GetNameString()) != end(selectedNodeGroups);

            // get the color and convert to Qt color
            AZ::Color color;
            color.FromU32(nodeGroup->GetColor());

            // create the visibility checkbox item
            QTableWidgetItem* visibilityCheckboxItem = new QTableWidgetItem();
            visibilityCheckboxItem->setFlags(visibilityCheckboxItem->flags() | Qt::ItemIsUserCheckable);
            visibilityCheckboxItem->setData(Qt::CheckStateRole, nodeGroup->GetIsVisible() ? Qt::Checked : Qt::Unchecked);

            // add the item, it's needed to have the background color + the widget
            m_tableWidget->setItem(i, 0, visibilityCheckboxItem);

            // create the color item
            QTableWidgetItem* colorItem = new QTableWidgetItem();

            // add the item, it's needed to have the background color + the widget
            m_tableWidget->setItem(i, 1, colorItem);

            // create the color widget
            AzQtComponents::ColorLabel* colorWidget = new AzQtComponents::ColorLabel(color);
            colorWidget->setTextInputVisible(false);

            QWidget* colorLayoutWidget = new QWidget();
            colorLayoutWidget->setObjectName("colorlayoutWidget");
            colorLayoutWidget->setStyleSheet("#colorlayoutWidget{ background: transparent; margin: 1px; }");
            QHBoxLayout* colorLayout = new QHBoxLayout();
            colorLayout->setAlignment(Qt::AlignCenter);
            colorLayout->setMargin(0);
            colorLayout->setSpacing(0);
            colorLayout->addWidget(colorWidget);
            colorLayoutWidget->setLayout(colorLayout);

            m_widgetTable.emplace_back(WidgetLookup{colorWidget, i});
            connect(colorWidget, &AzQtComponents::ColorLabel::colorChanged, this, &NodeGroupWindow::OnColorChanged);

            // add the color label in the table
            m_tableWidget->setCellWidget(i, 1, colorLayoutWidget);

            // create the node group name label
            QTableWidgetItem* nameItem = new QTableWidgetItem(nodeGroup->GetName());
            m_tableWidget->setItem(i, 2, nameItem);

            // set the item selected
            visibilityCheckboxItem->setSelected(itemSelected);
            colorItem->setSelected(itemSelected);
            nameItem->setSelected(itemSelected);

            // set the row height
            m_tableWidget->setRowHeight(i, 21);

            // check if the current item contains the find text
            if (QString(nodeGroup->GetName()).contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive))
            {
                m_tableWidget->showRow(i);
            }
            else
            {
                m_tableWidget->hideRow(i);
            }
        }

        // enable the sorting
        m_tableWidget->setSortingEnabled(true);

        // enable signals
        m_tableWidget->blockSignals(false);

        // update the interface
        UpdateInterface();
    }

    void NodeGroupWindow::OnItemChanged(QTableWidgetItem* item)
    {
        if (item->column() == 0)
        {
            OnIsVisible(item->data(Qt::CheckStateRole).toInt(), item->row());
        }
    }

    // add a new node group
    void NodeGroupWindow::OnAddNodeGroup()
    {
        // add the parameter
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            MCore::LogWarning("NodeGroupWindow::OnAddNodeGroup() - No AnimGraph active!");
            return;
        }

        AZStd::string commandString;
        AZStd::string resultString;
        commandString = AZStd::string::format("AnimGraphAddNodeGroup -animGraphID %i", animGraph->GetID());

        // execute the command
        if (GetCommandManager()->ExecuteCommand(commandString.c_str(), resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
        else
        {
            // select the new node group
            EMotionFX::AnimGraphNodeGroup* lastNodeGroup = animGraph->GetNodeGroup(animGraph->GetNumNodeGroups() - 1);
            const int numRows = m_tableWidget->rowCount();
            for (int i = 0; i < numRows; ++i)
            {
                if (m_tableWidget->item(i, 2)->text() == QString(lastNodeGroup->GetName()))
                {
                    m_tableWidget->selectRow(i);
                    break;
                }
            }
        }
    }


    // find the index for the given widget
    int NodeGroupWindow::FindGroupIndexByWidget(QObject* widget) const
    {
        const auto foundGroup = AZStd::find_if(begin(m_widgetTable), end(m_widgetTable), [widget](const auto& tableEntry)
        {
            return tableEntry.m_widget == widget;
        });
        return foundGroup != end(m_widgetTable) ? foundGroup->m_groupIndex : MCore::InvalidIndexT<int>;
    }


    void NodeGroupWindow::OnIsVisible(int state, int row)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the node group index by checking the widget lookup table
        const int groupIndex = row;
        assert(groupIndex != MCore::InvalidIndexT<int>);

        // get a pointer to the node group
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);

        bool isVisible = state == Qt::Checked;

        auto* command = aznew CommandSystem::CommandAnimGraphAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandSystem::CommandAnimGraphAdjustNodeGroup::s_commandName),
            /*animGraphId = */ animGraph->GetID(),
            /*name = */ nodeGroup->GetNameString(),
            /*visible = */ isVisible
        );

        // execute the command
        AZStd::string resultString;
        if (GetCommandManager()->ExecuteCommand(command, resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
    }


    // node group color changed
    void NodeGroupWindow::OnColorChanged(const AZ::Color& color)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the node group index by checking the widget lookup table
        const int groupIndex = FindGroupIndexByWidget(sender());
        assert(groupIndex != MCore::InvalidIndexT<int>);

        // get a pointer to the node group
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);

        // construct the command
        auto* command = aznew CommandSystem::CommandAnimGraphAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandSystem::CommandAnimGraphAdjustNodeGroup::s_commandName),
            /*animGraphId = */ animGraph->GetID(),
            /*name = */ nodeGroup->GetName(),
            /*visible = */ AZStd::nullopt,
            /*newName = */ AZStd::nullopt,
            /*nodeNames = */ AZStd::nullopt,
            /*nodeAction = */ AZStd::nullopt,
            /*color = */ color.ToU32()
        );

        // execute the command
        AZStd::string resultString;
        if (GetCommandManager()->ExecuteCommand(command, resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
    }


    void NodeGroupWindow::UpdateInterface()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            m_addAction->setEnabled(false);
            return;
        }

        m_addAction->setEnabled(true);
    }


    // remove a node group
    void NodeGroupWindow::OnRemoveSelectedGroups()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();

        // get the number of selected items
        const int numSelectedItems = selectedItems.count();
        if (selectedItems.empty())
        {
            return;
        }

        // filter the items
        AZStd::vector<int> rowIndices;
        rowIndices.reserve(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            const int rowIndex = selectedItems[i]->row();
            if (AZStd::find(begin(rowIndices), end(rowIndices), rowIndex) == end(rowIndices))
            {
                rowIndices.emplace_back(rowIndex);
            }
        }

        // sort the rows
        // it's used to select the next row
        AZStd::sort(begin(rowIndices), end(rowIndices));

        // get the number of selected rows
        const size_t numRowIndices = rowIndices.size();

        // set the command group name
        AZStd::string commandGroupName;
        if (numRowIndices == 1)
        {
            commandGroupName = "Remove 1 node group";
        }
        else
        {
            commandGroupName = AZStd::string::format("Remove %zu node groups", numRowIndices);
        }

        // create the command group
        MCore::CommandGroup internalCommandGroup(commandGroupName);

        // Add each command
        AZStd::string tempString;
        for (size_t i = 0; i < numRowIndices; ++i)
        {
            const AZStd::string nodeGroupName = FromQtString(m_tableWidget->item(rowIndices[i], 2)->text());
            if (i == 0 || i == numRowIndices - 1)
            {
                tempString = AZStd::string::format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), nodeGroupName.c_str());
            }
            else
            {
                tempString = AZStd::string::format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\" -nodeGroupWindowUpdate false", animGraph->GetID(), nodeGroupName.c_str());
            }
            internalCommandGroup.AddCommandString(tempString.c_str());
        }

        // execute the command group
        if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, tempString) == false)
        {
            MCore::LogError(tempString.c_str());
        }

        // selected the next row
        if (rowIndices[0] > (m_tableWidget->rowCount() - 1))
        {
            m_tableWidget->selectRow(rowIndices[0] - 1);
        }
        else
        {
            m_tableWidget->selectRow(rowIndices[0]);
        }
    }


    // rename a node group
    void NodeGroupWindow::OnRenameSelectedNodeGroup()
    {
        // take the item of the name column
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
        QTableWidgetItem* item = m_tableWidget->item(selectedItems[0]->row(), 2);

        // show the rename window
        NodeGroupRenameWindow nodeGroupRenameWindow(this, m_plugin->GetActiveAnimGraph(), FromQtString(item->text()));
        nodeGroupRenameWindow.exec();
    }


    void NodeGroupWindow::OnClearNodeGroups()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // make sure we really want to perform the action
        QMessageBox msgBox(QMessageBox::Warning, "Remove All Node Groups?", "Are you sure you want to remove all node groups from the anim graph?", QMessageBox::Yes | QMessageBox::No, this);
        msgBox.setTextFormat(Qt::RichText);
        if (msgBox.exec() != QMessageBox::Yes)
        {
            return;
        }

        CommandSystem::ClearNodeGroups(animGraph);
    }


    void NodeGroupWindow::OnTextFilterChanged(const QString& text)
    {
        FromQtString(text, &m_searchWidgetText);
        Init();
    }


    void NodeGroupWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            OnRemoveSelectedGroups();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void NodeGroupWindow::keyReleaseEvent(QKeyEvent* event)
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


    void NodeGroupWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();

        // get the number of selected items
        const int numSelectedItems = selectedItems.count();
        if (selectedItems.empty())
        {
            return;
        }

        // filter the items
        AZStd::vector<int> rowIndices;
        rowIndices.reserve(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            const int rowIndex = selectedItems[i]->row();
            if (AZStd::find(begin(rowIndices), end(rowIndices), rowIndex) == end(rowIndices))
            {
                rowIndices.emplace_back(rowIndex);
            }
        }

        // create the context menu
        QMenu menu(this);

        // add rename if only one selected
        if (rowIndices.size() == 1)
        {
            QAction* renameAction = menu.addAction("Rename Selected Node Group");
            connect(renameAction, &QAction::triggered, this, &NodeGroupWindow::OnRenameSelectedNodeGroup);
        }

        // at least one selected, remove action is possible
        if (!rowIndices.empty())
        {
            menu.addSeparator();
            QAction* removeAction = menu.addAction("Remove Selected Node Groups");
            connect(removeAction, &QAction::triggered, this, &NodeGroupWindow::OnRemoveSelectedGroups);
        }

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    bool UpdateAnimGraphNodeGroupWindow()
    {
        // get the plugin object
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = animGraphPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return false;
        }

        // re-init the node group window
        animGraphPlugin->GetNodeGroupWidget()->Init();
        return true;
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    // command callbacks
    //----------------------------------------------------------------------------------------------------------------------------------

    bool NodeGroupWindow::CommandAnimGraphAddNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); return commandLine.GetValueAsBool("updateUI", true) ? UpdateAnimGraphNodeGroupWindow() : true; }
    bool NodeGroupWindow::CommandAnimGraphAddNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)           { MCORE_UNUSED(command); return commandLine.GetValueAsBool("updateUI", true) ? UpdateAnimGraphNodeGroupWindow() : true; }
    bool NodeGroupWindow::CommandAnimGraphRemoveNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); return commandLine.GetValueAsBool("updateUI", true) ? UpdateAnimGraphNodeGroupWindow() : true; }
    bool NodeGroupWindow::CommandAnimGraphRemoveNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); return commandLine.GetValueAsBool("updateUI", true) ? UpdateAnimGraphNodeGroupWindow() : true; }
    bool NodeGroupWindow::CommandAnimGraphAdjustNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); return commandLine.GetValueAsBool("updateUI", true) ? UpdateAnimGraphNodeGroupWindow() : true; }
    bool NodeGroupWindow::CommandAnimGraphAdjustNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); return commandLine.GetValueAsBool("updateUI", true) ? UpdateAnimGraphNodeGroupWindow() : true; }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_NodeGroupWindow.cpp>
