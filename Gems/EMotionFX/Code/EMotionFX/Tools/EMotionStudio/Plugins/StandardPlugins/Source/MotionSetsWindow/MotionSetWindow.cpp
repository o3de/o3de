/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include "../../../../EMStudioSDK/Source/EMStudioCore.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/FileManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <MCore/Source/FileSystem.h>
#include "../MotionWindow/MotionListWindow.h"
#include "MotionSetManagementWindow.h"


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
        mMotionSet = motionSet;
        m_motionId = motionId;

        // Build a list of unique string id values from all motion set entries.
        mMotionSet->BuildIdStringList(m_existingIds);

        // Set the window title and minimum width.
        setWindowTitle("Enter new motion ID");
        setMinimumWidth(300);

        QVBoxLayout* layout = new QVBoxLayout();

        mLineEdit = new QLineEdit();
        connect(mLineEdit, &QLineEdit::textEdited, this, &RenameMotionEntryWindow::TextEdited);
        layout->addWidget(mLineEdit);

        // Set the old motion id as text and select all so that the user can directly start typing.
        mLineEdit->setText(m_motionId.c_str());
        mLineEdit->selectAll();

        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        mOKButton                   = new QPushButton("OK");
        QPushButton* cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(cancelButton);

        // Allow pressing the enter key as alternative to pressing the ok button for faster workflow.
        mOKButton->setAutoDefault(true);
        mOKButton->setDefault(true);

        connect(mOKButton, &QPushButton::clicked, this, &RenameMotionEntryWindow::Accepted);
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
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
            return;
        }

        //mErrorMsg->setVisible(false);
        mOKButton->setEnabled(true);
        mLineEdit->setStyleSheet("");
    }


    void RenameMotionEntryWindow::Accepted()
    {
        AZStd::string commandString = AZStd::string::format("MotionSetAdjustMotion -motionSetID %i -idString \"%s\" -newIDString \"%s\" -updateMotionNodeStringIDs true",
                mMotionSet->GetID(),
                m_motionId.c_str(),
                mLineEdit->text().toUtf8().data());

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
        mPlugin             = parentPlugin;
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
        m_tableWidget = new MotionSetTableWidget(mPlugin, this);
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
        EMotionFX::MotionSet* selectedSet = mPlugin->GetSelectedSet();
        const uint32 selectedSetIndex = EMotionFX::GetMotionManager().FindMotionSetIndex(selectedSet);
        if (selectedSetIndex != MCORE_INVALIDINDEX32)
        {
            UpdateMotionSetTable(m_tableWidget, mPlugin->GetSelectedSet());
        }
        else
        {
            UpdateMotionSetTable(m_tableWidget, nullptr);
        }
    }


    bool MotionSetWindow::AddMotion(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry)
    {
        // check if the motion set is the one we currently see in the interface, if not there is nothing to do
        if (mPlugin->GetSelectedSet() == motionSet)
        {
            InsertRow(motionSet, motionEntry, m_tableWidget, false);
        }

        // check if the motion set is the one we currently see in the interface in the right table, if not there is nothing to do
        //if (mRightSelectedSet == motionSet)
        //  InsertRow(motionSet, motionEntry, mMotionSetTableRight, true);

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
        if (mPlugin->GetSelectedSet() == motionSet)
        {
            FillRow(motionSet, motionEntry, rowIndex, m_tableWidget, false);
        }

        UpdateInterface();
        return true;
    }


    bool MotionSetWindow::RemoveMotion(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry)
    {
        // Check if the motion set is the one we currently see in the interface, if not there is nothing to do.
        if (mPlugin->GetSelectedSet() == motionSet)
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

        command = AZStd::string::format("Select -motionIndex %d", EMotionFX::GetMotionManager().FindMotionIndexByID(motion->GetID()));
        commandGroup.AddCommandString(command);

        EMotionFX::PlayBackInfo* defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();
        if (defaultPlayBackInfo)
        {
            // Don't blend in and out of the for previewing animations. We might only see a short bit of it for animations smaller than the blend in/out time.
            defaultPlayBackInfo->mBlendInTime = 0.0f;
            defaultPlayBackInfo->mBlendOutTime = 0.0f;
            defaultPlayBackInfo->mFreezeAtLastFrame = (defaultPlayBackInfo->mNumLoops != EMFX_LOOPFOREVER);
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
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

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

            row++;
        }

        // enable the sorting
        tableWidget->setSortingEnabled(true);
    }


    void MotionSetWindow::UpdateInterface()
    {
        EMotionFX::MotionSet* motionSet = mPlugin->GetSelectedSet();

        const bool isEnabled = (motionSet != nullptr);
        m_addAction->setEnabled(isEnabled);
        m_loadAction->setEnabled(isEnabled);
        m_editAction->setEnabled(isEnabled);
        m_tableWidget->setEnabled(isEnabled);

        if (!motionSet)
        {
            return;
        }

        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
        const uint32 numSelectedItems = selectedItems.count();

        // Get the row indices from the selected items.
        AZStd::vector<int> rowIndices;
        GetRowIndices(selectedItems, rowIndices);

        // actions which need at least one motion
        const bool hasMotions = m_tableWidget->rowCount() > 0;
        m_editAction->setEnabled(hasMotions);

        // Inform the time view plugin about the motion selection change.
        const bool hasSelectedRows = rowIndices.size() > 0;
        if (hasSelectedRows)
        {
            QTableWidgetItem* firstSelectedItem = selectedItems[0];
            EMotionFX::MotionSet::MotionEntry* motionEntry = FindMotionEntry(firstSelectedItem);
            if (motionEntry)
            {
                EMotionFX::Motion* motion = motionEntry->GetMotion();
                if (motion)
                {
                    MCore::CommandGroup commandGroup("Select motion");
                    commandGroup.AddCommandString("Unselect -motionIndex SELECT_ALL");
                    const AZ::u32 motionIndex = EMotionFX::GetMotionManager().FindMotionIndexByFileName(motion->GetFileName());
                    commandGroup.AddCommandString(AZStd::string::format("Select -motionIndex %d", motionIndex));

                    AZStd::string result;
                    if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result, false))
                    {
                        AZ_Error("EMotionFX", false, result.c_str());
                    }

                    emit MotionSelectionChanged();
                }
            }
        }
    }


    void MotionSetWindow::OnAddNewEntry()
    {
        EMotionFX::MotionSet* selectedSet = mPlugin->GetSelectedSet();
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
        EMotionFX::MotionSet* selectedSet = mPlugin->GetSelectedSet();
        if (!selectedSet)
        {
            return;
        }

        CommandSystem::LoadMotionsCommand(filenames);
        const size_t numFileNames = filenames.size();

        // Build a list of unique string id values from all motion set entries.
        AZStd::vector<AZStd::string> idStrings;
        idStrings.reserve(selectedSet->GetNumMotionEntries() + (uint32)numFileNames);
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
        EMotionFX::MotionSet* motionSet = mPlugin->GetSelectedSet();
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
        const size_t numRowIndices = rowIndices.size();

        // remove motion from motion window, too?
        bool removeMotion = false;

        // Construct message box title.
        AZStd::string msgBoxTitle;
        if (rowIndices.size() == 1)
        {
            msgBoxTitle = "<p align='center'>Also Remove The Selected Motion From Motions Window?</p>";
        }
        else
        {
            msgBoxTitle = "<p align='center'>Also Remove The Selected Motions From Motions Window?</p>";
        }

        // ask to remove motions
        QMessageBox* question = new QMessageBox(this);
        question->setText(tr(msgBoxTitle.c_str()));
        question->setWindowTitle("Remove From Workspace?");
        QAbstractButton* removeInAllButton = question->addButton(tr("Yes (Recommended)"), QMessageBox::YesRole);
        removeInAllButton->setObjectName("EMFX.MotionSet.RemoveMotionMessageBox.YesButton");
        QAbstractButton* removeInMotionSetButton = question->addButton(tr("No"), QMessageBox::NoRole);
        removeInMotionSetButton->setObjectName("EMFX.MotionSet.RemoveMotionMessageBox.NoButton");
        QAbstractButton* cancelActionButton = question->addButton(tr("Cancel"), QMessageBox::RejectRole);
        cancelActionButton->setObjectName("EMFX.MotionSet.RemoveMotionMessageBox.CancelButton");

        question->exec();

        // Create the failed remove motions array.
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;
        failedRemoveMotions.reserve(rowIndices.size());

        // Iterate over all motions and add them.
        AZStd::string motionIdsToRemoveString;
        AZStd::vector<AZStd::string> removeMotionCommands;
        AZStd::string commandString, motionFilename;

        if (question->clickedButton() == removeInAllButton)
        {
            removeMotion = true;
        }

        if (question->clickedButton() == cancelActionButton)
        {
            failedRemoveMotions.clear();
            return;
        }

        for (uint32 i = 0; i < numRowIndices; ++i)
        {
            QTableWidgetItem* idItem = m_tableWidget->item(rowIndices[i], 1);
            EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryById(idItem->text().toUtf8().data());

            // Check if the motion exists in multiple motion sets.
            const AZ::u32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            AZ::u32 numMotionSetContainsMotion = 0;

            for (AZ::u32 motionSetId = 0; motionSetId < numMotionSets; motionSetId++)
            {
                EMotionFX::MotionSet* motionSet2 = EMotionFX::GetMotionManager().GetMotionSet(motionSetId);
                if (motionSet2->FindMotionEntryById(motionEntry->GetId()))
                {
                    numMotionSetContainsMotion++;
                    if (numMotionSetContainsMotion > 1)
                    {
                        break;
                    }
                }
            }

            // If motion exists in multiple motion sets, then it should not be removed from motions window.
            if (removeMotion && numMotionSetContainsMotion > 1)
            {
                continue;
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

            // Check if the motion is not valid, that means the motion is not loaded.
            if (removeMotion && motionEntry->GetMotion())
            {
                // Calculcate how many motion sets except than the provided one use the given motion.
                uint32 numExternalUses = CalcNumMotionEntriesUsingMotionExcluding(motionEntry->GetFilename(), motionSet);

                // Remove the motion in case it was only used by the given motion set.
                if (numExternalUses == 0)
                {
                    motionFilename = motionSet->ConstructMotionFilename(motionEntry);
                    commandString = AZStd::string::format("RemoveMotion -filename \"%s\"", motionFilename.c_str());
                    removeMotionCommands.emplace_back(commandString);
                }
                else if (numExternalUses > 0)
                {
                    failedRemoveMotions.push_back(motionEntry->GetMotion());
                }
            }
        }

        // Find the lowest row selected.
        int lowestRowSelected = -1;
        for (uint32 i = 0; i < numRowIndices; ++i)
        {
            if (rowIndices[i] < lowestRowSelected)
            {
                lowestRowSelected = rowIndices[i];
            }
        }


        MCore::CommandGroup commandGroup("Motion set remove motions");

        // 1. Remove motion entries from the motion set.
        commandString = AZStd::string::format("MotionSetRemoveMotion -motionSetID %i -motionIds \"", motionSet->GetID());
        commandString += motionIdsToRemoveString;
        commandString += '\"';
        commandGroup.AddCommandString(commandString);

        // 2. Then get rid of the actual motions itself.
        for (const AZStd::string& command : removeMotionCommands)
        {
            commandGroup.AddCommandString(command);
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
        RenameMotionEntryWindow window(this, mPlugin->GetSelectedSet(), motionEntry->GetId().c_str());
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
        EMotionFX::MotionSet* motionSet = mPlugin->GetSelectedSet();
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
        EMotionFX::MotionSet* motionSet = mPlugin->GetSelectedSet();
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

        // ask to remove motions
        if (QMessageBox::question(this, "Remove Motions From Project?", "Remove the motions from the project entirely? This would also remove them from the motion list. Pressing no will remove them from the motion set but keep them inside the motion list inside the motions window.", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
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
                uint32 numExternalUses = CalcNumMotionEntriesUsingMotionExcluding(motionEntry->GetFilename(), motionSet);

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

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();

        // Get the row indices from the selected items.
        AZStd::vector<int> rowIndices;
        GetRowIndices(selectedItems, rowIndices);

        // get the selected motion set
        EMotionFX::MotionSet* motionSet = mPlugin->GetSelectedSet();

        // generate the motions IDs array
        AZStd::vector<AZStd::string> motionIDs;
        const size_t numSelectedRows = rowIndices.size();
        if (numSelectedRows > 0)
        {
            for (int i = 0; i < numSelectedRows; ++i)
            {
                QTableWidgetItem* item = m_tableWidget->item(rowIndices[i], 1);
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


    void MotionSetWindow::OnEntryDoubleClicked(QTableWidgetItem* item)
    {
        const EMotionFX::MotionSet* motionSet = mPlugin->GetSelectedSet();
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
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
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
        }
        else if (rowIndices.size() > 1)
        {
            // Unassign linked motions for the selected entries.
            QAction* unassignMotionAction = menu.addAction("Unassign Motions");
            connect(unassignMotionAction, &QAction::triggered, this, &MotionSetWindow::OnUnassignMotions);
        }

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
        mPlugin = parentPlugin;

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
        mPlugin->GetMotionSetWindow()->dropEvent(event);
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
        EMotionFX::MotionSet* motionSet = mPlugin->GetSelectedSet();
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
        mMotionSet = motionSet;
        mMotionIDs = motionIDs;

        // Reserve space.
        mValids.reserve(mMotionIDs.size());
        mMotionToModifiedMap.reserve(mMotionIDs.size());
        mModifiedMotionIDs.reserve(motionSet->GetNumMotionEntries());

        // set the window title
        setWindowTitle("Batch Edit Motion IDs");

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // create the spacer
        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        // create the combobox
        mComboBox = new QComboBox();
        mComboBox->addItem("Replace All");
        mComboBox->addItem("Replace First");
        mComboBox->addItem("Replace Last");

        // connect the combobox
        connect(mComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MotionEditStringIDWindow::CurrentIndexChanged);

        // create the string line edits
        mStringALineEdit = new QLineEdit();
        mStringBLineEdit = new QLineEdit();

        // connect the line edit
        connect(mStringALineEdit, &QLineEdit::textChanged, this, &MotionEditStringIDWindow::StringABChanged);
        connect(mStringBLineEdit, &QLineEdit::textChanged, this, &MotionEditStringIDWindow::StringABChanged);

        // add the operation layout
        QHBoxLayout* operationLayout = new QHBoxLayout();
        operationLayout->addWidget(new QLabel("Operation:"));
        operationLayout->addWidget(mComboBox);
        operationLayout->addWidget(spacerWidget);
        operationLayout->addWidget(new QLabel("StringA:"));
        operationLayout->addWidget(mStringALineEdit);
        operationLayout->addWidget(new QLabel("StringB:"));
        operationLayout->addWidget(mStringBLineEdit);
        layout->addLayout(operationLayout);

        // create the table widget
        mTableWidget = new QTableWidget();
        mTableWidget->setAlternatingRowColors(true);
        mTableWidget->setGridStyle(Qt::SolidLine);
        mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        mTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // set the table widget columns
        mTableWidget->setColumnCount(2);
        QStringList headerLabels;
        headerLabels.append("Before");
        headerLabels.append("After");
        mTableWidget->setHorizontalHeaderLabels(headerLabels);
        mTableWidget->horizontalHeader()->setStretchLastSection(true);
        mTableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        mTableWidget->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);

        // Set the row count
        const size_t numMotionIDs = mMotionIDs.size();
        mTableWidget->setRowCount(static_cast<int>(numMotionIDs));

        // disable the sorting
        mTableWidget->setSortingEnabled(false);

        // initialize the table
        for (size_t i = 0; i < numMotionIDs; ++i)
        {
            // create the before and after table widget items
            QTableWidgetItem* beforeTableWidgetItem = new QTableWidgetItem(mMotionIDs[i].c_str());
            QTableWidgetItem* afterTableWidgetItem = new QTableWidgetItem(mMotionIDs[i].c_str());

            // set the text of the row
            const int row = static_cast<int>(i);
            mTableWidget->setItem(row, 0, beforeTableWidgetItem);
            mTableWidget->setItem(row, 1, afterTableWidgetItem);
        }

        mTableWidget->setSortingEnabled(true);
        mTableWidget->resizeColumnToContents(0);
        mTableWidget->setCornerButtonEnabled(false);

        layout->addWidget(mTableWidget);

        // create the num motion IDs label
        // this label never change, it's the total of motion ID in the table
        mNumMotionIDsLabel = new QLabel();
        mNumMotionIDsLabel->setAlignment(Qt::AlignLeft);
        mNumMotionIDsLabel->setText(QString("Number of motion IDs: %1").arg(numMotionIDs));

        // create the num modified IDs label
        mNumModifiedIDsLabel = new QLabel();
        mNumModifiedIDsLabel->setAlignment(Qt::AlignCenter);
        mNumModifiedIDsLabel->setText("Number of modified IDs: 0");

        // create the num duplicate IDs label
        mNumDuplicateIDsLabel = new QLabel();
        mNumDuplicateIDsLabel->setAlignment(Qt::AlignRight);
        mNumDuplicateIDsLabel->setText("Number of duplicate IDs: 0");

        // add the stats layout
        QHBoxLayout* statsLayout = new QHBoxLayout();
        statsLayout->addWidget(mNumMotionIDsLabel);
        statsLayout->addWidget(mNumModifiedIDsLabel);
        statsLayout->addWidget(mNumDuplicateIDsLabel);
        layout->addLayout(statsLayout);

        // add the bottom buttons
        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        mApplyButton                = new QPushButton("Apply");
        QPushButton* closeButton    = new QPushButton("Close");
        buttonLayout->addWidget(mApplyButton);
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        // apply button is disabled because nothing is changed
        mApplyButton->setEnabled(false);

        // connect the buttons
        connect(mApplyButton, &QPushButton::clicked, this, &MotionEditStringIDWindow::Accepted);
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
        const size_t numValid = mValids.size();
        for (size_t i = 0; i < numValid; ++i)
        {
            // get the motion ID and the modified ID
            AZStd::string& motionID = mMotionIDs[mValids[i]];
            const AZStd::string& modifiedID = mModifiedMotionIDs[mMotionToModifiedMap[mValids[i]]];

            commandString = AZStd::string::format("MotionSetAdjustMotion -motionSetID %i -idString \"%s\" -newIDString \"%s\" -updateMotionNodeStringIDs true", mMotionSet->GetID(), motionID.c_str(), modifiedID.c_str());
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
        mStringALineEdit->blockSignals(true);
        mStringBLineEdit->blockSignals(true);

        // reset the string line edits
        mStringALineEdit->setText("");
        mStringBLineEdit->setText("");

        // enable signals after the reset
        mStringALineEdit->blockSignals(false);
        mStringBLineEdit->blockSignals(false);

        // disable the sorting
        mTableWidget->setSortingEnabled(false);

        // set the new table using modified motion IDs
        const size_t numMotionIDs = mMotionIDs.size();
        for (size_t i = 0; i < numMotionIDs; ++i)
        {
            // create the before and after table widget items
            QTableWidgetItem* beforeTableWidgetItem = new QTableWidgetItem(mMotionIDs[i].c_str());
            QTableWidgetItem* afterTableWidgetItem = new QTableWidgetItem(mMotionIDs[i].c_str());

            // set the text of the row
            const int row = static_cast<int>(i);
            mTableWidget->setItem(row, 0, beforeTableWidgetItem);
            mTableWidget->setItem(row, 1, afterTableWidgetItem);
        }

        // enable the sorting
        mTableWidget->setSortingEnabled(true);

        // resize before column
        mTableWidget->resizeColumnToContents(0);

        // reset the stats
        mNumModifiedIDsLabel->setText("Number of modified IDs: 0");
        mNumDuplicateIDsLabel->setText("Number of duplicate IDs: 0");

        // apply button is disabled because nothing is changed
        mApplyButton->setEnabled(false);
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
        const size_t numMotionIDs = mMotionIDs.size();

        // Remember the selected motion IDs so we can restore selection after swapping the table items.
        const QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();
        const int numSelectedItems = selectedItems.size();
        QVector<QString> selectedMotionIds(numSelectedItems);
        for (int i = 0; i < numSelectedItems; ++i)
        {
            selectedMotionIds[i] = selectedItems[i]->text();
        }

        // special case where the string A and B are empty, nothing is replaced
        if ((mStringALineEdit->text().isEmpty()) && (mStringBLineEdit->text().isEmpty()))
        {
            // disable the sorting
            mTableWidget->setSortingEnabled(false);

            // reset the table
            for (size_t i = 0; i < numMotionIDs; ++i)
            {
                // create the before and after table widget items
                QTableWidgetItem* beforeTableWidgetItem = new QTableWidgetItem(mMotionIDs[i].c_str());
                QTableWidgetItem* afterTableWidgetItem = new QTableWidgetItem(mMotionIDs[i].c_str());

                // set the text of the row
                const int row = static_cast<int>(i);
                mTableWidget->setItem(row, 0, beforeTableWidgetItem);
                mTableWidget->setItem(row, 1, afterTableWidgetItem);
            }

            // enable the sorting
            mTableWidget->setSortingEnabled(true);

            // reset the stats
            mNumModifiedIDsLabel->setText("Number of modified IDs: 0");
            mNumDuplicateIDsLabel->setText("Number of duplicate IDs: 0");

            // apply button is disabled because nothing is changed
            mApplyButton->setEnabled(false);

            // stop here
            return;
        }

        // found flags
        uint32 numDuplicateFound = 0;

        // Clear the arrays but keep the memory to avoid alloc.
        mValids.clear();
        mModifiedMotionIDs.clear();
        mMotionToModifiedMap.clear();

        // Copy all motion IDs from the motion set in the modified array.
        const EMotionFX::MotionSet::MotionEntries& motionEntries = mMotionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            mModifiedMotionIDs.push_back(motionEntry->GetId().c_str());
        }

        // Modify each ID using the operation in the modified array.
        AZStd::string newMotionID;
        AZStd::string tempString;
        for (uint32 i = 0; i < numMotionIDs; ++i)
        {
            // 0=Replace All, 1=Replace First, 2=Replace Last
            const int operationMode = mComboBox->currentIndex();

            // compute the new text
            switch (operationMode)
            {
                case 0:
                {
                    tempString = mMotionIDs[i].c_str();
                    AzFramework::StringFunc::Replace(tempString, mStringALineEdit->text().toUtf8().data(), mStringBLineEdit->text().toUtf8().data(), true /* case sensitive */);
                    newMotionID = tempString.c_str();
                    break;
                }

                case 1:
                {
                    tempString = mMotionIDs[i].c_str();
                    AzFramework::StringFunc::Replace(tempString, mStringALineEdit->text().toUtf8().data(), mStringBLineEdit->text().toUtf8().data(), true /* case sensitive */, true /* replace first */, false /* replace last */);
                    newMotionID = tempString.c_str();
                    break;
                }

                case 2:
                {
                    tempString = mMotionIDs[i].c_str();
                    AzFramework::StringFunc::Replace(tempString, mStringALineEdit->text().toUtf8().data(), mStringBLineEdit->text().toUtf8().data(), true /* case sensitive */, false /* replace first */, true /* replace last */);
                    newMotionID = tempString.c_str();
                    break;
                }
            }

            // change the value in the array and add the mapping motion to modified
            auto iterator = AZStd::find(mModifiedMotionIDs.begin(), mModifiedMotionIDs.end(), mMotionIDs[i]);
            const size_t modifiedIndex = iterator - mModifiedMotionIDs.begin();
            mModifiedMotionIDs[modifiedIndex] = newMotionID;
            mMotionToModifiedMap.push_back(static_cast<uint32>(modifiedIndex));
        }

        // disable the sorting
        mTableWidget->setSortingEnabled(false);

        // update each row
        for (uint32 i = 0; i < numMotionIDs; ++i)
        {
            // find the index in the motion set
            const AZStd::string& modifiedID = mModifiedMotionIDs[mMotionToModifiedMap[i]];

            // create the before and after table widget items
            QTableWidgetItem* beforeTableWidgetItem = new QTableWidgetItem(mMotionIDs[i].c_str());
            QTableWidgetItem* afterTableWidgetItem = new QTableWidgetItem(modifiedID.c_str());

            // find duplicate
            uint32 itemFoundCounter = 0;
            const AZ::u32 numMotionEntries = static_cast<AZ::u32>(mMotionSet->GetNumMotionEntries());
            for (uint32 k = 0; k < numMotionEntries; ++k)
            {
                if (mModifiedMotionIDs[k] == modifiedID)
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
                if (modifiedID != mMotionIDs[i])
                {
                    // set the row green
                    beforeTableWidgetItem->setForeground(Qt::green);
                    afterTableWidgetItem->setForeground(Qt::green);

                    // add a valid
                    mValids.push_back(i);
                }
            }

            // set the text of the row
            mTableWidget->setItem(i, 0, beforeTableWidgetItem);
            mTableWidget->setItem(i, 1, afterTableWidgetItem);
        }

        // enable the sorting
        mTableWidget->setSortingEnabled(true);

        // update the num modified label
        mNumModifiedIDsLabel->setText(QString("Number of modified IDs: %1").arg(mValids.size()));

        // update the num duplicate label
        // the number is in red if at least one found
        if (numDuplicateFound > 0)
        {
            mNumDuplicateIDsLabel->setText(QString("Number of duplicate IDs: <font color='red'>%1</font>").arg(numDuplicateFound));
        }
        else
        {
            mNumDuplicateIDsLabel->setText("Number of duplicate IDs: 0");
        }

        // enable or disable the apply button
        mApplyButton->setEnabled((mValids.size() > 0) && (numDuplicateFound == 0));

        // Reselect the remembered motions.
        mTableWidget->clearSelection();
        const int rowCount = mTableWidget->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            const QTableWidgetItem* item = mTableWidget->item(i, 0);
            if (AZStd::find(selectedMotionIds.begin(), selectedMotionIds.end(), item->text()) != selectedMotionIds.end())
            {
                mTableWidget->selectRow(i);
            }
        }
    }


    EMotionFX::MotionSet::MotionEntry* MotionSetWindow::FindMotionEntry(QTableWidgetItem* item) const
    {
        if (!item)
        {
            return nullptr;
        }

        const EMotionFX::MotionSet* motionSet = mPlugin->GetSelectedSet();
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


    void MotionSetWindow::GetRowIndices(const QList<QTableWidgetItem*>& items, AZStd::vector<int>& outRowIndices)
    {
        const int numItems = items.size();
        outRowIndices.reserve(numItems);

        for (int i = 0; i < numItems; ++i)
        {
            const int rowIndex = items[i]->row();
            if (AZStd::find(outRowIndices.begin(), outRowIndices.end(), rowIndex) == outRowIndices.end())
            {
                outRowIndices.push_back(rowIndex);
            }
        }
    }


    uint32 MotionSetWindow::CalcNumMotionEntriesUsingMotionExcluding(const AZStd::string& motionFilename, EMotionFX::MotionSet* excludedMotionSet)
    {
        if (motionFilename.empty())
        {
            return 0;
        }

        // Iterate through all available motion sets and count how many entries are refering to the given motion file.
        AZ::u32 counter = 0;
        const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (uint32 i = 0; i < numMotionSets; ++i)
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

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/moc_MotionSetWindow.cpp>
