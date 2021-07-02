/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "FilesAlreadyExistDialog.h"

// Qt
#include <QPushButton>
#include <QStyle>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AssetImporter/UI/ui_FilesAlreadyExistDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

FilesAlreadyExistDialog::FilesAlreadyExistDialog(QString message ,int numberOfFiles, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::FilesAlreadyExistDialog)
{
    m_ui->setupUi(this);

    UpdateMessage(message);
    InitializeButtons();
    UpdateCheckBoxState(numberOfFiles);
}

FilesAlreadyExistDialog::~FilesAlreadyExistDialog()
{
}

void FilesAlreadyExistDialog::InitializeButtons()
{
    m_ui->buttonBox->setContentsMargins(0, 0, 16, 16);
    QPushButton* overwriteButton = m_ui->buttonBox->addButton(tr("Overwrite"), QDialogButtonBox::AcceptRole);
    QPushButton* keepBothButton = m_ui->buttonBox->addButton(tr("Keep Both"), QDialogButtonBox::AcceptRole);
    QPushButton* skipButton = m_ui->buttonBox->addButton(tr("Skip"), QDialogButtonBox::AcceptRole);

    overwriteButton->setProperty("class", "Primary");
    overwriteButton->setDefault(true);

    keepBothButton->setProperty("class", "AssetImporterLargerButton");
    keepBothButton->style()->unpolish(keepBothButton);
    keepBothButton->style()->polish(keepBothButton);
    keepBothButton->update();

    skipButton->setProperty("class", "AssetImporterButton");
    skipButton->style()->unpolish(skipButton);
    skipButton->style()->polish(skipButton);
    skipButton->update();

    connect(overwriteButton, &QPushButton::clicked, this, &FilesAlreadyExistDialog::DoOverwrite);
    connect(keepBothButton, &QPushButton::clicked, this, &FilesAlreadyExistDialog::DoKeepBoth);
    connect(skipButton, &QPushButton::clicked, this, &FilesAlreadyExistDialog::DoSkipCurrentProcess);
}

void FilesAlreadyExistDialog::UpdateMessage(QString message)
{
    m_ui->message->setText(message);
}

void FilesAlreadyExistDialog::DoSkipCurrentProcess()
{
    QDialog::accept();
    Q_EMIT SkipCurrentProcess();
}

void FilesAlreadyExistDialog::DoOverwrite()
{
    QDialog::accept();
    Q_EMIT OverWriteFiles();
}

void FilesAlreadyExistDialog::DoKeepBoth()
{
    QDialog::accept();
    Q_EMIT KeepBothFiles();
}

void FilesAlreadyExistDialog::DoApplyActionToAllFiles()
{
    Q_EMIT ApplyActionToAllFiles(m_ui->applyToAllCheckBox->isChecked());
}

void FilesAlreadyExistDialog::UpdateCheckBoxState(int numberOfFiles)
{
    m_ui->applyToAllCheckBox->setVisible((numberOfFiles > 1));
    connect(m_ui->applyToAllCheckBox, &QCheckBox::stateChanged, this, &FilesAlreadyExistDialog::DoApplyActionToAllFiles);
}

void FilesAlreadyExistDialog::closeEvent([[maybe_unused]] QCloseEvent* ev)
{
    QDialog::reject();
    Q_EMIT CancelAllProcesses();
}

#include <AssetImporter/UI/moc_FilesAlreadyExistDialog.cpp>
