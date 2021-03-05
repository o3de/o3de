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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_TEXTEDITORCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_TEXTEDITORCTRL_H
#pragma once

// CTextEditorCtrl
#if !defined(Q_MOC_RUN)
#include "SyntaxColorizer.h"

#include <QTextEdit>
#endif

class CTextEditorCtrl
    : public QTextEdit
{
    Q_OBJECT

public:
    CTextEditorCtrl(QWidget* pParent = nullptr);
    virtual ~CTextEditorCtrl();

    void LoadFile(const QString& sFileName);
    void SaveFile(const QString& sFileName);
    QString GetFilename() const { return m_filename; }

    bool IsModified() const { return m_bModified; }

    //! Must be called after OnChange message.
    void OnChange();

protected:
    QString m_filename;
    CSyntaxColorizer m_sc;
    bool m_bModified;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_TEXTEDITORCTRL_H
