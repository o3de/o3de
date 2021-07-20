/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "StringDlg.h"

// Qt
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>


/////////////////////////////////////////////////////////////////////////////
// StringDlg dialog

StringDlg::StringDlg(const QString &title, QWidget* pParent, bool bFileNameLimitation)
    : QInputDialog(pParent)
    , m_bFileNameLimitation(bFileNameLimitation)
{
    setWindowTitle(title);
    setLabelText("");
}

void StringDlg::accept()
{
    if (m_bFileNameLimitation)
    {
        const QString text = textValue();
        const QString reservedCharacters("<>:\"/\\|?*}");
        foreach(auto reserv, reservedCharacters)
        {
            if (text.contains(reserv))
            {
                QMessageBox::warning(this, tr("Warning"), tr(" This string can't contain the following characters: %1").arg(reserv), QMessageBox::Ok);
                return;
            }
        }
    }
    if (m_Check && !m_Check(textValue()))
        return;

    QInputDialog::accept();
}


//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CMultiLineStringDlg dialog
StringGroupDlg::StringGroupDlg(const QString &title, QWidget *parent)
    : QDialog(parent)
{
    if (!title.isEmpty())
        setWindowTitle(title);

    m_group = new QLineEdit(this);
    m_string = new QLineEdit(this);

    QFrame *horLine = new QFrame(this);
    horLine->setFrameStyle(QFrame::HLine | QFrame::Plain);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Group"), this));
    layout->addWidget(m_group);
    layout->addWidget(new QLabel(tr("Name"), this));
    layout->addWidget(m_string);
    layout->addWidget(horLine);
    layout->addWidget(buttonBox);
}

void StringGroupDlg::SetString(const QString &str)
{
    m_string->setText(str);
}

QString StringGroupDlg::GetString() const
{
    return m_string->text();
}

void StringGroupDlg::SetGroup(const QString &str)
{
    m_group->setText(str);
}

QString StringGroupDlg::GetGroup() const
{
    return m_group->text();
}
