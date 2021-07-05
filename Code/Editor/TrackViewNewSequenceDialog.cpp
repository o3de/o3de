/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewNewSequenceDialog.h"

// Qt
#include <QMessageBox>

// CryCommon
#include <CryCommon/Maestro/Types/SequenceType.h>

// Editor
#include "TrackView/TrackViewSequenceManager.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_TrackViewNewSequenceDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

namespace
{
    struct seqTypeComboPair
    {
        const char*    name;
        SequenceType   type;
    };
}

// TrackViewNewSequenceDialog dialog
CTVNewSequenceDialog::CTVNewSequenceDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::CTVNewSequenceDialog)
    , m_inputFocusSet(false)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CTVNewSequenceDialog::OnOK);
    connect(ui->NAME, &QLineEdit::returnPressed, this, &CTVNewSequenceDialog::OnOK);
    setWindowTitle("Add New Sequence");

    OnInitDialog();
}

CTVNewSequenceDialog::~CTVNewSequenceDialog()
{
}

void CTVNewSequenceDialog::OnInitDialog()
{
}

void CTVNewSequenceDialog::OnOK()
{
    m_sequenceType = SequenceType::SequenceComponent;

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
    else if (m_sequenceName == LIGHT_ANIMATION_SET_NAME)
    {
        QMessageBox::warning(this, "New Sequence", QString("The sequence name '%1' is reserved.\n\nPlease choose a different name.").arg(LIGHT_ANIMATION_SET_NAME));
        return;
    }

    for (int k = 0; k < GetIEditor()->GetSequenceManager()->GetCount(); ++k)
    {
        CTrackViewSequence* pSequence = GetIEditor()->GetSequenceManager()->GetSequenceByIndex(k);
        QString fullname = QtUtil::ToQString(pSequence->GetName());

        if (fullname.compare(m_sequenceName, Qt::CaseInsensitive) == 0)
        {
            QMessageBox::warning(this, "New Sequence", "Sequence with this name already exists!");
            return;
        }
    }

    accept();
}

void CTVNewSequenceDialog::showEvent(QShowEvent* event)
{
    if (!m_inputFocusSet)
    {
        ui->NAME->setFocus();
        m_inputFocusSet = true;
    }

    QDialog::showEvent(event);
}

#include <moc_TrackViewNewSequenceDialog.cpp>
