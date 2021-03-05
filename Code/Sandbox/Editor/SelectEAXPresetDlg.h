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

// CSelectEAXPresetDlg dialog
#ifndef CRYINCLUDE_EDITOR_SELECTEAXPRESETDLG_H
#define CRYINCLUDE_EDITOR_SELECTEAXPRESETDLG_H

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

class QAbstractListModel;
class Ui_CSelectEAXPresetDlg;

class CSelectEAXPresetDlg
    : public QDialog
{
    Q_OBJECT

public:
    CSelectEAXPresetDlg(QWidget* pParent = nullptr);   // standard constructor
    ~CSelectEAXPresetDlg();

    void SetCurrPreset(const QString& sPreset);
    QString GetCurrPreset() const;

protected:
    void SetModel(QAbstractListModel* model);
    QAbstractListModel* Model() const;

private:
    Ui_CSelectEAXPresetDlg* m_ui;
};

#endif // CRYINCLUDE_EDITOR_SELECTEAXPRESETDLG_H
