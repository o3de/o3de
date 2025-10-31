/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// TrackViewFindDlg.h : header file

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>
#endif

#include <AzCore/std/containers/vector.h>

class CTrackViewDialog;

namespace Ui {
    class TrackViewFindDlg;
}

//////////////////////////////////////////////////////////////////////////
// CTrackViewFindDlg dialog
class CTrackViewFindDlg
    : public QDialog
{
    Q_OBJECT
    // Construction
public:
    CTrackViewFindDlg(const char* title = nullptr, QWidget* pParent = nullptr);   // standard constructor
    ~CTrackViewFindDlg();

    //Functions
    void FillData();
    void FillList();
    void Init(CTrackViewDialog* tvDlg);
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

    AZStd::vector<ObjName> m_objs;
    AZStd::vector<int> m_objsSourceIndex;
    CTrackViewDialog* m_tvDlg;

    int m_numSeqs;

    QScopedPointer<Ui::TrackViewFindDlg> ui;
};
