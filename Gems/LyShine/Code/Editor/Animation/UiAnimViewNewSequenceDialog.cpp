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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "UiCanvasEditor_precompiled.h"
#include "UiAnimViewNewSequenceDialog.h"
#include "Animation/UiAnimViewSequenceManager.h"
#include <Editor/Animation/ui_UiAnimViewNewSequenceDialog.h>
#include <QMessageBox>
#include "QtUtil.h"


// UiAnimViewNewSequenceDialog dialog
CUiAVNewSequenceDialog::CUiAVNewSequenceDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::CUiAVNewSequenceDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CUiAVNewSequenceDialog::OnOK);
    setWindowTitle("Add New Sequence");
}

CUiAVNewSequenceDialog::~CUiAVNewSequenceDialog()
{
}

void CUiAVNewSequenceDialog::OnOK()
{
    m_sequenceName = ui->NAME->text();

    if (m_sequenceName.isEmpty())
    {
        QMessageBox::warning(this, "New Sequence", "A sequence name cannot be empty!");
        return;
    }
    else if (m_sequenceName.contains('/'))
    {
        QMessageBox::warning(this, "New Sequence", "A sequence name cannot contain a '/' character!");
        return;
    }

    for (int k = 0; k < CUiAnimViewSequenceManager::GetSequenceManager()->GetCount(); ++k)
    {
        CUiAnimViewSequence* pSequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetSequenceByIndex(k);
        QString fullname = QtUtil::ToQString(pSequence->GetName());

        if (fullname.compare(m_sequenceName, Qt::CaseInsensitive) == 0)
        {
            QMessageBox::warning(this, "New Sequence", "Sequence with this name already exists!");
            return;
        }
    }

    accept();
}

#include <Animation/moc_UiAnimViewNewSequenceDialog.cpp>
