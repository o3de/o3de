/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>
#include <ProjectManagerDefs.h>
#include <QProcessEnvironment>
#include <QDir>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        // The list of clang C/C++ compiler command lines to validate on the host Linux system
        const QStringList SupportedClangCommands = {"clang-12|clang++-12"};

        AZ::Outcome<QProcessEnvironment, QString> GetCommandLineProcessEnvironment()
        {
            QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
            currentEnvironment.insert("CC", "clang-12");
            currentEnvironment.insert("CXX", "clang++-12");
            return AZ::Success(currentEnvironment);
        }

        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform()
        {
            // Validate that cmake is installed and is in the command line
            auto whichCMakeResult = ProjectUtils::ExecuteCommandResult("which", QStringList{ProjectCMakeCommand}, QProcessEnvironment::systemEnvironment());
            if (!whichCMakeResult.IsSuccess())
            {
                return AZ::Failure(QObject::tr("CMake not found. <br><br>"
                    "Make sure that the minimum version of CMake is installed and available from the command prompt. "
                    "Refer to the <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#cmake'>O3DE requirements</a> page for more information."));
            }

            // Look for the first compatible version of clang. The list below will contain the known clang compilers that have been tested for O3DE.
            for (const QString& supportClangCommand : SupportedClangCommands)
            {
                auto clangCompilers = supportClangCommand.split('|');
                AZ_Assert(clangCompilers.length()==2, "Invalid clang compiler pair specification");

                auto whichClangResult = ProjectUtils::ExecuteCommandResult("which", QStringList{clangCompilers[0]}, QProcessEnvironment::systemEnvironment());
                auto whichClangPPResult = ProjectUtils::ExecuteCommandResult("which", QStringList{clangCompilers[1]}, QProcessEnvironment::systemEnvironment());
                if (whichClangResult.IsSuccess() && whichClangPPResult.IsSuccess())
                {
                    return AZ::Success(supportClangCommand);
                }
            }
            return AZ::Failure(QObject::tr("Clang not found. <br><br>"
                "Make sure that the clang is installed and available from the command prompt. "
                "Refer to the <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#cmake'>O3DE requirements</a> page for more information."));
        }


        AZ::Outcome<void, QString> OpenCMakeGUI(const QString& projectPath)
        {
            AZ::Outcome processEnvResult = GetCommandLineProcessEnvironment();
            if (!processEnvResult.IsSuccess())
            {
                return AZ::Failure(processEnvResult.GetError());
            }

            QString projectBuildPath = QDir(projectPath).filePath(ProjectBuildPathPostfix);
            AZ::Outcome projectBuildPathResult = GetProjectBuildPath(projectPath);
            if (projectBuildPathResult.IsSuccess())
            {
                projectBuildPath = projectBuildPathResult.GetValue();
            }

            QProcess process;
            process.setProcessEnvironment(processEnvResult.GetValue());

            // if the project build path is relative, it should be relative to the project path 
            process.setWorkingDirectory(projectPath);

            process.setProgram("cmake-gui");
            process.setArguments({ "-S", projectPath, "-B", projectBuildPath });
            if(!process.startDetached())
            {
                return AZ::Failure(QObject::tr("Failed to start CMake GUI"));
            }

            return AZ::Success();
        }
        
        AZ::Outcome<QString, QString> RunGetPythonScript(const QString& engineRoot)
        {
            return ExecuteCommandResultModalDialog(
                QString("%1/python/get_python.sh").arg(engineRoot),
                {},
                QProcessEnvironment::systemEnvironment(),
                QObject::tr("Running get_python script..."));
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
