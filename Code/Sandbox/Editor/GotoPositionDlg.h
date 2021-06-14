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
