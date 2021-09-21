/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>

#include <QProcessEnvironment>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        AZ::Outcome<QProcessEnvironment, QString> GetCommandLineProcessEnvironment()
        {
            return AZ::Success(QProcessEnvironment(QProcessEnvironment::systemEnvironment()));
        }

        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform()
        {
            // Validate that cmake is installed and is in the command line
            if (QProcess::execute("which", QStringList{ "cmake" }) != 0)
            {
                return AZ::Failure(QObject::tr("CMake not found. \n\n"
                    "Make sure that the minimum version of CMake is installed and available from the command prompt. "
                    "Refer to the <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#cmake'>O3DE requirements</a> page for more information."));
            }

            // Look for the first compatible version of clang. The list below will contain the known clang compilers that have been tested for O3DE.
            QStringList supportedClangCommands = { "clang-12", "clang-6" };
            for (const QString& supportClangCommand : supportedClangCommands)
            {
                auto whichClangResult = ProjectUtils::ExecuteCommandResult("which", QStringList{ supportClangCommand }, QProcessEnvironment::systemEnvironment());
                if (whichClangResult.IsSuccess())
                {
                    return AZ::Success(supportClangCommand);
                }
            }
            return AZ::Failure(QObject::tr("Clang not found. \n\n"
                "Make sure that the clang is installed and available from the command prompt. "
                "Refer to the <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#cmake'>O3DE requirements</a> page for more information."));
        }
        
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
