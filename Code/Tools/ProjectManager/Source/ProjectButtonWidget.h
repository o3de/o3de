/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
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
        QSpacerItem* GetWarningSpacer();
        QLabel* GetBuildingAnimationLabel();
        QPushButton* GetOpenEditorButton();
        QPushButton* GetActionButton();
        QPushButton* GetActionCancelButton();
        QPushButton* GetShowLogsButton();
        QLabel* GetDarkenOverlay();

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

        QLabel* m_buildingAnimation = nullptr;

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
        BuildFailed
    };

    class ProjectButton
        : public QFrame
    {
        Q_OBJECT

    public:
        explicit ProjectButton(const ProjectInfo& m_projectInfo, QWidget* parent = nullptr);
        ~ProjectButton() = default;

        const ProjectInfo& GetProjectInfo() const;

        void SetState(enum ProjectButtonState state);

        void SetProjectButtonAction(const QString& text, AZStd::function<void()> lambda);
        void SetBuildLogsLink(const QUrl& logUrl);

        void SetContextualText(const QString& text);

        LabelButton* GetLabelButton();

    public slots:
        void ShowLogs();

    signals:
        void OpenProject(const QString& projectName);
        void EditProject(const QString& projectName);
        void EditProjectGems(const QString& projectName);
        void CopyProject(const ProjectInfo& projectInfo);
        void RemoveProject(const QString& projectName);
        void DeleteProject(const QString& projectName);
        void BuildProject(const ProjectInfo& projectInfo);
        void OpenCMakeGUI(const ProjectInfo& projectInfo);

    private:
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;

        void ShowReadyState();
        void ShowLaunchingState();
        void ShowBuildRequiredState();
        void ShowBuildingState();
        void ShowBuildFailedState();
        void ShowMessage(const QString& message = {}, const QString& submessage = {});
        void ShowWarning(const QString& warning = {});
        void ShowBuildButton();
        void SetLaunchingEnabled(bool enabled);
        void SetProjectBuilding(bool isBuilding);
        void HideContextualLabelButtonWidgets();

        QMenu* CreateProjectMenu();

        ProjectInfo m_projectInfo;

        LabelButton* m_projectImageLabel = nullptr;
        QPushButton* m_projectMenuButton = nullptr;
        QLayout* m_requiresBuildLayout = nullptr;

        QMetaObject::Connection m_actionButtonConnection;

        bool m_isProjectBuilding = false;
        bool m_canLaunch = true;

        ProjectButtonState m_currentState = ProjectButtonState::ReadyToLaunch;
    };
} // namespace O3DE::ProjectManager
