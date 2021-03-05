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
#include <QScopedPointer>
#include <QDialog>
#endif

namespace Ui {
    class CUiAVNewSequenceDialog;
}


class CUiAVNewSequenceDialog
    : public QDialog
{
    Q_OBJECT
public:
    CUiAVNewSequenceDialog(QWidget* pParent = 0);
    virtual ~CUiAVNewSequenceDialog();

    const QString& GetSequenceName() const { return m_sequenceName; };

protected:
    virtual void OnOK();

private:
    QString m_sequenceName;
    QScopedPointer<Ui::CUiAVNewSequenceDialog> ui;
};
