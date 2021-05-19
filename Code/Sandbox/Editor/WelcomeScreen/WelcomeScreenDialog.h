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
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include "NewsShared/LogType.h"
#include "NewsShared/ErrorCodes.h"
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
    QStringListModel* m_pRecentListModel;
    TNameFullPathArray m_levels;
    RecentFileList* m_pRecentList;
    News::ResourceManifest* m_manifest = nullptr;
    News::ArticleViewContainer* m_articleViewContainer = nullptr;
    const char* m_levelExtension = nullptr;
    bool m_waitingOnAsync = true;
    bool m_messageScrollReported = false;

    void RemoveLevelEntry(int index);

    void OnShowToolTip(const QModelIndex& index);
    void OnShowContextMenu(const QPoint& point);
    void OnNewLevelBtnClicked(bool checked);
    void OnNewLevelLabelClicked(const QString& checked);
    void OnOpenLevelBtnClicked(bool checked);
    void OnNewSliceBtnClicked(bool checked);
    void OnOpenSliceBtnClicked(bool checked);
    void OnRecentLevelListItemClicked(const QModelIndex& index);
    void OnGettingStartedBtnClicked(bool checked);
    void OnTutorialsBtnClicked(bool checked);
    void OnDocumentationBtnClicked(bool checked);
    void OnForumsBtnClicked(bool checked);
    void OnAutoLoadLevelBtnClicked(bool checked);
    void OnShowOnStartupBtnClicked(bool checked);
    void OnCloseBtnClicked(bool checked);

    void SyncUpdate(const QString& /* message */, News::LogType /* logType */) {}
    void SyncFail(News::ErrorCode error);
    void SyncSuccess();

private Q_SLOTS:
    void previewAreaScrolled();
};

