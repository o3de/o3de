/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EngineInfo.h>
#include <ProjectInfo.h>
#include <AzCore/std/functional.h>

#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QLayout>
#endif

QT_FORWARD_DECLARE_CLASS(QPixmap)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QProgressBar)
QT_FORWARD_DECLARE_CLASS(QLayout)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace AzQtComponents
{
    QT_FORWARD_DECLARE_CLASS(ElidingLabel)
}

namespace O3DE::ProjectManager
{
    class LabelButton
        : public QLabel
    {
        Q_OBJECT

    public:
        explicit LabelButton(QWidget* parent = nullptr);
        ~LabelButton() = default;

        QLabel* GetMessageLabel();
        QLabel* GetSubMessageLabel();
        QLabel* GetWarningIcon();
        QLabel* GetCloudIcon();
        QSpacerItem* GetWarningSpacer();
        QLabel* GetBuildingAnimationLabel();
        QPushButton* GetOpenEditorButton();
        QPushButton* GetActionButton();
        QPushButton* GetActionCancelButton();
        QPushButton* GetShowLogsButton();
        QLabel* GetDarkenOverlay();
        QProgressBar* GetProgressBar();
        QLabel* GetProgressPercentage();
        QLabel* GetDownloadMessageLabel();

    public slots:
        void mousePressEvent(QMouseEvent* event) override;

    signals:
        void triggered(QMouseEvent* event);

    private:
        QVBoxLayout* m_projectOverlayLayout = nullptr;

        QLabel* m_darkenOverlay = nullptr;

        QLabel* m_messageLabel = nullptr;
        QLabel* m_subMessageLabel = nullptr;

        QLabel* m_warningIcon = nullptr;
        QSpacerItem* m_warningSpacer = nullptr;

        QLabel* m_cloudIcon = nullptr;

        QLabel* m_buildingAnimation = nullptr;
        QLabel* m_downloadMessageLabel = nullptr;

        QProgressBar* m_progessBar = nullptr;
        QLabel* m_progressMessageLabel = nullptr;

        QPushButton* m_openEditorButton = nullptr;
        QPushButton* m_actionButton = nullptr;
        QPushButton* m_actionCancelButton = nullptr;
        QPushButton* m_showLogsButton = nullptr;
    };

    enum class ProjectButtonState
    {
        ReadyToLaunch = 0,
        Launching,
        NeedsToBuild,
        Building,
        BuildFailed,
        Exporting,
        ExportFailed,
        NotDownloaded,
        Downloading,
        DownloadingBuildQueued,
        DownloadFailed
    };

    class ProjectButton
        : public QFrame
    {
        Q_OBJECT

    public:
        ProjectButton(const ProjectInfo& projectInfo, const EngineInfo& engineInfo, QWidget* parent = nullptr);
        ~ProjectButton();

        const ProjectInfo& GetProjectInfo() const;

        void SetEngine(const EngineInfo& engine);
        void SetProject(const ProjectInfo& project);
        void SetState(ProjectButtonState state);
        const ProjectButtonState GetState() const
        {
            return m_currentState;
        }

        void SetProjectButtonAction(const QString& text, AZStd::function<void()> lambda);
        void SetBuildLogsLink(const QUrl& logUrl);

        void SetProgressBarPercentage(const float percent);

        void SetContextualText(const QString& text);

        LabelButton* GetLabelButton();

    public slots:
        void ShowLogs();

    signals:
        void OpenProject(const QString& projectName);
        void EditProject(const QString& projectName);
        void EditProjectGems(const QString& projectName);
        void ExportProject(const ProjectInfo& projectInfo, const QString& exportScript, bool skipDialogBox = false);
        void CopyProject(const ProjectInfo& projectInfo);
        void RemoveProject(const QString& projectName);
        void DeleteProject(const QString& projectName);
        void BuildProject(const ProjectInfo& projectInfo, bool skipDialogBox = false);
        void OpenProjectExportSettings(const QString& projectPath);
        void OpenCMakeGUI(const ProjectInfo& projectInfo);
        void OpenAndroidProjectGenerator(const QString& projectPath);

    private:
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;

        void ShowReadyState();
        void ShowLaunchingState();
        void ShowBuildRequiredState();
        void ShowBuildingState();
        void ShowExportingState();
        void ShowBuildFailedState();
        void ShowExportFailedState();
        void ShowNotDownloadedState();
        void ShowDownloadingState();
        void ResetButtonWidgets();
        void ShowMessage(const QString& message = {}, const QString& submessage = {});
        void ShowWarning(const QString& warning = {});
        void ShowBuildButton();
        void SetLaunchingEnabled(bool enabled);
        void SetProjectBuilding(bool isBuilding);
        void SetProjectExporting(bool isExporting);
        void HideContextualLabelButtonWidgets();

        QMenu* CreateProjectMenu();

        EngineInfo m_engineInfo;
        ProjectInfo m_projectInfo;

        LabelButton* m_projectImageLabel = nullptr;
        QPushButton* m_projectMenuButton = nullptr;
        QLayout* m_requiresBuildLayout = nullptr;
        AzQtComponents::ElidingLabel* m_projectNameLabel = nullptr;
        AzQtComponents::ElidingLabel* m_engineNameLabel = nullptr;

        QMetaObject::Connection m_actionButtonConnection;

        bool m_isProjectBuilding = false;
        bool m_isProjectExporting = false;
        bool m_canLaunch = true;

        ProjectButtonState m_currentState = ProjectButtonState::ReadyToLaunch;
    };
} // namespace O3DE::ProjectManager
