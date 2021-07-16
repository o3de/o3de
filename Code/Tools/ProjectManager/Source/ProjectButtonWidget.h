/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        void triggered();

    public slots:
        void mousePressEvent(QMouseEvent* event) override;
        void OnLinkActivated(const QString& link);

    private:
        QVBoxLayout* m_buildOverlayLayout;
        QLabel* m_overlayLabel;
        QProgressBar* m_progressBar;
        QPushButton* m_openEditorButton;
        QPushButton* m_actionButton;
        QLabel* m_warningText;
        QLabel* m_warningIcon;
        QUrl m_logUrl;
        bool m_enabled = true;
    };

    class ProjectButton
        : public QFrame
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit ProjectButton(const ProjectInfo& m_projectInfo, QWidget* parent = nullptr, bool processing = false);
        ~ProjectButton() = default;

        void SetProjectButtonAction(const QString& text, AZStd::function<void()> lambda);
        void SetProjectBuildButtonAction();
        void SetBuildLogsLink(const QUrl& logUrl);
        void ShowBuildFailed(bool show, const QUrl& logUrl);

        void SetLaunchButtonEnabled(bool enabled);
        void SetButtonOverlayText(const QString& text);
        void SetProgressBarValue(int progress);
        LabelButton* GetLabelButton();

    signals:
        void OpenProject(const QString& projectName);
        void EditProject(const QString& projectName);
        void CopyProject(const ProjectInfo& projectInfo);
        void RemoveProject(const QString& projectName);
        void DeleteProject(const QString& projectName);
        void BuildProject(const ProjectInfo& projectInfo);

    private:
        void BaseSetup();
        void ProcessingSetup();
        void ReadySetup();
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void BuildThisProject();

        ProjectInfo m_projectInfo;
        LabelButton* m_projectImageLabel;
        QFrame* m_projectFooter;
        QLayout* m_requiresBuildLayout;

        QMetaObject::Connection m_actionButtonConnection;
    };
} // namespace O3DE::ProjectManager
