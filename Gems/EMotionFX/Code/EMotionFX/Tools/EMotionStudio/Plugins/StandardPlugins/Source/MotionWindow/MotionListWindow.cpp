/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionListWindow.h"
#include "MotionWindowPlugin.h"
#include <QMenu>
#include <QTableWidget>
#include <QContextMenuEvent>
#include <QAction>
#include <QPushButton>
#include <QApplication>
#include <QApplication>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMimeData>
#include <QLineEdit>
#include <QMessageBox>
#include <QHeaderView>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/Source/MotionManager.h>
#include "../../../../EMStudioSDK/Source/NotificationWindow.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/FileManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/SaveChangedFilesManager.h"
#include "../MotionSetsWindow/MotionSetsWindowPlugin.h"

#include <AzQtComponents/Utilities/DesktopUtilities.h>

namespace EMStudio
{
    // constructor
    MotionListRemoveMotionsFailedWindow::MotionListRemoveMotionsFailedWindow(QWidget* parent, const AZStd::vector<EMotionFX::Motion*>& motions)
        : QDialog(parent)
    {
        // set the window title
        setWindowTitle("Remove Motions Failed");

        // resize the window
        resize(720, 405);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the top text
        layout->addWidget(new QLabel("The following motions failed to get removed because they are used by a motion set:"));

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
        const int numMotions = static_cast<int>(motions.size());
        tableWidget->setRowCount(numMotions);

        // add each motion in the table
        for (int i = 0; i < numMotions; ++i)
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
        connect(okButton, &QPushButton::clicked, this, &MotionListRemoveMotionsFailedWindow::accept);
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setAlignment(Qt::AlignRight);
        buttonLayout->addWidget(okButton);
        layout->addLayout(buttonLayout);

        // set the layout
        setLayout(layout);
    }


