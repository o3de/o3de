/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>
#include <ProjectUtils.h>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QThread>

namespace O3DE::ProjectManager
{
    QProcessEnvironment ProjectBuilderWorker::GetProcessEnvironment(const EngineInfo& engineInfo) const
    {
        QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
        QString pathValue = currentEnvironment.value("PATH");
        pathValue += ":/usr/local/bin";
        currentEnvironment.insert("PATH", pathValue);
        return currentEnvironment;
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(QString thirdPartyPath) const
    {
        // Query the cmake full path
        QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
        QString pathValue = currentEnvironment.value("PATH");
        pathValue += ":/usr/local/bin";
        currentEnvironment.insert("PATH", pathValue);

        auto queryCmakeInstalled = ProjectUtils::ExecuteCommandResult("which",QStringList {"cmake"}, currentEnvironment);
        if (!queryCmakeInstalled.IsSuccess())
        {
            return AZ::Failure(QObject::tr("Unable to detect CMake on this host."));
        }
        QString cmakeInstalledPath = queryCmakeInstalled.GetValue().split("\n")[0];

        QStringList cmakeGenerateArgumentList{ cmakeInstalledPath,
                                               "-B",
                                               QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix),
                                               "-S", m_projectInfo.m_path,
                                               "-DLY_UNITY_BUILD=ON"
        };
        return AZ::Success(cmakeGenerateArgumentList);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments() const
    {
        // Query the cmake full path
        QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
        QString pathValue = currentEnvironment.value("PATH");
        pathValue += ":/usr/local/bin";
        currentEnvironment.insert("PATH", pathValue);

        auto queryCmakeInstalled = ProjectUtils::ExecuteCommandResult("which",QStringList {"cmake"}, currentEnvironment);
        if (!queryCmakeInstalled.IsSuccess())
        {
            return AZ::Failure(QObject::tr("Unable to detect CMake on this host."));
        }
        QString cmakeInstalledPath = queryCmakeInstalled.GetValue().split("\n")[0];
        
        QStringList cmakeBuildCmd{ cmakeInstalledPath,
                                   "--build", QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix),
                                   "--config", "profile",
                                   "--target", m_projectInfo.m_projectName + ".GameLauncher", "Editor" };
        return AZ::Success(cmakeBuildCmd);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(QString pidToKill) const
    {
        QStringList killProcCmd{ "kill", "-9", pidToKill };
        return AZ::Success(killProcCmd);
    }
    
} // namespace O3DE::ProjectManager
