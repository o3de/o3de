/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/NewFileDialog.h>
#include <source/ui/ui_NewFileDialog.h>

#include <source/ui/PlatformSelectionWidget.h>
#include <source/utils/utils.h>

#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Asset/AssetBundler.h>

#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

namespace AssetBundler
{
    NewFileDialog::NewFileDialog(
        QWidget* parent,
        const QString& dialogTitle,
        const QString& startingPath,
        const char* fileExtension,
        const QString& fileNameFilter,
        const AzFramework::PlatformFlags& enabledPlatforms,
        bool isRunningRule)
        : QDialog(parent)
        , m_startingPath(startingPath)
        , m_fileExtension(fileExtension)
    {
        m_ui.reset(new Ui::NewFileDialog);
        m_ui->setupUi(this);

        setWindowTitle(dialogTitle);

        // Set up File Name section
        m_ui->fileNameLineEdit->setEnabled(false);
        connect(m_ui->browseButton, &QPushButton::clicked, this, &NewFileDialog::OnBrowseButtonPressed);

        m_newFileDialog.setFileMode(QFileDialog::AnyFile);
        m_newFileDialog.setNameFilter(fileNameFilter);
        m_newFileDialog.setViewMode(QFileDialog::Detail);
        m_newFileDialog.setDirectory(m_startingPath);
        // We are not creating a new file when Qt thinks we are, so we need to block signals or else the file watcher will be
        // triggered too soon
        m_newFileDialog.blockSignals(true);

        // Set up Platform selection
        QString disabledPatformMessageOverride;
        if (isRunningRule)
        {
            disabledPatformMessageOverride = tr("This platform is not valid for all input Asset Lists.");
        }
        m_ui->platformSelectionWidget->Init(enabledPlatforms, disabledPatformMessageOverride);
        connect(m_ui->platformSelectionWidget,
            &PlatformSelectionWidget::PlatformsSelected,
            this,
            &NewFileDialog::OnPlatformSelectionChanged);

        // Set up Cancel and Create New File buttons
        m_ui->createFileButton->setEnabled(false);
        connect(m_ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_ui->createFileButton, &QPushButton::clicked, this, &NewFileDialog::OnCreateFileButtonPressed);
    }

    AZStd::string NewFileDialog::GetAbsoluteFilePath()
    {
        return m_absoluteFilePath;
    }

    AzFramework::PlatformFlags NewFileDialog::GetPlatformFlags()
    {
        return m_ui->platformSelectionWidget->GetSelectedPlatforms();
    }

    void NewFileDialog::OnBrowseButtonPressed()
    {
        int result = m_newFileDialog.exec();
        if (result == QDialog::DialogCode::Accepted)
        {
            m_absoluteFilePath = m_newFileDialog.selectedFiles()[0].toUtf8().data();
            AzToolsFramework::RemovePlatformIdentifier(m_absoluteFilePath);

            if (m_fileExtension && !AzFramework::StringFunc::Path::HasExtension(m_absoluteFilePath.c_str()))
            {
                m_absoluteFilePath.append(".");
                m_absoluteFilePath.append(m_fileExtension);
            }

            m_ui->fileNameLineEdit->setEnabled(true);
            m_ui->fileNameLineEdit->setText(m_absoluteFilePath.c_str());
        }

        m_fileNameIsValid = !m_absoluteFilePath.empty();
        m_ui->createFileButton->setEnabled(m_platformIsValid && m_fileNameIsValid);
    }

    void NewFileDialog::OnPlatformSelectionChanged(const AzFramework::PlatformFlags& selectedPlatforms)
    {
        // Hide the "Create File" button if no platforms are selected
        m_platformIsValid = selectedPlatforms != AzFramework::PlatformFlags::Platform_NONE;
        m_ui->createFileButton->setEnabled(m_platformIsValid && m_fileNameIsValid);
    }

