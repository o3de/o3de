/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_TEXTEDITORCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_TEXTEDITORCTRL_H
#pragma once

// CTextEditorCtrl
#if !defined(Q_MOC_RUN)
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
    bool m_bModified;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_TEXTEDITORCTRL_H
