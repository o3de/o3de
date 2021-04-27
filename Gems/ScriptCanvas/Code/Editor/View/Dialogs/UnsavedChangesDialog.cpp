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

#include "precompiled.h"
#include "UnsavedChangesDialog.h"

#include <QPushButton>

#include "Editor/View/Dialogs/ui_UnsavedChangesDialog.h"

namespace ScriptCanvasEditor
{
    UnsavedChangesDialog::UnsavedChangesDialog(const QString& filename, QWidget* pParent /*=nullptr*/)
        : QDialog(pParent)
        , ui(new Ui::UnsavedChangesDialog)
    {
        ui->setupUi(this);

        ui->m_saveLabel->setText(filename);

        QObject::connect(ui->m_saveButton, &QPushButton::clicked, this, &UnsavedChangesDialog::OnSaveButton);
        QObject::connect(ui->m_continueButton, &QPushButton::clicked, this, &UnsavedChangesDialog::OnContinueWithoutSavingButton);
        QObject::connect(ui->m_cancelButton, &QPushButton::clicked, this, &UnsavedChangesDialog::OnCancelWithoutSavingButton);
    }

    void UnsavedChangesDialog::OnSaveButton(bool)
    {
        m_result = UnsavedChangesOptions::SAVE;
        accept();
    }

    void UnsavedChangesDialog::OnContinueWithoutSavingButton(bool)
    {
        m_result = UnsavedChangesOptions::CONTINUE_WITHOUT_SAVING;
        accept();
    }

    void UnsavedChangesDialog::OnCancelWithoutSavingButton([[maybe_unused]] bool clicked)
    {
        m_result = UnsavedChangesOptions::CANCEL_WITHOUT_SAVING;
        accept();
    }

#include <Editor/View/Dialogs/moc_UnsavedChangesDialog.cpp>
}
