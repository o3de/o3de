/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/SettingsDialog/SettingsDialog.h>
#include <Window/SettingsDialog/SettingsWidget.h>

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace MaterialEditor
{
    SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Material Editor Settings");
        setFixedSize(600, 300);
        setLayout(new QVBoxLayout(this));

        auto settingsWidget = new SettingsWidget(this);
        settingsWidget->Populate();
        layout()->addWidget(settingsWidget);

        // Create the bottom row of the dialog with action buttons
        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        layout()->addWidget(buttonBox);

        QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setModal(true);
    }
} // namespace MaterialEditor

//#include <Window/SettingsDialog/moc_SettingsDialog.cpp>
