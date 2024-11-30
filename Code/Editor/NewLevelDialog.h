/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <QScopedPointer>
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
    CNewLevelDialog(QWidget* pParent = nullptr);
    ~CNewLevelDialog();

    QString GetLevel() const;
    bool ValidateLevel();
    QString GetTemplateName() const;

protected:
    void UpdateData(bool fromUi = true);
    void OnInitDialog();
    void ReloadLevelFolder();
    void showEvent(QShowEvent* event) override;
    QString GetLevelsFolder() const;
    void InitTemplateListWidget() const;

protected slots:
    void OnLevelNameChange();
    void OnClearButtonClicked();
    void PopupAssetPicker();
    void OnStartup();

public:
    QString         m_level;
    QString         m_levelFolders;
    QScopedPointer<Ui::CNewLevelDialog> ui;
    bool m_initialized;
};
