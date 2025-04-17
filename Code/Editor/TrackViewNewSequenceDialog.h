/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Qt
#if !defined(Q_MOC_RUN)
#include <QScopedPointer>
#include <QDialog>
#endif

// CryCommon
#include <CryCommon/Maestro/Types/SequenceType.h>

namespace Ui {
    class CTVNewSequenceDialog;
    class CTVNewSequenceDialogValidator;
}

class QValidator;

class CTVNewSequenceDialog
    : public QDialog
{
    Q_OBJECT
public:
    CTVNewSequenceDialog(QWidget* pParent = 0);
    virtual ~CTVNewSequenceDialog();

    const QString& GetSequenceName() const { return m_sequenceName; }
    SequenceType   GetSequenceType() const { return m_sequenceType; }

    void showEvent(QShowEvent* event) override;

    friend class CTVNewSequenceDialogValidator;

protected:
    virtual void OnOK();
    void OnInitDialog();

private:
    QString        m_sequenceName;
    SequenceType   m_sequenceType;
    QScopedPointer<Ui::CTVNewSequenceDialog> ui;
    bool           m_inputFocusSet;
    QValidator*    m_validator;
};
