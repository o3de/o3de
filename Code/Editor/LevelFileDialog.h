/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_LEVELFILEDIALOG_H
#define CRYINCLUDE_EDITOR_LEVELFILEDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>
#endif

namespace Ui {
    class LevelFileDialog;
}

class LevelTreeModel;
class LevelTreeModelFilter;

class CLevelFileDialog
    : public QDialog
{
    Q_OBJECT
public:
    explicit CLevelFileDialog(bool openDialog, QWidget* parent = nullptr);
    ~CLevelFileDialog();

    QString GetFileName() const;

    static bool CheckLevelFolder(const QString folder, QStringList* levelFiles = nullptr);

protected Q_SLOTS:
    void OnOK();
    void OnCancel();
    void OnTreeSelectionChanged();
    void OnNewFolder();
    void OnFilterChanged();
    void OnNameChanged();
protected:
    void ReloadTree();
    bool ValidateSaveLevelPath(QString& errorMessage) const;
    bool ValidateLevelPath(const QString& folder) const;

    void SaveLastUsedLevelPath();
    void LoadLastUsedLevelPath();

private:
    bool eventFilter(QObject* watched, QEvent* event) override;

    QString NameForIndex(const QModelIndex& index) const;

    bool IsValidLevelSelected();
    QString GetLevelPath() const;
    QString GetEnteredPath() const;
    QString GetFileName(QString levelPath);

    QScopedPointer<Ui::LevelFileDialog> ui;
    QString m_fileName;
    QString m_filter;
    const bool m_bOpenDialog;
    bool m_initialized = false;
    LevelTreeModel* const m_model;
    LevelTreeModelFilter* const m_filterModel;
};

#endif // CRYINCLUDE_EDITOR_LEVELFILEDIALOG_H
