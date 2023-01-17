/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "KinematicDescriptionDialog.h"

// Qt
#include <QPushButton>
#include <QStyle>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <Editor/ui_KinematicDescriptionDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

KinematicDescriptionDialog::KinematicDescriptionDialog(QString message ,[[maybe_unused]] int numberOfFiles, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::KinematicDescriptionDialog)
{
    m_ui->setupUi(this);

    UpdateMessage(message);
    InitializeButtons();
}

KinematicDescriptionDialog::~KinematicDescriptionDialog()
{
}

void KinematicDescriptionDialog::InitializeButtons()
{
    //m_ui->buttonBox->setContentsMargins(0, 0, 16, 16);
   /* QPushButton* overwriteButton = m_ui->buttonBox->addButton(tr("Overwrite"), QDialogButtonBox::AcceptRole);
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
    skipButton->update();*/

    /*connect(overwriteButton, &QPushButton::clicked, this, &KinematicDescriptionDialog::DoOverwrite);
    connect(keepBothButton, &QPushButton::clicked, this, &KinematicDescriptionDialog::DoKeepBoth);
    connect(skipButton, &QPushButton::clicked, this, &KinematicDescriptionDialog::DoSkipCurrentProcess);*/
}

void KinematicDescriptionDialog::UpdateMessage(QString message)
{
    //m_ui->message->setText(message);
}

//void PropertyBoolComboBoxCtrl::onChildComboBoxValueChange(int newValue)
//{
//    emit valueChanged(newValue == 0 ? false : true);
//}


#include <moc_KinematicDescriptionDialog.cpp>
