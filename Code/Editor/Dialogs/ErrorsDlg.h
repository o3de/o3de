/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
