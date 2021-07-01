/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectInfo.h>
#include <QLabel>
#endif

QT_FORWARD_DECLARE_CLASS(QPixmap)
QT_FORWARD_DECLARE_CLASS(QPushButton)
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
        QPushButton* GetBuildButton();
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
        QPushButton* m_buildButton;
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

        void SetLaunchButtonEnabled(bool enabled);
        void ShowBuildButton(bool show);
        void ShowBuildFailed(bool show, const QUrl& logUrl);
        void SetButtonOverlayText(const QString& text);
        void SetProgressBarValue(int progress);

    signals:
        void OpenProject(const QString& projectName);
        void EditProject(const QString& projectName);
        void CopyProject(const QString& projectName);
        void RemoveProject(const QString& projectName);
        void DeleteProject(const QString& projectName);
        void BuildProject(const ProjectInfo& projectInfo);

    private:
        void BaseSetup();
        void ProcessingSetup();
        void ReadySetup();
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;

        ProjectInfo m_projectInfo;
        LabelButton* m_projectImageLabel;
        QFrame* m_projectFooter;
        QLayout* m_requiresBuildLayout;
    };
} // namespace O3DE::ProjectManager
