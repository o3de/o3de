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


        QStringList args = { 
            ProjectCMakeCommand,
            "-B", targetBuildPath,
            "-S", m_projectInfo.m_path,
            QString("-DLY_3RDPARTY_PATH=").append(thirdPartyPath) 
            };

        if (m_projectInfo.m_isScriptOnly)
        {
            // Due to the way Visual Studio / MSBuild works, the Visual Studio CMake Generator is unable
            // to override the compiler / linker to use in any trivial manner (it would instead require an entire
            // visual studio toolchain to be actually installed on the machine).
            // It completely ignores the CMAKE_CXX_COMPILER and CMAKE_C_COMPILER variables, among other things.
            // We must use something else instead of MSBuild/VS on Windows, because of this.
            // The easiest is Ninja.  On other platforms, the default generators, for example
            // "Unix Makefiles", will actually just do what you tell them to do in regards to fake compilers
            // and thus do not need to be overridden.
            args.append("-GNinja Multi-Config");
        }

        return AZ::Success( args );
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments() const
    {
        const QString targetBuildPath = QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix);
        const QString gameLauncherTargetName = m_projectInfo.m_projectName + ".GameLauncher";
        const QString headlessServerLauncherTargetName = m_projectInfo.m_projectName + ".HeadlessServerLauncher";
        const QString serverLauncherTargetName = m_projectInfo.m_projectName + ".ServerLauncher";
        const QString unifiedLauncherTargetName = m_projectInfo.m_projectName + ".UnifiedLauncher";

        return AZ::Success(QStringList{ ProjectCMakeCommand,
                                        "--build", targetBuildPath,
                                        "--config", "profile",
                                        "--target", gameLauncherTargetName, headlessServerLauncherTargetName, serverLauncherTargetName, unifiedLauncherTargetName, ProjectCMakeBuildTargetEditor });
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(const QString& pidToKill) const
    {
        return AZ::Success(QStringList { "cmd.exe", "/C", "taskkill", "/pid", pidToKill, "/f", "/t" } );
    }

} // namespace O3DE::ProjectManager
