/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenWidget.h>
#include <ProjectInfo.h>

#include <QQueue>
#endif

QT_FORWARD_DECLARE_CLASS(QPaintEvent)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QLayout)
QT_FORWARD_DECLARE_CLASS(FlowLayout)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ProjectBuilderController);
    QT_FORWARD_DECLARE_CLASS(ProjectButton);

    class ProjectsScreen
        : public ScreenWidget
    {

    public:
        explicit ProjectsScreen(QWidget* parent = nullptr);
        ~ProjectsScreen();

        ProjectManagerScreen GetScreenEnum() override;
        QString GetTabText() override;
        bool IsTab() override;

    protected:
        void NotifyCurrentScreen() override;
        void SuggestBuildProjectMsg(const ProjectInfo& projectInfo, bool showMessage);

    protected slots:
        void HandleNewProjectButton();
        void HandleAddProjectButton();
        void HandleOpenProject(const QString& projectPath);
        void HandleEditProject(const QString& projectPath);
        void HandleEditProjectGems(const QString& projectPath);
        void HandleCopyProject(const ProjectInfo& projectInfo);
        void HandleRemoveProject(const QString& projectPath);
        void HandleDeleteProject(const QString& projectPath);

        void SuggestBuildProject(const ProjectInfo& projectInfo);
        void QueueBuildProject(const ProjectInfo& projectInfo);
        void UnqueueBuildProject(const ProjectInfo& projectInfo);

        void ProjectBuildDone(bool success = true);

        void paintEvent(QPaintEvent* event) override;

    private:
        QFrame* CreateFirstTimeContent();
        QFrame* CreateProjectsContent();
        ProjectButton* CreateProjectButton(const ProjectInfo& project);
        void ResetProjectsContent();
        bool ShouldDisplayFirstTimeContent();
        bool RemoveInvalidProjects();

        bool StartProjectBuild(const ProjectInfo& projectInfo);
        QList<ProjectInfo>::iterator RequiresBuildProjectIterator(const QString& projectPath);
        bool BuildQueueContainsProject(const QString& projectPath);
        bool WarnIfInBuildQueue(const QString& projectPath);

        QAction* m_createNewProjectAction = nullptr;
        QAction* m_addExistingProjectAction = nullptr;
        QPixmap m_background;
        QFrame* m_firstTimeContent = nullptr;
        QFrame* m_projectsContent = nullptr;
        FlowLayout* m_projectsFlowLayout = nullptr;
        QStackedWidget* m_stack = nullptr;
        QHash<QString, ProjectButton*> m_projectButtons;
        QList<ProjectInfo> m_requiresBuild;
        QQueue<ProjectInfo> m_buildQueue;
        ProjectBuilderController* m_currentBuilder = nullptr;

        inline constexpr static int s_contentMargins = 80;
        inline constexpr static int s_spacerSize = 20;
    };

} // namespace O3DE::ProjectManager
