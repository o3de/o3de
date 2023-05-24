/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace News {
    class ResourceManifest;
    class ArticleViewContainer;
}

namespace Ui {
    class WelcomeScreenDialog;
}

class QStringListModel;
class RecentFileList;

class WelcomeScreenDialog
    : public QDialog
{
    Q_OBJECT
public:
    WelcomeScreenDialog(QWidget* pParent = nullptr);
    ~WelcomeScreenDialog();

    const QString& GetLevelPath();
    void SetRecentFileList(RecentFileList* pList);

    bool eventFilter(QObject *watched, QEvent *event) override;

public Q_SLOTS:
    void done(int result) override;

private:
    typedef std::pair<QString, QString> TNamePathPair;
    typedef std::vector<TNamePathPair> TNameFullPathArray;

    Ui::WelcomeScreenDialog* ui;

    QString m_levelPath;
    TNameFullPathArray m_levels;
    RecentFileList* m_pRecentList;
    const char* m_levelExtension = nullptr;
    bool m_messageScrollReported = false;

    bool IsValidLevelName(const QString& path);
    void RemoveLevelEntry(int index);

    void OnShowToolTip(const QModelIndex& index);
    void OnShowContextMenu(const QPoint& point);
    void OnNewLevelBtnClicked(bool checked);
    void OnNewLevelLabelClicked(const QString& checked);
    void OnOpenLevelBtnClicked(bool checked);
    void OnRecentLevelTableItemClicked(const QModelIndex& index);
    void OnCloseBtnClicked(bool checked);

private Q_SLOTS:
    void previewAreaScrolled();
};

