/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui {
    class ProcessingAssetsDialog;
}

class ProcessingAssetsDialog
    : public QDialog
{
    Q_OBJECT

public:
    ProcessingAssetsDialog(int numberOfProcessedFiles, QWidget* parent = nullptr);
    ~ProcessingAssetsDialog();

    void InitializeButtons();

Q_SIGNALS:
    void CloseProcessingAssetsDialog();
    void OpenLogDialog();

public Q_SLOTS:
    void accept();
    void reject();

private:
    void UpdateTextsAndTitle(int numberOfProcessedFiles);
    QScopedPointer<Ui::ProcessingAssetsDialog> m_ui;
};
