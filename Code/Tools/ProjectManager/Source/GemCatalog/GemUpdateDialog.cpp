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
    GemUpdateDialog::GemUpdateDialog(const QString& gemName, QWidget* parent)
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
        QLabel* subTitleLabel = new QLabel(tr("Update to the latest version of %1?").arg(gemName));
        subTitleLabel->setObjectName("gemCatalogDialogSubTitle");
        layout->addWidget(subTitleLabel);

        layout->addSpacing(10);

        QLabel* bodyLabel = new QLabel(tr("The latest version of this Gem may not be compatible with your engine. "
                                          "Updating this Gem will remove any local changes made to this Gem, "
                                          "and may remove old features that are in use."));
        bodyLabel->setWordWrap(true);
        bodyLabel->setFixedWidth(440);
        layout->addWidget(bodyLabel);

        layout->addSpacing(60);

        // Buttons
        QDialogButtonBox* dialogButtons = new QDialogButtonBox();
        dialogButtons->setObjectName("footer");
        layout->addWidget(dialogButtons);

        QPushButton* cancelButton = dialogButtons->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
        cancelButton->setProperty("secondary", true);
        QPushButton* updateButton = dialogButtons->addButton(tr("Update Gem"), QDialogButtonBox::ApplyRole);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(updateButton, &QPushButton::clicked, this, &QDialog::accept);
    }
} // namespace O3DE::ProjectManager
