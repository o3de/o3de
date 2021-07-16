/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/EditSeedDialog.h>
#include <source/ui/ui_EditSeedDialog.h>

#include <source/ui/PlatformSelectionWidget.h>

#include <QPushButton>

namespace AssetBundler
{

    EditSeedDialog::EditSeedDialog(
        QWidget* parent,
        const AzFramework::PlatformFlags& enabledPlatforms,
        const AzFramework::PlatformFlags& selectedPlatforms,
        const AzFramework::PlatformFlags& partiallySelectedPlatforms)
        : QDialog(parent)
    {
        m_ui.reset(new Ui::EditSeedDialog);
        m_ui->setupUi(this);

        m_ui->platformSelectionWidget->Init(enabledPlatforms);
        m_ui->platformSelectionWidget->SetSelectedPlatforms(selectedPlatforms, partiallySelectedPlatforms);
        connect(m_ui->platformSelectionWidget,
            &PlatformSelectionWidget::PlatformsSelected,
            this,
            &EditSeedDialog::OnPlatformSelectionChanged);

        // Set up confirm and cancel buttons
        connect(m_ui->applyChangesButton, &QPushButton::clicked, this, &QDialog::accept);
        connect(m_ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    AzFramework::PlatformFlags EditSeedDialog::GetPlatformFlags()
    {
        return m_ui->platformSelectionWidget->GetSelectedPlatforms();
    }

    AzFramework::PlatformFlags EditSeedDialog::GetPartiallySelectedPlatformFlags()
    {
        return m_ui->platformSelectionWidget->GetPartiallySelectedPlatforms();
    }
    
    void EditSeedDialog::OnPlatformSelectionChanged(
        const AzFramework::PlatformFlags& selectedPlatforms,
        const AzFramework::PlatformFlags& partiallySelectedPlatforms)
    {
        // Disable the "Apply Changes" button if no platforms are selected
        bool areAnyPlatformsSelected = selectedPlatforms != AzFramework::PlatformFlags::Platform_NONE ||
            partiallySelectedPlatforms != AzFramework::PlatformFlags::Platform_NONE;
        m_ui->applyChangesButton->setEnabled(areAnyPlatformsSelected);
    }

} // namespace AssetBundler

#include <source/ui/moc_EditSeedDialog.cpp>
