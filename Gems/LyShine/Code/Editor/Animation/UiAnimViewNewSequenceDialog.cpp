/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

    for (unsigned int k = 0; k < CUiAnimViewSequenceManager::GetSequenceManager()->GetCount(); ++k)
    {
        CUiAnimViewSequence* pSequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetSequenceByIndex(k);
        QString fullname = QString::fromUtf8(pSequence->GetName().c_str());

        if (fullname.compare(m_sequenceName, Qt::CaseInsensitive) == 0)
        {
            QMessageBox::warning(this, "New Sequence", "Sequence with this name already exists!");
            return;
        }
    }

    accept();
}

#include <Animation/moc_UiAnimViewNewSequenceDialog.cpp>
