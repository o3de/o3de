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

#ifndef CRYINCLUDE_EDITOR_SHADERSDIALOG_H
#define CRYINCLUDE_EDITOR_SHADERSDIALOG_H

#pragma once
// ShadersDialog.h : header file
//

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

class QStringListModel;

namespace Ui {
    class CShadersDialog;
}

/////////////////////////////////////////////////////////////////////////////
// CShadersDialog dialog

class CShadersDialog
    : public QDialog
{
    Q_OBJECT

    // Construction
public:
    CShadersDialog(const QString& selection, QWidget* pParent = nullptr);   // standard constructor
    ~CShadersDialog();

    QString m_selection;

    QString GetSelection() { return m_selection; };

protected:
    void OnSelchangeShaders();
    virtual void OnInitDialog();
    void OnDblclkShaders();

public:
    void OnBnClickedEdit();
    void OnBnClickedSave();
    void OnEnChangeText();

    QStringListModel* m_shadersModel;
    QScopedPointer<Ui::CShadersDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_SHADERSDIALOG_H
