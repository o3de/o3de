/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
#ifndef CRYINCLUDE_EDITOR_NEWLEVELDIALOG_H
#define CRYINCLUDE_EDITOR_NEWLEVELDIALOG_H

#if !defined(Q_MOC_RUN)
#include <QScopedPointer>

#include <vector>

#include <QAbstractButton>
#include <QDialog>
#endif

namespace Ui {
    class CNewLevelDialog;
}

class CNewLevelDialog
    : public QDialog
{
    Q_OBJECT

public:
    CNewLevelDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CNewLevelDialog();

    QString GetLevel() const;
    void IsResize(bool bIsResize);
    bool ValidateLevel();

protected:
    void UpdateData(bool fromUi = true);
    void OnInitDialog();

    void ReloadLevelFolder();

    void showEvent(QShowEvent* event);

    QString GetLevelsFolder() const;

protected slots:
    void OnLevelNameChange();
    void OnClearButtonClicked();
    void PopupAssetPicker();
    void OnStartup();

public:
    QString         m_level;
    QString         m_levelFolders;
    bool                m_bIsResize;
    bool                m_bUpdate;

    std::vector<QString>    m_itemFolders;

    QScopedPointer<Ui::CNewLevelDialog> ui;
    bool m_initialized;
};
#endif // CRYINCLUDE_EDITOR_NEWLEVELDIALOG_H
