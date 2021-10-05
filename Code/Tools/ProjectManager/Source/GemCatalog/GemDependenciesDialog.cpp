/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemDependenciesDialog.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

namespace O3DE::ProjectManager
{
    GemDependenciesDialog::GemDependenciesDialog([[maybe_unused]] const QStringList& gems, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Unused Gem dependencies deactivated"));
        setObjectName("GemDependenciesDialog");
        setModal(true);

        QVBoxLayout* layout = new QVBoxLayout();
        setLayout(layout);

        QLabel* instructionLabel = new QLabel(tr("The following gem dependencies are no longer "
            "needed and will be deactivated. Select any gem dependencies you want to remain active."));
        layout->addWidget(instructionLabel);

        QDialogButtonBox* dialogButtons = new QDialogButtonBox();
        layout->addWidget(dialogButtons);

        QPushButton* cancelButton = dialogButtons->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
        //cancelButton->setProperty("secondary", true);
        QPushButton* continueButton = dialogButtons->addButton(tr("Continue"), QDialogButtonBox::ApplyRole);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(continueButton, &QPushButton::clicked, this, &QDialog::accept);
    }
} // namespace O3DE::ProjectManager
