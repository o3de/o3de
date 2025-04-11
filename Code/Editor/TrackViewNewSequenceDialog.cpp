/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewNewSequenceDialog.h"

// Qt
#include <QValidator>
#include <QPushButton>

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

class CTVNewSequenceDialogValidator : public QValidator
{
    public:
        CTVNewSequenceDialogValidator(QObject* parent)
            : QValidator(parent)
        {
            m_parentDialog = qobject_cast<CTVNewSequenceDialog*>(parent);
        }

        QValidator::State validate(QString& input, [[maybe_unused]] int& pos) const override
        {
            constexpr int MaxInputLength = 160;

            SetEnabled(true);
            SetToolTip("");

            if (input.isEmpty())
            {
                return QValidator::Acceptable; // Allow further editing
            }

            if (input.contains('/'))
            {
                SetToolTip("A sequence name cannot contain a '/' character");
                return QValidator::Invalid; // Undo this change
            }

            if (input.length() > MaxInputLength)
            {
                SetToolTip(QString("A sequence name cannot exceed %1 characters").arg(MaxInputLength).toStdString().c_str());
                return QValidator::Invalid; // Undo this change
            }

            bool isValid = true;
            if (input == LIGHT_ANIMATION_SET_NAME)
            {
                SetToolTip("The sequence name " LIGHT_ANIMATION_SET_NAME " is reserved.\nPlease choose a different name");
                isValid = false;
            }
            else
            {
                for (unsigned int k = 0; k < GetIEditor()->GetSequenceManager()->GetCount(); ++k)
                {
                    CTrackViewSequence* pSequence = GetIEditor()->GetSequenceManager()->GetSequenceByIndex(k);
                    const QString fullname = QString::fromUtf8(pSequence->GetName().c_str());

                    if (fullname.compare(input, Qt::CaseInsensitive) == 0)
                    {
                        isValid = false;
                        SetToolTip("Sequence with this name already exists");
                        break;
                    }
                }
            }

            SetEnabled(isValid);  // Disable OK button if input is invalid.
            return QValidator::Acceptable; // Accept the change and allow further name editing even if input is invalid.
        }

    private:

        void SetEnabled(bool enable) const
        {
            m_parentDialog->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
        }

        void SetToolTip(const char* toolTipText) const
        {
            m_parentDialog->ui->NAME->setToolTip(toolTipText);
        }

        const CTVNewSequenceDialog* m_parentDialog;
};

// TrackViewNewSequenceDialog dialog
CTVNewSequenceDialog::CTVNewSequenceDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::CTVNewSequenceDialog)
    , m_inputFocusSet(false)
    , m_validator(new CTVNewSequenceDialogValidator(this))
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CTVNewSequenceDialog::OnOK);
    connect(ui->NAME, &QLineEdit::returnPressed, this, &CTVNewSequenceDialog::OnOK);
    ui->NAME->setValidator(m_validator);
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
