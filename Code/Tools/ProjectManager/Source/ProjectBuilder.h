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

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ProjectButton)

    class ProjectBuilderWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit ProjectBuilderWorker(const ProjectInfo& projectInfo);
        ~ProjectBuilderWorker() = default;

        QString LogFilePath() const;

    public slots:
        void BuildProject();

    signals:
        void UpdateProgress(int progress);
        void Done(QString result);

    private:
        void WriteErrorLog(const QString& log);

        ProjectInfo m_projectInfo;
    };

    class ProjectBuilderController : public QObject
    {
        Q_OBJECT

    public:
        explicit ProjectBuilderController(const ProjectInfo& projectInfo, ProjectButton* projectButton, QWidget* parent = nullptr);
        ~ProjectBuilderController();

        void SetProjectButton(ProjectButton* projectButton);
        QString GetProjectPath() const;

    public slots:
        void Start();
        void UpdateUIProgress(int progress);
        void HandleResults(const QString& result);

    signals:
        void Done();

    private:
        ProjectInfo m_projectInfo;
        ProjectBuilderWorker* m_worker;
        QThread m_workerThread;
        ProjectButton* m_projectButton;
        QWidget* m_parent;
    };
} // namespace O3DE::ProjectManager
