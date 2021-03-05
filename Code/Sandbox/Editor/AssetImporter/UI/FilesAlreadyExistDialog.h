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
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

class QTreeView;

namespace Ui {
    class FilesAlreadyExistDialog;
}

class FilesAlreadyExistDialog
    : public QDialog
{
    Q_OBJECT

public:
    FilesAlreadyExistDialog(QString message, int numberOfFiles, QWidget* parent = nullptr);
    ~FilesAlreadyExistDialog();

Q_SIGNALS:
    void OverWriteFiles();
    void KeepBothFiles();
    void SkipCurrentProcess();
    void CancelAllProcesses();
    void ApplyActionToAllFiles(bool result);

public Q_SLOTS:
    void DoSkipCurrentProcess();
    void DoOverwrite();
    void DoKeepBoth();
    void DoApplyActionToAllFiles();

private:
    void InitializeButtons();
    void UpdateMessage(QString message);
    void UpdateCheckBoxState(int numberOfFiles);
    void closeEvent(QCloseEvent* ev) override;
    QScopedPointer<Ui::FilesAlreadyExistDialog> m_ui;
};
