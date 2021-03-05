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

// Description : Modeless dialog for list of errors.
//               To avoid interuption on start Editor time and on load level time.
//               Using: To add messages from any part of CryEngine you can use this style:
//               gEnv->pSystem->ShowMessage("Text", "Caption", MB_OK);


#ifndef CRYINCLUDE_EDITOR_DIALOGS_ERRORSDLG_H
#define CRYINCLUDE_EDITOR_DIALOGS_ERRORSDLG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui {
    class CErrorsDlg;
}

class CErrorsDlg
    : public QDialog
{
    Q_OBJECT

public:
    CErrorsDlg(QWidget* pParent = nullptr);
    virtual ~CErrorsDlg();
    void AddMessage(const QString& text, const QString& caption);

protected:
    void OnCancel();
    void OnInitDialog();

    void OnCopyErrors();
    void OnClearErrors();

private:
    bool m_bFirstMessage;
    QScopedPointer<Ui::CErrorsDlg> ui;
};

#endif // CRYINCLUDE_EDITOR_DIALOGS_ERRORSDLG_H
