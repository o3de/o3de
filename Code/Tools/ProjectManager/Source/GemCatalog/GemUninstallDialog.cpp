/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemUninstallDialog.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVariant>

namespace O3DE::ProjectManager
{
    GemUninstallDialog::GemUninstallDialog(const QString& gemName, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Uninstall Remote Gem"));
        setObjectName("GemUninstallDialog");
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(true);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(30);
        layout->setAlignment(Qt::AlignTop);
        setLayout(layout);

        // Body
        QLabel* subTitleLabel = new QLabel(tr("Are you sure you want to uninstall %1?").arg(gemName));
        subTitleLabel->setObjectName("dialogSubTitle");
        layout->addWidget(subTitleLabel);

        layout->addSpacing(10);

        QLabel* bodyLabel = new QLabel(tr("The Gem and its related files will be uninstalled. This does not affect the Gem's repository. "
                                          "You can re-install this Gem from the Catalog, but its contents may be subject to change."));
        bodyLabel->setWordWrap(true);
        bodyLabel->setFixedSize(QSize(440, 80));
        layout->addWidget(bodyLabel);

        layout->addSpacing(40);

        // Buttons
        QDialogButtonBox* dialogButtons = new QDialogButtonBox();
        dialogButtons->setObjectName("footer");
        layout->addWidget(dialogButtons);

        QPushButton* cancelButton = dialogButtons->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
        cancelButton->setProperty("secondary", true);
        QPushButton* uninstallButton = dialogButtons->addButton(tr("Uninstall Gem"), QDialogButtonBox::ApplyRole);
        uninstallButton->setProperty("danger", true);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(uninstallButton, &QPushButton::clicked, this, &QDialog::accept);
    }
} // namespace O3DE::ProjectManager
