/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
