/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#if !defined(Q_MOC_RUN)
#include <ScreenWidget.h>
#include <EngineInfo.h>
#include <ProjectInfo.h>

#include <DownloadController.h>

#include <QQueue>
#include <QVector>
#endif

QT_FORWARD_DECLARE_CLASS(QPaintEvent)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QLayout)
QT_FORWARD_DECLARE_CLASS(FlowLayout)
QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ProjectBuilderController);
    QT_FORWARD_DECLARE_CLASS(ProjectExportController);
    QT_FORWARD_DECLARE_CLASS(ProjectButton);
    QT_FORWARD_DECLARE_CLASS(DownloadController);

    class ProjectsScreen
        : public ScreenWidget
    {

    public:
        explicit ProjectsScreen(DownloadController* downloadController, QWidget* parent = nullptr);
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
        void HandleAddRemoteProjectButton();
        void HandleOpenProject(const QString& projectPath);
        void HandleEditProject(const QString& projectPath);
        void HandleEditProjectGems(const QString& projectPath);
        void HandleCopyProject(const ProjectInfo& projectInfo);
        void HandleRemoveProject(const QString& projectPath);
        void HandleDeleteProject(const QString& projectPath);
        void HandleOpenAndroidProjectGenerator(const QString& projectPath);
        void HandleOpenProjectExportSettings(const QString& projectPath);



        void SuggestBuildProject(const ProjectInfo& projectInfo);
        void QueueBuildProject(const ProjectInfo& projectInfo, bool skipDialogBox = false);
        void UnqueueBuildProject(const ProjectInfo& projectInfo);

        void QueueExportProject(const ProjectInfo& projectInfo, const QString& exportScript, bool skipDialogBox = false);
        void UnqueueExportProject(const ProjectInfo& projectInfo);

        void StartProjectDownload(const QString& projectName, const QString& destinationPath, bool queueBuild);
        void HandleDownloadProgress(const QString& projectName, DownloadController::DownloadObjectType objectType, int bytesDownloaded, int totalBytes);
        void HandleDownloadResult(const QString& projectName, bool succeeded);

        void ProjectBuildDone(bool success = true);
        void ProjectExportDone(bool success = false);

        void paintEvent(QPaintEvent* event) override;

        void HandleProjectFilePathChanged(const QString& path);

    private:
        QFrame* CreateFirstTimeContent();
        QFrame* CreateProjectsContent();
        ProjectButton* CreateProjectButton(const ProjectInfo& project, const EngineInfo& engine);
        QVector<ProjectInfo> GetAllProjects();
        void UpdateWithProjects(const QVector<ProjectInfo>& projects);
        void UpdateIfCurrentScreen();
        bool ShouldDisplayFirstTimeContent(bool projectsFound);
        void RemoveProjectButtonsFromFlowLayout(const QVector<ProjectInfo>& projectsToKeep);

        bool StartProjectBuild(const ProjectInfo& projectInfo, bool skipDialogBox = false);
        QList<ProjectInfo>::iterator RequiresBuildProjectIterator(const QString& projectPath);
        bool BuildQueueContainsProject(const QString& projectPath);
        bool WarnIfInBuildQueue(const QString& projectPath);

        bool StartProjectExport(const ProjectInfo& projectInfo, bool skipDialogBox = false);
        bool ExportQueueContainsProject(const QString& projectPath);
        bool WarnIfInExportQueue(const QString& projectPath);

        QAction* m_createNewProjectAction = nullptr;
        QAction* m_addExistingProjectAction = nullptr;
        QAction* m_addRemoteProjectAction = nullptr;
        QPixmap m_background;
        QFrame* m_firstTimeContent = nullptr;
        QFrame* m_projectsContent = nullptr;
        FlowLayout* m_projectsFlowLayout = nullptr;
        QFileSystemWatcher* m_fileSystemWatcher = nullptr;
        QStackedWidget* m_stack = nullptr;
        AZStd::unordered_map<AZ::IO::Path, ProjectButton*> m_projectButtons;
        QList<ProjectInfo> m_requiresBuild;
        QQueue<ProjectInfo> m_buildQueue;
        QQueue<ProjectInfo> m_exportQueue;
        ProjectBuilderController* m_currentBuilder = nullptr;
        AZStd::unique_ptr<ProjectExportController> m_currentExporter;
        DownloadController* m_downloadController = nullptr;

        inline constexpr static int s_contentMargins = 80;
        inline constexpr static int s_spacerSize = 20;
    };

} // namespace O3DE::ProjectManager
