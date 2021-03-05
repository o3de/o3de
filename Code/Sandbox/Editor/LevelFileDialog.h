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