    void NewFileDialog::OnCreateFileButtonPressed()
    {
        // Check to see if any of the selected platform-specific files already exist on-disk
        QString overwriteExistingFilesList;
        AZStd::fixed_vector<AZStd::string, AzFramework::NumPlatforms> selectedPlatformNames{ AZStd::from_range,
            AzFramework::PlatformHelper::GetPlatforms(GetPlatformFlags()) };
        for (const AZStd::string& platformName : selectedPlatformNames)
        {
            FilePath platformSpecificFilePath(GetAbsoluteFilePath(), platformName);
            const char* platformSpecificFileAbsolutePath = platformSpecificFilePath.AbsolutePath().c_str();
            if (AZ::IO::FileIOBase::GetInstance()->Exists(platformSpecificFileAbsolutePath))
            {
                overwriteExistingFilesList.append(QString("%1\n").arg(platformSpecificFileAbsolutePath));
            }
        }

        // Ask the user if they are sure they want to overwrite existing files
        if (!overwriteExistingFilesList.isEmpty())
        {
            QString messageBoxText = QString(tr(
                "The following files already exist on-disk. Generating new files will overwrite the existing ones.\n\n%1\n\nDo you wish to permanently delete the existing files?")).arg(overwriteExistingFilesList);

            QMessageBox::StandardButton confirmDeleteFileResult =
                QMessageBox::question(this, QString(tr("Replace Existing Files")), messageBoxText);
            if (confirmDeleteFileResult != QMessageBox::StandardButton::Yes)
            {
                // User canceled out of the operation
                return;
            }
        }

        emit QDialog::accept();
    }

    AZStd::string NewFileDialog::OSNewFileDialog(
        QWidget* parent,
        const char* fileExtension,
        const char* fileTypeDisplayName,
        const AZStd::string& startingDirectory)
    {
        QFileDialog filePathDialog(parent);
        filePathDialog.setFileMode(QFileDialog::AnyFile);
        filePathDialog.setNameFilter(QString(tr("%1 (*.%2)")).arg(fileTypeDisplayName).arg(fileExtension));
        filePathDialog.setViewMode(QFileDialog::Detail);
        filePathDialog.setDirectory(startingDirectory.c_str());

        // Since we handle file creation instead of the OS, we have to block signals or else the model will be reloaded,
        //    and our in-memory changes will be lost.
        filePathDialog.blockSignals(true);
        int result = filePathDialog.exec();
        if (result == QDialog::DialogCode::Rejected || filePathDialog.selectedFiles().empty())
        {
            // User canceled out of the file dialog, cancel operation
            return "";
        }

        AZStd::string absoluteFilePath(filePathDialog.selectedFiles()[0].toUtf8().data());
        if (!AzFramework::StringFunc::Path::HasExtension(absoluteFilePath.c_str()))
        {
            absoluteFilePath =
                AZStd::string::format("%s%c%s", absoluteFilePath.c_str(), AZ_FILESYSTEM_EXTENSION_SEPARATOR, fileExtension);
        }

        return absoluteFilePath;
    }

    int NewFileDialog::FileGenerationResultMessageBox(
        QWidget* parent,
        const AZStd::vector<AZStd::string>& generatedFiles,
        bool generatedWithErrors)
    {
        QMessageBox messageBox(parent);
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.setDefaultButton(QMessageBox::Ok);

        if (generatedFiles.empty())
        {
            messageBox.setText(QString("No files were generated. Please refer to the console for more information."));
            messageBox.setIcon(QMessageBox::Critical);
            return messageBox.exec();
        }

        QString messageText;
        if (generatedWithErrors)
        {
            messageText = QString("The following files were generated with errors. Please refer to the console for more information.\n\n");
            messageBox.setIcon(QMessageBox::Warning);
        }
        else
        {
            messageText = QString("You have successfully generated:\n\n");
            messageBox.setIcon(QMessageBox::NoIcon);
        }

        AZStd::string fileName;
        for (const AZStd::string& filePath : generatedFiles)
        {
            AzFramework::StringFunc::Path::GetFullFileName(filePath.c_str(), fileName);
            messageText.append(QString("%1\n").arg(fileName.c_str()));
        }

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(fileName.c_str(), extension, false);
        if (extension == AzToolsFramework::AssetSeedManager::GetAssetListFileExtension())
        {
            messageText.append("\nVisit the Asset Lists tab to see the lists.");
        }
        else if (extension == AzToolsFramework::AssetBundleSettings::GetBundleFileExtension())
        {
            messageText.append("\nVisit the Completed Bundles tab to see the bundles.");
        }

        messageBox.setText(messageText);

        // QMessageBoxes try to shrink to the smallest size possible, so we need to make a spacer
        QSpacerItem* horizontalSpacer = new QSpacerItem(550, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        QGridLayout* layout = static_cast<QGridLayout*>(messageBox.layout());
        layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());

        return messageBox.exec();
    }

} //namespace AssetBundler

#include <source/ui/moc_NewFileDialog.cpp>
