/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzCore/std/algorithm.h"
#include "MotionSetsWindowPlugin.h"
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzCore/IO/FileIO.h>
#include <QAction>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QHeaderView>
#include <QMenu>
#include <QUrl>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QTableWidget>
#include <QMimeData>
#include <QSplitter>
#include <QClipboard>
#include <QApplication>
#include <QDesktopWidget>
#include <QToolBar>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/FileManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <MCore/Source/FileSystem.h>
#include "MotionSetManagementWindow.h"
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <Editor/SaveDirtyFilesCallbacks.h>

namespace EMStudio
{
    MotionSetRemoveMotionsFailedWindow::MotionSetRemoveMotionsFailedWindow(QWidget* parent, const AZStd::vector<EMotionFX::Motion*>& motions)
        : QDialog(parent)
    {
        // set the window title
        setWindowTitle("Remove Motions Failed");

        // resize the window
        resize(720, 405);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the top text
        layout->addWidget(new QLabel("The following motions failed to get removed because they are used by another motion set:"));

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
        headerLabels.append("Filename");
        tableWidget->setHorizontalHeaderLabels(headerLabels);
        tableWidget->horizontalHeader()->setStretchLastSection(true);
        tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        tableWidget->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
        tableWidget->verticalHeader()->setVisible(false);

        // Set the number of rows.
        const size_t numMotions = motions.size();
        tableWidget->setRowCount(static_cast<int>(numMotions));

        // Add each motion in the table.
        for (size_t i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = motions[i];

            // create the name table widget item
            QTableWidgetItem* nameTableWidgetItem = new QTableWidgetItem(motion->GetName());
            nameTableWidgetItem->setToolTip(motion->GetName());

            // create the filename table widget item
            QTableWidgetItem* fileNameTableWidgetItem = new QTableWidgetItem(motion->GetFileName());
            fileNameTableWidgetItem->setToolTip(motion->GetFileName());

            // set the text of the row
            const int row = static_cast<int>(i);
            tableWidget->setItem(row, 0, nameTableWidgetItem);
            tableWidget->setItem(row, 1, fileNameTableWidgetItem);
        }

        // resize the first column to contents
        tableWidget->resizeColumnToContents(0);

        // add the table widget in the layout
        layout->addWidget(tableWidget);

        // add the button to close the window
        QPushButton* okButton = new QPushButton("OK");
        connect(okButton, &QPushButton::clicked, this, &MotionSetRemoveMotionsFailedWindow::accept);
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setAlignment(Qt::AlignRight);
        buttonLayout->addWidget(okButton);
        layout->addLayout(buttonLayout);

        // set the layout
        setLayout(layout);
    }


    RenameMotionEntryWindow::RenameMotionEntryWindow(QWidget* parent, EMotionFX::MotionSet* motionSet, const AZStd::string& motionId)
        : QDialog(parent)
    {
        m_motionSet = motionSet;
        m_motionId = motionId;

        // Build a list of unique string id values from all motion set entries.
        m_motionSet->BuildIdStringList(m_existingIds);

        // Set the window title and minimum width.
        setWindowTitle("Enter new motion ID");
        setMinimumWidth(300);

        QVBoxLayout* layout = new QVBoxLayout();

        m_lineEdit = new QLineEdit();
        connect(m_lineEdit, &QLineEdit::textEdited, this, &RenameMotionEntryWindow::TextEdited);
        layout->addWidget(m_lineEdit);

        // Set the old motion id as text and select all so that the user can directly start typing.
        m_lineEdit->setText(m_motionId.c_str());
        m_lineEdit->selectAll();

        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        m_okButton                   = new QPushButton("OK");
        QPushButton* cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(cancelButton);

        // Allow pressing the enter key as alternative to pressing the ok button for faster workflow.
        m_okButton->setAutoDefault(true);
        m_okButton->setDefault(true);

        connect(m_okButton, &QPushButton::clicked, this, &RenameMotionEntryWindow::Accepted);
        connect(cancelButton, &QPushButton::clicked, this, &RenameMotionEntryWindow::reject);

        layout->addLayout(buttonLayout);
        setLayout(layout);
    }


    void RenameMotionEntryWindow::TextEdited(const QString& text)
    {
        const AZStd::string newId = text.toUtf8().data();

        // Disable the ok button and put the text edit in error state in case the new motion id is either empty or does already exist in the motion set.
        if (newId.empty() || AZStd::find(m_existingIds.begin(), m_existingIds.end(), newId) != m_existingIds.end())
        {
            m_okButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(m_lineEdit);
            return;
        }

        m_okButton->setEnabled(true);
        m_lineEdit->setStyleSheet("");
    }


    void RenameMotionEntryWindow::Accepted()
    {
        AZStd::string commandString = AZStd::string::format("MotionSetAdjustMotion -motionSetID %i -idString \"%s\" -newIDString \"%s\" -updateMotionNodeStringIDs true",
                m_motionSet->GetID(),
                m_motionId.c_str(),
                m_lineEdit->text().toUtf8().data());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        accept();
    }


    // constructor
    MotionSetWindow::MotionSetWindow(MotionSetsWindowPlugin* parentPlugin, QWidget* parent)
        : QWidget(parent)
    {
        m_plugin             = parentPlugin;
    }


    // destructor
    MotionSetWindow::~MotionSetWindow()
    {
    }


    bool MotionSetWindow::Init()
    {
        setAcceptDrops(true);

        // create the main aidget and put it to the dialog stack
        QVBoxLayout* layout = new QVBoxLayout();
        setLayout(layout);
        layout->setMargin(0);

        QVBoxLayout* tableLayout = new QVBoxLayout();
        tableLayout->setMargin(0);
        tableLayout->setSpacing(2);

        QToolBar* toolBar = new QToolBar(this);
        toolBar->setObjectName("MotionSetWindow.ToolBar");

        m_addAction = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.svg"),
            tr("Add a new entry"),
            this, &MotionSetWindow::OnAddNewEntry);
        m_addAction->setObjectName("MotionSetWindow.ToolBar.AddANewEntry");

