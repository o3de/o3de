/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ExternalLinkDialog.h>
#include <LinkWidget.h>
#include <ProjectManagerSettings.h>

#include <AzCore/Settings/SettingsRegistry.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVariant>
#include <QIcon>
#include <QCheckBox>

namespace O3DE::ProjectManager
{
    ExternalLinkDialog::ExternalLinkDialog(const QUrl& url, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Leaving O3DE"));
        setObjectName("ExternalLinkDialog");
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(true);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(30);
        hLayout->setAlignment(Qt::AlignTop);
        setLayout(hLayout);

        QVBoxLayout* warningLayout = new QVBoxLayout();
        warningLayout->setMargin(0);
        warningLayout->setAlignment(Qt::AlignTop);
        hLayout->addLayout(warningLayout);

        QLabel* warningIcon = new QLabel(this);
        warningIcon->setPixmap(QIcon(":/Warning.svg").pixmap(32, 32));
        warningLayout->addWidget(warningIcon);

        warningLayout->addStretch();

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setAlignment(Qt::AlignTop);
        hLayout->addLayout(layout);

        // Body
        QLabel* subTitleLabel = new QLabel(tr("You are about to leave O3DE Project Manager to visit an external link."));
        subTitleLabel->setObjectName("dialogSubTitle");
        layout->addWidget(subTitleLabel);

        layout->addSpacing(10);

        QLabel* bodyLabel = new QLabel(tr("If you trust this source, you can proceed to this link, or click \"Cancel\" to return."));
        layout->addWidget(bodyLabel);

        // Don't actually set linkUrl we are just using LinkLabel superficially here
        LinkLabel* linkLabel = new LinkLabel(url.toString(), {}, 12);
        layout->addWidget(linkLabel);

        layout->addSpacing(40);

        QCheckBox* skipDialogCheckbox = new QCheckBox(tr("Do not show this again"));
        layout->addWidget(skipDialogCheckbox);
        connect(skipDialogCheckbox, &QCheckBox::stateChanged, this, &ExternalLinkDialog::SetSkipDialogSetting);

        // Buttons
        QDialogButtonBox* dialogButtons = new QDialogButtonBox();
        dialogButtons->setObjectName("footer");
        layout->addWidget(dialogButtons);

        QPushButton* cancelButton = dialogButtons->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
        cancelButton->setProperty("secondary", true);
        QPushButton* acceptButton = dialogButtons->addButton(tr("Proceed"), QDialogButtonBox::ApplyRole);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(acceptButton, &QPushButton::clicked, this, &QDialog::accept);
    }

    void ExternalLinkDialog::SetSkipDialogSetting(bool state)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            QString settingsKey = GetExternalLinkWarningKey();
            settingsRegistry->Set(settingsKey.toStdString().c_str(), state);
            SaveProjectManagerSettings();
        }
    }
} // namespace O3DE::ProjectManager
