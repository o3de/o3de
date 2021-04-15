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

#include "AzToolsFramework_precompiled.h"
#include "SaveChangesDialog.hxx"

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <AzToolsFramework/UI/UICore/ui_SaveChangesDialog.h>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    SaveChangesDialog::SaveChangesDialog(QWidget *pParent) 
        : QDialog(pParent)
    {
        m_result = SCDR_CancelOperation;
        guiConstructor = azcreate(Ui::SaveChangesDialog,());
        AZ_Assert(pParent, "There must be a parent.");
        guiConstructor->setupUi(this);
    }

    SaveChangesDialog::~SaveChangesDialog()
    {
        azdestroy(guiConstructor);
    }

    void SaveChangesDialog::OnSaveButton()
    {
        m_result = SCDR_Save;
        emit accept();
    }

    void SaveChangesDialog::OnCancelButton()
    {
        m_result = SCDR_CancelOperation;
        emit accept();
    }

    void SaveChangesDialog::OnContinueButton()
    {
        m_result = SCDR_DiscardAndContinue;
        emit accept();
    }
}

#include "UI/UICore/moc_SaveChangesDialog.cpp"
