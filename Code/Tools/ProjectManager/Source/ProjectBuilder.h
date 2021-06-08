/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
