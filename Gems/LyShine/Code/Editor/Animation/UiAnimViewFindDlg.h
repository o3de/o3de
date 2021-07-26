/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

// UiAnimViewFindDlg.h : header file
//
#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>
#endif

class CUiAnimViewDialog;

namespace Ui {
    class UiAnimViewFindDlg;
}

/////////////////////////////////////////////////////////////////////////////
// CUiAnimViewFindDlg dialog
class CUiAnimViewFindDlg
    : public QDialog
{
    Q_OBJECT
    // Construction
public:
    CUiAnimViewFindDlg(const char* title = NULL, QWidget* pParent = NULL);   // standard constructor
    ~CUiAnimViewFindDlg();

    //Functions
    void FillData();
    void FillList();
    void Init(CUiAnimViewDialog* tvDlg);
    void ProcessSel();

protected slots:
    void OnOK();
    void OnCancel();
    void OnFilterChange(const QString& text);
    void OnItemDoubleClicked();

protected:
    struct ObjName
    {
        QString m_objName;
        QString m_directorName;
        QString m_seqName;
    };

    std::vector<ObjName> m_objs;
    CUiAnimViewDialog* m_tvDlg;

    int m_numSeqs;

    QScopedPointer<Ui::UiAnimViewFindDlg> ui;
};

