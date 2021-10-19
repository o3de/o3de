/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoAddDialog.h>
#include <FormFolderBrowseEditWidget.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>

namespace O3DE::ProjectManager
{
    GemRepoAddDialog::GemRepoAddDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Add a User Repository"));
        setModal(true);
        setObjectName("addGemRepoDialog");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(30, 30, 25, 10);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        QLabel* instructionTitleLabel = new QLabel(tr("Enter a valid path to add a new user repository"));
        instructionTitleLabel->setObjectName("gemRepoAddDialogInstructionTitleLabel");
        instructionTitleLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(instructionTitleLabel);

        vLayout->addSpacing(10);

        QLabel* instructionContextLabel = new QLabel(tr("The path can be a Repository URL or a Local Path in your directory."));
        instructionContextLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(instructionContextLabel);

        m_repoPath = new FormFolderBrowseEditWidget(tr("Repository Path"), "", this);
        m_repoPath->setFixedWidth(600);
        vLayout->addWidget(m_repoPath);

        vLayout->addSpacing(40);

        QDialogButtonBox* dialogButtons = new QDialogButtonBox();
        dialogButtons->setObjectName("footer");
        vLayout->addWidget(dialogButtons);

        QPushButton* cancelButton = dialogButtons->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
        cancelButton->setProperty("secondary", true);
        QPushButton* applyButton = dialogButtons->addButton(tr("Add"), QDialogButtonBox::ApplyRole);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(applyButton, &QPushButton::clicked, this, &QDialog::accept);
    }

    QString GemRepoAddDialog::GetRepoPath()
    {
        return m_repoPath->lineEdit()->text();
    }
} // namespace O3DE::ProjectManager
