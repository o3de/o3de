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

#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWFINDDLG_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWFINDDLG_H
#pragma once

// TrackViewFindDlg.h : header file
//
#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>
#endif

class CTrackViewDialog;

namespace Ui {
    class TrackViewFindDlg;
}

/////////////////////////////////////////////////////////////////////////////
// CTrackViewFindDlg dialog
class CTrackViewFindDlg
    : public QDialog
{
    Q_OBJECT
    // Construction
public:
    CTrackViewFindDlg(const char* title = NULL, QWidget* pParent = NULL);   // standard constructor
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

    std::vector<ObjName> m_objs;
    CTrackViewDialog* m_tvDlg;

    int m_numSeqs;

    QScopedPointer<Ui::TrackViewFindDlg> ui;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWFINDDLG_H
