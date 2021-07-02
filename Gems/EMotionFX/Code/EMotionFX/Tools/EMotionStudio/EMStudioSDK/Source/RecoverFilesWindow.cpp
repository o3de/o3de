/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <MCore/Source/StringConversions.h>
#include "RecoverFilesWindow.h"
#include <QHeaderView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>


namespace EMStudio
{
    RecoverFilesWindow::RecoverFilesWindow(QWidget* parent, const AZStd::vector<AZStd::string>& files)
        : QDialog(parent)
    {
        mFiles = files;

        // Update title of the dialog.
        setWindowTitle("Recover Files");

        // Set the window default size.
        resize(1024, 576);

        QVBoxLayout* layout = new QVBoxLayout(this);

        // Add the top window message.
        layout->addWidget(new QLabel("Some files have been corrupted but can be restored. The following files can be recovered:"));

        // Create the table widget.
        mTableWidget = new QTableWidget();
        mTableWidget->setAlternatingRowColors(true);
        mTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
        mTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mTableWidget->setMinimumHeight(250);
        mTableWidget->setMinimumWidth(600);
        mTableWidget->horizontalHeader()->setStretchLastSection(true);
        mTableWidget->setCornerButtonEnabled(false);
        mTableWidget->setSortingEnabled(false);

        mTableWidget->setColumnCount(3);

        // Set the header items.
        QTableWidgetItem* headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("Filename");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Type");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(2, headerItem);

        // Set the horizontal header params.
        QHeaderView* horizontalHeader = mTableWidget->horizontalHeader();
        horizontalHeader->setSectionResizeMode(0, QHeaderView::Fixed);
        horizontalHeader->setStretchLastSection(true);

        mTableWidget->verticalHeader()->hide();

        // Set the left column smaller to only fits the checkbox.
        mTableWidget->horizontalHeader()->resizeSection(0, 19);

        // Set the row count.
        const size_t numFiles = files.size();
        const int rowCount = static_cast<int>(numFiles);
        mTableWidget->setRowCount(rowCount);

        // For each file that might be recovered.
        AZStd::string backupFilename;
        AZStd::string originalFilename;
        AZStd::string filenameText;
        AZStd::string fileTypeString;
        for (size_t i = 0; i < numFiles; ++i)
        {
            backupFilename = files[i];
            AzFramework::StringFunc::Path::StripExtension(backupFilename);

            // Get the original filename from the recover json.
            originalFilename = GetOriginalFilenameFromRecoverFile(backupFilename.c_str());

            // Create the filename table entry.
            if (originalFilename.empty())
            {
                filenameText = "<empty>";
            }
            else
            {
                AZStd::string path, filenameOnly;
                AzFramework::StringFunc::Path::GetFullPath(originalFilename.c_str(), path);
                AzFramework::StringFunc::Path::GetFullFileName(originalFilename.c_str(), filenameOnly);

                filenameText = AZStd::string::format("<qt>%s<b>%s</b></qt>", path.c_str(), filenameOnly.c_str());
            }

            // Create the checkbox.
            QCheckBox* checkbox = new QCheckBox("");
            checkbox->setStyleSheet("background: transparent; padding-left: 3px; max-width: 13px;");
            checkbox->setChecked(true);

            // Create the filename label.
            QLabel* filenameLabel = new QLabel();
            filenameLabel->setText(filenameText.c_str());
            filenameLabel->setToolTip(backupFilename.c_str());

            // Set the file type.
            switch (EMotionFX::GetImporter().CheckFileType(backupFilename.c_str()))
            {
                case EMotionFX::Importer::FILETYPE_UNKNOWN:
                {
                    // Get the extension of the backup filename.
                    AZStd::string extensionOnly;
                    AzFramework::StringFunc::Path::GetExtension(backupFilename.c_str(), extensionOnly, false /* include dot */);

                    // Check the extension to set the file type text.
                    if (AzFramework::StringFunc::Equal(extensionOnly.c_str(), "emfxworkspace"))
                    {
                        fileTypeString = "Workspace";
                    }
                    if (AzFramework::StringFunc::Equal(extensionOnly.c_str(), "emfxMeta"))
                    {
                        fileTypeString = "Meta Data";
                    }
                    else
                    {
                        fileTypeString = "Unknown";
                    }
                    break;
                }

                case EMotionFX::Importer::FILETYPE_ACTOR:
                    fileTypeString = "Actor";
                    break;

                case EMotionFX::Importer::FILETYPE_MOTION:
                    fileTypeString = "Motion";
                    break;

                case EMotionFX::Importer::FILETYPE_ANIMGRAPH:
                    fileTypeString = "Anim Graph";
                    break;

                case EMotionFX::Importer::FILETYPE_MOTIONSET:
                    fileTypeString = "Motion Set";
                    break;

                case EMotionFX::Importer::FILETYPE_NODEMAP:
                    fileTypeString = "Node Map";
                    break;

                default:
                    fileTypeString.clear();
                    break;
            }

            // Create the table item.
            const int row = static_cast<int>(i);
            QTableWidgetItem* itemType = new QTableWidgetItem(fileTypeString.c_str());
            itemType->setData(Qt::UserRole, row);

            // Add table items to the current row.
            mTableWidget->setCellWidget(row, 0, checkbox);
            mTableWidget->setCellWidget(row, 1, filenameLabel);
            mTableWidget->setItem(row, 2, itemType);

            mTableWidget->setRowHeight(row, 21);
        }

        mTableWidget->setSortingEnabled(true);

        // Set the size of the filename column to take the whole space.
        mTableWidget->setColumnWidth(1, 894);

        // Needed to have the last column stretching correctly.
        mTableWidget->setColumnWidth(2, 0);

        layout->addWidget(mTableWidget);

        // Create the warning message.
        QLabel* warningLabel = new QLabel("<font color='yellow'>Warning: Files that will not be recovered will be deleted</font>");
        warningLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        // Add the button layout.
        QHBoxLayout* buttonLayout      = new QHBoxLayout();
        QPushButton* recoverButton     = new QPushButton("Recover Selected");
        QPushButton* skipRecoverButton = new QPushButton("Skip Recovering");
        buttonLayout->addWidget(warningLabel);
        buttonLayout->addWidget(recoverButton);
        buttonLayout->addWidget(skipRecoverButton);
        layout->addLayout(buttonLayout);

        connect(recoverButton, &QPushButton::clicked, this, &RecoverFilesWindow::accept);
        connect(skipRecoverButton, &QPushButton::clicked, this, &RecoverFilesWindow::reject);
        
        connect(this, &RecoverFilesWindow::accepted, this, &RecoverFilesWindow::Accepted);
        connect(this, &RecoverFilesWindow::rejected, this, &RecoverFilesWindow::Rejected);

        setFocus();
    }


