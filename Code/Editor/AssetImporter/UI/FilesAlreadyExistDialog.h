/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
