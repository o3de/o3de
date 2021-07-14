/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>

#include <QDir>

//#define MOCK_BUILD_PROJECT true

namespace O3DE::ProjectManager
{
    ProjectBuilderWorker::ProjectBuilderWorker(const ProjectInfo& projectInfo)
        : QObject()
        , m_projectInfo(projectInfo)
        , m_progressEstimate(0)
    {
    }

    void ProjectBuilderWorker::BuildProject()
    {
#ifdef MOCK_BUILD_PROJECT
        for (int i = 0; i < 10; ++i)
        {
            QThread::sleep(1);
            UpdateProgress(i * 10);
        }
        Done("");
#else
        auto result = BuildProjectForPlatform();

        if (result.IsSuccess())
        {
            emit Done();
        }
        else
        {
            emit Done(result.GetError());
        }
#endif
    }

    QString ProjectBuilderWorker::GetLogFilePath() const
    {
        QDir logFilePath(m_projectInfo.m_path);
        // Make directories if they aren't on disk
        if (!logFilePath.cd(ProjectBuildPathPostfix))
        {
            logFilePath.mkpath(ProjectBuildPathPostfix);
            logFilePath.cd(ProjectBuildPathPostfix);

        }
        if (!logFilePath.cd(ProjectBuildPathCmakeFiles))
        {
            logFilePath.mkpath(ProjectBuildPathCmakeFiles);
            logFilePath.cd(ProjectBuildPathCmakeFiles);
        }
        return logFilePath.filePath(ProjectBuildErrorLogName);
    }

    void ProjectBuilderWorker::QStringToAZTracePrint([[maybe_unused]] const QString& error)
    {
        AZ_TracePrintf("Project Manager", error.toStdString().c_str());
    }
} // namespace O3DE::ProjectManager
