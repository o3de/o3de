/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
