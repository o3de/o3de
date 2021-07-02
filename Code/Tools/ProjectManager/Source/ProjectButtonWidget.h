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

        QLabel* GetOverlayLabel();
        QProgressBar* GetProgressBar();
        QPushButton* GetActionButton();

    signals:
        void triggered();

    public slots:
        void mousePressEvent(QMouseEvent* event) override;

    private:
        QLabel* m_overlayLabel;
        QProgressBar* m_progressBar;
        QPushButton* m_actionButton;
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

        void SetLaunchButtonEnabled(bool enabled);
        void SetButtonOverlayText(const QString& text);
        void SetProgressBarValue(int progress);
        LabelButton* GetLabelButton();

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
        void BuildThisProject();

        ProjectInfo m_projectInfo;
        LabelButton* m_projectImageLabel;
        QFrame* m_projectFooter;

        QMetaObject::Connection m_actionButtonConnection;
    };
} // namespace O3DE::ProjectManager
