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

namespace Ui
{
    class GotoPositionDialog;
}

//! GotoPositionDialog for setting camera position and rotation.
class GotoPositionDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit GotoPositionDialog(QWidget* parent = nullptr);
    ~GotoPositionDialog();

protected:
    void OnInitDialog();
    void accept() override;

    void OnUpdateNumbers();
    void OnChangeEdit();

public:
    QString m_transform;

private:
    QScopedPointer<Ui::GotoPositionDialog> m_ui;
};
