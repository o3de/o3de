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
