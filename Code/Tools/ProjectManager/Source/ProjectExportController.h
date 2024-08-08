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

#include <QThread>
#endif

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ProjectButton)
    QT_FORWARD_DECLARE_CLASS(ProjectExportWorker)

    class ProjectExportController : public QObject
    {
        Q_OBJECT

    public:
        explicit ProjectExportController(const ProjectInfo& projectInfo, ProjectButton* projectButton, QWidget* parent = nullptr);
        ~ProjectExportController();

        void SetProjectButton(ProjectButton* projectButton);
        const ProjectInfo& GetProjectInfo() const;

        constexpr static int s_maxDisplayedBuiltOutputChars = 25;
        inline static const char * LauncherExportFailedMessage = "Launcher failed to export.";

    public slots:
        void Start();
        void UpdateUIProgress(const QString& lastLine);
        void HandleResults(const QString& result);
        void HandleCancel();

    signals:
        void Done(bool success = true);
        void NotifyExportProject(const ProjectInfo& projectInfo);

    private:
        ProjectInfo m_projectInfo;
        ProjectExportWorker* m_worker;
        QThread m_workerThread;
        ProjectButton* m_projectButton;
        QWidget* m_parent;

        QString m_lastLine;
    };
} // namespace O3DE::ProjectManager
