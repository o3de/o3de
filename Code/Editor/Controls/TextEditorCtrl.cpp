/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TextEditorCtrl.h"


// CTextEditorCtrl
CTextEditorCtrl::CTextEditorCtrl(QWidget* pParent)
    : QTextEdit(pParent)
{
     m_bModified = true;

    QFont font;
    font.setFamily("Courier New");
    font.setFixedPitch(true);
    font.setPointSize(10);
    setFont(font);

    setLineWrapMode(NoWrap);

    connect(this, &QTextEdit::textChanged, this, &CTextEditorCtrl::OnChange);
}

CTextEditorCtrl::~CTextEditorCtrl()
{
}


// CTextEditorCtrl message handlers

void CTextEditorCtrl::LoadFile(const QString& sFileName)
{
    if (m_filename == sFileName)
    {
        return;
    }

    m_filename = sFileName;

    clear();

    CCryFile file(sFileName.toUtf8().data(), "rb");
    if (file.Open(sFileName.toUtf8().data(), "rb"))
    {
        size_t length = file.GetLength();

        QByteArray text;
        text.resize(length);
        file.ReadRaw(text.data(), length);

        setPlainText(text);
    }

    m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
void CTextEditorCtrl::SaveFile(const QString& sFileName)
{
    if (sFileName.isEmpty())
    {
        return;
    }

    if (!CFileUtil::OverwriteFile(sFileName.toUtf8().data()))
    {
        return;
    }

    QFile file(sFileName);
    file.open(QFile::WriteOnly);

    file.write(toPlainText().toUtf8());

    m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
void CTextEditorCtrl::OnChange()
{

    m_bModified = true;
}

#include <Controls/moc_TextEditorCtrl.cpp>
