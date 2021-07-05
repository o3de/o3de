/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"
#include "OverwritePromptDialog.hxx"

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/UI/UICore/ui_OverwritePromptDialog.h>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    OverwritePromptDialog::OverwritePromptDialog(QWidget *pParent) 
        : QDialog(pParent)
    {
        m_result = false;
        guiConstructor = azcreate(Ui::OverwritePromptDialog,());
        AZ_Assert(pParent, "There must be a parent.");
        guiConstructor->setupUi(this);
    }

    void OverwritePromptDialog::UpdateLabel(QString label)
    {
        guiConstructor->label->setText(
            tr("<html><head/><body><p align=\"center\"><span style=\" font-weight:600;\">The file you want to save as already exists.<br>  Are you sure you want to overwrite '%1'?</span></p></body></html>")
                .arg(label));
    }

    OverwritePromptDialog::~OverwritePromptDialog()
    {
        azdestroy(guiConstructor);
    }

    void OverwritePromptDialog::on_OverwriteButton_clicked()
    {
        m_result = true;
        emit accept();
    }

    void OverwritePromptDialog::on_CancelButton_clicked()
    {
        m_result = false;
        emit accept();
    }

}

#include "UI/UICore/moc_OverwritePromptDialog.cpp"
