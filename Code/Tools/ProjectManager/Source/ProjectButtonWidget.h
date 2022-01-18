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
        Q_OBJECT // AUTOMOC

    public:
        explicit LabelButton(QWidget* parent = nullptr);
        ~LabelButton() = default;

        void SetEnabled(bool enabled);
        void SetOverlayText(const QString& text);
        void SetLogUrl(const QUrl& url);

        QLabel* GetOverlayLabel();
        QProgressBar* GetProgressBar();
        QPushButton* GetOpenEditorButton();
        QPushButton* GetActionButton();
        QLabel* GetWarningLabel();
        QLabel* GetWarningIcon();
        QLayout* GetBuildOverlayLayout();

    signals:
        void triggered(QMouseEvent* event);

    public slots:
        void mousePressEvent(QMouseEvent* event) override;
        void OnLinkActivated(const QString& link);

    private:
        QVBoxLayout* m_buildOverlayLayout = nullptr;
        QLabel* m_overlayLabel = nullptr;
        QProgressBar* m_progressBar = nullptr;
        QPushButton* m_openEditorButton = nullptr;
        QPushButton* m_actionButton = nullptr;
        QLabel* m_warningText = nullptr;
        QLabel* m_warningIcon = nullptr;

        QUrl m_logUrl;
        bool m_enabled = true;
    };

    class ProjectButton
        : public QFrame
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit ProjectButton(const ProjectInfo& m_projectInfo, QWidget* parent = nullptr);
        ~ProjectButton() = default;

        const ProjectInfo& GetProjectInfo() const;

        void RestoreDefaultState();

        void SetProjectButtonAction(const QString& text, AZStd::function<void()> lambda);
        void SetBuildLogsLink(const QUrl& logUrl);
        void ShowBuildFailed(bool show, const QUrl& logUrl);
        void ShowBuildRequired();
        void SetProjectBuilding();

        void SetLaunchButtonEnabled(bool enabled);
        void SetButtonOverlayText(const QString& text);
        void SetProgressBarValue(int progress);
        LabelButton* GetLabelButton();

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
        void ShowWarning(bool show, const QString& warning);
        void ShowDefaultBuildButton();

        QMenu* CreateProjectMenu();

        ProjectInfo m_projectInfo;

        LabelButton* m_projectImageLabel = nullptr;
        QPushButton* m_projectMenuButton = nullptr;
        QLayout* m_requiresBuildLayout = nullptr;

        QMetaObject::Connection m_actionButtonConnection;
    };
} // namespace O3DE::ProjectManager