    // constructor
    MotionListWindow::MotionListWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin)
        : QWidget(parent)
    {
        setObjectName("MotionListWindow");
        m_motionTable        = nullptr;
        m_motionWindowPlugin = motionWindowPlugin;
    }


    // destructor
    MotionListWindow::~MotionListWindow()
    {
    }


    void MotionListWindow::Init()
    {
        m_vLayout = new QVBoxLayout();
        m_vLayout->setMargin(3);
        m_vLayout->setSpacing(2);
        m_motionTable = new MotionTableWidget(m_motionWindowPlugin, this);
        m_motionTable->setObjectName("EMFX.MotionListWindow.MotionTable");
        m_motionTable->setAlternatingRowColors(true);
        connect(m_motionTable, &MotionTableWidget::cellDoubleClicked, this, &MotionListWindow::cellDoubleClicked);
        connect(m_motionTable, &MotionTableWidget::itemSelectionChanged, this, &MotionListWindow::itemSelectionChanged);

        // set the table to row single selection
        m_motionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_motionTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // make the table items read only
        m_motionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // disable the corner button between the row and column selection thingies
        m_motionTable->setCornerButtonEnabled(false);

        // enable the custom context menu for the motion table
        m_motionTable->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the column count
        m_motionTable->setColumnCount(5);

        // add the name column
        QTableWidgetItem* nameHeaderItem = new QTableWidgetItem("Name");
        nameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_motionTable->setHorizontalHeaderItem(0, nameHeaderItem);

        // add the length column
        QTableWidgetItem* lengthHeaderItem = new QTableWidgetItem("Duration");
        lengthHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_motionTable->setHorizontalHeaderItem(1, lengthHeaderItem);

        // add the sub column
        QTableWidgetItem* subHeaderItem = new QTableWidgetItem("Joints");
        subHeaderItem->setToolTip("Number of joints inside the motion");
        subHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_motionTable->setHorizontalHeaderItem(2, subHeaderItem);

        // add the msub column
        QTableWidgetItem* msubHeaderItem = new QTableWidgetItem("Morphs");
        msubHeaderItem->setToolTip("Number of morph targets inside the motion");
        msubHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_motionTable->setHorizontalHeaderItem(3, msubHeaderItem);

        // add the type column
        QTableWidgetItem* typeHeaderItem = new QTableWidgetItem("Type");
        typeHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_motionTable->setHorizontalHeaderItem(4, typeHeaderItem);

        // set the sorting order on the first column
        m_motionTable->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);

        // hide the vertical columns
        QHeaderView* verticalHeader = m_motionTable->verticalHeader();
        verticalHeader->setVisible(false);

        // set the last column to take the whole available space
        m_motionTable->horizontalHeader()->setStretchLastSection(true);

        // set the column width
        m_motionTable->setColumnWidth(0, 300);
        m_motionTable->setColumnWidth(1, 55);
        m_motionTable->setColumnWidth(2, 50);
        m_motionTable->setColumnWidth(3, 55);
        m_motionTable->setColumnWidth(4, 105);

        m_vLayout->addWidget(m_motionTable);
        setLayout(m_vLayout);

        ReInit();
    }


    // called when the filter string changed
    void MotionListWindow::OnTextFilterChanged(const QString& text)
    {
        m_searchWidgetText = text.toLower().toUtf8().data();
        ReInit();
    }


    bool MotionListWindow::AddMotionByID(uint32 motionID)
    {
        // find the motion entry based on the id
        MotionWindowPlugin::MotionTableEntry* motionEntry = m_motionWindowPlugin->FindMotionEntryByID(motionID);
        if (motionEntry == nullptr)
        {
            return false;
        }

        // if the motion is not visible at all skip it completely
        if (CheckIfIsMotionVisible(motionEntry) == false)
        {
            return true;
        }

        // get the motion
        EMotionFX::Motion* motion = motionEntry->m_motion;

        // disable the sorting
        m_motionTable->setSortingEnabled(false);

        // insert the new row
        const int rowIndex = 0;
        m_motionTable->insertRow(rowIndex);
        m_motionTable->setRowHeight(rowIndex, 21);

        // create the name item
        QTableWidgetItem* nameTableItem = new QTableWidgetItem(motion->GetName());

        // store the motion ID on this item
        nameTableItem->setData(Qt::UserRole, motion->GetID());

        // set the tooltip to the filename
        nameTableItem->setToolTip(motion->GetFileName());

        // set the item in the motion table
        m_motionTable->setItem(rowIndex, 0, nameTableItem);

        // create the length item
        AZStd::string length;
        length = AZStd::string::format("%.2f sec", motion->GetDuration());
        QTableWidgetItem* lengthTableItem = new QTableWidgetItem(length.c_str());

        // set the item in the motion table
        m_motionTable->setItem(rowIndex, 1, lengthTableItem);

        // set the sub and msub text
        AZStd::string sub, msub;
        EMotionFX::MotionData* motionData = motion->GetMotionData();
        sub = AZStd::string::format("%zu", motionData->GetNumJoints());
        msub = AZStd::string::format("%zu", motionData->GetNumMorphs());

        // create the sub and msub item
        QTableWidgetItem* subTableItem = new QTableWidgetItem(sub.c_str());
        QTableWidgetItem* msubTableItem = new QTableWidgetItem(msub.c_str());

        // set the items in the motion table
        m_motionTable->setItem(rowIndex, 2, subTableItem);
        m_motionTable->setItem(rowIndex, 3, msubTableItem);

        // create and set the type item
        QTableWidgetItem* typeTableItem = new QTableWidgetItem(motionData->RTTI_GetTypeName());
        m_motionTable->setItem(rowIndex, 4, typeTableItem);

        // set the items italic if the motion is dirty
        if (motion->GetDirtyFlag())
        {
            // create the font italic, all columns use the same font
            QFont font = subTableItem->font();
            font.setItalic(true);

            // set the font for each item
            nameTableItem->setFont(font);
            lengthTableItem->setFont(font);
            subTableItem->setFont(font);
            msubTableItem->setFont(font);
            typeTableItem->setFont(font);
        }

        // enable the sorting
        m_motionTable->setSortingEnabled(true);

        // update the interface
        UpdateInterface();

        // return true because the row is correctly added
        return true;
    }


    uint32 MotionListWindow::FindRowByMotionID(uint32 motionID)
    {
        // iterate through the rows and compare the motion IDs
        const int rowCount = m_motionTable->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            if (GetMotionID(i) == motionID)
            {
                return i;
            }
        }

        // failure, motion id not found in any of the rows
        return MCORE_INVALIDINDEX32;
    }


    bool MotionListWindow::RemoveMotionByID(uint32 motionID)
    {
        // find the row for the motion
        const uint32 rowIndex = FindRowByMotionID(motionID);
        if (rowIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // remove the row
        m_motionTable->removeRow(rowIndex);

        // update the interface
        UpdateInterface();

        // return true because the row is correctly removed
        return true;
    }


    bool MotionListWindow::CheckIfIsMotionVisible(MotionWindowPlugin::MotionTableEntry* entry)
    {
        if (entry->m_motion->GetIsOwnedByRuntime())
        {
            return false;
        }

        AZStd::string motionNameLowered = entry->m_motion->GetNameString();
        AZStd::to_lower(motionNameLowered.begin(), motionNameLowered.end());
        if (m_searchWidgetText.empty() || motionNameLowered.find(m_searchWidgetText) != AZStd::string::npos)
        {
            return true;
        }
        return false;
    }


    void MotionListWindow::ReInit()
    {
        const CommandSystem::SelectionList selection = GetCommandManager()->GetCurrentSelection();

        size_t numMotions = m_motionWindowPlugin->GetNumMotionEntries();
        m_shownMotionEntries.clear();
        m_shownMotionEntries.reserve(numMotions);

        for (size_t i = 0; i < numMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = m_motionWindowPlugin->GetMotionEntry(i);
            if (CheckIfIsMotionVisible(entry))
            {
                m_shownMotionEntries.push_back(entry);
            }
        }
        numMotions = m_shownMotionEntries.size();

        // set the number of rows
        m_motionTable->setRowCount(static_cast<int>(numMotions));

        // set the sorting disabled
        m_motionTable->setSortingEnabled(false);

        // iterate through the motions and fill in the table
        for (int i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = m_shownMotionEntries[static_cast<size_t>(i)]->m_motion;

            // set the row height
            m_motionTable->setRowHeight(i, 21);

            // create the name item
            QTableWidgetItem* nameTableItem = new QTableWidgetItem(motion->GetName());

            // store the motion ID on this item
            nameTableItem->setData(Qt::UserRole, motion->GetID());

            // set tooltip to filename
            nameTableItem->setToolTip(motion->GetFileName());

            // set the item in the motion table
            m_motionTable->setItem(i, 0, nameTableItem);

            // create the length item
            AZStd::string length;
            length = AZStd::string::format("%.2f sec", motion->GetDuration());
            QTableWidgetItem* lengthTableItem = new QTableWidgetItem(length.c_str());

            // set the item in the motion table
            m_motionTable->setItem(i, 1, lengthTableItem);

            // set the sub and msub text
            AZStd::string sub, msub;
            EMotionFX::MotionData* motionData = motion->GetMotionData();
            sub = AZStd::string::format("%zu", motionData->GetNumJoints());
            msub = AZStd::string::format("%zu", motionData->GetNumMorphs());

            // create the sub and msub item
            QTableWidgetItem* subTableItem = new QTableWidgetItem(sub.c_str());
            QTableWidgetItem* msubTableItem = new QTableWidgetItem(msub.c_str());

            // set the items in the motion table
            m_motionTable->setItem(i, 2, subTableItem);
            m_motionTable->setItem(i, 3, msubTableItem);

            // create and set the type item
            QTableWidgetItem* typeTableItem = new QTableWidgetItem(motionData->RTTI_GetTypeName());
            m_motionTable->setItem(i, 4, typeTableItem);

            // set the items italic if the motion is dirty
            if (motion->GetDirtyFlag())
            {
                // create the font italic, all columns use the same font
                QFont font = subTableItem->font();
                font.setItalic(true);

                // set the font for each item
                nameTableItem->setFont(font);
                lengthTableItem->setFont(font);
                subTableItem->setFont(font);
                msubTableItem->setFont(font);
                typeTableItem->setFont(font);
            }
        }

        // set the sorting enabled
        m_motionTable->setSortingEnabled(true);

        // set the old selection as before the reinit
        UpdateSelection(selection);
    }


    // update the selection
    void MotionListWindow::UpdateSelection(const CommandSystem::SelectionList& selectionList)
    {
        // block signals to not have the motion table events when selection changed
        m_motionTable->blockSignals(true);

        // clear the selection
        m_motionTable->clearSelection();

        // iterate through the selected motions and select the corresponding rows in the table widget
        const size_t numSelectedMotions = selectionList.GetNumSelectedMotions();
        for (uint32 i = 0; i < numSelectedMotions; ++i)
        {
            // get the index of the motion inside the motion manager (which is equal to the row in the motion table) and select the row at the motion index
            EMotionFX::Motion* selectedMotion = selectionList.GetMotion(i);
            const uint32 row = FindRowByMotionID(selectedMotion->GetID());
            if (row != MCORE_INVALIDINDEX32)
            {
                // select the entire row
                const int columnCount = m_motionTable->columnCount();
                for (int c = 0; c < columnCount; ++c)
                {
                    QTableWidgetItem* tableWidgetItem = m_motionTable->item(row, c);
                    tableWidgetItem->setSelected(true);
                }
            }
        }

        // enable the signals now all rows are selected
        m_motionTable->blockSignals(false);

        // call the selection changed
        itemSelectionChanged();
    }


    void MotionListWindow::UpdateInterface()
    {
    }


    uint32 MotionListWindow::GetMotionID(uint32 rowIndex)
    {
        QTableWidgetItem* tableItem = m_motionTable->item(rowIndex, 0);
        if (tableItem)
        {
            return tableItem->data(Qt::UserRole).toInt();
        }
        return MCORE_INVALIDINDEX32;
    }


    void MotionListWindow::cellDoubleClicked(int row, int column)
    {
        MCORE_UNUSED(column);

        const uint32        motionID    = GetMotionID(row);
        EMotionFX::Motion*  motion      = EMotionFX::GetMotionManager().FindMotionByID(motionID);

        if (motion)
        {
            m_motionWindowPlugin->PlayMotion(motion);
        }
    }


    void MotionListWindow::itemSelectionChanged()
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = m_motionTable->selectedItems();

        // get the number of selected items
        const int numSelectedItems = selectedItems.count();

        // filter the items
        AZStd::vector<int> rowIndices;
        rowIndices.reserve(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            const int rowIndex = selectedItems[i]->row();
            if (AZStd::find(rowIndices.begin(), rowIndices.end(), rowIndex) == rowIndices.end())
            {
                rowIndices.push_back(rowIndex);
            }
        }

        // clear the selected motion IDs
        m_selectedMotionIDs.clear();

        // get the number of selected items and iterate through them
        m_selectedMotionIDs.reserve(rowIndices.size());
        for (const int rowIndex : rowIndices)
        {
            m_selectedMotionIDs.push_back(GetMotionID(rowIndex));
        }

        // unselect all motions
        GetCommandManager()->GetCurrentSelection().ClearMotionSelection();

        // get the number of selected motions and iterate through them
        for (uint32 selectedMotionID : m_selectedMotionIDs)
        {
            // find the motion by name in the motion library and select it
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(selectedMotionID);
            if (motion)
            {
                GetCommandManager()->GetCurrentSelection().AddMotion(motion);
            }
        }

        // update the interface
        m_motionWindowPlugin->UpdateInterface();

        // emit signal that tells other windows that the motion selection changed
        emit MotionSelectionChanged();
    }

    // add the selected motions in the selected motion sets
    void MotionListWindow::OnAddMotionsInSelectedMotionSets()
    {
        // get the current selection
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const size_t numSelectedMotions = selection.GetNumSelectedMotions();
        if (numSelectedMotions == 0)
        {
            return;
        }

        // get the motion sets window plugin
        EMStudioPlugin* motionWindowPlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (motionWindowPlugin == nullptr)
        {
            return;
        }

        // Get the selected motion sets.
        AZStd::vector<EMotionFX::MotionSet*> selectedMotionSets;
        MotionSetsWindowPlugin* motionSetsWindowPlugin = static_cast<MotionSetsWindowPlugin*>(motionWindowPlugin);
        motionSetsWindowPlugin->GetManagementWindow()->GetSelectedMotionSets(selectedMotionSets);
        if (selectedMotionSets.empty())
        {
            return;
        }

        // Set the command group name based on the number of motions to add.
        AZStd::string groupName;
        if (numSelectedMotions > 1)
        {
            groupName = "Add motions in motion sets";
        }
        else
        {
            groupName = "Add motion in motion sets";
        }

        MCore::CommandGroup commandGroup(groupName);

        // add in each selected motion set
        AZStd::string motionName;
        for (const EMotionFX::MotionSet* motionSet : selectedMotionSets)
        {
            // Build a list of unique string id values from all motion set entries.
            AZStd::vector<AZStd::string> idStrings;
            motionSet->BuildIdStringList(idStrings);

            // add each selected motion in the motion set
            for (size_t i = 0; i < numSelectedMotions; ++i)
            {
                // remove the media root folder from the absolute motion filename so that we get the relative one to the media root folder
                motionName = selection.GetMotion(i)->GetFileName();
                EMotionFX::GetEMotionFX().GetFilenameRelativeToMediaRoot(&motionName);

                // construct and call the command for actually adding it
                CommandSystem::AddMotionSetEntry(motionSet->GetID(), "", idStrings, motionName.c_str(), &commandGroup);
            }
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionListWindow::OnOpenInFileBrowser()
    {
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        // iterate through the selected motions and show them
        for (size_t i = 0; i < selection.GetNumSelectedMotions(); ++i)
        {
            EMotionFX::Motion* motion = selection.GetMotion(i);
            AzQtComponents::ShowFileOnDesktop(motion->GetFileName());
        }
    }


    void MotionListWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            emit RemoveMotionsRequested();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void MotionListWindow::keyReleaseEvent(QKeyEvent* event)
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


    void MotionListWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        // get the current selection
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        // create the context menu
        QMenu menu(this);

        // add the motion related menus
        if (selection.GetNumSelectedMotions() > 0)
        {
            // get the motion sets window plugin
            EMStudioPlugin* motionWindowPlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
            if (motionWindowPlugin)
            {
                // Get the selected motion sets.
                AZStd::vector<EMotionFX::MotionSet*> selectedMotionSets;
                MotionSetsWindowPlugin* motionSetsWindowPlugin = static_cast<MotionSetsWindowPlugin*>(motionWindowPlugin);
                motionSetsWindowPlugin->GetManagementWindow()->GetSelectedMotionSets(selectedMotionSets);

                // add the menu if at least one motion set is selected
                if (!selectedMotionSets.empty())
                {
                    // add the menu to add in motion sets
                    QAction* addInSelectedMotionSetsAction = menu.addAction("Add To Selected Motion Sets");
                    connect(addInSelectedMotionSetsAction, &QAction::triggered, this, &MotionListWindow::OnAddMotionsInSelectedMotionSets);

                    menu.addSeparator();
                }
            }

            // add the remove menu
            QAction* removeAction = menu.addAction("Remove Selected Motions");
            removeAction->setObjectName("EMFX.MotionListWindow.RemoveSelectionMotionsAction");
            connect(removeAction, &QAction::triggered, this, &MotionListWindow::RemoveMotionsRequested);

            menu.addSeparator();

            // add the save menu
            QAction* saveAction = menu.addAction("Save Selected Motions");
            connect(saveAction, &QAction::triggered, this, &MotionListWindow::SaveRequested);

            menu.addSeparator();

            // browse in explorer option
            QAction* browserAction = menu.addAction(AzQtComponents::fileBrowserActionName());
            connect(browserAction, &QAction::triggered, this, &MotionListWindow::OnOpenInFileBrowser);
        }

        // show the menu at the given position
        if (menu.isEmpty() == false)
        {
            menu.exec(event->globalPos());
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    MotionTableWidget::MotionTableWidget(MotionWindowPlugin* parentPlugin, QWidget* parent)
        : QTableWidget(parent)
    {
        m_plugin = parentPlugin;

        // enable dragging
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragOnly);
    }


    // destructor
    MotionTableWidget::~MotionTableWidget()
    {
    }


    // return the mime data
    QMimeData* MotionTableWidget::mimeData(const QList<QTableWidgetItem*> items) const
    {
        MCORE_UNUSED(items);

        // get the current selection list
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();

        // get the number of selected motions and return directly if there are no motions selected
        AZStd::string textData, command;
        const size_t numMotions = selectionList.GetNumSelectedMotions();
        for (size_t i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = selectionList.GetMotion(i);

            // construct the drag&drop data string
            command = AZStd::string::format("-window \"MotionWindow\" -motionID %i\n", motion->GetID());
            textData += command;
        }

        // create the data, set the text and return it
        QMimeData* mimeData = new QMimeData();
        mimeData->setText(textData.c_str());
        return mimeData;
    }


    // return the supported mime types
    QStringList MotionTableWidget::mimeTypes() const
    {
        QStringList result;
        result.append("text/plain");
        return result;
    }


    // get the allowed drop actions
    Qt::DropActions MotionTableWidget::supportedDropActions() const
    {
        return Qt::CopyAction;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/moc_MotionListWindow.cpp>
