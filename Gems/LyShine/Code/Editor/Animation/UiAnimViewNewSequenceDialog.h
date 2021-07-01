/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
