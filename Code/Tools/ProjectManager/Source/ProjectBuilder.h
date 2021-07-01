/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    class ProjectBuilderWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit ProjectBuilderWorker(const ProjectInfo& projectInfo);
        ~ProjectBuilderWorker() = default;

        QString GetLogFilePath() const;

    public slots:
        void BuildProject();

    signals:
        void UpdateProgress(int progress);
        void Done(QString result);

    private:
        QProcess* m_configProjectProcess = nullptr;
        QProcess* m_buildProjectProcess = nullptr;
        ProjectInfo m_projectInfo;

        int m_progressEstimate;
    };

    class ProjectBuilderController : public QObject
    {
        Q_OBJECT

    public:
        explicit ProjectBuilderController(const ProjectInfo& projectInfo, ProjectButton* projectButton, QWidget* parent = nullptr);
        ~ProjectBuilderController();

        void SetProjectButton(ProjectButton* projectButton);
        const ProjectInfo& GetProjectInfo() const;

    public slots:
        void Start();
        void UpdateUIProgress(int progress);
        void HandleResults(const QString& result);
        void HandleCancel();

    signals:
        void Done(bool success = true);

    private:
        ProjectInfo m_projectInfo;
        ProjectBuilderWorker* m_worker;
        QThread m_workerThread;
        ProjectButton* m_projectButton;
        QWidget* m_parent;

        int m_lastProgress;
    };
} // namespace O3DE::ProjectManager
