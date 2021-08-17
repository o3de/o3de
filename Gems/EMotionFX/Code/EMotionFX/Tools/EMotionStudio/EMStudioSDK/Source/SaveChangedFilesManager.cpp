/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMStudioManager.h"
#include "SaveChangedFilesManager.h"
#include <MysticQt/Source/MysticQtConfig.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/MCoreCommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <QSettings>
#include <QGridLayout>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QCheckBox>


namespace EMStudio
{
    SaveDirtyFilesCallback::ObjectPointer::ObjectPointer() :
        m_actor(nullptr),
        m_motion(nullptr),
        m_motionSet(nullptr),
        m_animGraph(nullptr),
        m_workspace(nullptr)
    {
    }


    // constructor
    DirtyFileManager::DirtyFileManager()
    {
    }


    void DirtyFileManager::RemoveCallback(SaveDirtyFilesCallback* callback, bool delFromMem)
    {
        if (callback == nullptr)
        {
            return;
        }

        m_saveDirtyFilesCallbacks.erase(AZStd::remove(m_saveDirtyFilesCallbacks.begin(), m_saveDirtyFilesCallbacks.end(), callback), m_saveDirtyFilesCallbacks.end());

        if (delFromMem)
        {
            delete callback;
        }
    }


    void DirtyFileManager::SaveSettings()
    {
        const size_t numDirtyFilesCallbacks = m_saveDirtyFilesCallbacks.size();

        // save the callback settings to the config file
        QSettings settings;
        settings.beginGroup("EMotionFX");

        settings.beginGroup("DirtyFileManager");

        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            settings.beginGroup(m_saveDirtyFilesCallbacks[i]->GetFileType());
            settings.setValue("FileExtension", m_saveDirtyFilesCallbacks[i]->GetExtension());
            settings.endGroup();
        }

