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
    class KinematicDescriptionDialog;
}

class KinematicDescriptionDialog
    : public QDialog
{
    Q_OBJECT

public:
    KinematicDescriptionDialog(QString message, int numberOfFiles, QWidget* parent = nullptr);
    ~KinematicDescriptionDialog();
//
//Q_SIGNALS:
//    void valueChanged(bool newValue);
//
//public Q_SLOTS:
//    void setValue(bool val);
//
//protected Q_SLOTS:
//    void onChildComboBoxValueChange(int value);

private:
    void InitializeButtons();
    void UpdateMessage(QString message);
    //void UpdateCheckBoxState(int numberOfFiles);
    //void closeEvent(QCloseEvent* ev) override;
    QScopedPointer<Ui::KinematicDescriptionDialog> m_ui;
};
