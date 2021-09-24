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
        auto    whichNinjaResult = ProjectUtils::ExecuteCommandResult("which", QStringList{"ninja"}, QProcessEnvironment::systemEnvironment());
        QString cmakeGenerator = (whichNinjaResult.IsSuccess()) ? "Ninja Multi-Config" : "Unix Makefiles";
        bool    compileProfileOnBuild = (whichNinjaResult.IsSuccess());

        // On Linux the default compiler is gcc. For O3DE, it is clang, so we need to specify the version of clang that is detected
        // in order to get the compiler option.
        auto compilerOptionResult = ProjectUtils::FindSupportedCompilerForPlatform();
        if (!compilerOptionResult.IsSuccess())
        {
            return AZ::Failure(compilerOptionResult.GetError());
        }
        auto clangCompilers = compilerOptionResult.GetValue().split('|');
        AZ_Assert(clangCompilers.length()==2, "Invalid clang compiler pair specification");

        QString clangCompilerOption = clangCompilers[0];
        QString clangPPCompilerOption = clangCompilers[1];
        QString targetBuildPath = QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix);
        QStringList generateProjectArgs = QStringList{ProjectCMakeCommand,
                                                      "-B", ProjectBuildPathPostfix,
                                                      "-S", ".",
                                                      QString("-G%1").arg(cmakeGenerator),
                                                      QString("-DCMAKE_C_COMPILER=").append(clangCompilerOption),
                                                      QString("-DCMAKE_CXX_COMPILER=").append(clangPPCompilerOption),
                                                      QString("-DLY_3RDPARTY_PATH=").append(thirdPartyPath)};
        if (!compileProfileOnBuild)
        {
            generateProjectArgs.append("-DCMAKE_BUILD_TYPE=profile");
        }
        return AZ::Success(generateProjectArgs);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments() const
    {
        auto    whichNinjaResult = ProjectUtils::ExecuteCommandResult("which", QStringList{"ninja"}, QProcessEnvironment::systemEnvironment());
        bool    compileProfileOnBuild = (whichNinjaResult.IsSuccess());
        QString targetBuildPath = QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix);
        QString launcherTargetName = m_projectInfo.m_projectName + ".GameLauncher";

        QStringList buildProjectArgs = QStringList{ProjectCMakeCommand,
                                                   "--build", ProjectBuildPathPostfix,
                                                   "--target", launcherTargetName, ProjectCMakeBuildTargetEditor};
        if (compileProfileOnBuild)
        {
            buildProjectArgs.append(QStringList{"--config","profile"});
        }
        return AZ::Success(buildProjectArgs);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(const QString& pidToKill) const
    {
        return AZ::Success(QStringList{"kill", "-9", pidToKill});
    }

} // namespace O3DE::ProjectManager
