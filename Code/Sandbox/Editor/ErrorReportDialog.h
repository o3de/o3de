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

#ifndef CRYINCLUDE_EDITOR_ERRORREPORTDIALOG_H
#define CRYINCLUDE_EDITOR_ERRORREPORTDIALOG_H
#pragma once

// CErrorReportDialog dialog

#if !defined(Q_MOC_RUN)
#include "ErrorReport.h"

namespace Ui {
    class CErrorReportDialog;
}

class CErrorReportTableModel;

#include <QWidget>
#endif

class CErrorReportDialog
    : public QWidget
{
    Q_OBJECT
public:
    explicit CErrorReportDialog(QWidget* parent = nullptr);   // standard constructor
    virtual ~CErrorReportDialog();

    static void RegisterViewClass();

    static void Open(CErrorReport* pReport);
    static void Close();
    static void Clear();

    // you are required to implement this to satisfy the unregister/registerclass requirements on "AzToolsFramework::RegisterViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {ea523b7e-3f63-821b-4823-a131fc5b46a3}
        static const GUID guid =
        {
            0xea523b7e, 0x3f63, 0x821b, { 0x48, 0x23, 0xa1, 0x31, 0xfc, 0x5b, 0x46, 0xa3 }
        };
        return guid;
    }

    bool eventFilter(QObject* watched, QEvent* event) override;

public Q_SLOTS:
    void CopyToClipboard();

    void SendInMail();
    void OpenInExcel();

protected:
    void SetReport(CErrorReport* report);
    void UpdateErrors();

protected Q_SLOTS:
    void OnReportItemClick(const QModelIndex& index);
    void OnReportItemRClick();
    void OnReportColumnRClick();
    void OnReportItemDblClick(const QModelIndex& index);
    void OnSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);

protected:
    void OnReportHyperlink(const QModelIndex& index);

    void keyPressEvent(QKeyEvent* event) override;


    CErrorReport* m_pErrorReport;

    static CErrorReportDialog* m_instance;

    QScopedPointer<Ui::CErrorReportDialog> ui;
    CErrorReportTableModel* m_errorReportModel;

    int m_sortIndicatorColumn;
    Qt::SortOrder m_sortIndicatorOrder;
};

#endif // CRYINCLUDE_EDITOR_ERRORREPORTDIALOG_H
