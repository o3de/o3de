/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ErrorsDlg.h"

// Qt
#include <QClipboard>
#include <QStyle>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <Dialogs/ui_ErrorsDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


CErrorsDlg::CErrorsDlg(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::CErrorsDlg)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_bFirstMessage = true;

    OnInitDialog();

    connect(ui->m_buttonCopyErrors, &QPushButton::clicked, this, &CErrorsDlg::OnCopyErrors);
    connect(ui->m_buttonClearErrors, &QPushButton::clicked, this, &CErrorsDlg::OnClearErrors);
    connect(ui->m_buttonCancel, &QPushButton::clicked, this, &CErrorsDlg::OnCancel);
}

CErrorsDlg::~CErrorsDlg()
{
}

void CErrorsDlg::OnInitDialog()
{
    auto icon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    
    ui->m_errorIconCtrl->setPixmap(icon.pixmap(ui->m_errorIconCtrl->width()));
}

void CErrorsDlg::AddMessage(const QString& text, const QString& caption)
{
    // At the load time this dialog is frozen, cause there is no message loop in progress.
    // We need to dispatch messages before showing a window if it was closed by user.
    if (!isVisible())
    {
        show();
    }

    ui->m_richEdit->moveCursor(QTextCursor::End);
    auto textCursor = ui->m_richEdit->textCursor();

    if (m_bFirstMessage)
    {
        m_bFirstMessage = false;
    }
    else
    {
        textCursor.insertText("\n\n");
    }

    QTextCharFormat format;
    format.setFontWeight(QFont::Bold);

    textCursor.setCharFormat(format);
    textCursor.insertText(caption + "\n");

    format.setFontWeight(QFont::Normal);
    // Show message in a dialog
    textCursor.setCharFormat(format);
    textCursor.insertText(text);
}

void CErrorsDlg::OnCancel()
{
    hide();
}


void CErrorsDlg::OnCopyErrors()
{
    QString text = ui->m_richEdit->toPlainText();
    QApplication::clipboard()->setText(text);
}


void CErrorsDlg::OnClearErrors()
{
    m_bFirstMessage = true;
    ui->m_richEdit->clear();
}

#include <Dialogs/moc_ErrorsDlg.cpp>