    RecoverFilesWindow::~RecoverFilesWindow()
    {
    }


    AZStd::string RecoverFilesWindow::GetOriginalFilenameFromRecoverFile(const char* recoverFilename)
    {
        AZStd::string jsonFilename = recoverFilename;
        jsonFilename += ".recover";

        AZ::IO::FileIOBase* fileReader = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        if (fileReader->Open(jsonFilename.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle))
        {
            AZ::u64 fileSize = 0;
            fileReader->Size(fileHandle, fileSize);

            AZStd::string fileBuffer;
            fileBuffer.resize(fileSize);

            if (fileReader->Read(fileHandle, fileBuffer.data(), fileSize, true))
            {
                rapidjson::Document document;
                document.Parse(fileBuffer.data());
                if (document.HasParseError())
                {
                    AZ_Error("EMotionStudio", false, "Cannot parse json file %s.", jsonFilename.c_str());
                }
                else
                {
                    rapidjson::Value::MemberIterator memberIterator = document.FindMember("OriginalFileName");
                    if (memberIterator != document.MemberEnd())
                    {
                        fileReader->Close(fileHandle);
                        return memberIterator->value.GetString();
                    }
                }
            }

            fileReader->Close(fileHandle);
        }

        return AZStd::string();
    }


    // Called in case we want to recover our files.
    void RecoverFilesWindow::Accepted()
    {
        using namespace AZ::IO;
        FileIOBase* fileIo = FileIOBase::GetInstance();

        AZStd::string backupFilename;
        AZStd::string originalFilename;

        const int numRows = mTableWidget->rowCount();
        for (int i = 0; i < numRows; ++i)
        {
            QWidget*            widget      = mTableWidget->cellWidget(i, 0);
            QCheckBox*          checkbox    = static_cast<QCheckBox*>(widget);
            QTableWidgetItem*   item        = mTableWidget->item(i, 2);
            const int32         filesIndex  = item->data(Qt::UserRole).toInt();

            // Get the recover and the backup filenames
            const AZStd::string& recoverFilename = mFiles[filesIndex];
            
            backupFilename = recoverFilename;
            AzFramework::StringFunc::Path::StripExtension(backupFilename);

            // Add the file if the checkbox is checked.
            if (checkbox->isChecked())
            {
                // If the backup file doesn't exist anymore, we continue to the next file.
                if (!fileIo->Exists(backupFilename.c_str()))
                {
                    continue;
                }

                // Read the original filename from the .recover json file and check if it is valid.
                originalFilename = GetOriginalFilenameFromRecoverFile(backupFilename.c_str());
                if (originalFilename.empty())
                {
                    continue;
                }

                // Remove the original file.
                // This is needed because if the file still sexists, it's not possible to copy the backup file over.
                if (fileIo->Exists(originalFilename.c_str()))
                {
                    if (fileIo->Remove(originalFilename.c_str()) == ResultCode::Error)
                    {
                        const AZStd::string errorMessage = AZStd::string::format("Cannot delete file '<b>%s</b>'.", originalFilename.c_str());
                        CommandSystem::GetCommandManager()->AddError(errorMessage);
                        AZ_Error("EMotionFX", false, errorMessage.c_str());
                    }
                }

                // Copy the backup file over to the original file path.
                if (fileIo->Copy(backupFilename.c_str(), originalFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("Cannot copy file from '<b>%s</b>' to '<b>%s</b>'.", backupFilename.c_str(), originalFilename.c_str());
                    CommandSystem::GetCommandManager()->AddError(errorMessage);
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }

                // Remove the backup file.
                if (fileIo->Remove(backupFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("Cannot delete file '<b>%s</b>'.", backupFilename.c_str());
                    CommandSystem::GetCommandManager()->AddError(errorMessage);
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }

                // Remove the recover file.
                if (fileIo->Remove(recoverFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("Cannot delete file '<b>%s</b>'.", recoverFilename.c_str());
                    CommandSystem::GetCommandManager()->AddError(errorMessage);
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }
            }
            else
            {
                // Remove the recover file.
                if (fileIo->Remove(recoverFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("Cannot delete file '<b>%s</b>'.", recoverFilename.c_str());
                    CommandSystem::GetCommandManager()->AddError(errorMessage);
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }
                
                // Remove the backup file.
                if (fileIo->Remove(backupFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("Cannot delete file '<b>%s</b>'.", backupFilename.c_str());
                    CommandSystem::GetCommandManager()->AddError(errorMessage);
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }
            }
        }
    }


    // Called in case we don't want to recover our files.
    void RecoverFilesWindow::Rejected()
    {
        using namespace AZ::IO;
        FileIOBase* fileIo = FileIOBase::GetInstance();

        const size_t numFiles = mFiles.size();
        AZStd::string backupFilename;
        for (size_t i = 0; i < numFiles; ++i)
        {
            backupFilename = mFiles[i];
            AzFramework::StringFunc::Path::StripExtension(backupFilename);

            // Remove the recover file.
            if (fileIo->Remove(mFiles[i].c_str()) == ResultCode::Error)
            {
                const AZStd::string errorMessage = AZStd::string::format("Cannot delete file '<b>%s</b>'.", mFiles[i].c_str());
                CommandSystem::GetCommandManager()->AddError(errorMessage);
                AZ_Error("EMotionFX", false, errorMessage.c_str());
            }

            // Remove the backup file.
            if (fileIo->Remove(backupFilename.c_str()) == ResultCode::Error)
            {
                const AZStd::string errorMessage = AZStd::string::format("Cannot delete file '<b>%s</b>'.", backupFilename.c_str());
                CommandSystem::GetCommandManager()->AddError(errorMessage);
                AZ_Error("EMotionFX", false, errorMessage.c_str());
            }
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_RecoverFilesWindow.cpp>
