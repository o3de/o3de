/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>
#include <PythonBindingsInterface.h>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QThread>

#include "EngineInfo.h"

namespace O3DE::ProjectManager
{
    QProcessEnvironment ProjectBuilderWorker::GetProcessEnvironment(const EngineInfo& engineInfo) const
    {
        QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
        // Append cmake path to PATH incase it is missing
        QDir cmakePath(engineInfo.m_path);
        cmakePath.cd("cmake/runtime/bin");
        QString pathValue = currentEnvironment.value("PATH");
        pathValue += ";" + cmakePath.path();
        currentEnvironment.insert("PATH", pathValue);
        return currentEnvironment;
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(QString thirdPartyPath) const
    {
        // Validate that cmake is installed and is in the command line
        QString cmakeCommand = "cmake";

        int cmakeVersionResult = QProcess::execute(cmakeCommand, QStringList{ QString("--version") });
        if (cmakeVersionResult != 0)
        {
            return AZ::Failure(QString("Unable to resolve cmake in the command line. Make sure that cmake is installed and in the PATH environment."));
        }

        QStringList cmakeGenerateArgumentList{ cmakeCommand ,
                                               "-B",
                                               QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix),
                                               "-S", m_projectInfo.m_path,
                                               QString("-DLY_3RDPARTY_PATH=").append(thirdPartyPath),
                                               "-DLY_UNITY_BUILD=ON"
        };
        return AZ::Success(cmakeGenerateArgumentList);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments() const
    {
        QStringList cmakeBuildCmd{ "cmake",
                                   "--build", QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix), "--config", "profile",
                                   "--target", m_projectInfo.m_projectName + ".GameLauncher", "Editor" };
        return AZ::Success(cmakeBuildCmd);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(QString pidToKill) const
    {
        QStringList killProcCmd{ "cmd.exe", "/C", "taskkill", "/pid", pidToKill, "/f", "/t" };
        return AZ::Success(killProcCmd);
    }

} // namespace O3DE::ProjectManager