        settings.endGroup();
        settings.endGroup();
    }


    // destructor
    DirtyFileManager::~DirtyFileManager()
    {
        const size_t numDirtyFilesCallbacks = m_saveDirtyFilesCallbacks.size();
        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            delete m_saveDirtyFilesCallbacks[i];
        }
        m_saveDirtyFilesCallbacks.clear();
    }


    void DirtyFileManager::AddCallback(SaveDirtyFilesCallback* callback)
    {
        QSettings settings;
        settings.beginGroup("EMotionFX");
        settings.beginGroup("DirtyFileManager");

        QString compareString = callback->GetExtension();
        QStringList childGroups = settings.childGroups();
        QString fileExtension;
        const uint32 numGroups = childGroups.count();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            settings.beginGroup(childGroups[i]);

            fileExtension = settings.value("FileExtension").toString();
            if (fileExtension == compareString)
            {
                //const bool askIndividually = settings.value("AskIndividually").toBool();
                //const bool skipSaving = settings.value("skipSaving").toBool();
                //callback->SetAskIndividually( askIndividually );
                //callback->SetSkipSaving( skipSaving );
                settings.endGroup();
                break;
            }

            settings.endGroup();
        }

        settings.endGroup();
        settings.endGroup();

        // get the priority of the new callback
        const uint32 newPriority = callback->GetPriority();
        size_t insertIndex = MCORE_INVALIDINDEX32;

        // get the number of callbacks and iterate through them
        const size_t numCallbacks = m_saveDirtyFilesCallbacks.size();
        for (size_t i = 0; i < numCallbacks; ++i)
        {
            const uint32 currentPriority = m_saveDirtyFilesCallbacks[i]->GetPriority();

            if (newPriority > currentPriority)
            {
                insertIndex = i;
                break;
            }
        }

        // add the new callback
        if (insertIndex == MCORE_INVALIDINDEX32)
        {
            m_saveDirtyFilesCallbacks.push_back(callback);
        }
        else
        {
            m_saveDirtyFilesCallbacks.insert(m_saveDirtyFilesCallbacks.begin()+insertIndex, callback);
        }
    }


    int DirtyFileManager::SaveDirtyFiles(uint32 type, uint32 filter, QDialogButtonBox::StandardButtons buttons)
    {
        AZStd::vector<SaveDirtyFilesCallback*> neededCallbacks;

        const size_t numDirtyFilesCallbacks = m_saveDirtyFilesCallbacks.size();

        // check if there are any dirty files
        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            SaveDirtyFilesCallback* callback = m_saveDirtyFilesCallbacks[i];

            // make sure we want to handle the given save dirty files callback
            if ((type != MCORE_INVALIDINDEX32 && callback->GetType() != type) || (filter != MCORE_INVALIDINDEX32 && callback->GetType() == filter))
            {
                continue;
            }
            neededCallbacks.push_back(callback);
        }

        return SaveDirtyFiles(neededCallbacks, buttons);
    }

    int DirtyFileManager::SaveDirtyFiles(const AZStd::vector<AZ::TypeId>& typeIds, QDialogButtonBox::StandardButtons buttons)
    {
        AZStd::vector<SaveDirtyFilesCallback*> neededCallbacks;

        const size_t numDirtyFilesCallbacks = m_saveDirtyFilesCallbacks.size();

        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            SaveDirtyFilesCallback* callback = m_saveDirtyFilesCallbacks[i];

            // make sure we want to handle the given save dirty files callback
            if (AZStd::find(typeIds.begin(), typeIds.end(), callback->GetFileRttiType()) != typeIds.end())
            {
                neededCallbacks.push_back(callback);
            }
        }

        return SaveDirtyFiles(neededCallbacks, buttons);
    }

    int DirtyFileManager::SaveDirtyFiles(const AZStd::vector<SaveDirtyFilesCallback*>& neededSaveDirtyFilesCallbacks, QDialogButtonBox::StandardButtons buttons)
    {
        const size_t numDirtyFilesCallbacks = neededSaveDirtyFilesCallbacks.size();

        // check if there are any dirty files
        AZStd::vector<AZStd::string> dirtyFileNames;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> objects;
        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            neededSaveDirtyFilesCallbacks[i]->GetDirtyFileNames(&dirtyFileNames, &objects);
        }

        bool hasDirtyFiles = !dirtyFileNames.empty();

        // only go on in case there are dirty files
        if (hasDirtyFiles == false)
        {
            return NOFILESTOSAVE;
        }

        AZStd::vector<AZStd::string> selectedFileNames;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> selectedObjects;

        // show the dirty files settings window
        SaveDirtySettingsWindow settingsWindow((QWidget*)GetMainWindow(), dirtyFileNames, objects, buttons);
        settingsWindow.setObjectName("EMFX.DirtyFileManager.SaveDirtySettingsWindow");
        if (settingsWindow.exec() == QDialog::Accepted)
        {
            SaveSettings();

            // if we pressed "Skip Saving", return directly
            if (settingsWindow.GetSaveDirtyFiles() == false)
            {
                return FINISHED;
            }
            else
            {
                settingsWindow.GetSelectedFileNames(&selectedFileNames, &selectedObjects);
            }
        }
        else
        {
            // the user pressed the "Cancel" button, return directly without saving dirty files
            SaveSettings();
            return CANCELED;
        }

        // save the dirty files
        if (!selectedFileNames.empty())
        {
            // create one command group for saving all dirty files
            MCore::CommandGroup commandGroup("Save Dirty Files");
            commandGroup.SetReturnFalseAfterError(true);

            // process all non-post processed callbacks
            // iterate through the callbacks
            for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
            {
                SaveDirtyFilesCallback* callback = neededSaveDirtyFilesCallbacks[i];

                // is this a post processed callback? if yes skip
                if (callback->GetIsPostProcessed())
                {
                    continue;
                }

                // save dirty files, in case saving fails return cancel so that they can check them out before closing and don't loose the changes
                if (callback->SaveDirtyFiles(selectedFileNames, selectedObjects, &commandGroup) == CANCELED)
                {
                    return CANCELED;
                }
            }

            // execute the command group
            AZStd::string result;
            if (GetCommandManager()->ExecuteCommandGroup(commandGroup, result, false, true, true) == false)
            {
                AZ_Error("EMotionFX", false, result.c_str());
                return FAILED;
            }

            // process post processed callbacks
            // iterate through the callbacks
            for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
            {
                SaveDirtyFilesCallback* callback = neededSaveDirtyFilesCallbacks[i];

                // is this a post processed callback? if not skip
                if (callback->GetIsPostProcessed() == false)
                {
                    continue;
                }

                // save dirty files, in case saving fails return cancel so that they can check them out before closing and don't loose the changes
                if (callback->SaveDirtyFiles(selectedFileNames, selectedObjects, &commandGroup) == CANCELED)
                {
                    return FAILED;
                }
            }

            // in case no commands get called, still handle errors
            if (commandGroup.GetNumCommands() == 0)
            {
                if (GetCommandManager()->ShowErrorReport())
                {
                    return FAILED;
                }
            }
        }

        return FINISHED;
    }


    // constructor
    SaveDirtyFilesCallback::SaveDirtyFilesCallback()
    {
    }


    // destructor
    SaveDirtyFilesCallback::~SaveDirtyFilesCallback()
    {
    }


    // constructor
    SaveDirtySettingsWindow::SaveDirtySettingsWindow(QWidget* parent, const AZStd::vector<AZStd::string>& dirtyFileNames, const AZStd::vector<SaveDirtyFilesCallback::ObjectPointer>& objects, QDialogButtonBox::StandardButtons buttons)
        : QDialog(parent)
    {
        MCORE_ASSERT(dirtyFileNames.size() == objects.size());

        // store values
        m_fileNames      = dirtyFileNames;
        m_objects        = objects;
        m_saveDirtyFiles = true;

        // update title of the dialog
        setWindowTitle("Save Changes To Files");

        // set the default size
        resize(1024, 576);

        // create the layout
        QVBoxLayout* vLayout = new QVBoxLayout(this);

        // add the top message
        vLayout->addWidget(new QLabel("Do you want to save changes? The following files have been changed but have not been saved yet:"));

        // create the lod information table
        m_tableWidget = new QTableWidget();
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setMinimumHeight(250);
        m_tableWidget->setMinimumWidth(600);
        m_tableWidget->verticalHeader()->hide();

        // disable the corner button between the row and column selection thingies
        m_tableWidget->setCornerButtonEnabled(false);

        // enable the custom context menu for the motion table
        m_tableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);

        // disable sorting when adding the items
        m_tableWidget->setSortingEnabled(false);

        // clear the table widget
        m_tableWidget->clear();
        m_tableWidget->setColumnCount(3);

        // set header items for the table
        QTableWidgetItem* headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("FileName");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Type");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(2, headerItem);

        // set column resize modes.  The main filename is in column 1, and
        // stretches to fill any remaining space. The other columns are
        // informational only, and not very wide, so setting them to always
        // resize to their contents means we don't have to manage their column
        // widths.
        QHeaderView* horizontalHeader = m_tableWidget->horizontalHeader();
        horizontalHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        horizontalHeader->setSectionResizeMode(1, QHeaderView::Stretch);
        horizontalHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);

        const size_t numDirtyFiles = dirtyFileNames.size();
        m_tableWidget->setRowCount(static_cast<int>(numDirtyFiles));

        for (size_t i = 0; i < numDirtyFiles; ++i)
        {
            SaveDirtyFilesCallback::ObjectPointer object = m_objects[i];

            QString labelText;
            if (dirtyFileNames[i].empty())
            {
                labelText = "<not saved yet>";
            }
            else
            {
                const AZStd::string& productFilename = dirtyFileNames[i];

                // Get the asset source name from the product filename.
                bool sourceAssetFound = false;
                AZStd::string sourceAssetFilename;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceAssetFound,
                    &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath,
                    productFilename,
                    sourceAssetFilename);

                const AZStd::string usedFilename = sourceAssetFound ? sourceAssetFilename : dirtyFileNames[i];

                // Separate the path from the filename, so that we can display the filename in bold.
                AZStd::string fullPath;
                AzFramework::StringFunc::Path::GetFullPath(usedFilename.c_str(), fullPath);
                AzFramework::StringFunc::RelativePath::Normalize(fullPath); // Add trailing slash in case it is missing
                AZStd::string fullFilename;
                AzFramework::StringFunc::Path::GetFullFileName(usedFilename.c_str(), fullFilename);
                labelText = QString("<qt>%1<b>%2</b></qt>").arg(fullPath.c_str(), fullFilename.c_str());
            }

            // create the checkbox
            QCheckBox* checkbox = new QCheckBox("");
            checkbox->setStyleSheet("background: transparent;");
            checkbox->setChecked(true);

            // create the filename table item
            QLabel* filenameLabel = new QLabel();
            filenameLabel->setToolTip(labelText);
            filenameLabel->setText(labelText);

            QString typeString;
            if (object.m_motion)
            {
                typeString = "Motion";
            }
            else if (object.m_actor)
            {
                typeString = "Actor";
            }
            else if (object.m_motionSet)
            {
                typeString = "Motion Set";
            }
            else if (object.m_animGraph)
            {
                typeString = "Anim Graph";
            }
            else if (object.m_workspace)
            {
                typeString = "Workspace";
            }

            QTableWidgetItem* itemType = new QTableWidgetItem(typeString);
            const int row = static_cast<int>(i);
            itemType->setData(Qt::UserRole, row);

            // add table items to the current row
            m_tableWidget->setCellWidget(row, 0, checkbox);
            m_tableWidget->setCellWidget(row, 1, filenameLabel);
            m_tableWidget->setItem(row, 2, itemType);

            // set the row height
            m_tableWidget->setRowHeight(row, 21);
        }

        // enable sorting
        m_tableWidget->setSortingEnabled(true);

        // add the table in the layout
        vLayout->addWidget(m_tableWidget);

        // the buttons at the bottom of the dialog
        QDialogButtonBox* buttonBox = new QDialogButtonBox(buttons);
        if (buttons & QDialogButtonBox::Save)
        {
            QPushButton* saveButton = buttonBox->button(QDialogButtonBox::Save);
            saveButton->setText("&Save Selected");
            saveButton->setObjectName("EMFX.SaveDirtySettingsWindow.SaveButton");
        }
        if (buttons & QDialogButtonBox::Discard)
        {
            QPushButton* discardButton = buttonBox->button(QDialogButtonBox::Discard);
            discardButton->setText("&Discard Changes");
            discardButton->setObjectName("EMFX.SaveDirtySettingsWindow.DiscardButton");
        }
        if (buttons & QDialogButtonBox::Cancel)
        {
            QPushButton* cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
            cancelButton->setText("&Cancel");
            cancelButton->setObjectName("EMFX.SaveDirtySettingsWindow.CancelButton");
        }
        vLayout->addWidget(buttonBox);

        // connect the buttons
        connect(buttonBox, &QDialogButtonBox::accepted, this, &SaveDirtySettingsWindow::OnSaveButton);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &SaveDirtySettingsWindow::OnCancelButton);
        connect(buttonBox, &QDialogButtonBox::clicked, this, [this, buttonBox](QAbstractButton* button) {
            if (buttonBox->buttonRole(button) == QDialogButtonBox::DestructiveRole) {
                this->OnSkipSavingButton();
            }
        });

        // set the focus on the window
        setFocus();
    }


    // destructor
    SaveDirtySettingsWindow::~SaveDirtySettingsWindow()
    {
    }


    void SaveDirtySettingsWindow::GetSelectedFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<SaveDirtyFilesCallback::ObjectPointer>* outObjects)
    {
        outFileNames->clear();
        outObjects->clear();

        const uint32 numRows = m_tableWidget->rowCount();

        // iteration for motions, motion sets, actors and anim graphs
        for (uint32 i = 0; i < numRows; ++i)
        {
            // get the checkbox
            QWidget*    widget      = m_tableWidget->cellWidget(i, 0);
            QCheckBox*  checkbox    = static_cast<QCheckBox*>(widget);

            // get the type item
            QTableWidgetItem*   item            = m_tableWidget->item(i, 2);
            const int32         filenameIndex   = item->data(Qt::UserRole).toInt();

            // get the object pointer
            SaveDirtyFilesCallback::ObjectPointer objPointer = m_objects[filenameIndex];

            // add the filename to the list of selected filenames in case the checkbox in the same row is checked
            if (checkbox->isChecked())
            {
                outFileNames->push_back(m_fileNames[filenameIndex]);
                outObjects->push_back(objPointer);
            }
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_SaveChangedFilesManager.cpp>
