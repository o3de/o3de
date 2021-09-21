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
    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(QString thirdPartyPath) const
    {
        return AZ::Success( QStringList { "cmake",
                                          "-B", QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix),
                                          "-S", m_projectInfo.m_path,
                                          QString("-DLY_3RDPARTY_PATH=").append(thirdPartyPath),
                                          "-DLY_UNITY_BUILD=ON" } );
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments() const
    {
        return AZ::Success( QStringList { "cmake",
                                          "--build", QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix), "--config", "profile",
                                          "--target", m_projectInfo.m_projectName + ".GameLauncher", "Editor" } );
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(QString pidToKill) const
    {
        return AZ::Success( QStringList { "cmd.exe", "/C", "taskkill", "/pid", pidToKill, "/f", "/t" } );
    }

} // namespace O3DE::ProjectManager
