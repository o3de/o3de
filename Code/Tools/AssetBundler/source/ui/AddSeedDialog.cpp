/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/AddSeedDialog.h>
#include <source/ui/ui_AddSeedDialog.h>

#include <source/ui/PlatformSelectionWidget.h>

#include <QFileDialog>
#include <QPushButton>

const char QtRelativePathPrefix[] = "../";

namespace AssetBundler
{
    AddSeedDialog::AddSeedDialog(
        QWidget* parent,
        const AzFramework::PlatformFlags& enabledPlatforms,
        const AZStd::string& platformSpecificCachePath)
        : QDialog(parent)
        , m_platformSpecificCachePath(platformSpecificCachePath.c_str())
    {
        m_ui.reset(new Ui::AddSeedDialog);
        m_ui->setupUi(this);

        // Set up Browse File button
        m_ui->fileNameLineEdit->setReadOnly(true);
        connect(m_ui->browseFileButton, &QPushButton::clicked, this, &AddSeedDialog::OnBrowseFileButtonPressed);

        // Set up Platform selection
        m_ui->platformSelectionWidget->Init(enabledPlatforms);
        connect(m_ui->platformSelectionWidget,
            &PlatformSelectionWidget::PlatformsSelected,
            this,
            &AddSeedDialog::OnPlatformSelectionChanged);

        // Set up Cancel and Create New File buttons
        m_ui->addSeedButton->setEnabled(false);
        connect(m_ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_ui->addSeedButton, &QPushButton::clicked, this, &QDialog::accept);
    }

    AZStd::string AddSeedDialog::GetFileName()
    {
        return m_fileName;
    }

    AzFramework::PlatformFlags AddSeedDialog::GetPlatformFlags()
    {
        return m_ui->platformSelectionWidget->GetSelectedPlatforms();
    }

    void AddSeedDialog::OnBrowseFileButtonPressed()
    {
        QString seedAbsolutePath = QFileDialog::getOpenFileName(this, tr("Add New Seed"), m_platformSpecificCachePath);
        m_fileNameIsValid = !seedAbsolutePath.isEmpty();
        if (!m_fileNameIsValid)
        {
            // The user canceled out of the window, do not update anything
            return;
        }

        // Make sure the path is relative to the platform-specific cache
        QDir platformSpecificCacheDir(m_platformSpecificCachePath);
        QString seedRelativePath = platformSpecificCacheDir.relativeFilePath(seedAbsolutePath);

        // trim off the "../" from the relative path
        FormatRelativePathString(seedRelativePath);
        m_fileName = seedRelativePath.toUtf8().data();

        // Update the ui
        m_ui->fileNameLineEdit->setText(seedRelativePath);
        m_ui->addSeedButton->setEnabled(m_platformIsValid && m_fileNameIsValid);
    }

    void AddSeedDialog::OnPlatformSelectionChanged(const AzFramework::PlatformFlags& selectedPlatforms)
    {
        // Hide the "Create File" button if no platforms are selected
        m_platformIsValid = selectedPlatforms != AzFramework::PlatformFlags::Platform_NONE;
        m_ui->addSeedButton->setEnabled(m_platformIsValid && m_fileNameIsValid);
    }

    void AddSeedDialog::FormatRelativePathString(QString& relativePath)
    {
        if (relativePath.startsWith(QtRelativePathPrefix))
        {
            relativePath.remove(0, static_cast<int>(strlen(QtRelativePathPrefix)));
        }
    }

} //namespace AssetBundler

#include <source/ui/moc_AddSeedDialog.cpp>
