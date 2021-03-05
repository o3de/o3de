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

#ifndef CRYINCLUDE_EDITOR_TRACKVIEWNEWSEQUENCEDIALOG_H
#define CRYINCLUDE_EDITOR_TRACKVIEWNEWSEQUENCEDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QScopedPointer>
#include <QDialog>
#endif

namespace Ui {
    class CTVNewSequenceDialog;
}


class CTVNewSequenceDialog
    : public QDialog
{
    Q_OBJECT
public:
    CTVNewSequenceDialog(QWidget* pParent = 0);
    virtual ~CTVNewSequenceDialog();

    const QString& GetSequenceName() const { return m_sequenceName; };
    SequenceType   GetSequenceType() const { return m_sequenceType; };

    void showEvent(QShowEvent* event) override;

protected:
    virtual void OnOK();
    void OnInitDialog();

private:
    QString        m_sequenceName;
    SequenceType   m_sequenceType;
    QScopedPointer<Ui::CTVNewSequenceDialog> ui;
    bool           m_inputFocusSet;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEWNEWSEQUENCEDIALOG_H
