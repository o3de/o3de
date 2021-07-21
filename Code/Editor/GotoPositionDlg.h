/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_GOTOPOSITIONDLG_H
#define CRYINCLUDE_EDITOR_GOTOPOSITIONDLG_H

#pragma once
// GotoPositionDlg.h : header file
//

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class GotoPositionDlg;
}

/////////////////////////////////////////////////////////////////////////////
// CGotoPositionDlg dialog

class CGotoPositionDlg
    : public QDialog
{
    Q_OBJECT
    // Construction
public:
    CGotoPositionDlg(QWidget* pParent = nullptr);   // standard constructor
    ~CGotoPositionDlg();

    // Implementation
protected:
    void OnInitDialog();
    void accept() override;

    void OnUpdateNumbers();
    void OnChangeEdit();


public:
    QString m_sPos;

private:
    QScopedPointer<Ui::GotoPositionDlg> m_ui;
};

#endif // CRYINCLUDE_EDITOR_GOTOPOSITIONDLG_H
