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

#ifndef CRYINCLUDE_EDITOR_SETTINGSMANAGERDIALOG_H
#define CRYINCLUDE_EDITOR_SETTINGSMANAGERDIALOG_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>
#endif


class CSettingsManager;

//////////////////////////////////////////////////////////////////////////
// Settings Manager Dialog

namespace Ui {
    class SettingsManagerDialog;
}

class CSettingsManagerDialog
    : public QDialog
{
    Q_OBJECT

public:
    CSettingsManagerDialog(QWidget* pParent = nullptr);
    ~CSettingsManagerDialog();

    static const GUID& GetClassID();
    static void RegisterViewClass();

protected:
    void OnReadBtnClick();
    void OnExportBtnClick();
    void ImportSettings(QString file);
    void ImportLayouts(QString file, const QStringList& layouts);
    void OnImportBtnClick();
    void OnCloseAllTools();

private:
    QString m_importFileStr;

    QScopedPointer<Ui::SettingsManagerDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_SETTINGSMANAGERDIALOG_H
