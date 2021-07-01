/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ProcessingAssetsDialog.h"

// Qt
#include <QPushButton>
#include <QStyle>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AssetImporter/UI/ui_ProcessingAssetsDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

ProcessingAssetsDialog::ProcessingAssetsDialog(int numberOfProcessedFiles, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ProcessingAssetsDialog)
{
    m_ui->setupUi(this);
    UpdateTextsAndTitle(numberOfProcessedFiles);
    InitializeButtons();
}

ProcessingAssetsDialog::~ProcessingAssetsDialog()
{
}

void ProcessingAssetsDialog::InitializeButtons()
{
    QPushButton* viewStatusButton = m_ui->buttonBox->addButton(tr("View status"), QDialogButtonBox::AcceptRole);
    QPushButton* closeButton = m_ui->buttonBox->addButton(tr("Close"), QDialogButtonBox::RejectRole);

    viewStatusButton->setDefault(true);
    viewStatusButton->setProperty("class", "AssetImporterLargerButton");
    viewStatusButton->style()->unpolish(viewStatusButton);
    viewStatusButton->style()->polish(viewStatusButton);
    viewStatusButton->update();

    closeButton->setProperty("class", "AssetImporterButton");
    closeButton->style()->unpolish(closeButton);
    closeButton->style()->polish(closeButton);
    closeButton->update();

    connect(viewStatusButton, &QPushButton::clicked, this, &ProcessingAssetsDialog::accept);
    connect(closeButton, &QPushButton::clicked, this, &ProcessingAssetsDialog::reject); 
}

void ProcessingAssetsDialog::accept()
{
    Q_EMIT OpenLogDialog();
    QDialog::accept();
}

void ProcessingAssetsDialog::reject()
{
    Q_EMIT CloseProcessingAssetsDialog();
    QDialog::reject();
}

void ProcessingAssetsDialog::UpdateTextsAndTitle(int numberOfProcessedFiles)
{
    if (numberOfProcessedFiles > 1)
    {
        setWindowTitle("Processing assets");
        m_ui->label->setText("The Asset Processor will process your assets and when they are finished they will appear in the Asset Browser. You can view the status of your assets in the Asset Processor.");
    }
    else
    {
        setWindowTitle("Processing asset");
        m_ui->label->setText("The Asset Processor will process your asset and when it is finished it will appear in the Asset Browser. You can view the status of your asset in the Asset Processor.");
    }   
}

#include <AssetImporter/UI/moc_ProcessingAssetsDialog.cpp>
