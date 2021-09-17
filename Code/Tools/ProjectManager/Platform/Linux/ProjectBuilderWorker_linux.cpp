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
        return currentEnvironment;
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(QString thirdPartyPath) const
    {
        // Validate that cmake is installed and is in the command line
        if (QProcess::execute("which", QStringList{ "cmake" }) != 0)
        {
            return AZ::Failure(QString("Unable to resolve cmake in the command line. Make sure that cmake is installed."));
        }

        // Attempt to use the Ninja build system if it is installed (described in the o3de documentation) if possible,
        // otherwise default to the the default for Linux (Unix Makefiles)
        QString cmakeGenerator = (QProcess::execute("which", QStringList{ "ninja" }) == 0) ? "Ninja" : "Unix Makefiles";

        // On Linux the default compiiler is gcc. For O3DE, it is clang, so we need to specify the version of clang that is detected
        // in order to get the compiler option.
        auto compilerOptionResult = ProjectUtils::FindSupportedCompilerForPlatform();
        if (!compilerOptionResult.IsSuccess())
        {
            return AZ::Failure(compilerOptionResult.GetError());
        }
        QString compilerOption = compilerOptionResult.GetValue();

        QStringList cmakeGenerateArgumentList{ "cmake",
                                               "-B",
                                               QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix),
                                               "-S", m_projectInfo.m_path,
                                               "-G", cmakeGenerator,
                                               QString("-DCMAKE_C_COMPILER=").append(compilerOption),
                                               QString("-DCMAKE_CXX_COMPILER=").append(compilerOption),
                                               QString("-DLY_3RDPARTY_PATH=").append(thirdPartyPath),
                                               "-DCMAKE_BUILD_TYPE=profile",
                                               "-DLY_UNITY_BUILD=ON"
        };
        return AZ::Success(cmakeGenerateArgumentList);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments() const
    {
        QStringList cmakeBuildCmd{ "cmake",
                                   "--build", QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix),
                                   "--target", m_projectInfo.m_projectName + ".GameLauncher", "Editor" };
        return AZ::Success(cmakeBuildCmd);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(QString pidToKill) const
    {
        QStringList killProcCmd{ "kill", "-9", pidToKill };
        return AZ::Success(killProcCmd);
    }
} // namespace O3DE::ProjectManager
