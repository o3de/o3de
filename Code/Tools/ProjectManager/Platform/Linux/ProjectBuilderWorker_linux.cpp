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
#include <QString>

namespace O3DE::ProjectManager
{
    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(const QString& thirdPartyPath) const
    {
        // Attempt to use the Ninja build system if it is installed (described in the o3de documentation) if possible,
        // otherwise default to the the default for Linux (Unix Makefiles)
        QString cmakeGenerator = (QProcess::execute("which", QStringList{ "ninja" }) == 0) ? "Ninja" : "Unix Makefiles";

        // On Linux the default compiler is gcc. For O3DE, it is clang, so we need to specify the version of clang that is detected
        // in order to get the compiler option.
        auto compilerOptionResult = ProjectUtils::FindSupportedCompilerForPlatform();
        if (!compilerOptionResult.IsSuccess())
        {
            return AZ::Failure(compilerOptionResult.GetError());
        }

        QString compilerOption = compilerOptionResult.GetValue();
        QString targetBuildPath = QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix);

        return AZ::Success( QStringList { ProjectCMakeCommand,
                                          "-B", targetBuildPath,
                                          "-S", m_projectInfo.m_path,
                                          "-G", cmakeGenerator,
                                          QString("-DCMAKE_C_COMPILER=").append(compilerOption),
                                          QString("-DCMAKE_CXX_COMPILER=").append(compilerOption),
                                          QString("-DLY_3RDPARTY_PATH=").append(thirdPartyPath),
                                          "-DCMAKE_BUILD_TYPE=profile",
                                          "-DLY_UNITY_BUILD=ON"} );
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments() const
    {
        QString targetBuildPath = QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix);
        QString launcherTargetName = m_projectInfo.m_projectName + ".GameLauncher";

        return AZ::Success( QStringList { ProjectCMakeCommand,
                                          "--build", targetBuildPath,
                                          "--target", launcherTargetName, ProjectCMakeBuildTargetEditor } );
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(const QString& pidToKill) const
    {
        return AZ::Success( QStringList { "kill", "-9", pidToKill } );
    }

} // namespace O3DE::ProjectManager
