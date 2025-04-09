/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenDefs.h>
#include <ProjectInfo.h>

#include <QStackedWidget>
#include <QStack>
#endif

QT_FORWARD_DECLARE_CLASS(QTabWidget)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ScreenWidget);
    QT_FORWARD_DECLARE_CLASS(DownloadController);

    class ScreensCtrl
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit ScreensCtrl(QWidget* parent = nullptr, DownloadController* downloadController = nullptr);
        ~ScreensCtrl() = default;

        void BuildScreens(QVector<ProjectManagerScreen> screens);
        ScreenWidget* FindScreen(ProjectManagerScreen screen);
        ScreenWidget* GetCurrentScreen();

    signals:
        void NotifyCurrentProject(const QString& projectPath);
        void NotifyBuildProject(const ProjectInfo& projectInfo);
        void NotifyProjectRemoved(const QString& projectPath);
        void NotifyRemoteContentRefreshed();

    public slots:
        bool ChangeToScreen(ProjectManagerScreen screen);
        bool ForceChangeToScreen(ProjectManagerScreen screen, bool addVisit = true);
        bool GoToPreviousScreen();
        void ResetScreen(ProjectManagerScreen screen);
        void ResetAllScreens();
        void DeleteScreen(ProjectManagerScreen screen);
        void DeleteAllScreens();
        void TabChanged(int index);

    private:
        int GetScreenTabIndex(ProjectManagerScreen screen);

        QStackedWidget* m_screenStack;
        QHash<ProjectManagerScreen, ScreenWidget*> m_screenMap;
        QStack<ProjectManagerScreen> m_screenVisitOrder;
        QTabWidget* m_tabWidget;
        DownloadController* m_downloadController;
    };

} // namespace O3DE::ProjectManager