        m_loadAction = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.svg"),
            tr("Add entries by selecting motions."),
            this, &MotionSetWindow::OnLoadEntries);

        m_saveAction = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Menu/FileSave.svg"),
            tr("Save selected motions"),
            this, &MotionSetWindow::OnSave);

        toolBar->addSeparator();

        m_editAction = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Edit.svg"),
            tr("Batch edit selected motion IDs"),
            this, &MotionSetWindow::OnEditButton);


        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        toolBar->addWidget(spacerWidget);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &MotionSetWindow::OnTextFilterChanged);
        toolBar->addWidget(m_searchWidget);

        layout->addWidget(toolBar);


        // left side
        m_tableWidget = new MotionSetTableWidget(m_plugin, this);
        m_tableWidget->setObjectName("EMFX.MotionSetWindow.TableWidget");
        tableLayout->addWidget(m_tableWidget);
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setGridStyle(Qt::SolidLine);
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_tableWidget->setCornerButtonEnabled(false);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        connect(m_tableWidget, &MotionSetTableWidget::itemDoubleClicked, this, &MotionSetWindow::OnEntryDoubleClicked);
        connect(m_tableWidget, &MotionSetTableWidget::itemSelectionChanged, this, &MotionSetWindow::UpdateInterface);

        // set the column count
        m_tableWidget->setColumnCount(7);

        // set the column labels
        QTableWidgetItem* headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(0, headerItem);

        headerItem = new QTableWidgetItem("ID");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(1, headerItem);

        headerItem = new QTableWidgetItem("Duration");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(2, headerItem);

        headerItem = new QTableWidgetItem("Joints");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        headerItem->setToolTip("The number of joints inside the motion");
        m_tableWidget->setHorizontalHeaderItem(3, headerItem);

        headerItem = new QTableWidgetItem("Morphs");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        headerItem->setToolTip("The number of morph targets inside the motion.");
        m_tableWidget->setHorizontalHeaderItem(4, headerItem);

        headerItem = new QTableWidgetItem("Type");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(5, headerItem);

        headerItem = new QTableWidgetItem("Filename");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(6, headerItem);

        // set the column params
        m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        m_tableWidget->horizontalHeader()->setSortIndicator(1, Qt::AscendingOrder);

        // hide the vertical columns
        QHeaderView* verticalHeader = m_tableWidget->verticalHeader();
        verticalHeader->setVisible(false);

        // set the last column to take the whole available space
        m_tableWidget->horizontalHeader()->setStretchLastSection(true);

        // set the column width
        m_tableWidget->setColumnWidth(0, 23);
        m_tableWidget->setColumnWidth(1, 300);
        m_tableWidget->setColumnWidth(2, 55);
        m_tableWidget->setColumnWidth(3, 45);
        m_tableWidget->setColumnWidth(4, 50);
        m_tableWidget->setColumnWidth(5, 100);

        layout->addLayout(tableLayout);

        return true;
    }


    void MotionSetWindow::ReInit()
    {
        EMotionFX::MotionSet* selectedSet = m_plugin->GetSelectedSet();
        const size_t selectedSetIndex = EMotionFX::GetMotionManager().FindMotionSetIndex(selectedSet);
        if (selectedSetIndex != InvalidIndex)
        {
            UpdateMotionSetTable(m_tableWidget, m_plugin->GetSelectedSet());
        }
        else
        {
            UpdateMotionSetTable(m_tableWidget, nullptr);
        }
    }


    bool MotionSetWindow::AddMotion(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry)
    {
        // check if the motion set is the one we currently see in the interface, if not there is nothing to do
        if (m_plugin->GetSelectedSet() == motionSet)
        {
            InsertRow(motionSet, motionEntry, m_tableWidget, false);
        }

        UpdateInterface();
        return true;
    }


    bool MotionSetWindow::UpdateMotion(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry, const AZStd::string& oldMotionId)
    {
        int rowIndex = -1;
        const int rowCount = m_tableWidget->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            QTableWidgetItem* item = m_tableWidget->item(i, 1);
            if (item->text() == oldMotionId.c_str())
            {
                rowIndex = i;
                break;
            }
        }

        // Check if the motion set is the one we currently see in the interface, if not there is nothing to do.
        if (m_plugin->GetSelectedSet() == motionSet)
        {
            FillRow(motionSet, motionEntry, rowIndex, m_tableWidget, false);
        }

        UpdateInterface();
        return true;
    }


    bool MotionSetWindow::RemoveMotion(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry)
    {
        // Check if the motion set is the one we currently see in the interface, if not there is nothing to do.
        if (m_plugin->GetSelectedSet() == motionSet)
        {
            RemoveRow(motionSet, motionEntry, m_tableWidget);
        }

        UpdateInterface();
        return true;
    }


    void MotionSetWindow::PlayMotion(EMotionFX::Motion* motion)
    {
        if (!motion)
        {
            AZ_Assert(false, "Can't play an empty motion.");
            return;
        }

        MCore::CommandGroup commandGroup("Play motion");
        AZStd::string command, commandParameters;

        commandGroup.AddCommandString("Unselect -motionIndex SELECT_ALL");

        command = AZStd::string::format("Select -motionIndex %zu", EMotionFX::GetMotionManager().FindMotionIndexByID(motion->GetID()));
        commandGroup.AddCommandString(command);

        EMotionFX::PlayBackInfo* defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();
        if (defaultPlayBackInfo)
        {
            // Don't blend in and out of the for previewing animations. We might only see a short bit of it for animations smaller than the blend in/out time.
            defaultPlayBackInfo->m_blendInTime = 0.0f;
            defaultPlayBackInfo->m_blendOutTime = 0.0f;
            defaultPlayBackInfo->m_freezeAtLastFrame = (defaultPlayBackInfo->m_numLoops != EMFX_LOOPFOREVER);
            commandParameters = CommandSystem::CommandPlayMotion::PlayBackInfoToCommandParameters(defaultPlayBackInfo);
        }

        command = AZStd::string::format("PlayMotion -filename \"%s\" %s", motion->GetFileName(), commandParameters.c_str());
        commandGroup.AddCommandString(command);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    bool MotionSetWindow::FillRow(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry, uint32 rowIndex, QTableWidget* tableWidget, bool readOnly)
    {
        // Preload motions to get infos
        //motionSet->Preload();
        motionSet->LoadMotion(motionEntry);

        // disable the sorting to avoid row index issue
        tableWidget->setSortingEnabled(false);

        // create the id entry
        if (readOnly)
        {
            QTableWidgetItem* idTableItem = new QTableWidgetItem(motionEntry->GetId().c_str());
            tableWidget->setItem(rowIndex, 1, idTableItem);
            idTableItem->setFlags(Qt::NoItemFlags);
        }
        else
        {
            QTableWidgetItem* idTableItem = new QTableWidgetItem(motionEntry->GetId().c_str());
            tableWidget->setItem(rowIndex, 1, idTableItem);
        }

        // get the entry motion
        EMotionFX::Motion* entryMotion = motionEntry->GetMotion();

        // create the length entry
        AZStd::string tempString;
        if (entryMotion)
        {
            tempString = AZStd::string::format("%.2f sec", entryMotion->GetDuration());
        }
        else
        {
            tempString = "";
        }
        QTableWidgetItem* lengthTableItem = new QTableWidgetItem(tempString.c_str());
        tableWidget->setItem(rowIndex, 2, lengthTableItem);
        if (readOnly)
        {
            lengthTableItem->setFlags(Qt::NoItemFlags);
        }

        // skeletal motion only
        if (entryMotion)
        {
            // create the sub entry
            AZStd::to_string(tempString, entryMotion->GetMotionData()->GetNumJoints());
            QTableWidgetItem* subTableItem = new QTableWidgetItem(tempString.c_str());
            tableWidget->setItem(rowIndex, 3, subTableItem);
            if (readOnly)
            {
                subTableItem->setFlags(Qt::NoItemFlags);
            }

            // create the msub entry
            AZStd::to_string(tempString, entryMotion->GetMotionData()->GetNumMorphs());
            QTableWidgetItem* msubTableItem = new QTableWidgetItem(tempString.c_str());
            tableWidget->setItem(rowIndex, 4, msubTableItem);
            if (readOnly)
            {
                msubTableItem->setFlags(Qt::NoItemFlags);
            }
        }
        else
        {
            // create the sub entry
            QTableWidgetItem* subTableItem = new QTableWidgetItem("");
            tableWidget->setItem(rowIndex, 3, subTableItem);
            if (readOnly)
            {
                subTableItem->setFlags(Qt::NoItemFlags);
            }

            // create the msub entry
            QTableWidgetItem* msubTableItem = new QTableWidgetItem("");
            tableWidget->setItem(rowIndex, 4, msubTableItem);
            if (readOnly)
            {
                msubTableItem->setFlags(Qt::NoItemFlags);
            }
        }

        // create the type entry
        if (entryMotion)
        {
            QTableWidgetItem* typeTableItem = new QTableWidgetItem(entryMotion->GetMotionData()->RTTI_GetTypeName());
            tableWidget->setItem(rowIndex, 5, typeTableItem);
            if (readOnly)
            {
                typeTableItem->setFlags(Qt::NoItemFlags);
            }
        }
        else
        {
            QTableWidgetItem* typeTableItem = new QTableWidgetItem("");
            tableWidget->setItem(rowIndex, 5, typeTableItem);
            if (readOnly)
            {
                typeTableItem->setFlags(Qt::NoItemFlags);
            }
        }

        // create the filename entry
        QTableWidgetItem* filenameTableItem = new QTableWidgetItem(motionEntry->GetFilename());
        tableWidget->setItem(rowIndex, 6, filenameTableItem);
        if (readOnly)
        {
            filenameTableItem->setFlags(Qt::NoItemFlags);
        }

        // Show the exclamation mark in case the motion file cannot be found.
        const AZStd::string motionFileName = motionSet->ConstructMotionFilename(motionEntry);
        if (!motionFileName.empty() && !AZ::IO::FileIOBase::GetInstance()->Exists(motionFileName.c_str()))
        {
            const AZStd::string tooltipText = "The motion file cannot be located. Please check if the file exists on disk.";

            QTableWidgetItem* exclamationTableItem = new QTableWidgetItem("");
            exclamationTableItem->setFlags(Qt::NoItemFlags);
            exclamationTableItem->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/ExclamationMark.svg"));
            exclamationTableItem->setToolTip(tooltipText.c_str());
            tableWidget->setItem(rowIndex, 0, exclamationTableItem);
        }
        else
        {
            tableWidget->setItem(rowIndex, 0, new QTableWidgetItem(""));
        }

        // check if the current item contains the find text
        if (QString(motionEntry->GetId().c_str()).contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive))
        {
            tableWidget->showRow(rowIndex);
        }
        else
        {
            tableWidget->hideRow(rowIndex);
        }

        // enable the sorting
        tableWidget->setSortingEnabled(true);

        return true;
    }


    bool MotionSetWindow::InsertRow(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry, QTableWidget* tableWidget, bool readOnly)
    {
        if (!motionEntry)
        {
            return false;
        }

        // Add a new row to the table widget.
        const int newRowIndex = tableWidget->rowCount();
        tableWidget->insertRow(newRowIndex);

        return FillRow(motionSet, motionEntry, newRowIndex, tableWidget, readOnly);
    }


    bool MotionSetWindow::RemoveRow([[maybe_unused]] EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry, QTableWidget* tableWidget)
    {
        if (!motionEntry)
        {
            return false;
        }

        const int rowCount = tableWidget->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            QTableWidgetItem* item = tableWidget->item(i, 1);
            if (item->text() == motionEntry->GetId().c_str())
            {
                tableWidget->removeRow(i);
                break;
            }
        }

        return true;
    }


    void MotionSetWindow::UpdateMotionSetTable(QTableWidget* tableWidget, EMotionFX::MotionSet* motionSet, bool readOnly)
    {
        // Get the previously selected items.
        const QList<QTableWidgetItem*> selectedItems = tableWidget->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // Store the previously selected motion ids.
        AZStd::vector<AZStd::string> selectedMotionIds;
        selectedMotionIds.reserve(numSelectedItems);

        AZStd::string idString;
        for (int i = 0; i < numSelectedItems; ++i)
        {
            const int rowIndex = selectedItems[i]->row();
            QTableWidgetItem* motionIdItem = tableWidget->item(rowIndex, 1);

            idString = motionIdItem->text().toUtf8().data();
            if (AZStd::find(selectedMotionIds.begin(), selectedMotionIds.end(), idString) == selectedMotionIds.end())
            {
                selectedMotionIds.push_back(idString);
            }
        }

        // Now that we remembered the selected motion entries, clear selection.
        tableWidget->clearSelection();


        // Is the given motion set valid?
        if (!motionSet)
        {
            tableWidget->setRowCount(0);
            tableWidget->horizontalHeader()->setVisible(false);
            return;
        }

        // Set the horizontal header visible, it's needed because the header can be hidden.
        tableWidget->horizontalHeader()->setVisible(true);

        // Pre-load motions to get infos
        motionSet->Preload();

        // Get the number of motion entries and adjust row count accordingly.
        const int numMotionEntries = static_cast<int>(motionSet->GetNumMotionEntries());
        tableWidget->setRowCount(numMotionEntries);

        // Disable the sorting to avoid row index issue.
        tableWidget->setSortingEnabled(false);

        // Add table widget items for all motion entries.
        AZStd::string motionFileName;
        AZStd::string tempString;
        int row = 0;
        const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
        for (const auto& motionEntryPair : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = motionEntryPair.second;

            // Was the motion entry selected before?
            const bool isSelected = AZStd::find(selectedMotionIds.begin(), selectedMotionIds.end(), motionEntry->GetId().c_str()) != selectedMotionIds.end();

            // Create the table widget item.
            QTableWidgetItem* idTableItem = new QTableWidgetItem(motionEntry->GetId().c_str());
            tableWidget->setItem(row, 1, idTableItem);

            if (readOnly)
            {
                idTableItem->setFlags(Qt::NoItemFlags);
            }

            const EMotionFX::Motion* motion = motionEntry->GetMotion();

            // Create the motion length entry.
            if (motion)
            {
                tempString = AZStd::string::format("%.2f sec", motion->GetDuration());
            }
            else
            {
                tempString = "";
            }
            QTableWidgetItem* lengthTableItem = new QTableWidgetItem(tempString.c_str());
            tableWidget->setItem(row, 2, lengthTableItem);
            if (readOnly)
            {
                lengthTableItem->setFlags(Qt::NoItemFlags);
            }

            if (motion)
            {
                // create the sub entry
                AZStd::to_string(tempString, motion->GetMotionData()->GetNumJoints());
                QTableWidgetItem* subTableItem = new QTableWidgetItem(tempString.c_str());
                tableWidget->setItem(row, 3, subTableItem);
                if (readOnly)
                {
                    subTableItem->setFlags(Qt::NoItemFlags);
                }

                // create the msub entry
                AZStd::to_string(tempString, motion->GetMotionData()->GetNumMorphs());
                QTableWidgetItem* msubTableItem = new QTableWidgetItem(tempString.c_str());
                tableWidget->setItem(row, 4, msubTableItem);
                if (readOnly)
                {
                    msubTableItem->setFlags(Qt::NoItemFlags);
                }
            }
            else
            {
                // Create the sub entry.
                QTableWidgetItem* subTableItem = new QTableWidgetItem("");
                tableWidget->setItem(row, 3, subTableItem);
                if (readOnly)
                {
                    subTableItem->setFlags(Qt::NoItemFlags);
                }

                // Create the msub entry.
                QTableWidgetItem* msubTableItem = new QTableWidgetItem("");
                tableWidget->setItem(row, 4, msubTableItem);
                if (readOnly)
                {
                    msubTableItem->setFlags(Qt::NoItemFlags);
                }
            }

            // Create the type entry.
            if (motion)
            {
                QTableWidgetItem* typeTableItem = new QTableWidgetItem(motion->GetMotionData()->RTTI_GetTypeName());
                tableWidget->setItem(row, 5, typeTableItem);
                if (readOnly)
                {
                    typeTableItem->setFlags(Qt::NoItemFlags);
                }
            }
            else
            {
                QTableWidgetItem* typeTableItem = new QTableWidgetItem("");
                tableWidget->setItem(row, 5, typeTableItem);
                if (readOnly)
                {
                    typeTableItem->setFlags(Qt::NoItemFlags);
                }
            }

            // Create the filename entry.
            QTableWidgetItem* filenameTableItem = new QTableWidgetItem(motionEntry->GetFilename());
            tableWidget->setItem(row, 6, filenameTableItem);
            if (readOnly)
            {
                filenameTableItem->setFlags(Qt::NoItemFlags);
            }

            // Show the exclamation mark in case the motion file cannot be found.
            motionFileName = motionSet->ConstructMotionFilename(motionEntry);
            if (!motionFileName.empty() && !AZ::IO::FileIOBase::GetInstance()->Exists(motionFileName.c_str()))
            {
                const AZStd::string tooltipText = "The motion file cannot be located. Please check if the file exists on disk.";

                QTableWidgetItem* exclamationTableItem = new QTableWidgetItem("");
                exclamationTableItem->setFlags(Qt::NoItemFlags);
                exclamationTableItem->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/ExclamationMark.svg"));
                exclamationTableItem->setToolTip(tooltipText.c_str());
                tableWidget->setItem(row, 0, exclamationTableItem);
            }
            else
            {
                tableWidget->setItem(row, 0, new QTableWidgetItem(""));
            }

            // Select the row in case the motion entry is selected
            if (!readOnly && isSelected)
            {
                tableWidget->selectRow(row);
            }

            // check if the current item contains the find text
            if (QString(motionEntry->GetId().c_str()).contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive))
            {
                tableWidget->showRow(row);
            }
            else
            {
                tableWidget->hideRow(row);
            }

            // Set all row items italic in case the motion is dirty.
            if (motion && motion->GetDirtyFlag())
            {
                SetRowItalic(row, true);
            }

            row++;
        }

        // enable the sorting
        tableWidget->setSortingEnabled(true);
    }

    void MotionSetWindow::SyncMotionDirtyFlag(int motionId)
    {
        EMotionFX::MotionSet::MotionEntry* motionEntry = FindMotionEntryByMotionId(motionId);
        if (motionEntry)
        {
            QTableWidgetItem* item = FindTableWidgetItemByEntry(motionEntry);
            if (item)
            {
                const EMotionFX::Motion* motion = motionEntry->GetMotion();
                if (motion)
                {
                    SetRowItalic(item->row(), motion->GetDirtyFlag());
                }
            }
        }
    }

    void MotionSetWindow::SetRowItalic(int row, bool italic)
    {
        QTableWidgetItem* defaultItem = m_tableWidget->item(row, 0);
        if (!defaultItem)
        {
            return;
        }

        QFont italicFont = defaultItem->font();
        italicFont.setItalic(italic);

        const int columnCount = m_tableWidget->columnCount();
        for (int i = 0; i < columnCount; ++i)
        {
            QTableWidgetItem* item = m_tableWidget->item(row, i);
            item->setFont(italicFont);
        }
    }

    void MotionSetWindow::UpdateInterface()
    {
        EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();

        const bool isEnabled = (motionSet != nullptr);
        m_tableWidget->setEnabled(isEnabled);
        m_addAction->setEnabled(isEnabled);
        m_loadAction->setEnabled(isEnabled);
        m_editAction->setEnabled(isEnabled);

        const bool isToolbarEnabled = (isEnabled && !selectedItems.empty());
        m_saveAction->setEnabled(isToolbarEnabled);

        if (!motionSet)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Select motion");
        commandGroup.AddCommandString("Unselect -motionIndex SELECT_ALL");

        // Inform the time view plugin about the motion selection change.
        for (QTableWidgetItem* selectedItem : selectedItems)
        {
            EMotionFX::MotionSet::MotionEntry* motionEntry = FindMotionEntry(selectedItem);
            if (motionEntry)
            {
                EMotionFX::Motion* motion = motionEntry->GetMotion();
                if (motion)
                {
                    const size_t motionIndex = EMotionFX::GetMotionManager().FindMotionIndexByFileName(motion->GetFileName());
                    commandGroup.AddCommandString(AZStd::string::format("Select -motionIndex %zu", motionIndex));
                }
            }
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result, false))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        emit MotionSelectionChanged();
    }


    void MotionSetWindow::OnAddNewEntry()
    {
        EMotionFX::MotionSet* selectedSet = m_plugin->GetSelectedSet();
        if (!selectedSet)
        {
            return;
        }

        // Build a list of unique string id values from all motion set entries.
        AZStd::vector<AZStd::string> idStrings;
        selectedSet->BuildIdStringList(idStrings);

        // Construct, fill and execute the command group.
        MCore::CommandGroup commandGroup("Add new motion set entry");
        CommandSystem::AddMotionSetEntry(selectedSet->GetID(), "<undefined>", idStrings, "", &commandGroup);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionSetWindow::OnLoadEntries()
    {
        const AZStd::vector<AZStd::string> filenames = GetMainWindow()->GetFileManager()->LoadMotionsFileDialog(this);
        GetMainWindow()->activateWindow();
        if (filenames.empty())
        {
            return;
        }

        AddMotions(filenames);
    }


    void MotionSetWindow::AddMotions(const AZStd::vector<AZStd::string>& filenames)
    {
        EMotionFX::MotionSet* selectedSet = m_plugin->GetSelectedSet();
        if (!selectedSet)
        {
            return;
        }

        CommandSystem::LoadMotionsCommand(filenames);
        const size_t numFileNames = filenames.size();

        // Build a list of unique string id values from all motion set entries.
        AZStd::vector<AZStd::string> idStrings;
        idStrings.reserve(selectedSet->GetNumMotionEntries() + numFileNames);
        selectedSet->BuildIdStringList(idStrings);

        AZStd::string parameterString;

        // iterate over all motions and add them
        AZStd::string idString;
        AZStd::string motionName;
        bool isAbsoluteMotion = false;
        for (const AZStd::string& filename : filenames)
        {
            // remove the media root folder from the absolute motion filename so that we get the relative one to the media root folder
            motionName = filename.c_str();
            EMotionFX::GetEMotionFX().GetFilenameRelativeToMediaRoot(&motionName);

            if (EMotionFX::MotionSet::MotionEntry::CheckIfIsAbsoluteFilename(motionName.c_str()))
            {
                isAbsoluteMotion = true;
            }

            idString = CommandSystem::GenerateMotionId(motionName.c_str(), "", idStrings);

            parameterString += motionName;
            parameterString += ";";
            parameterString += idString;
            parameterString += ";";

            // add the id we gave to this motion to the id string list so that the other new motions can't get that one
            idStrings.emplace_back(idString);
        }

        if (!parameterString.empty())
        {
            parameterString.pop_back(); // remove the last ";"

            AZStd::string command = AZStd::string::format("MotionSetAddMotion -motionSetID %i -motionFilenamesAndIds \"", selectedSet->GetID());
            command += parameterString;
            command += "\"";

            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }

            if (isAbsoluteMotion)
            {
                AZStd::string text = AZStd::string::format("Some of the motions are located outside of the asset folder of your project:\n\n%s\n\nThis means that the motion set cannot store relative filenames and will hold absolute filenames.", EMotionFX::GetEMotionFX().GetMediaRootFolder());
                QMessageBox::warning(this, "Warning", text.c_str());
            }
        }
    }


    // when dropping stuff in our window
    void MotionSetWindow::dropEvent(QDropEvent* event)
    {
        // dont accept dragging/drop from and to yourself
        if (event->source() == this || event->source() == m_tableWidget)
        {
            return;
        }

        // check if we dropped any files to the application
        const QMimeData* mimeData = event->mimeData();
        if (mimeData->hasUrls())
        {
            // read out the dropped file names
            QList<QUrl> urls = mimeData->urls();
            const int numUrls = urls.count();

            AZStd::vector<AZStd::string> filenames;
            filenames.reserve(numUrls);

            // Get the number of dropped urls and iterate through them.
            AZStd::string filename;
            AZStd::string extension;
            for (int i = 0; i < numUrls; ++i)
            {
                // get the complete file name and extract the extension
                //fileName  = urls[i].toLocalFile().toAscii().data();
                filename = urls[i].toLocalFile().toUtf8().data();
                AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);

                // check if we are dealing with a valid motion file
                if (extension == "motion")
                {
                    filenames.push_back(filename.c_str());
                }
            }

            if (!filenames.empty())
            {
                AddMotions(filenames);
                event->acceptProposedAction();
                event->accept();
                update();
                return;
            }
        }

        // if we have text, get it
        const AZStd::string dropText = event->mimeData()->text().toUtf8().data();
        MCore::CommandLine commandLine(dropText.c_str());
        AZStd::vector<AZStd::string> filenames;

        // check if the drag & drop is coming from an external window
        if (commandLine.CheckIfHasParameter("window"))
        {
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(dropText.c_str(), tokens, "\n", false, true);

            const size_t numTokens = tokens.size();
            for (size_t i = 0; i < numTokens; ++i)
            {
                MCore::CommandLine currentCommandLine(tokens[i].c_str());

                // get the name of the window where the drag came from
                AZStd::string dragWindow;
                currentCommandLine.GetValue("window", "", &dragWindow);

                // drag&drop coming from the motion window from the standard plugins
                if (dragWindow == "MotionWindow")
                {
                    // get the motion id and the corresponding motion object
                    const uint32 motionID = currentCommandLine.GetValueAsInt("motionID", MCORE_INVALIDINDEX32);
                    EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);

                    if (motion)
                    {
                        filenames.push_back(motion->GetFileName());
                    }
                }
            }

            AddMotions(filenames);
            event->acceptProposedAction();
        }

        event->accept();
        update();
    }


    // drag enter
    void MotionSetWindow::dragEnterEvent(QDragEnterEvent* event)
    {
        // this is needed to actually reach the drop event function
        event->acceptProposedAction();
    }


    void MotionSetWindow::OnRemoveMotions()
    {
        EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();
        if (!motionSet)
        {
            return;
        }

        // Get the selected items and return in case nothing is selected.
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
        const int numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return;
        }

        // Get the row indices from the selected items.
        AZStd::vector<int> rowIndices;
        GetRowIndices(selectedItems, rowIndices);

        // Create the failed remove motions array.
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;
        failedRemoveMotions.reserve(rowIndices.size());

        AZStd::vector<EMotionFX::MotionSet::MotionEntry*> motionEntriesToRemove;
        motionEntriesToRemove.reserve(rowIndices.size());

        // Iterate over all motions and add them.
        AZStd::string motionIdsToRemoveString;
        for (const int rowIndex : rowIndices)
        {
            QTableWidgetItem* idItem = m_tableWidget->item(rowIndex, 1);
            EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryById(idItem->text().toUtf8().data());

            // Check if the motion exists in multiple motion sets.
            const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            size_t numMotionSetContainsMotion = 0;

            for (size_t motionSetId = 0; motionSetId < numMotionSets; motionSetId++)
            {
                EMotionFX::MotionSet* motionSet2 = EMotionFX::GetMotionManager().GetMotionSet(motionSetId);

                if (motionSet2->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (motionSet2->FindMotionEntryById(motionEntry->GetId()))
                {
                    numMotionSetContainsMotion++;
                    if (numMotionSetContainsMotion > 1)
                    {
                        break;
                    }
                }
            }

            // check the reference counter if only one reference registered
            // two is needed because the remove motion command has to be called to have the undo/redo possible
            // without it the motion list is also not updated because the remove motion callback is not called
            // also avoid the issue to remove from set but not the motion list, to keep it in the motion list
            if (motionEntry->GetMotion() && motionEntry->GetMotion()->GetReferenceCount() == 1)
            {
                motionEntry->GetMotion()->IncreaseReferenceCount();
            }

            // Add the motion set remove motion command.
            if (!motionIdsToRemoveString.empty())
            {
                motionIdsToRemoveString += ';';
            }
            motionIdsToRemoveString += motionEntry->GetId();

            // If motion exists in multiple motion sets, then it should not be removed from motions window.
            if (numMotionSetContainsMotion > 1)
            {
                continue;
            }

            // Check if the motion is not valid, that means the motion is not loaded.
            if (motionEntry->GetMotion())
            {
                // Calculcate how many motion sets except than the provided one use the given motion.
                size_t numExternalUses = CalcNumMotionEntriesUsingMotionExcluding(motionEntry->GetFilename(), motionSet);

                // Remove the motion in case it was only used by the given motion set.
                if (numExternalUses == 0)
                {
                    motionEntriesToRemove.push_back(motionEntry);
                }
                else if (numExternalUses > 0)
                {
                    failedRemoveMotions.push_back(motionEntry->GetMotion());
                }
            }
        }

        // Find the lowest row selected.
        int lowestRowSelected = -1;
        for (int selectedRowIndex : rowIndices)
        {
            if (selectedRowIndex < lowestRowSelected)
            {
                lowestRowSelected = selectedRowIndex;
            }
        }

        MCore::CommandGroup commandGroup("Motion set remove motions");
        AZStd::string commandString;

        // 1. Remove motion entries from the motion set.
        commandString = AZStd::string::format("MotionSetRemoveMotion -motionSetID %i -motionIds \"", motionSet->GetID());
        commandString += motionIdsToRemoveString;
        commandString += '\"';
        commandGroup.AddCommandString(commandString);

        // 2. Then get rid of the actual motions itself.
        for (EMotionFX::MotionSet::MotionEntry* motionEntry : motionEntriesToRemove)
        {
            // In case we modified the motion, ask if the user wants to save changes it before removing it.
            const AZStd::string motionFilename = motionSet->ConstructMotionFilename(motionEntry);
            SaveDirtyMotionFilesCallback::SaveDirtyMotion(motionEntry->GetMotion(), /*commandGroup=*/nullptr, /*askBeforeSaving=*/true, /*showCancelButton=*/false);

            commandString = AZStd::string::format("RemoveMotion -filename \"%s\"", motionFilename.c_str());
            commandGroup.AddCommandString(commandString);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // selected the next row
        if (lowestRowSelected > (m_tableWidget->rowCount() - 1))
        {
            m_tableWidget->selectRow(lowestRowSelected - 1);
        }
        else
        {
            m_tableWidget->selectRow(lowestRowSelected);
        }

        // show the window if at least one failed remove motion
        if (!failedRemoveMotions.empty())
        {
            MotionSetRemoveMotionsFailedWindow removeMotionsFailedWindow(this, failedRemoveMotions);
            removeMotionsFailedWindow.exec();
        }
    }


    void MotionSetWindow::RenameEntry(QTableWidgetItem* item)
    {
        // Find the motion entry by the table widget item.
        EMotionFX::MotionSet::MotionEntry* motionEntry = FindMotionEntry(item);

        // Show the entry renaming window.
        RenameMotionEntryWindow window(this, m_plugin->GetSelectedSet(), motionEntry->GetId().c_str());
        window.exec();
    }


    void MotionSetWindow::OnRenameEntry()
    {
        // Get the selected items and check if there is at least one item selected.
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
        if (selectedItems.empty())
        {
            return;
        }

        RenameEntry(selectedItems[0]);
    }


    void MotionSetWindow::OnUnassignMotions()
    {
        EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();
        if (!motionSet)
        {
            return;
        }

        // Get the selected items and check if there is at least one item selected.
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
        if (selectedItems.empty())
        {
            return;
        }

        // Construct the command group.
        MCore::CommandGroup commandGroup("Unassign motions");

        // Iterate through all selected items
        AZStd::string commandString;
        for (QTableWidgetItem* item : selectedItems)
        {
            // Find the motion entry by the table widget item.
            EMotionFX::MotionSet::MotionEntry* motionEntry = FindMotionEntry(item);

            commandString = AZStd::string::format("MotionSetAdjustMotion -motionSetID %i -idString \"%s\" -motionFileName \"\"",
                    motionSet->GetID(),
                    motionEntry->GetId().c_str());

            commandGroup.AddCommandString(commandString);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionSetWindow::OnCopyMotionID()
    {
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
        QTableWidgetItem* item = m_tableWidget->item(selectedItems[0]->row(), 1);
        QApplication::clipboard()->setText(item->text());
    }

    void MotionSetWindow::OnClearMotions()
    {
        EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();
        if (!motionSet)
        {
            return;
        }
        const size_t numMotionEntries = motionSet->GetNumMotionEntries();

        // create the command group
        MCore::CommandGroup commandGroup("Motion set clear motions");

        // Increase the reference counter if needed for each motion.
        const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            // Check the reference counter if only one reference registered.
            // Two is needed because the remove motion command has to be called to have the undo/redo possible
            // without it the motion list is also not updated because the remove motion callback is not called.
            // Also avoid the issue to remove from set but not the motion list, to keep it in the motion list.
            if (motionEntry->GetMotion() && motionEntry->GetMotion()->GetReferenceCount() == 1)
            {
                motionEntry->GetMotion()->IncreaseReferenceCount();
            }
        }

        // add the remove commands
        CommandSystem::ClearMotionSetMotions(motionSet, &commandGroup);

        // create the failed remove motions array
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;
        failedRemoveMotions.reserve(numMotionEntries);

        // Remove motions.
        {
            AZStd::string motionFileName;
            AZStd::string tempString;
            for (const auto& item : motionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

                // check if the motion is not valid, that means the motion is not loaded
                if (!motionEntry->GetMotion())
                {
                    continue;
                }

                // Calculcate how many motion sets except than the provided one use the given motion.
                size_t numExternalUses = CalcNumMotionEntriesUsingMotionExcluding(motionEntry->GetFilename(), motionSet);

                // Remove the motion in case it was only used by the given motion set.
                if (numExternalUses == 0)
                {
                    motionFileName = motionSet->ConstructMotionFilename(motionEntry);
                    tempString = AZStd::string::format("RemoveMotion -filename \"%s\"", motionFileName.c_str());
                    commandGroup.AddCommandString(tempString);
                }
                else if (numExternalUses > 0)
                {
                    failedRemoveMotions.push_back(motionEntry->GetMotion());
                }
            }
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // show the window if at least one failed remove motion
        if (!failedRemoveMotions.empty())
        {
            MotionSetRemoveMotionsFailedWindow removeMotionsFailedWindow(this, failedRemoveMotions);
            removeMotionsFailedWindow.exec();
        }
    }


    void MotionSetWindow::OnEditButton()
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();

        // Get the row indices from the selected items.
        AZStd::vector<int> rowIndices;
        GetRowIndices(selectedItems, rowIndices);

        // get the selected motion set
        EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();

        // generate the motions IDs array
        AZStd::vector<AZStd::string> motionIDs;
        if (!rowIndices.empty())
        {
            for (const int rowIndex : rowIndices)
            {
                QTableWidgetItem* item = m_tableWidget->item(rowIndex, 1);
                motionIDs.push_back(item->text().toUtf8().data());
            }
        }
        else
        {
            const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
            for (const auto& item : motionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;
                motionIDs.push_back(motionEntry->GetId().c_str());
            }
        }

        // show the batch edit window
        MotionEditStringIDWindow motionEditStringIDWindow(this, motionSet, motionIDs);
        motionEditStringIDWindow.exec();
    }

    void MotionSetWindow::OnSave()
    {
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const size_t numMotions = selectionList.GetNumSelectedMotions();
        if (numMotions == 0)
        {
            return;
        }

        // Collect motion ids of the motion to be saved.
        AZStd::vector<AZ::u32> motionIds;
        motionIds.reserve(numMotions);
        for (size_t i = 0; i < numMotions; ++i)
        {
            const EMotionFX::Motion* motion = selectionList.GetMotion(i);
            motionIds.push_back(motion->GetID());
        }

        // Save all selected motions.
        for (const AZ::u32 motionId : motionIds)
        {
            const EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionId);
            AZ_Assert(motion, "Expected to find the motion pointer for motion with id %d.", motionId);
            if (motion->GetDirtyFlag())
            {
                GetMainWindow()->GetFileManager()->SaveMotion(motionId);
            }
        }
    }


    void MotionSetWindow::OnEntryDoubleClicked(QTableWidgetItem* item)
    {
        const EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();
        if (!motionSet)
        {
            return;
        }

        // Decide what we going to do based on the double-clicked column.
        const int32 itemColumn = m_tableWidget->column(item);
        if (itemColumn == 0)
        {
            // User clicked on the exclamation mark, return directly and do nothing.
            return;
        }
        if (itemColumn == 1)
        {
            // User clicked on the motion id play it
            EMotionFX::MotionSet::MotionEntry* motionEntry = FindMotionEntry(item);
            EMotionFX::Motion* motion = motionEntry->GetMotion();
            if (motion)
            {
                PlayMotion(motionEntry->GetMotion());
                return;
            }
            else
            {
                // If the motion path is invalid, let user pick another motion.
                if (QMessageBox::question(this, "Invalid motion", "Motion has invalid path. Do you want to select a different motion?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
                {
                    return;
                }
            }
        }

        // Select the new motion for the entry
        AZStd::string motionFilename = GetMainWindow()->GetFileManager()->LoadMotionFileDialog(this);
        if (motionFilename.empty())
        {
            return;
        }

        // Pre-load the motion in case the selected motion is valid.
        //if (!motionFilename.empty())
        {
            AZStd::vector<AZStd::string> onlyOneMotionFileNames;
            onlyOneMotionFileNames.push_back(motionFilename);
            CommandSystem::LoadMotionsCommand(onlyOneMotionFileNames);

            // Remove the media root folder from the absolute motion filename so that we get the relative one to the media root folder.
            EMotionFX::GetEMotionFX().GetFilenameRelativeToMediaRoot(&motionFilename);
        }

        // Find the motion entry by the table widget item.
        const EMotionFX::MotionSet::MotionEntry* motionEntry = FindMotionEntry(item);

        // Construct the command and execute it.
        const AZStd::string command = AZStd::string::format("MotionSetAdjustMotion -motionSetID %i -idString \"%s\" -motionFileName \"%s\"",
                motionSet->GetID(),
                motionEntry->GetId().c_str(),
                motionFilename.c_str());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionSetWindow::OnTextFilterChanged(const QString& text)
    {
        FromQtString(text, &m_searchWidgetText);
        ReInit();
    }


    void MotionSetWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            OnRemoveMotions();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void MotionSetWindow::keyReleaseEvent(QKeyEvent* event)
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


    void MotionSetWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();

        // get the number of selected items
        if (selectedItems.empty())
        {
            return;
        }

        // Get the row indices from the selected items.
        AZStd::vector<int> rowIndices;
        GetRowIndices(selectedItems, rowIndices);

        // create the menu
        QMenu menu(this);

        // add the rename action if only one selected
        if (rowIndices.size() == 1)
        {
            // add the rename selected motion action
            QAction* renameSelectedMotionAction = menu.addAction("Rename Motion ID");
            connect(renameSelectedMotionAction, &QAction::triggered, this, &MotionSetWindow::OnRenameEntry);

            // Unassign the linked motion.
            QAction* unassignMotionAction = menu.addAction("Unassign Motion");
            connect(unassignMotionAction, &QAction::triggered, this, &MotionSetWindow::OnUnassignMotions);

            // add the copy selected motion ID action
            QAction* copySelectedMotionIDAction = menu.addAction("Copy Selected Motion ID");
            connect(copySelectedMotionIDAction, &QAction::triggered, this, &MotionSetWindow::OnCopyMotionID);

            QAction* browserAction = menu.addAction(AzQtComponents::fileBrowserActionName());
            connect(browserAction, &QAction::triggered, this, []()
                {
                    const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
                    for (size_t i = 0; i < selection.GetNumSelectedMotions(); ++i)
                    {
                        EMotionFX::Motion* motion = selection.GetMotion(i);

                        // The browser action should point to the source file's folder.
                        AZStd::string fileName = motion->GetFileName();
                        GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(fileName);

                        AzQtComponents::ShowFileOnDesktop(fileName.c_str());
                    }
                });
        }
        else if (rowIndices.size() > 1)
        {
            // Unassign linked motions for the selected entries.
            QAction* unassignMotionAction = menu.addAction("Unassign Motions");
            connect(unassignMotionAction, &QAction::triggered, this, &MotionSetWindow::OnUnassignMotions);
        }

        QAction* saveMotionsAction = menu.addAction("Save Selected Motions");
        saveMotionsAction->setObjectName("EMFX.MotionSetTableWidget.SaveSelectedMotionsAction");
        connect(saveMotionsAction, &QAction::triggered, this, &MotionSetWindow::OnSave);

        menu.addSeparator();

        QAction* removeSelectedMotionsAction = menu.addAction("Remove Selected Motions");
        removeSelectedMotionsAction->setObjectName("EMFX.MotionSetTableWidget.RemoveSelectedMotionsAction");
        connect(removeSelectedMotionsAction, &QAction::triggered, this, &MotionSetWindow::OnRemoveMotions);

        // execute the menu
        menu.exec(event->globalPos());
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    MotionSetTableWidget::MotionSetTableWidget(MotionSetsWindowPlugin* parentPlugin, QWidget* parent)
        : QTableWidget(parent)
    {
        // keep the parent plugin
        m_plugin = parentPlugin;

        // enable drop only
        setAcceptDrops(true);
        setDragEnabled(false);
        setDragDropMode(QAbstractItemView::DropOnly);
    }


    // destructor
    MotionSetTableWidget::~MotionSetTableWidget()
    {
    }


    void MotionSetTableWidget::dropEvent(QDropEvent* event)
    {
        m_plugin->GetMotionSetWindow()->dropEvent(event);
    }


    void MotionSetTableWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        event->acceptProposedAction();
    }


    void MotionSetTableWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        event->accept();
    }


    // return the mime data
    QMimeData* MotionSetTableWidget::mimeData(const QList<QTableWidgetItem*> items) const
    {
        EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();
        if (motionSet == nullptr)
        {
            return nullptr;
        }

        if (items.count() != 3 && items.count() != 2)
        {
            return nullptr;
        }

        MCORE_ASSERT(1 == 0); // reimplement this function

        AZStd::string textData;

        // create the data, set the text and return it
        QMimeData* mimeData = new QMimeData();
        mimeData->setText(textData.c_str());
        return mimeData;
    }


    // return the supported mime types
    QStringList MotionSetTableWidget::mimeTypes() const
    {
        QStringList result;
        result.append("text/plain");
        return result;
    }


    // get the allowed drop actions
    Qt::DropActions MotionSetTableWidget::supportedDropActions() const
    {
        return Qt::CopyAction;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    MotionEditStringIDWindow::MotionEditStringIDWindow(QWidget* parent, EMotionFX::MotionSet* motionSet, const AZStd::vector<AZStd::string>& motionIDs)
        : QDialog(parent)
    {
        // save the motion set and the motion IDs
        m_motionSet = motionSet;
        m_motionIDs = motionIDs;

        // Reserve space.
        m_valids.reserve(m_motionIDs.size());
        m_motionToModifiedMap.reserve(m_motionIDs.size());
        m_modifiedMotionIDs.reserve(motionSet->GetNumMotionEntries());

        // set the window title
        setWindowTitle("Batch Edit Motion IDs");

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // create the spacer
        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        // create the combobox
        m_comboBox = new QComboBox();
        m_comboBox->addItem("Replace All");
        m_comboBox->addItem("Replace First");
        m_comboBox->addItem("Replace Last");

        // connect the combobox
        connect(m_comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MotionEditStringIDWindow::CurrentIndexChanged);

        // create the string line edits
        m_stringALineEdit = new QLineEdit();
        m_stringBLineEdit = new QLineEdit();

        // connect the line edit
        connect(m_stringALineEdit, &QLineEdit::textChanged, this, &MotionEditStringIDWindow::StringABChanged);
        connect(m_stringBLineEdit, &QLineEdit::textChanged, this, &MotionEditStringIDWindow::StringABChanged);

        // add the operation layout
        QHBoxLayout* operationLayout = new QHBoxLayout();
        operationLayout->addWidget(new QLabel("Operation:"));
        operationLayout->addWidget(m_comboBox);
        operationLayout->addWidget(spacerWidget);
        operationLayout->addWidget(new QLabel("StringA:"));
        operationLayout->addWidget(m_stringALineEdit);
        operationLayout->addWidget(new QLabel("StringB:"));
        operationLayout->addWidget(m_stringBLineEdit);
        layout->addLayout(operationLayout);

        // create the table widget
        m_tableWidget = new QTableWidget();
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setGridStyle(Qt::SolidLine);
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // set the table widget columns
        m_tableWidget->setColumnCount(2);
        QStringList headerLabels;
        headerLabels.append("Before");
        headerLabels.append("After");
        m_tableWidget->setHorizontalHeaderLabels(headerLabels);
        m_tableWidget->horizontalHeader()->setStretchLastSection(true);
        m_tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        m_tableWidget->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);

        // Set the row count
        const size_t numMotionIDs = m_motionIDs.size();
        m_tableWidget->setRowCount(static_cast<int>(numMotionIDs));

        // disable the sorting
        m_tableWidget->setSortingEnabled(false);

        // initialize the table
        for (size_t i = 0; i < numMotionIDs; ++i)
        {
            // create the before and after table widget items
            QTableWidgetItem* beforeTableWidgetItem = new QTableWidgetItem(m_motionIDs[i].c_str());
            QTableWidgetItem* afterTableWidgetItem = new QTableWidgetItem(m_motionIDs[i].c_str());

            // set the text of the row
            const int row = static_cast<int>(i);
            m_tableWidget->setItem(row, 0, beforeTableWidgetItem);
            m_tableWidget->setItem(row, 1, afterTableWidgetItem);
        }

        m_tableWidget->setSortingEnabled(true);
        m_tableWidget->resizeColumnToContents(0);
        m_tableWidget->setCornerButtonEnabled(false);

        layout->addWidget(m_tableWidget);

        // create the num motion IDs label
        // this label never change, it's the total of motion ID in the table
        m_numMotionIDsLabel = new QLabel();
        m_numMotionIDsLabel->setAlignment(Qt::AlignLeft);
        m_numMotionIDsLabel->setText(QString("Number of motion IDs: %1").arg(numMotionIDs));

        // create the num modified IDs label
        m_numModifiedIDsLabel = new QLabel();
        m_numModifiedIDsLabel->setAlignment(Qt::AlignCenter);
        m_numModifiedIDsLabel->setText("Number of modified IDs: 0");

        // create the num duplicate IDs label
        m_numDuplicateIDsLabel = new QLabel();
        m_numDuplicateIDsLabel->setAlignment(Qt::AlignRight);
        m_numDuplicateIDsLabel->setText("Number of duplicate IDs: 0");

        // add the stats layout
        QHBoxLayout* statsLayout = new QHBoxLayout();
        statsLayout->addWidget(m_numMotionIDsLabel);
        statsLayout->addWidget(m_numModifiedIDsLabel);
        statsLayout->addWidget(m_numDuplicateIDsLabel);
        layout->addLayout(statsLayout);

        // add the bottom buttons
        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        m_applyButton                = new QPushButton("Apply");
        QPushButton* closeButton    = new QPushButton("Close");
        buttonLayout->addWidget(m_applyButton);
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        // apply button is disabled because nothing is changed
        m_applyButton->setEnabled(false);

        // connect the buttons
        connect(m_applyButton, &QPushButton::clicked, this, &MotionEditStringIDWindow::Accepted);
        connect(closeButton, &QPushButton::clicked, this, &MotionEditStringIDWindow::reject);

        setLayout(layout);

        setMinimumSize(480, 720);
    }


    void MotionEditStringIDWindow::Accepted()
    {
        // create the command group
        MCore::CommandGroup group("Motion set edit IDs");

        // add each command
        AZStd::string commandString;
        for (size_t validID : m_valids)
        {
            // get the motion ID and the modified ID
            AZStd::string& motionID = m_motionIDs[validID];
            const AZStd::string& modifiedID = m_modifiedMotionIDs[m_motionToModifiedMap[validID]];

            commandString = AZStd::string::format("MotionSetAdjustMotion -motionSetID %i -idString \"%s\" -newIDString \"%s\" -updateMotionNodeStringIDs true", m_motionSet->GetID(), motionID.c_str(), modifiedID.c_str());
            motionID = modifiedID;

            // add the command in the group
            group.AddCommandString(commandString);
        }

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(group, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // block signals for the reset
        m_stringALineEdit->blockSignals(true);
        m_stringBLineEdit->blockSignals(true);

        // reset the string line edits
        m_stringALineEdit->setText("");
        m_stringBLineEdit->setText("");

        // enable signals after the reset
        m_stringALineEdit->blockSignals(false);
        m_stringBLineEdit->blockSignals(false);

        // disable the sorting
        m_tableWidget->setSortingEnabled(false);

        // set the new table using modified motion IDs
        const size_t numMotionIDs = m_motionIDs.size();
        for (size_t i = 0; i < numMotionIDs; ++i)
        {
            // create the before and after table widget items
            QTableWidgetItem* beforeTableWidgetItem = new QTableWidgetItem(m_motionIDs[i].c_str());
            QTableWidgetItem* afterTableWidgetItem = new QTableWidgetItem(m_motionIDs[i].c_str());

            // set the text of the row
            const int row = static_cast<int>(i);
            m_tableWidget->setItem(row, 0, beforeTableWidgetItem);
            m_tableWidget->setItem(row, 1, afterTableWidgetItem);
        }

        // enable the sorting
        m_tableWidget->setSortingEnabled(true);

        // resize before column
        m_tableWidget->resizeColumnToContents(0);

        // reset the stats
        m_numModifiedIDsLabel->setText("Number of modified IDs: 0");
        m_numDuplicateIDsLabel->setText("Number of duplicate IDs: 0");

        // apply button is disabled because nothing is changed
        m_applyButton->setEnabled(false);
    }


    void MotionEditStringIDWindow::StringABChanged(const QString& text)
    {
        MCORE_UNUSED(text);
        UpdateTableAndButton();
    }


    void MotionEditStringIDWindow::CurrentIndexChanged(int index)
    {
        MCORE_UNUSED(index);
        UpdateTableAndButton();
    }


    void MotionEditStringIDWindow::UpdateTableAndButton()
    {
        // get the number of motion IDs
        const size_t numMotionIDs = m_motionIDs.size();

        // Remember the selected motion IDs so we can restore selection after swapping the table items.
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
        const int numSelectedItems = selectedItems.size();
        QVector<QString> selectedMotionIds(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            selectedMotionIds[i] = selectedItems[i]->text();
        }

        // special case where the string A and B are empty, nothing is replaced
        if ((m_stringALineEdit->text().isEmpty()) && (m_stringBLineEdit->text().isEmpty()))
        {
            // disable the sorting
            m_tableWidget->setSortingEnabled(false);

            // reset the table
            for (size_t i = 0; i < numMotionIDs; ++i)
            {
                // create the before and after table widget items
                QTableWidgetItem* beforeTableWidgetItem = new QTableWidgetItem(m_motionIDs[i].c_str());
                QTableWidgetItem* afterTableWidgetItem = new QTableWidgetItem(m_motionIDs[i].c_str());

                // set the text of the row
                const int row = static_cast<int>(i);
                m_tableWidget->setItem(row, 0, beforeTableWidgetItem);
                m_tableWidget->setItem(row, 1, afterTableWidgetItem);
            }

            // enable the sorting
            m_tableWidget->setSortingEnabled(true);

            // reset the stats
            m_numModifiedIDsLabel->setText("Number of modified IDs: 0");
            m_numDuplicateIDsLabel->setText("Number of duplicate IDs: 0");

            // apply button is disabled because nothing is changed
            m_applyButton->setEnabled(false);

            // stop here
            return;
        }

        // Clear the arrays but keep the memory to avoid alloc.
        m_valids.clear();
        m_modifiedMotionIDs.clear();
        m_motionToModifiedMap.clear();

        // Copy all motion IDs from the motion set in the modified array.
        const EMotionFX::MotionSet::MotionEntries& motionEntries = m_motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            m_modifiedMotionIDs.push_back(motionEntry->GetId().c_str());
        }

        // Modify each ID using the operation in the modified array.
        AZStd::string newMotionID;
        AZStd::string tempString;
        for (const AZStd::string& motionID : m_motionIDs)
        {
            // 0=Replace All, 1=Replace First, 2=Replace Last
            const int operationMode = m_comboBox->currentIndex();

            // compute the new text
            switch (operationMode)
            {
                case 0:
                {
                    tempString = motionID.c_str();
                    AzFramework::StringFunc::Replace(tempString, m_stringALineEdit->text().toUtf8().data(), m_stringBLineEdit->text().toUtf8().data(), true /* case sensitive */);
                    newMotionID = tempString.c_str();
                    break;
                }

                case 1:
                {
                    tempString = motionID.c_str();
                    AzFramework::StringFunc::Replace(tempString, m_stringALineEdit->text().toUtf8().data(), m_stringBLineEdit->text().toUtf8().data(), true /* case sensitive */, true /* replace first */, false /* replace last */);
                    newMotionID = tempString.c_str();
                    break;
                }

                case 2:
                {
                    tempString = motionID.c_str();
                    AzFramework::StringFunc::Replace(tempString, m_stringALineEdit->text().toUtf8().data(), m_stringBLineEdit->text().toUtf8().data(), true /* case sensitive */, false /* replace first */, true /* replace last */);
                    newMotionID = tempString.c_str();
                    break;
                }
            }

            // change the value in the array and add the mapping motion to modified
            auto iterator = AZStd::find(m_modifiedMotionIDs.begin(), m_modifiedMotionIDs.end(), motionID);
            const size_t modifiedIndex = iterator - m_modifiedMotionIDs.begin();
            m_modifiedMotionIDs[modifiedIndex] = newMotionID;
            m_motionToModifiedMap.push_back(modifiedIndex);
        }

        // disable the sorting
        m_tableWidget->setSortingEnabled(false);

        // found flags
        size_t numDuplicateFound = 0;

        // update each row
        for (size_t i = 0; i < numMotionIDs; ++i)
        {
            // find the index in the motion set
            const AZStd::string& modifiedID = m_modifiedMotionIDs[m_motionToModifiedMap[i]];

            // create the before and after table widget items
            QTableWidgetItem* beforeTableWidgetItem = new QTableWidgetItem(m_motionIDs[i].c_str());
            QTableWidgetItem* afterTableWidgetItem = new QTableWidgetItem(modifiedID.c_str());

            // find duplicate
            size_t itemFoundCounter = 0;
            const size_t numMotionEntries = m_motionSet->GetNumMotionEntries();
            for (size_t k = 0; k < numMotionEntries; ++k)
            {
                if (m_modifiedMotionIDs[k] == modifiedID)
                {
                    ++itemFoundCounter;
                    if (itemFoundCounter > 1)
                    {
                        ++numDuplicateFound;
                        break;
                    }
                }
            }

            // set the row red if duplicate, green if the value is valid, nothing if the value is the same
            if (itemFoundCounter > 1)
            {
                beforeTableWidgetItem->setForeground(Qt::red);
                afterTableWidgetItem->setForeground(Qt::red);
            }
            else
            {
                if (modifiedID != m_motionIDs[i])
                {
                    // set the row green
                    beforeTableWidgetItem->setForeground(Qt::green);
                    afterTableWidgetItem->setForeground(Qt::green);

                    // add a valid
                    m_valids.push_back(i);
                }
            }

            // set the text of the row
            m_tableWidget->setItem(aznumeric_caster(i), 0, beforeTableWidgetItem);
            m_tableWidget->setItem(aznumeric_caster(i), 1, afterTableWidgetItem);
        }

        // enable the sorting
        m_tableWidget->setSortingEnabled(true);

        // update the num modified label
        m_numModifiedIDsLabel->setText(QString("Number of modified IDs: %1").arg(m_valids.size()));

        // update the num duplicate label
        // the number is in red if at least one found
        if (numDuplicateFound > 0)
        {
            m_numDuplicateIDsLabel->setText(QString("Number of duplicate IDs: <font color='red'>%1</font>").arg(numDuplicateFound));
        }
        else
        {
            m_numDuplicateIDsLabel->setText("Number of duplicate IDs: 0");
        }

        // enable or disable the apply button
        m_applyButton->setEnabled((!m_valids.empty()) && (numDuplicateFound == 0));

        // Reselect the remembered motions.
        m_tableWidget->clearSelection();
        const int rowCount = m_tableWidget->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            const QTableWidgetItem* item = m_tableWidget->item(i, 0);
            if (AZStd::find(selectedMotionIds.begin(), selectedMotionIds.end(), item->text()) != selectedMotionIds.end())
            {
                m_tableWidget->selectRow(i);
            }
        }
    }

    void MotionSetWindow::Select(EMotionFX::MotionSet::MotionEntry* motionEntry)
    {
        m_tableWidget->clearSelection();

        const int rowCount = m_tableWidget->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            const QTableWidgetItem* item = m_tableWidget->item(i, 1);
            if (item->text() == motionEntry->GetId().c_str())
            {
                m_tableWidget->selectRow(i);
            }
        }
    }

    EMotionFX::MotionSet::MotionEntry* MotionSetWindow::FindMotionEntry(QTableWidgetItem* item) const
    {
        if (!item)
        {
            return nullptr;
        }

        const EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();
        if (!motionSet)
        {
            return nullptr;
        }

        // Get the row of the item and use it as index to retrieve the table widget item where we store our motion id.
        const int row = item->row();
        const QTableWidgetItem* idItem = m_tableWidget->item(row, 1);

        // Find the motion entry based on the string id and return the result.
        EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryById(idItem->text().toUtf8().data());
        AZ_Assert(motionEntry, "Motion entry for item (Text='%s', Row=%d) not found.", item->text().toUtf8().data(), item->row());
        return motionEntry;
    }

    EMotionFX::MotionSet::MotionEntry* MotionSetWindow::FindMotionEntryByMotionId(AZ::u32 motionId) const
    {
        const EMotionFX::MotionSet* motionSet = m_plugin->GetSelectedSet();
        if (!motionSet)
        {
            return nullptr;
        }

        const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
        for (const auto& pair : motionEntries)
        {
            EMotionFX::MotionSet::MotionEntry* entry = pair.second;
            const EMotionFX::Motion* motion = entry->GetMotion();
            if (motion &&
                motionId == motion->GetID())
            {
                return entry;
            }
        }

        return nullptr;
    }

    QTableWidgetItem* MotionSetWindow::FindTableWidgetItemByEntry(EMotionFX::MotionSet::MotionEntry* motionEntry) const
    {
        const AZStd::string& motionEntryId = motionEntry->GetId();
        const int rowCount = m_tableWidget->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            QTableWidgetItem* item = m_tableWidget->item(i, 1);
            if (item->text() == motionEntryId.c_str())
            {
                return item;
            }
        }

        return nullptr;
    }

    void MotionSetWindow::GetRowIndices(const QList<QTableWidgetItem*>& items, AZStd::vector<int>& outRowIndices)
    {
        const int numItems = items.size();
        outRowIndices.reserve(numItems);

        for (const QTableWidgetItem* item : items)
        {
            const int rowIndex = item->row();
            if (AZStd::find(outRowIndices.begin(), outRowIndices.end(), rowIndex) == outRowIndices.end())
            {
                outRowIndices.push_back(rowIndex);
            }
        }
    }


    size_t MotionSetWindow::CalcNumMotionEntriesUsingMotionExcluding(const AZStd::string& motionFilename, EMotionFX::MotionSet* excludedMotionSet)
    {
        if (motionFilename.empty())
        {
            return 0;
        }

        // Iterate through all available motion sets and count how many entries are refering to the given motion file.
        size_t counter = 0;
        const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (size_t i = 0; i < numMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }
            if (motionSet == excludedMotionSet)
            {
                continue;
            }

            const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
            for (const auto& item : motionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

                if (motionFilename == motionEntry->GetFilename())
                {
                    counter++;
                }
            }
        }

        return counter;
    }
} // namespace EMStudio
