/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>

#include <QDir>
#include <QString>

namespace O3DE::ProjectManager
{
    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(const QString& thirdPartyPath) const
    {
        QString targetBuildPath = QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix);

        return AZ::Success(QStringList{ ProjectCMakeCommand,
                                        "-B", targetBuildPath,
                                        "-S", m_projectInfo.m_path,
                                        QString("-DLY_3RDPARTY_PATH=").append(thirdPartyPath) } );
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments() const
    {
        QString targetBuildPath = QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix);
        QString launcherTargetName = m_projectInfo.m_projectName + ".GameLauncher";

        return AZ::Success(QStringList{ ProjectCMakeCommand,
                                        "--build", targetBuildPath,
                                        "--config", "profile",
                                        "--target", launcherTargetName, ProjectCMakeBuildTargetEditor });
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(const QString& pidToKill) const
    {
        return AZ::Success(QStringList { "cmd.exe", "/C", "taskkill", "/pid", pidToKill, "/f", "/t" } );
    }

} // namespace O3DE::ProjectManager
