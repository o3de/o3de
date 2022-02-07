/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemUpdateDialog.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

namespace O3DE::ProjectManager
{
    GemUpdateDialog::GemUpdateDialog(const QString& gemName, bool updateAvaliable, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Update Remote Gem"));
        setObjectName("GemUpdateDialog");
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(true);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(30);
        layout->setAlignment(Qt::AlignTop);
        setLayout(layout);

        // Body
        QLabel* subTitleLabel = new QLabel(tr("%1 to the latest version of %2?").arg(
                                           updateAvaliable ? tr("Update") : tr("Force update"), gemName));
        subTitleLabel->setObjectName("gemCatalogDialogSubTitle");
        layout->addWidget(subTitleLabel);

        layout->addSpacing(10);

        QLabel* bodyLabel = new QLabel(tr("%1The latest version of this Gem may not be compatible with your engine. "
                                          "Updating this Gem will remove any local changes made to this Gem, "
                                          "and may remove old features that are in use.").arg(
                                              updateAvaliable ? "" : tr("No update detected for Gem. "
                                              "This will force a re-download of the gem. ")));
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
        QPushButton* updateButton =
            dialogButtons->addButton(tr("%1Update Gem").arg(updateAvaliable ? "" : tr("Force ")), QDialogButtonBox::ApplyRole);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(updateButton, &QPushButton::clicked, this, &QDialog::accept);
    }
} // namespace O3DE::ProjectManager
