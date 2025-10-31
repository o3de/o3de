/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
    void OnSelectionChanged();

private:
    QString m_importFileStr;

    QScopedPointer<Ui::SettingsManagerDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_SETTINGSMANAGERDIALOG_H
